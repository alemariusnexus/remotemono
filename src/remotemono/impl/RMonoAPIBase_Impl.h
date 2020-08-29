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

#pragma once

#include "../config.h"

#include "RMonoAPIBase_Def.h"

#include "RMonoAPIDispatcher_Def.h"



namespace remotemono
{


RMonoAPIBase::RMonoAPIBase(blackbone::Process& process)
		: apid(new RMonoAPIDispatcher), process(process), worker(nullptr)
{
}


RMonoAPIBase::~RMonoAPIBase()
{
	delete apid;
}


typename RMonoAPIBase::HandleBackendIterator RMonoAPIBase::registerMonoHandleBackend(RMonoHandleBackendBase* backend)
{
	registeredHandles.push_back(backend);
	HandleBackendIterator it = registeredHandles.end();
	it--;
	return it;
}


void RMonoAPIBase::unregisterMonoHandleBackend(HandleBackendIterator backendIt)
{
	registeredHandles.erase(backendIt);
}


size_t RMonoAPIBase::getRegisteredHandleCount() const
{
	return registeredHandles.size();
}


RMonoAPIDispatcher* RMonoAPIBase::getAPIDispatcher()
{
	return apid;
}


blackbone::Process& RMonoAPIBase::getProcess()
{
	return process;
}


blackbone::ThreadPtr RMonoAPIBase::getWorkerThread()
{
	return worker;
}


}
