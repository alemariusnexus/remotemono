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
//#include <remotemono/RemoteMonoAPI.h>
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


		/*auto rootDomain = mono.getRootDomain();
		RMonoLogInfo("Root Domain: %llX", rootDomain);

		auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
		RMonoLogInfo("Assembly: %llX", ass);

		auto img = mono.assemblyGetImage(ass);
		RMonoLogInfo("Image: %llX", img);

		auto cls = mono.classFromName(img, "", "RemoteMonoTestTarget");
		RMonoLogInfo("Class: %llX", cls);


		auto prop = mono.classGetPropertyFromName(cls, "TestValueProp");
		auto propGet = mono.propertyGetGetMethod(prop);
		int32_t testVal = mono.objectUnbox<int32_t>(mono.runtimeInvoke(propGet));

		RMonoLogInfo("Test Value: %d", testVal);


		auto pointCls = mono.classFromName(img, "", "MyPoint");
		auto pointObj = mono.objectNew(mono.domainGet(), pointCls);
		auto pointCtor = mono.classGetMethodFromName(pointCls, ".ctor", 2);
		mono.runtimeInvoke(pointCtor, mono.objectUnboxRefVariant(pointObj), {777.0f, 987.123f});


		auto method = mono.classGetMethodFromName(cls, "HackMe");

		int32_t howMany = 5;
		RMonoStringPtr didntHackWho = mono.stringNew(rootDomain, "N00b");
		mono.runtimeInvoke(method, nullptr, {mono.stringNew(rootDomain, "the infamous hacker 4chan"), &howMany, &didntHackWho,
				mono.objectUnboxRefVariant(pointObj)});

		RMonoLogInfo("How often has he hacked stuff? %d times!", howMany);
		RMonoLogInfo("And who didn't he hack? %s!", mono.stringToUTF8(didntHackWho).data());

		RMonoLogInfo("And where did he teleport to? %s!", mono.objectToStringUTF8(pointObj).data());

		RMonoLogInfo("Old str gchandle: %u", *didntHackWho);
		RMonoLogInfo("Old str raw: %llX", didntHackWho.raw());

		RMonoStringPtr pinned = didntHackWho.pin();

		RMonoLogInfo("New str gchandle: %u", *pinned);
		RMonoLogInfo("New str raw: %llX", pinned.raw());


		RMonoLogInfo("Done.");*/


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

#if 0
	/*RMonoStdoutLogFunction::getInstance().registerLogFunction();

	RMonoLogger::getInstance().setLogLevel(RMonoLogger::LOG_LEVEL_DEBUG);

	RMonoAPIDispatcher apid;

	apid.foreach([&](auto& e) {
		typedef decltype(e.abi) ABI;
		RMonoLogInfo("Have an ABI with sizeof(irmono_voidp) = %u", (unsigned int) sizeof(typename ABI::irmono_voidp));
		if constexpr(sizeof(typename ABI::irmono_voidp) == 8) {
			RMonoLogInfo("  -> selecting it.");
			abid.selectABI<ABI>();
		}
	});

	apid.apply([&](auto& e) {
		RMonoLogInfo("Selected ABI has sizeof(irmono_voidp) = %u", (unsigned int) sizeof(typename decltype(e.abi)::irmono_voidp));
	});

	RMonoAPI mono;

	float val = 1234.0f;
	RMonoVariant v(&val);

	char data[32];
	memset(data, 0, sizeof(data));

	apid.apply([&](auto& e) {
		size_t size = v.getRemoteMemorySize(e.abi);
		RMonoLogInfo("Size: %u", (unsigned int) size);

		v.copyForRemoteMemory(e.abi, data);
	});

	val = 0.0f;

	apid.apply([&](auto& e) {
		v.updateFromRemoteMemory(e.abi, mono, data);
	});

	RMonoLogInfo("Data: %s", DumpByteArray(data, sizeof(data)).data());
	RMonoLogInfo("val: %f", val);

	return 0;*/

