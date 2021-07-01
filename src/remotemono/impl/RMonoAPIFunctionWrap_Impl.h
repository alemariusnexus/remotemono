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

#include "../config.h"

#include "RMonoAPIFunctionWrap_Def.h"

#include <type_traits>
#include "RMonoAPIBase_Def.h"
#include "../util.h"
#include "../asmutil.h"
#include "../log.h"



namespace remotemono
{






// **************************************************************
// *															*
// *						FRONT CLASS							*
// *															*
// **************************************************************


// !!!!!   IMPORTANT   !!!!!
//
// I know that this is a bit of a hairy mess. This is why this is so heavily documented.
//
// A few notes and general guidelines for writing assembly code in this class:
//
//		* Whenever possible, try to write code that works on both x86 and x64 without resorting to conditional
//		  checks. This is not about performance, but about readability and maintenance. This means: Don't
//		  use registers r8-r15 unless absolutely necessary, even if it's tempting.
//		* The wrapper functions have a __cdecl calling convention on x86, and Microsoft x64 on x64, because
//		  those calling conventions are relatively similar and make for a more unified code.
//		* Almost all of the assembler code here is interleaved with the corresponding (pseudo-)C code in
//		  comments. When editing the assembly, make sure to update the C code as well.
//		* It's okay to do a bunch of copying between registers and stack, even if that could be optimized
//		  out. We are calling methods in a remote process - this function is certainly not the bottleneck
//		  of the system.



template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
asmjit::Label RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::compileWrap(backend::RMonoAsmHelper& a)
{
	asmjit::Label label = a->newLabel();
	a->bind(label);

	RMonoAPIBase* mono = getRemoteMonoAPI();
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();

	AsmBuildContext ctx;
	ctx.a = &a;

	apid->apply([&](auto& e) {
		ctx.gchandleGetTargetAddr = e.api.gchandle_get_target.getRawFuncAddress();
		ctx.gchandleNewAddr = e.api.gchandle_new.getRawFuncAddress();
		ctx.objectGetClassAddr = e.api.object_get_class.getRawFuncAddress();
		ctx.classIsValuetypeAddr = e.api.class_is_valuetype.getRawFuncAddress();
		ctx.objectUnboxAddr = e.api.object_unbox.getRawFuncAddress();
	});

	if constexpr(needsWrapFunc()) {
		generateWrapperAsm(ctx);
	}

	return label;
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::generateWrapperAsm(AsmBuildContext& ctx)
{
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	// TODO: We might want to handle exceptions differently: When an exception output parameter is set (i.e. an exception
	// was thrown), return values and other output parameters may not actually be valid. So is it safe to handle them
	// like we do when an exception occurs?


	rmono_funcp rawFuncAddr = static_cast<CommonT*>(this)->getRawFuncAddress();


	Label lFuncRet = a->newLabel();

	ctx.x64 = (sizeof(irmono_voidp) == 8);
	ctx.regSize = sizeof(irmono_voidp);
	ctx.rawArgStackSize = 0;



	// **********************************************************
	// *					FUNCTION PROLOG						*
	// **********************************************************


	// ********** Save registers to stack **********

	std::vector<GpReg> savedRegisters = { a->zbp, a->zbx, a->zsi, a->zdi };

	if (ctx.x64) {
		genWrapperSpillArgsToStackX64<0>(ctx);
	}

	for (GpReg reg : savedRegisters) {
		a->push(reg);
	}


	// ********** Reserve misc. static stack space **********

	int32_t miscStaticStackSize = 0;

	// Return value
	miscStaticStackSize += ctx.regSize;

	a->sub(a->zsp, miscStaticStackSize);

	ctx.stackOffsRetval = 0;


	// ********** Save static stack base in ZBP **********

	a->mov(a->zbp, a->zsp);

	// Misc. space, saved registers, return address
	ctx.stackOffsArgBase = int32_t(miscStaticStackSize + savedRegisters.size()*ctx.regSize + ctx.regSize);


	// ********** Reserve dynamic stack space **********

	// Align stack to pointer size before we start to allocate dynamic stack. Not entirely sure if it's necessary, but it's
	// safer to do so for two reasons:
	//		1.	We'll pass pointers to elements in the dynamic stack region to the raw function, and some calling
	//			conventions might require "natural alignment" for values behind argument pointers.
	//		2.	We'll put some MonoObjectPtrRaws there, so the Mono GC doesn't move them, and it's possible that the GC
	//			only scans for pointers at pointer-aligned addresses (don't know if it actually does).
	if (ctx.x64) {
		a->and_(rsp, 0xFFFFFFFFFFFFFFF0);
	} else {
		a->and_(esp, 0xFFFFFFF8);
	}

	// zbx := curDynStackPtr
	a->mov(a->zbx, a->zsp);

	// We don't need stackRetval until we start processing the raw function's return value, so we can use its stack
	// location to store curDynStackPtr for just after the raw function call.
	//
	// stackRetval = curDynStackPtr;
	a->mov(ptr(a->zbp, ctx.stackOffsRetval), a->zbx);

	// NOTE: From here on out, the only non-volatile registers that can be freely used (by the functions reserving stack,
	// handling arguments and return types, etc.) are ZSI and ZDI. This is not a lot, but it will have to do.
	//
	// ZBX is in use for curDynStackPtr.
	// ZBP points to the static stack base (and is used later to restore ZSP)
	// ZSP is obviously used as a stack pointer.
	// R12-R15 should be avoided because they are x64-only.

	genWrapperReserveStack(ctx);

	genWrapperCalculateRawArgStackSize<0>(ctx);

	if (ctx.x64) {
		// Reserve at least the 32 bytes of shadow space for x64 calling conventions
		if (ctx.rawArgStackSize < 32) {
			ctx.rawArgStackSize = 32;
		}
	}

	// uint8_t stackMisalignment = (zsp - ctx.rawArgStackSize) & 0xF;
	a->mov(a->zcx, a->zsp);
	a->sub(a->zcx, ctx.rawArgStackSize);
	a->and_(a->zcx, 0xF);

	// zsp -= stackMisalignment
	a->sub(a->zsp, a->zcx);

	// zsp -= ctx.rawArgStackSize;
	a->sub(a->zsp, ctx.rawArgStackSize);

	// -> Stack is now aligned, raw function argument space directly above ZSP.




	// **********************************************************
	// *					FUNCTION PAYLOAD					*
	// **********************************************************


	// ********** Build raw function arguments **********

	genWrapperBuildRawArgs(ctx);


	// ********** Call raw function **********

	if (ctx.x64) {
		// Move first 4 arguments from stack to registers
		genWrapperMoveStackArgsToRegsX64<0>(ctx);

		// NOTE: Shadow space has already been allocated as part of the rawArgStack. Even if less than 4 parameters were
		// used, rawArgStack has been extended to at least 32 bytes. Also, we don't need to remove the shadow space right
		// away - we'll just restore ZSP from ZBP in the epilog anyway.
		a->mov(a->zax, rawFuncAddr);
		a->call(a->zax);
	} else {
		a->mov(a->zax, rawFuncAddr);
		a->call(a->zax);
	}


	// ********** Restore dynamic stack pointer **********

	// zbx := curDynStackPtr
	a->mov(a->zbx, ptr(a->zbp, ctx.stackOffsRetval)); // We stored it there when we first defined curDynStackPtr


	// ********** Handle return value and output arguments **********

	genWrapperHandleRetAndOutParams(ctx);

	//	return stackRetval;
	a->mov(a->zax, ptr(a->zbp, ctx.stackOffsRetval));




	// **********************************************************
	// *					FUNCTION EPILOG						*
	// **********************************************************

	a->bind(lFuncRet);

	a->mov(a->zsp, a->zbp);

	a->add(a->zsp, miscStaticStackSize);

	for (auto it = savedRegisters.rbegin() ; it != savedRegisters.rend() ; it++) {
		a->pop(*it);
	}

	// NOTE: We don't need to restore register values spilled by a.GenPrologue(), so we can safely omit most of the epilogue
	//a.GenEpilogue();
	a->ret();
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t wrapArgIdx>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperSpillArgsToStackX64(AsmBuildContext& ctx)
{
	using namespace asmjit;
	using namespace asmjit::host;

	typedef typename RMonoAPIFunctionWrapTraits<CommonT>::WrapArgsTuple WrapArgsTuple;

	backend::RMonoAsmHelper& a = *ctx.a;

	if constexpr (
				wrapArgIdx < 4
			&&	wrapArgIdx < std::tuple_size_v<WrapArgsTuple>
	) {
		static const GpReg intRegs[] = { rcx, rdx, r8, r9 };
		static const XmmReg floatRegs[] = { xmm0, xmm1, xmm2, xmm3 };

		static_assert(sizeof(std::tuple_element_t<wrapArgIdx, WrapArgsTuple>) <= sizeof(irmono_voidp),
				"Spilling large arguments is not supported in X64.");

		if constexpr(std::is_floating_point_v<std::tuple_element_t<wrapArgIdx, WrapArgsTuple>>) {
			a->movq(ptr(a->zsp, (wrapArgIdx+1) * sizeof(irmono_voidp)), floatRegs[wrapArgIdx]);
		} else {
			a->mov(ptr(a->zsp, (wrapArgIdx+1) * sizeof(irmono_voidp)), intRegs[wrapArgIdx]);
		}

		genWrapperSpillArgsToStackX64<wrapArgIdx+1>(ctx);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperReserveStack(AsmBuildContext& ctx)
{
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		if constexpr(sizeof...(ArgsT) != 0) {
			// Skip first wrap argument (hidden Variant output parameter)
			genWrapperReserveArgStack<1, ArgsT...>(ctx, PackHelper<ArgsT>()...);
		}
	} else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		if constexpr(sizeof...(ArgsT) != 0) {
			// Skip first wrap argument (hidden dataBlockPtr parameter)
			genWrapperReserveArgStack<1, ArgsT...>(ctx, PackHelper<ArgsT>()...);
		}
	} else if constexpr(sizeof...(ArgsT) != 0) {
		genWrapperReserveArgStack<0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t wrapArgIdx, typename ArgT, typename... RestT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperReserveArgStack (
		AsmBuildContext& ctx,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	// IMPORTANT: Always allocate stack space in multiples of sizeof(RemotePtrT) to ensure that values are aligned to
	// the pointer size. See comment at dynamic stack allocation above for why that's necessary.

	if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		Label lReserveEnd = a->newLabel();

		//	if (wrapArgs[wrapArgIdx] != nullptr) {
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));
			a->jecxz(a->zcx, lReserveEnd);

		//		// !!! Wrap argument points to payload, and flags are stored BEFORE the payload !!!
		//		variantflags_t flags = *((variantflags_t*) (wrapArgs[wrapArgIdx] - sizeof(variantflags_t)));
				a->movzx(a->zcx, ptr(a->zcx, - (int32_t) sizeof(variantflags_t), sizeof(variantflags_t)));

		//		if ((flags & ParamFlagMonoObjectPtr) != 0) {
				a->test(a->zcx, Self::ParamFlagMonoObjectPtr);
				a->jz(lReserveEnd);

		//			__dynstack IRMonoObjectPtrRaw variantDummyPtr;
					a->sub(a->zsp, sizeof(IRMonoObjectPtrRaw));
		//		}
		//	}
			a->bind(lReserveEnd);
	}

	else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		// We need to reserve sizeof(RemotePtrT) bytes on the stack for each MonoObjectPtr argument (except for our own
		// fixed arguments), because when calling mono_runtime_invoke(), we need to make sure that the raw pointers can
		// be found on the stack so the Mono GC doesn't move them. It is NOT enough to have the raw pointers in the
		// dataBlock, because dataBlock is heap-allocated and Mono's GC doesn't look for references on the heap.
		// Because it's easier and quicker, we'll just allocate space for all arguments, even the non-MonoObjectPtr ones.
		// It doesn't make a difference unless we call methods with thousands of arguments, and who does that?

		Label lReserveEnd = a->newLabel();

		//	uint8_t* blockPtr = wrapArgs[wrapArgIdx];
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));

