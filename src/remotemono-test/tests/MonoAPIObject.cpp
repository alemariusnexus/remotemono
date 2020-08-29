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



TEST(MonoAPIObjectTest, ObjectMetadata)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto baseCls = mono.classFromName(img, "", "RemoteMonoBase");
	auto derivedCls = mono.classFromName(img, "", "RemoteMonoDerived");

	auto derivedObj = mono.objectNew(mono.domainGet(), derivedCls);
	mono.runtimeObjectInit(derivedObj);

	EXPECT_EQ(mono.objectGetClass(derivedObj), derivedCls);

	EXPECT_EQ(mono.objectToStringUTF8(derivedObj), std::string("I'm a RemoteMonoDerived instance"));

	EXPECT_EQ(mono.objectGetDomain(derivedObj), mono.domainGet());

	EXPECT_TRUE(mono.objectIsInst(derivedObj, derivedCls));
	EXPECT_TRUE(mono.objectIsInst(derivedObj, baseCls));
	EXPECT_TRUE(mono.objectIsInst(derivedObj, mono.getObjectClass()));
	EXPECT_FALSE(mono.objectIsInst(derivedObj, mono.getExceptionClass()));
}


TEST(MonoAPIObjectTest, ObjectVirtualCall)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto baseCls = mono.classFromName(img, "", "RemoteMonoBase");
	auto derivedCls = mono.classFromName(img, "", "RemoteMonoDerived");

	auto derivedObj = mono.objectNew(mono.domainGet(), derivedCls);
	mono.runtimeObjectInit(derivedObj);

	auto baseCalculate = mono.classGetMethodFromName(baseCls, "Calculate", 2);
	EXPECT_EQ(mono.objectUnbox<int32_t>(mono.runtimeInvoke(baseCalculate, derivedObj, {5, 6})), 11);

	auto virtualBaseCalculate = mono.objectGetVirtualMethod(derivedObj, baseCalculate);
	EXPECT_EQ(mono.objectUnbox<int32_t>(mono.runtimeInvoke(virtualBaseCalculate, derivedObj, {5, 6})), 30);
}
