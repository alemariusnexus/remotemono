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

#include "config.h"

#include <string>
#include <vector>


class TestBackend
{
public:
	static std::vector<TestBackend*> getSupportedBackends() { return supportedBackends; }
	static TestBackend* getDefaultBackend();

public:
	std::string getID() const { return id; }
	int getPriority() const { return priority; }

	virtual void attachProcessByExecutablePath(std::string path) = 0;
	virtual void attachProcessByPID(DWORD pid) = 0;
	virtual void attachProcessByExecutableFilename(std::string name) = 0;
	virtual void terminateProcess() = 0;

protected:
	TestBackend(const std::string& id, int priority)
			: id(id), priority(priority)
	{
		supportedBackends.push_back(this);
	}

private:
	static std::vector<TestBackend*> supportedBackends;

private:
	std::string id;
	int priority;
};
