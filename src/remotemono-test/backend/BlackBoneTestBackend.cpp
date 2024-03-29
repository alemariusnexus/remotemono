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

#include "BlackBoneTestBackend.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

#include <filesystem>
#include <cstdio>
#include "../TestEnvException.h"
#include "../System.h"

namespace fs = std::filesystem;



BlackBoneTestBackend::BlackBoneTestBackend()
		: TestBackend(backend::blackbone::RMonoBlackBoneBackend::getInstance().getID(), 1000),
		  proc(&bbProc)
{
}


void BlackBoneTestBackend::attachProcessByExecutablePath(std::string path)
{
	std::wstring wTargetExePath = ConvertStringToWString(path);

	std::wstring wTargetExeDirPath = fs::path(wTargetExePath).parent_path().wstring();

	NTSTATUS status = bbProc.CreateAndAttach(wTargetExePath.data(), false, true, L"", wTargetExeDirPath.empty() ? nullptr : wTargetExeDirPath.data());
	if (!NT_SUCCESS(status)) {
		char statusStr[32];
		snprintf(statusStr, sizeof(statusStr), "%lX", status);
		throw TestEnvException(std::string("Error creating and attaching to target executable: ").append(statusStr));
	}

	System::getInstance().setProcess(&proc);

	Sleep(1000);
}


void BlackBoneTestBackend::attachProcessByPID(DWORD pid)
{
	NTSTATUS status = bbProc.Attach(pid);
	if (!NT_SUCCESS(status)) {
		char statusStr[32];
		snprintf(statusStr, sizeof(statusStr), "%lX", status);
		throw TestEnvException(std::string("Error attaching to target process: ").append(statusStr));
	}

	System::getInstance().setProcess(&proc);
}


void BlackBoneTestBackend::attachProcessByExecutableFilename(std::string name)
{
	std::wstring wTargetRunningExeName = ConvertStringToWString(name);

	auto pids = bbProc.EnumByName(wTargetRunningExeName);
	if (pids.empty()) {
		throw TestEnvException("Target process not found.");
	} else if (pids.size() > 1) {
		throw TestEnvException("Multiple target process candidates found.");
	}

	NTSTATUS status = bbProc.Attach(pids[0]);
	if (!NT_SUCCESS(status)) {
		char statusStr[32];
		snprintf(statusStr, sizeof(statusStr), "%lX", status);
		throw TestEnvException(std::string("Error attaching to target process: ").append(statusStr));
	}

	System::getInstance().setProcess(&proc);
}


void BlackBoneTestBackend::terminateProcess()
{
	bbProc.Terminate();
}

#endif
