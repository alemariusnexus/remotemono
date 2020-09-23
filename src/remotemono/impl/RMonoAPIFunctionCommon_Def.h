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



namespace remotemono
{


template <typename ABI>
class RMonoAPIFunctionCommon
{
protected:
	typedef uint16_t variantflags_t;

	enum ParamFlags
	{
		ParamFlagMonoObjectPtr = 0x0001,
		ParamFlagOut = 0x0002,
		ParamFlagDirectPtr = 0x0004,
		ParamFlagDisableAutoUnbox = 0x0008,

		ParamFlagLastArrayElement = 0x8000
	};
};


}
