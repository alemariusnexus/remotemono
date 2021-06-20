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
