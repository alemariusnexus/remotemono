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

#include "../../config.h"

#include "RMonoProperty_Def.h"

#include "RMonoMethod_Def.h"



namespace remotemono
{


RMonoMethod RMonoProperty::getter() const
{
	if (isStatic()) {
		return d->getter;
	} else {
		if (!isInstanced()) {
			throw RMonoException("Property is non-static but RMonoProperty object is non-instanced.");
		}
		if (!id->obj) {
			throw RMonoException("Property is non-static but instance is invalid.");
		}
		return id->getter;
	}
}


RMonoMethod RMonoProperty::setter() const
{
	if (isStatic()) {
		return d->setter;
	} else {
		if (!isInstanced()) {
			throw RMonoException("Property is non-static but RMonoProperty object is non-instanced.");
		}
		if (!id->obj) {
			throw RMonoException("Property is non-static but instance is invalid.");
		}
		return id->setter;
	}
}


RMonoObject RMonoProperty::get(RMonoVariantArray& args) const
{
	auto getterMethod = getter();
	if (!getterMethod) {
		throw RMonoException("Property isn't readable");
	}
	return getterMethod.invoke(args);
}


RMonoObject RMonoProperty::get(RMonoVariantArray&& args) const
{
	return get(args);
}


template <typename... VariantT>
RMonoObject RMonoProperty::get(VariantT... args) const
{
	return get(RMonoVariantArray({args...}));
}


template <typename T>
std::enable_if_t<!std::is_same_v<T, RMonoObject>, T> RMonoProperty::get() const
{
	return get().unbox<T>();
}


RMonoObject RMonoProperty::set(RMonoVariantArray& args) const
{
	auto setterMethod = setter();
	if (!setterMethod) {
		throw RMonoException("Property isn't writable");
	}
	return setterMethod.invoke(args);
}


RMonoObject RMonoProperty::set(RMonoVariantArray&& args) const
{
	return set(args);
}


template <typename... VariantT>
RMonoObject RMonoProperty::set(VariantT... args) const
{
	return set(RMonoVariantArray({args...}));
}


}