#if 1
	/*auto pids = Process::EnumByName(L"monotest.exe");
	if (pids.empty()) {
		RMLogError("monotest.exe not running.");
		return 1;
	}
	if (pids.size() > 1) {
		RMLogError("Multiple monotest.exe instances running.");
		return 1;
	}

	auto pid = pids[0];

	Process process;
	NTSTATUS status = process.Attach(pid);
	if (!NT_SUCCESS(status)) {
		RMLogError("Error attaching to monotest.exe.");
		return 1;
	}*/

	RMonoStdoutLogFunction::getInstance().registerLogFunction();

	//RMonoLogger::getInstance().setLogLevel(RMonoLogger::LOG_LEVEL_INFO);
	RMonoLogger::getInstance().setLogLevel(RMonoLogger::LOG_LEVEL_DEBUG);
	//RMonoLogger::getInstance().setLogLevel(RMonoLogger::LOG_LEVEL_VERBOSE);

	Process process;
	NTSTATUS status = process.CreateAndAttach(L"remotemono-test-target.exe", false, true, L"");
	if (!NT_SUCCESS(status)) {
		RMonoLogError("Error creating and attaching to remotemono-test-target.exe.");
		return 1;
	}

	Sleep(1000);


	try {
		RMonoAPI mono(process);

		mono.attach();

		RMonoLogInfo("Attached!");

		{

			//mono.stringNew(mono.getRootDomain(), "Test String");

			/*uint32_t val = 0x99999999;
			MonoObjectPtr objVal;

			RemoteMonoVariantArray<uint64_t> params({13.37f, MonoObjectPtr(99, &mono), &val, &objVal});
			mono.runtimeInvoke(0x1234, MonoObjectPtr(66, &mono), params, true);*/

			//mono.fieldSetValue(MonoObjectPtr(99, &mono), 0x123456, 13.37f);

#if 1
			auto rootDomain = mono.getRootDomain();
			RMonoLogInfo("Root Domain: %llX", rootDomain);

			auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
			RMonoLogInfo("Assembly: %llX", ass);

			auto img = mono.assemblyGetImage(ass);
			RMonoLogInfo("Image: %llX", img);

			auto cls = mono.classFromName(img, "", "RemoteMonoTestTarget");
			RMonoLogInfo("Class: %llX", cls);


			/*auto method = mono.classGetMethodFromName(cls, "DoAdd");

			auto field = mono.classGetFieldFromName(cls, "TestValue");
			auto vtable = mono.classVTable(rootDomain, cls);

			uint64_t s = GetTickCount64();
			uint64_t total = 0;

			MemBlock remBlock = std::move(process.memory().Allocate(sizeof(int32_t)).result());
			VoidPtr remPtr = (VoidPtr) remBlock.ptr();

			ProcessMemory& mem = process.memory();

			size_t counter = 0;
			while ((GetTickCount64()-s) < 1000) {
				//auto resObj = mono.runtimeInvoke(method, nullptr, {5, 7});
				//int32_t res = mono.objectUnbox<int32_t>(resObj);
				int32_t res = mono.fieldStaticGetValue<int32_t>(vtable, field);
				total += res;
				counter++;
			}

			RMLogInfo("Ran %llu iterations, calculating %llu", counter, total);*/



			/*auto pointCls = mono.classFromName(img, "", "MyPoint");

			struct MyPoint
			{
				float x;
				float y;
			};

			MyPoint p = {13.37f, 42.69f};
			auto pointObj = mono.valueBox(mono.domainGet(), pointCls, RMonoVariant(&p, sizeof(p)));

			auto lenMethod = mono.classGetMethodFromName(pointCls, "length");

			rmono_voidp pref = mono.objectUnboxRef(pointObj);

			//auto lenObj = mono.runtimeInvoke(lenMethod, RMonoVariant(pref, RMonoVariant::customValueRef));

			float len = mono.objectUnbox<float>(lenObj);

			RMonoLogInfo("Length: %f", len);

			auto vtable = mono.classVTable(mono.domainGet(), cls);


			Sleep(2000);
			RMonoLogInfo("Setting ...");
			auto pointField = mono.classGetFieldFromName(cls, "PointValue");
			mono.fieldSetValue(nullptr, pointField, RMonoVariant(pref, RMonoVariant::customValueRef));
			RMonoLogInfo("... set!");

			Sleep(2000);*/


			/*auto pointMethod = mono.classGetMethodFromName(cls, "GiveMeAPoint", 0);
			RMonoObjectPtr pointObj = mono.runtimeInvoke(pointMethod);
			RMonoClassPtr pointCls = mono.objectGetClass(pointObj);

			struct MyPoint
			{
				float x;
				float y;
			};

			MyPoint p = mono.objectUnbox<MyPoint>(pointObj);
			RMonoLogInfo("Point: %f, %f", p.x, p.y);

			rmono_voidp pref = mono.objectUnboxRef(pointObj);
			RMonoLogInfo("pref: %llX", pref);

			process.memory().Read(pref, sizeof(p), &p);
			RMonoLogInfo("Point 2: %f, %f", p.x, p.y);

			auto lenMethod = mono.classGetMethodFromName(pointCls, "length");

			RMonoLogInfo("Calling 1: %llX", *lenMethod);
			auto lenObj = mono.runtimeInvoke(lenMethod, RMonoVariant(pref, RMonoVariant::customValueRef));
			RMonoLogInfo("lenObj: %llX", *lenObj);
			float len = mono.objectUnbox<float>(lenObj);
			RMonoLogInfo("Length 1: %f", len);*/

			/*auto pointCls = mono.objectGetClass(pointObj);

			float len = mono.objectUnbox<float>(mono.runtimeInvoke(mono.classGetMethodFromName(pointCls, "length"), pointObj));
			RMonoLogInfo("Length: %f", len);

			auto pointStr = mono.objectToStringUTF8(pointObj);
			RMonoLogInfo("Point: %s", pointStr.data());*/


			/*auto pointCls = mono.classFromName(img, "", "MyPoint");
			auto pointObj = mono.objectNew(mono.domainGet(), pointCls);

			auto pointCtor = mono.classGetMethodFromName(pointCls, ".ctor", 2);
			mono.runtimeInvoke(pointCtor, mono.objectUnboxRefVariant(pointObj), {123.45f, 678.9f});

			RMonoLogInfo("Point: %s", mono.objectToStringUTF8(pointObj).data());*/


			/*for (RMonoAssemblyPtr ass : mono.assemblyList()) {
				auto assName = mono.assemblyNameGetName(mono.assemblyGetName(ass));
				RMonoLogInfo("---------- Assembly: %s", assName.data());
			}


			auto prop = mono.classGetPropertyFromName(cls, "TestValueProp");
			auto propGet = mono.propertyGetGetMethod(prop);
			int32_t testVal = mono.objectUnbox<int32_t>(mono.runtimeInvoke(propGet));

			RMonoLogInfo("Test Value: %d", testVal);


			auto pointCls = mono.classFromName(img, "", "MyPoint");
			auto pointObj = mono.objectNew(mono.domainGet(), pointCls);
			auto pointCtor = mono.classGetMethodFromName(pointCls, ".ctor", 2);
			mono.runtimeInvoke(pointCtor, mono.objectUnboxRefVariant(pointObj), {777.0f, 987.123f});


			auto method = mono.classGetMethodFromName(cls, "HackMe");

			int32_t howMany = 5;
			RMonoStringPtr didntHackWho = mono.stringNew(rootDomain, "N00b");
			mono.runtimeInvoke(method, nullptr, {mono.stringNew(rootDomain, "the infamous hacker 4chan"), &howMany, &didntHackWho,
					mono.objectUnboxRefVariant(pointObj)});

			RMonoLogInfo("How often has he hacked stuff? %d times!", howMany);
			RMonoLogInfo("And who didn't he hack? %s!", mono.stringToUTF8(didntHackWho).data());

			RMonoLogInfo("And where did he teleport to? %s!", mono.objectToStringUTF8(pointObj).data());

			RMonoLogInfo("Old str gchandle: %u", *didntHackWho);
			RMonoLogInfo("Old str raw: %llX", didntHackWho.raw());

			RMonoStringPtr pinned = didntHackWho.pin();

			RMonoLogInfo("New str gchandle: %u", *pinned);
			RMonoLogInfo("New str raw: %llX", pinned.raw());*/


			/*auto arrField = mono.classGetFieldFromName(cls, "StrArrValue");
			auto arr = mono.fieldGetValue<RMonoArrayPtr>(nullptr, arrField);

			for (auto val : mono.arrayAsVector<RMonoStringPtr>(arr)) {
				RMonoLogInfo("Array: %s", mono.stringToUTF8(val).data());
			}*/

			//auto arrField = mono.classGetFieldFromName(cls, "IntArrValue");
			//auto arr = mono.fieldGetValue<RMonoArrayPtr>(nullptr, arrField);

			auto arr = mono.arrayFromVector<RMonoStringPtr>(mono.domainGet(), mono.getStringClass(), {
					mono.stringNew(mono.domainGet(), "Wer"),
					mono.stringNew(mono.domainGet(), "anderen"),
					mono.stringNew(mono.domainGet(), "eine"),
					mono.stringNew(mono.domainGet(), "Grube"),
					mono.stringNew(mono.domainGet(), "graebt")
			});

			RMonoLogInfo("Length: %llu", mono.arrayLength(arr));
			RMonoLogInfo("ElemSize: %d", mono.arrayElementSize(mono.objectGetClass(arr)));
			RMonoLogInfo("Rank: %d", mono.classGetRank(mono.objectGetClass(arr)));

			/*for (auto val : mono.arrayAsVector<int32_t>(arr)) {
				RMonoLogInfo("Array (A): %d", val);
			}*/
			for (auto val : mono.arrayAsVector<RMonoStringPtr>(arr)) {
				RMonoLogInfo("Array (A): %s", val ? mono.stringToUTF8(val).data() : "[NULL]");
			}

			/*//auto arr = mono.arrayNew(mono.domainGet(), mono.getInt32Class(), 10);
			auto arr = mono.arrayNewFull(mono.domainGet(), mono.arrayClassGet(mono.getInt32Class(), 2), {4, 8}, {2, 0});

			auto descMethod = mono.classGetMethodFromName(cls, "DescribeArray");
			mono.runtimeInvoke(descMethod, nullptr, {arr});*/

			/*arr = mono.arrayClone(arr);

			for (auto val : mono.arrayAsVector<int32_t>(arr)) {
				RMonoLogInfo("Array (A): %d", val);
			}

			mono.arraySet(arr, 2, int32_t(1337));
			mono.arraySet(arr, 3, int32_t(1338));

			for (auto val : mono.arrayAsVector<int32_t>(arr)) {
				RMonoLogInfo("Array (B): %d", val);
			}

			Sleep(1500);

			mono.fieldSetValue(nullptr, arrField, arr);

			Sleep(1500);*/

			/*auto arrField = mono.classGetFieldFromName(cls, "StrArrValue");
			auto arr = mono.fieldGetValue<RMonoArrayPtr>(nullptr, arrField);

			for (auto val : mono.arrayAsVector<RMonoStringPtr>(arr)) {
				RMonoLogInfo("Array (A): %s", mono.stringToUTF8(val).data());
			}

			auto newStr = mono.stringNew(mono.domainGet(), "Hello World from RemoteMono!");
			RMonoLogInfo("Set 1");
			//mono.arraySet(arr, 2, newStr);
			mono.arraySet(arr, 2, nullptr);
			RMonoLogInfo("Set 2");

			for (auto val : mono.arrayAsVector<RMonoStringPtr>(arr)) {
				RMonoLogInfo("Array (B): %s", val ? mono.stringToUTF8(val).data() : "[NULL]");
			}*/

			/*rmono_uintptr_t arrLen = mono.arrayLength(arr);
			for (rmono_uintptr_t i = 0 ; i < arrLen ; i++) {
				//int32_t val = mono.arrayGet<int32_t>(arr, i);
				RMonoStringPtr val = mono.arrayGet<RMonoStringPtr>(arr, i);
				RMonoLogInfo("Array[%llu] = %s", i, mono.stringToUTF8(val).data());
			}*/


			Sleep(1000);
#endif

			//auto img = mono.assemblyGetImage(ass);

			//auto testCls = mono.classFromName(img, "", "MonoTestTarget");

			//Sleep(5000);

			/*MonoObjectPtr val;
			RemoteMonoVariant<uint64_t> valVar(&val);
			mono.getMonoAPI().array_addr_with_size(valVar, MonoObjectPtr(99, &mono), 4, 3);*/
		}

		mono.detach();
	} catch (std::exception& ex) {
		RMonoLogError("Error: Caught unhandled exception: %s", ex.what());
	}

	process.Terminate();

	RMonoLogInfo("Done!");

	return 0;
#endif
#endif
}
