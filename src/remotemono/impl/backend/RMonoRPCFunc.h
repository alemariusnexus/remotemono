#pragma once

#include "../../config.h"

#include "../RMonoTypes.h"
#include "../exception/RMonoException_Def.h"
#include "RMonoProcess.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#include "blackbone/RMonoBlackBoneRPCFunc.h"
#include "blackbone/RMonoBlackBoneProcess.h"
#endif


namespace remotemono
{
namespace backend
{


template <RMonoCallingConvention cconv, typename RetT, typename... ArgsT>
class RMonoRPCFunc
{
public:
	RMonoRPCFunc(RMonoProcess* process, rmono_funcp fptr)
			: process(process), fptr(fptr)
#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
			  , bbFunc(nullptr)
#endif
	{
#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
		blackbone::RMonoBlackBoneProcess* bbProc = dynamic_cast<blackbone::RMonoBlackBoneProcess*>(process);
		if (bbProc) {
			bbFunc = new blackbone::RMonoBlackBoneRPCFunc<cconv, RetT, ArgsT...>(**bbProc, static_cast<::blackbone::ptr_t>(fptr));
		}
#endif
	}

	RetT operator()(ArgsT... args)
	{
#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
		if (bbFunc) {
			return (*bbFunc)(args...);
		}
#endif
		throw RMonoException("Invalid backend for RMonoRPCFunc");
		return RetT();
	}

	rmono_funcp getFunctionPointer() const { return fptr; }

private:
	RMonoProcess* process;
	rmono_funcp fptr;

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
	blackbone::RMonoBlackBoneRPCFunc<cconv, RetT, ArgsT...>* bbFunc;
#endif
};


}
}
