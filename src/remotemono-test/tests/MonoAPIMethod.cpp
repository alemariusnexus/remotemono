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
#include <remotemono/impl/mono/metadata/blob.h>
#include <remotemono/impl/mono/metadata/metadata.h>
#include "../System.h"



TEST(MonoAPIMethodTest, MethodLookupSimple)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "MethodTest");

	auto simpleMethod = mono.classGetMethodFromName(cls, "SimpleMethod", 0);
	auto addFloat2 = mono.classGetMethodFromName(cls, "AddFloat", 2);
	auto addFloat3 = mono.classGetMethodFromName(cls, "AddFloat", 3);

	EXPECT_EQ(mono.methodGetName(simpleMethod), std::string("SimpleMethod"));

	EXPECT_EQ(mono.methodFullName(addFloat3, false), std::string("MethodTest:AddFloat"));
	EXPECT_EQ(mono.methodFullName(addFloat3, true), std::string("MethodTest:AddFloat (single,single,single)"));
}


TEST(MonoAPIMethodTest, MethodLookupDesc)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "MethodTest");

	auto addFloat2 = mono.classGetMethodFromName(cls, "AddFloat", 2);

	auto addFloat3 = mono.methodDescSearchInClass(":AddFloat(single,single,single)", false, cls);
	ASSERT_TRUE(addFloat3);
	EXPECT_EQ(mono.methodFullName(addFloat3, true), std::string("MethodTest:AddFloat (single,single,single)"));

	auto addFloat2Desc = mono.methodDescNew(":AddFloat(single,single)", false);

	EXPECT_FALSE(mono.methodDescMatch(addFloat2Desc, addFloat3));
	EXPECT_TRUE(mono.methodDescMatch(addFloat2Desc, addFloat2));

	auto addFloat3InImage = mono.methodDescSearchInImage("MethodTest:AddFloat(single,single,single)", false, img);
	ASSERT_TRUE(addFloat3InImage);
	EXPECT_EQ(addFloat3, addFloat3InImage);
}


TEST(MonoAPIMethodTest, MethodSignature)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "MethodTest");

	auto method = mono.classGetMethodFromName(cls, "InterestingSignatureMethod");
	ASSERT_TRUE(method);

	auto sig = mono.methodSignature(method);
	ASSERT_TRUE(sig);

	EXPECT_EQ(mono.signatureGetDesc(sig, false), std::string("string,int,int,single&"));

	auto retType = mono.signatureGetReturnType(sig);

	EXPECT_EQ(mono.typeGetType(retType), MONO_TYPE_STRING);
	EXPECT_EQ(mono.signatureGetCallConv(sig), MONO_CALL_DEFAULT);

	auto params = mono.signatureGetParams(sig);
	ASSERT_EQ(params.size(), 4);

	EXPECT_EQ(mono.typeGetType(params[0]), MONO_TYPE_STRING);
	EXPECT_EQ(mono.typeGetType(params[1]), MONO_TYPE_I4);
	EXPECT_EQ(mono.typeGetType(params[2]), MONO_TYPE_I4);
	EXPECT_EQ(mono.typeGetType(params[3]), MONO_TYPE_R4);

	EXPECT_TRUE(mono.typeIsByRef(params[3]));
}


