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



TEST(MonoAPIPropertyTest, PropertyMetadata)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "PropertyTest");

	auto floatProp = mono.classGetPropertyFromName(cls, "FloatProp");
	auto stringProp = mono.classGetPropertyFromName(cls, "StringProp");

	EXPECT_EQ(mono.propertyGetName(floatProp), std::string("FloatProp"));
	EXPECT_EQ(mono.propertyGetName(stringProp), std::string("StringProp"));

	EXPECT_EQ(mono.propertyGetParent(floatProp), cls);
	EXPECT_EQ(mono.propertyGetParent(stringProp), cls);
}


TEST(MonoAPIPropertyTest, PropertyGetSet)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "PropertyTest");

	auto floatProp = mono.classGetPropertyFromName(cls, "FloatProp");
	auto stringProp = mono.classGetPropertyFromName(cls, "StringProp");

	auto obj = mono.objectNew(mono.domainGet(), cls);
	mono.runtimeInvoke(mono.classGetMethodFromName(cls, ".ctor", 2), obj, {5544.0f, mono.stringNew(mono.domainGet(), "Yet another string")});

	EXPECT_EQ(mono.objectUnbox<float>(mono.propertyGetValue(floatProp, obj)), 5544.0f);
	EXPECT_EQ(mono.stringToUTF8(mono.propertyGetValue(stringProp, obj)), std::string("Yet another string"));

	mono.propertySetValue(floatProp, obj, {98765.0f});
	EXPECT_EQ(mono.objectUnbox<float>(mono.propertyGetValue(floatProp, obj)), 98765.0f);

	mono.propertySetValue(stringProp, obj, {mono.stringNew(mono.domainGet(), "A different test string")});
	EXPECT_EQ(mono.stringToUTF8(mono.propertyGetValue(stringProp, obj)), std::string("A different test string"));
}


TEST(MonoAPIPropertyTest, PropertyGetSetMethod)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "PropertyTest");

	auto floatProp = mono.classGetPropertyFromName(cls, "FloatProp");
	auto stringProp = mono.classGetPropertyFromName(cls, "StringProp");

	auto floatPropGet = mono.propertyGetGetMethod(floatProp);
	auto floatPropSet = mono.propertyGetSetMethod(floatProp);

	auto stringPropGet = mono.propertyGetGetMethod(stringProp);
	auto stringPropSet = mono.propertyGetSetMethod(stringProp);

	auto obj = mono.objectNew(mono.domainGet(), cls);
	mono.runtimeInvoke(mono.classGetMethodFromName(cls, ".ctor", 2), obj, {5544.0f, mono.stringNew(mono.domainGet(), "Yet another string")});

	EXPECT_EQ(mono.objectUnbox<float>(mono.runtimeInvoke(floatPropGet, obj)), 5544.0f);
	EXPECT_EQ(mono.stringToUTF8(mono.runtimeInvoke(stringPropGet, obj)), std::string("Yet another string"));

	mono.runtimeInvoke(floatPropSet, obj, {98765.0f});
	EXPECT_EQ(mono.objectUnbox<float>(mono.runtimeInvoke(floatPropGet, obj)), 98765.0f);

	mono.runtimeInvoke(stringPropSet, obj, {mono.stringNew(mono.domainGet(), "A different test string")});
	EXPECT_EQ(mono.stringToUTF8(mono.runtimeInvoke(stringPropGet, obj)), std::string("A different test string"));
}



