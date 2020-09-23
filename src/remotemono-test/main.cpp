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

#include "config.h"

#include <cstdio>
#include <exception>
#include <functional>
#include <memory>
#include <list>
#include <locale>
#include <codecvt>
#include <string>
#include <filesystem>
#include <windows.h>
#include <BlackBone/Process/Process.h>
#include <remotemono/log.h>
#include <remotemono/util.h>
#include <remotemono/RMonoAPI.h>
#include <gtest/gtest.h>
#include "CLI11.hpp"
#include "System.h"
#include "TestEnvException.h"

using namespace remotemono;
namespace fs = std::filesystem;






void AttachProcessByExecutablePath(std::string path)
{
	blackbone::Process& proc = System::getInstance().getProcess();

	std::wstring wTargetExePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);

	std::wstring wTargetExeDirPath = fs::path(wTargetExePath).parent_path().wstring();

	NTSTATUS status = proc.CreateAndAttach(wTargetExePath.data(), false, true, L"", wTargetExeDirPath.empty() ? nullptr : wTargetExeDirPath.data());
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error creating and attaching to target executable.");
	}

	Sleep(1000);
}


void AttachProcessByPID(DWORD pid)
{
	blackbone::Process& proc = System::getInstance().getProcess();

	NTSTATUS status = proc.Attach(pid);
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error attaching to target process.");
	}
}


void AttachProcessByExecutableFilename(std::string name)
{
	blackbone::Process& proc = System::getInstance().getProcess();

	std::wstring wTargetRunningExeName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(name);

	auto pids = proc.EnumByName(wTargetRunningExeName);
	if (pids.empty()) {
		throw TestEnvException("Target process not found.");
	} else if (pids.size() > 1) {
		throw TestEnvException("Multiple target process candidates found.");
	}

	NTSTATUS status = proc.Attach(pids[0]);
	if (!NT_SUCCESS(status)) {
		throw TestEnvException("Error attaching to target process.");
	}
}



int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	CLI::App app("remotemono-test");

	app.set_help_flag();

	auto help = app.add_flag("-h,--help", "Print help and exit.");

	std::string targetExePath;
	DWORD targetPID;
	std::string targetRunningExeName;

	std::string targetAssemblyPath;

	std::string logLevelStr;

	app.add_option("-t,--target-file", targetExePath, "Path to the target executable to use for testing.");
	app.add_option("-p,--target-pid", targetPID, "PID of the running process to use for testing.");
	app.add_option("-T,--target-name", targetRunningExeName, "Executable file name of the running process to use for testing.");

	app.add_option("-A,--target-assembly", targetAssemblyPath, "Path to the Mono target assembly.");

	app.add_option("-l,--log-level", logLevelStr, "The logging level. Valid values are: verbose, debug, info, warning, error, none.");

	try {
		app.parse(argc, argv);
		if (*help) {
			printf("\n\n\n");
			throw CLI::CallForHelp();
		}
	} catch (const CLI::Error& e) {
		return app.exit(e);
	}


	bool terminateTarget = false;


	try {
		RMonoStdoutLogFunction::getInstance().registerLogFunction();

		RMonoLogger::LogLevel logLevel = RMonoLogger::LOG_LEVEL_INFO;

		if (logLevelStr == "none") {
			logLevel = RMonoLogger::LOG_LEVEL_NONE;
		} else if (logLevelStr == "verbose") {
			logLevel = RMonoLogger::LOG_LEVEL_VERBOSE;
		} else if (logLevelStr == "debug") {
			logLevel = RMonoLogger::LOG_LEVEL_DEBUG;
		} else if (logLevelStr == "info") {
			logLevel = RMonoLogger::LOG_LEVEL_INFO;
		} else if (logLevelStr == "warning") {
			logLevel = RMonoLogger::LOG_LEVEL_WARNING;
		} else if (logLevelStr == "error") {
			logLevel = RMonoLogger::LOG_LEVEL_ERROR;
		}

		RMonoLogger::getInstance().setLogLevel(logLevel);


		System& sys = System::getInstance();

		if (!targetExePath.empty()) {
			AttachProcessByExecutablePath(targetExePath);
			terminateTarget = true;
		} else if (app.count("--target-pid") != 0) {
			AttachProcessByPID(targetPID);
		} else if (!targetRunningExeName.empty()) {
			AttachProcessByExecutableFilename(targetRunningExeName);
		} else {
			AttachProcessByExecutablePath("remotemono-test-target.exe");
			terminateTarget = true;
		}

		if (!sys.getProcess().valid()) {
			throw TestEnvException("Unable to attach to target process.");
		}


		if (targetAssemblyPath.empty()) {
			targetAssemblyPath = "remotemono-test-target-mono.dll";
		}


		sys.attach(targetAssemblyPath);



		RMonoAPI& mono = sys.getMono();


		int res = RUN_ALL_TESTS();

		sys.detach();

		if (terminateTarget) {
			sys.getProcess().Terminate();
		}

		return res;
	} catch (TestEnvException& ex) {
		RMonoLogError("Test environment exception: %s", ex.what());
		return 1;
	} catch (std::exception& ex) {
		RMonoLogError("Caught unhandled exception: %s", ex.what());
		return 1;
	}

	return 1;
}
