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



TEST(RMonoHelpersTest, ClassCreateTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls;
		EXPECT_FALSE(cls.isValid());
		EXPECT_TRUE(cls.isNull());
		EXPECT_FALSE(cls);

		RMonoClass cls2(nullptr);
		EXPECT_FALSE(cls2);
		EXPECT_EQ(cls2, cls);
	}

	{
		RMonoClass cls(hc, img, "", "RemoteMonoDerived");
		EXPECT_TRUE(cls);
	}

	{
		RMonoClass cls(hc, img, "remotemono", "RemoteMonoNamespacedClass");
		EXPECT_TRUE(cls);

		auto cls2 = mono.classFromName(img, "remotemono", "RemoteMonoNamespacedClass");
		EXPECT_EQ(cls2, *cls);
		EXPECT_EQ(cls2, cls);

		RMonoClass cls3(hc, cls2);
		EXPECT_TRUE(cls3);
		EXPECT_EQ(cls3, cls);
		EXPECT_EQ(*cls3, *cls);
	}

	{
		RMonoClass cls(hc, img, "", "RemoteMonoDerived/DoesNotExist");
		EXPECT_FALSE(cls);
	}
}


TEST(RMonoHelpersTest, ClassNameTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls(hc, img, "", "RemoteMonoDerived");
		EXPECT_EQ(cls.getName(), "RemoteMonoDerived");
		EXPECT_EQ(cls.getNamespace(), "");
	}

	{
		RMonoClass cls(hc, img, "remotemono", "RemoteMonoNamespacedClass");
		EXPECT_EQ(cls.getName(), "RemoteMonoNamespacedClass");
		EXPECT_EQ(cls.getNamespace(), "remotemono");
	}
}


TEST(RMonoHelpersTest, ObjectCreateTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass fieldTestCls(hc, img, "", "HelperFieldTest");

	{
		RMonoObject o;
		EXPECT_FALSE(o.isValid());
		EXPECT_TRUE(o.isNull());
		EXPECT_FALSE(o);

		RMonoObject o2(hc, RMonoObjectPtr());
		EXPECT_FALSE(o2);
		EXPECT_EQ(o, o2);

		RMonoObject o3(nullptr);
		EXPECT_FALSE(o3);
		EXPECT_EQ(o, o3);
	}

	{
		auto ro = mono.fieldGetValueObject(mono.classGetFieldFromName(fieldTestCls, "Instance"));
		ASSERT_TRUE(ro);

		RMonoObject o(hc, ro, fieldTestCls);
		EXPECT_TRUE(o);
		EXPECT_EQ(**o, *ro);
		EXPECT_EQ(*o, ro);
		EXPECT_EQ(o, ro);

		RMonoObject o2(hc, ro);
		EXPECT_TRUE(o2);
		EXPECT_EQ(o2, o);
	}
}


TEST(RMonoHelpersTest, ObjectNewTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass fieldTestCls(hc, img, "", "HelperFieldTest");

		RMonoObject o = fieldTestCls.allocObject();
		EXPECT_TRUE(o);

		mono.runtimeInvoke(mono.classGetMethodFromName(fieldTestCls, ".ctor", 0), *o);

		EXPECT_EQ(mono.fieldGetValue<int32_t>(o, mono.classGetFieldFromName(fieldTestCls, "IntField")), 13);
	}

	{
		RMonoClass cls(hc, img, "", "MyPoint");

		RMonoObject p1 = cls.newObject(3.0f, 4.0f);

		EXPECT_FLOAT_EQ(mono.objectUnbox<float>(mono.runtimeInvoke(mono.classGetMethodFromName(cls, "length"), p1)), 5.0f);

		EXPECT_ANY_THROW(cls.newObject(1.0f, 2.0f, 3.0f));
		EXPECT_ANY_THROW(cls.newObject(1.0f));
	}

	{
		RMonoClass cls(hc, img, "", "HelperNewObjectTest");

		RMonoObject o1 = cls.newObjectDesc("single,string", 18.1f, hc->str("Test 1"));
		EXPECT_TRUE(o1);

		RMonoObject o2 = cls.newObjectDesc("int,string", 13, hc->str("Test 2"));
		EXPECT_TRUE(o2);

		EXPECT_EQ(o1.field("constructorUsed").get<int32_t>(), 1);
		EXPECT_EQ(o2.field("constructorUsed").get<int32_t>(), 2);
	}
}


