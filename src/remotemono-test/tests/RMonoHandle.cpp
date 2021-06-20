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

#include <algorithm>
#include <gtest/gtest.h>
#include "../System.h"



TEST(RMonoHandleTest, HandleRaw)
{
	RMonoAPI& mono = System::getInstance().getMono();

	RMonoAssemblyPtr h0(0, &mono, false);
	RMonoAssemblyPtr h1234(1234, &mono, false);
	RMonoAssemblyPtr h1235(1235, &mono, false);

	EXPECT_FALSE(h0);
	EXPECT_TRUE(h1234);
	EXPECT_TRUE(h1235);

	EXPECT_EQ(h1234, RMonoAssemblyPtr(1234, &mono, false));
	EXPECT_NE(h1234, h1235);

	EXPECT_EQ(*h0, RMonoAssemblyPtrRaw(0));
	EXPECT_EQ(*h1234, RMonoAssemblyPtrRaw(1234));
	EXPECT_EQ(*h1235, RMonoAssemblyPtrRaw(1235));

	EXPECT_EQ(RMonoAssemblyPtr(1234, &mono, false), RMonoAssemblyPtr(1234, &mono, false));
	EXPECT_NE(RMonoAssemblyPtr(1234, &mono, false), RMonoAssemblyPtr(1235, &mono, false));

	EXPECT_EQ(h1234.getMonoAPI(), &mono);

	EXPECT_FALSE(h0.takeOwnership());
	EXPECT_FALSE(h0.takeOwnership());
	EXPECT_FALSE(h1235.takeOwnership());
	EXPECT_FALSE(h1235.takeOwnership());

	EXPECT_TRUE(h1234.isValid());
	EXPECT_FALSE(h1234.isNull());

	EXPECT_FALSE(h0.isValid());
	EXPECT_TRUE(h0.isNull());

	h1234.reset();
	EXPECT_EQ(h1234, h0);
}


TEST(RMonoHandleTest, HandleObject)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto obj = mono.stringNew(mono.domainGet(), "Just a test string");
	ASSERT_TRUE(obj);
	ASSERT_NE(*obj, 0);

	auto objPinned = obj.pin();
	ASSERT_TRUE(objPinned);

	EXPECT_NE(*obj, *objPinned);

	ASSERT_NE(objPinned.raw(), 0);

	RMonoObjectPtr objAlias = obj.clone();

	EXPECT_NE(*obj, *objAlias);
	EXPECT_EQ(obj, objAlias);

	EXPECT_EQ(obj.raw(), objAlias.raw());
}
