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

#include "../../config.h"

#include <exception>
#include <string>
#include "../RMonoTypes.h"
#include "../RMonoHandle_Def.h"
#include "../../log.h"



namespace remotemono
{



/**
 * A local representation of an exception thrown by managed remote code. When exception handling is requested, exceptions
 * thrown by managed remote code will be detected by RMonoAPIFunctionAPI, and an object of this class is created and thrown
 * in the local process. This way, managed exceptions are thrown "across" the process barrier and can be handled as normal
 * C++ exceptions.
 *
 * You can get access to the managed MonoException* object by calling getMonoException(). You can then use the normal
 * RemoteMono APIs to access fields, properties or methods on this managed object to get more detailed information about
 * the exception.
 */
class RMonoRemoteException : public std::exception
{
public:
	typedef RMonoRemoteException Self;

public:
	RMonoRemoteException(RMonoExceptionPtr ex) noexcept
			: ex(ex), dataFetched(false) { fetchRemoteData(); }
	RMonoRemoteException(const Self& other) noexcept
			: ex(other.ex), dataFetched(other.dataFetched), message(other.message), toStrRes(other.toStrRes) {}

	Self& operator=(const Self& other) noexcept
	{
		if (this != &other) {
			ex = other.ex;
			dataFetched = other.dataFetched;
			message = other.message;
			toStrRes = other.toStrRes;
		}
		return *this;
	}

	virtual const char* what() const noexcept
	{
		const_cast<Self*>(this)->fetchRemoteData();

		if (!toStrRes.empty()) {
			return toStrRes.data();
		}

		return "MonoException";
	}

	std::string getMessage() noexcept
	{
		fetchRemoteData();
		return message;
	}

	RMonoExceptionPtr getMonoException() { return ex; }

private:
	inline void fetchRemoteData() noexcept;

private:
	RMonoExceptionPtr ex;

	bool dataFetched;
	std::string message;
	std::string toStrRes;
};



};
