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

#include "../../config.h"

#include <string>
#include "../RMonoTypes.h"
#include "../exception/RMonoException_Def.h"


namespace remotemono
{
namespace backend
{


class RMonoModule
{
public:
	struct Export
	{
		rmono_funcp procPtr;
	};

public:
	virtual bool getExport(Export& exp, const std::string& name) = 0;

	Export getExport(const std::string& name)
	{
		Export exp;
		if (!getExport(exp, name)) {
			throw RMonoException(std::string("Export not found: ").append(name));
		}
		return exp;
	}

	virtual std::string getName() const = 0;
};


}
}
