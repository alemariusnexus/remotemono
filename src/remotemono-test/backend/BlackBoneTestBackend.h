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

#include "../config.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

#include "../TestBackend.h"
#include <remotemono/RMonoBackendBlackBone.h>

using namespace remotemono;


class BlackBoneTestBackend : public TestBackend
{
public:
	BlackBoneTestBackend();

	void attachProcessByExecutablePath(std::string path) override;
	void attachProcessByPID(DWORD pid) override;
	void attachProcessByExecutableFilename(std::string name) override;
	void terminateProcess() override;

private:
	blackbone::Process bbProc;
	backend::blackbone::RMonoBlackBoneProcess proc;
};

#endif
