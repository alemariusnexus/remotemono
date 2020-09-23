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

#include "RMonoHelperContext_Def.h"

#include "RMonoClass_Def.h"
#include "RMonoObject_Def.h"



namespace remotemono
{


void RMonoHelperContext::init()
{
	clsObject = nullptr;
	clsInt16 = nullptr;
	clsInt32 = nullptr;
	clsInt64 = nullptr;
	clsDouble = nullptr;
	clsSingle = nullptr;
	clsString = nullptr;
	clsThread = nullptr;
	clsUInt16 = nullptr;
	clsUInt32 = nullptr;
	clsUInt64 = nullptr;
	clsVoid = nullptr;
	clsArray = nullptr;
	clsBoolean = nullptr;
	clsByte = nullptr;
	clsSByte = nullptr;
	clsChar = nullptr;
	clsException = nullptr;
}


RMonoClass RMonoHelperContext::getCachedClass(RMonoClassPtr cls)
{
	auto it = classesByPtr.find(cls);
	if (it != classesByPtr.end()) {
		return it->second;
	}

	RMonoClass c(this, cls);
	classesByPtr.insert(std::pair<RMonoClassPtr, RMonoClass>(cls, c));
	return c;
}


RMonoClass RMonoHelperContext::classFromName(RMonoImagePtr image, const std::string& nameSpace, const std::string& name)
{
	return RMonoClass(this, image, nameSpace, name);
}


RMonoObject RMonoHelperContext::str(const std::string_view& s)
{
	return RMonoObject(this, mono->stringNew(s), classString());
}


RMonoClass RMonoHelperContext::classObject()
{
	if (!clsObject) {
		clsObject = new RMonoClass(getCachedClass(mono->getObjectClass()));
	}
	return *clsObject;
}


RMonoClass RMonoHelperContext::classString()
{
	if (!clsString) {
		clsString = new RMonoClass(getCachedClass(mono->getStringClass()));
	}
	return *clsString;
}


}
