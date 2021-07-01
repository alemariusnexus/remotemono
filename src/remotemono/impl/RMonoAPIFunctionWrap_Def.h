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

#include <type_traits>
#include <tuple>
#include "RMonoAPIBase_Def.h"
#include "RMonoAPIFunctionTypeAdapters.h"
#include "RMonoAPIFunctionSimple_Def.h"
#include "RMonoAPIFunctionCommon_Def.h"
#include "abi/RMonoABITypeTraits.h"
#include "backend/RMonoAsmHelper.h"



namespace remotemono
{



// **************************************************************
// *															*
// *						BASE CLASS							*
// *															*
// **************************************************************


/**
 * A component of RMonoAPIFunction, this class provides the wrapper functions around a raw Mono API function. Wrapper
 * functions are generated at runtime using AsmJit and then injected into the remote process (just copied over into
 * executable memory).
 *
 * Wrapper functions are necessary for many Mono API functions for the following reasons:
 *
 * 1. For MonoObject* parameters, we pass GC handles instead of raw pointers. To be GC-safe, we only convert the
 *    GC handles back to raw pointers in the remote process, and this conversion (and the one in the opposite
 *    direction) is what these wrapper functions do. We must do it in the remote process because the Mono GC is
 *    only guaranteed not to move the memory around randomly as long as a raw pointer to that memory is either
 *    in a CPU register or on the stack of a Mono-attached thread in the remote process.
 * 2. Some other parameter types can be more easily and/or efficiently handled if we can do a few more things
 *    before or after the actual API call, like calculating the length of a returned string.
 *
 * This class contains code to generate general wrapper functions depending on the signature of the Mono API
 * function. To do this, it makes *heavy* use of template metaprogramming and it requires certain additional information
 * about parameter types to be encoded into the function signature (this is what the tag classes like ParamOut<> or
 * ReturnOwn<> are for). This class is also heavily tied to the implementation of RMonoAPIFunctionAPI, which is
 * responsible for building the specific parameters and the data block required by the generated wrappers.
 *
 * Note that not every Mono API function will need (or have) a wrapper function. For example, functions that only handle
 * fundamental data types and/or pointers to non-managed resources will often not have a wrapper. For those functions,
 * RemoteMono will just call the raw function.
 *
 * @see RMonoAPIFunctionAPIBase
 */
template <class CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapBase : public RMonoAPIFunctionCommon<ABI>
{
public:
	typedef RetT WrapRetType;
	typedef std::tuple<ArgsT...> WrapArgsTuple;

	typedef RMonoAPIFunctionSimple<RetT, ArgsT...> WrapFunc;

public:
	RMonoAPIFunctionWrapBase() {}
	~RMonoAPIFunctionWrapBase() {}

	void linkWrap(rmono_funcp wrapFuncAddr)
	{
		this->wrapFunc.rebuild(getRemoteMonoAPI()->getProcess(), wrapFuncAddr);
	}

	RetT invokeWrap(ArgsT... args) { return wrapFunc(args...); }

	rmono_funcp getWrapFuncAddress() const { return wrapFunc.getAddress(); }

private:
	ABI* getABI() { return static_cast<CommonT*>(this)->getABI(); }
	RMonoAPIBase* getRemoteMonoAPI() { return static_cast<CommonT*>(this)->getRemoteMonoAPI(); }

protected:
	WrapFunc wrapFunc;
};







// **************************************************************
// *															*
// *						ADAPTER CHAIN						*
// *															*
// **************************************************************

// Can be used to adapt the definition types into the API types, e.g. by adding, changing or removing arguments.

template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapterFinal : public RMonoAPIFunctionWrapBase <
		CommonT,
		ABI,
		typename RMonoAPIReturnTypeAdapter<ABI, RetT>::WrapType,
		typename RMonoAPIParamTypeAdapter<ABI, ArgsT>::WrapType...
		> {};


template <typename Enable, typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapterRetToOutParam;

// Transform Variant return type to output parameter
template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapterRetToOutParam <
		std::enable_if_t<std::is_base_of_v<RMonoVariant, typename RetT::Type>>,
		CommonT, ABI, RetT, ArgsT...
	> : public RMonoAPIFunctionWrapAdapterFinal<CommonT, ABI,
			tags::ReturnNull<typename RMonoABITypeTraits<ABI>::irmono_voidp>, tags::ParamOut<RMonoVariant>, ArgsT...> {};

// Add hidden dataBlockPtr parameter for string return types
template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapterRetToOutParam <
		std::enable_if_t <
				std::is_base_of_v<std::string, typename RetT::Type>
			||	std::is_base_of_v<std::u16string, typename RetT::Type>
			||	std::is_base_of_v<std::u32string, typename RetT::Type>
			>,
		CommonT, ABI, RetT, ArgsT...
	> : public RMonoAPIFunctionWrapAdapterFinal<CommonT, ABI, RetT, tags::ParamNull<typename RMonoABITypeTraits<ABI>::irmono_voidp>, ArgsT...> {};

template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapterRetToOutParam <
		std::enable_if_t <
				!std::is_base_of_v<RMonoVariant, typename RetT::Type>
			&&	!std::is_base_of_v<std::string, typename RetT::Type>
			&&	!std::is_base_of_v<std::u16string, typename RetT::Type>
			&&	!std::is_base_of_v<std::u32string, typename RetT::Type>
			>,
		CommonT, ABI, RetT, ArgsT...
	> : public RMonoAPIFunctionWrapAdapterFinal<CommonT, ABI, RetT, ArgsT...> {};


template <typename Enable, typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrapAdapter : public RMonoAPIFunctionWrapAdapterRetToOutParam<void, CommonT, ABI, RetT, ArgsT...> {};







// **************************************************************
// *															*
// *						FRONT CLASS							*
// *															*
// **************************************************************

/**
 * Documentation of this class is at RMonoAPIFunctionWrapBase
 *
 * @see RMonoAPIFunctionWrapBase
 */
template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionWrap : public RMonoAPIFunctionWrapAdapter<void, CommonT, ABI, RetT, ArgsT...>
{
private:
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(ABITypeTraits)

	typedef RMonoVariant Variant;
	typedef RMonoVariantArray VariantArray;

	typedef typename RMonoAPIFunctionCommon<ABI>::variantflags_t variantflags_t;

	typedef typename RMonoAPIFunctionCommonTraits<CommonT>::DefRetType DefRetType;
	typedef typename RMonoAPIFunctionCommonTraits<CommonT>::DefArgsTuple DefArgsTuple;

	struct AsmBuildContext
	{
		backend::RMonoAsmHelper* a;

		bool x64;

		rmono_funcp gchandleGetTargetAddr;
		rmono_funcp gchandleNewAddr;
		rmono_funcp objectGetClassAddr;
		rmono_funcp classIsValuetypeAddr;
		rmono_funcp objectUnboxAddr;

		int32_t regSize;
		int32_t rawArgStackSize;

		// ZBP-relative offsets
		int32_t stackOffsArgBase;
		int32_t stackOffsRetval;
	};

public:
	typedef RMonoAPIFunctionWrap<CommonT, ABI, RetT, ArgsT...> Self;

public:
	template <size_t idx = 0>
	constexpr static std::enable_if_t<idx < std::tuple_size_v<DefArgsTuple>, bool> needsWrapFuncArg()
	{
		typedef std::tuple_element_t<idx, DefArgsTuple> ArgType;

		// TODO: We don't actually need a wrapper for primitive ParamOut<> arguments. A raw call would be enough, but
		// we'd need to allocate memory for the output, which is why we'll currently just let the wrapper logic handle
		// such arguments.
		if constexpr (
				std::is_base_of_v<Variant, typename ArgType::Type>
			||	std::is_base_of_v<VariantArray, typename ArgType::Type>
			||	std::is_base_of_v<std::string_view, typename ArgType::Type>
			||	std::is_base_of_v<std::u16string_view, typename ArgType::Type>
			||	std::is_base_of_v<std::u32string_view, typename ArgType::Type>
			||	std::is_base_of_v<RMonoObjectHandleTag, typename ArgType::Type>
			||	tags::has_param_tag_v<ArgType, tags::ParamOutTag>
		) {
			return true;
		} else {
			return needsWrapFuncArg<idx+1>();
		}
	}

	template <size_t idx = 0>
	constexpr static std::enable_if_t<idx == std::tuple_size_v<DefArgsTuple>, bool> needsWrapFuncArg() { return false; }

	/**
	 * Determines if a wrapper function is even needed for this API function.
	 */
	constexpr static bool needsWrapFunc()
	{
		if constexpr (
				std::is_base_of_v<Variant, typename DefRetType::Type>
			||	std::is_base_of_v<VariantArray, typename DefRetType::Type>
			||	std::is_base_of_v<std::string, typename DefRetType::Type>
			||	std::is_base_of_v<std::u16string, typename DefRetType::Type>
			||	std::is_base_of_v<std::u32string, typename DefRetType::Type>
			||	std::is_base_of_v<RMonoObjectHandleTag, typename DefRetType::Type>
		) {
			return true;
		} else {
			return needsWrapFuncArg<0>();
		}
	}

public:
	asmjit::Label compileWrap(backend::RMonoAsmHelper& a);

protected:
	void resetWrap()
	{
		this->wrapFunc.reset();
	}

private:
	void generateWrapperAsm(AsmBuildContext& ctx);


	template <size_t wrapArgIdx>
	void genWrapperSpillArgsToStackX64(AsmBuildContext& ctx);


	void genWrapperReserveStack(AsmBuildContext& ctx);

	template <size_t wrapArgIdx, typename ArgT, typename... RestT>
	void genWrapperReserveArgStack(AsmBuildContext& ctx, PackHelper<ArgT>, PackHelper<RestT>... rest);
	template <size_t wrapArgIdx>
	void genWrapperReserveArgStack(AsmBuildContext& ctx) {}

	template <size_t rawArgIdx>
	void genWrapperCalculateRawArgStackSize(AsmBuildContext& ctx);


	void genWrapperBuildRawArgs(AsmBuildContext& ctx);

	template <size_t wrapArgIdx, size_t rawArgIdx, typename ArgT, typename... RestT>
	void genWrapperBuildRawArg(AsmBuildContext& ctx, PackHelper<ArgT>, PackHelper<RestT>... rest);
	template <size_t wrapArgIdx, size_t rawArgIdx>
	void genWrapperBuildRawArg(AsmBuildContext& ctx) {}


	template <size_t rawArgIdx>
	void genWrapperMoveStackArgsToRegsX64(AsmBuildContext& ctx);


	void genWrapperHandleRetAndOutParams(AsmBuildContext& ctx);

	template <size_t wrapArgIdx, size_t rawArgIdx, typename ArgT, typename... RestT>
	void genWrapperHandleOutParams(AsmBuildContext& ctx, PackHelper<ArgT>, PackHelper<RestT>... rest);
	template <size_t wrapArgIdx, size_t rawArgIdx>
	void genWrapperHandleOutParams(AsmBuildContext& ctx) {}


	void genGchandleGetTargetChecked(AsmBuildContext& ctx);
	void genGchandleNewChecked(AsmBuildContext& ctx);
	void genIsValueTypeInstance(AsmBuildContext& ctx);
	void genObjectUnbox(AsmBuildContext& ctx);
	void genObjectGetClass(AsmBuildContext& ctx);

	template <size_t argIdx, typename ArgsTupleT>
	constexpr size_t calcStackArgOffset()
	{
		if constexpr(argIdx == 0) {
			return 0;
		} else {
			return static_align(sizeof(std::tuple_element_t<argIdx-1, ArgsTupleT>), sizeof(irmono_voidp))
					+ calcStackArgOffset<argIdx-1, ArgsTupleT>();
		}
	}

	template <size_t argIdx>
	asmjit::X86Mem ptrWrapFuncArg(AsmBuildContext& ctx, size_t partIdx = 0, uint32_t size = 0)
	{
		return asmjit::host::ptr (
				(*ctx.a)->zbp,
				int32_t (
						ctx.stackOffsArgBase
						+ calcStackArgOffset<argIdx, typename RMonoAPIFunctionWrapTraits<CommonT>::WrapArgsTuple>()
						+ partIdx*sizeof(irmono_voidp)
						),
				size
				);
	}

	template <size_t argIdx>
	asmjit::X86Mem ptrRawFuncArg(AsmBuildContext& ctx, size_t partIdx = 0, uint32_t size = 0)
	{
		return asmjit::host::ptr (
				(*ctx.a)->zsp,
				int32_t (
						calcStackArgOffset<argIdx, typename RMonoAPIFunctionRawTraits<CommonT>::RawArgsTuple>()
						+ partIdx*sizeof(irmono_voidp)
						),
				size
				);
	}

private:
	ABI* getABI() { return static_cast<CommonT*>(this)->getABI(); }
	RMonoAPIBase* getRemoteMonoAPI() { return static_cast<CommonT*>(this)->getRemoteMonoAPI(); }
};


}


