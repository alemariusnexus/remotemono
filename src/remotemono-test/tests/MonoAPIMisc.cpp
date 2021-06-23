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

#include "../config.h"

#include <string>
#include <algorithm>
#include <vector>
#include <gtest/gtest.h>
#include <remotemono/RMonoBackendBlackBone.h>
#include "../System.h"



TEST(MonoAPIMiscTest, CompileMethodAndCallNative)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "NativeCallTest");

	auto staticAdd3 = mono.classGetMethodFromName(cls, "StaticAdd3");

	rmono_voidp staticAdd3Addr = mono.compileMethod(staticAdd3);
	ASSERT_NE(staticAdd3Addr, 0);

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
	backend::blackbone::RMonoBlackBoneProcess* bbProc = dynamic_cast<backend::blackbone::RMonoBlackBoneProcess*> (
			&mono.getProcess());

	// TODO: Implement this for other backends.
	if (bbProc) {
		auto staticAdd3Func = blackbone::MakeRemoteFunction<int32_t (*)(int32_t, int32_t, int32_t)> (
				**bbProc, (blackbone::ptr_t) staticAdd3Addr);

		auto res = staticAdd3Func.Call(staticAdd3Func.MakeArguments(5, 7, -2), (**bbProc).remote().getWorker());
		ASSERT_TRUE((bool) res);

		EXPECT_EQ(*res, 10);
	}
#endif
}


TEST(MonoAPIMiscTest, DisasmCode)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "NativeCallTest");

	auto staticAdd3 = mono.classGetMethodFromName(cls, "StaticAdd3");

	rmono_voidp staticAdd3Addr = mono.compileMethod(staticAdd3);
	ASSERT_NE(staticAdd3Addr, 0);

	uint32_t codeSize, maxStack;

	auto header = mono.methodGetHeader(staticAdd3);
	rmono_voidp codeAddr = mono.methodHeaderGetCode(header, &codeSize, &maxStack);

	std::string code = mono.disasmCode(nullptr, staticAdd3, codeAddr, codeAddr + codeSize);
	EXPECT_NE(code.find("ldarg.0"), std::string::npos);
	EXPECT_NE(code.find("ldarg.1"), std::string::npos);
	EXPECT_NE(code.find("ldarg.2"), std::string::npos);
	EXPECT_NE(code.find("add"), std::string::npos);
	EXPECT_NE(code.find("ret"), std::string::npos);
}


TEST(MonoAPIMiscTest, GCLeakBuffered)
{
	RMonoAPI& mono = System::getInstance().getMono();

	mono.setFreeBufferMaxCount(8192);

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto clsCounter = mono.classFromName(img, "", "GCFreeTestCounter");
	auto clsObj = mono.classFromName(img, "", "GCFreeTestObj");

	auto fieldRefcount = mono.classGetFieldFromName(clsCounter, "refcount");

	EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), 0);

	const int32_t numTestObjs = 1000;

	RMonoObjectPtr objs[numTestObjs];

	for (int32_t i = 0 ; i < numTestObjs ; i++) {
		objs[i] = mono.objectNew(clsObj);
		mono.runtimeObjectInit(objs[i]);
	}

	EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), numTestObjs);

	for (int32_t i = numTestObjs/2 ; i < numTestObjs ; i++) {
		objs[i].reset();
	}

	mono.gcCollect(mono.gcMaxGeneration());

	// TODO: These checks were disabled because Mono's GC doesn't seem to be predictable enough for them to always work
	//EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), numTestObjs/2);

	for (int32_t i = 0 ; i < numTestObjs/2 ; i++) {
		objs[i].reset();
	}

	mono.gcCollect(mono.gcMaxGeneration());

	//EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), 0);
}


TEST(MonoAPIMiscTest, GCLeakUnbuffered)
{
	RMonoAPI& mono = System::getInstance().getMono();

	mono.setFreeBufferMaxCount(1);

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto clsCounter = mono.classFromName(img, "", "GCFreeTestCounter");
	auto clsObj = mono.classFromName(img, "", "GCFreeTestObj");

	auto fieldRefcount = mono.classGetFieldFromName(clsCounter, "refcount");

	EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), 0);

	const int32_t numTestObjs = 1000;

	RMonoObjectPtr objs[numTestObjs];

	for (int32_t i = 0 ; i < numTestObjs ; i++) {
		objs[i] = mono.objectNew(clsObj);
		mono.runtimeObjectInit(objs[i]);
	}

	EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), numTestObjs);

	for (int32_t i = numTestObjs/2 ; i < numTestObjs ; i++) {
		objs[i].reset();
	}

	mono.gcCollect(mono.gcMaxGeneration());

	// TODO: These checks were disabled because Mono's GC doesn't seem to be predictable enough for them to always work
	//EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), numTestObjs/2);

	for (int32_t i = 0 ; i < numTestObjs/2 ; i++) {
		objs[i].reset();
	}

	mono.gcCollect(mono.gcMaxGeneration());

	//EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, fieldRefcount), 0);

	mono.setFreeBufferMaxCount(8192);
}
