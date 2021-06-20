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

#include "../RMonoAsmHelper.h"
#include "RMonoBlackBoneBackend.h"
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


class RMonoBlackBoneAsmHelper : public RMonoAsmHelper
{
private:
	static uint32_t convertAsmJitArch(backend::RMonoProcessorArch arch)
	{
		switch (arch) {
		case backend::ProcessorArchX86:
			return asmjit::kArchX86;
		case backend::ProcessorArchX86_64:
			return asmjit::kArchX64;
		}
		return asmjit::kArchNone;
	}

public:
	RMonoBlackBoneAsmHelper(::blackbone::AsmHelperPtr&& helper)
			: helper(std::move(helper))
	{
	}

	asmjit::X86Assembler* getAssembler() override { return helper->assembler(); }

	void genCall (
			rmono_funcp fptr,
			const std::vector<RMonoAsmVariant>& args,
			backend::RMonoCallingConvention cconv = backend::CallConvStdcall
			) override
	{
		std::vector<::blackbone::AsmVariant> bbArgs;
		bbArgs.reserve(args.size()); // TODO: Why is this necessary? Doesn't AsmVariant have copy constructors?!
		for (const RMonoAsmVariant& arg : args) {
			bbArgs.push_back(convertAsmVariant(arg));
		}
		helper->GenCall(fptr, bbArgs, RMonoBlackBoneBackend::convertCallingConv(cconv));
	}

private:
	::blackbone::AsmVariant convertAsmVariant(const RMonoAsmVariant& v) const
	{
		RMonoAsmVariant::Type type = v.getType();

		if (type == RMonoAsmVariant::TypeImmediate) {
			uint64_t iv = v.getValueImmediate64();
			switch (v.getSize()) {
			case 1:
				return ::blackbone::AsmVariant(static_cast<uint8_t>(iv));
			case 2:
				return ::blackbone::AsmVariant(static_cast<uint16_t>(iv));
			case 4:
				return ::blackbone::AsmVariant(static_cast<uint32_t>(iv));
			case 8:
				return ::blackbone::AsmVariant(iv);
			default:
				assert(false);
				return ::blackbone::AsmVariant(iv);
			}
		} else if (type == RMonoAsmVariant::TypeRegister) {
			return ::blackbone::AsmVariant(v.getValueRegister());
		} else if (type == RMonoAsmVariant::TypeMemory) {
			return ::blackbone::AsmVariant(v.getValueMemory());
		}
		assert(false);
		return ::blackbone::AsmVariant(0);
	}

private:
	::blackbone::AsmHelperPtr helper;
};


}
}
}
