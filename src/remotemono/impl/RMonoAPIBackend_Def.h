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

#include <string>
#include <string_view>
#include <unordered_set>
#include "RMonoTypes.h"
#include "RMonoAPIFunctionSimple_Def.h"
#include "RMonoAPIFunction_Def.h"
#include "RMonoAPIFunctionTypeAdapters.h"
#include "../IPCVector.h"
#include "../util.h"
#include "visit_struct/visit_struct_intrusive.hpp"
#include "abi/RMonoABITypeTraits.h"
#include "backend/RMonoProcess.h"
#include "backend/RMonoMemBlock.h"




#define REMOTEMONO_API_PART_BEGIN(structname, part) \
		struct structname ## Part ## part { \
			BEGIN_VISITABLES(structname ## Part ## part);
#define REMOTEMONO_API_PART_END() \
			END_VISITABLES; \
		};

#define REMOTEMONO_API_PART_LIST1(structname) structname ## Part1
#define REMOTEMONO_API_PART_LIST2(structname) structname ## Part1, structname ## Part2
#define REMOTEMONO_API_PART_LIST3(structname) structname ## Part1, structname ## Part2, structname ## Part3
#define REMOTEMONO_API_PART_LIST4(structname) structname ## Part1, structname ## Part2, structname ## Part3, structname ## Part4
#define REMOTEMONO_API_PART_LIST5(structname) structname ## Part1, structname ## Part2, structname ## Part3, structname ## Part4, structname ## Part5
#define REMOTEMONO_API_PART_LISTN(structname, n) REMOTEMONO_API_PART_LIST ## n(structname)

#define REMOTEMONO_API_MAIN_DECLARE(structname, nparts) \
		struct structname : public REMOTEMONO_API_PART_LISTN(structname, nparts) { \
			using internal_api_parts = PackHelper<REMOTEMONO_API_PART_LISTN(structname, nparts)>; \
		};

#define REMOTEMONO_API_FUNCTYPE(required, ret, ...) \
		RMonoAPIFunction<ABI, required, ret, __VA_ARGS__>
#define REMOTEMONO_API(name, required, ret, ...) \
		VISITABLE(REMOTEMONO_API_FUNCTYPE(required, ret, __VA_ARGS__), name);

#define REMOTEMONO_API_SIMPLE_FUNCTYPE(ret, ...) \
		RMonoAPIFunctionSimple<ret, __VA_ARGS__>
#define REMOTEMONO_API_SIMPLE(name, ret, ...) \
		VISITABLE(REMOTEMONO_API_SIMPLE_FUNCTYPE(ret, __VA_ARGS__), name);





namespace remotemono
{



/**
 * Contains the ABI-specific definitions of all supported Mono API functions. This is the first place to go to if you want to add
 * a new Mono API function that RemoteMono doesn't support yet. If you're currently looking at the Doxygen-generated docs, stop
 * reading now and look at the actual source code of this class. Doxygen is useless here.
 */
template <typename ABI>
class RMonoAPIBackendStructs
{
private:
	REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(RMonoABITypeTraits<ABI>)

	template <typename TypeT> using ParamOut = tags::ParamOut<TypeT>;
	template <typename TypeT> using ParamInOut = tags::ParamInOut<TypeT>;
	template <typename TypeT> using ParamOvwrInOut = tags::ParamOvwrInOut<TypeT>;
	template <typename TypeT> using ParamException = tags::ParamException<TypeT>;
	template <typename TypeT> using ReturnOwn = tags::ReturnOwn<TypeT>;

	using string = std::string;
	using u16string = std::u16string;
	using u32string = std::u32string;

	using string_view = std::string_view;
	using u16string_view = std::u16string_view;
	using u32string_view = std::u32string_view;

	using Variant = RMonoVariant;
	using VariantArray = RMonoVariantArray;

public:

	// NOTE: Be sure to USE INTERNAL TYPES in the API function definitions below. If one of the types in RETURN or ARGUMENTS starts
	// with 'rmono_' or 'RMono' instead of 'irmono_' or 'IRMono', then something is wrong.

	// **********************************************************
	// *														*
	// *					MAIN MONO API						*
	// *														*
	// **********************************************************

	// NOTE: For the functions listed under MonoAPI only, the actual function name in the remote process will have 'mono_' prepended.
	// This is to keep this definition table short and readable.

	// TODO: mono_string_chars() - Returns char*, but doesn't seem to pass ownership?

REMOTEMONO_API_PART_BEGIN(MonoAPI, 1)