TEST(MonoAPIMethodTest, RuntimeInvokeReferenceType)
{
	char buf[256];

	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "InvokeTest");
	auto pointCls = mono.classFromName(img, "", "MyPoint");

	auto pointFieldX = mono.classGetFieldFromName(pointCls, "x");
	auto pointFieldY = mono.classGetFieldFromName(pointCls, "y");

	mono.runtimeInvoke(mono.classGetMethodFromName(cls, "DoAbsolutelyNothing"), nullptr, {});

	mono.runtimeInvoke(mono.classGetMethodFromName(cls, "DoAbsolutelyNothingWithOneArg"), nullptr, {(int32_t) 1337});

	auto addRes = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "StaticAdd2"), nullptr, {69, 42});

	ASSERT_TRUE(addRes);
	EXPECT_EQ(mono.objectUnbox<int32_t>(addRes), int32_t(69+42));

	auto p1 = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "StaticGiveMeTwoPoints"), nullptr, {
			40.0f, 60.0f,
			110.0f, 10.0f,
			RMonoVariant(buf, 8, false).out()
	});
	auto p2 = mono.valueBox(mono.domainGet(), pointCls, RMonoVariant(buf, 8));

	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p1, pointFieldX), 40.0f);
	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p1, pointFieldY), 60.0f);

	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, pointFieldX), 110.0f);
	EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(p2, pointFieldY), 10.0f);

	{
		// Pass custom value types by raw pointer

		auto rp1 = mono.objectUnboxRaw(p1);
		auto rp2 = mono.objectUnboxRaw(p2);

		auto mid = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "StaticPointMid"), nullptr, {
				//mono.objectUnboxRaw(p1), mono.objectUnboxRaw(p2)
				rp1, rp2
		});

		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(mid, pointFieldX), 75.0f);
		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(mid, pointFieldY), 35.0f);
	}

	{
		// Pass custom value types by boxed object (auto-unboxing in wrapper function)
		auto mid = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "StaticPointMid"), nullptr, {p1, p2});
		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(mid, pointFieldX), 75.0f);
		EXPECT_FLOAT_EQ(mono.fieldGetValue<float>(mid, pointFieldY), 35.0f);
	}

	auto obj = mono.objectNew(mono.domainGet(), cls);

	mono.runtimeInvoke(mono.classGetMethodFromName(cls, ".ctor", 1), obj, {mono.stringNew(mono.domainGet(), "-")});

	RMonoStringPtr formatted;
	auto resObj = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "CalculateAndFormat"), obj,
			{123, 456, RMonoVariant(&formatted).out()}
			);

	EXPECT_EQ(mono.objectUnbox<int32_t>(resObj), -333);
	EXPECT_EQ(mono.stringToUTF8(formatted), std::string("123-456 = -333"));

	auto throwIfNegative = mono.classGetMethodFromName(cls, "ThrowIfNegative");

	EXPECT_NO_THROW(mono.runtimeInvoke(throwIfNegative, obj, {0.3f}));

	try {
		mono.runtimeInvoke(throwIfNegative, obj, {-0.1f});
		FAIL() << "ThrowIfNegative() didn't throw for negative value.";
	} catch (RMonoRemoteException& ex) {
		auto mex = ex.getMonoException();
		EXPECT_TRUE(mex);
		EXPECT_TRUE(mono.objectIsInst(mex, mono.getExceptionClass()));
		EXPECT_NE(ex.getMessage().find("Parameter is negative!"), std::string::npos);
	}
}


TEST(MonoAPIMethodTest, RuntimeInvokeValueType)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "MyPoint");

	auto p1 = mono.objectNew(mono.domainGet(), cls);
	mono.runtimeInvoke(mono.classGetMethodFromName(cls, ".ctor", 2), mono.objectUnboxRaw(p1), {69.0f, 1337.0f});

	{
		// Invoke on value type object by passing raw pointer
		auto lenObj = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "length"), mono.objectUnboxRaw(p1));
		ASSERT_TRUE(lenObj);
		EXPECT_FLOAT_EQ(mono.objectUnbox<float>(lenObj), 1338.779f);
	}

	{
		// Invoke on value type object by passing boxed object (auto-unboxing in wrapper function)
		auto lenObj = mono.runtimeInvoke(mono.classGetMethodFromName(cls, "length"), p1);
		ASSERT_TRUE(lenObj);
		EXPECT_FLOAT_EQ(mono.objectUnbox<float>(lenObj), 1338.779f);
	}
}


TEST(MonoAPIMethodTest, RuntimeInvokeWithRetCls)
{
	char buf[256];

	RMonoAPI& mono = System::getInstance().getMono();

	auto ass = mono.assemblyLoaded("remotemono-test-target-mono");
	auto img = mono.assemblyGetImage(ass);

	auto cls = mono.classFromName(img, "", "InvokeTest");

	RMonoClassPtr retvalCls;
	auto retval = mono.runtimeInvokeWithRetCls(retvalCls, mono.classGetMethodFromName(cls, "GiveMeAString"), nullptr, {});

	ASSERT_TRUE(retval);

	RMonoClassPtr actualRetvalCls = mono.objectGetClass(retval);

	EXPECT_EQ(actualRetvalCls, mono.getStringClass());
	EXPECT_NE(actualRetvalCls, mono.getExceptionClass());

	EXPECT_TRUE(retvalCls);
	EXPECT_EQ(retvalCls, actualRetvalCls);
}




