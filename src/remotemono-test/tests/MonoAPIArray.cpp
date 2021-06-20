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
#include <gtest/gtest.h>
#include "../System.h"



TEST(MonoAPIArrayTest, ArraySimple)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();

	auto i32Cls = mono.getInt32Class();

	auto arr1 = mono.arrayNew(dom, i32Cls, 7);
	EXPECT_EQ(mono.arrayLength(arr1), 7);

	EXPECT_EQ(mono.objectGetClass(arr1), mono.arrayClassGet(i32Cls, 1));
	EXPECT_EQ(mono.arrayElementSize(mono.objectGetClass(arr1)), 4);
	EXPECT_EQ(mono.classArrayElementSize(i32Cls), 4);

	mono.arraySet(arr1, 0, 67);
	mono.arraySet(arr1, 1, 164);
	mono.arraySet(arr1, 2, -8);
	mono.arraySet(arr1, 3, 5);

	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 0), 67);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 1), 164);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 2), -8);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 3), 5);

	auto arr2 = mono.arrayFromVector<int32_t>(dom, i32Cls, {10, 20, 30, 40, 50, 60});
	EXPECT_EQ(mono.arrayLength(arr2), 6);
	EXPECT_EQ(mono.arrayAsVector<int32_t>(arr2), std::vector<int32_t>({10, 20, 30, 40, 50, 60}));
}


TEST(MonoAPIArrayTest, ArrayReferenceType)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();

	auto arr1 = mono.arrayNew(dom, mono.getStringClass(), 5);
	EXPECT_EQ(mono.arrayLength(arr1), 5);

	EXPECT_EQ(mono.objectGetClass(arr1), mono.arrayClassGet(mono.getStringClass(), 1));

	mono.arraySet(arr1, 0, mono.stringNew(dom, "Element 1"));
	mono.arraySet(arr1, 1, mono.stringNew(dom, "Element 2"));
	mono.arraySet(arr1, 2, mono.stringNew(dom, "Element 3"));
	mono.arraySet(arr1, 3, mono.stringNew(dom, "Element 4"));
	mono.arraySet(arr1, 4, mono.stringNew(dom, "Element 5"));

	EXPECT_EQ(mono.stringToUTF8(mono.arrayGet<RMonoStringPtr>(arr1, 1)), std::string("Element 2"));
	EXPECT_EQ(mono.stringToUTF8(mono.arrayGet<RMonoStringPtr>(arr1, 3)), std::string("Element 4"));
	EXPECT_EQ(mono.stringToUTF8(mono.arrayGet<RMonoStringPtr>(arr1, 4)), std::string("Element 5"));

	auto arr2 = mono.arrayFromVector<RMonoStringPtr>(dom, mono.getStringClass(), {
			mono.stringNew(dom, "This"),
			mono.stringNew(dom, "is"),
			mono.stringNew(dom, "a"),
			mono.stringNew(dom, "test")
	});
	auto arr2Vec = mono.arrayAsVector<RMonoStringPtr>(arr2);

	EXPECT_EQ(mono.stringToUTF8(arr2Vec[0]), std::string("This"));
	EXPECT_EQ(mono.stringToUTF8(arr2Vec[1]), std::string("is"));
	EXPECT_EQ(mono.stringToUTF8(arr2Vec[2]), std::string("a"));
	EXPECT_EQ(mono.stringToUTF8(arr2Vec[3]), std::string("test"));
}


TEST(MonoAPIArrayTest, ArrayMultiDim)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();

	auto i32Cls = mono.getInt32Class();

	auto arr1 = mono.arrayNewFull(dom, mono.arrayClassGet(i32Cls, 3), {3, 3, 3});
	EXPECT_EQ(mono.classGetRank(mono.objectGetClass(arr1)), 3);

	EXPECT_EQ(mono.classGetElementClass(mono.objectGetClass(arr1)), i32Cls);

	EXPECT_EQ(mono.arrayLength(arr1), 3*3*3);

	for (int z = 0 ; z < 3 ; z++) {
		for (int y = 0 ; y < 3 ; y++) {
			for (int x = 0 ; x < 3 ; x++) {
				mono.arraySet(arr1, z*3*3 + y*3 + x, z*100 + y*10 + x);
			}
		}
	}

	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 0*3*3 + 1*3 + 2), 12);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 2*3*3 + 0*3 + 1), 201);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 2*3*3 + 2*3 + 2), 222);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 0*3*3 + 0*3 + 0), 0);
}


TEST(MonoAPIArrayTest, ArrayClone)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();

	auto i32Cls = mono.getInt32Class();

	auto arr1 = mono.arrayFromVector<int32_t>(dom, i32Cls, {10, 20, 30, 40, 50, 60});
	auto arr2 = mono.arrayClone(arr1);

	EXPECT_EQ(mono.arrayLength(arr2), 6);
	EXPECT_EQ(mono.arrayAsVector<int32_t>(arr1), mono.arrayAsVector<int32_t>(arr2));

	mono.arraySet(arr1, 1, 1337);

	EXPECT_EQ(mono.arrayGet<int32_t>(arr1, 1), 1337);
	EXPECT_EQ(mono.arrayGet<int32_t>(arr2, 1), 20);
}
