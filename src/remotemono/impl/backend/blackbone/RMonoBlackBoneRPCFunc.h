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

#include "../../../config.h"

#include "../../RMonoTypes.h"
#include "RMonoBlackBoneBackend.h"

#include <BlackBone/Process/Process.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "Included BlackBone backend header, but BlackBone backend is disabled."
#endif


namespace remotemono
{
namespace backend
{
namespace blackbone
{


template <RMonoCallingConvention cconv, typename RetT, typename... ArgsT>
class RMonoBlackBoneRPCFunc
{
public:
	typedef ::blackbone::RemoteFunctionBase<RMonoBlackBoneBackend::convertCallingConv<cconv>(), RetT, ArgsT...> RemoteFunc;

public:
	RMonoBlackBoneRPCFunc(::blackbone::Process& process, ::blackbone::ptr_t fptr)
			: process(process), f(process, fptr)
	{}

	RemoteFunc& getRemoteFunc() { return f; }
	const RemoteFunc& getRemoteFunc() const { return f; }

	RetT operator()(ArgsT... args)
	{
		typename RemoteFunc::CallArguments cargs(args...);
		if constexpr(!std::is_same_v<RetT, void>) {
			return *f.Call(cargs, process.remote().getWorker());
		} else {
			f.Call(cargs, process.remote().getWorker());
		}
	}

private:
	::blackbone::Process& process;
	RemoteFunc f;
};


}
}
}
