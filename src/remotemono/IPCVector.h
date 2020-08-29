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

#include <stdint.h>
#include <BlackBone/Patterns/PatternSearch.h>
#include <BlackBone/Process/Process.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>



namespace remotemono
{


// NOTE: We need these classes to specify the calling convention explicitly. blackbone::RemoteFunction gets it from
// the function signature, but for x64 locals, function pointer types that differ only in the calling convention (e.g.
// the __fastcall prefix) are indistinguishable (all aliases for Microsoft's x64 calling convention), so BlackBone will
// always detect them as __cdecl, even when defined as __fastcall. This is a problem with x86 remotes, where these
// calling conventions still make a difference.
template<typename Fn>
class RemoteFunctionFastcall;

template<typename R, typename... Args>
class RemoteFunctionFastcall<R ( __fastcall* )(Args...)> : public blackbone::RemoteFunctionBase<blackbone::cc_fastcall, R, Args...>
{
public:
    using RemoteFunctionBase::RemoteFunctionBase;

    RemoteFunctionFastcall( blackbone::Process& proc, R( __fastcall* ptr )(Args...), blackbone::ThreadPtr boundThread = nullptr )
        : blackbone::RemoteFunctionBase( proc, reinterpret_cast<ptr_t>(ptr), boundThread )
    {
    }
};



/**
 * A simple class representing a dynamically growing array in either the local or remote process. If it is in the remote
 * process, a series of simple functions will be injected into the remote that can be called to manipulate such array
 * objects in the remote process itself, and a remote array can also be queried and modified locally through RPC calls.
 *
 * In RemoteMono, this class is mainly used to support the `mono_*_foreach()` functions by having them collect the
 * objects they iterate over into an IPCVector array that can then be read by the local process.
 */
template <typename ElemT, typename IntPtrT>
class IPCVector
{
public:
	typedef IntPtrT VectorPtr;
	typedef IntPtrT DataPtr;

	typedef VectorPtr (__fastcall *VECTOR_NEW)(uint32_t cap);
	typedef void (__fastcall *VECTOR_FREE)(VectorPtr v);
	typedef void (__fastcall *VECTOR_ADD)(VectorPtr v, ElemT elem);
	typedef void (__fastcall *VECTOR_CLEAR)(VectorPtr v);
	typedef uint32_t (__fastcall *VECTOR_LENGTH)(VectorPtr v);
	typedef uint32_t (__fastcall *VECTOR_CAPACITY)(VectorPtr v);
	typedef DataPtr (__fastcall *VECTOR_DATA)(VectorPtr v);

	typedef void (__fastcall *VECTOR_GROW)(VectorPtr v, uint32_t);

	struct VectorAPI
	{
		blackbone::ptr_t vectorNew;
		blackbone::ptr_t vectorFree;
		blackbone::ptr_t vectorAdd;
		blackbone::ptr_t vectorClear;
		blackbone::ptr_t vectorLength;
		blackbone::ptr_t vectorCapacity;
		blackbone::ptr_t vectorData;

		blackbone::ptr_t vectorGrow;
	};
	struct VectorLocalAPI
	{
		VECTOR_NEW vectorNew;
		VECTOR_FREE vectorFree;
		VECTOR_ADD vectorAdd;
		VECTOR_CLEAR vectorClear;
		VECTOR_LENGTH vectorLength;
		VECTOR_CAPACITY vectorCapacity;
		VECTOR_DATA vectorData;

		VECTOR_GROW vectorGrow;
	};
	struct VectorRemoteAPI
	{
		RemoteFunctionFastcall<VECTOR_NEW> vectorNew;
		RemoteFunctionFastcall<VECTOR_FREE> vectorFree;
		RemoteFunctionFastcall<VECTOR_ADD> vectorAdd;
		RemoteFunctionFastcall<VECTOR_CLEAR> vectorClear;
		RemoteFunctionFastcall<VECTOR_LENGTH> vectorLength;
		RemoteFunctionFastcall<VECTOR_LENGTH> vectorCapacity;
		RemoteFunctionFastcall<VECTOR_DATA> vectorData;

		RemoteFunctionFastcall<VECTOR_GROW> vectorGrow;
	};

private:
	struct Vector
	{
		DataPtr data;
		uint32_t len;
		uint32_t cap;
	};

public:
	IPCVector();
	virtual ~IPCVector();

	void inject(blackbone::Process* process);
	void uninject();

	VectorAPI& getAPI() { return api; }

	VectorPtr vectorNew(uint32_t cap = 16);
	void vectorFree(VectorPtr v);
	void vectorAdd(VectorPtr v, ElemT elem);
	void vectorClear(VectorPtr v);
	uint32_t vectorLength(VectorPtr v);
	uint32_t vectorCapacity(VectorPtr v);
	DataPtr vectorData(VectorPtr v);

	void vectorGrow(VectorPtr v, uint32_t cap);

	VectorPtr create(const std::vector<ElemT>& data);
	void read(VectorPtr v, std::vector<ElemT>& out);

private:
	blackbone::Process* process;
	bool injected;
	VectorAPI api;
	VectorLocalAPI localApi;
	VectorRemoteAPI* remAPI;
	blackbone::MemBlock remoteCode;
	void* code;
};


}




#include "IPCVector_Impl.h"
