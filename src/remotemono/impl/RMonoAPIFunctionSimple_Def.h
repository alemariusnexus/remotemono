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

#include "../config.h"

#include <tuple>
#include <type_traits>
#include "backend/RMonoProcess.h"
#include "backend/RMonoRPCFunc.h"




namespace remotemono
{



template <typename RetT, typename... ArgsT>
class RMonoAPIFunctionSimple
{
public:
	typedef RetT RetType;
	typedef std::tuple<ArgsT...> ArgsTuple;

	typedef backend::RMonoRPCFunc<backend::CallConvCdecl, RetT, ArgsT...> RemoteFunc;

public:
	RMonoAPIFunctionSimple() : process(nullptr), addr(0), f(nullptr) {}
	RMonoAPIFunctionSimple(backend::RMonoProcess& proc, rmono_funcp addr)
			: process(&process), addr(addr), f(nullptr)
	{
		rebuildRemoteFunc();
	}
	~RMonoAPIFunctionSimple()
	{
		delete f;
	}

	void reset()
	{
		delete f;

		process = nullptr;
		addr = 0;
		f = nullptr;
	}

	void rebuild(backend::RMonoProcess& proc, rmono_funcp addr)
	{
		this->process = &proc;
		this->addr = addr;
		rebuildRemoteFunc();
	}

	rmono_funcp getAddress() const { return addr; }

	operator bool() const { return f != nullptr; }

	RetT operator()(ArgsT... args)
	{
		assert(f);

		if constexpr(!std::is_same_v<RetT, void>) {
			return (*f)(args...);
		} else {
			(*f)(args...);
		}
	}

private:
	void rebuildRemoteFunc()
	{
		delete f;
		f = new RemoteFunc(process, addr);
	}

private:
	backend::RMonoProcess* process;
	rmono_funcp addr;
	RemoteFunc* f;
};



}
