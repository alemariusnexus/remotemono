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
#include "impl/backend/RMonoProcess.h"
#include "impl/backend/RMonoMemBlock.h"
#include "impl/backend/RMonoRPCFunc.h"



namespace remotemono
{


template<typename Fn>
class RemoteFunctionFastcall;

template <typename R, typename... Args>
class RemoteFunctionFastcall<R (__fastcall*)(Args...)> : public backend::RMonoRPCFunc<backend::CallConvFastcall, R, Args...>
{
public:
	using RMonoRPCFunc::RMonoRPCFunc;

	RemoteFunctionFastcall(backend::RMonoProcess* process, rmono_funcp fptr) : RMonoRPCFunc(process, fptr) {}
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
		rmono_funcp vectorNew;
		rmono_funcp vectorFree;
		rmono_funcp vectorAdd;
		rmono_funcp vectorClear;
		rmono_funcp vectorLength;
		rmono_funcp vectorCapacity;
		rmono_funcp vectorData;

		rmono_funcp vectorGrow;
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

	void inject(backend::RMonoProcess* process);
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
	backend::RMonoProcess* process;
	bool injected;
	VectorAPI api;
	VectorLocalAPI localApi;
	VectorRemoteAPI* remAPI;
	backend::RMonoMemBlock remoteCode;
	void* code;
};


}




#include "IPCVector_Impl.h"
