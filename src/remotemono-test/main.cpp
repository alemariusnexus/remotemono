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



void RunBenchmark()
{
	System& sys = System::getInstance();
	RMonoAPI& mono = sys.getMono();

	//mono.setFreeBufferMaxCount(1);

	backend::RMonoProcess& proc = mono.getProcess();

	uint32_t token = static_cast<uint32_t>(std::rand());

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
	backend::blackbone::RMonoBlackBoneProcess* bbProc = dynamic_cast<backend::blackbone::RMonoBlackBoneProcess*>(&proc);

	backend::RMonoMemBlock remoteCode;
	rmono_voidp benchTestAddr = 0;

	if (bbProc) {
		auto asmPtr = proc.createAssembler();
		auto& a = *asmPtr;

		// __fastcall uint32_t BenchTest();
		a->mov(a->zax, token);
		a->ret();

		remoteCode = std::move(backend::RMonoMemBlock::alloc(&proc, a->getCodeSize()));

		void* code = malloc(a->getCodeSize());
		a->relocCode(code);

		proc.writeMemory(*remoteCode, a->getCodeSize(), code);

		free(code);

		benchTestAddr = *remoteCode;
	}

	auto benchTestFunc = blackbone::MakeRemoteFunction<uint32_t (__fastcall *)()> (
			**bbProc, (blackbone::ptr_t) benchTestAddr);
#endif

	auto rootDomain = mono.getRootDomain();

	auto benchCls = mono.classFromName(img, "", "BenchmarkTest");
	auto pointCls = mono.classFromName(img, "", "MyPoint");

	auto benchStr = mono.stringNew("Just some test string");

	auto buildMyPointWithPointlessStringArg = mono.classGetMethodFromName(benchCls, "BuildMyPointWithPointlessStringArg");


	RMonoLogInfo("Running benchmark ...");
	Sleep(1000);



	uint32_t numRawRPCPerSec = 0;
	uint32_t numAWRCyclesPerSec = 0;
	uint32_t numWRCyclesPerSec = 0;
	uint32_t numMonoRPCPerSec = 0;
	uint32_t numRInvokePerSec = 0;

	const uint32_t duration = 2000;
	uint32_t t;

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

	{
		uint32_t s = GetTickCount();
		uint64_t numCalls = 0;

		while ((t = GetTickCount()) - s < duration) {
			auto res = benchTestFunc.Call({}, (**bbProc).remote().getWorker());
			if (!res) {
				throw TestEnvException("Error calling remote function BenchTest()");
			}

			if (token != *res) {
				throw TestEnvException("Invalid token returned by remote function BenchTest()");
			}

			numCalls++;
		}

		numRawRPCPerSec = (uint32_t) (numCalls / (duration / (double) 1000));
	}
#endif

	Sleep(250);

	{
		const char* testdata = "Just some data that should be read back from the remote";
		size_t testdataLen = strlen(testdata);

		char readbackBuf[256];

		uint32_t s = GetTickCount();
		uint64_t numCycles = 0;

		while ((t = GetTickCount()) - s < duration) {
			backend::RMonoMemBlock block = std::move(backend::RMonoMemBlock::alloc(&proc, testdataLen+1));
			block.write(0, testdataLen+1, testdata);
			block.read(0, testdataLen+1, readbackBuf);

			numCycles++;
		}

		numAWRCyclesPerSec = (uint32_t) (numCycles / (duration / (double) 1000));
	}

	Sleep(250);

	{
		const char* testdata = "Just some data that should be read back from the remote";
		size_t testdataLen = strlen(testdata);

		char readbackBuf[256];

		backend::RMonoMemBlock block = std::move(backend::RMonoMemBlock::alloc(&proc, testdataLen+1));

		uint32_t s = GetTickCount();
		uint64_t numCycles = 0;

		while ((t = GetTickCount()) - s < duration) {
			block.write(0, testdataLen+1, testdata);
			block.read(0, testdataLen+1, readbackBuf);

			numCycles++;
		}

		numWRCyclesPerSec = (uint32_t) (numCycles / (duration / (double) 1000));
	}

	Sleep(250);

	{
		uint32_t s = GetTickCount();
		uint64_t numCalls = 0;

		while ((t = GetTickCount()) - s < duration) {
			auto testRootDomain = mono.getRootDomain();

			if (testRootDomain != rootDomain) {
				throw TestEnvException("Invalid root domain");
			}

			numCalls++;
		}

		numMonoRPCPerSec = (uint32_t) (numCalls / (duration / (double) 1000));
	}

	Sleep(250);

	{
		uint32_t s = GetTickCount();
		uint64_t numCalls = 0;

		while ((t = GetTickCount()) - s < duration) {
			auto res = mono.runtimeInvoke(buildMyPointWithPointlessStringArg, nullptr, {benchStr, 123.45f, 678.9f}, false);

			numCalls++;
		}

		numRInvokePerSec = (uint32_t) (numCalls / (duration / (double) 1000));
	}


	RMonoLogInfo("**********");
	RMonoLogInfo("Raw RPCs / second:    %u", numRawRPCPerSec);
	RMonoLogInfo("AWR Cycles / second:  %u", numAWRCyclesPerSec);
	RMonoLogInfo("WR Cycles / second:   %u", numWRCyclesPerSec);
	RMonoLogInfo("Mono RPCs / second:   %u", numMonoRPCPerSec);
	RMonoLogInfo("RInvoke / second:     %u", numRInvokePerSec);
	RMonoLogInfo("**********");
}



int main(int argc, char** argv)
{
	System& sys = System::getInstance();

	TestBackend::init();

	std::srand((int) std::time(0));

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

	app.add_flag("-M,--benchmark", "Run performance benchmark instead of unit tests.");

	try {
		app.parse(argc, argv);
		if (*help) {
			printf("\n\n\n");
			throw CLI::CallForHelp();
		}
	} catch (const CLI::Error& e) {
		TestBackend::shutdown();
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


		int res;

		if (app["--benchmark"]->as<bool>()) {
			RunBenchmark();
			res = 0;
		} else {
			res = RUN_ALL_TESTS();
		}

		sys.detach();

		if (terminateTarget) {
			testBackend->terminateProcess();
		}

		TestBackend::shutdown();

		return res;
	} catch (TestEnvException& ex) {
		RMonoLogError("Test environment exception: %s", ex.what());
		TestBackend::shutdown();
		return 1;
	} catch (std::exception& ex) {
		RMonoLogError("Caught unhandled exception: %s", ex.what());
		TestBackend::shutdown();
		return 1;
	}

	return 1;
}