TEST(RMonoHelpersTest, ObjectToVariantTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass fieldTestCls(hc, img, "", "HelperFieldTest");

	{
		RMonoObject o = fieldTestCls.allocObject();
		EXPECT_TRUE(o);

		RMonoVariant v(o);
		EXPECT_EQ(v.getType(), RMonoVariant::TypeMonoObjectPtr);
		EXPECT_EQ(v.getMonoObjectPtr(), *o);

		RMonoVariant v2 = o;
		EXPECT_EQ(v2.getType(), RMonoVariant::TypeMonoObjectPtr);
		EXPECT_EQ(v2.getMonoObjectPtr(), *o);
	}
}


TEST(RMonoHelpersTest, ObjectInOutTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls(hc, img, "", "InvokeTest");

		RMonoObject opObj(hc, mono.stringNew("+"));

		RMonoObject o = cls.allocObject();
		mono.runtimeInvoke(mono.classGetMethodFromName(cls, ".ctor", 1), o, {opObj});

		auto calcAndFormat = mono.classGetMethodFromName(cls, "CalculateAndFormat");
		auto calcAndFormatWithPrefix = mono.classGetMethodFromName(cls, "CalculateAndFormatWithPrefix");

		RMonoObject strObj(hc);
		mono.runtimeInvoke(calcAndFormat, o, {15, 8, strObj.out()});
		EXPECT_EQ(mono.stringToUTF8(strObj), std::string("15+8 = 23"));

		RMonoObject strObj2(hc, mono.stringNew("Original value"));
		mono.runtimeInvoke(calcAndFormat, o, {7, 1, strObj2});
		EXPECT_EQ(mono.stringToUTF8(strObj2), std::string("Original value"));

		RMonoObject strObj3(hc, mono.stringNew("Original value"));
		mono.runtimeInvoke(calcAndFormat, o, {7, 1, strObj3.inout()});
		EXPECT_EQ(mono.stringToUTF8(strObj3), std::string("7+1 = 8"));

		RMonoObject strObj4(hc, mono.stringNew("Original value: "));
		mono.runtimeInvoke(calcAndFormatWithPrefix, o, {7, 1, strObj4.inout()});
		EXPECT_EQ(mono.stringToUTF8(strObj4), std::string("Original value: 7+1 = 8"));

		RMonoObject strObj5(hc);
		mono.runtimeInvoke(calcAndFormatWithPrefix, o, {7, 1, strObj5.inout()});
		EXPECT_EQ(mono.stringToUTF8(strObj5), std::string("7+1 = 8"));
	}
}


TEST(RMonoHelpersTest, ArrayTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto i32Cls = mono.getInt32Class();

	{
		auto arrPtr = mono.arrayFromVector<int32_t>(i32Cls, {10, 20, 30, 40, 50, 60});
		EXPECT_EQ(mono.arrayLength(arrPtr), 6);
		EXPECT_EQ(mono.arrayAsVector<int32_t>(arrPtr), std::vector<int32_t>({10, 20, 30, 40, 50, 60}));

		RMonoObject arr(hc, arrPtr);
		EXPECT_EQ(arr.arrayAsVector<int32_t>(), std::vector<int32_t>({10, 20, 30, 40, 50, 60}));
	}

	{
		auto arrPtr = mono.arrayFromVector<RMonoStringPtr>(mono.getStringClass(), {
				mono.stringNew("This"),
				mono.stringNew("is"),
				mono.stringNew("a"),
				mono.stringNew("test")
		});
		RMonoObject arr(hc, arrPtr);
		auto arrVec = arr.arrayAsVector();

		EXPECT_EQ(mono.stringToUTF8(arrVec[0]), std::string("This"));
		EXPECT_EQ(mono.stringToUTF8(arrVec[1]), std::string("is"));
		EXPECT_EQ(mono.stringToUTF8(arrVec[2]), std::string("a"));
		EXPECT_EQ(mono.stringToUTF8(arrVec[3]), std::string("test"));
	}
}


