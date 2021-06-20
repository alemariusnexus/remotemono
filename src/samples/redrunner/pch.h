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

// This is the main precompiled header file. It is VERY useful to put the RemoteMono header includes (and some other stuff)
// in here to keep compilation times low. RemoteMono is a very heavily templated library, so compiling its headers takes
// some time. With precompiled headers, it needs to be compiled only once even if it's included in multiple source files,
// as long as the precompiled header file is included before anything else (?) in every source file.
//
// pch.h is the new name of the file called stdafx.h prior to Visual Studio 2017.

#include "config.h"

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "RedRunner sample currently needs the BlackBone backend to be enabled."
#endif

#include <windows.h>
#include <BlackBone/Process/Process.h>
#include <BlackBone/Process/RPC/RemoteFunction.hpp>
#include <remotemono/log.h>
#include <remotemono/util.h>
#include <remotemono/RMonoAPI.h>
#include <remotemono/helper/RMonoHelpers.h>
#include <remotemono/RMonoBackend.h>
#include <remotemono/RMonoBackendBlackBone.h>