	//				FUNCTION						REQD	RETURN						ARGUMENTS

	REMOTEMONO_API(	free,							false,	void,						irmono_voidp												)

	REMOTEMONO_API(	jit_init,						false,	IRMonoDomainPtr,			string_view													)
	REMOTEMONO_API(	jit_cleanup,					false,	void,						IRMonoDomainPtr												)

	REMOTEMONO_API(	get_root_domain,				true,	IRMonoDomainPtr																			)
	REMOTEMONO_API(	domain_set,						false,	irmono_bool,				IRMonoDomainPtr, irmono_bool								)
	REMOTEMONO_API(	domain_get,						false,	IRMonoDomainPtr																			)
	REMOTEMONO_API(	domain_foreach,					false,	void,						irmono_funcp, irmono_voidp									)
	REMOTEMONO_API(	domain_create_appdomain,		false,	IRMonoDomainPtr,			string_view, string_view									)
	REMOTEMONO_API(	domain_assembly_open,			false,	IRMonoAssemblyPtr,			IRMonoDomainPtr, string_view								)
	REMOTEMONO_API(	domain_unload,					false,	void,						IRMonoDomainPtr												)
	REMOTEMONO_API(	domain_get_friendly_name,		false,	string,						IRMonoDomainPtr												)

	REMOTEMONO_API(	thread_attach,					true,	IRMonoThreadPtr,			IRMonoDomainPtr												)
	REMOTEMONO_API(	thread_detach,					true,	void,						IRMonoThreadPtr												)

	REMOTEMONO_API(	assembly_close,					false,	void,						IRMonoAssemblyPtr											)
	REMOTEMONO_API(	assembly_foreach,				false,	void,						irmono_funcp, irmono_voidp									)
	REMOTEMONO_API(	assembly_get_image,				false,	IRMonoImagePtr,				IRMonoAssemblyPtr											)
	REMOTEMONO_API(	assembly_get_name,				false,	IRMonoAssemblyNamePtr,		IRMonoAssemblyPtr											)
	REMOTEMONO_API(	assembly_name_new,				false,	ReturnOwn<IRMonoAssemblyNamePtr>,
																						string_view													)
	REMOTEMONO_API(	assembly_name_parse,			false,	irmono_bool,				string_view, IRMonoAssemblyNamePtr							)
	REMOTEMONO_API(	assembly_name_free,				false,	void,						IRMonoAssemblyNamePtrRaw									)
	REMOTEMONO_API(	assembly_name_get_name,			false,	std::string,				IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	assembly_name_get_culture,		false,	std::string,				IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	assembly_name_get_version,		false,	uint16_t,					IRMonoAssemblyNamePtr, ParamOut<uint16_t>,
																						ParamOut<uint16_t>, ParamOut<uint16_t>						)
	REMOTEMONO_API(	stringify_assembly_name,		false,	ReturnOwn<string>,			IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	assembly_loaded,				false,	IRMonoAssemblyPtr,			IRMonoAssemblyNamePtr										)

	REMOTEMONO_API(	image_get_name,					false,	string,						IRMonoImagePtr												)
	REMOTEMONO_API(	image_get_filename,				false,	string,						IRMonoImagePtr												)
	REMOTEMONO_API(	image_get_table_info,			false,	IRMonoTableInfoPtr,			IRMonoImagePtr, irmono_int									)
	REMOTEMONO_API(	table_info_get_rows,			false,	irmono_int,					IRMonoTableInfoPtr											)
	REMOTEMONO_API(	image_rva_map,					false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)

	REMOTEMONO_API(	metadata_decode_row_col,		false,	uint32_t,					IRMonoTableInfoPtr, irmono_int, irmono_uint					)
	REMOTEMONO_API(	metadata_guid_heap,				false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	metadata_string_heap,			false,	string,						IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	metadata_blob_heap,				false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	metadata_user_string,			false,	string,						IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	metadata_decode_blob_size,		false,	uint32_t,					irmono_voidp, ParamOut<irmono_voidp>						)

	REMOTEMONO_API(	get_object_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_int16_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_int32_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_int64_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_double_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_single_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_string_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_thread_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_uint16_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_uint32_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_uint64_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_void_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_array_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_boolean_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_byte_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_sbyte_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_char_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	get_exception_class,			false,	IRMonoClassPtr																			)

