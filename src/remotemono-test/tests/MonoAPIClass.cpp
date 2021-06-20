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



TEST(MonoAPIClassTest, ClassFromName)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");
	EXPECT_TRUE(cls);

	cls = mono.classFromName(img, "remotemono", "RemoteMonoNamespacedClass");
	EXPECT_TRUE(cls);

	cls = mono.classFromName(img, "", "RemoteMonoDerived/Nested");
	EXPECT_TRUE(cls);

	cls = mono.classFromName(img, "", "RemoteMonoDerived/DoesNotExist");
	EXPECT_FALSE(cls);
}


TEST(MonoAPIClassTest, ClassName)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");
	EXPECT_EQ(mono.classGetName(cls), std::string("RemoteMonoDerived"));
	EXPECT_EQ(mono.classGetNamespace(cls), std::string(""));

	cls = mono.classFromName(img, "remotemono", "RemoteMonoNamespacedClass");
	EXPECT_EQ(mono.classGetName(cls), std::string("RemoteMonoNamespacedClass"));
	EXPECT_EQ(mono.classGetNamespace(cls), std::string("remotemono"));
}


TEST(MonoAPIClassTest, ClassGetFields)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	std::set<std::string> foundFields;

	for (auto field : mono.classGetFields(cls)) {
		foundFields.insert(mono.fieldGetName(field));
	}

	EXPECT_NE(foundFields.find("privateField"), foundFields.end());
	EXPECT_NE(foundFields.find("publicField"), foundFields.end());
}


TEST(MonoAPIClassTest, ClassGetMethods)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	std::set<std::string> foundMethods;

	for (auto method : mono.classGetMethods(cls)) {
		foundMethods.insert(mono.methodGetName(method));
	}

	EXPECT_NE(foundMethods.find("ProtectedMethod"), foundMethods.end());
	EXPECT_NE(foundMethods.find("UnqualifiedMethod"), foundMethods.end());
}


TEST(MonoAPIClassTest, ClassGetProperties)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	std::set<std::string> foundProps;

	for (auto prop : mono.classGetProperties(cls)) {
		foundProps.insert(mono.propertyGetName(prop));
	}

	EXPECT_NE(foundProps.find("PublicFieldProp"), foundProps.end());
	EXPECT_NE(foundProps.find("PrivateFieldProp"), foundProps.end());
}


TEST(MonoAPIClassTest, ClassGetElementsFromName)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	EXPECT_TRUE(mono.classGetFieldFromName(cls, "privateField"));
	EXPECT_FALSE(mono.classGetFieldFromName(cls, "blablaField"));

	EXPECT_TRUE(mono.classGetMethodFromName(cls, "UnqualifiedMethod"));
	EXPECT_TRUE(mono.classGetMethodFromName(cls, "UnqualifiedMethod", 2));
	EXPECT_FALSE(mono.classGetMethodFromName(cls, "UnqualifiedMethod", 1));
	EXPECT_FALSE(mono.classGetMethodFromName(cls, "QualifiedMethod"));
	EXPECT_FALSE(mono.classGetMethodFromName(cls, "QualifiedMethod", 0));

	EXPECT_TRUE(mono.classGetPropertyFromName(cls, "PublicFieldProp"));
	EXPECT_FALSE(mono.classGetPropertyFromName(cls, "PorousFieldProp"));
}


TEST(MonoAPIClassTest, ClassGetFlags)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived/Nested");

	uint32_t flags = mono.classGetFlags(cls);

	EXPECT_TRUE((flags & TYPE_ATTRIBUTE_ABSTRACT)  !=  0);
	EXPECT_TRUE((flags & TYPE_ATTRIBUTE_INTERFACE)  !=  0);
	EXPECT_EQ((flags & TYPE_ATTRIBUTE_VISIBILITY_MASK), TYPE_ATTRIBUTE_NESTED_PRIVATE);
	EXPECT_FALSE((flags & TYPE_ATTRIBUTE_SEALED)  !=  0);
	EXPECT_FALSE((flags & TYPE_ATTRIBUTE_EXPLICIT_LAYOUT)  !=  0);
}


TEST(MonoAPIClassTest, ClassList)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	std::set<std::string> foundClasses;

	for (auto cls : mono.listClasses(img)) {
		foundClasses.insert(mono.classGetName(cls));
	}

	EXPECT_NE(foundClasses.find("RemoteMonoNamespacedClass"), foundClasses.end());
	EXPECT_NE(foundClasses.find("RemoteMonoBase"), foundClasses.end());
	EXPECT_NE(foundClasses.find("RemoteMonoDerived"), foundClasses.end());
	EXPECT_NE(foundClasses.find("RemoteMonoTestTarget"), foundClasses.end());
	EXPECT_NE(foundClasses.find("MyPoint"), foundClasses.end());
}


TEST(MonoAPIClassTest, ClassIsValueType)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	EXPECT_FALSE(mono.classIsValueType(mono.classFromName(img, "", "RemoteMonoBase")));
	EXPECT_TRUE(mono.classIsValueType(mono.classFromName(img, "", "MyPoint")));
}


TEST(MonoAPIClassTest, ClassGetParent)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "RemoteMonoDerived");

	auto clsParent = mono.classGetParent(cls);
	ASSERT_TRUE(clsParent);
	EXPECT_EQ(mono.classGetName(clsParent), std::string("RemoteMonoBase"));

	auto clsParentParent = mono.classGetParent(clsParent);
	ASSERT_TRUE(clsParentParent);
	EXPECT_EQ(mono.classGetName(clsParentParent), std::string("Object"));

	auto clsParentParentParent = mono.classGetParent(clsParentParent);
	ASSERT_FALSE(clsParentParentParent);
}


