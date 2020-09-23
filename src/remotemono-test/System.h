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

#include "config.h"

#include <BlackBone/Process/Process.h>
#include <remotemono/RMonoAPI.h>

using namespace remotemono;



class System
{
public:
	static System& getInstance();

public:
	void attach(const std::string& testAssemblyPath);
	void detach();

	blackbone::Process& getProcess() { return process; }
	RMonoAPI& getMono() { return mono; }
	RMonoHelperContext& getMonoHelperContext() { return helperCtx; }

	RMonoDomainPtr getTestDomain() { return testDomain; }
	RMonoAssemblyPtr getTestAssembly() { return testAssembly; }
	std::string getTestDomainFriendlyName() const { return testDomainFriendlyName; }

private:
	System();

private:
	blackbone::Process process;
	RMonoAPI mono;
	RMonoHelperContext helperCtx;

	RMonoDomainPtr testDomain;
	RMonoAssemblyPtr testAssembly;
	std::string testDomainFriendlyName;
};