	REMOTEMONO_API(	class_vtable,					false,	IRMonoVTablePtr,			IRMonoDomainPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	runtime_class_init,				false,	void,						IRMonoVTablePtr												)
	REMOTEMONO_API(	class_get_parent,				false,	IRMonoClassPtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	class_get_type,					false,	IRMonoTypePtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	class_from_name,				false,	IRMonoClassPtr,				IRMonoImagePtr, string_view, string_view					)
	REMOTEMONO_API(	class_from_mono_type,			false,	IRMonoClassPtr,				IRMonoTypePtr												)
	REMOTEMONO_API(	class_get_name,					false,	string,						IRMonoClassPtr												)
	REMOTEMONO_API(	class_get_namespace,			false,	string,						IRMonoClassPtr												)
	REMOTEMONO_API(	class_get_fields,				false,	IRMonoClassFieldPtr,		IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	class_get_methods,				false,	IRMonoMethodPtr,			IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	class_get_properties,			false,	IRMonoPropertyPtr,			IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	class_get_field_from_name,		false,	IRMonoClassFieldPtr,		IRMonoClassPtr, string_view									)
	REMOTEMONO_API(	class_get_method_from_name,		false,	IRMonoMethodPtr,			IRMonoClassPtr, string_view, irmono_int						)
	REMOTEMONO_API(	class_get_property_from_name,	false,	IRMonoPropertyPtr,			IRMonoClassPtr, string_view									)
	REMOTEMONO_API(	class_get_element_class,		false,	IRMonoClassPtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	class_get_flags,				false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	class_get_rank,					false,	irmono_int,					IRMonoClassPtr												)
	REMOTEMONO_API(	class_is_valuetype,				true,	irmono_bool,				IRMonoClassPtr												)
	REMOTEMONO_API(	class_data_size,				false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	class_instance_size,			false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	class_value_size,				true,	int32_t,					IRMonoClassPtr, ParamOut<uint32_t>							)

	REMOTEMONO_API(	type_get_object,				false,	IRMonoReflectionTypePtr,	IRMonoDomainPtr, IRMonoTypePtr								)
	REMOTEMONO_API(	type_get_name,					false,	ReturnOwn<string>,			IRMonoTypePtr												)
	REMOTEMONO_API(	type_get_class,					false,	IRMonoClassPtr,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_get_type,					false,	irmono_int,					IRMonoTypePtr												)
	REMOTEMONO_API(	type_is_byref,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_is_pointer,				false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_is_reference,				false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_is_struct,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_is_void,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	type_size,						false,	irmono_int,					IRMonoTypePtr, ParamOut<irmono_int>							)
	REMOTEMONO_API(	type_stack_size,				false,	irmono_int,					IRMonoTypePtr, ParamOut<irmono_int>							)

	REMOTEMONO_API(	field_get_name,					false,	string,						IRMonoClassFieldPtr											)
	REMOTEMONO_API(	field_get_flags,				false,	uint32_t,					IRMonoClassFieldPtr											)
	REMOTEMONO_API(	field_get_parent,				false,	IRMonoClassPtr,				IRMonoClassFieldPtr											)
	REMOTEMONO_API(	field_get_type,					false,	IRMonoTypePtr,				IRMonoClassFieldPtr											)
	REMOTEMONO_API(	field_set_value,				false,	void,						IRMonoObjectPtr, IRMonoClassFieldPtr, Variant				)
	REMOTEMONO_API(	field_get_value,				false,	void,						IRMonoObjectPtr, IRMonoClassFieldPtr, ParamOut<Variant>		)
	REMOTEMONO_API(	field_get_value_object,			false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassFieldPtr, IRMonoObjectPtr		)
	REMOTEMONO_API(	field_static_set_value,			false,	void,						IRMonoVTablePtr, IRMonoClassFieldPtr, Variant				)
	REMOTEMONO_API(	field_static_get_value,			false,	void,						IRMonoVTablePtr, IRMonoClassFieldPtr, ParamOut<Variant>		)
	REMOTEMONO_API(	field_get_offset,				false,	uint32_t,					IRMonoClassFieldPtr											)

REMOTEMONO_API_PART_END()
REMOTEMONO_API_PART_BEGIN(MonoAPI, 2)

