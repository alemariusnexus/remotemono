/*
	Copyright 2020 David "Alemarius Nexus" Lerch

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

#include "config.h"

#include "IPCVector.h"

#include <cstring>
#include "util.h"
#include "log.h"


using namespace blackbone;




namespace remotemono
{


template <typename ElemT, typename IntPtrT>
IPCVector<ElemT, IntPtrT>::IPCVector()
		: process(nullptr), injected(false), remAPI(nullptr), code(nullptr)
{
}


template <typename ElemT, typename IntPtrT>
IPCVector<ElemT, IntPtrT>::~IPCVector()
{
	uninject();
}


template <typename ElemT, typename IntPtrT>
typename IPCVector<ElemT, IntPtrT>::VectorPtr IPCVector<ElemT, IntPtrT>::vectorNew(uint32_t cap)
{
	if (process) {
		return *remAPI->vectorNew.Call({cap}, process->remote().getWorker());
	} else {
		return localApi.vectorNew(cap);
	}
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::vectorFree(VectorPtr v)
{
	if (process) {
		remAPI->vectorFree.Call({v}, process->remote().getWorker());
	} else {
		localApi.vectorFree(v);
	}
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::vectorAdd(VectorPtr v, ElemT elem)
{
	if (process) {
		remAPI->vectorAdd.Call({v, elem}, process->remote().getWorker());
	} else {
		localApi.vectorAdd(v, elem);
	}
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::vectorClear(VectorPtr v)
{
	if (process) {
		remAPI->vectorClear.Call({v}, process->remote().getWorker());
	} else {
		localApi.vectorClear(v);
	}
}


template <typename ElemT, typename IntPtrT>
uint32_t IPCVector<ElemT, IntPtrT>::vectorLength(VectorPtr v)
{
	if (process) {
		return *remAPI->vectorLength.Call({v}, process->remote().getWorker());
	} else {
		return localApi.vectorLength(v);
	}
}


template <typename ElemT, typename IntPtrT>
uint32_t IPCVector<ElemT, IntPtrT>::vectorCapacity(VectorPtr v)
{
	if (process) {
		return *remAPI->vectorCapacity.Call({v}, process->remote().getWorker());
	} else {
		return localApi.vectorCapacity(v);
	}
}


template <typename ElemT, typename IntPtrT>
typename IPCVector<ElemT, IntPtrT>::DataPtr IPCVector<ElemT, IntPtrT>::vectorData(VectorPtr v)
{
	if (process) {
		return *remAPI->vectorData.Call({v}, process->remote().getWorker());
	} else {
		return localApi.vectorData(v);
	}
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::vectorGrow(VectorPtr v, uint32_t cap)
{
	if (process) {
		remAPI->vectorGrow.Call({v, cap}, process->remote().getWorker());
	} else {
		localApi.vectorGrow(v, cap);
	}
}


template <typename ElemT, typename IntPtrT>
typename IPCVector<ElemT, IntPtrT>::VectorPtr IPCVector<ElemT, IntPtrT>::create(const std::vector<ElemT>& data)
{
	VectorPtr v = vectorNew((uint32_t) data.size());
	for (const ElemT& e : data) {
		vectorAdd(v, e);
	}
	return v;
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::read(VectorPtr v, std::vector<ElemT>& out)
{
	uint32_t len = vectorLength(v);
	out.resize(len);

	if (process) {
		ProcessMemory& mem = process->memory();
		mem.Read((ptr_t) vectorData(v), len*sizeof(ElemT), out.data());
	} else {
		memcpy(out.data(), (void*) (uintptr_t) vectorData(v), len*sizeof(ElemT));
	}
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::inject(Process* process)
{
	using namespace asmjit;
	using namespace asmjit::host;


	if (injected) {
		return;
	}

	this->process = process;

	ProcessModules* modules = process ? &process->modules() : nullptr;
	ProcessMemory* mem = process ? &process->memory() : nullptr;

	bool x64 = (sizeof(IntPtrT) == 8);

	RMonoLogVerbose("Assembling IPCVector functions for %s", x64 ? "x64" : "x86");

	auto asmPtr = AsmFactory::GetAssembler(!x64);
	auto& a = *asmPtr;


	Label lVectorGrow = a->newLabel();
	Label lVectorNew = a->newLabel();
	Label lVectorFree = a->newLabel();
	Label lVectorAdd = a->newLabel();
	Label lVectorClear = a->newLabel();
	Label lVectorLength = a->newLabel();
	Label lVectorCapacity = a->newLabel();
	Label lVectorData = a->newLabel();

	ptr_t pHeapAlloc;
	ptr_t pHeapReAlloc;
	ptr_t pHeapFree;
	ptr_t pGetProcessHeap;

	if (process) {
		auto k32 = modules->GetModule(L"kernel32.dll");

		pHeapAlloc = modules->GetExport(k32, "HeapAlloc")->procAddress;
		pHeapReAlloc = modules->GetExport(k32, "HeapReAlloc")->procAddress;
		pHeapFree = modules->GetExport(k32, "HeapFree")->procAddress;
		pGetProcessHeap = modules->GetExport(k32, "GetProcessHeap")->procAddress;
	} else {
		pHeapAlloc = (ptr_t) &HeapAlloc;
		pHeapReAlloc = (ptr_t) &HeapReAlloc;
		pHeapFree = (ptr_t) &HeapFree;
		pGetProcessHeap = (ptr_t) &GetProcessHeap;
	}


	// IMPORTANT: Make sure that each function's prolog aligns the stack to 16 bytes on x64 if it calls other functions.
	// It's off by 8 bytes at prolog start because of the return address pushed by `call`.

	// TODO: In 64-bit mode, do we actually need to call these WIN32 API functions using x64 calling conventions, or are
	// they still using 32-bit stdcall?


	// __fastcall void VectorGrow(VectorPtr v, uint32_t cap);
	{
		Label lVectorGrowRet = a->newLabel();
		Label lVectorGrowPow2Loop = a->newLabel();
		Label lVectorGrowPow2LoopEnd = a->newLabel();

		a->bind(lVectorGrow);
		a->push(a->zbx);
		a->push(a->zsi);
		a->push(a->zdi);
		a->mov(a->zbx, a->zcx);
		a->mov(a->zsi, a->zdx);

		// if (cap <= v->cap)
		// 	return;
		a->sub(edx, ptr(a->zbx, offsetof(Vector, cap)));
		a->jbe(lVectorGrowRet);

		// v->cap = 16;
		a->mov(a->zcx, 16);

		// while (v->cap < cap) {
		// 	v->cap <<= 1;
		// }
		a->bind(lVectorGrowPow2Loop);
		a->mov(a->zdx, a->zcx);
		a->sub(a->zdx, a->zsi);
		a->jae(lVectorGrowPow2LoopEnd);
		a->shl(a->zcx, 1);
		a->jmp(lVectorGrowPow2Loop);
		a->bind(lVectorGrowPow2LoopEnd);
		a->mov(a->zsi, a->zcx);
		a->mov(ptr(a->zbx, offsetof(Vector, cap)), ecx);

		// HANDLE heap = GetProcessHeap();
		if (x64) {
			a->mov(a->zax, pGetProcessHeap);
			a->sub(a->zsp, 32);
			a->call(a->zax);
			a->add(a->zsp, 32);
		} else {
			a->mov(a->zax, pGetProcessHeap);
			a->call(a->zax);
		}
		a->mov(a->zdi, a->zax);

		// v->data = (DataPtr) HeapReAlloc(heap, 0, v->data, v->cap*sizeof(ElemT));
		a->shl(a->zsi, static_ilog2(sizeof(ElemT)));
		a.GenCall(pHeapReAlloc, {a->zdi, 0, ptr(a->zbx, offsetof(Vector, data), a->zbx.getSize()), a->zsi}, cc_stdcall);
		a->mov(ptr(a->zbx, offsetof(Vector, data)), a->zax);

		a->bind(lVectorGrowRet);
		a->pop(a->zdi);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}


	// __fastcall VectorPtr VectorNew(uint32_t cap);
	{
		a->bind(lVectorNew);
		a->push(a->zbx);
		a->push(a->zsi);
		a->push(a->zdi);
		a->mov(a->zdi, a->zcx);

		// HANDLE heap = GetProcessHeap();
		if (x64) {
			a->mov(a->zax, pGetProcessHeap);
			a->sub(a->zsp, 32);
			a->call(a->zax);
			a->add(a->zsp, 32);
		} else {
			a->mov(a->zax, pGetProcessHeap);
			a->call(a->zax);
		}
		a->mov(a->zsi, a->zax);

		// VectorPtr v = (VectorPtr) HeapAlloc(heap, 0, sizeof(Vector));
		a.GenCall(pHeapAlloc, {a->zsi, 0, sizeof(Vector)}, cc_stdcall);
		a->mov(a->zbx, a->zax);

		// v->len = 0;
		// v->cap = cap;
		a->xor_(ecx, ecx);
		a->mov(ptr(a->zbx, offsetof(Vector, len)), ecx);
		a->mov(ptr(a->zbx, offsetof(Vector, cap)), edi);

		// v->data = (DataPtr) HeapAlloc(heap, 0, cap * sizeof(ElemT));
		a->shl(a->zdi, static_ilog2(sizeof(ElemT)));
		a.GenCall(pHeapAlloc, {a->zsi, 0, a->zdi}, cc_stdcall);
		a->mov(ptr(a->zbx, offsetof(Vector, data)), a->zax);

		// return v;
		a->mov(a->zax, a->zbx);
		a->pop(a->zdi);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}


	// __fastcall void VectorFree(VectorPtr v);
	{
		a->bind(lVectorFree);
		a->push(a->zbx);
		a->push(a->zsi);
		a->sub(a->zsp, 8); // Align rsp to 16 bytes
		a->mov(a->zbx, a->zcx);

		// HANDLE heap = GetProcessHeap();
		if (x64) {
			a->mov(a->zax, pGetProcessHeap);
			a->sub(a->zsp, 32);
			a->call(a->zax);
			a->add(a->zsp, 32);
		} else {
			a->mov(a->zax, pGetProcessHeap);
			a->call(a->zax);
		}
		a->mov(a->zsi, a->zax);

		// HeapFree(heap, 0, v->data);
		a.GenCall(pHeapFree, {a->zsi, 0, ptr(a->zbx, offsetof(Vector, data), a->zbx.getSize())}, cc_stdcall);

		// HeapFree(heap, 0, v);
		a.GenCall(pHeapFree, {a->zsi, 0, a->zbx}, cc_stdcall);

		a->add(a->zsp, 8);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}


	// __fastcall void VectorAdd(VectorPtr v, ElemT elem);
	{
		a->bind(lVectorAdd);
		a->push(a->zbx);
		a->push(a->zsi);
		a->sub(a->zsp, 8); // Align rsp to 16 bytes
		a->mov(a->zbx, a->zcx);
		a->mov(a->zsi, a->zdx);

		// VectorGrow(v, v->len+1);
		a->mov(edx, ptr(a->zcx, offsetof(Vector, len)));
		a->inc(a->zdx);
		if (x64) {
			a->sub(a->zsp, 32);
			a->call(lVectorGrow);
			a->add(a->zsp, 32);
		} else {
			a->call(lVectorGrow);
		}

		// v->data[v->len] = data
		a->mov(ecx, ptr(a->zbx, offsetof(Vector, len)));
		a->mov(a->zax, ptr(a->zbx, offsetof(Vector, data)));
		a->mov(ptr(a->zax, a->zcx, static_ilog2(sizeof(ElemT))), a->zsi);

		// v->len++
		a->inc(ptr(a->zbx, offsetof(Vector, len)));

		a->add(a->zsp, 8);
		a->pop(a->zsi);
		a->pop(a->zbx);
		a->ret();
	}


	// __fastcall void VectorClear(VectorPtr v);
	{
		a->bind(lVectorClear);
		a->mov(dword_ptr(a->zcx, offsetof(Vector, len)), 0);
		a->ret();
	}


	// __fastcall uint32_t VectorLength(VectorPtr v);
	{
		a->bind(lVectorLength);
		a->mov(eax, ptr(a->zcx, offsetof(Vector, len)));
		a->ret();
	}


	// __fastcall uint32_t VectorCapacity(VectorPtr v);
	{
		a->bind(lVectorCapacity);
		a->mov(eax, ptr(a->zcx, offsetof(Vector, cap)));
		a->ret();
	}


	// __fastcall DataPtr VectorData(VectorPtr v);
	{
		a->bind(lVectorData);
		a->mov(a->zax, ptr(a->zcx, offsetof(Vector, data)));
		a->ret();
	}


	ptr_t codeBaseAddr;

	if (process) {
		remoteCode = std::move(mem->Allocate(a->getCodeSize()).result());

		code = malloc(a->getCodeSize());
		a->relocCode(code);

		mem->Write(remoteCode.ptr(), a->getCodeSize(), code);

		free(code);
		code = nullptr;

		codeBaseAddr = remoteCode.ptr();
	} else {
		code = VirtualAlloc(NULL, a->getCodeSize(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		a->relocCode(code);
		codeBaseAddr = (ptr_t) code;
	}

	api.vectorNew = codeBaseAddr + a->getLabelOffset(lVectorNew);
	api.vectorFree = codeBaseAddr + a->getLabelOffset(lVectorFree);
	api.vectorAdd = codeBaseAddr + a->getLabelOffset(lVectorAdd);
	api.vectorClear = codeBaseAddr + a->getLabelOffset(lVectorClear);
	api.vectorLength = codeBaseAddr + a->getLabelOffset(lVectorLength);
	api.vectorCapacity = codeBaseAddr + a->getLabelOffset(lVectorCapacity);
	api.vectorData = codeBaseAddr + a->getLabelOffset(lVectorData);

	api.vectorGrow = codeBaseAddr + a->getLabelOffset(lVectorGrow);

	if (process) {
		remAPI = new VectorRemoteAPI {
			RemoteFunctionFastcall<VECTOR_NEW>(*process, api.vectorNew),
			RemoteFunctionFastcall<VECTOR_FREE>(*process, api.vectorFree),
			RemoteFunctionFastcall<VECTOR_ADD>(*process, api.vectorAdd),
			RemoteFunctionFastcall<VECTOR_CLEAR>(*process, api.vectorClear),
			RemoteFunctionFastcall<VECTOR_LENGTH>(*process, api.vectorLength),
			RemoteFunctionFastcall<VECTOR_CAPACITY>(*process, api.vectorCapacity),
			RemoteFunctionFastcall<VECTOR_DATA>(*process, api.vectorData),

			RemoteFunctionFastcall<VECTOR_GROW>(*process, api.vectorGrow)
		};
	} else {
		localApi.vectorNew = (VECTOR_NEW) api.vectorNew;
		localApi.vectorFree = (VECTOR_FREE) api.vectorFree;
		localApi.vectorAdd = (VECTOR_ADD) api.vectorAdd;
		localApi.vectorClear = (VECTOR_CLEAR) api.vectorClear;
		localApi.vectorLength = (VECTOR_LENGTH) api.vectorLength;
		localApi.vectorCapacity = (VECTOR_CAPACITY) api.vectorCapacity;
		localApi.vectorData = (VECTOR_DATA) api.vectorData;

		localApi.vectorGrow = (VECTOR_GROW) api.vectorGrow;
	}

	injected = true;
}


template <typename ElemT, typename IntPtrT>
void IPCVector<ElemT, IntPtrT>::uninject()
{
	if (!injected) {
		return;
	}

	if (process) {
		delete remAPI;

		remoteCode.Free();
		remoteCode = MemBlock();

		process = nullptr;
	} else {
		if (code) {
			VirtualFree(code, 0, MEM_RELEASE);
			code = nullptr;
		}
	}

	injected = false;
}


}
