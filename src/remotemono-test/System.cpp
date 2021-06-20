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

#include "System.h"

#include <filesystem>
#include "TestEnvException.h"


namespace fs = std::filesystem;


System& System::getInstance()
{
	static System inst;
	return inst;
}



System::System()
		: testBackend(nullptr), process(nullptr), mono(nullptr), helperCtx(nullptr)
{
}


void System::attach(const std::string& testAssemblyPath)
{
	mono = new RMonoAPI(getProcess());
	helperCtx = new RMonoHelperContext(mono);

	mono->attach();

	std::string absTestAssemblyPath = fs::absolute(fs::path(testAssemblyPath)).string();

	std::string fname = fs::path(testAssemblyPath).filename().string();
	testDomainFriendlyName = fname;

	if (mono->isAPIFunctionSupported("mono_domain_get_friendly_name")) {
		for (RMonoDomainPtr dom : mono->domainList()) {
			if (mono->domainGetFriendlyName(dom) == fname) {
				RMonoLogInfo("Unloading existing remotemono-test domain ...");
				mono->domainUnload(dom);
			}
		}

		for (RMonoDomainPtr dom : mono->domainList()) {
			if (mono->domainGetFriendlyName(dom) == fname) {
				throw TestEnvException("Domain still loaded after unloading.");
			}
		}
	}

	RMonoLogInfo("Creating test domain in remote process ...");
	testDomain = mono->domainCreateAppdomain(fname);

	if (!testDomain) {
		throw TestEnvException("Unable to create remote appdomain.");
	}

	mono->domainSet(testDomain, false);

	RMonoLogInfo("Opening test assembly in remote process ...");
	testAssembly = mono->domainAssemblyOpen(testDomain, absTestAssemblyPath);

	if (!testAssembly) {
		throw TestEnvException("Unable to open remote test assembly");
	}
}


void System::detach()
{
	// TODO: Can we somehow unload the domain and then still detach here?

	delete helperCtx;
	helperCtx = nullptr;

	mono->detach();
	delete mono;
	mono = nullptr;
}


backend::RMonoProcess& System::getProcess()
{
	if (!process) {
		throw TestEnvException("Process not open yet.");
	}
	return *process;
}


RMonoAPI& System::getMono()
{
	if (!mono) {
		throw TestEnvException("RMonoAPI not created yet.");
	}
	return *mono;
}


RMonoHelperContext& System::getMonoHelperContext()
{
	if (!helperCtx) {
		throw TestEnvException("RMonoHelperContext not created yet.");
	}
	return *helperCtx;
}
