/*
	Copyright 2020-2021 David "Alemarius Nexus" Lerch

	This file is part of RemoteMono.

	RemoteMono is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RemoteMono is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with RemoteMono.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../config.h"

#include "RMonoAPIBackend_Def.h"

#include <algorithm>
#include <exception>
#include <map>
#include <cstddef>
#include <cstdlib>
#include <stdint.h>
#include "../log.h"
#include "../util.h"
#include "backend/RMonoModule.h"




namespace remotemono
{


template <typename ABI>
RMonoAPIBackend<ABI>::RMonoAPIBackend(ABI* abi)
		: abi(abi), process(nullptr), injected(false), gchandleFreeBufCount(0), rawFreeBufCount(0),
		  gchandleFreeBufCountMax(REMOTEMONO_GCHANDLE_FREE_BUF_SIZE_MAX),
		  rawFreeBufCountMax(REMOTEMONO_RAW_FREE_BUF_SIZE_MAX)
{
}


template <typename ABI>
RMonoAPIBackend<ABI>::~RMonoAPIBackend()
{
}


template <typename ABI>
void RMonoAPIBackend<ABI>::injectAPI(RMonoAPI* mono, backend::RMonoProcess& process)
{
	if (injected) {
		return;
	}

	this->process = &process;

	ABI* abi = getABI();

	bool x64 = (sizeof(irmono_voidp) == 8);

	ipcVec.inject(&process);

	backend::RMonoModule* monoDll = process.getModule("mono.dll");

	if (!monoDll) {
		auto loadedModules = process.getAllModules();

		//for (auto it = loadedModules.begin() ; it != loadedModules.end() ; it++) {
		for (backend::RMonoModule* mod : loadedModules) {
			backend::RMonoModule::Export exp;
			if (mod->getExport(exp, "mono_get_root_domain")) {
				monoDll = mod;
				break;
			}
		}
	}

	if (monoDll) {
		RMonoLogInfo("Found Mono Embedded API in '%s'", monoDll->getName().c_str());
	} else {
		throw std::runtime_error("Couldn't find module containing Mono Embedded API in remote process.");
	}



	// ********** PREPARE REMOTE FUNCTIONS **********

	foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
		std::string exportName("mono_");
		exportName.append(name);
		backend::RMonoModule::Export exp;
		if (monoDll->getExport(exp, exportName.data())) {
			func.init(abi, mono, exportName, exp.procPtr);
		} else {
			RMonoLogDebug("API function not found in remote process: %s", exportName.data());
			func.initInvalid(exportName);
			if (func.isRequired()) {
				throw std::runtime_error(std::string("Required export not found in mono.dll: ").append(exportName));
			}
		}
	});

	foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
		backend::RMonoModule::Export exp;
		if (monoDll->getExport(exp, name)) {
			func.init(abi, mono, name, exp.procPtr);
		} else {
			RMonoLogDebug("API function not found in remote process: %s", name);
			func.initInvalid(name);
			if (func.isRequired()) {
				throw std::runtime_error(std::string("Required export not found in mono.dll: ").append(name));
			}
		}
	});



	// ********** COMPILE REMOTE FUNCTIONS **********

	std::string monoAPIWrapperCode;
	std::string miscAPIWrapperCode;

	struct APIWrapperInfo
	{
		asmjit::Label startLabel;
		asmjit::Label endLabel;
		intptr_t offset = 0;
		size_t size = 0;
	};

	std::map<std::string, APIWrapperInfo> monoAPIWrapperInfo;
	std::map<std::string, APIWrapperInfo> miscAPIWrapperInfo;

	{
		auto asmPtr = process.createAssembler();
		auto& a = *asmPtr;

		foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
			if (func) {
				APIWrapperInfo info;

				info.startLabel = func.compile(a);
				info.endLabel = a->newLabel();
				a->bind(info.endLabel);

				monoAPIWrapperInfo.insert(std::pair<std::string, APIWrapperInfo>(name, info));
			}
		});

		void* code = a->make();

		if (!code  &&  a->getCodeSize() != 0) {
			RMonoLogError("Error assembling MonoAPI wrapper code: %d", (int) a->getError());
			throw RMonoException("Error assembling MonoAPI wrapper code.");
		}

		monoAPIWrapperCode = std::string((const char*) code, a->getCodeSize());

		for (auto& p : monoAPIWrapperInfo) {
			p.second.offset = a->getLabelOffset(p.second.startLabel);
			p.second.size = a->getLabelOffset(p.second.endLabel) - p.second.offset;
		}
	}

	{
		auto asmPtr = process.createAssembler();
		auto& a = *asmPtr;

		foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
			if (func) {
				APIWrapperInfo info;

				info.startLabel = func.compile(a);
				info.endLabel = a->newLabel();
				a->bind(info.endLabel);

				miscAPIWrapperInfo.insert(std::pair<std::string, APIWrapperInfo>(name, info));
			}
		});

		void* code = a->make();

		if (!code  &&  a->getCodeSize() != 0) {
			RMonoLogError("Error assembling MiscAPI wrapper code: %d", (int) a->getError());
			throw RMonoException("Error assembling MiscAPI wrapper code.");
		}

		miscAPIWrapperCode = std::string((const char*) code, a->getCodeSize());

		for (auto& p : miscAPIWrapperInfo) {
			p.second.offset = a->getLabelOffset(p.second.startLabel);
			p.second.size = a->getLabelOffset(p.second.endLabel) - p.second.offset;
		}
	}



	// ********** ASSEMBLE BOILERPLATE CODE **********

	std::string boilerplateCode = assembleBoilerplateCode();



	// ********** DUMP REMOTE FUNCTION SIGNATURES **********

	if (RMonoLogger::getInstance().isLogLevelActive(RMonoLogger::LOG_LEVEL_VERBOSE)) {
		foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
			if (func) {
				func.debugDumpSignatures();
			}
		});
		foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
			if (func) {
				func.debugDumpSignatures();
			}
		});
	}



	// ********** ALLOCATE REMOTE DATA BLOCK **********

	this->remDataBlock = std::move(backend::RMonoMemBlock::alloc(&process,
			monoAPIWrapperCode.size() + boilerplateCode.size(), PAGE_EXECUTE_READWRITE));

	size_t monoAPIWrapperCodeOffs = 0;
	size_t miscAPIWrapperCodeOffs = monoAPIWrapperCodeOffs + monoAPIWrapperCode.size();
	size_t boilerplateCodeOffs = miscAPIWrapperCodeOffs + miscAPIWrapperCode.size();

	remDataBlock.write(monoAPIWrapperCodeOffs, monoAPIWrapperCode.size(), monoAPIWrapperCode.data());
	remDataBlock.write(miscAPIWrapperCodeOffs, miscAPIWrapperCode.size(), miscAPIWrapperCode.data());
	remDataBlock.write(boilerplateCodeOffs, boilerplateCode.size(), boilerplateCode.data());

	RMonoLogDebug("Remote Data Block: %llu bytes", (long long unsigned) remDataBlock.size());



	// ********** LINK REMOTE FUNCTIONS **********

	foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
		if (func) {
			const APIWrapperInfo& info = monoAPIWrapperInfo[name];

			func.link(*remDataBlock + monoAPIWrapperCodeOffs + info.offset);

			if (func.needsWrapFunc()) {
				RMonoLogDebug("Wrapper for '%s' is at %llX (size: %llu)", func.getName().data(),
						(long long unsigned) (*remDataBlock + monoAPIWrapperCodeOffs + info.offset), (long long unsigned) info.size);
			} else {
				RMonoLogVerbose("No wrapper required for '%s'", func.getName().data());
			}
		}
	});

	foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
		if (func) {
			const APIWrapperInfo& info = miscAPIWrapperInfo[name];

			func.link(*remDataBlock + miscAPIWrapperCodeOffs + info.offset);

			if (func.needsWrapFunc()) {
				RMonoLogDebug("Wrapper for '%s' is at %llX (size: %llu)", func.getName().data(),
						(long long unsigned) (*remDataBlock + miscAPIWrapperCodeOffs + info.offset), (long long unsigned) info.size);
			} else {
				RMonoLogVerbose("No wrapper required for '%s'", func.getName().data());
			}
		}
	});

	foreachAPI(*dynamic_cast<BoilerplateAPI*>(this), [&](const char* name, auto& func) {
		if (func) {
			func.rebuild(process, *remDataBlock + boilerplateCodeOffs + func.getAddress());
		}
	});



	// ********** COLLECT VALID FUNCTIONS **********

	foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
		if (func) {
			validAPIFuncNames.insert(func.getName());
		}
	});
	foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
		if (func) {
			validAPIFuncNames.insert(func.getName());
		}
	});

	injected = true;
}


template <typename ABI>
void RMonoAPIBackend<ABI>::uninjectAPI()
{
	if (!injected) {
		return;
	}

	flushGchandleFreeBuffer();
	flushRawFreeBuffer();

	remDataBlock.reset();

	ipcVec.vectorFree(ipcVecPtr);

	foreachAPI(*dynamic_cast<BoilerplateAPI*>(this), [&](const char* name, auto& func) {
		func.reset();
	});
	foreachAPI(*dynamic_cast<MiscAPI*>(this), [&](const char* name, auto& func) {
		func.reset();
	});
	foreachAPI(*dynamic_cast<MonoAPI*>(this), [&](const char* name, auto& func) {
		func.reset();
	});

	ipcVec.uninject();

	injected = false;
}


template <typename ABI>
void RMonoAPIBackend<ABI>::setGchandleFreeBufferMaxCount(uint32_t maxCount)
{
	maxCount = (std::max)(maxCount, (uint32_t) 1);
	maxCount = (std::min)(maxCount, (uint32_t) REMOTEMONO_GCHANDLE_FREE_BUF_SIZE_MAX);

	if (maxCount >= gchandleFreeBufCountMax) {
		flushGchandleFreeBuffer();
	}

	gchandleFreeBufCountMax = maxCount;

	assert(gchandleFreeBufCountMax <= REMOTEMONO_GCHANDLE_FREE_BUF_SIZE_MAX);
}


template <typename ABI>
void RMonoAPIBackend<ABI>::setRawFreeBufferMaxCount(uint32_t maxCount)
{
	maxCount = (std::max)(maxCount, (uint32_t) 1);
	maxCount = (std::min)(maxCount, (uint32_t) REMOTEMONO_RAW_FREE_BUF_SIZE_MAX);

	if (maxCount >= rawFreeBufCountMax) {
		flushRawFreeBuffer();
	}

	rawFreeBufCountMax = maxCount;

	assert(rawFreeBufCountMax <= REMOTEMONO_RAW_FREE_BUF_SIZE_MAX);
}


template <typename ABI>
void RMonoAPIBackend<ABI>::setFreeBufferMaxCount(uint32_t maxCount)
{
	setGchandleFreeBufferMaxCount(maxCount);
	setRawFreeBufferMaxCount(maxCount);
}


template <typename ABI>
void RMonoAPIBackend<ABI>::freeLaterGchandle(irmono_gchandle handle)
{
	assert(gchandleFreeBufCountMax >= 1);
	assert(gchandleFreeBufCount < gchandleFreeBufCountMax);

	gchandleFreeBuf[gchandleFreeBufCount++] = handle;

	if (gchandleFreeBufCount == gchandleFreeBufCountMax) {
		flushGchandleFreeBuffer();
	}
}


template <typename ABI>
void RMonoAPIBackend<ABI>::freeLaterRaw(irmono_voidp ptr)
{
	assert(rawFreeBufCountMax >= 1);
	assert(rawFreeBufCount < rawFreeBufCountMax);

	rawFreeBuf[rawFreeBufCount++] = ptr;

	if (rawFreeBufCount == rawFreeBufCountMax) {
		flushRawFreeBuffer();
	}
}


template <typename ABI>
void RMonoAPIBackend<ABI>::flushGchandleFreeBuffer()
{
	if (gchandleFreeBufCount == 0) {
		return;
	} else if (gchandleFreeBufCount == 1) {
		this->gchandle_free(gchandleFreeBuf[0]);
		gchandleFreeBufCount = 0;
		return;
	}

	backend::RMonoMemBlock arr = std::move(backend::RMonoMemBlock::alloc(process, gchandleFreeBufCount*sizeof(irmono_gchandle)));
	arr.write(0, gchandleFreeBufCount*sizeof(irmono_gchandle), gchandleFreeBuf);

	this->rmono_gchandle_free_multi (
			abi->p2i_rmono_voidp(*arr),
			abi->p2i_rmono_voidp(*arr + gchandleFreeBufCount*sizeof(irmono_gchandle))
			);

	gchandleFreeBufCount = 0;
}


template <typename ABI>
void RMonoAPIBackend<ABI>::flushRawFreeBuffer()
{
	if (rawFreeBufCount == 0) {
		return;
	} else if (rawFreeBufCount == 1) {
		if (this->free) {
			this->free(rawFreeBuf[0]);
		} else if (this->g_free) {
			this->g_free(rawFreeBuf[0]);
		} else {
			throw RMonoException("No remote free() function found for flushRawFreeBuffer()");
		}
		rawFreeBufCount = 0;
		return;
	}

	backend::RMonoMemBlock arr = std::move(backend::RMonoMemBlock::alloc(process, rawFreeBufCount*sizeof(irmono_voidp)));
	arr.write(0, rawFreeBufCount*sizeof(irmono_voidp), rawFreeBuf);

	this->rmono_raw_free_multi (
			abi->p2i_rmono_voidp(*arr),
			abi->p2i_rmono_voidp(*arr + rawFreeBufCount*sizeof(irmono_voidp))
			);

	rawFreeBufCount = 0;
}


template <typename ABI>
void RMonoAPIBackend<ABI>::flushFreeBuffers()
{
	flushGchandleFreeBuffer();
	flushRawFreeBuffer();
}


template <typename ABI>
std::string RMonoAPIBackend<ABI>::assembleBoilerplateCode()
{
	using namespace asmjit;
	using namespace asmjit::host;

	ipcVecPtr = ipcVec.vectorNew();

	bool x64 = (sizeof(irmono_voidp) == 8);

	RMonoLogVerbose("Assembling BoilerplateAPI functions for %s", x64 ? "x64" : "x86");

	auto asmPtr = process->createAssembler();
	auto& a = *asmPtr;

	asmjit::Label lForeachIPCVecAdapter = a->newLabel();
	asmjit::Label lGchandlePin = a->newLabel();
	asmjit::Label lArraySetref = a->newLabel();
	asmjit::Label lArraySlice = a->newLabel();
	asmjit::Label lGchandleFreeMulti = a->newLabel();
	asmjit::Label lRawFreeMulti = a->newLabel();

	{
		// __cdecl void rmono_foreach_ipcvec_adapter(irmono_voidp elem, irmono_voidp vec);
		a->bind(lForeachIPCVecAdapter);

		//	IPCVector_VectorAdd(vec, elem);
			if (x64) {
				a->push(a->zsp); // Aligns stack to 16 bytes
				a->xchg(a->zcx, a->zdx);
				a->mov(a->zax, (uint64_t) ipcVec.getAPI().vectorAdd);
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
				a->pop(a->zsp);
			} else {
				a->mov(a->zcx, ptr(a->zsp, 8));
				a->mov(a->zdx, ptr(a->zsp, 4));
				a->mov(a->zax, (uint32_t) (uintptr_t) ipcVec.getAPI().vectorAdd);
				a->call(a->zax);
			}

		a->ret();
	}

	{
		// __cdecl irmono_gchandle rmono_gchandle_pin(irmono_gchandle unpinned);
		a->bind(lGchandlePin);

		if (x64) {
			a->push(a->zsp); // Aligns stack to 16 bytes

			//	IRMonoObjectRawPtr rawObj = gchandle_get_target(unpinned);
				a->mov(a->zax, this->gchandle_get_target.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);

			//	return gchandle_new(rawObj, true);
				a->mov(a->zcx, a->zax);
				a->mov(a->zdx, 1);
				a->mov(a->zax, this->gchandle_new.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);

			a->pop(a->zsp);
		} else {
			//	IRMonoObjectRawPtr rawObj = gchandle_get_target(unpinned);
				a->push(dword_ptr(a->zsp, 4));
				a->mov(a->zax, this->gchandle_get_target.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 4);

			//	return gchandle_new(rawObj, true);
				a->push(1);
				a->push(a->zax);
				a->mov(a->zax, this->gchandle_new.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 8);
		}

		a->ret();
	}

	if (this->array_addr_with_size  &&  this->gc_wbarrier_set_arrayref) {
		// __cdecl void rmono_array_setref(irmono_gchandle arr, irmono_uintptr_t idx, irmono_gchandle val);
		a->bind(lArraySetref);
		a->push(a->zbx);
		a->push(a->zsi);
		a->push(a->zdi);

		if (x64) {
			a->mov(a->zsi, a->zdx);
			a->mov(a->zdi, r8);
		} else {
			a->mov(a->zbx, ptr(a->zsp, 16));
			a->mov(a->zsi, ptr(a->zsp, 20));
			a->mov(a->zdi, ptr(a->zsp, 24));
			a->mov(a->zcx, a->zbx);
		}

		//	IMonoArrayPtrRaw rawArr = mono_gchandle_get_target_checked(arr);
			AsmGenGchandleGetTargetChecked(a, this->gchandle_get_target.getRawFuncAddress(), x64);
			a->mov(a->zbx, a->zax);

		//	IRMonoObjectPtrRaw rawVal = mono_gchandle_get_target_checked(val);
			a->mov(a->zcx, a->zdi);
			AsmGenGchandleGetTargetChecked(a, this->gchandle_get_target.getRawFuncAddress(), x64);
			a->mov(a->zdi, a->zax);

		//	IRMonoObjectPtrRaw* p = mono_array_addr_with_size(rawArr, sizeof(IRMonoObjectPtrRaw), idx);
			if (x64) {
				a->mov(a->zcx, a->zbx);
				a->mov(a->zdx, sizeof(IRMonoObjectPtrRaw));
				a->mov(asmjit::host::r8, a->zsi);
				a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zsi);
				a->push(sizeof(IRMonoObjectPtrRaw));
				a->push(a->zbx);
				a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 12);
			}
			a->mov(a->zsi, a->zax);

		//	mono_gc_wbarrier_set_arrayref(rawArr, p, rawVal);
			if (x64) {
				a->mov(a->zcx, a->zbx);
				a->mov(a->zdx, a->zsi);
				a->mov(asmjit::host::r8, a->zdi);
				a->mov(a->zax, this->gc_wbarrier_set_arrayref.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zdi);
				a->push(a->zsi);
				a->push(a->zbx);
				a->mov(a->zax, this->gc_wbarrier_set_arrayref.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 12);
			}

		a->pop(a->zdi);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}

	{
		Label lRawType = a->newLabel();
		Label lTypeEnd = a->newLabel();
		Label lRawTypeWhileStart = a->newLabel();
		Label lRawTypeWhileEnd = a->newLabel();
		Label lObjTypeWhileStart = a->newLabel();
		Label lObjTypeWhileEnd = a->newLabel();
		Label lRawTypeMemcpyStart = a->newLabel();
		Label lRawTypeMemcpyEnd = a->newLabel();

		// __cdecl irmono_uintptr_t rmono_array_slice (
		//			irmono_voidp outBuf,
		//			irmono_gchandle arr,
		//			irmono_uintptr_t start, irmono_uintptr_t end,
		//			uint32_t elemSize)

		a->bind(lArraySlice);
		a->push(a->zbx);
		a->push(a->zsi);
		a->push(a->zdi);
		a->push(a->zbp);
		a->push(a->zsp); // Aligns stack to 16 bytes

		if (x64) {
			a->mov(a->zbx, a->zcx); // outBuf
			a->mov(a->zsi, a->zdx); // arr / rawArr
			a->mov(a->zdi, r8); // start
			a->mov(a->zbp, r9); // end
		} else {
			a->mov(a->zbx, ptr(a->zsp, 24)); // outBuf
			a->mov(a->zsi, ptr(a->zsp, 28)); // arr / rawArr
			a->mov(a->zdi, ptr(a->zsp, 32)); // start
			a->mov(a->zbp, ptr(a->zsp, 36)); // end
		}

		//	IRMonoArrayHandleRaw rawArr = mono_gchandle_get_target_checked(arr);
			a->mov(a->zcx, a->zsi);
			AsmGenGchandleGetTargetChecked(a, this->gchandle_get_target.getRawFuncAddress(), x64);
			a->mov(a->zsi, a->zax);

			if (x64) {
				a->mov(a->zdx, dword_ptr(a->zsp, 80)); // elemSize
			} else {
				a->mov(a->zdx, ptr(a->zsp, 40)); // elemSize
			}


		//	if (elemSize == 0) {
			a->test(edx, edx);
			a->jnz(lRawType);

		//		while (start < end) {
				a->bind(lObjTypeWhileStart);
				a->cmp(a->zdi, a->zbp);
				a->jae(lObjTypeWhileEnd);

		//			void* elemPtr = mono_array_addr_with_size(rawArr, sizeof(IRMonoObjectPtrRaw), start);
					if (x64) {
						a->mov(a->zcx, a->zsi);
						a->mov(a->zdx, sizeof(IRMonoObjectPtrRaw));
						a->mov(r8, a->zdi);
						a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
						a->sub(a->zsp, 32);
						a->call(a->zax);
						a->add(a->zsp, 32);
					} else {
						a->push(a->zdi);
						a->push(sizeof(IRMonoObjectPtrRaw));
						a->push(a->zsi);
						a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
						a->call(a->zax);
						a->add(a->zsp, 12);
					}

		//			*outBuf = mono_gchandle_new_checked(*((IRMonoObjectPtrRaw*) elemPtr));
					a->mov(a->zcx, ptr(a->zax));
					AsmGenGchandleNewChecked(a, this->gchandle_new.getRawFuncAddress(), x64);
					a->mov(dword_ptr(a->zbx), eax);

		//			outBuf += sizeof(irmono_gchandle);
		//			start++;
					a->add(a->zbx, sizeof(irmono_gchandle));
					a->inc(a->zdi);

				a->jmp(lObjTypeWhileStart);
		//		}
				a->bind(lObjTypeWhileEnd);

			a->jmp(lTypeEnd);
		//	} else {
			a->bind(lRawType);

		//		while (start < end) {
				a->bind(lRawTypeWhileStart);
				a->cmp(a->zdi, a->zbp);
				a->jae(lRawTypeWhileEnd);

		//			void* elemPtr = mono_array_addr_with_size(rawArr, elemSize, start);
					if (x64) {
						a->mov(a->zcx, a->zsi);
						a->mov(a->zdx, dword_ptr(a->zsp, 80));
						a->mov(r8, a->zdi);
						a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
						a->sub(a->zsp, 32);
						a->call(a->zax);
						a->add(a->zsp, 32);
						a->mov(a->zdx, dword_ptr(a->zsp, 80)); // Restore elemSize
					} else {
						a->push(a->zdi);
						a->push(dword_ptr(a->zsp, 44));
						a->push(a->zsi);
						a->mov(a->zax, this->array_addr_with_size.getRawFuncAddress());
						a->call(a->zax);
						a->add(a->zsp, 12);
						a->mov(a->zdx, ptr(a->zsp, 40)); // Restore elemSize
					}

		//			while (elemSize != 0) {
					a->bind(lRawTypeMemcpyStart);
					a->test(a->zdx, a->zdx);
					a->jz(lRawTypeMemcpyEnd);

		//				*((char*) outBuf) = ((char*) elemPtr);
						a->mov(cl, byte_ptr(a->zax));
						a->mov(byte_ptr(a->zbx), cl);

		//				outBuf++;
		//				elemPtr++;
		//				elemSize--;
						a->inc(a->zbx);
						a->inc(a->zax);
						a->dec(a->zdx);

					a->jmp(lRawTypeMemcpyStart);
		//			}
					a->bind(lRawTypeMemcpyEnd);

		//			start++;
					a->inc(a->zdi);

				a->jmp(lRawTypeWhileStart);
		//		}
				a->bind(lRawTypeWhileEnd);

		//	}
			a->bind(lTypeEnd);

		//	return end;
			a->mov(a->zax, a->zbp);

		a->pop(a->zsp);
		a->pop(a->zbp);
		a->pop(a->zdi);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}

	{
		Label lLoopStart = a->newLabel();
		Label lLoopEnd = a->newLabel();

		// __cdecl void rmono_gchandle_free_multi(irmono_voidp beg, irmono_voidp end);
		a->bind(lGchandleFreeMulti);
		a->push(a->zbx);
		a->push(a->zsi);

		if (x64) {
			a->mov(a->zbx, a->zcx); // beg
			a->mov(a->zsi, a->zdx); // end
		} else {
			a->mov(a->zbx, ptr(a->zsp, 12)); // beg
			a->mov(a->zsi, ptr(a->zsp, 16)); // end
		}

		//	while (beg != end) {
			a->bind(lLoopStart);
			a->cmp(a->zbx, a->zsi);
			a->je(lLoopEnd);

				if (x64) {
					//	gchandle_free(*((irmono_gchandle*) beg));
						a->mov(ecx, ptr(a->zbx));
						a->mov(a->zax, this->gchandle_free.getRawFuncAddress());
						a->sub(a->zsp, 32);
						a->call(a->zax);
						a->add(a->zsp, 32);
				} else {
					//	gchandle_free(*((irmono_gchandle*) beg));
						a->push(dword_ptr(a->zbx));
						a->mov(a->zax, this->gchandle_free.getRawFuncAddress());
						a->call(a->zax);
						a->add(a->zsp, 4);
				}

		//		beg += sizeof(irmono_gchandle);
				a->add(a->zbx, sizeof(irmono_gchandle));
				a->jmp(lLoopStart);

		//	}
			a->bind(lLoopEnd);

		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}

	{
		Label lLoopStart = a->newLabel();
		Label lLoopEnd = a->newLabel();

		// __cdecl void rmono_raw_free_multi(irmono_voidp beg, irmono_voidp end);
		a->bind(lRawFreeMulti);
		a->push(a->zbx);
		a->push(a->zsi);

		if (x64) {
			a->mov(a->zbx, a->zcx); // beg
			a->mov(a->zsi, a->zdx); // end
		} else {
			a->mov(a->zbx, ptr(a->zsp, 12)); // beg
			a->mov(a->zsi, ptr(a->zsp, 16)); // end
		}

		//	while (beg != end) {
			a->bind(lLoopStart);
			a->cmp(a->zbx, a->zsi);
			a->je(lLoopEnd);

				if (x64) {
					//	free(*((irmono_voidp*) beg));
						a->mov(a->zcx, ptr(a->zbx));
						if (this->free) {
							a->mov(a->zax, this->free.getRawFuncAddress());
						} else if (this->g_free) {
							a->mov(a->zax, this->g_free.getRawFuncAddress());
						} else {
							throw RMonoException("No remote free() function found for rmono_raw_free_multi()");
						}
						a->sub(a->zsp, 32);
						a->call(a->zax);
						a->add(a->zsp, 32);
				} else {
					//	free(*((irmono_voidp*) beg));
						a->push(dword_ptr(a->zbx));
						if (this->free) {
							a->mov(a->zax, this->free.getRawFuncAddress());
						} else if (this->g_free) {
							a->mov(a->zax, this->g_free.getRawFuncAddress());
						} else {
							throw RMonoException("No remote free() function found for rmono_raw_free_multi()");
						}
						a->call(a->zax);
						a->add(a->zsp, 4);
				}

		//		beg += sizeof(irmono_voidp);
				a->add(a->zbx, sizeof(irmono_voidp));
				a->jmp(lLoopStart);

		//	}
			a->bind(lLoopEnd);

		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}

	std::string boilerplateCode((const char*) a->make(), a->getCodeSize());

	if (a->isLabelBound(lForeachIPCVecAdapter)) {
		this->rmono_foreach_ipcvec_adapter.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lForeachIPCVecAdapter)));
	}
	if (a->isLabelBound(lGchandlePin)) {
		this->rmono_gchandle_pin.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lGchandlePin)));
	}
	if (a->isLabelBound(lArraySetref)) {
		this->rmono_array_setref.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lArraySetref)));
	}
	if (a->isLabelBound(lArraySlice)) {
		this->rmono_array_slice.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lArraySlice)));
	}
	if (a->isLabelBound(lGchandleFreeMulti)) {
		this->rmono_gchandle_free_multi.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lGchandleFreeMulti)));
	}
	if (a->isLabelBound(lRawFreeMulti)) {
		this->rmono_raw_free_multi.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lRawFreeMulti)));
	}

	return boilerplateCode;
}







template <typename ABI>
template <typename FuncT, typename MainT, typename... PartialT>
void RMonoAPIBackend<ABI>::foreachAPIPacked(MainT& main, FuncT func, PackHelper<PartialT...>)
{
	foreachAPIRecurse(func, *dynamic_cast<PartialT*>(&main)...);
}


template <typename ABI>
template <typename FuncT, typename FirstT, typename... OtherT>
void RMonoAPIBackend<ABI>::foreachAPIRecurse(FuncT func, FirstT& part, OtherT&... other)
{
	visit_struct::for_each(part, func);

	if constexpr (sizeof...(other) != 0) {
		foreachAPIRecurse(func, other...);
	}
}


template <typename ABI>
template <typename MainAPIT, typename FuncT>
void RMonoAPIBackend<ABI>::foreachAPI(MainAPIT& api, FuncT func)
{
	foreachAPIPacked(api, func, typename MainAPIT::internal_api_parts());
}


}
