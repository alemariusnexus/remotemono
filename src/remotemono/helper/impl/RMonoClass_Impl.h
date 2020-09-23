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

#include "RMonoClass_Def.h"

#include "RMonoHelperContext_Def.h"
#include "RMonoField_Def.h"
#include "RMonoProperty_Def.h"
#include "RMonoMethod_Def.h"
#include "RMonoObject_Def.h"



namespace remotemono
{


RMonoField RMonoClass::field(const std::string& name) const
{
	auto it = d->fieldsByName.find(name);
	if (it != d->fieldsByName.end()) {
		return it->second;
	}
	RMonoField f(d->ctx, d->mono->classGetFieldFromName(d->cls, name), *this);
	d->fieldsByName.insert(std::pair<std::string, RMonoField>(name, f));
	return f;
}


RMonoProperty RMonoClass::property(const std::string& name) const
{
	auto it = d->propsByName.find(name);
	if (it != d->propsByName.end()) {
		return it->second;
	}
	RMonoProperty p(d->ctx, d->mono->classGetPropertyFromName(d->cls, name), *this);
	d->propsByName.insert(std::pair<std::string, RMonoProperty>(name, p));
	return p;
}


RMonoMethod RMonoClass::method(const std::string& name, int32_t paramCount) const
{
	MethodNameWithParamCount key(name, paramCount);
	auto it = d->methodsByName.find(key);
	if (it != d->methodsByName.end()) {
		return it->second;
	}
	RMonoMethod m(d->ctx, d->mono->classGetMethodFromName(d->cls, name, paramCount), *this);
	d->methodsByName.insert(std::pair<MethodNameWithParamCount, RMonoMethod>(key, m));
	return m;
}


RMonoMethod RMonoClass::methodDesc(const std::string& desc, bool includeNamespace) const
{
	MethodDesc key(desc, includeNamespace);
	auto it = d->methodsByDesc.find(key);
	if (it != d->methodsByDesc.end()) {
		return it->second;
	}
	RMonoMethod m(d->ctx, d->mono->methodDescSearchInClass(desc, includeNamespace, d->cls), *this);
	d->methodsByDesc.insert(std::pair<MethodDesc, RMonoMethod>(key, m));
	return m;
}


RMonoObject RMonoClass::allocObject()
{
	return RMonoObject(d->ctx, d->mono->objectNew(d->cls), *this);
}


RMonoObject RMonoClass::newObject(RMonoVariantArray& args)
{
	RMonoObject obj = allocObject();

	auto ctor = obj.method(".ctor", (int32_t) args.size());
	if (!ctor) {
		throw RMonoException("No suitable constructor found.");
	}
	ctor(args);

	return obj;
}


RMonoObject RMonoClass::newObject(RMonoVariantArray&& args)
{
	return newObject(args);
}


template <typename... VariantT>
RMonoObject RMonoClass::newObject(VariantT... args)
{
	return newObject(RMonoVariantArray({args...}));
}


RMonoObject RMonoClass::newObjectDesc(const std::string_view& argsDesc, RMonoVariantArray& args)
{
	RMonoObject obj = allocObject();

	std::string desc = std::string(":.ctor(").append(argsDesc).append(")");

	auto ctor = obj.methodDesc(desc);
	if (!ctor) {
		throw RMonoException("No suitable constructor found.");
	}
	ctor(args);

	return obj;
}


RMonoObject RMonoClass::newObjectDesc(const std::string_view& argsDesc, RMonoVariantArray&& args)
{
	return newObjectDesc(argsDesc, args);
}


template <typename... VariantT>
RMonoObject RMonoClass::newObjectDesc(const std::string_view& argsDesc, VariantT... args)
{
	return newObjectDesc(argsDesc, RMonoVariantArray({args...}));
}


}