TEST(RMonoHelpersTest, FieldFromClassTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls(hc, img, "", "HelperFieldTest");
		ASSERT_TRUE(cls);

		auto staticIntField = cls.field("StaticIntField");
		EXPECT_TRUE(staticIntField);

		// Test that the cached fields are valid
		EXPECT_EQ(*staticIntField, *cls.field("StaticIntField"));
		EXPECT_EQ(staticIntField, cls.field("StaticIntField"));

		auto staticStringField = cls.field("IntField");
		EXPECT_TRUE(staticStringField);
	}

	{
		RMonoClass cls(hc, img, "", "HelperValFieldTest");
		ASSERT_TRUE(cls);
	}
}


TEST(RMonoHelpersTest, FieldFromObjectTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls(hc, img, "", "HelperValFieldTest");
		ASSERT_TRUE(cls);

		auto ro = mono.fieldGetValueObject(mono.classGetFieldFromName(cls, "Instance"));
		ASSERT_TRUE(ro);

		RMonoObject o(hc, ro, cls);

		auto intField = o.field("IntField");
		EXPECT_TRUE(intField);

		// Test that the cached fields are valid
		EXPECT_EQ(*intField, *o.field("IntField"));
		EXPECT_EQ(intField, o.field("IntField"));
	}
}


TEST(RMonoHelpersTest, FieldMiscMethodsTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls(hc, img, "", "HelperValFieldTest");
		ASSERT_TRUE(cls);

		auto ro = mono.fieldGetValueObject(mono.classGetFieldFromName(cls, "Instance"));
		ASSERT_TRUE(ro);

		RMonoObject o(hc, ro, cls);

		auto stringField = cls.field("StringField");
		auto pointField = cls.field("PointField");
		auto staticIntField = cls.field("StaticIntField");

		EXPECT_FALSE(stringField.isStatic());
		EXPECT_FALSE(pointField.isStatic());
		EXPECT_TRUE(staticIntField.isStatic());

		EXPECT_EQ(stringField.getClass(), cls);
		EXPECT_EQ(staticIntField.getClass(), cls);

		EXPECT_FALSE(stringField.isInstanced());
		EXPECT_FALSE(pointField.isInstanced());
		EXPECT_FALSE(staticIntField.isInstanced());

		auto instPointField = o.field("PointField");

		EXPECT_FALSE(instPointField.isStatic());
		EXPECT_TRUE(instPointField.isInstanced());

		EXPECT_EQ(instPointField, pointField);
	}
}


TEST(RMonoHelpersTest, FieldGetSetValueOnClassTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls(hc, img, "", "HelperFieldTest");
	ASSERT_TRUE(cls);

	{
		int32_t ival = 0;
		cls.field("StaticIntField").get(&ival);
		EXPECT_EQ(ival, 25);

		EXPECT_EQ(cls.field("StaticIntField").get<int32_t>(), 25);

		EXPECT_EQ(mono.stringToUTF8(cls.field("StaticStringField").get()), std::string("jumps over the lazy dog"));
		EXPECT_EQ(mono.stringToUTF8(cls.field("StaticStringField").get<RMonoObject>()), std::string("jumps over the lazy dog"));
		EXPECT_EQ(mono.stringToUTF8(cls.field("StaticStringField").get<RMonoObjectPtr>()), std::string("jumps over the lazy dog"));
	}

	{
		cls.field("StaticIntField").set(1337);
		EXPECT_EQ(cls.field("StaticIntField").get<int32_t>(), 1337);

		cls.field("StaticStringField").set(RMonoObject(hc, mono.stringNew("bla bla overwritten by test")));
		EXPECT_EQ(mono.stringToUTF8(cls.field("StaticStringField").get()), std::string("bla bla overwritten by test"));
	}
}


