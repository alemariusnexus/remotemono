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

#ifndef REMOTEMONO_BACKEND_DISABLE_BLACKBONE
#define REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#endif


// BlackBone defines WordSize in Include/Macro.h as a macro (and doesn't use it anywhere else), but Qt uses
// it as an enum identifier in qsysinfo.h, so we'll include Include/Macro.h before anything else and then
// immdediately #undef WordSize to avoid name conflicts.
// TODO: This hack shouldn't be necessary. BlackBone only ever uses WordSize ONCE in Include/Macro.h, so we
// should just patch BlackBone and remove it as a macro altogether.

// Macro.h uses some of it, so this means we have to include it in EVERY SINGLE FILE *sigh*
#include <windows.h>

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

#ifdef WordSize
#error Looks like you included a BlackBone header before including config.h. This won't work because of incompatibilities with Qt.
#endif

#include <BlackBone/Include/Macro.h>

#undef WordSize

#endif


#define REMOTEMONO_VERSION_MAJOR 0
#define REMOTEMONO_VERSION_MINOR 1
#define REMOTEMONO_VERSION_PATCH 0

#define REMOTEMONO_VERSION ((REMOTEMONO_VERSION_MAJOR << 24) | (REMOTEMONO_VERSION_MINOR << 16) | (REMOTEMONO_VERSION_PATCH))
