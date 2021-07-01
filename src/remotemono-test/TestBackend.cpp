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

#include "TestBackend.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#include "backend/BlackBoneTestBackend.h"
#endif


std::vector<TestBackend*>* TestBackend::supportedBackends;


void TestBackend::init()
{
	supportedBackends = new std::vector<TestBackend*>();

	supportedBackends->push_back(new BlackBoneTestBackend);
}


void TestBackend::shutdown()
{
	for (TestBackend* backend : *supportedBackends) {
		delete backend;
	}
	delete supportedBackends;
}


std::vector<TestBackend*> TestBackend::getSupportedBackends()
{
	return *supportedBackends;
}


TestBackend* TestBackend::getDefaultBackend()
{
	TestBackend* bestBe = nullptr;
	for (TestBackend* be : getSupportedBackends()) {
		if (!bestBe  ||  (be->getPriority() < bestBe->getPriority())) {
			bestBe = be;
		}
	}
	return bestBe;
}