TEST(RMonoHelpersTest, FieldGetSetValueOnObjectTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls(hc, img, "", "HelperFieldTest");
	ASSERT_TRUE(cls);

	{
		auto obj = cls.newObject();

		int32_t ival = 0;
		obj.field("IntField").get(&ival);
		EXPECT_EQ(ival, 13);

		EXPECT_EQ(obj.field("IntField").get<int32_t>(), 13);

		EXPECT_EQ(mono.stringToUTF8(obj.field("StringField").get()), std::string("The quick brown fox"));
	}

	{
		auto obj = cls.newObject();

		obj.field("IntField").set(42069);
		EXPECT_EQ(obj.field("IntField").get<int32_t>(), 42069);

		obj.field("StringField").set(mono.stringNew("Wayne interessierts?"));
		EXPECT_EQ(mono.stringToUTF8(obj.field("StringField").get()), std::string("Wayne interessierts?"));
	}

	{
		auto obj = cls.newObject();

		cls.field("IntField").inst(obj).set(42069);
		EXPECT_EQ(cls.field("IntField").inst(obj).get<int32_t>(), 42069);
	}

	{
		auto obj = cls.newObject();

		obj.field("StaticIntField").set(25);
		EXPECT_EQ(obj.field("StaticIntField").get<int32_t>(), 25);
	}

	{
		EXPECT_ANY_THROW(cls.field("IntField").set(0xDEADBEEF));
		EXPECT_ANY_THROW(cls.field("StringField").set(0xDEADBEEF));
	}
}


TEST(RMonoHelpersTest, MethodFromClassTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "InvokeTest");
	ASSERT_TRUE(cls);

	{
		ASSERT_TRUE(cls.method("DoAbsolutelyNothing"));

		auto dan = cls.method("DoAbsolutelyNothingWithOneArg");

		ASSERT_EQ(cls.method("DoAbsolutelyNothingWithOneArg"), dan);
		ASSERT_EQ(cls.method("DoAbsolutelyNothingWithOneArg"), dan);

		ASSERT_EQ(cls.method("DoAbsolutelyNothingWithOneArg", 1), dan);
		ASSERT_EQ(cls.method("DoAbsolutelyNothingWithOneArg", 1), dan);

		ASSERT_FALSE(cls.method("DoAbsolutelyNothingWithOneArg", 2));
		ASSERT_FALSE(cls.method("DoAbsolutelyNothingWithOneArg", 2));

		ASSERT_EQ(cls.methodDesc(":DoAbsolutelyNothingWithOneArg(int)"), cls.method("DoAbsolutelyNothingWithOneArg", 1));
	}
}


TEST(RMonoHelpersTest, MethodFromObjectTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "MethodTest");

	{
		RMonoObject o = cls.newObject();

		auto addFloat2 = o.method("AddFloat", 2);
		ASSERT_TRUE(addFloat2);

		auto addFloat3 = o.method("AddFloat", 3);
		ASSERT_TRUE(addFloat3);

		ASSERT_EQ(o.method("AddFloat", 3), addFloat3);
		ASSERT_EQ(o.method("AddFloat", 2), addFloat2);

		ASSERT_EQ(o.methodDesc(":AddFloat(single,single,single)"), addFloat3);
	}
}


TEST(RMonoHelpersTest, MethodMiscMethodsTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "InvokeTest");

	{
		RMonoObject o = cls.newObject(hc->str("-"));

		auto sa2 = cls.method("StaticAdd2");
		auto caf = cls.method("CalculateAndFormat");
		auto instCaf = o.method("CalculateAndFormat");

		EXPECT_TRUE(sa2.isStatic());
		EXPECT_FALSE(caf.isStatic());
		EXPECT_FALSE(instCaf.isStatic());

		EXPECT_EQ(caf, instCaf);

		EXPECT_FALSE(sa2.isInstanced());
		EXPECT_FALSE(caf.isInstanced());
		EXPECT_TRUE(instCaf.isInstanced());
	}
}


