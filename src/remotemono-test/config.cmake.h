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

#cmakedefine REMOTEMONO_BACKEND_ENABLE_BLACKBONE
#ifndef REMOTEMONO_BACKEND_ENABLE_BLACKBONE
#define REMOTEMONO_BACKEND_DISABLE_BLACKBONE
#endif

#include <remotemono/config.h>


#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
