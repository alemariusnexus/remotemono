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

#include <tuple>
#include <type_traits>
#include <cstdlib>
#include "abi/RMonoABI.h"
#include "RMonoAPIBackend_Def.h"




namespace remotemono
{



class RMonoAPIDispatcherBase
{
public:
	/**
	 * A single ABI and its corresponding RMonoAPIBackend.
	 */
	template <typename ABI>
	struct ABIEntry
	{
		typedef ABI ABIType;

		ABIEntry() : api(&abi) {}

		/**
		 * The given ABI. It is an object of one of the classes registered in RMonoSupportedABITuple.
		 */
		ABI abi;

		/**
		 * The RMonoAPIBackend for the given ABI.
		 */
		RMonoAPIBackend<ABI> api;
	};

protected:
	static auto genABIEntryTupleDummy()
	{
		return std::apply([](auto... ts) {
			return std::tuple<ABIEntry<decltype(ts)>...>();
		}, RMonoSupportedABITuple());
	}
};



/**
 * A helper class for selecting between different RMonoAPIBackend instances for the supported ABIs. Usually, one ABI
 * will be selected using selectABI(), and then apply() can be called to run a function with the selected ABI.
 */
class RMonoAPIDispatcher : public RMonoAPIDispatcherBase
{
public:
	typedef decltype(RMonoAPIDispatcherBase::genABIEntryTupleDummy()) ABIEntryTuple;

public:
	RMonoAPIDispatcher() : selectedABIIdx(std::tuple_size_v<ABIEntryTuple>) {}
	~RMonoAPIDispatcher() {}

	ABIEntryTuple& getABIEntries() { return abis; }


	/**
	 * @return true if an ABI has been selected by selectABI(), false otherwise.
	 */
	bool hasSelectedABI() const
	{
		return selectedABIIdx < std::tuple_size_v<ABIEntryTuple>;
	}

	/**
	 * Select the given ABI to be used for methods like apply().
	 */
	template <typename ABI>
	void selectABI()
	{
		selectedABIIdx = abiIndexOf<ABI>();
	}


	/**
	 * Run the given function on **all** registered ABI, not just the selected one.
	 */
	template <typename FuncT, size_t idx = 0>
	typename std::enable_if_t<idx < std::tuple_size_v<ABIEntryTuple>, void> foreach(FuncT f)
	{
		f(std::get<idx>(abis));
		foreach<FuncT, idx+1>(f);
	}

	/**
	 * Run the given function on **all** registered ABI, not just the selected one.
	 */
	template <typename FuncT, size_t idx = 0>
	typename std::enable_if_t<idx == std::tuple_size_v<ABIEntryTuple>, void> foreach(FuncT f) {}



	/**
	 * Run the given function only on the currently selected ABI. The function will be called only once with a single
	 * RMonoAPIDispatcherBase::ABIEntry from which you can get the RMonoAPIBackend and the ABI itself.
	 */
	template <typename FuncT>
	auto apply(FuncT f) -> decltype(applyInternal<FuncT, 0>(f))
	{
		return applyInternal<FuncT, 0>(f);
	}

private:
	template<typename ABI, size_t idx = 0>
	constexpr std::enable_if_t<idx < std::tuple_size_v<ABIEntryTuple>, size_t> abiIndexOf()
	{
		if constexpr(std::is_same_v<typename std::tuple_element_t<idx, ABIEntryTuple>::ABIType, ABI>) {
			return idx;
		} else {
			return abiIndexOf<ABI, idx+1>();
		}
	}

	template<typename ABI, size_t idx = 0>
	constexpr std::enable_if_t<idx == std::tuple_size_v<ABIEntryTuple>, size_t> abiIndexOf() { static_assert(false); return 0; }

	template <typename FuncT, size_t idx>
	auto applyInternal(FuncT f) -> decltype(f(std::get<0>(abis)))
	{
		if constexpr(idx < std::tuple_size_v<ABIEntryTuple>) {
			if (idx == selectedABIIdx) {
				return f(std::get<idx>(abis));
			} else {
				return applyInternal<FuncT, idx+1>(f);
			}
		} else {
			assert(false);
			return f(std::get<0>(abis)); // Should never happen, but needed to please the compiler
		}
	}


private:
	ABIEntryTuple abis;
	size_t selectedABIIdx;
};



}