TEST(RMonoHelpersTest, MethodInvokeTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	{
		RMonoClass cls = hc->classFromName(img, "", "InvokeTest");
		RMonoObject o = cls.newObject(hc->str("-"));

		auto sa2 = cls.method("StaticAdd2");
		auto instSa2 = cls.method("StaticAdd2");
		auto caf = cls.method("CalculateAndFormat");
		auto instCaf = o.method("CalculateAndFormat");

		EXPECT_EQ(sa2(18, -11).unbox<int32_t>(), 7);
		EXPECT_EQ(instSa2(-1, 7).unbox<int32_t>(), 6);

		RMonoObject formatted(hc);
		EXPECT_EQ(instCaf(9, 6, formatted.out()).unbox<int32_t>(), 3);
		EXPECT_EQ(formatted.str(), std::string("9-6 = 3"));

		EXPECT_ANY_THROW(caf(1, 2, formatted.out()));
	}

	{
		RMonoClass cls = hc->classFromName(img, "", "HelperMethodRetTypeTest");

		auto str = cls.method("GiveMeAString")();
		EXPECT_NE(str.getClass(), cls);
		EXPECT_EQ(str.getClass(), hc->classString());
	}
}


TEST(RMonoHelpersTest, PropertyFromClassTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "HelperPropTest");
	ASSERT_TRUE(cls);

	{
		ASSERT_TRUE(cls.property("StaticFloatProp"));

		auto intProp = cls.property("IntProp");
		EXPECT_TRUE(intProp);
		EXPECT_EQ(cls.property("IntProp"), intProp);
		EXPECT_EQ(cls.property("IntProp"), intProp);

		EXPECT_FALSE(cls.property("InvalidProp"));
		EXPECT_FALSE(cls.property("InvalidProp"));
	}
}


TEST(RMonoHelpersTest, PropertyFromObjectTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "HelperPropTest");
	ASSERT_TRUE(cls);

	{
		auto o = cls.newObject(hc->str("Test 1"), 17);

		EXPECT_TRUE(o.property("StaticFloatProp"));
		EXPECT_EQ(o.property("StaticFloatProp"), cls.property("StaticFloatProp"));

		EXPECT_TRUE(o.property("StringProp"));
		EXPECT_EQ(o.property("StringProp"), cls.property("StringProp"));
	}
}


TEST(RMonoHelpersTest, PropertyGetSetStaticTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "HelperPropTest");
	ASSERT_TRUE(cls);

	{
		auto staticFloatProp = cls.property("StaticFloatProp");
		auto stringProp = cls.property("StringProp");

		EXPECT_TRUE(staticFloatProp);
		EXPECT_TRUE(stringProp);

		EXPECT_FLOAT_EQ(staticFloatProp.get().unbox<float>(), 13.37f);

		staticFloatProp.set(69.420f);
		EXPECT_FLOAT_EQ(staticFloatProp.get().unbox<float>(), 69.420f);

		EXPECT_ANY_THROW(stringProp.get());
		EXPECT_ANY_THROW(stringProp.set(hc->str("Will not work")));

		staticFloatProp.setter().invoke(4.2f);
		EXPECT_FLOAT_EQ(staticFloatProp.getter().invoke().unbox<float>(), 4.2f);
	}
}


TEST(RMonoHelpersTest, PropertyGetSetTest)
{
	RMonoAPI& mono = System::getInstance().getMono();
	RMonoHelperContext* hc = &System::getInstance().getMonoHelperContext();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	RMonoClass cls = hc->classFromName(img, "", "HelperPropTest");
	ASSERT_TRUE(cls);

	{
		auto o = cls.newObject(hc->str("Test 1"), 17);

		EXPECT_EQ(o.property("IntProp").get().unbox<int32_t>(), 17);
		EXPECT_EQ(o.property("StringProp").get().str(), std::string("Test 1"));

		o.property("StringProp").set(hc->str("A different string"));
		EXPECT_EQ(o.property("StringProp").get().str(), std::string("A different string"));

		o.property("StaticFloatProp").set(77.7f);
		EXPECT_EQ(o.property("StaticFloatProp").get().unbox<float>(), 77.7f);

		o.property("IntProp").setter().invoke(98765);
		EXPECT_EQ(o.property("IntProp").getter().invoke().unbox<int32_t>(), 98765);
	}
}



