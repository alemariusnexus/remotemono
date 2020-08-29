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

#include "../../config.h"

#include "../RMonoTypes.h"
#include "../RMonoHandle_FwdDef.h"
#include "RMonoABI_FwdDef.h"




#define REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(TraitsT)																							\
		typedef typename TraitsT::irmono_uintptr_t								irmono_uintptr_t;											\
		typedef typename TraitsT::irmono_intptr_t								irmono_intptr_t;											\
		typedef typename TraitsT::irmono_int									irmono_int;													\
		typedef typename TraitsT::irmono_uint									irmono_uint;												\
		typedef typename TraitsT::irmono_bool									irmono_bool;												\
		typedef typename TraitsT::irmono_byte									irmono_byte;												\
		typedef typename TraitsT::irmono_unichar2								irmono_unichar2;											\
		typedef typename TraitsT::irmono_unichar4								irmono_unichar4;											\
		typedef typename TraitsT::irmono_gchandle								irmono_gchandle;											\
																																			\
		typedef typename TraitsT::irmono_voidp									irmono_voidp;												\
		typedef typename TraitsT::irmono_charp									irmono_charp;												\
		typedef typename TraitsT::irmono_ccharp									irmono_ccharp;												\
		typedef typename TraitsT::irmono_bytep									irmono_bytep;												\
		typedef typename TraitsT::irmono_cbytep									irmono_cbytep;												\
		typedef typename TraitsT::irmono_unichar2p								irmono_unichar2p;											\
		typedef typename TraitsT::irmono_cunichar2p								irmono_cunichar2p;											\
		typedef typename TraitsT::irmono_unichar4p								irmono_unichar4p;											\
		typedef typename TraitsT::irmono_cunichar4p								irmono_cunichar4p;											\
																																			\
		typedef typename TraitsT::irmono_voidpp									irmono_voidpp;												\
		typedef typename TraitsT::irmono_charpp									irmono_charpp;												\
		typedef typename TraitsT::irmono_ccharpp								irmono_ccharpp;												\
		typedef typename TraitsT::irmono_bytepp									irmono_bytepp;												\
		typedef typename TraitsT::irmono_cbytepp								irmono_cbytepp;												\
		typedef typename TraitsT::irmono_unichar2pp								irmono_unichar2pp;											\
		typedef typename TraitsT::irmono_cunichar2pp							irmono_cunichar2pp;											\
		typedef typename TraitsT::irmono_unichar4pp								irmono_unichar4pp;											\
		typedef typename TraitsT::irmono_cunichar4pp							irmono_cunichar4pp;											\
																																			\
		typedef typename TraitsT::irmono_funcp									irmono_funcp;												\
																																			\
		typedef typename TraitsT::IRMonoDomainPtrRaw							IRMonoDomainPtrRaw;											\
		typedef typename TraitsT::IRMonoAssemblyPtrRaw							IRMonoAssemblyPtrRaw;										\
		typedef typename TraitsT::IRMonoAssemblyNamePtrRaw						IRMonoAssemblyNamePtrRaw;									\
		typedef typename TraitsT::IRMonoImagePtrRaw								IRMonoImagePtrRaw;											\
		typedef typename TraitsT::IRMonoClassPtrRaw								IRMonoClassPtrRaw;											\
		typedef typename TraitsT::IRMonoTypePtrRaw								IRMonoTypePtrRaw;											\
		typedef typename TraitsT::IRMonoTableInfoPtrRaw							IRMonoTableInfoPtrRaw;										\
		typedef typename TraitsT::IRMonoClassFieldPtrRaw						IRMonoClassFieldPtrRaw;										\
		typedef typename TraitsT::IRMonoVTablePtrRaw							IRMonoVTablePtrRaw;											\
		typedef typename TraitsT::IRMonoMethodPtrRaw							IRMonoMethodPtrRaw;											\
		typedef typename TraitsT::IRMonoPropertyPtrRaw							IRMonoPropertyPtrRaw;										\
		typedef typename TraitsT::IRMonoMethodSignaturePtrRaw					IRMonoMethodSignaturePtrRaw;								\
		typedef typename TraitsT::IRMonoMethodHeaderPtrRaw						IRMonoMethodHeaderPtrRaw;									\
		typedef typename TraitsT::IRMonoMethodDescPtrRaw						IRMonoMethodDescPtrRaw;										\
		typedef typename TraitsT::IRMonoJitInfoPtrRaw							IRMonoJitInfoPtrRaw;										\
		typedef typename TraitsT::IRMonoDisHelperPtrRaw							IRMonoDisHelperPtrRaw;										\
		typedef typename TraitsT::IRMonoObjectPtrRaw							IRMonoObjectPtrRaw;											\
		typedef typename TraitsT::IRMonoThreadPtrRaw							IRMonoThreadPtrRaw;											\
		typedef typename TraitsT::IRMonoStringPtrRaw							IRMonoStringPtrRaw;											\
		typedef typename TraitsT::IRMonoArrayPtrRaw								IRMonoArrayPtrRaw;											\
		typedef typename TraitsT::IRMonoExceptionPtrRaw							IRMonoExceptionPtrRaw;										\
		typedef typename TraitsT::IRMonoReflectionTypePtrRaw					IRMonoReflectionTypePtrRaw;									\
																																			\
		typedef typename TraitsT::IRMonoDomainPtrPtrRaw							IRMonoDomainPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoAssemblyPtrPtrRaw						IRMonoAssemblyPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoAssemblyNamePtrPtrRaw					IRMonoAssemblyNamePtrPtrRaw;								\
		typedef typename TraitsT::IRMonoImagePtrPtrRaw							IRMonoImagePtrPtrRaw;										\
		typedef typename TraitsT::IRMonoClassPtrPtrRaw							IRMonoClassPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoTypePtrPtrRaw							IRMonoTypePtrPtrRaw;										\
		typedef typename TraitsT::IRMonoTableInfoPtrPtrRaw						IRMonoTableInfoPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoClassFieldPtrPtrRaw						IRMonoClassFieldPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoVTablePtrPtrRaw							IRMonoVTablePtrPtrRaw;										\
		typedef typename TraitsT::IRMonoMethodPtrPtrRaw							IRMonoMethodPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoPropertyPtrPtrRaw						IRMonoPropertyPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoMethodSignaturePtrPtrRaw				IRMonoMethodSignaturePtrPtrRaw;								\
		typedef typename TraitsT::IRMonoMethodHeaderPtrPtrRaw					IRMonoMethodHeaderPtrPtrRaw;								\
		typedef typename TraitsT::IRMonoMethodDescPtrPtrRaw						IRMonoMethodDescPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoJitInfoPtrPtrRaw						IRMonoJitInfoPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoDisHelperPtrPtrRaw						IRMonoDisHelperPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoObjectPtrPtrRaw							IRMonoObjectPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoThreadPtrPtrRaw							IRMonoThreadPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoStringPtrPtrRaw							IRMonoStringPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoArrayPtrPtrRaw							IRMonoArrayPtrPtrRaw;										\
		typedef typename TraitsT::IRMonoExceptionPtrPtrRaw						IRMonoExceptionPtrPtrRaw;									\
		typedef typename TraitsT::IRMonoReflectionTypePtrPtrRaw					IRMonoReflectionTypePtrPtrRaw;								\
																																			\
		typedef typename TraitsT::IRMonoDomainPtr								IRMonoDomainPtr;											\
		typedef typename TraitsT::IRMonoAssemblyPtr								IRMonoAssemblyPtr;											\
		typedef typename TraitsT::IRMonoAssemblyNamePtr							IRMonoAssemblyNamePtr;										\
		typedef typename TraitsT::IRMonoImagePtr								IRMonoImagePtr;												\
		typedef typename TraitsT::IRMonoClassPtr								IRMonoClassPtr;												\
		typedef typename TraitsT::IRMonoTypePtr									IRMonoTypePtr;												\
		typedef typename TraitsT::IRMonoTableInfoPtr							IRMonoTableInfoPtr;											\
		typedef typename TraitsT::IRMonoClassFieldPtr							IRMonoClassFieldPtr;										\
		typedef typename TraitsT::IRMonoVTablePtr								IRMonoVTablePtr;											\
		typedef typename TraitsT::IRMonoMethodPtr								IRMonoMethodPtr;											\
		typedef typename TraitsT::IRMonoPropertyPtr								IRMonoPropertyPtr;											\
		typedef typename TraitsT::IRMonoMethodSignaturePtr						IRMonoMethodSignaturePtr;									\
		typedef typename TraitsT::IRMonoMethodHeaderPtr							IRMonoMethodHeaderPtr;										\
		typedef typename TraitsT::IRMonoMethodDescPtr							IRMonoMethodDescPtr;										\
		typedef typename TraitsT::IRMonoJitInfoPtr								IRMonoJitInfoPtr;											\
		typedef typename TraitsT::IRMonoDisHelperPtr							IRMonoDisHelperPtr;											\
																																			\
		typedef typename TraitsT::IRMonoObjectPtr								IRMonoObjectPtr;											\
		typedef typename TraitsT::IRMonoThreadPtr								IRMonoThreadPtr;											\
		typedef typename TraitsT::IRMonoStringPtr								IRMonoStringPtr;											\
		typedef typename TraitsT::IRMonoArrayPtr								IRMonoArrayPtr;												\
		typedef typename TraitsT::IRMonoExceptionPtr							IRMonoExceptionPtr;											\
		typedef typename TraitsT::IRMonoReflectionTypePtr						IRMonoReflectionTypePtr;





