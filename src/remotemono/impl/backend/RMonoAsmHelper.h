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

#include "../../config.h"

#include "RMonoBackendTypes.h"
#include "AsmJit.h"


namespace remotemono
{
namespace backend
{


class RMonoAsmVariant
{
public:
	enum Type
	{
		TypeRegister,
		TypeImmediate,
		TypeMemory
	};

public:
	template <typename T>
	RMonoAsmVariant(T&& arg)
	{
		using arg_t = std::decay_t<T>;
		constexpr size_t argSize = sizeof(arg_t);

		if constexpr (std::is_integral_v<arg_t>) {
			type = TypeImmediate;
			size = argSize;
			valImm64 = static_cast<uint64_t>(arg);
		} else {
			static_assert(std::is_integral_v<arg_t>);
		}
	}

	RMonoAsmVariant(asmjit::GpReg reg)
			: type(TypeRegister), size(sizeof(uintptr_t)), valReg(reg)
	{
	}

	RMonoAsmVariant(asmjit::Mem mem)
			: type(TypeMemory), size(sizeof(uintptr_t)), valMem(mem)
	{
	}

	RMonoAsmVariant(const RMonoAsmVariant&) = default;
	RMonoAsmVariant(RMonoAsmVariant&&) = default;
	RMonoAsmVariant& operator=(const RMonoAsmVariant&) = default;

	Type getType() const { return type; }
	size_t getSize() const { return size; }

	uint64_t getValueImmediate64() const { return valImm64; }
	asmjit::GpReg getValueRegister() const { return valReg; }
	asmjit::Mem getValueMemory() const { return valMem; }

private:
	Type type;
	size_t size;

	asmjit::GpReg valReg;
	asmjit::Mem valMem;
	union
	{
		uint64_t valImm64;
	};
};


class RMonoAsmHelper
{
public:
	virtual asmjit::X86Assembler* getAssembler() = 0;

	virtual void genCall (
			rmono_funcp fptr,
			const std::vector<RMonoAsmVariant>& args,
			backend::RMonoCallingConvention cconv = backend::CallConvStdcall
			) = 0;

	asmjit::X86Assembler* assembler() { return getAssembler(); }
	asmjit::X86Assembler* operator->() { return getAssembler(); }
};


}
}
