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
#include <set>
#include <gtest/gtest.h>
#include <remotemono/impl/mono/metadata/tabledefs.h>
#include "../System.h"



TEST(MonoAPIFieldTest, FieldMetadata)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	auto pubField = mono.classGetFieldFromName(cls, "publicField");

	EXPECT_EQ(mono.fieldGetName(pubField), std::string("publicField"));
	EXPECT_EQ(mono.classGetName(mono.fieldGetParent(pubField)), std::string("RemoteMonoDerived"));


	cls = mono.classFromName(img, "", "ClassWithExplicitLayout");

	auto intAt0 = mono.classGetFieldFromName(cls, "IntAt0");
	auto intAt10 = mono.classGetFieldFromName(cls, "IntAt10");
	auto intAt15 = mono.classGetFieldFromName(cls, "IntAt15");

	uint32_t intAt0Offs = mono.fieldGetOffset(intAt0);
	uint32_t intAt10Offs = mono.fieldGetOffset(intAt10);
	uint32_t intAt15Offs = mono.fieldGetOffset(intAt15);

	EXPECT_EQ(intAt10Offs, intAt0Offs+10);
	EXPECT_EQ(intAt15Offs, intAt0Offs+15);
}


TEST(MonoAPIFieldTest, FieldValueReferenceType)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "FieldTest");
	auto vtable = mono.classVTable(mono.domainGet(), cls);

	ASSERT_TRUE(vtable);

	mono.runtimeClassInit(vtable);

	// Check that double-init doesn't hurt
	mono.runtimeClassInit(vtable);

	auto staticIntField = mono.classGetFieldFromName(cls, "StaticIntField");
	auto staticStringField = mono.classGetFieldFromName(cls, "StaticStringField");
	auto instanceField = mono.classGetFieldFromName(cls, "Instance");
	auto intField = mono.classGetFieldFromName(cls, "IntField");
	auto stringField = mono.classGetFieldFromName(cls, "StringField");

	ASSERT_TRUE(staticIntField);
	ASSERT_TRUE(instanceField);
	ASSERT_TRUE(intField);
	ASSERT_TRUE(stringField);



	// ********** STATIC FIELDS **********

	int32_t i32 = 0;

	EXPECT_EQ(mono.fieldStaticGetValue<int32_t>(vtable, staticIntField), int32_t(25));
	EXPECT_EQ(mono.fieldGetValue<int32_t>(nullptr, staticIntField), int32_t(25));

	mono.fieldStaticSetValue(vtable, staticIntField, int32_t(28));

	i32 = 9999;
	mono.fieldStaticGetValue(vtable, staticIntField, RMonoVariant(&i32));
	EXPECT_EQ(i32, 28);

	mono.fieldSetValue(nullptr, staticIntField, int32_t(22));

	i32 = 9999;
	mono.fieldGetValue(nullptr, staticIntField, RMonoVariant(&i32));
	EXPECT_EQ(i32, 22);


	auto obj1 = mono.fieldStaticGetValue<RMonoObjectPtr>(vtable, instanceField);
	EXPECT_TRUE(obj1);

	RMonoObjectPtr obj2;
	mono.fieldStaticGetValue(vtable, instanceField, &obj2);
	EXPECT_TRUE(obj2);

	EXPECT_EQ(obj1, obj2);

	obj2.reset();
	ASSERT_NE(obj1, obj2);

	obj2 = mono.fieldGetValue<RMonoObjectPtr>(nullptr, instanceField);
	EXPECT_EQ(obj1, obj2);

	obj2.reset();
	mono.fieldGetValue(nullptr, instanceField, &obj2);
	EXPECT_EQ(obj1, obj2);

	EXPECT_EQ(mono.stringToUTF8(mono.fieldGetValue<RMonoStringPtr>(nullptr, staticStringField)), std::string("jumps over the lazy dog"));

	mono.fieldSetValue(nullptr, staticStringField, mono.stringNew(mono.domainGet(), "jumps over the lazy god"));
	EXPECT_EQ(mono.stringToUTF8(mono.fieldGetValue<RMonoStringPtr>(nullptr, staticStringField)), std::string("jumps over the lazy god"));

	RMonoClassPtr fieldCls;
	EXPECT_EQ(mono.stringToUTF8(mono.fieldGetValueObjectWithRetCls(fieldCls, staticStringField)), std::string("jumps over the lazy god"));
	EXPECT_EQ(fieldCls, mono.getStringClass());



	// ********** INSTANCE FIELDS **********

	auto obj = mono.fieldStaticGetValue<RMonoObjectPtr>(vtable, instanceField);
	ASSERT_TRUE(obj);

	EXPECT_EQ(mono.fieldGetValue<int32_t>(obj, intField), int32_t(13));

	mono.fieldSetValue(obj, intField, 15);

	i32 = 9999;
	mono.fieldGetValue(obj, intField, &i32);
	EXPECT_EQ(i32, int32_t(15));

	auto str = mono.fieldGetValue<RMonoStringPtr>(obj, stringField);
	ASSERT_TRUE(str);
	EXPECT_EQ(mono.stringToUTF8(str), std::string("The quick brown fox"));

	str.reset();
	ASSERT_FALSE(str);

	mono.fieldSetValue(obj, stringField, mono.stringNew(mono.domainGet(), "The quick brown box"));

	mono.fieldGetValue(obj, stringField, &str);
	ASSERT_TRUE(str);
	EXPECT_EQ(mono.stringToUTF8(str), std::string("The quick brown box"));
}


