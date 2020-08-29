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

#include "../config.h"

#include <stdint.h>
#include "RMonoHandle_FwdDef.h"




namespace remotemono
{


// These are the public types used by RemoteMono. They are used in the frontend classes like RMonoAPI and are independent
// of the actual ABI that is used in the backend. In contrast, the internal types (defined in the RMonoABITypeTraits
// ABI component) are ABI-specific and are used in the backend classes like RMonoAPIBackend or RMonoAPIFunction. The
// internal types can be distinguished by an addition 'i' at the start of their names.
// Because these public types are supposed to work with all supported ABIs, they need to be large enough to hold the largest
// corresponding internal type of any ABI. Conversion between these public types and the internal types is done by the
// RMonoABIConverter ABI component. Most of the types that RemoteMono uses are either these public or the internal types,
// because we need to mirror the types of the remote process, not of the local process. For example, if the local process is
// 64-bit and the remote is 32-bit, then `sizeof(irmono_voidp) == 4 != sizeof(void*)`, but `sizeof(rmono_voidp) == 8`.
// The only types that don't need to be wrapped with one of these typedefs are explicitly-sized types like uint32_t and
// types that don't actually appear in the raw Mono API but are specially supported by RemoteMono, like std::string,
// RMonoHandle or RMonoVariant.


// ********** Fundamental Types **********

typedef	uint64_t			rmono_uintptr_t;					// uintptr_t
typedef	int64_t				rmono_intptr_t;						// intptr_t
typedef int32_t				rmono_int;							// int
typedef uint32_t			rmono_uint;							// unsigned int
typedef int32_t				rmono_bool;							// mono_bool
typedef uint8_t				rmono_byte;							// mono_byte
typedef uint16_t			rmono_unichar2;						// mono_unichar2
typedef uint16_t			rmono_unichar4;						// mono_unichar4
typedef uint32_t			rmono_gchandle;						// (Type for Mono's GC handles, no special remote typedef exists)


// ********** Pointers to Fundamental Types **********

typedef rmono_uintptr_t		rmono_voidp;						// void*
typedef rmono_voidp			rmono_charp;						// char*
typedef rmono_voidp			rmono_ccharp;						// const char*
typedef rmono_voidp			rmono_bytep;						// mono_byte*
typedef rmono_voidp			rmono_cbytep;						// const mono_byte*
typedef rmono_voidp			rmono_unichar2p;					// mono_unichar2*
typedef rmono_voidp			rmono_cunichar2p;					// const mono_unichar2*
typedef rmono_voidp			rmono_unichar4p;					// mono_unichar4*
typedef rmono_voidp			rmono_cunichar4p;					// const mono_unichar4*


// ********** Double-Pointers to Fundamental Types **********

typedef rmono_uintptr_t		rmono_voidpp;						// void**
typedef rmono_voidp			rmono_charpp;						// char**
typedef rmono_voidp			rmono_ccharpp;						// const char**
typedef rmono_voidp			rmono_bytepp;						// mono_byte**
typedef rmono_voidp			rmono_cbytepp;						// const mono_byte**
typedef rmono_voidp			rmono_unichar2pp;					// mono_unichar2**
typedef rmono_voidp			rmono_cunichar2pp;					// const mono_unichar2**
typedef rmono_voidp			rmono_unichar4pp;					// mono_unichar4**
typedef rmono_voidp			rmono_cunichar4pp;					// const mono_unichar4**


// ********** Miscellaneous Fundamental Types **********

typedef rmono_uintptr_t		rmono_funcp;						// (Generic function pointer)


// ********** Raw Pointers to Handle Types **********

typedef rmono_voidp			RMonoDomainPtrRaw;					// MonoDomain*
typedef rmono_voidp			RMonoAssemblyPtrRaw;				// MonoAssembly*
typedef rmono_voidp			RMonoAssemblyNamePtrRaw;			// MonoAssemblyName*
typedef rmono_voidp			RMonoImagePtrRaw;					// MonoImage*
typedef rmono_voidp			RMonoClassPtrRaw;					// MonoClass*
typedef rmono_voidp			RMonoTypePtrRaw;					// MonoType*
typedef rmono_voidp			RMonoTableInfoPtrRaw;				// MonoTableInfo*
typedef rmono_voidp			RMonoClassFieldPtrRaw;				// MonoClassField*
typedef rmono_voidp			RMonoVTablePtrRaw;					// MonoVTable*
typedef rmono_voidp			RMonoMethodPtrRaw;					// MonoMethod*
typedef rmono_voidp			RMonoPropertyPtrRaw;				// MonoProperty*
typedef rmono_voidp			RMonoMethodSignaturePtrRaw;			// MonoMethodSignature*
typedef rmono_voidp			RMonoMethodHeaderPtrRaw;			// MonoMethodHeader*
typedef rmono_voidp			RMonoMethodDescPtrRaw;				// MonoMethodDescPtr*
typedef rmono_voidp			RMonoJitInfoPtrRaw;					// MonoJitInfo*
typedef rmono_voidp			RMonoDisHelperPtrRaw;				// MonoDisHelperPtr*
typedef rmono_voidp			RMonoObjectPtrRaw;					// MonoObjectPtr*
typedef rmono_voidp			RMonoThreadPtrRaw;					// MonoThread*
typedef rmono_voidp			RMonoStringPtrRaw;					// MonoString*
typedef rmono_voidp			RMonoArrayPtrRaw;					// MonoArray*
typedef rmono_voidp			RMonoExceptionPtrRaw;				// MonoException*
typedef rmono_voidp			RMonoReflectionTypePtrRaw;			// MonoReflectionType*


// ********** Raw Double-Pointers to Handle Types **********

typedef rmono_voidp			RMonoDomainPtrPtrRaw;				// MonoDomain**
typedef rmono_voidp			RMonoAssemblyPtrPtrRaw;				// MonoAssembly**
typedef rmono_voidp			RMonoAssemblyNamePtrPtrRaw;			// MonoAssemblyName**
typedef rmono_voidp			RMonoImagePtrPtrRaw;				// MonoImage**
typedef rmono_voidp			RMonoClassPtrPtrRaw;				// MonoClass**
typedef rmono_voidp			RMonoTypePtrPtrRaw;					// MonoType**
typedef rmono_voidp			RMonoTableInfoPtrPtrRaw;			// MonoTableInfo**
typedef rmono_voidp			RMonoClassFieldPtrPtrRaw;			// MonoClassField**
typedef rmono_voidp			RMonoVTablePtrPtrRaw;				// MonoVTable**
typedef rmono_voidp			RMonoMethodPtrPtrRaw;				// MonoMethod**
typedef rmono_voidp			RMonoPropertyPtrPtrRaw;				// MonoProperty**
typedef rmono_voidp			RMonoMethodSignaturePtrPtrRaw;		// MonoMethodSignature**
typedef rmono_voidp			RMonoMethodHeaderPtrPtrRaw;			// MonoMethodHeader**
typedef rmono_voidp			RMonoMethodDescPtrPtrRaw;			// MonoMethodDescPtr**
typedef rmono_voidp			RMonoJitInfoPtrPtrRaw;				// MonoJitInfo**
typedef rmono_voidp			RMonoDisHelperPtrPtrRaw;			// MonoDisHelperPtr**
typedef rmono_voidp			RMonoObjectPtrPtrRaw;				// MonoObjectPtr**
typedef rmono_voidp			RMonoThreadPtrPtrRaw;				// MonoThread**
typedef rmono_voidp			RMonoStringPtrPtrRaw;				// MonoString**
typedef rmono_voidp			RMonoArrayPtrPtrRaw;				// MonoArray**
typedef rmono_voidp			RMonoExceptionPtrPtrRaw;			// MonoException**
typedef rmono_voidp			RMonoReflectionTypePtrPtrRaw;		// MonoReflectionType**


// ********** Simple Remote Handles **********

typedef RMonoHandle<RMonoDomainPtrRaw>														RMonoDomainPtr;
typedef RMonoHandle<RMonoAssemblyPtrRaw>													RMonoAssemblyPtr;
typedef RMonoHandle<RMonoAssemblyNamePtrRaw, &RMonoHandleAssemblyNamePtrDelete>				RMonoAssemblyNamePtr;
typedef RMonoHandle<RMonoImagePtrRaw>														RMonoImagePtr;
typedef RMonoHandle<RMonoClassPtrRaw>														RMonoClassPtr;
typedef RMonoHandle<RMonoTypePtrRaw>														RMonoTypePtr;
typedef RMonoHandle<RMonoTableInfoPtrRaw>													RMonoTableInfoPtr;
typedef RMonoHandle<RMonoClassFieldPtrRaw>													RMonoClassFieldPtr;
typedef RMonoHandle<RMonoVTablePtrRaw>														RMonoVTablePtr;
typedef RMonoHandle<RMonoMethodPtrRaw>														RMonoMethodPtr;
typedef RMonoHandle<RMonoPropertyPtrRaw>													RMonoPropertyPtr;
typedef RMonoHandle<RMonoMethodSignaturePtrRaw>												RMonoMethodSignaturePtr;
typedef RMonoHandle<RMonoMethodHeaderPtrRaw>												RMonoMethodHeaderPtr;
typedef RMonoHandle<RMonoMethodDescPtrRaw, &RMonoHandleMethodDescPtrDelete>					RMonoMethodDescPtr;
typedef RMonoHandle<RMonoJitInfoPtrRaw>														RMonoJitInfoPtr;
typedef RMonoHandle<RMonoDisHelperPtrRaw>													RMonoDisHelperPtr;


// ********** Remote Handles Based on MonoObject **********

typedef RMonoObjectHandle<RMonoObjectPtrRaw>					RMonoObjectPtr;
typedef RMonoObjectHandle<RMonoThreadPtrRaw>					RMonoThreadPtr;
typedef RMonoObjectHandle<RMonoStringPtrRaw>					RMonoStringPtr;
typedef RMonoObjectHandle<RMonoArrayPtrRaw>						RMonoArrayPtr;
typedef RMonoObjectHandle<RMonoExceptionPtrRaw>					RMonoExceptionPtr;
typedef RMonoObjectHandle<RMonoReflectionTypePtrRaw>			RMonoReflectionTypePtr;






#define REMOTEMONO_GCHANDLE_INVALID 0


}