	REMOTEMONO_API(	method_get_class,				false,	IRMonoClassPtr,				IRMonoMethodPtr												)
	REMOTEMONO_API(	method_get_name,				false,	string,						IRMonoMethodPtr												)
	REMOTEMONO_API(	method_get_flags,				false,	uint32_t,					IRMonoMethodPtr, ParamOut<uint32_t>							)
	REMOTEMONO_API(	method_full_name,				false,	ReturnOwn<string>,			IRMonoMethodPtr, irmono_bool								)
	REMOTEMONO_API(	method_signature,				false,	IRMonoMethodSignaturePtr,	IRMonoMethodPtr												)
	REMOTEMONO_API(	method_get_header,				false,	IRMonoMethodHeaderPtr,		IRMonoMethodPtr												)
	REMOTEMONO_API(	method_header_get_code,			false,	irmono_funcp,				IRMonoMethodHeaderPtr, ParamOut<uint32_t>,
																						ParamOut<uint32_t>											)
	REMOTEMONO_API(	method_desc_new,				false,	ReturnOwn<IRMonoMethodDescPtr>,
																						string_view, irmono_bool									)
	REMOTEMONO_API(	method_desc_free,				false,	void,						IRMonoMethodDescPtrRaw										)
	REMOTEMONO_API(	method_desc_match,				false,	irmono_bool,				IRMonoMethodDescPtr, IRMonoMethodPtr						)
	REMOTEMONO_API(	method_desc_search_in_class,	false,	IRMonoMethodPtr,			IRMonoMethodDescPtr, IRMonoClassPtr							)
	REMOTEMONO_API(	method_desc_search_in_image,	false,	IRMonoMethodPtr,			IRMonoMethodDescPtr, IRMonoImagePtr							)

	REMOTEMONO_API(	property_get_name,				false,	string,						IRMonoPropertyPtr											)
	REMOTEMONO_API(	property_get_flags,				false,	uint32_t,					IRMonoPropertyPtr											)
	REMOTEMONO_API(	property_get_parent,			false,	IRMonoClassPtr,				IRMonoPropertyPtr											)
	REMOTEMONO_API(	property_get_set_method,		false,	IRMonoMethodPtr,			IRMonoPropertyPtr											)
	REMOTEMONO_API(	property_get_get_method,		false,	IRMonoMethodPtr,			IRMonoPropertyPtr											)
	REMOTEMONO_API(	property_get_value,				false,	IRMonoObjectPtr,			IRMonoPropertyPtr, RMonoVariant,
																						ParamOvwrInOut<ParamOut<VariantArray>>,
																						ParamException<IRMonoExceptionPtr>							)
	REMOTEMONO_API(	property_set_value,				false,	void,						IRMonoPropertyPtr, RMonoVariant,
																						ParamOvwrInOut<VariantArray>,
																						ParamException<IRMonoExceptionPtr>							)

	REMOTEMONO_API(	signature_get_return_type,		false,	IRMonoTypePtr,				IRMonoMethodSignaturePtr									)
	REMOTEMONO_API(	signature_get_params,			false,	IRMonoTypePtr,				IRMonoMethodSignaturePtr, irmono_voidpp						)
	REMOTEMONO_API(	signature_get_call_conv,		false,	uint32_t,					IRMonoMethodSignaturePtr									)
	REMOTEMONO_API(	signature_get_desc,				false,	ReturnOwn<string>,			IRMonoMethodSignaturePtr, irmono_bool						)

	REMOTEMONO_API(	object_get_class,				true,	IRMonoClassPtr,				IRMonoObjectPtr												)
	REMOTEMONO_API(	object_new,						false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	runtime_object_init,			false,	void,						Variant														)
	REMOTEMONO_API(	object_unbox,					true,	Variant,					IRMonoObjectPtr												)
	REMOTEMONO_API(	value_box,						false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassPtr, RMonoVariant				)
	REMOTEMONO_API(	object_to_string,				false,	IRMonoStringPtr,			Variant, ParamException<IRMonoExceptionPtr>					)
	REMOTEMONO_API(	object_clone,					false,	IRMonoObjectPtr,			IRMonoObjectPtr												)
	REMOTEMONO_API(	object_get_domain,				false,	IRMonoDomainPtr,			IRMonoObjectPtr												)
	REMOTEMONO_API(	object_get_virtual_method,		false,	IRMonoMethodPtr,			IRMonoObjectPtr, IRMonoMethodPtr							)
	REMOTEMONO_API(	object_isinst,					false,	IRMonoObjectPtr,			IRMonoObjectPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	object_get_size,				false,	irmono_uint,				IRMonoObjectPtr												)

