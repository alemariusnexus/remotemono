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

#include <string>
#include "RMonoAPIFunctionTypeAdapters.h"
#include "RMonoAPIFunctionSimple_Def.h"
#include "RMonoAPIFunctionRaw_Def.h"
#include "RMonoAPIFunctionWrap_Def.h"
#include "RMonoAPIFunctionAPI_Def.h"
#include "RMonoAPIBase_Def.h"



namespace remotemono
{





/**
 * This clusterfuck of classes (all of the RMonoAPIFunction* classes) is used to represent a Mono API function as
 * it is used by RemoteMono. This class (and RMonoAPIFunction, and everything in between) is the umbrella class,
 * and RMonoAPIFunctionRaw, RMonoAPIFunctionWrap and RMonoAPIFunctionAPI are the components that provide most of
 * the actual functionality. See their respective documentation for more details.
 *
 * All of these classes take an ABI as template parameter, because they are ABI-specific. RMonoAPIDispatcher and
 * RMonoAPI are used to abstract away to low-level ABI details by selecting the correct ABI-specific instance.
 */
template <typename ABI, bool required, typename RetT, typename... ArgsT>
class RMonoAPIFunctionBase :
		public RMonoAPIFunctionRaw<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>, ABI, RetT, ArgsT...>,
		public RMonoAPIFunctionWrap<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>, ABI, RetT, ArgsT...>,
		public RMonoAPIFunctionAPI<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>, ABI, RetT, ArgsT...>
{
public:
	typedef RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...> Self;

	typedef RetT DefRetType;
	typedef std::tuple<ArgsT...> DefArgsTuple;

public:
	constexpr static bool isRequired()
	{
		return required;
	}

public:
	RMonoAPIFunctionBase() : abi(nullptr), mono(nullptr) {}

	/**
	 * Reset all the function pointers and start over as if creating a fresh object.
	 */
	void reset()
	{
		resetRaw();
		resetWrap();
		resetAPI();

		abi = nullptr;
		mono = nullptr;
		name.clear();
	}

	/**
	 * Initialize some general function data and the RMonoAPIFunctionRaw component.
	 *
	 * @param abi The ABI object to use for this function.
	 * @param mono The RMonoAPI instance to use for the function.
	 * @param name The function's full name.
	 * @param rawFuncAddr The address to the raw function in remote memory.
	 */
	void init(ABI* abi, RMonoAPIBase* mono, const std::string& name, blackbone::ptr_t rawFuncAddr)
	{
		this->abi = abi;
		this->mono = mono;
		this->name = name;
		this->initRaw(rawFuncAddr);
	}

	/**
	 * Initialize the function to be invalid. This way, the invalid function can still have its name.
	 *
	 * @param name The function's full name.
	 */
	void initInvalid(const std::string& name)
	{
		this->name = name;
	}

	/**
	 * Compile the RMonoAPIFunctionWrap component, i.e. generate the wrapper function's assembly code.
	 *
	 * @param a The IAsmHelper to use for assembly generation.
	 * @return The label to the function's start.
	 */
	asmjit::Label compile(blackbone::IAsmHelper& a)
	{
		return this->compileWrap(a);
	}

	/**
	 * Link the RMonoAPIFunctionWrap component, i.e. provide it with the address of the generated wrapper function
	 * in remote memory.
	 *
	 * @param wrapFuncAddr The wrapper function's remote address.
	 */
	void link(blackbone::ptr_t wrapFuncAddr)
	{
		this->linkWrap(wrapFuncAddr);
	}

	/**
	 * Log the signatures of the different function versions (definition, raw, wrap, API).
	 */
	void debugDumpSignatures();

	/**
	 * Return the function's ABI object.
	 */
	ABI* getABI() { return abi; }

	/**
	 * Return the function's RMonoAPI object.
	 */
	RMonoAPIBase* getRemoteMonoAPI() { return mono; }

	/**
	 * Return the function's full name.
	 */
	std::string getName() const { return name; }

	/**
	 * Determine if the function is valid, i.e. if at least a valid remote raw function address was provided with init().
	 */
	operator bool() const { return (bool) rawFunc; }

private:
	template <typename ArgsTupleT, size_t argIdx>
	void debugDumpSignaturesArgRecurse(std::string& sigStr);

private:
	ABI* abi;
	RMonoAPIBase* mono;
	std::string name;
};





// These methods can be used to access types defined in the component classes without the umbrella class having been fully defined yet.

template <typename ABI, bool required, typename RetT, typename... ArgsT>
struct RMonoAPIFunctionCommonTraits<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>>
{
	typedef RetT DefRetType;
	typedef std::tuple<ArgsT...> DefArgsTuple;
};

template <typename ABI, bool required, typename RetT, typename... ArgsT>
struct RMonoAPIFunctionRawTraits<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>>
{
	typedef RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...> CommonType;

	typedef RMonoAPIFunctionRaw<CommonType, ABI, RetT, ArgsT...> RawComponent;

	typedef typename RawComponent::RawRetType RawRetType;
	typedef typename RawComponent::RawArgsTuple RawArgsTuple;
};

template <typename ABI, bool required, typename RetT, typename... ArgsT>
struct RMonoAPIFunctionWrapTraits<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>>
{
	typedef RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...> CommonType;

	typedef RMonoAPIFunctionWrap<CommonType, ABI, RetT, ArgsT...> WrapComponent;

	typedef typename WrapComponent::WrapRetType WrapRetType;
	typedef typename WrapComponent::WrapArgsTuple WrapArgsTuple;
};

template <typename ABI, bool required, typename RetT, typename... ArgsT>
struct RMonoAPIFunctionAPITraits<RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>>
{
	typedef RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...> CommonType;

	typedef RMonoAPIFunctionAPI<CommonType, ABI, RetT, ArgsT...> APIComponent;

	typedef typename APIComponent::APIRetType APIRetType;
	typedef typename APIComponent::APIArgsTuple APIArgsTuple;
};






/**
 * Adds ReturnNull<> and ParamNull<> tags to each return type and argument, to make their handling easier later.
 */
template <typename ABI, bool required, typename RetT, typename... ArgsT>
class RMonoAPIFunctionAutoAddNullTags : public RMonoAPIFunctionBase <
		ABI,
		required,
		typename tags::ReturnNull<RetT>,
		typename tags::ParamNull<ArgsT>...
		> {};


/**
 * Documentation for this class is at RMonoAPIFunctionBase.
 *
 * @see RMonoAPIFunctionBase.
 */
template <typename ABI, bool required, typename RetT, typename... ArgsT>
using RMonoAPIFunction = RMonoAPIFunctionAutoAddNullTags<ABI, required, RetT, ArgsT...>;



}




