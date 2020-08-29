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

#include "RMonoABITypeTraits.h"
#include "../RMonoTypes.h"
#include "../RMonoHandle_Def.h"





namespace remotemono
{



class RMonoAPIBase;



#define REMOTEMONO_ABI_CONV_SIMPLEINT_U(pubtype)																					\
		struct PtrConv_##pubtype {																									\
			PtrConv_##pubtype(pubtype* p) : p(p), v(p ? ((I##pubtype) *p) : I##pubtype()) {}										\
			~PtrConv_##pubtype() { if (p) *p = (pubtype) v; }																		\
			operator I##pubtype* () { return &v; }																					\
			pubtype* p;																												\
			I##pubtype v;																											\
		};																															\
		I ## pubtype p2i_ ## pubtype (pubtype v) const { return (I ## pubtype) v; }													\
		pubtype i2p_ ## pubtype (I ## pubtype v) const { return (pubtype) v; }														\
		template <typename PubT = pubtype>																							\
		typename std::enable_if_t<!std::is_same_v<PubT, I##pubtype>, PtrConv_##pubtype>												\
				pp2i_##pubtype (pubtype* v) const { return PtrConv_##pubtype(v); }													\
		template <typename PubT = pubtype>																							\
		typename std::enable_if_t<std::is_same_v<PubT, I##pubtype>, I##pubtype*>													\
				pp2i_##pubtype (pubtype* v) const { return v; }
#define REMOTEMONO_ABI_CONV_SIMPLEINT_L(pubtype)																					\
		struct PtrConv_##pubtype {																									\
			PtrConv_##pubtype(pubtype* p) : p(p), v(p ? ((i##pubtype) *p) : i##pubtype()) {}										\
			~PtrConv_##pubtype() { if (p) *p = (pubtype) v; }																		\
			operator i##pubtype* () { return &v; }																					\
			pubtype* p;																												\
			i##pubtype v;																											\
		};																															\
		i##pubtype p2i_##pubtype (pubtype v) const { return (i##pubtype) v; }														\
		pubtype i2p_##pubtype (i##pubtype v) const { return (pubtype) v; }															\
		template <typename PubT = pubtype>																							\
		typename std::enable_if_t<!std::is_same_v<PubT, i##pubtype>, PtrConv_##pubtype>											\
				pp2i_##pubtype (pubtype* v) const { return PtrConv_##pubtype(v); }													\
		template <typename PubT = pubtype>																							\
		typename std::enable_if_t<std::is_same_v<PubT, i##pubtype>, i##pubtype*>													\
				pp2i_##pubtype (pubtype* v) const { return v; }

#define REMOTEMONO_ABI_CONV_IDENTITY(pubtype)																						\
		pubtype p2i_ ## pubtype (pubtype v) const { return v; }																		\
		pubtype i2p_ ## pubtype (pubtype v) const { return v; }																		\

#define REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(pubtype)																					\
		I ## pubtype ## Raw hp2i_ ## pubtype (const pubtype& v) const { return (I ## pubtype ## Raw) *v; }							\
		pubtype hi2p_ ## pubtype (I ## pubtype ## Raw v, RMonoAPIBase* mono, bool owned) const											\
		{																															\
			return pubtype((typename pubtype::HandleType) v, mono, owned);															\
		}

#define REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(pubtype)																				\
		irmono_gchandle hp2i_ ## pubtype (const pubtype& v) const { return (irmono_gchandle) *v; }									\
		pubtype hi2p_ ## pubtype (irmono_gchandle h, RMonoAPIBase* mono) const															\
		{																															\
			return pubtype((typename pubtype::HandleType) h, mono, true);															\
		}




/**
 * The ABI component that defined methods for converting between the public (e.g. `rmono_voidp`) and internal (e.g. `irmono_voidp`)
 * types. Methods named `p2i_TYPE` convert from public to internal, and `i2p_TYPE` convert from internal to public types. Methods
 * called `hp2i_TYPE` or `hi2p_TYPE` convert between RMonoHandle instances and their corresponding raw handles.
 */
template <typename ABI>
class RMonoABIConvCommon
{
private:
	REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(RMonoABITypeTraits<ABI>)

public:
	// ********** Fundamental Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_uintptr_t								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_intptr_t								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_int									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_uint									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_bool									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_byte									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar2								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar4								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_gchandle								)


	// ********** Pointers to Fundamental Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_voidp									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_charp									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_ccharp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_bytep									)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cbytep								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar2p								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cunichar2p							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar4p								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cunichar4p							)


	// ********** Double-Pointers to Fundamental Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_voidpp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_charpp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_ccharpp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_bytepp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cbytepp								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar2pp							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cunichar2pp							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_unichar4pp							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_cunichar4pp							)


	// ********** Miscellaneous Fundamental Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_L(	rmono_funcp									)


	// ********** Raw Pointers to Handle Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoDomainPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoAssemblyPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoAssemblyNamePtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoImagePtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoClassPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoTypePtrRaw								)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoTableInfoPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoClassFieldPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoVTablePtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoPropertyPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodSignaturePtrRaw					)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodHeaderPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodDescPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoJitInfoPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoDisHelperPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoObjectPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoThreadPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoStringPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoArrayPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoExceptionPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoReflectionTypePtrRaw					)


	// ********** Raw Double-Pointers to Handle Types **********

	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoDomainPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoAssemblyPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoAssemblyNamePtrPtrRaw					)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoImagePtrPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoClassPtrPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoTypePtrPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoTableInfoPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoClassFieldPtrPtrRaw					)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoVTablePtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoPropertyPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodSignaturePtrPtrRaw				)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodHeaderPtrPtrRaw					)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoMethodDescPtrPtrRaw					)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoJitInfoPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoDisHelperPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoObjectPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoThreadPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoStringPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoArrayPtrPtrRaw							)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoExceptionPtrPtrRaw						)
	REMOTEMONO_ABI_CONV_SIMPLEINT_U(	RMonoReflectionTypePtrPtrRaw				)


	// ********** Simple Remote Handles **********

	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoDomainPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoAssemblyPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoAssemblyNamePtr						)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoImagePtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoClassPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoTypePtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoTableInfoPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoClassFieldPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoVTablePtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoMethodPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoPropertyPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoMethodSignaturePtr						)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoMethodHeaderPtr						)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoMethodDescPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoJitInfoPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoDisHelperPtr							)


	// ********** Remote Handles Based on MonoObject **********

	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoObjectPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoThreadPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoStringPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoArrayPtr								)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoExceptionPtr							)
	REMOTEMONO_ABI_CONV_IDENTITY(	RMonoReflectionTypePtr						)

public:
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoDomainPtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoAssemblyPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoAssemblyNamePtr						)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoImagePtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoClassPtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoTypePtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoTableInfoPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoClassFieldPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoVTablePtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoMethodPtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoPropertyPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoMethodSignaturePtr						)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoMethodHeaderPtr						)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoMethodDescPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoJitInfoPtr								)
	REMOTEMONO_ABI_CONV_HANDLE_RAWPTR(	RMonoDisHelperPtr							)

	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoObjectPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoThreadPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoStringPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoArrayPtr							)
	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoExceptionPtr						)
	REMOTEMONO_ABI_CONV_HANDLE_MONOOBJECT(	RMonoReflectionTypePtr					)
};



}
