#pragma once

#include "../../config.h"

#include <memory>
#include "../RMonoTypes.h"
#include "RMonoProcess.h"


namespace remotemono
{
namespace backend
{


class RMonoMemBlock
{
public:
	static RMonoMemBlock alloc(RMonoProcess* process, size_t size, int prot = PAGE_EXECUTE_READWRITE, bool owned = true)
	{
		return RMonoMemBlock(process, process->allocRawMemory(size, prot), size, owned);
	}

public:
	RMonoMemBlock() : process(nullptr), ptr(0), msize(0), owned(false) {}

	RMonoMemBlock(RMonoMemBlock&& o)
			: process(o.process), ptr(o.ptr), msize(o.msize), owned(o.owned)
	{
		o.process = nullptr;
		o.ptr = 0;
		o.msize = 0;
		o.owned = false;
	}

	RMonoMemBlock(RMonoProcess* process, rmono_voidp ptr, size_t size, bool owned = true)
			: process(process), ptr(ptr), msize(size), owned(owned)
	{}

	RMonoMemBlock(RMonoProcess* process, rmono_voidp ptr, bool owned = true)
			: process(process), ptr(ptr), msize(process->getMemoryRegionSize(ptr)), owned(owned)
	{}

	~RMonoMemBlock()
	{
		free();
	}

	RMonoMemBlock& operator=(RMonoMemBlock&& o)
	{
		if (this == &o) {
			return *this;
		}
		process = o.process;
		ptr = o.ptr;
		msize = o.msize;
		owned = o.owned;
		o.process = nullptr;
		o.ptr = 0;
		o.msize = 0;
		o.owned = false;
		return *this;
	}

	void free()
	{
		if (owned) {
			process->freeRawMemory(ptr);
			owned = false;
		}
		ptr = 0;
		msize = 0;
	}

	void reset()
	{
		free();
		process = nullptr;
	}

	rmono_voidp getPointer() const { return ptr; }
	rmono_voidp operator*() const { return getPointer(); }

	size_t getSize() const { return msize; }
	size_t size() const { return getSize(); }

	void read(uintptr_t offs, size_t size, void* data)
	{
		process->readMemory(ptr + offs, size, data);
	}

	void write(uintptr_t offs, size_t size, const void* data)
	{
		process->writeMemory(ptr + offs, size, data);
	}

private:
	RMonoProcess* process;
	rmono_voidp ptr;
	size_t msize;
	bool owned;
};


}
}
