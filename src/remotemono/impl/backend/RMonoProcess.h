#pragma once

#include "../../config.h"

#include <memory>
#include <vector>
#include "RMonoBackend.h"
#include "RMonoModule.h"
#include "RMonoAsmHelper.h"


namespace remotemono
{
namespace backend
{


class RMonoProcess
{
public:


public:
	RMonoBackend* getBackend() { return backend; }

	virtual void attach() = 0;

	virtual RMonoModule* getModule(const std::string& name) = 0;
	virtual std::vector<RMonoModule*> getAllModules() = 0;

	virtual rmono_voidp allocRawMemory(size_t size, int prot = PAGE_EXECUTE_READWRITE) = 0;
	virtual void freeRawMemory(rmono_voidp ptr) = 0;

	virtual void readMemory(rmono_voidp remPtr, size_t size, void* data) = 0;
	virtual void writeMemory(rmono_voidp remPtr, size_t size, const void* data) = 0;

	virtual RMonoProcessorArch getProcessorArchitecture() = 0;

	virtual size_t getMemoryRegionSize(rmono_voidp remPtr) = 0;

	virtual size_t getPageSize()
	{
		SYSTEM_INFO info = {{0}};
		GetNativeSystemInfo(&info);
		return static_cast<size_t>(info.dwPageSize);
	}

	virtual std::unique_ptr<RMonoAsmHelper> createAssembler() = 0;

protected:
	RMonoProcess(backend::RMonoBackend* backend) : backend(backend) {}

private:
	RMonoProcess() {}

private:
	backend::RMonoBackend* backend;
};


}
}