namespace remotemono
{




// **********************************************************************
// *																	*
// *							TYPE TRAITS								*
// *																	*
// **********************************************************************


/**
 * The ABITypeTraits class is an ABI component that defines the internal types corresponding to the public types defined in
 * RMonoTypes.h. These internal types are specific to an ABI, e.g. irmono_voidp is 4 bytes for 32-bit ABIs and 8 bytes for 64-bit
 * ABIs. Public and internal types are converted into each other by the RMonoABIConverter component.
 */
template <class ABI>
class RMonoABITypeTraits;

template <typename UIntPtrT, typename IntPtrT>
class RMonoABITypeTraitsWinCommon
{
public:
	// ********** Fundamental Types **********

	typedef	UIntPtrT			irmono_uintptr_t;						// uintptr_t
	typedef	IntPtrT				irmono_intptr_t;						// intptr_t
	typedef int32_t				irmono_int;								// int
	typedef uint32_t			irmono_uint;							// unsigned int
	typedef int32_t				irmono_bool;							// mono_bool
	typedef uint8_t				irmono_byte;							// mono_byte
	typedef uint16_t			irmono_unichar2;						// mono_unichar2
	typedef uint16_t			irmono_unichar4;						// mono_unichar4
	typedef uint32_t			irmono_gchandle;						// (Type for Mono's GC handles, no special remote typedef exists)