		//	if (blockPtr != nullptr) {
			a->jecxz(a->zcx, lReserveEnd);

		//		uint32_t numElems = *((uint32_t*) blockPtr);
				a->mov(ecx, ptr(a->zcx));

		//		struct VariantArrayStackEntry {
		//			IRMonoObjectPtrRaw objPtr;
		//			IRMonoObjectPtrRaw origArrPtr;	// Only valid if (MonoObjectPtr && Out), otherwise NULL
		//		}
		//		__dynstack VariantArrayStackEntry variantArrStackData[numElems];
				a->shl(a->zcx, static_ilog2(2*sizeof(IRMonoObjectPtrRaw)));
				a->sub(a->zsp, a->zcx);

		//	}
			a->bind(lReserveEnd);
	}

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			Label lReserveEnd = a->newLabel();

			//	if (wrapArgs[wrapArgIdx] != nullptr) {
				a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));
				a->jecxz(a->zcx, lReserveEnd);

			//		__dynstack IRMonoObjectPtrRaw outMonoObjectPtr;
					a->sub(a->zsp, sizeof(IRMonoObjectPtrRaw));

			//	}
				a->bind(lReserveEnd);
		}
	}

	genWrapperReserveArgStack<wrapArgIdx+1, RestT...>(ctx, rest...);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t argIdx>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperCalculateRawArgStackSize(AsmBuildContext& ctx)
{
	if constexpr(argIdx < std::tuple_size_v<typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple>) {
		ctx.rawArgStackSize += static_align (
				(int32_t) sizeof(std::tuple_element_t<argIdx, typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple>),
				(int32_t) sizeof(irmono_voidp)
				);

		genWrapperCalculateRawArgStackSize<argIdx+1>(ctx);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperBuildRawArgs(AsmBuildContext& ctx)
{
	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		if constexpr(sizeof...(ArgsT) != 0) {
			// Skip first wrap argument (hidden Variant output parameter)
			genWrapperBuildRawArg<1, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
		}
	} else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		if constexpr(sizeof...(ArgsT) != 0) {
			// Skip first wrap argument (hidden dataBlockPtr parameter)
			genWrapperBuildRawArg<1, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
		}
	} else if constexpr(sizeof...(ArgsT) != 0) {
		genWrapperBuildRawArg<0, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t wrapArgIdx, size_t rawArgIdx, typename ArgT, typename... RestT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperBuildRawArg (
		AsmBuildContext& ctx,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	if constexpr(tags::has_param_tag_v<ArgT, tags::ParamOutRetClsTag>) {
		// Skip raw argument
		genWrapperBuildRawArg<wrapArgIdx+1, rawArgIdx, RestT...>(ctx, rest...);
	} else if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		Label lBuildEnd = a->newLabel();
		Label lNullPtr = a->newLabel();
		Label lNotMonoObjectPtr = a->newLabel();
		Label lNoAutoUnbox = a->newLabel();
		Label lNotDirectPtr = a->newLabel();
		Label lMonoObjectPtrNotOut = a->newLabel();

		//	if (wrapArgs[wrapArgIdx] != nullptr) {
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));
			a->test(a->zcx, a->zcx);
			a->jz(lNullPtr);

		//		// !!! Wrap argument points to payload, and flags are stored BEFORE the payload !!!
		//		variantflags_t flags = *((variantflags_t*) (wrapArgs[wrapArgIdx] - sizeof(variantflags_t)));
				a->movzx(a->zsi, ptr(a->zcx, - (int32_t) sizeof(variantflags_t), sizeof(variantflags_t)));

		//		if ((flags & ParamFlagMonoObjectPtr) != 0) {
				a->test(a->zsi, Self::ParamFlagMonoObjectPtr);
				a->jz(lNotMonoObjectPtr);

		//			irmono_gchandle gchandle = *((irmono_gchandle*) wrapArgs[wrapArgIdx]);
		//			IRMonoObjectPtrRaw objPtr = mono_gchandle_get_target_checked(gchandle);
					a->mov(ecx, ptr(a->zcx));
					genGchandleGetTargetChecked(ctx);
					a->mov(a->zdi, a->zax);

		//			curDynStackPtr -= sizeof(IRMonoObjectPtrRaw);
					a->sub(a->zbx, sizeof(IRMonoObjectPtrRaw));

		//			variantDummyPtr = objPtr;
					a->mov(ptr(a->zbx), a->zdi);

		//			if ((flags & ParamFlagDisableAutoUnbox)  ==  0  &&  is_value_type_instance(objPtr)) {
					a->test(a->zsi, Self::ParamFlagDisableAutoUnbox);
					a->jnz(lNoAutoUnbox);
					a->mov(a->zcx, a->zdi);
					genIsValueTypeInstance(ctx);
					a->test(a->zax, a->zax);
					a->jz(lNoAutoUnbox);

		//				irmono_voidp unboxed = mono_object_unbox(objPtr);
						a->mov(a->zcx, a->zdi);
						genObjectUnbox(ctx);

		//				rawArgs[rawArgIdx] = unboxed;
						a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zax);

					a->jmp(lBuildEnd);
		//			} else {
					a->bind(lNoAutoUnbox);

		//				if ((flags & ParamFlagOut) != 0) {
						a->test(a->zsi, Self::ParamFlagOut);
						a->jz(lMonoObjectPtrNotOut);

		//					rawArgs[rawArgIdx] = &variantDummyPtr;
							a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zbx);

						a->jmp(lBuildEnd);
		//				} else {
						a->bind(lMonoObjectPtrNotOut);

		//					rawArgs[rawArgIdx] = objPtr;
							a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zdi);

		//				}

		//			}

				a->jmp(lBuildEnd);
		//		} else if ((flags & ParamFlagDirectPtr) != 0) {
				a->bind(lNotMonoObjectPtr);
				a->test(a->zsi, Self::ParamFlagDirectPtr);
				a->jz(lNotDirectPtr);

		//			rawArgs[rawArgIdx] = *((irmono_voidp*) wrapArgs[wrapArgIdx]);
					a->mov(a->zax, ptr(a->zcx));
					a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zax);

				a->jmp(lBuildEnd);
		//		} else {
				a->bind(lNotDirectPtr);

		//			rawArgs[rawArgIdx] = (irmono_voidp) wrapArgs[wrapArgIdx];
					a->lea(a->zax, ptr(a->zcx));
					a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zax);

		//		}

			a->jmp(lBuildEnd);
		//	} else {
			a->bind(lNullPtr);

				a->mov(ptrRawFuncArg<rawArgIdx>(ctx, 0, sizeof(irmono_voidp)), 0);

		//	}
			a->bind(lBuildEnd);
	}

	else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		Label lBuildEnd = a->newLabel();
		Label lBlockPtrNull = a->newLabel();
		Label lLoopStart = a->newLabel();
		Label lLoopFinal = a->newLabel();
		Label lLoopEnd = a->newLabel();
		Label lNotMonoObjectPtr = a->newLabel();
		Label lNotOut = a->newLabel();
		Label lNoAutoUnbox = a->newLabel();
		Label lNoAutoUnboxNotOut = a->newLabel();

		//	blockPtr = wrapArgs[wrapArgIdx];
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));

		//	if (blockPtr != nullptr  &&  numElems != 0) {
			a->test(a->zcx, a->zcx);
			a->jz(lBlockPtrNull);
			a->cmp(dword_ptr(a->zcx), 0);
			a->jz(lBlockPtrNull);

		//		uint32_t numElems = *((uint32_t*) blockPtr);
				a->mov(a->zdx, dword_ptr(a->zcx));

		//		// For the alignment, if sizeof(irmono_voidp) is 4, we are automatically aligned. If it's 8, we are
		//		// either aligned or exactly 4 bytes off.
		//		irmono_voidpp arrEntryPtr = (irmono_voidpp) align(blockPtr + sizeof(uint32_t), sizeof(irmono_voidpp));
				a->lea(a->zsi, ptr(a->zcx, sizeof(uint32_t)));
				if (sizeof(irmono_voidp) == 8) {
		//			arrEntryPtr += (arrEntryPtr & 0x7);
					a->mov(a->zax, a->zsi);
					a->and_(a->zax, 0x7);
					a->add(a->zsi, a->zax);
				}

		//		// As long as sizeof(variantflags_t) <= sizeof(irmono_voidp), we are automatically aligned.
		//		variantflags_t* flagsPtr = (uint8_t*) align(arrEntryPtr + numElems*sizeof(irmono_voidp), sizeof(variantflags_t);
				a->lea(a->zdi, ptr(a->zsi, a->zdx, static_ilog2(sizeof(irmono_voidp))));

		//		rawArgs[rawArgIdx] = arrEntryPtr;
				a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zsi);

		//		// We'll use the presence of ParamFlagLastArrayElement as a stop condition for the loop. We've already checked
				// that there's at least one entry in the array above. This way, we don't need to store the loop counter nor
				// the number of elements in the array. Now we can use ZSI and ZDI for other purposes. :)
		//		do {
				a->bind(lLoopStart);

		//			curDynStackPtr -= sizeof(VariantArrayStackEntry);
					a->sub(a->zbx, 2*sizeof(IRMonoObjectPtrRaw));

		//			// Must be NULL unless (MonoObjectPtr && Out == true) for genWrapperHandleOutParams() below.
		//			variantArrStackData[i].origArrPtr = 0;
					a->mov(ptr(a->zbx, sizeof(IRMonoObjectPtrRaw), sizeof(IRMonoObjectPtrRaw)), 0);

		//			if (*arrEntryPtr != nullptr) {
					a->cmp(ptr(a->zsi, 0, sizeof(irmono_voidp)), 0);
					a->je(lLoopFinal);

		//				if (((*flagsPtr) & ParamFlagMonoObjectPtr) != 0) {
						a->test(ptr(a->zdi, 0, sizeof(variantflags_t)), Self::ParamFlagMonoObjectPtr);
						a->jz(lNotMonoObjectPtr);

		//					irmono_gchandle gchandle = *((irmono_gchandle*) *arrEntryPtr);
		//					IRMonoObjectPtrRaw objPtr = mono_gchandle_get_target_checked(gchandle);
							a->mov(a->zcx, ptr(a->zsi));
							a->mov(ecx, ptr(a->zcx));
							genGchandleGetTargetChecked(ctx);

		//					variantArrStackData[i].objPtr = objPtr;
							a->mov(ptr(a->zbx), a->zax);

		//					if (((*flagsPtr) & ParamFlagOut) != 0) {
							a->test(ptr(a->zdi, 0, sizeof(variantflags_t)), Self::ParamFlagOut);
							a->jz(lNotOut);

		//						variantArrStackData[i].origArrPtr = *arrEntryPtr;
								a->mov(a->zcx, ptr(a->zsi));
								a->mov(ptr(a->zbx, sizeof(IRMonoObjectPtrRaw)), a->zcx);

		//					}
							a->bind(lNotOut);

		//					// Always store objPtr into the array first. Even if it's overwritten in the next few lines
		//					// of code, we can use this to restore objPtr from the array after zcx has been overwritten
		//					// by a function call.
		//					*arrEntryPtr = objPtr;
							a->mov(ptr(a->zsi), a->zax);

		//					if (((*flagsPtr) & ParamFlagDisableAutoUnbox)  ==  0  &&  is_value_type_instance(objPtr)) {
							a->test(ptr(a->zdi, 0, sizeof(variantflags_t)), Self::ParamFlagDisableAutoUnbox);
							a->jnz(lNoAutoUnbox);
							a->mov(a->zcx, a->zax);
							genIsValueTypeInstance(ctx);
							a->test(a->zax, a->zax);
							a->mov(a->zax, ptr(a->zsi)); // Restore objPtr (overwritten by function call)
							a->jz(lNoAutoUnbox);

		//						irmono_voidp unboxed = mono_object_unbox(objPtr);
								a->mov(a->zcx, a->zax);
								genObjectUnbox(ctx);

		//						*arrEntryPtr = unboxed;
								a->mov(ptr(a->zsi), a->zax);

							a->jmp(lLoopFinal);
		//					} else {
							a->bind(lNoAutoUnbox);

		//						if (((*flagsPtr) & ParamFlagOut) != 0) {
								a->test(ptr(a->zdi, 0, sizeof(variantflags_t)), Self::ParamFlagOut);
								a->jz(lNoAutoUnboxNotOut);

		//							*arrEntryPtr = &variantArrStackData[i].objPtr;
									a->mov(ptr(a->zsi), a->zbx);

		//						}
								a->bind(lNoAutoUnboxNotOut);

		//					}

						a->jmp(lLoopFinal);
		//				} else if (((*flagsPtr) & ParamFlagDirectPtr) != 0) {
						a->bind(lNotMonoObjectPtr);
						a->test(ptr(a->zdi, 0, sizeof(variantflags_t)), Self::ParamFlagDirectPtr);
						a->jz(lLoopFinal);

		//					*arrEntryPtr = *((irmono_voidp*) *arrEntryPtr);
							a->mov(a->zax, ptr(a->zsi));
							a->mov(a->zax, ptr(a->zax));
							a->mov(ptr(a->zsi), a->zax);

		//				}
		//			}

					a->bind(lLoopFinal);

		//			arrEntryPtr += sizeof(irmono_voidp);
					a->add(a->zsi, sizeof(irmono_voidp));

		//		} while (((*flagsPtr++) & ParamFlagLastArrayElement)  ==  0);
				a->mov(a->zcx, ptr(a->zdi, 0, sizeof(variantflags_t)));
				a->add(a->zdi, sizeof(variantflags_t));
				a->test(a->zcx, Self::ParamFlagLastArrayElement);
				a->jz(lLoopStart);
				a->bind(lLoopEnd);

			a->jmp(lBuildEnd);
		//	} else {
			a->bind(lBlockPtrNull);

		//		rawArgs[rawArgIdx] = (irmono_voidp) 0;
				a->mov(ptrRawFuncArg<rawArgIdx>(ctx, 0, sizeof(irmono_voidp)), 0);

		//	}
			a->bind(lBuildEnd);
	}

	else if constexpr (
			std::is_base_of_v<std::string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u16string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u32string_view, typename ArgT::Type>
	) {
		//	rawArgs[rawArgIdx] = wrapArgs[wrapArgIdx]
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));
			a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zcx);
	}

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		Label lBuildEnd = a->newLabel();
		Label lNull = a->newLabel();

		//	uint8_t* blockPtr = wrapArgs[wrapArgIdx];
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx));

		//	if (blockPtr != nullptr) {
			a->jecxz(a->zcx, lNull);

				if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
		//			irmono_gchandle gchandle = *((irmono_gchandle*) blockPtr);
		//			IRMonoObjectPtrRaw objPtr = mono_gchandle_get_target_checked(gchandle);
					a->mov(ecx, ptr(a->zcx));
					genGchandleGetTargetChecked(ctx);

		//			curDynStackPtr -= sizeof(IRMonoObjectPtrRaw);
					a->sub(a->zbx, sizeof(IRMonoObjectPtrRaw));

		//			outMonoObjectPtr = objPtr;
					a->mov(ptr(a->zbx), a->zax);

		//			rawArgs[rawArgIdx] = &outMonoObjectPtr;
					a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zbx);
				} else {
		//			irmono_gchandle gchandle = (irmono_gchandle) wrapArgs[wrapArgIdx];
		//			rawArgs[rawArgIdx] = mono_gchandle_get_target_checked(gchandle);
					genGchandleGetTargetChecked(ctx);
					a->mov(ptrRawFuncArg<rawArgIdx>(ctx), a->zax);
				}

			a->jmp(lBuildEnd);
		//	} else {
			a->bind(lNull);

		//		rawArgs[rawArgIdx] = (irmono_voidp) 0;
				a->mov(ptrRawFuncArg<rawArgIdx>(ctx, 0, sizeof(irmono_voidp)), 0);

		//	}
			a->bind(lBuildEnd);
	}

	else {
		constexpr int32_t argSize = (int32_t) sizeof(std::tuple_element_t<rawArgIdx, typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple>);

		static_assert(argSize == sizeof(std::tuple_element_t<wrapArgIdx, typename RMonoAPIFunctionWrapTraits<CommonT>::WrapArgsTuple>),
				"Different non-special argument type size for wrap and raw functions is not supported.");

		// Arguments larger than sizeof(irmono_voidp) are passed as multiple stack entries (e.g. double or uint64_t on 32-bit).
		for (size_t partIdx = 0 ; partIdx*sizeof(irmono_voidp) < argSize ; partIdx++) {
			a->mov(a->zcx, ptrWrapFuncArg<wrapArgIdx>(ctx, partIdx));
			a->mov(ptrRawFuncArg<rawArgIdx>(ctx, partIdx), a->zcx);
		}
	}

	genWrapperBuildRawArg<wrapArgIdx+1, rawArgIdx+1, RestT...>(ctx, rest...);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t rawArgIdx>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperMoveStackArgsToRegsX64 (
		AsmBuildContext& ctx
) {
	using namespace asmjit;
	using namespace asmjit::host;

	typedef typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple RawArgsTuple;

	backend::RMonoAsmHelper& a = *ctx.a;

	assert(ctx.x64);

	if constexpr (
				rawArgIdx < 4
			&&  rawArgIdx < std::tuple_size_v<typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple>
	) {
		static const GpReg intRegs[] = { rcx, rdx, r8, r9 };
		static const XmmReg floatRegs[] = { xmm0, xmm1, xmm2, xmm3 };

		static_assert(sizeof(std::tuple_element_t<rawArgIdx, RawArgsTuple>) <= sizeof(irmono_voidp),
				"Raw argument larger than 8 bytes isn't supported for wrapper function on x64.");

		if constexpr(std::is_floating_point_v<std::tuple_element_t<rawArgIdx, RawArgsTuple>>) {
			a->movq(floatRegs[rawArgIdx], ptr(a->zsp, rawArgIdx*sizeof(irmono_voidp)));
		} else {
			a->mov(intRegs[rawArgIdx], ptr(a->zsp, rawArgIdx*sizeof(irmono_voidp)));
		}

		genWrapperMoveStackArgsToRegsX64<rawArgIdx+1>(ctx);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperHandleRetAndOutParams(AsmBuildContext& ctx)
{
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		Label lHandleEnd = a->newLabel();

		// TODO: This assumes that the raw function returns MonoObject**, and not MonoObject* directly. This is
		// what mono_array_addr_with_size() does, but are there functions that return it directly? Should we make
		// it configurable via a ReturnTag<>?

		//	variantflags_t flags = (variantflags_t) wrapArgs[0];
			a->mov(a->zcx, ptrWrapFuncArg<0>(ctx));

		//	if ((flags & ParamFlagMonoObjectPtr) != 0) {
			a->test(a->zcx, Self::ParamFlagMonoObjectPtr);
			a->jz(lHandleEnd);

		//		IRMonoObjectPtrRaw rawObj = *((IRMonoObjectPtrRaw*) rawRetval);
				a->mov(a->zcx, ptr(a->zax));

		//		rawRetval = mono_gchandle_new_checked(rawObj);
				genGchandleNewChecked(ctx);

		//	}
			a->bind(lHandleEnd);

		//	stackRetval = rawRetval;
			a->mov(ptr(a->zbp, ctx.stackOffsRetval), a->zax);

		// Skip first wrap argument (hidden Variant output param)
		genWrapperHandleOutParams<1, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename RetT::Type>) {
		//	stackRetval = mono_gchandle_new_checked(rawRetval);
			a->mov(a->zcx, a->zax);
			genGchandleNewChecked(ctx);
			a->mov(ptr(a->zbp, ctx.stackOffsRetval), a->zax);

		genWrapperHandleOutParams<0, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}

	else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		Label lHandleEnd = a->newLabel();
		Label lNull = a->newLabel();
		Label lStrlenLoopStart = a->newLabel();
		Label lStrlenLoopEnd = a->newLabel();

		// TODO: Maybe find and use a proper strlen() function if available.

		//	if (rawRetval != nullptr) {
			a->test(a->zax, a->zax);
			a->jz(lNull);

		//		irmono_voidp str = rawRetval;
				a->mov(a->zsi, a->zax);

		//		while ( *((CharTPtr) str) != 0 ) {
				a->bind(lStrlenLoopStart);
				a->cmp(ptr(a->zsi, 0, sizeof(typename RetT::Type::value_type)), 0);
				a->je(lStrlenLoopEnd);

		//			str += sizeof(CharT);
					if (sizeof(typename RetT::Type::value_type) == 1) {
						a->inc(a->zsi);
					} else {
						a->add(a->zsi, sizeof(typename RetT::Type::value_type));
					}

				a->jmp(lStrlenLoopStart);
		//		}
				a->bind(lStrlenLoopEnd);

		//		uint8_t* blockPtr = wrapArgs[0];
				a->mov(a->zcx, ptrWrapFuncArg<0>(ctx));

		//		*((uint32_t*) blockPtr) = (str-rawRetval) / sizeof(CharT);
				a->sub(a->zsi, a->zax);
				if (sizeof(typename RetT::Type::value_type) != 1) {
					a->shr(a->zsi, static_ilog2(sizeof(typename RetT::Type::value_type)));
				}
				a->mov(dword_ptr(a->zcx), esi);

		//		stackRetval = rawRetval;
				a->mov(ptr(a->zbp, ctx.stackOffsRetval), a->zax);

			a->jmp(lHandleEnd);
		//	} else {
			a->bind(lNull);

		//		stackRetval = 0;
				a->mov(ptr(a->zbp, ctx.stackOffsRetval, sizeof(irmono_voidp)), 0);

		//	}
			a->bind(lHandleEnd);

		// Skip first wrap argument (hidden dataBlockPtr parameter)
		genWrapperHandleOutParams<1, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}

	else if constexpr(std::is_same_v<typename RMonoAPIFunctionWrapTraits<CommonT>::WrapRetType, void>) {
		// Nothing to do for return value

		genWrapperHandleOutParams<0, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}

	else {
		//	stackRetval = rawRetval;
			a->mov(ptr(a->zbp, ctx.stackOffsRetval), a->zax);

		genWrapperHandleOutParams<0, 0, ArgsT...>(ctx, PackHelper<ArgsT>()...);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t wrapArgIdx, size_t rawArgIdx, typename ArgT, typename... RestT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genWrapperHandleOutParams (
		AsmBuildContext& ctx,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	using namespace asmjit;
	using namespace asmjit::host;

	backend::RMonoAsmHelper& a = *ctx.a;

	if constexpr(tags::has_param_tag_v<ArgT, tags::ParamOutRetClsTag>) {
		//	IRMonoObjectPtrRaw obj = mono_gchandle_get_target_checked(stackRetval);
			a->mov(a->zcx, ptr(a->zbp, ctx.stackOffsRetval));
			genGchandleGetTargetChecked(ctx);

		//	IRMonoClassPtr objCls = mono_object_get_class(obj);
			a->mov(a->zcx, a->zax);
			genObjectGetClass(ctx);

		//	*((irmono_voidp*) wrapArgs[wrapArgIdx]) = objCls;
			a->mov(a->zdx, ptrWrapFuncArg<wrapArgIdx>(ctx));
			a->mov(ptr(a->zdx), a->zax);

		genWrapperHandleOutParams<wrapArgIdx+1, rawArgIdx, RestT...>(ctx, rest...);

		return;
	} else if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		Label lHandleEnd = a->newLabel();

		//	uint8_t* blockPtr = wrapArgs[wrapArgIdx];
			a->mov(a->zdi, ptrWrapFuncArg<wrapArgIdx>(ctx));

		//	if (blockPtr != nullptr) {
			a->test(a->zdi, a->zdi);
			a->jz(lHandleEnd);

		//		// !!! Wrap argument points to payload, and flags are stored BEFORE the payload !!!
		//		variantflags_t flags = *((variantflags_t*) (blockPtr - sizeof(variantflags_t)));
				a->movzx(a->zcx, ptr(a->zdi, - (int32_t) sizeof(variantflags_t), sizeof(variantflags_t)));

		//		if ((flags & ParamFlagMonoObjectPtr) != 0) {
				a->test(a->zcx, Self::ParamFlagMonoObjectPtr);
				a->jz(lHandleEnd);

		//			curDynStackPtr -= sizeof(IRMonoObjectPtrRaw);
					a->sub(a->zbx, sizeof(IRMonoObjectPtrRaw));

		//			if ((flags & ParamFlagOut) != 0) {
					a->test(a->zcx, Self::ParamFlagOut);
					a->jz(lHandleEnd);

						if constexpr (
								tags::has_param_tag_v<ArgT, tags::ParamOutTag>
							||	tags::has_param_tag_v<ArgT, tags::ParamOvwrInOutTag>
						) {
		//					irmono_gchandle gchandle = mono_gchandle_new_checked(variantDummyPtr);
							a->mov(a->zcx, ptr(a->zbx));
							genGchandleNewChecked(ctx);

		//					*((irmono_gchandle*) blockPtr) = gchandle;
							a->mov(dword_ptr(a->zdi), eax);
						}

		//			}

		//		}

		//	}
			a->bind(lHandleEnd);
	}

	else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		Label lHandleEnd = a->newLabel();
		Label lLoopStart = a->newLabel();
		Label lLoopFinal = a->newLabel();
		Label lLoopEnd = a->newLabel();

		//	blockPtr = wrapArgs[wrapArgIdx];
			a->mov(a->zdi, ptrWrapFuncArg<wrapArgIdx>(ctx));

		//	if (blockPtr != nullptr) {
			a->test(a->zdi, a->zdi);
			a->jz(lHandleEnd);

				if constexpr (
						tags::has_param_tag_v<ArgT, tags::ParamOutTag>
					||	tags::has_param_tag_v<ArgT, tags::ParamOvwrInOutTag>
				) {
		//			uint32_t i = 0;
					a->xor_(esi, esi);

		//			while (i < *((uint32_t*) blockPtr)) {
					a->bind(lLoopStart);
					a->cmp(esi, dword_ptr(a->zdi));
					a->je(lLoopEnd);

		//				curDynStackPtr -= sizeof(VariantArrayStackEntry);
						a->sub(a->zbx, 2*sizeof(IRMonoObjectPtrRaw));

		//				if (variantArrStackData[i].origArrPtr != nullptr) {
						a->cmp(ptr(a->zbx, sizeof(IRMonoObjectPtrRaw), sizeof(irmono_voidp)), 0);
						a->je(lLoopFinal);

		//					irmono_gchandle gchandle = mono_gchandle_new_checked(variantArrStackData[i].objPtr);
							a->mov(a->zcx, ptr(a->zbx));
							genGchandleNewChecked(ctx);

		//					*((irmono_gchandle*) variantArrStackData[i].origArrPtr) = gchandle;
							a->mov(a->zcx, ptr(a->zbx, sizeof(IRMonoObjectPtrRaw)));
							a->mov(dword_ptr(a->zcx), eax);

		//				}

		//				i++;
						a->bind(lLoopFinal);
						a->inc(esi);
						a->jmp(lLoopStart);

		//			}
					a->bind(lLoopEnd);
				} else {
		//			curDynStackPtr -= *((uint32_t*) blockPtr) * sizeof(VariantArrayStackEntry);
					a->mov(a->zcx, dword_ptr(a->zdi));
					a->shl(a->zcx, static_ilog2(2*sizeof(IRMonoObjectPtrRaw)));
					a->sub(a->zbx, a->zcx);
				}

		//	}
			a->bind(lHandleEnd);
	}

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			Label lHandleEnd = a->newLabel();

			//	uint8_t* blockPtr = wrapArgs[wrapArgIdx];
				a->mov(a->zdi, ptrWrapFuncArg<wrapArgIdx>(ctx));

			//	if (blockPtr != nullptr) {
				a->test(a->zdi, a->zdi);
				a->jz(lHandleEnd);

			//		curDynStackPtr -= sizeof(IRMonoObjectPtrRaw);
					a->sub(a->zbx, sizeof(IRMonoObjectPtrRaw));

			//		irmono_gchandle gchandle = mono_gchandle_new_checked(outMonoObjectPtr);
					a->mov(a->zcx, ptr(a->zbx));
					genGchandleNewChecked(ctx);

			//		*((irmono_gchandle*) blockPtr) = gchandle;
					a->mov(dword_ptr(a->zdi), eax);

			//	}
				a->bind(lHandleEnd);
		}
	}

	genWrapperHandleOutParams<wrapArgIdx+1, rawArgIdx+1, RestT...>(ctx, rest...);
}






template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genGchandleGetTargetChecked(AsmBuildContext& ctx)
{
	// NOTE: Always expects a MonoGCHandle in zcx.

	AsmGenGchandleGetTargetChecked(*ctx.a, ctx.gchandleGetTargetAddr, ctx.x64);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genGchandleNewChecked(AsmBuildContext& ctx)
{
	// NOTE: Always expects a MonoObjectPtrRaw in zcx.

	AsmGenGchandleNewChecked(*ctx.a, ctx.gchandleNewAddr, ctx.x64);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genIsValueTypeInstance(AsmBuildContext& ctx)
{
	AsmGenIsValueTypeInstance(*ctx.a, ctx.objectGetClassAddr, ctx.classIsValuetypeAddr, ctx.x64);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genObjectUnbox(AsmBuildContext& ctx)
{
	AsmGenObjectUnbox(*ctx.a, ctx.objectUnboxAddr, ctx.x64);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...>::genObjectGetClass(AsmBuildContext& ctx)
{
	AsmGenObjectGetClass(*ctx.a, ctx.objectGetClassAddr, ctx.x64);
}


}
