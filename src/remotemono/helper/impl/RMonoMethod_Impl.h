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

#include "RMonoMethod_Def.h"



namespace remotemono
{


RMonoObject RMonoMethod::invoke(RMonoVariantArray& args)
{
	RMonoObjectPtr res;

	if (isStatic()) {
		res = d->mono->runtimeInvoke(d->method, nullptr, args);
	} else {
		if (!isInstanced()) {
			throw RMonoException("Method is non-static but RMonoMethod object is non-instanced.");
		}
		if (!id->obj) {
			throw RMonoException("Method is non-static but instance is invalid.");
		}
		res = d->mono->runtimeInvoke(d->method, id->obj, args);
	}

	return RMonoObject(d->ctx, res);
}


RMonoObject RMonoMethod::invoke(RMonoVariantArray&& args)
{
	return invoke(args);
}


template <typename... VariantT>
RMonoObject RMonoMethod::invoke(VariantT... args)
{
	return invoke(RMonoVariantArray({args...}));
}


RMonoObject RMonoMethod::operator()(RMonoVariantArray& args)
{
	return invoke(args);
}


RMonoObject RMonoMethod::operator()(RMonoVariantArray&& args)
{
	return invoke(std::move(args));
}


template <typename... VariantT>
RMonoObject RMonoMethod::operator()(VariantT... args)
{
	return invoke(args...);
}


}
