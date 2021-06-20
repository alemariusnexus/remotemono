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

#include <vector>
#include "RMonoVariant_Def.h"



namespace remotemono
{



class RMonoVariant;



/**
 * An array of RMonoVariant objects.
 *
 * This class is used in certain places where the raw Mono API has parameters of type `void**` representing arrays
 * of Mono/.NET reference or value type instances, specifically in `mono_runtime_invoke()` or `mono_property_get_value()`.
 *
 * @see RMonoVariant
 */
class RMonoVariantArray
{
public:
	typedef RMonoVariantArray Self;

	typedef RMonoVariant Variant;

public:
	/**
	 * Construct an empty, non-null variant array.
	 */
	RMonoVariantArray() : isnull(false) {}

	/**
	 * Construct an empty, null variant array.
	 */
	RMonoVariantArray(std::nullptr_t) : isnull(true) {}


	/**
	 * Copy constructor.
	 */
	RMonoVariantArray(const Self& other) : vec(other.vec), isnull(other.isnull) {}

	/**
	 * Move constructor.
	 */
	RMonoVariantArray(Self&& other) : vec(std::move(other.vec)), isnull(other.isnull) { other.isnull = true; }


	/**
	 * Creates the array from an std::vector of RMonoVariant objects.
	 */
	RMonoVariantArray(const std::vector<Variant>& vec) : vec(vec), isnull(false) {}

	/**
	 * Creates the array from an std::vector of RMonoVariant objects.
	 */
	RMonoVariantArray(std::vector<Variant>&& vec) : vec(vec), isnull(false) {}


	/**
	 * Creates the array from an std::initializer_list of RMonoVariant objects.
	 */
	RMonoVariantArray(std::initializer_list<Variant>&& list) : vec(list), isnull(false) {}


	/**
	 * Creates the array from a parameter pack of RMonoVariant objects.
	 */
	template <typename... VariantT>
	RMonoVariantArray(VariantT... list) : vec({list...}), isnull(false) {}


	/**
	 * Assignment operator.
	 */
	Self& operator=(const Self& other)
	{
		if (&other != this) {
			vec = other.vec;
			isnull = other.isnull;
		}
		return *this;
	}

	/**
	 * Move-assignment operator.
	 */
	Self& operator=(Self&& other)
	{
		if (&other != this) {
			vec = std::move(other.vec);
			isnull = other.isnull;
			other.isnull = true;
		}
		return *this;
	}


	/**
	 * Returns the number of elements in the array.
	 */
	size_t size() const { return vec.size(); }

	/**
	 * Returns true if this is a null array. Note that empty arrays are not necessarily null arrays.
	 */
	bool isNull() const { return isnull; }


	/**
	 * Returns an iterator to the beginning of the array.
	 */
	typename std::vector<Variant>::iterator begin() { return vec.begin(); }

	/**
	 * Returns a const-iterator to the beginning of the array.
	 */
	typename std::vector<Variant>::const_iterator begin() const { return vec.begin(); }


	/**
	 * Returns an iterator past the end of the array.
	 */
	typename std::vector<Variant>::iterator end() { return vec.end(); }

	/**
	 * Returns a const-iterator past the end of the array.
	 */
	typename std::vector<Variant>::const_iterator end() const { return vec.end(); }


	/**
	 * Returns a reference to an array element by index.
	 */
	Variant& operator[](size_t idx) { return vec[idx]; }

	/**
	 * Returns a const-reference to an array element by index.
	 */
	const Variant& operator[](size_t idx) const { return vec[idx]; }


	/**
	 * Returns a reference to the std::vector holding the elements.
	 */
	std::vector<Variant>& data() { return vec; }

	/**
	 * Returns a const-reference to the std::vector holding the elements.
	 */
	const std::vector<Variant>& data() const { return vec; }

private:
	std::vector<Variant> vec;
	bool isnull;
};



};