	REMOTEMONO_API(	string_new,						false,	IRMonoStringPtr,			IRMonoDomainPtr, string_view								)
	REMOTEMONO_API(	string_new_len,					false,	IRMonoStringPtr,			IRMonoDomainPtr, string_view, irmono_uint					)
	REMOTEMONO_API(	string_new_utf16,				false,	IRMonoStringPtr,			IRMonoDomainPtr, u16string_view, int32_t					)
	REMOTEMONO_API(	string_new_utf32,				false,	IRMonoStringPtr,			IRMonoDomainPtr, u32string_view, int32_t					)
	REMOTEMONO_API(	string_to_utf8,					false,	ReturnOwn<string>,			IRMonoStringPtr												)
	REMOTEMONO_API(	string_to_utf16,				false,	ReturnOwn<u16string>,		IRMonoStringPtr												)
	REMOTEMONO_API(	string_to_utf32,				false,	ReturnOwn<u32string>,		IRMonoStringPtr												)
	REMOTEMONO_API(	string_chars,					false,	u16string,					IRMonoStringPtr												)
	REMOTEMONO_API(	string_length,					false,	irmono_int,					IRMonoStringPtr												)
	REMOTEMONO_API(	string_equal,					false,	irmono_bool,				IRMonoStringPtr, IRMonoStringPtr							)

	REMOTEMONO_API(	array_new,						false,	IRMonoArrayPtr,				IRMonoDomainPtr, IRMonoClassPtr, irmono_uintptr_t			)
	REMOTEMONO_API(	array_new_full,					false,	IRMonoArrayPtr,				IRMonoDomainPtr, IRMonoClassPtr, irmono_voidp, irmono_voidp	)
	REMOTEMONO_API(	array_class_get,				false,	IRMonoClassPtr,				IRMonoClassPtr, uint32_t									)
	REMOTEMONO_API(	array_addr_with_size,			false,	Variant,					IRMonoArrayPtr, irmono_int, irmono_uintptr_t				)
	REMOTEMONO_API(	array_length,					false,	irmono_uintptr_t,			IRMonoArrayPtr												)
	REMOTEMONO_API(	array_element_size,				false,	int32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	class_array_element_size,		false,	int32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	array_clone,					false,	IRMonoArrayPtr,				IRMonoArrayPtr												)

	REMOTEMONO_API(	gchandle_new,					true,	irmono_gchandle,			IRMonoObjectPtr, irmono_bool								)
	REMOTEMONO_API(	gchandle_new_weakref,			false,	irmono_gchandle,			IRMonoObjectPtr, irmono_bool								)
	REMOTEMONO_API(	gchandle_get_target,			true,	IRMonoObjectPtrRaw,			irmono_gchandle												)
	REMOTEMONO_API(	gchandle_free,					true,	void,						irmono_gchandle												)

	REMOTEMONO_API(	gc_collect,						false,	void,						irmono_int													)
	REMOTEMONO_API(	gc_max_generation,				false,	irmono_int																				)
	REMOTEMONO_API(	gc_get_generation,				false,	irmono_int,					IRMonoObjectPtr												)
	REMOTEMONO_API(	gc_wbarrier_set_arrayref,		false,	void,						IRMonoArrayPtr, irmono_voidp, IRMonoObjectPtr				)

	REMOTEMONO_API(	runtime_invoke,					false,	IRMonoObjectPtr,			IRMonoMethodPtr, Variant,
																						ParamOvwrInOut<VariantArray>,
																						ParamException<IRMonoExceptionPtr>							)

	REMOTEMONO_API(	compile_method,					false,	irmono_voidp,				IRMonoMethodPtr												)

	REMOTEMONO_API(	jit_info_table_find,			false,	IRMonoJitInfoPtr,			IRMonoDomainPtr, irmono_voidp								)
	REMOTEMONO_API(	jit_info_get_code_start,		false,	irmono_funcp,				IRMonoJitInfoPtr											)
	REMOTEMONO_API(	jit_info_get_code_size,			false,	irmono_int,					IRMonoJitInfoPtr											)
	REMOTEMONO_API(	jit_info_get_method,			false,	IRMonoMethodPtr,			IRMonoJitInfoPtr											)

	REMOTEMONO_API(	disasm_code,					false,	ReturnOwn<string>,			IRMonoDisHelperPtr, IRMonoMethodPtr,
																						irmono_cbytep, irmono_cbytep								)
	REMOTEMONO_API(	pmip,							false,	ReturnOwn<string>,			irmono_voidp												)
REMOTEMONO_API_PART_END()