TEST(MonoAPIFieldTest, FieldValueValueType)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "ValFieldTest");
	auto vtable = mono.classVTable(mono.domainGet(), cls);

	auto pointCls = mono.classFromName(img, "", "MyPoint");

	ASSERT_TRUE(vtable);

	mono.runtimeClassInit(vtable);

	// Check that double-init doesn't hurt
	mono.runtimeClassInit(vtable);

	auto instanceField = mono.classGetFieldFromName(cls, "Instance");
	auto stringField = mono.classGetFieldFromName(cls, "StringField");
	auto intField = mono.classGetFieldFromName(cls, "IntField");
	auto pointField = mono.classGetFieldFromName(cls, "PointField");
	auto staticIntField = mono.classGetFieldFromName(cls, "StaticIntField");

	ASSERT_TRUE(instanceField);
	ASSERT_TRUE(stringField);
	ASSERT_TRUE(intField);
	ASSERT_TRUE(staticIntField);

	EXPECT_EQ(mono.fieldStaticGetValue<int32_t>(vtable, staticIntField), int32_t(64));

	uint32_t instSize = mono.classInstanceSize(cls);

	char* instData = new char[instSize];
	mono.fieldStaticGetValue(vtable, instanceField, RMonoVariant(instData, instSize, true));

	auto obj = mono.valueBox(mono.domainGet(), cls, RMonoVariant(instData, instSize));

	mono.fieldSetValue(obj, stringField, mono.stringNew(mono.domainGet(), "Just a simple test string"));

	auto str = mono.fieldGetValue<RMonoStringPtr>(obj, stringField);
	EXPECT_EQ(mono.stringToUTF8(str), std::string("Just a simple test string"));

	mono.fieldSetValue(obj, intField, 15589);
	EXPECT_EQ(mono.fieldGetValue<int32_t>(obj, intField), 15589);

	auto p1 = mono.objectNew(mono.domainGet(), pointCls);

	mono.runtimeInvoke(mono.classGetMethodFromName(pointCls, ".ctor", 2), p1, {555.1f, 317.9f});

	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p1, mono.classGetFieldFromName(pointCls, "x")), 555.1f);
	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p1, mono.classGetFieldFromName(pointCls, "y")), 317.9f);

	{
		// Set and get value type field by raw pointer

		auto p2 = mono.objectNew(mono.domainGet(), pointCls);
		mono.runtimeInvoke(mono.classGetMethodFromName(pointCls, ".ctor", 2), p2, {1.0f, 2.0f});

		mono.fieldSetValue(obj, pointField, mono.objectUnboxRaw(p1));
		mono.fieldGetValue(obj, pointField, mono.objectUnboxRaw(p2));

		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, mono.classGetFieldFromName(pointCls, "x")), 555.1f);
		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, mono.classGetFieldFromName(pointCls, "y")), 317.9f);
	}

	{
		// Set and get value type field by boxed object (auto-unboxing in wrapper function)

		auto p2 = mono.objectNew(mono.domainGet(), pointCls);
		mono.runtimeInvoke(mono.classGetMethodFromName(pointCls, ".ctor", 2), p2, {1.0f, 2.0f});

		mono.fieldSetValue(obj, pointField, p1);
		mono.fieldGetValue(obj, pointField, p2);

		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, mono.classGetFieldFromName(pointCls, "x")), 555.1f);
		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, mono.classGetFieldFromName(pointCls, "y")), 317.9f);
	}

	// TODO: More tests here, e.g. with RMonoVariant::TypeCustomValueCopy.
}


