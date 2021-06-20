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



TEST(MonoAPIStringTest, StringTest)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();

	EXPECT_EQ(mono.stringToUTF8(mono.stringNew(dom, "Hello World!")), std::string("Hello World!"));
	EXPECT_EQ(mono.stringToUTF16(mono.stringNew(dom, "Convert me")), u"Convert me");
	EXPECT_EQ(mono.stringToUTF32(mono.stringNew(dom, "Convert me")), U"Convert me");

	EXPECT_EQ(mono.stringToUTF8(mono.stringNewUTF16(dom, u"More conversions")), std::string("More conversions"));
	EXPECT_EQ(mono.stringToUTF8(mono.stringNewUTF32(dom, U"More conversions")), std::string("More conversions"));

	EXPECT_EQ(mono.stringLength(mono.stringNew(dom, "A few words make up a string.")), 29);
	EXPECT_EQ(mono.stringLength(mono.stringNewUTF16(dom, u"Works with Unicode as well!")), 27);

	EXPECT_EQ(mono.stringToUTF8(mono.stringNew(dom, u8"日本語もいいよ。")), u8"日本語もいいよ。");
	EXPECT_EQ(mono.stringToUTF8(mono.stringNewUTF16(dom, u"日本語もいいよ。")), u8"日本語もいいよ。");
	EXPECT_EQ(mono.stringToUTF8(mono.stringNewUTF32(dom, U"日本語もいいよ。")), u8"日本語もいいよ。");

	EXPECT_TRUE(mono.stringEqual(mono.stringNew(dom, u8"これは面白い文字列ね"), mono.stringNewUTF16(dom, u"これは面白い文字列ね")));
	EXPECT_FALSE(mono.stringEqual(mono.stringNew(dom, u8"これは面白い文字列ね"), mono.stringNewUTF16(dom, u"それも面白い文字列ね")));
}
