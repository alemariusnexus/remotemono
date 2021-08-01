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



#define REMOTEMONO_GCHANDLE_FREE_BUF_SIZE_MAX 256
#define REMOTEMONO_RAW_FREE_BUF_SIZE_MAX 256


#define REMOTEMONO_API_SIMPLE_FUNCTYPE(ret, ...) RMonoAPIFunctionSimple<ret, ##__VA_ARGS__>
#define REMOTEMONO_API_FUNCTYPE(required, ret, ...) RMonoAPIFunction<ABI, required, ret, ##__VA_ARGS__>

#ifdef __clang__

// There's a bug in visit_struct that causes errors with the default macros below in Clang,
// if there are more than 2 VISITABLEs in a single struct. Until that issue is resolved, we
// put every VISITABLE in its own struct and join them all via inheritance in the end.
// That's pretty extreme, but I haven't found an easier solution.
// See: https://github.com/garbageslam/visit_struct/issues/20

#define REMOTEMONO__L19(a,b) a,1,b,a,2,b,a,3,b,a,4,b,a,5,b,a,6,b,a,7,b,a,8,b,a,9,b,a,10,b,a,11,b,a,12,b,a,13,b,a,14,b,a,15,b,a,16,b,a,17,b,a,18,b,a,19,b
#define REMOTEMONO__L399(a,b) a,1,b,a,2,b,a,3,b,a,4,b,a,5,b,a,6,b,a,7,b,a,8,b,a,9,b,a,10,b,a,11,b,a,12,b,a,13,b,a,14,b,a,15,b,a,16,b,a,17,b,a,18,b,a,19,b,a,20,b,a,21,b,a,22,b,a,23,b,a,24,b,a,25,b,a,26,b,a,27,b,a,28,b,a,29,b,a,30,b,a,31,b,a,32,b,a,33,b,a,34,b,a,35,b,a,36,b,a,37,b,a,38,b,a,39,b,a,40,b,a,41,b,a,42,b,a,43,b,a,44,b,a,45,b,a,46,b,a,47,b,a,48,b,a,49,b,a,50,b,a,51,b,a,52,b,a,53,b,a,54,b,a,55,b,a,56,b,a,57,b,a,58,b,a,59,b,a,60,b,a,61,b,a,62,b,a,63,b,a,64,b,a,65,b,a,66,b,a,67,b,a,68,b,a,69,b,a,70,b,a,71,b,a,72,b,a,73,b,a,74,b,a,75,b,a,76,b,a,77,b,a,78,b,a,79,b,a,80,b,a,81,b,a,82,b,a,83,b,a,84,b,a,85,b,a,86,b,a,87,b,a,88,b,a,89,b,a,90,b,a,91,b,a,92,b,a,93,b,a,94,b,a,95,b,a,96,b,a,97,b,a,98,b,a,99,b,a,100,b,a,101,b,a,102,b,a,103,b,a,104,b,a,105,b,a,106,b,a,107,b,a,108,b,a,109,b,a,110,b,a,111,b,a,112,b,a,113,b,a,114,b,a,115,b,a,116,b,a,117,b,a,118,b,a,119,b,a,120,b,a,121,b,a,122,b,a,123,b,a,124,b,a,125,b,a,126,b,a,127,b,a,128,b,a,129,b,a,130,b,a,131,b,a,132,b,a,133,b,a,134,b,a,135,b,a,136,b,a,137,b,a,138,b,a,139,b,a,140,b,a,141,b,a,142,b,a,143,b,a,144,b,a,145,b,a,146,b,a,147,b,a,148,b,a,149,b,a,150,b,a,151,b,a,152,b,a,153,b,a,154,b,a,155,b,a,156,b,a,157,b,a,158,b,a,159,b,a,160,b,a,161,b,a,162,b,a,163,b,a,164,b,a,165,b,a,166,b,a,167,b,a,168,b,a,169,b,a,170,b,a,171,b,a,172,b,a,173,b,a,174,b,a,175,b,a,176,b,a,177,b,a,178,b,a,179,b,a,180,b,a,181,b,a,182,b,a,183,b,a,184,b,a,185,b,a,186,b,a,187,b,a,188,b,a,189,b,a,190,b,a,191,b,a,192,b,a,193,b,a,194,b,a,195,b,a,196,b,a,197,b,a,198,b,a,199,b,a,200,b,a,201,b,a,202,b,a,203,b,a,204,b,a,205,b,a,206,b,a,207,b,a,208,b,a,209,b,a,210,b,a,211,b,a,212,b,a,213,b,a,214,b,a,215,b,a,216,b,a,217,b,a,218,b,a,219,b,a,220,b,a,221,b,a,222,b,a,223,b,a,224,b,a,225,b,a,226,b,a,227,b,a,228,b,a,229,b,a,230,b,a,231,b,a,232,b,a,233,b,a,234,b,a,235,b,a,236,b,a,237,b,a,238,b,a,239,b,a,240,b,a,241,b,a,242,b,a,243,b,a,244,b,a,245,b,a,246,b,a,247,b,a,248,b,a,249,b,a,250,b,a,251,b,a,252,b,a,253,b,a,254,b,a,255,b,a,256,b,a,257,b,a,258,b,a,259,b,a,260,b,a,261,b,a,262,b,a,263,b,a,264,b,a,265,b,a,266,b,a,267,b,a,268,b,a,269,b,a,270,b,a,271,b,a,272,b,a,273,b,a,274,b,a,275,b,a,276,b,a,277,b,a,278,b,a,279,b,a,280,b,a,281,b,a,282,b,a,283,b,a,284,b,a,285,b,a,286,b,a,287,b,a,288,b,a,289,b,a,290,b,a,291,b,a,292,b,a,293,b,a,294,b,a,295,b,a,296,b,a,297,b,a,298,b,a,299,b,a,300,b,a,301,b,a,302,b,a,303,b,a,304,b,a,305,b,a,306,b,a,307,b,a,308,b,a,309,b,a,310,b,a,311,b,a,312,b,a,313,b,a,314,b,a,315,b,a,316,b,a,317,b,a,318,b,a,319,b,a,320,b,a,321,b,a,322,b,a,323,b,a,324,b,a,325,b,a,326,b,a,327,b,a,328,b,a,329,b,a,330,b,a,331,b,a,332,b,a,333,b,a,334,b,a,335,b,a,336,b,a,337,b,a,338,b,a,339,b,a,340,b,a,341,b,a,342,b,a,343,b,a,344,b,a,345,b,a,346,b,a,347,b,a,348,b,a,349,b,a,350,b,a,351,b,a,352,b,a,353,b,a,354,b,a,355,b,a,356,b,a,357,b,a,358,b,a,359,b,a,360,b,a,361,b,a,362,b,a,363,b,a,364,b,a,365,b,a,366,b,a,367,b,a,368,b,a,369,b,a,370,b,a,371,b,a,372,b,a,373,b,a,374,b,a,375,b,a,376,b,a,377,b,a,378,b,a,379,b,a,380,b,a,381,b,a,382,b,a,383,b,a,384,b,a,385,b,a,386,b,a,387,b,a,388,b,a,389,b,a,390,b,a,391,b,a,392,b,a,393,b,a,394,b,a,395,b,a,396,b,a,397,b,a,398,b,a,399,b
#define REMOTEMONO_API_LISTN(apiidx, n) REMOTEMONO__L ## n (_RMonoVisitable<apiidx, void>)

