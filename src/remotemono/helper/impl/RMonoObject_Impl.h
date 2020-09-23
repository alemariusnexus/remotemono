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

#include "RMonoObject_Def.h"

#include "RMonoField_Def.h"



namespace remotemono
{


RMonoField RMonoObject::field(const std::string& name) const
{
	return RMonoField(d->cls.field(name), *this);
}


RMonoProperty RMonoObject::property(const std::string& name) const
{
	return RMonoProperty(d->cls.property(name), *this);
}


RMonoMethod RMonoObject::method(const std::string& name, int32_t paramCount) const
{
	return RMonoMethod(d->cls.method(name, paramCount), *this);
}


RMonoMethod RMonoObject::methodDesc(const std::string& desc, bool includeNamespace) const
{
	return RMonoMethod(d->cls.methodDesc(desc, includeNamespace), *this);
}


}
