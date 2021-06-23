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

#include "impl/RMonoTypes.h"
#include "impl/backend/RMonoAsmHelper.h"



namespace remotemono
{



/**
 * Generates a call to `mono_gchandle_get_target(gchandle)`, but instead returns a NULL pointer if the given GC handle
 * is invalid.
 *
 * Note that this always expects the GC handle in ZCX and returns the result in ZAX. You do not need to reserve
 * any shadow stack space before calling this function.
 *
 * @param a The RMonoAsmHelper used to generate the code.
 * @param rawAddr The address to the raw mono_gchandle_get_target() function.
 * @param x64 true to generate x64 code, false for x86
 */
inline void AsmGenGchandleGetTargetChecked(backend::RMonoAsmHelper& a, rmono_funcp rawAddr, bool x64)
{
	// NOTE: Always expects an irmono_gchandle in zcx.

	using namespace asmjit;
	using namespace asmjit::host;

	auto lSkip = a->newLabel();

	static_assert(REMOTEMONO_GCHANDLE_INVALID == 0);

	//	zax = nullptr;
		a->xor_(a->zax, a->zax);

	//	if (zcx != REMOTEMONO_GCHANDLE_INVALID) {
		a->jecxz(a->zcx, lSkip);

	//		zax = mono_gchandle_get_target(zcx);
			a->mov(a->zax, rawAddr);
			if (x64) {
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zcx);
				a->call(a->zax);
				a->pop(a->zcx);
			}

	//	}
		a->bind(lSkip);
}


/**
 * Generates a call to `mono_gchandle_new(gchandle, false)`, but instead returns an invalid GC handle if the
 * input pointer is NULL.
 *
 * Note that this always expects the raw pointer in ZCX and returns the result in ZAX. You do not need to reserve
 * any shadow stack space before calling this function.
 *
 * Also note that this always creates non-pinned GC handles, and this (pseudo-)function takes only a single
 * parameter: the raw pointer itself.
 *
 * @param a The RMonoAsmHelper used to generate the code.
 * @param rawAddr The address to the raw mono_gchandle_new() function.
 * @param x64 true to generate x64 code, false for x86
 */
inline void AsmGenGchandleNewChecked(backend::RMonoAsmHelper& a, rmono_funcp rawAddr, bool x64)
{
	// NOTE: Always expects an IRMonoObjectPtrRaw in zcx.

	using namespace asmjit;
	using namespace asmjit::host;

	auto lSkip = a->newLabel();

	static_assert(REMOTEMONO_GCHANDLE_INVALID == 0);

	//	zax = REMOTEMONO_GCHANDLE_INVALID;
		a->xor_(a->zax, a->zax);

	//	if (zcx != nullptr) {
		a->jecxz(a->zcx, lSkip);

	//		zax = mono_gchandle_new(zcx, false);
			a->mov(a->zax, rawAddr);
			if (x64) {
				a->xor_(a->zdx, a->zdx); // Don't pin the GCHandle
				a->sub(a->zsp, 32);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(0); // Don't pin the GCHandle
				a->push(a->zcx);
				a->call(a->zax);
				a->add(a->zsp, 2*sizeof(uint32_t));
			}

	//	}
		a->bind(lSkip);
}


inline void AsmGenIsValueTypeInstance (
		backend::RMonoAsmHelper& a,
		rmono_funcp objectGetClassAddr,
		rmono_funcp classIsValuetypeAddr,
		bool x64
) {
	// bool is_value_type_instance(IRMonoObjectPtrRaw obj)
	//
	//		obj: zcx

	using namespace asmjit;
	using namespace asmjit::host;

	auto lSkip = a->newLabel();

	//	zax = false;
	a->xor_(a->zax, a->zax);

	//	if (obj != nullptr) {
		a->jecxz(a->zcx, lSkip);

	//		zax = mono_class_is_valuetype(mono_object_get_class(obj));
			if (x64) {
				a->sub(a->zsp, 32);
				a->mov(a->zax, objectGetClassAddr);
				a->call(a->zax);
				a->mov(a->zcx, a->zax);
				a->mov(a->zax, classIsValuetypeAddr);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zcx);
				a->mov(a->zax, objectGetClassAddr);
				a->call(a->zax);
				a->mov(ptr(a->zsp), a->zax);
				a->mov(a->zax, classIsValuetypeAddr);
				a->call(a->zax);
				a->add(a->zsp, sizeof(uint32_t));
			}

	//	}
		a->bind(lSkip);
}


inline void AsmGenObjectUnbox (
		backend::RMonoAsmHelper& a,
		rmono_funcp objectUnboxAddr,
		bool x64
) {
	// void* object_unbox(IRMonoObjectPtrRaw obj)
	//
	//		obj: zcx

	using namespace asmjit;
	using namespace asmjit::host;


	//	zax = mono_object_unbox(obj);
		if (x64) {
			a->mov(a->zax, objectUnboxAddr);
			a->sub(a->zsp, 32);
			a->call(a->zax);
			a->add(a->zsp, 32);
		} else {
			a->push(a->zcx);
			a->mov(a->zax, objectUnboxAddr);
			a->call(a->zax);
			a->add(a->zsp, sizeof(uint32_t));
		}
}


inline void AsmGenObjectGetClass (
		backend::RMonoAsmHelper& a,
		rmono_funcp objectGetClassAddr,
		bool x64
) {
	// IRMonoClassPtr object_get_class(IRMonoObjectPtrRaw obj)
	//
	//		obj: zcx

	using namespace asmjit;
	using namespace asmjit::host;

	auto lSkip = a->newLabel();

	//	zax = nullptr;
	a->xor_(a->zax, a->zax);

	//	if (obj != nullptr) {
		a->jecxz(a->zcx, lSkip);

	//		zax = mono_object_get_class(obj);
			if (x64) {
				a->sub(a->zsp, 32);
				a->mov(a->zax, objectGetClassAddr);
				a->call(a->zax);
				a->add(a->zsp, 32);
			} else {
				a->push(a->zcx);
				a->mov(a->zax, objectGetClassAddr);
				a->call(a->zax);
				a->add(a->zsp, sizeof(uint32_t));
			}

	//	}
		a->bind(lSkip);
}



}