#define REMOTEMONO_API_GLOBAL_BEGIN \
		template <size_t apiIdx, size_t idx, typename Enable> \
		struct _RMonoVisitable;
#define REMOTEMONO_API_BEGIN(structname, apiidx) \
		struct _RMonoVisitable ## structname ## Tag {}; \
        template <typename Enable> \
        struct _RMonoVisitable<apiidx,0,Enable> {};
#define REMOTEMONO_API_END(structname, apiidx, finalidx) \
		struct structname : REMOTEMONO_API_LISTN(apiidx, finalidx) { \
			using internal_api_parts = PackHelper<REMOTEMONO_API_LISTN(apiidx, finalidx)>; \
		};

#define REMOTEMONO_API(apiidx, idx, name, required, ret, ...) \
		template <typename Enable> \
		struct _RMonoVisitable<apiidx, idx, Enable> { \
            typedef _RMonoVisitable<apiidx, idx, Enable> Self; \
            BEGIN_VISITABLES(Self); \
            VISITABLE(REMOTEMONO_API_FUNCTYPE(required, ret, ##__VA_ARGS__), name); \
            END_VISITABLES; \
        };
#define REMOTEMONO_API_SKIP(apiidx, first, last) \
		template <size_t idx> \
		struct _RMonoVisitable<apiidx, idx, std::enable_if_t<idx >= first  &&  idx <= last>> { \
            typedef _RMonoVisitable<apiidx, idx, void> Self; \
            virtual ~_RMonoVisitable() = default; \
            BEGIN_VISITABLES(Self); \
            END_VISITABLES; \
        };

#define REMOTEMONO_API_SIMPLE(apiidx, idx, name, ret, ...) \
		template <typename Enable> \
		struct _RMonoVisitable<apiidx, idx, Enable> { \
            typedef _RMonoVisitable<apiidx, idx, Enable> Self; \
            BEGIN_VISITABLES(Self); \
            VISITABLE(REMOTEMONO_API_SIMPLE_FUNCTYPE(ret, ##__VA_ARGS__), name); \
            END_VISITABLES; \
        };

#else

#define REMOTEMONO_API_GLOBAL_BEGIN
#define REMOTEMONO_API_BEGIN(structname, apiidx) \
		struct structname { \
			BEGIN_VISITABLES(structname);
#define REMOTEMONO_API_END(structname, apiidx, finalidx) \
			END_VISITABLES; \
			using internal_api_parts = PackHelper<structname>; \
		};

#define REMOTEMONO_API(apiidx, idx, name, required, ret, ...) VISITABLE(REMOTEMONO_API_FUNCTYPE(required, ret, ##__VA_ARGS__), name);
#define REMOTEMONO_API_SKIP(apiidx, first, last)

#define REMOTEMONO_API_SIMPLE(apiidx, idx, name, ret, ...) VISITABLE(REMOTEMONO_API_SIMPLE_FUNCTYPE(ret, ##__VA_ARGS__), name);

#endif





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
	template <typename TypeT> using ParamOutRetCls = tags::ParamOutRetCls<TypeT>;
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

	REMOTEMONO_API_GLOBAL_BEGIN

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

	REMOTEMONO_API_BEGIN(MonoAPI, 0)

	//						FUNCTION						REQD	RETURN						ARGUMENTS

	REMOTEMONO_API(	0,	1,	free,							false,	void,						irmono_voidp												)
	REMOTEMONO_API_SKIP(0, 2, 9)

	REMOTEMONO_API(	0,	10,	jit_init,						false,	IRMonoDomainPtr,			string_view													)
	REMOTEMONO_API(	0,	11,	jit_cleanup,					false,	void,						IRMonoDomainPtr												)
	REMOTEMONO_API_SKIP(0, 12, 19)


	REMOTEMONO_API(	0,	20,	get_root_domain,				true,	IRMonoDomainPtr																			)
	REMOTEMONO_API(	0,	21,	domain_set,						false,	irmono_bool,				IRMonoDomainPtr, irmono_bool								)
	REMOTEMONO_API(	0,	22,	domain_get,						false,	IRMonoDomainPtr																			)
	REMOTEMONO_API(	0,	23,	domain_foreach,					false,	void,						irmono_funcp, irmono_voidp									)
	REMOTEMONO_API(	0,	24,	domain_create_appdomain,		false,	IRMonoDomainPtr,			string_view, string_view									)
	REMOTEMONO_API(	0,	25,	domain_assembly_open,			false,	IRMonoAssemblyPtr,			IRMonoDomainPtr, string_view								)
	REMOTEMONO_API(	0,	26,	domain_unload,					false,	void,						IRMonoDomainPtr												)
	REMOTEMONO_API(	0,	27,	domain_get_friendly_name,		false,	string,						IRMonoDomainPtr												)
	REMOTEMONO_API_SKIP(0, 28, 39)

	REMOTEMONO_API(	0,	40,	thread_attach,					true,	IRMonoThreadPtr,			IRMonoDomainPtr												)
	REMOTEMONO_API(	0,	41,	thread_detach,					true,	void,						IRMonoThreadPtr												)
	REMOTEMONO_API_SKIP(0, 42, 49)

	REMOTEMONO_API(	0,	50,	assembly_close,					false,	void,						IRMonoAssemblyPtr											)
	REMOTEMONO_API(	0,	51,	assembly_foreach,				false,	void,						irmono_funcp, irmono_voidp									)
	REMOTEMONO_API(	0,	52,	assembly_get_image,				false,	IRMonoImagePtr,				IRMonoAssemblyPtr											)
	REMOTEMONO_API(	0,	53,	assembly_get_name,				false,	IRMonoAssemblyNamePtr,		IRMonoAssemblyPtr											)
	REMOTEMONO_API(	0,	54,	assembly_name_new,				false,	ReturnOwn<IRMonoAssemblyNamePtr>,
																								string_view													)
	REMOTEMONO_API(	0,	55,	assembly_name_parse,			false,	irmono_bool,				string_view, IRMonoAssemblyNamePtr							)
	REMOTEMONO_API(	0,	56,	assembly_name_free,				false,	void,						IRMonoAssemblyNamePtrRaw									)
	REMOTEMONO_API(	0,	57,	assembly_name_get_name,			false,	std::string,				IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	0,	58,	assembly_name_get_culture,		false,	std::string,				IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	0,	59,	assembly_name_get_version,		false,	uint16_t,					IRMonoAssemblyNamePtr, ParamOut<uint16_t>,
																								ParamOut<uint16_t>, ParamOut<uint16_t>						)
	REMOTEMONO_API(	0,	60,	stringify_assembly_name,		false,	ReturnOwn<string>,			IRMonoAssemblyNamePtr										)
	REMOTEMONO_API(	0,	61,	assembly_loaded,				false,	IRMonoAssemblyPtr,			IRMonoAssemblyNamePtr										)
	REMOTEMONO_API_SKIP(0, 62, 69)

	REMOTEMONO_API(	0,	70,	image_get_name,					false,	string,						IRMonoImagePtr												)
	REMOTEMONO_API(	0,	71,	image_get_filename,				false,	string,						IRMonoImagePtr												)
	REMOTEMONO_API(	0,	72,	image_get_table_info,			false,	IRMonoTableInfoPtr,			IRMonoImagePtr, irmono_int									)
	REMOTEMONO_API(	0,	73,	table_info_get_rows,			false,	irmono_int,					IRMonoTableInfoPtr											)
	REMOTEMONO_API(	0,	74,	image_rva_map,					false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API_SKIP(0, 75, 79)

	REMOTEMONO_API(	0,	80,	metadata_decode_row_col,		false,	uint32_t,					IRMonoTableInfoPtr, irmono_int, irmono_uint					)
	REMOTEMONO_API(	0,	81,	metadata_guid_heap,				false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	0,	82,	metadata_string_heap,			false,	string,						IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	0,	83,	metadata_blob_heap,				false,	irmono_voidp,				IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	0,	84,	metadata_user_string,			false,	string,						IRMonoImagePtr, uint32_t									)
	REMOTEMONO_API(	0,	85,	metadata_decode_blob_size,		false,	uint32_t,					irmono_voidp, ParamOut<irmono_voidp>						)
	REMOTEMONO_API_SKIP(0, 86, 99)

	REMOTEMONO_API(	0,	100,get_object_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	101,get_int16_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	102,get_int32_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	103,get_int64_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	104,get_double_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	105,get_single_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	106,get_string_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	107,get_thread_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	108,get_uint16_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	109,get_uint32_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	110,get_uint64_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	111,get_void_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	112,get_array_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	113,get_boolean_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	114,get_byte_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	115,get_sbyte_class,				false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	116,get_char_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	117,get_exception_class,			false,	IRMonoClassPtr																			)
	REMOTEMONO_API(	0,	118,get_enum_class,					false,	IRMonoClassPtr																			)
	REMOTEMONO_API_SKIP(0, 119, 129)

	REMOTEMONO_API(	0,	130,class_vtable,					false,	IRMonoVTablePtr,			IRMonoDomainPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	0,	131,runtime_class_init,				false,	void,						IRMonoVTablePtr												)
	REMOTEMONO_API(	0,	132,class_get_parent,				false,	IRMonoClassPtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	0,	133,class_get_type,					false,	IRMonoTypePtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	0,	134,class_from_name,				false,	IRMonoClassPtr,				IRMonoImagePtr, string_view, string_view					)
	REMOTEMONO_API(	0,	135,class_from_mono_type,			false,	IRMonoClassPtr,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	136,class_get_name,					false,	string,						IRMonoClassPtr												)
	REMOTEMONO_API(	0,	137,class_get_namespace,			false,	string,						IRMonoClassPtr												)
	REMOTEMONO_API(	0,	138,class_get_fields,				false,	IRMonoClassFieldPtr,		IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	0,	139,class_get_methods,				false,	IRMonoMethodPtr,			IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	0,	140,class_get_properties,			false,	IRMonoPropertyPtr,			IRMonoClassPtr, irmono_voidpp								)
	REMOTEMONO_API(	0,	141,class_get_field_from_name,		false,	IRMonoClassFieldPtr,		IRMonoClassPtr, string_view									)
	REMOTEMONO_API(	0,	142,class_get_method_from_name,		false,	IRMonoMethodPtr,			IRMonoClassPtr, string_view, irmono_int						)
	REMOTEMONO_API(	0,	143,class_get_property_from_name,	false,	IRMonoPropertyPtr,			IRMonoClassPtr, string_view									)
	REMOTEMONO_API(	0,	144,class_get_element_class,		false,	IRMonoClassPtr,				IRMonoClassPtr												)
	REMOTEMONO_API(	0,	145,class_get_flags,				false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	146,class_get_rank,					false,	irmono_int,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	147,class_is_valuetype,				true,	irmono_bool,				IRMonoClassPtr												)
	REMOTEMONO_API(	0,	148,class_data_size,				false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	149,class_instance_size,			false,	uint32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	150,class_value_size,				true,	int32_t,					IRMonoClassPtr, ParamOut<uint32_t>							)
    REMOTEMONO_API(	0,	151,class_get_image,				false,	IRMonoImagePtr,				IRMonoClassPtr                  							)
	REMOTEMONO_API_SKIP(0, 152, 169)

	REMOTEMONO_API(	0,	170,type_get_object,				false,	IRMonoReflectionTypePtr,	IRMonoDomainPtr, IRMonoTypePtr								)
	REMOTEMONO_API(	0,	171,type_get_name,					false,	ReturnOwn<string>,			IRMonoTypePtr												)
	REMOTEMONO_API(	0,	172,type_get_class,					false,	IRMonoClassPtr,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	173,type_get_type,					false,	irmono_int,					IRMonoTypePtr												)
	REMOTEMONO_API(	0,	174,type_is_byref,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	175,type_is_pointer,				false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	176,type_is_reference,				false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	177,type_is_struct,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	178,type_is_void,					false,	irmono_bool,				IRMonoTypePtr												)
	REMOTEMONO_API(	0,	179,type_size,						false,	irmono_int,					IRMonoTypePtr, ParamOut<irmono_int>							)
	REMOTEMONO_API(	0,	180,type_stack_size,				false,	irmono_int,					IRMonoTypePtr, ParamOut<irmono_int>							)
	REMOTEMONO_API_SKIP(0, 181, 199)

	REMOTEMONO_API(	0,	200,field_get_name,					false,	string,						IRMonoClassFieldPtr											)
	REMOTEMONO_API(	0,	201,field_get_flags,				false,	uint32_t,					IRMonoClassFieldPtr											)
	REMOTEMONO_API(	0,	202,field_get_parent,				false,	IRMonoClassPtr,				IRMonoClassFieldPtr											)
	REMOTEMONO_API(	0,	203,field_get_type,					false,	IRMonoTypePtr,				IRMonoClassFieldPtr											)
	REMOTEMONO_API(	0,	204,field_set_value,				false,	void,						IRMonoObjectPtr, IRMonoClassFieldPtr, Variant				)
	REMOTEMONO_API(	0,	205,field_get_value,				false,	void,						IRMonoObjectPtr, IRMonoClassFieldPtr, ParamOut<Variant>		)
	REMOTEMONO_API(	0,	206,field_get_value_object,			false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassFieldPtr, IRMonoObjectPtr,
																								ParamOutRetCls<IRMonoClassPtr>								)
	REMOTEMONO_API(	0,	207,field_static_set_value,			false,	void,						IRMonoVTablePtr, IRMonoClassFieldPtr, Variant				)
	REMOTEMONO_API(	0,	208,field_static_get_value,			false,	void,						IRMonoVTablePtr, IRMonoClassFieldPtr, ParamOut<Variant>		)
	REMOTEMONO_API(	0,	209,field_get_offset,				false,	uint32_t,					IRMonoClassFieldPtr											)
	REMOTEMONO_API_SKIP(0, 210, 229)

	REMOTEMONO_API(	0,	230,method_get_class,				false,	IRMonoClassPtr,				IRMonoMethodPtr												)
	REMOTEMONO_API(	0,	231,method_get_name,				false,	string,						IRMonoMethodPtr												)
	REMOTEMONO_API(	0,	232,method_get_flags,				false,	uint32_t,					IRMonoMethodPtr, ParamOut<uint32_t>							)
	REMOTEMONO_API(	0,	233,method_full_name,				false,	ReturnOwn<string>,			IRMonoMethodPtr, irmono_bool								)
	REMOTEMONO_API(	0,	234,method_signature,				false,	IRMonoMethodSignaturePtr,	IRMonoMethodPtr												)
	REMOTEMONO_API(	0,	235,method_get_header,				false,	IRMonoMethodHeaderPtr,		IRMonoMethodPtr												)
	REMOTEMONO_API(	0,	236,method_header_get_code,			false,	irmono_funcp,				IRMonoMethodHeaderPtr, ParamOut<uint32_t>,
																								ParamOut<uint32_t>											)
	REMOTEMONO_API(	0,	237,method_desc_new,				false,	ReturnOwn<IRMonoMethodDescPtr>,
																								string_view, irmono_bool									)
	REMOTEMONO_API(	0,	238,method_desc_free,				false,	void,						IRMonoMethodDescPtrRaw										)
	REMOTEMONO_API(	0,	239,method_desc_match,				false,	irmono_bool,				IRMonoMethodDescPtr, IRMonoMethodPtr						)
	REMOTEMONO_API(	0,	240,method_desc_search_in_class,	false,	IRMonoMethodPtr,			IRMonoMethodDescPtr, IRMonoClassPtr							)
	REMOTEMONO_API(	0,	241,method_desc_search_in_image,	false,	IRMonoMethodPtr,			IRMonoMethodDescPtr, IRMonoImagePtr							)

	REMOTEMONO_API(	0,	242,property_get_name,				false,	string,						IRMonoPropertyPtr											)
	REMOTEMONO_API(	0,	243,property_get_flags,				false,	uint32_t,					IRMonoPropertyPtr											)
	REMOTEMONO_API(	0,	244,property_get_parent,			false,	IRMonoClassPtr,				IRMonoPropertyPtr											)
	REMOTEMONO_API(	0,	245,property_get_set_method,		false,	IRMonoMethodPtr,			IRMonoPropertyPtr											)
	REMOTEMONO_API(	0,	246,property_get_get_method,		false,	IRMonoMethodPtr,			IRMonoPropertyPtr											)
	REMOTEMONO_API(	0,	247,property_get_value,				false,	IRMonoObjectPtr,			IRMonoPropertyPtr, RMonoVariant,
																								ParamOvwrInOut<ParamOut<VariantArray>>,
																								ParamException<IRMonoExceptionPtr>,
																								ParamOutRetCls<IRMonoClassPtr>								)
	REMOTEMONO_API(	0,	248,property_set_value,				false,	void,						IRMonoPropertyPtr, RMonoVariant,
																								ParamOvwrInOut<VariantArray>,
																								ParamException<IRMonoExceptionPtr>							)
	REMOTEMONO_API_SKIP(0, 249, 259)

	REMOTEMONO_API(	0,	260,signature_get_return_type,		false,	IRMonoTypePtr,				IRMonoMethodSignaturePtr									)
	REMOTEMONO_API(	0,	261,signature_get_params,			false,	IRMonoTypePtr,				IRMonoMethodSignaturePtr, irmono_voidpp						)
	REMOTEMONO_API(	0,	262,signature_get_call_conv,		false,	uint32_t,					IRMonoMethodSignaturePtr									)
	REMOTEMONO_API(	0,	263,signature_get_desc,				false,	ReturnOwn<string>,			IRMonoMethodSignaturePtr, irmono_bool						)
	REMOTEMONO_API_SKIP(0, 264, 269)

	REMOTEMONO_API(	0,	270,object_get_class,				true,	IRMonoClassPtr,				IRMonoObjectPtr												)
	REMOTEMONO_API(	0,	271,object_new,						false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	0,	272,runtime_object_init,			false,	void,						Variant														)
	REMOTEMONO_API(	0,	273,object_unbox,					true,	Variant,					IRMonoObjectPtr												)
	REMOTEMONO_API(	0,	274,value_box,						false,	IRMonoObjectPtr,			IRMonoDomainPtr, IRMonoClassPtr, RMonoVariant				)
	REMOTEMONO_API(	0,	275,object_to_string,				false,	IRMonoStringPtr,			Variant, ParamException<IRMonoExceptionPtr>					)
	REMOTEMONO_API(	0,	276,object_clone,					false,	IRMonoObjectPtr,			IRMonoObjectPtr												)
	REMOTEMONO_API(	0,	277,object_get_domain,				false,	IRMonoDomainPtr,			IRMonoObjectPtr												)
	REMOTEMONO_API(	0,	278,object_get_virtual_method,		false,	IRMonoMethodPtr,			IRMonoObjectPtr, IRMonoMethodPtr							)
	REMOTEMONO_API(	0,	279,object_isinst,					false,	IRMonoObjectPtr,			IRMonoObjectPtr, IRMonoClassPtr								)
	REMOTEMONO_API(	0,	280,object_get_size,				false,	irmono_uint,				IRMonoObjectPtr												)
	REMOTEMONO_API_SKIP(0, 281, 299)

	REMOTEMONO_API(	0,	300,string_new,						false,	IRMonoStringPtr,			IRMonoDomainPtr, string_view								)
	REMOTEMONO_API(	0,	301,string_new_len,					false,	IRMonoStringPtr,			IRMonoDomainPtr, string_view, irmono_uint					)
	REMOTEMONO_API(	0,	302,string_new_utf16,				false,	IRMonoStringPtr,			IRMonoDomainPtr, u16string_view, int32_t					)
	REMOTEMONO_API(	0,	303,string_new_utf32,				false,	IRMonoStringPtr,			IRMonoDomainPtr, u32string_view, int32_t					)
	REMOTEMONO_API(	0,	304,string_to_utf8,					false,	ReturnOwn<string>,			IRMonoStringPtr												)
	REMOTEMONO_API(	0,	305,string_to_utf16,				false,	ReturnOwn<u16string>,		IRMonoStringPtr												)
	REMOTEMONO_API(	0,	306,string_to_utf32,				false,	ReturnOwn<u32string>,		IRMonoStringPtr												)
	REMOTEMONO_API(	0,	307,string_chars,					false,	u16string,					IRMonoStringPtr												)
	REMOTEMONO_API(	0,	308,string_length,					false,	irmono_int,					IRMonoStringPtr												)
	REMOTEMONO_API(	0,	309,string_equal,					false,	irmono_bool,				IRMonoStringPtr, IRMonoStringPtr							)
	REMOTEMONO_API_SKIP(0, 310, 319)

	REMOTEMONO_API(	0,	320,array_new,						false,	IRMonoArrayPtr,				IRMonoDomainPtr, IRMonoClassPtr, irmono_uintptr_t			)
	REMOTEMONO_API(	0,	321,array_new_full,					false,	IRMonoArrayPtr,				IRMonoDomainPtr, IRMonoClassPtr, irmono_voidp, irmono_voidp	)
	REMOTEMONO_API(	0,	322,array_class_get,				false,	IRMonoClassPtr,				IRMonoClassPtr, uint32_t									)
	REMOTEMONO_API(	0,	323,array_addr_with_size,			false,	Variant,					IRMonoArrayPtr, irmono_int, irmono_uintptr_t				)
	REMOTEMONO_API(	0,	324,array_length,					false,	irmono_uintptr_t,			IRMonoArrayPtr												)
	REMOTEMONO_API(	0,	325,array_element_size,				false,	int32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	326,class_array_element_size,		false,	int32_t,					IRMonoClassPtr												)
	REMOTEMONO_API(	0,	327,array_clone,					false,	IRMonoArrayPtr,				IRMonoArrayPtr												)
	REMOTEMONO_API_SKIP(0, 328, 339)

	REMOTEMONO_API(	0,	340,gchandle_new,					true,	irmono_gchandle,			IRMonoObjectPtr, irmono_bool								)
	REMOTEMONO_API(	0,	341,gchandle_new_weakref,			false,	irmono_gchandle,			IRMonoObjectPtr, irmono_bool								)
	REMOTEMONO_API(	0,	342,gchandle_get_target,			true,	IRMonoObjectPtrRaw,			irmono_gchandle												)
	REMOTEMONO_API(	0,	343,gchandle_free,					true,	void,						irmono_gchandle												)
	REMOTEMONO_API_SKIP(0, 344, 349)

	REMOTEMONO_API(	0,	350,gc_collect,						false,	void,						irmono_int													)
	REMOTEMONO_API(	0,	351,gc_max_generation,				false,	irmono_int																				)
	REMOTEMONO_API(	0,	352,gc_get_generation,				false,	irmono_int,					IRMonoObjectPtr												)
	REMOTEMONO_API(	0,	353,gc_wbarrier_set_arrayref,		false,	void,						IRMonoArrayPtr, irmono_voidp, IRMonoObjectPtr				)
	REMOTEMONO_API_SKIP(0, 354, 359)

	REMOTEMONO_API(	0,	360,runtime_invoke,					false,	IRMonoObjectPtr,			IRMonoMethodPtr, Variant,
																								ParamOvwrInOut<VariantArray>,
																								ParamException<IRMonoExceptionPtr>,
																								ParamOutRetCls<IRMonoClassPtr>								)
	REMOTEMONO_API(	0,	361,compile_method,					false,	irmono_voidp,				IRMonoMethodPtr												)
	REMOTEMONO_API_SKIP(0, 362, 369)

	REMOTEMONO_API(	0,	370,jit_info_table_find,			false,	IRMonoJitInfoPtr,			IRMonoDomainPtr, irmono_voidp								)
	REMOTEMONO_API(	0,	371,jit_info_get_code_start,		false,	irmono_funcp,				IRMonoJitInfoPtr											)
	REMOTEMONO_API(	0,	372,jit_info_get_code_size,			false,	irmono_int,					IRMonoJitInfoPtr											)
	REMOTEMONO_API(	0,	373,jit_info_get_method,			false,	IRMonoMethodPtr,			IRMonoJitInfoPtr											)
	REMOTEMONO_API_SKIP(0, 374, 379)

	REMOTEMONO_API(	0,	380,disasm_code,					false,	ReturnOwn<string>,			IRMonoDisHelperPtr, IRMonoMethodPtr,
																								irmono_cbytep, irmono_cbytep								)
	REMOTEMONO_API(	0,	381,pmip,							false,	ReturnOwn<string>,			irmono_voidp												)
	REMOTEMONO_API_SKIP(0, 382, 399)

	REMOTEMONO_API_END(MonoAPI, 0, 399)







	// **********************************************************
	// *														*
	// *					MISC MONO API						*
	// *														*
	// **********************************************************

	REMOTEMONO_API_BEGIN(MiscAPI, 1)

	//						FUNCTION						REQD	RETURN						ARGUMENTS

	REMOTEMONO_API(	1,	1,	g_free,							false,	void,						irmono_voidp												)
	REMOTEMONO_API_SKIP(1, 2, 19)

	REMOTEMONO_API_END(MiscAPI, 1, 19)






	// **********************************************************
	// *														*
	// *					BOILERPLATE API						*
	// *														*
	// **********************************************************

	// These functions are RemoteMono additions. They are created by assembleBoilerplateCode() and then injected into the remote process.

	REMOTEMONO_API_BEGIN(BoilerplateAPI, 2)

	//								FUNCTION						RETURN						ARGUMENTS

	REMOTEMONO_API_SIMPLE(	2,	1,	rmono_foreach_ipcvec_adapter,	void,						irmono_voidp, irmono_voidp									)
	REMOTEMONO_API_SIMPLE(	2,	2,	rmono_gchandle_pin,				irmono_gchandle,			irmono_gchandle												)
	REMOTEMONO_API_SIMPLE(	2,	3,	rmono_array_setref,				void,						irmono_gchandle, irmono_uintptr_t, irmono_gchandle			)
	REMOTEMONO_API_SIMPLE(	2,	4,	rmono_array_slice,				irmono_uintptr_t,			irmono_voidp, irmono_gchandle,
																								irmono_uintptr_t, irmono_uintptr_t, uint32_t				)
	REMOTEMONO_API_SIMPLE(	2,	5,	rmono_gchandle_free_multi,		void,						irmono_voidp, irmono_voidp									)
	REMOTEMONO_API_SIMPLE(	2,	6,	rmono_raw_free_multi,			void,						irmono_voidp, irmono_voidp									)
	REMOTEMONO_API_SKIP(2, 7, 19)

	REMOTEMONO_API_END(BoilerplateAPI, 2, 19)
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

	void setGchandleFreeBufferMaxCount(uint32_t maxCount);
	void setRawFreeBufferMaxCount(uint32_t maxCount);
	void setFreeBufferMaxCount(uint32_t maxCount);

	void freeLaterGchandle(irmono_gchandle handle);
	void freeLaterRaw(irmono_voidp ptr);

	void flushGchandleFreeBuffer();
	void flushRawFreeBuffer();
	void flushFreeBuffers();

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

	irmono_gchandle gchandleFreeBuf[REMOTEMONO_GCHANDLE_FREE_BUF_SIZE_MAX];
	irmono_voidp rawFreeBuf[REMOTEMONO_RAW_FREE_BUF_SIZE_MAX];

	uint32_t gchandleFreeBufCount;
	uint32_t rawFreeBufCount;

	uint32_t gchandleFreeBufCountMax;
	uint32_t rawFreeBufCountMax;
};


}