	// ********** Pointers to Fundamental Types **********

	typedef irmono_uintptr_t		irmono_voidp;						// void*
	typedef irmono_voidp			irmono_charp;						// char*
	typedef irmono_voidp			irmono_ccharp;						// const char*
	typedef irmono_voidp			irmono_bytep;						// mono_byte*
	typedef irmono_voidp			irmono_cbytep;						// const mono_byte*
	typedef irmono_voidp			irmono_unichar2p;					// mono_unichar2*
	typedef irmono_voidp			irmono_cunichar2p;					// const mono_unichar2*
	typedef irmono_voidp			irmono_unichar4p;					// mono_unichar4*
	typedef irmono_voidp			irmono_cunichar4p;					// const mono_unichar4*


	// ********** Double-Pointers to Fundamental Types **********

	typedef irmono_uintptr_t		irmono_voidpp;						// void**
	typedef irmono_voidp			irmono_charpp;						// char**
	typedef irmono_voidp			irmono_ccharpp;						// const char**
	typedef irmono_voidp			irmono_bytepp;						// mono_byte**
	typedef irmono_voidp			irmono_cbytepp;						// const mono_byte**
	typedef irmono_voidp			irmono_unichar2pp;					// mono_unichar2**
	typedef irmono_voidp			irmono_cunichar2pp;					// const mono_unichar2**
	typedef irmono_voidp			irmono_unichar4pp;					// mono_unichar4**
	typedef irmono_voidp			irmono_cunichar4pp;					// const mono_unichar4**


	// ********** Miscellaneous Fundamental Types **********

	typedef irmono_uintptr_t		irmono_funcp;						// (Generic function pointer)


	// ********** Raw Pointers to Handle Types **********

	typedef irmono_voidp			IRMonoDomainPtrRaw;					// MonoDomain*
	typedef irmono_voidp			IRMonoAssemblyPtrRaw;				// MonoAssembly*
	typedef irmono_voidp			IRMonoAssemblyNamePtrRaw;			// MonoAssemblyName*
	typedef irmono_voidp			IRMonoImagePtrRaw;					// MonoImage*
	typedef irmono_voidp			IRMonoClassPtrRaw;					// MonoClass*
	typedef irmono_voidp			IRMonoTypePtrRaw;					// MonoType*
	typedef irmono_voidp			IRMonoTableInfoPtrRaw;				// MonoTableInfo*
	typedef irmono_voidp			IRMonoClassFieldPtrRaw;				// MonoClassField*
	typedef irmono_voidp			IRMonoVTablePtrRaw;					// MonoVTable*
	typedef irmono_voidp			IRMonoMethodPtrRaw;					// MonoMethod*
	typedef irmono_voidp			IRMonoPropertyPtrRaw;				// MonoProperty*
	typedef irmono_voidp			IRMonoMethodSignaturePtrRaw;		// MonoMethodSignature*
	typedef irmono_voidp			IRMonoMethodHeaderPtrRaw;			// MonoMethodHeader*
	typedef irmono_voidp			IRMonoMethodDescPtrRaw;				// MonoMethodDescPtr*
	typedef irmono_voidp			IRMonoJitInfoPtrRaw;				// MonoJitInfo*
	typedef irmono_voidp			IRMonoDisHelperPtrRaw;				// MonoDisHelperPtr*
	typedef irmono_voidp			IRMonoObjectPtrRaw;					// MonoObjectPtr*
	typedef irmono_voidp			IRMonoThreadPtrRaw;					// MonoThread*
	typedef irmono_voidp			IRMonoStringPtrRaw;					// MonoString*
	typedef irmono_voidp			IRMonoArrayPtrRaw;					// MonoArray*
	typedef irmono_voidp			IRMonoExceptionPtrRaw;				// MonoException*
	typedef irmono_voidp			IRMonoReflectionTypePtrRaw;			// MonoReflectionType*


