#pragma once

#include "../config.h"

#ifdef REMOTEMONO_BACKEND_BLACKBONE_ENABLED

#include "../TestBackend.h"
#include <remotemono/RMonoBackendBlackBone.h>

using namespace remotemono;


class BlackBoneTestBackend : public TestBackend
{
public:
	BlackBoneTestBackend();

	void attachProcessByExecutablePath(std::string path) override;
	void attachProcessByPID(DWORD pid) override;
	void attachProcessByExecutableFilename(std::string name) override;
	void terminateProcess() override;

private:
	blackbone::Process bbProc;
	backend::blackbone::RMonoBlackBoneProcess proc;
};

#endif
