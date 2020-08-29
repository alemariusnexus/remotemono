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


namespace remotemono
{


/**
 * Base class for most exceptions thrown by RemoteMono. Don't rely on it being the only that that RemoteMono can possibly throw.
 */
class RMonoException : public std::exception
{
public:
	typedef RMonoException Self;

public:
	RMonoException(const std::string& msg) noexcept : msg(msg) {}
	RMonoException(const Self& other) noexcept : msg(other.msg) {}

	Self& operator=(const Self& other) noexcept
	{
		if (this != &other) {
			msg = other.msg;
		}
		return *this;
	}

	virtual const char* what() const noexcept
	{
		return msg.data();
	}

private:
	std::string msg;
};


}
