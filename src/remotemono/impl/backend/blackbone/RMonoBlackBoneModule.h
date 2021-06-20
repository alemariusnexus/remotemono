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

#include "../../../config.h"

#include <string>
#include "../RMonoModule.h"
#include "../../../util.h"
#include <BlackBone/Process/Process.h>

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "Included BlackBone backend header, but BlackBone backend is disabled."
#endif


namespace remotemono
{
namespace backend
{
namespace blackbone
{


class RMonoBlackBoneModule : public RMonoModule
{
public:
	RMonoBlackBoneModule(::blackbone::Process& process, ::blackbone::ModuleDataPtr modulePtr)
			: process(process), modulePtr(modulePtr)
	{
	}

	::blackbone::ModuleDataPtr operator*() { return modulePtr; }

	bool getExport(Export& exp, const std::string& name) override
	{
		auto res = process.modules().GetExport(modulePtr, name.c_str());
		if (res) {
			exp.procPtr = res->procAddress;
			return true;
		}
		return false;
	}

	std::string getName() const override
	{
		return ConvertWStringToString(modulePtr->name);
	}

private:
	::blackbone::Process& process;
	::blackbone::ModuleDataPtr modulePtr;
};


}
}
}
