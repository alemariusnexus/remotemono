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

#pragma once

#include "../config.h"

#include <list>
#include "backend/RMonoProcess.h"



namespace remotemono
{


class RMonoHandleBackendBase;
class RMonoAPIDispatcher;



/**
 * The base class of RMonoAPI. This class is separate from RMonoAPI only in order to to reduce dependencies inside
 * RemoteMono's own code. See RMonoAPI for more details.
 */
class RMonoAPIBase
{
public:
	typedef std::list<RMonoHandleBackendBase*> HandleBackendList;
	typedef typename HandleBackendList::iterator HandleBackendIterator;

public:
	inline virtual ~RMonoAPIBase();

	/**
	 * Register a handle backend, so that RMonoHandleBackendBase::forceDelete() will be called on it when this instance
	 * is detached from the remote process, giving any leftover handles a last chance to free their resources to
	 * avoid leaking memory in the remote process.
	 */
	inline HandleBackendIterator registerMonoHandleBackend(RMonoHandleBackendBase* backend);

	/**
	 * Unregister a handle backend.
	 *
	 * @see registerMonoHandleBackend()
	 */
	inline void unregisterMonoHandleBackend(HandleBackendIterator backendIt);

	/**
	 * Returns the number of registered handles. Note that this may not be 0 even if you don't have any handles, because
	 * RMonoAPI keeps a few handles itself.
	 */
	inline size_t getRegisteredHandleCount() const;

	/**
	 * Returns the RMonoAPIDispatcher that is used to call the actual Mono API functions. You can use it to get direct
	 * access to the RMonoAPIFunction instances.
	 */
	inline RMonoAPIDispatcher* getAPIDispatcher();

	/**
	 * Returns the remote process.
	 */
	inline backend::RMonoProcess& getProcess();

protected:
	inline RMonoAPIBase(backend::RMonoProcess& process);

protected:
	RMonoAPIDispatcher* apid;
	HandleBackendList registeredHandles;

	backend::RMonoProcess& process;
};


}