	REMOTEMONO_API_MAIN_DECLARE(MonoAPI, 2)







	// **********************************************************
	// *														*
	// *					MISC MONO API						*
	// *														*
	// **********************************************************

REMOTEMONO_API_PART_BEGIN(MiscAPI, 1)

	//				FUNCTION						REQD	RETURN						ARGUMENTS

	REMOTEMONO_API(	g_free,							false,	void,						irmono_voidp												)

REMOTEMONO_API_PART_END()

	REMOTEMONO_API_MAIN_DECLARE(MiscAPI, 1)






	// **********************************************************
	// *														*
	// *					BOILERPLATE API						*
	// *														*
	// **********************************************************

	// These functions are RemoteMono additions. They are created by assembleBoilerplateCode() and then injected into the remote process.

REMOTEMONO_API_PART_BEGIN(BoilerplateAPI, 1)

	//						FUNCTION						RETURN						ARGUMENTS

	REMOTEMONO_API_SIMPLE(	rmono_foreach_ipcvec_adapter,	void,						irmono_voidp, irmono_voidp									)
	REMOTEMONO_API_SIMPLE(	rmono_gchandle_pin,				irmono_gchandle,			irmono_gchandle												)
	REMOTEMONO_API_SIMPLE(	rmono_array_setref,				void,						irmono_gchandle, irmono_uintptr_t, irmono_gchandle			)

REMOTEMONO_API_PART_END()

	REMOTEMONO_API_MAIN_DECLARE(BoilerplateAPI, 1)
};




/**
 * @see RMonoAPIBackendBase
 */
template <typename ABI>
class RMonoAPIBackend :
		public RMonoAPIBackendStructs<ABI>::MonoAPI,
		public RMonoAPIBackendStructs<ABI>::MiscAPI,
		public RMonoAPIBackendStructs<ABI>::BoilerplateAPI
{
private:
	REMOTEMONO_ABI_TYPETRAITS_TYPEDEFS(RMonoABITypeTraits<ABI>)

	typedef typename RMonoAPIBackendStructs<ABI>::MonoAPI MonoAPI;
	typedef typename RMonoAPIBackendStructs<ABI>::MiscAPI MiscAPI;
	typedef typename RMonoAPIBackendStructs<ABI>::BoilerplateAPI BoilerplateAPI;

public:
	typedef IPCVector<irmono_voidp, irmono_voidp> IPCVec;

public:
	RMonoAPIBackend(ABI* abi);
	~RMonoAPIBackend();

	/**
	 * Create all the Mono API wrapper functions and find the raw functions in the remote process.
	 *
	 * @param mono The frontend object being used. You can not use RMonoAPIBackend without a frontend.
	 * @param process The remote process.
	 * @param workerThread The already created worker thread in the remote process. It does not need to be Mono-attached
	 * 		for calling this method yet.
	 */
	void injectAPI(RMonoAPI* mono, backend::RMonoProcess& process);

	/**
	 * Release all resources in the remote process and detach the backend.
	 */
	void uninjectAPI();


	/**
	 * Returns the IPCVector object used for the various mono_*_foreach() functions.
	 */
	IPCVec& getIPCVector() { return ipcVec; }

	/**
	 * Returns the actual remote vector pointer for the IPCVector.
	 *
	 * @see getIPCVector()
	 */
	typename IPCVec::VectorPtr getIPCVectorInstance() { return ipcVecPtr; }

	bool isAPIFunctionSupported(const std::string& name) const { return validAPIFuncNames.find(name) != validAPIFuncNames.end(); }

private:
	ABI* getABI() { return abi; }

	std::string assembleBoilerplateCode();

	template <typename FuncT, typename MainT, typename... PartialT>
	void foreachAPIPacked(MainT& main, FuncT func, PackHelper<PartialT...>);

	template <typename FuncT, typename FirstT, typename... OtherT>
	void foreachAPIRecurse(FuncT func, FirstT& part, OtherT&... other);

	template <typename MainAPIT, typename FuncT>
	void foreachAPI(MainAPIT& api, FuncT func);

private:
	ABI* abi;
	backend::RMonoProcess* process;
	IPCVec ipcVec;
	typename IPCVec::VectorPtr ipcVecPtr;
	backend::RMonoMemBlock remDataBlock;
	bool injected;
	std::unordered_set<std::string> validAPIFuncNames;
};


}
