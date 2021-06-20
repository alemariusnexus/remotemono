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

#include "config.h"

#include <cstdio>
#include <exception>
#include <functional>
#include <memory>
#include <list>
#include <string>
#include <windows.h>
#include <BlackBone/Process/Process.h>
#include <remotemono/log.h>
#include <remotemono/util.h>
#include <remotemono/RMonoAPI.h>
#include <remotemono/RMonoBackend.h>
#include <remotemono/RMonoBackendBlackBone.h>
#include <gtest/gtest.h>
#include "CLI11.hpp"
#include "System.h"
#include "TestBackend.h"
#include "TestEnvException.h"

using namespace remotemono;




int main(int argc, char** argv)
{
	System& sys = System::getInstance();

	::testing::InitGoogleTest(&argc, argv);

	CLI::App app("remotemono-test");

	app.set_help_flag();

	auto help = app.add_flag("-h,--help", "Print help and exit.");

	std::string targetExePath;
	DWORD targetPID;
	std::string targetRunningExeName;

	std::string targetAssemblyPath;

	std::string logLevelStr;

	std::string backendStr;

	std::string backendListStr;
	//for (System::BackendType type : sys.getSupportedBackendTypes()) {
	for (TestBackend* be : TestBackend::getSupportedBackends()) {
		std::string typeID = be->getID();
		if (!backendListStr.empty()) {
			backendListStr.append(", ");
		}
		backendListStr.append(typeID);
	}

	app.add_option("-t,--target-file", targetExePath, "Path to the target executable to use for testing.");
	app.add_option("-p,--target-pid", targetPID, "PID of the running process to use for testing.");
	app.add_option("-T,--target-name", targetRunningExeName, "Executable file name of the running process to use for testing.");

	app.add_option("-A,--target-assembly", targetAssemblyPath, "Path to the Mono target assembly.");

	app.add_option("-l,--log-level", logLevelStr, "The logging level. Valid values are: verbose, debug, info, warning, error, none.");

	app.add_option("-B,--backend", backendStr, std::string("The backend to use. Valid values are: ").append(backendListStr).append("."));

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


		//System::BackendType backendType = System::BackendTypeInvalid;
		TestBackend* testBackend = nullptr;

		if (backendStr.empty()) {
			testBackend = TestBackend::getDefaultBackend();
		} else {
			//for (System::BackendType type : sys.getSupportedBackendTypes()) {
			for (TestBackend* be : TestBackend::getSupportedBackends()) {
				std::string typeID = be->getID();
				if (backendStr == typeID) {
					testBackend = be;
					break;
				}
			}
		}

		if (!testBackend) {
			throw TestEnvException("Invalid test backend.");
		}

		sys.setTestBackend(testBackend);



		if (!targetExePath.empty()) {
			testBackend->attachProcessByExecutablePath(targetExePath);
			terminateTarget = true;
		} else if (app.count("--target-pid") != 0) {
			testBackend->attachProcessByPID(targetPID);
		} else if (!targetRunningExeName.empty()) {
			testBackend->attachProcessByExecutableFilename(targetRunningExeName);
		} else {
			testBackend->attachProcessByExecutablePath("remotemono-test-target.exe");
			terminateTarget = true;
		}


		if (targetAssemblyPath.empty()) {
			targetAssemblyPath = "remotemono-test-target-mono.dll";
		}


		sys.attach(targetAssemblyPath);



		RMonoAPI& mono = sys.getMono();


		int res = RUN_ALL_TESTS();

		sys.detach();

		if (terminateTarget) {
			testBackend->terminateProcess();
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
