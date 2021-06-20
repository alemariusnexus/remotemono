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

#include "../../config.h"

#include <exception>
#include <string>


namespace remotemono
{


/**
 * This exception will be thrown when you attempt to call a Mono API method that is not available in the remote process.
 */
class RMonoUnsupportedAPIException : public std::exception
{
public:
	typedef RMonoUnsupportedAPIException Self;

public:
	RMonoUnsupportedAPIException(const std::string& apiName) noexcept
			: apiName(apiName), msg(std::string("Mono API not supported by remote: ").append(apiName)) {}
	RMonoUnsupportedAPIException(const Self& other) noexcept
			: apiName(other.apiName), msg(other.msg) {}

	Self& operator=(const Self& other) noexcept
	{
		if (this != &other) {
			apiName = other.apiName;
			msg = other.msg;
		}
		return *this;
	}

	virtual const char* what() const noexcept
	{
		return msg.data();
	}

	std::string getAPIFunctionName() const noexcept
	{
		return msg;
	}

private:
	std::string apiName;
	std::string msg;
};


}
