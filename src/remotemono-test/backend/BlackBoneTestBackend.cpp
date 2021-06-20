#include "BlackBoneTestBackend.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

#include <locale>
#include <codecvt>
#include <filesystem>
#include "../TestEnvException.h"
#include "../System.h"

namespace fs = std::filesystem;


static BlackBoneTestBackend BlackBoneTestBackendInstance;



BlackBoneTestBackend::BlackBoneTestBackend()
		: TestBackend(backend::blackbone::RMonoBlackBoneBackend::getInstance().getID(), 1000),
		  proc(&bbProc)
{
}


void BlackBoneTestBackend::attachProcessByExecutablePath(std::string path)
{
	std::wstring wTargetExePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);

	std::wstring wTargetExeDirPath = fs::path(wTargetExePath).parent_path().wstring();

	NTSTATUS status = bbProc.CreateAndAttach(wTargetExePath.data(), false, true, L"", wTargetExeDirPath.empty() ? nullptr : wTargetExeDirPath.data());
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error creating and attaching to target executable.");
	}

	System::getInstance().setProcess(&proc);

	Sleep(1000);
}


void BlackBoneTestBackend::attachProcessByPID(DWORD pid)
{
	NTSTATUS status = bbProc.Attach(pid);
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error attaching to target process.");
	}

	System::getInstance().setProcess(&proc);
}


void BlackBoneTestBackend::attachProcessByExecutableFilename(std::string name)
{
	std::wstring wTargetRunningExeName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(name);

	auto pids = bbProc.EnumByName(wTargetRunningExeName);
	if (pids.empty()) {
		throw TestEnvException("Target process not found.");
	} else if (pids.size() > 1) {
		throw TestEnvException("Multiple target process candidates found.");
	}

	NTSTATUS status = bbProc.Attach(pids[0]);
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error attaching to target process.");
	}

	System::getInstance().setProcess(&proc);
}


void BlackBoneTestBackend::terminateProcess()
{
	bbProc.Terminate();
}

#endif