	// ********** Raw Double-Pointers to Handle Types **********

	typedef irmono_voidp			IRMonoDomainPtrPtrRaw;				// MonoDomain**
	typedef irmono_voidp			IRMonoAssemblyPtrPtrRaw;			// MonoAssembly**
	typedef irmono_voidp			IRMonoAssemblyNamePtrPtrRaw;		// MonoAssemblyName**
	typedef irmono_voidp			IRMonoImagePtrPtrRaw;				// MonoImage**
	typedef irmono_voidp			IRMonoClassPtrPtrRaw;				// MonoClass**
	typedef irmono_voidp			IRMonoTypePtrPtrRaw;				// MonoType**
	typedef irmono_voidp			IRMonoTableInfoPtrPtrRaw;			// MonoTableInfo**
	typedef irmono_voidp			IRMonoClassFieldPtrPtrRaw;			// MonoClassField**
	typedef irmono_voidp			IRMonoVTablePtrPtrRaw;				// MonoVTable**
	typedef irmono_voidp			IRMonoMethodPtrPtrRaw;				// MonoMethod**
	typedef irmono_voidp			IRMonoPropertyPtrPtrRaw;			// MonoProperty**
	typedef irmono_voidp			IRMonoMethodSignaturePtrPtrRaw;		// MonoMethodSignature**
	typedef irmono_voidp			IRMonoMethodHeaderPtrPtrRaw;		// MonoMethodHeader**
	typedef irmono_voidp			IRMonoMethodDescPtrPtrRaw;			// MonoMethodDescPtr**
	typedef irmono_voidp			IRMonoJitInfoPtrPtrRaw;				// MonoJitInfo**
	typedef irmono_voidp			IRMonoDisHelperPtrPtrRaw;			// MonoDisHelperPtr**
	typedef irmono_voidp			IRMonoObjectPtrPtrRaw;				// MonoObjectPtr**
	typedef irmono_voidp			IRMonoThreadPtrPtrRaw;				// MonoThread**
	typedef irmono_voidp			IRMonoStringPtrPtrRaw;				// MonoString**
	typedef irmono_voidp			IRMonoArrayPtrPtrRaw;				// MonoArray**
	typedef irmono_voidp			IRMonoExceptionPtrPtrRaw;			// MonoException**
	typedef irmono_voidp			IRMonoReflectionTypePtrPtrRaw;		// MonoReflectionType**


	// ********** Simple Remote Handles **********

	typedef RMonoDomainPtr						IRMonoDomainPtr;
	typedef RMonoAssemblyPtr					IRMonoAssemblyPtr;
	typedef RMonoAssemblyNamePtr				IRMonoAssemblyNamePtr;
	typedef RMonoImagePtr						IRMonoImagePtr;
	typedef RMonoClassPtr						IRMonoClassPtr;
	typedef RMonoTypePtr						IRMonoTypePtr;
	typedef RMonoTableInfoPtr					IRMonoTableInfoPtr;
	typedef RMonoClassFieldPtr					IRMonoClassFieldPtr;
	typedef RMonoVTablePtr						IRMonoVTablePtr;
	typedef RMonoMethodPtr						IRMonoMethodPtr;
	typedef RMonoPropertyPtr					IRMonoPropertyPtr;
	typedef RMonoMethodSignaturePtr				IRMonoMethodSignaturePtr;
	typedef RMonoMethodHeaderPtr				IRMonoMethodHeaderPtr;
	typedef RMonoMethodDescPtr					IRMonoMethodDescPtr;
	typedef RMonoJitInfoPtr						IRMonoJitInfoPtr;
	typedef RMonoDisHelperPtr					IRMonoDisHelperPtr;


	// ********** Remote Handles Based on MonoObject **********

	typedef RMonoObjectPtr						IRMonoObjectPtr;
	typedef RMonoThreadPtr						IRMonoThreadPtr;
	typedef RMonoStringPtr						IRMonoStringPtr;
	typedef RMonoArrayPtr						IRMonoArrayPtr;
	typedef RMonoExceptionPtr					IRMonoExceptionPtr;
	typedef RMonoReflectionTypePtr				IRMonoReflectionTypePtr;
};



}




#include "winx64/RMonoABITypeTraitsWinX64.h"
#include "winx32/RMonoABITypeTraitsWinX32.h"


