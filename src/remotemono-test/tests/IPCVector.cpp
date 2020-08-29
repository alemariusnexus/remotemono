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

#include <gtest/gtest.h>
#include "../System.h"



TEST(IPCVectorTest, Basic)
{
	RMonoAPIDispatcher* apid = System::getInstance().getMono().getAPIDispatcher();

	apid->apply([&](auto& e) {
		auto& ipcv = e.api.getIPCVector();

		auto v = ipcv.vectorNew(64);

		ASSERT_NE(v, 0);
		ASSERT_EQ(ipcv.vectorCapacity(v), 64);
		ASSERT_EQ(ipcv.vectorLength(v), 0);

		ipcv.vectorFree(v);
	});
}


TEST(IPCVectorTest, Manipulate)
{
	RMonoAPIDispatcher* apid = System::getInstance().getMono().getAPIDispatcher();

	apid->apply([&](auto& e) {
		typedef typename decltype(e.abi)::irmono_voidp irmono_voidp;

		auto& ipcv = e.api.getIPCVector();

		auto v = ipcv.vectorNew(8);

		ASSERT_NE(v, 0);
		ASSERT_EQ(ipcv.vectorCapacity(v), 8);

		ipcv.vectorAdd(v, irmono_voidp(10));
		ipcv.vectorAdd(v, irmono_voidp(20));
		ipcv.vectorAdd(v, irmono_voidp(30));
		ipcv.vectorAdd(v, irmono_voidp(40));
		ipcv.vectorAdd(v, irmono_voidp(50));

		ASSERT_EQ(ipcv.vectorLength(v), 5);
		ASSERT_EQ(ipcv.vectorCapacity(v), 8);

		std::vector<irmono_voidp> d;
		ipcv.read(v, d);
		ASSERT_EQ(d, std::vector<irmono_voidp>({10, 20, 30, 40, 50}));

		ipcv.vectorAdd(v, irmono_voidp(60));
		ipcv.vectorAdd(v, irmono_voidp(70));
		ipcv.vectorAdd(v, irmono_voidp(80));
		ipcv.vectorAdd(v, irmono_voidp(90));

		ASSERT_EQ(ipcv.vectorLength(v), 9);
		ASSERT_GE(ipcv.vectorCapacity(v), 9u);

		d.clear();
		ipcv.read(v, d);
		ASSERT_EQ(d, std::vector<irmono_voidp>({10, 20, 30, 40, 50, 60, 70, 80, 90}));

		ipcv.vectorGrow(v, 500);
		ASSERT_EQ(ipcv.vectorLength(v), 9);
		ASSERT_GE(ipcv.vectorCapacity(v), 500u);

		d.clear();
		ipcv.read(v, d);
		ASSERT_EQ(d, std::vector<irmono_voidp>({10, 20, 30, 40, 50, 60, 70, 80, 90}));

		ipcv.vectorClear(v);

		ASSERT_EQ(ipcv.vectorLength(v), 0);
		ASSERT_GE(ipcv.vectorCapacity(v), 500u);

		ipcv.vectorAdd(v, irmono_voidp(1337));

		d.clear();
		ipcv.read(v, d);
		ASSERT_EQ(d, std::vector<irmono_voidp>({1337}));

		ipcv.vectorFree(v);
	});
}


TEST(IPCVectorTest, Create)
{
	RMonoAPIDispatcher* apid = System::getInstance().getMono().getAPIDispatcher();

	apid->apply([&](auto& e) {
		typedef typename decltype(e.abi)::irmono_voidp irmono_voidp;

		auto& ipcv = e.api.getIPCVector();

		std::vector<irmono_voidp> d = {1, 2, 4, 8, 16, 32, 1337};
		auto v = ipcv.create(d);

		ASSERT_EQ(ipcv.vectorLength(v), (uint32_t) d.size());
		ASSERT_GE(ipcv.vectorCapacity(v), (uint32_t) d.size());

		std::vector<irmono_voidp> d2;
		ipcv.read(v, d2);

		ASSERT_EQ(d, d2);

		ipcv.vectorFree(v);
	});
}
