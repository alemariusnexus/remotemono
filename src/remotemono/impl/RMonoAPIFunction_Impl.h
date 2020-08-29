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

#include "RMonoAPIFunction_Def.h"

#include "../log.h"
#include "../util.h"




namespace remotemono
{



template <typename ABI, bool required, typename RetT, typename... ArgsT>
void RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>::debugDumpSignatures()
{
	std::string defSigStr = std::string(typeid(RetT).name()).append(" ").append(name).append("(");
	std::string rawSigStr = std::string(typeid(typename RMonoAPIFunctionRawTraits<Self>::RawRetType).name()).append(" ").append(name).append("(");
	std::string wrapSigStr = std::string(typeid(typename RMonoAPIFunctionWrapTraits<Self>::WrapRetType).name()).append(" ").append(name).append("(");
	std::string apiSigStr = std::string(typeid(typename RMonoAPIFunctionAPITraits<Self>::APIRetType).name()).append(" ").append(name).append("(");

	debugDumpSignaturesArgRecurse<DefArgsTuple, 0>(defSigStr);
	debugDumpSignaturesArgRecurse<typename RMonoAPIFunctionRawTraits<Self>::RawArgsTuple, 0>(rawSigStr);
	debugDumpSignaturesArgRecurse<typename RMonoAPIFunctionWrapTraits<Self>::WrapArgsTuple, 0>(wrapSigStr);
	debugDumpSignaturesArgRecurse<typename RMonoAPIFunctionAPITraits<Self>::APIArgsTuple, 0>(apiSigStr);

	defSigStr.append(")");
	rawSigStr.append(")");
	wrapSigStr.append(")");
	apiSigStr.append(")");

	RMonoLogVerbose("Signatures for '%s':", name.data());
	//RMonoLogVerbose("    Def:    %s", defSigStr.data());
	RMonoLogVerbose("    Raw:    %s", rawSigStr.data());
	RMonoLogVerbose("    Wrap:   %s", wrapSigStr.data());
	RMonoLogVerbose("    API:    %s", apiSigStr.data());
}


template <typename ABI, bool required, typename RetT, typename... ArgsT>
template <typename ArgsTupleT, size_t argIdx>
void RMonoAPIFunctionBase<ABI, required, RetT, ArgsT...>::debugDumpSignaturesArgRecurse (
		std::string& sigStr
) {
	if constexpr(argIdx < std::tuple_size_v<ArgsTupleT>) {
		if constexpr(argIdx != 0) {
			sigStr.append(", ");
		}

		sigStr.append(qualified_type_name<std::tuple_element_t<argIdx, ArgsTupleT>>());

		debugDumpSignaturesArgRecurse<ArgsTupleT, argIdx+1>(sigStr);
	}
}



}
