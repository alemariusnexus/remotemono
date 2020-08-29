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



TEST(MonoAPIAssemblyTest, AssemblyLoaded)
{
	RMonoAPI& mono = System::getInstance().getMono();

	EXPECT_TRUE((bool) mono.assemblyLoaded("remotemono-test-target-mono"));
	EXPECT_FALSE((bool) mono.assemblyLoaded("ridiculous-assembly-name-that-doesnt-exist-420133769"));
}


TEST(MonoAPIAssemblyTest, AssemblyList)
{
	RMonoAPI& mono = System::getInstance().getMono();

	std::string expName = "remotemono-test-target-mono";

	auto asses = mono.assemblyList();
	bool expNameFound = false;

	for (auto ass : asses) {
		auto assName = mono.assemblyGetName(ass);
		std::string name = mono.assemblyNameGetName(assName);

		if (name == expName) {
			expNameFound = true;
			break;
		}
	}

	ASSERT_TRUE(expNameFound);
}


TEST(MonoAPIAssemblyTest, AssemblyName)
{
	RMonoAPI& mono = System::getInstance().getMono();

	size_t handleCountBefore = mono.getRegisteredHandleCount();

	{
		auto assName = mono.assemblyNameNew("TestAssembly, Version=4.2.0.1337, Culture=ja, PublicKeyToken=null");

		EXPECT_GT(mono.getRegisteredHandleCount(), handleCountBefore);

		EXPECT_EQ(mono.assemblyNameGetName(assName), std::string("TestAssembly"));
		EXPECT_EQ(mono.assemblyNameGetCulture(assName), std::string("ja"));

		uint16_t major, minor, build, rev;
		major = mono.assemblyNameGetVersion(assName, &minor, &build, &rev);

		EXPECT_EQ(major, 4);
		EXPECT_EQ(minor, 2);
		EXPECT_EQ(build, 0);
		EXPECT_EQ(rev, 1337);
	}

	EXPECT_EQ(mono.getRegisteredHandleCount(), handleCountBefore);
}


TEST(MonoAPIAssemblyTest, AssemblyNameManualFree)
{
	RMonoAPI& mono = System::getInstance().getMono();

	size_t handleCountBefore = mono.getRegisteredHandleCount();

	RMonoAssemblyNamePtrRaw rawAssName = 0;

	{
		auto assName = mono.assemblyNameNew("TestAssembly, Version=4.2.0.1337, Culture=ja, PublicKeyToken=null");

		EXPECT_GT(mono.getRegisteredHandleCount(), handleCountBefore);

		EXPECT_TRUE(assName.takeOwnership());
		rawAssName = *assName;

		EXPECT_EQ(mono.getRegisteredHandleCount(), handleCountBefore);
	}

	EXPECT_EQ(mono.getRegisteredHandleCount(), handleCountBefore);

	mono.assemblyNameFree(rawAssName);
}
