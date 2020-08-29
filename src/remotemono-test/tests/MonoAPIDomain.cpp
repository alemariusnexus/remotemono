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

#include <algorithm>
#include <gtest/gtest.h>
#include "../System.h"



TEST(MonoAPIDomainTest, DomainGet)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto dom = mono.domainGet();
	ASSERT_TRUE(dom);

	auto rdom = mono.getRootDomain();
	ASSERT_TRUE(rdom);

	ASSERT_NE(dom, rdom);
}


TEST(MonoAPIDomainTest, DomainSet)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto oldDom = mono.domainGet();
	auto rdom = mono.getRootDomain();

	ASSERT_TRUE(mono.domainSet(rdom, false));

	auto dom = mono.domainGet();

	ASSERT_NE(oldDom, dom);
	ASSERT_EQ(dom, rdom);

	ASSERT_TRUE(mono.domainSet(oldDom, false));
	ASSERT_EQ(mono.domainGet(), oldDom);
}


TEST(MonoAPIDomainTest, DomainList)
{
	RMonoAPI& mono = System::getInstance().getMono();

	auto doms = mono.domainList();
	auto dom = mono.domainGet();

	ASSERT_GE(doms.size(), 2u);

	//ASSERT_NE(doms.find(dom), doms.end());
	ASSERT_NE(std::find(doms.begin(), doms.end(), dom), doms.end());
}


TEST(MonoAPIDomainTest, DomainGetFriendlyName)
{
	System& sys = System::getInstance();
	RMonoAPI& mono = sys.getMono();
	std::string fname = sys.getTestDomainFriendlyName();

	if (mono.isAPIFunctionSupported("mono_domain_get_friendly_name")) {
		bool testFnameFound = false;

		for (auto dom : mono.domainList()) {
			std::string domFname = mono.domainGetFriendlyName(dom);

			if (domFname == fname) {
				testFnameFound = true;
				break;
			}
		}

		ASSERT_TRUE(testFnameFound);
	}
}
