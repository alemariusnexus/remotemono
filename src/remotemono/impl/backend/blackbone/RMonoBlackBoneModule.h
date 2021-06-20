#pragma once

#include "../../../config.h"

#include <string>
#include "../RMonoModule.h"
#include "../../../util.h"
#include <BlackBone/Process/Process.h>

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "Included BlackBone backend header, but BlackBone backend is disabled."
#endif


namespace remotemono
{
namespace backend
{
namespace blackbone
{


class RMonoBlackBoneModule : public RMonoModule
{
public:
	RMonoBlackBoneModule(::blackbone::Process& process, ::blackbone::ModuleDataPtr modulePtr)
			: process(process), modulePtr(modulePtr)
	{
	}

	::blackbone::ModuleDataPtr operator*() { return modulePtr; }

	bool getExport(Export& exp, const std::string& name) override
	{
		auto res = process.modules().GetExport(modulePtr, name.c_str());
		if (res) {
			exp.procPtr = res->procAddress;
			return true;
		}
		return false;
	}

	std::string getName() const override
	{
		return ConvertWStringToString(modulePtr->name);
	}

private:
	::blackbone::Process& process;
	::blackbone::ModuleDataPtr modulePtr;
};


}
}
}
