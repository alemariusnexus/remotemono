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

#include "../RMonoABIConverter.h"




namespace remotemono
{



/**
 * The default 32-bit (x86) ABI.
 */
class RMonoABIWinX32 : public RMonoABIConvCommon<RMonoABIWinX32>
{
public:
	REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(RMonoABITypeTraits<RMonoABIWinX32>)
};



}
