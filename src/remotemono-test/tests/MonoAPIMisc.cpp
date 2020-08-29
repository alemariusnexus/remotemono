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

#include "../config.h"

#include <string>
#include <algorithm>
#include <gtest/gtest.h>
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

	auto staticAdd3Func = blackbone::MakeRemoteFunction<int32_t (*)(int32_t, int32_t, int32_t)> (
			mono.getProcess(), (blackbone::ptr_t) staticAdd3Addr);

	auto res = staticAdd3Func.Call(staticAdd3Func.MakeArguments(5, 7, -2), mono.getWorkerThread());
	ASSERT_TRUE((bool) res);

	EXPECT_EQ(*res, 10);
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
