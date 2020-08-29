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
#include <BlackBone/Process/Process.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>




namespace remotemono
{



template <typename RetT, typename... ArgsT>
class RMonoAPIFunctionSimple
{
public:
	typedef RetT RetType;
	typedef std::tuple<ArgsT...> ArgsTuple;

	typedef RetT (*Func)(ArgsT...);
	typedef blackbone::RemoteFunction<Func> RemoteFunc;

public:
	RMonoAPIFunctionSimple() : process(nullptr), addr(0), boundThread(nullptr), f(nullptr) {}
	RMonoAPIFunctionSimple(blackbone::Process& proc, blackbone::ptr_t addr, blackbone::ThreadPtr boundThread = nullptr)
			: process(&process), addr(addr), boundThread(boundThread), f(nullptr)
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
		boundThread = nullptr;
		f = nullptr;
	}

	void rebuild(blackbone::Process& proc, blackbone::ptr_t addr, blackbone::ThreadPtr boundThread = nullptr)
	{
		this->process = &proc;
		this->addr = addr;
		this->boundThread = boundThread;
		rebuildRemoteFunc();
	}

	blackbone::ptr_t getAddress() const { return addr; }

	operator bool() const { return f != nullptr; }

	RetT operator()(ArgsT... args)
	{
		assert(f);

		if constexpr(!std::is_same_v<RetT, void>) {
			return *f->Call(typename RemoteFunc::CallArguments(args...), boundThread);
		} else {
			f->Call(typename RemoteFunc::CallArguments(args...), boundThread);
		}
	}

private:
	void rebuildRemoteFunc()
	{
		delete f;
		f = new RemoteFunc(*process, addr);
	}

private:
	blackbone::Process* process;
	blackbone::ptr_t addr;
	blackbone::ThreadPtr boundThread;
	RemoteFunc* f;
};



}
