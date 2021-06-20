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
		: abi(abi), process(nullptr), injected(false)
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
			if constexpr (func.isRequired()) {
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
			if constexpr (func.isRequired()) {
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
		intptr_t offset;
		size_t size;
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

			if constexpr (func.needsWrapFunc()) {
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

			if constexpr (func.needsWrapFunc()) {
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
				a->mov(a->zax, gchandle_get_target.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);

			//	return gchandle_new(rawObj, true);
				a->mov(a->zcx, a->zax);
				a->mov(a->zdx, 1);
				a->mov(a->zax, gchandle_new.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);

			a->pop(a->zsp);
		} else {
			//	IRMonoObjectRawPtr rawObj = gchandle_get_target(unpinned);
				a->push(dword_ptr(a->zsp, 4));
				a->mov(a->zax, gchandle_get_target.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 4);

			//	return gchandle_new(rawObj, true);
				a->push(1);
				a->push(a->zax);
				a->mov(a->zax, gchandle_new.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 8);
		}

		a->ret();
	}

	if (array_addr_with_size  &&  gc_wbarrier_set_arrayref) {
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
			AsmGenGchandleGetTargetChecked(a, gchandle_get_target.getRawFuncAddress(), x64);
			a->mov(a->zbx, a->zax);

		//	IRMonoObjectPtrRaw rawVal = mono_gchandle_get_target_checked(val);
			a->mov(a->zcx, a->zdi);
			AsmGenGchandleGetTargetChecked(a, gchandle_get_target.getRawFuncAddress(), x64);
			a->mov(a->zdi, a->zax);

		//	IRMonoObjectPtrRaw* p = mono_array_addr_with_size(rawArr, sizeof(IRMonoObjectPtrRaw), idx);
			if (x64) {
				a->mov(a->zcx, a->zbx);
				a->mov(a->zdx, sizeof(IRMonoObjectPtrRaw));
				a->mov(asmjit::host::r8, a->zsi);
				a->mov(a->zax, array_addr_with_size.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zsi);
				a->push(sizeof(IRMonoObjectPtrRaw));
				a->push(a->zbx);
				a->mov(a->zax, array_addr_with_size.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 12);
			}
			a->mov(a->zsi, a->zax);

		//	mono_gc_wbarrier_set_arrayref(rawArr, p, rawVal);
			if (x64) {
				a->mov(a->zcx, a->zbx);
				a->mov(a->zdx, a->zsi);
				a->mov(asmjit::host::r8, a->zdi);
				a->mov(a->zax, gc_wbarrier_set_arrayref.getRawFuncAddress());
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zdi);
				a->push(a->zsi);
				a->push(a->zbx);
				a->mov(a->zax, gc_wbarrier_set_arrayref.getRawFuncAddress());
				a->call(a->zax);
				a->add(a->zsp, 12);
			}

		a->pop(a->zdi);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}

	std::string boilerplateCode((const char*) a->make(), a->getCodeSize());

	if (a->isLabelBound(lForeachIPCVecAdapter)) {
		rmono_foreach_ipcvec_adapter.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lForeachIPCVecAdapter)));
	}
	if (a->isLabelBound(lGchandlePin)) {
		rmono_gchandle_pin.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lGchandlePin)));
	}
	if (a->isLabelBound(lArraySetref)) {
		rmono_array_setref.rebuild(*process, static_cast<rmono_funcp>(a->getLabelOffset(lArraySetref)));
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
