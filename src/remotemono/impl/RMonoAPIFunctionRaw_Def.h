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
#include "RMonoAPIFunctionTypeAdapters.h"
#include "RMonoAPIFunctionSimple_Def.h"
#include "RMonoAPIFunctionCommon_Def.h"


namespace remotemono
{



// **************************************************************
// *															*
// *						BASE CLASS							*
// *															*
// **************************************************************


/**
 * Represents the raw Mono API function part of a RMonoAPIFunction object. Mainly provides a way using invokeRaw() to
 * directly invoke the raw remote function without any wrapper functionality or type conversions.
 */
template <class CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionRawBase : public RMonoAPIFunctionCommon<ABI>
{
public:
	typedef RetT RawRetType;
	typedef std::tuple<ArgsT...> RawArgsTuple;

	typedef RMonoAPIFunctionSimple<RetT, ArgsT...> RawFunc;

public:
	RMonoAPIFunctionRawBase() {}
	~RMonoAPIFunctionRawBase() {}

	void initRaw(rmono_funcp rawFuncAddr)
	{
		this->rawFunc.rebuild(getRemoteMonoAPI()->getProcess(), rawFuncAddr);
	}

	RetT invokeRaw(ArgsT... args) { return rawFunc(args...); }

	rmono_funcp getRawFuncAddress() const { return rawFunc.getAddress(); }

private:
	RMonoAPIBase* getRemoteMonoAPI() { return static_cast<CommonT*>(this)->getRemoteMonoAPI(); }

protected:
	RawFunc rawFunc;
};









// **************************************************************
// *															*
// *						ADAPTER CHAIN						*
// *															*
// **************************************************************

// Can be used to adapt the definition types into the API types, e.g. by adding, changing or removing arguments.

template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionRawAdapterFinal : public RMonoAPIFunctionRawBase <
		CommonT,
		ABI,
		typename RMonoAPIReturnTypeAdapter<ABI, RetT>::RawType,
		typename RMonoAPIParamTypeAdapter<ABI, ArgsT>::RawType...
		> {};


template <typename Enable, typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionRawAdapter : public RMonoAPIFunctionRawAdapterFinal<CommonT, ABI, RetT, ArgsT...> {};






// **************************************************************
// *															*
// *						FRONT CLASS							*
// *															*
// **************************************************************


/**
 * Documentation for this class is at RMonoAPIFunctionRawBase
 *
 * @see RMonoAPIFunctionRawBase
 */
template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
class RMonoAPIFunctionRaw : public RMonoAPIFunctionRawAdapter<void, CommonT, ABI, RetT, ArgsT...>
{
protected:
	void resetRaw()
	{
		rawFunc.reset();
	}
};



}
