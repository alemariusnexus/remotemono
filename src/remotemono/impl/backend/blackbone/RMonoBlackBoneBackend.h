#pragma once

#include "../../../config.h"

#include "../RMonoBackend.h"
#include <BlackBone/Asm/IAsmHelper.h>

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "Included BlackBone backend header, but BlackBone backend is disabled."
#endif


namespace remotemono
{
namespace backend
{
namespace blackbone
{


class RMonoBlackBoneBackend : public RMonoBackend
{
public:
	template <RMonoCallingConvention cconv>
	constexpr static ::blackbone::eCalligConvention convertCallingConv()
	{
		if constexpr(cconv == CallConvFastcall) {
			return ::blackbone::cc_fastcall;
		} else if constexpr (cconv == CallConvStdcall) {
			return ::blackbone::cc_stdcall;
		} else {
			return ::blackbone::cc_cdecl;
		}
	}

	static ::blackbone::eCalligConvention convertCallingConv(RMonoCallingConvention cconv)
	{
		if (cconv == CallConvFastcall) {
			return ::blackbone::cc_fastcall;
		} else if (cconv == CallConvStdcall) {
			return ::blackbone::cc_stdcall;
		} else {
			return ::blackbone::cc_cdecl;
		}
	}

public:
	static RMonoBlackBoneBackend& getInstance()
	{
		static RMonoBlackBoneBackend inst;
		return inst;
	}

public:
	std::string getID() const override { return "blackbone"; }
	std::string getName() const override { return "BlackBone"; }

private:
	RMonoBlackBoneBackend() {}
};


}
}
}
