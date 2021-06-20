#pragma once

#include "../../../config.h"

#include <string>
#include <unordered_map>
#include <vector>
#include "../../exception/RMonoException_Def.h"
#include "../../../util.h"
#include "RMonoBlackBoneBackend.h"
#include "RMonoBlackBoneModule.h"
#include "RMonoBlackBoneAsmHelper.h"
#include <BlackBone/Process/Process.h>
#include <BlackBone/Asm/AsmFactory.h>

#ifndef REMOTEMONO_BACKEND_BLACKBONE_ENABLED
#error "Included BlackBone backend header, but BlackBone backend is disabled."
#endif


namespace remotemono
{
namespace backend
{
namespace blackbone
{


class RMonoBlackBoneProcess : public RMonoProcess
{
private:
	typedef ::blackbone::ptr_t ptr_t;

public:
	RMonoBlackBoneProcess(::blackbone::Process* process, bool ownProcess = false)
			: RMonoProcess(&RMonoBlackBoneBackend::getInstance()), process(process), ownProcess(ownProcess)
	{}
	~RMonoBlackBoneProcess()
	{
		for (auto it = modules.begin() ; it != modules.end() ; it++) {
			delete it->second;
		}
		if (ownProcess) {
			delete process;
		}
	}

	::blackbone::Process& operator*() { return *process; }
	const ::blackbone::Process& operator*() const { return *process; }

	void attach() override
	{
		::blackbone::RemoteExec& rem = process->remote();
		rem.CreateRPCEnvironment(::blackbone::Worker_CreateNew, true);
	}

	RMonoModule* getModule(const std::string& name) override
	{
		auto it = modules.find(name);
		if (it != modules.end()) {
			return it->second;
		}
		auto modulePtr = process->modules().GetModule(ConvertStringToWString(name));
		if (!modulePtr) {
			return nullptr;
		}
		RMonoBlackBoneModule* mod = new RMonoBlackBoneModule(*process, modulePtr);
		modules[name] = mod;
		return mod;
	}

	std::vector<RMonoModule*> getAllModules() override
	{
		std::vector<RMonoModule*> outMods;
		auto bbModules = process->modules().GetAllModules();

		for (auto it = bbModules.begin() ; it != bbModules.end() ; it++) {
			std::string name = ConvertWStringToString(it->first.first);

			RMonoModule* mod = getModule(name);
			if (mod) {
				outMods.push_back(mod);
			}
		}

		return outMods;
	}

	rmono_voidp allocRawMemory(size_t size, int prot = PAGE_EXECUTE_READWRITE) override
	{
		auto res = ::blackbone::MemBlock::Allocate(process->memory(), size, 0, static_cast<DWORD>(prot), false);
		if (!res) {
			throw RMonoException("Error allocating remote memory");
		}
		return static_cast<rmono_voidp>(res->ptr());
	}

	void freeRawMemory(rmono_voidp ptr) override
	{
		::blackbone::MemBlock mem(&process->memory(), static_cast<ptr_t>(ptr));
	}

	void readMemory(rmono_voidp remPtr, size_t size, void* data) override
	{
		process->memory().Read(static_cast<ptr_t>(remPtr), size, data);
	}

	void writeMemory(rmono_voidp remPtr, size_t size, const void* data) override
	{
		process->memory().Write(static_cast<ptr_t>(remPtr), size, data);
	}

	RMonoProcessorArch getProcessorArchitecture() override
	{
		SYSTEM_INFO sysinfo;
		GetNativeSystemInfo(&sysinfo);

		if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
			return ProcessorArchX86;
		} else {
			return process->core().isWow64() ? ProcessorArchX86 : ProcessorArchX86_64;
		}
	}

	size_t getMemoryRegionSize(rmono_voidp remPtr) override
	{
		MEMORY_BASIC_INFORMATION64 mbi = {0};
		process->memory().Query(remPtr, &mbi);
		return static_cast<size_t>(mbi.RegionSize);
	}

	std::unique_ptr<RMonoAsmHelper> createAssembler() override
	{
		::blackbone::AsmHelperPtr bbHelper;
		RMonoProcessorArch arch = getProcessorArchitecture();
		if (arch == ProcessorArchX86) {
			bbHelper = ::blackbone::AsmFactory::GetAssembler(::blackbone::AsmFactory::asm32);
		} else if (arch == ProcessorArchX86_64) {
			bbHelper = ::blackbone::AsmFactory::GetAssembler(::blackbone::AsmFactory::asm64);
		} else {
			assert(false);
		}
		return std::make_unique<RMonoBlackBoneAsmHelper>(std::move(bbHelper));
	}

private:
	::blackbone::Process* process;
	bool ownProcess;
	std::unordered_map<std::string, RMonoModule*> modules;
};


}
}
}
