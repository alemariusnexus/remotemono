#pragma once

#include "../../../config.h"

#include "../RMonoTypes.h"
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
		if constexpr(!std::is_same_v<RetT, void>) {
			return *f.Call(typename RemoteFunc::CallArguments(args...), process.remote().getWorker());
		} else {
			f.Call(typename RemoteFunc::CallArguments(args...), process.remote().getWorker());
		}
	}

private:
	::blackbone::Process& process;
	RemoteFunc f;
};


}
}
}
