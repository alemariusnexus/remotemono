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
