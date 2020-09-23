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

#include <type_traits>
#include "RMonoTypes.h"
#include "RMonoHandle_Def.h"
#include "RMonoVariant_Def.h"
#include "RMonoVariantArray_Def.h"
#include "abi/RMonoABITypeTraits.h"
#include "../util.h"



#define REMOTEMONO_DEFINE_PARAM_TAG(tag)																								\
		struct Param ## tag ## Tag {};																									\
																																		\
		template <typename TypeT>																										\
		struct Param ## tag :																											\
				public Param ## tag ## Tag,																								\
				public std::conditional_t<std::is_base_of_v<ParamBase, TypeT>, TypeT, ParamBase>										\
		{																																\
			typedef typename std::conditional_t<std::is_base_of_v<ParamBase, TypeT>, TypeT, Identity<TypeT>>::Type Type;				\
		};
#define REMOTEMONO_DEFINE_PARAM_TAG_INHERIT1(tag, parent)																				\
		struct Param ## tag ## Tag {};																									\
																																		\
		template <typename TypeT>																										\
		struct Param ## tag :																											\
				public Param ## tag ## Tag,																								\
				public Param ## parent <TypeT>																							\
		{																																\
			typedef typename Param ## parent <TypeT>::Type Type;																		\
		};
#define REMOTEMONO_DEFINE_PARAM_TAG_INHERIT2(tag, parent1, parent2)																		\
		struct Param ## tag ## Tag {};																									\
																																		\
		template <typename TypeT>																										\
		struct Param ## tag :																											\
				public Param ## tag ## Tag,																								\
				public Param ## parent1 <Param ## parent2 <TypeT>>																		\
		{																																\
			typedef typename Param ## parent1 <Param ## parent2 <TypeT>>::Type Type;													\
		};

#define REMOTEMONO_DEFINE_RETURN_TAG(tag)																								\
		struct Return ## tag ## Tag {};																									\
																																		\
		template <typename TypeT>																										\
		struct Return ## tag :																											\
				public Return ## tag ## Tag,																							\
				public std::conditional_t<std::is_base_of_v<ReturnBase, TypeT>, TypeT, ReturnBase>										\
		{																																\
			typedef typename std::conditional_t<std::is_base_of_v<ReturnBase, TypeT>, TypeT, Identity<TypeT>>::Type Type;				\
		};



namespace remotemono
{


namespace tags
{

struct ParamBase {};
struct ReturnBase {};


template <class ParamT, class TagT>
using has_param_tag = std::is_base_of<TagT, ParamT>;

template <class ParamT, class TagT>
inline constexpr bool has_param_tag_v = has_param_tag<ParamT, TagT>::value;


template <class RetT, class TagT>
using has_return_tag = std::is_base_of<TagT, RetT>;

template <class RetT, class TagT>
inline constexpr bool has_return_tag_v = has_return_tag<RetT, TagT>::value;


REMOTEMONO_DEFINE_PARAM_TAG(Null)
REMOTEMONO_DEFINE_PARAM_TAG(Out)
REMOTEMONO_DEFINE_PARAM_TAG_INHERIT1(InOut, Out)
REMOTEMONO_DEFINE_PARAM_TAG_INHERIT1(Exception, Out)
REMOTEMONO_DEFINE_PARAM_TAG(OvwrInOut)
REMOTEMONO_DEFINE_PARAM_TAG(Own)

REMOTEMONO_DEFINE_RETURN_TAG(Null)
REMOTEMONO_DEFINE_RETURN_TAG(Own)

};





template <class CommonT>
struct RMonoAPIFunctionCommonTraits;

template <class CommonT>
struct RMonoAPIFunctionRawTraits;

template <class CommonT>
struct RMonoAPIFunctionWrapTraits;

template <class CommonT>
struct RMonoAPIFunctionAPITraits;








// ********** Default (e.g. input value types) **********

template <typename ABI, typename DefTypeT, typename Enable = void>
struct RMonoAPIParamTypeAdapter
{
	typedef typename DefTypeT::Type DefType;
	typedef typename DefTypeT::Type APIType;
	typedef typename DefTypeT::Type RawType;
	typedef typename DefTypeT::Type WrapType;
};

template <typename ABI, typename DefTypeT, typename Enable = void>
struct RMonoAPIReturnTypeAdapter
{
	typedef typename DefTypeT::Type DefType;
	typedef typename DefTypeT::Type APIType;
	typedef typename DefTypeT::Type RawType;
	typedef typename DefTypeT::Type WrapType;
};




// ********** void **********

template <typename ABI>
struct RMonoAPIReturnTypeAdapter<ABI, void, void>
{
	typedef void DefType;
	typedef void APIType;
	typedef void RawType;
	typedef void WrapType;
};




// ********** RMonoHandle **********

// Input
template <typename ABI, typename TaggedHandleT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		TaggedHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoHandleTag, typename TaggedHandleT::Type>
			&&	! std::is_base_of_v<RMonoObjectHandleTag, typename TaggedHandleT::Type>
			&&	! tags::has_param_tag_v<TaggedHandleT, tags::ParamOutTag>
			>
		>
{
	typedef TaggedHandleT DefType;
	typedef typename TaggedHandleT::Type APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};

// Output
template <typename ABI, typename TaggedHandleT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		TaggedHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoHandleTag, typename TaggedHandleT::Type>
			&&	! std::is_base_of_v<RMonoObjectHandleTag, typename TaggedHandleT::Type>
			&&	tags::has_param_tag_v<TaggedHandleT, tags::ParamOutTag>
			>
		>
{
	typedef TaggedHandleT DefType;
	typedef typename TaggedHandleT::Type* APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidpp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidpp WrapType;
};

// Return
template <typename ABI, typename TaggedHandleT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoHandleTag, typename TaggedHandleT::Type>
			&&	! std::is_base_of_v<RMonoObjectHandleTag, typename TaggedHandleT::Type>
			>
		>
{
	typedef TaggedHandleT DefType;
	typedef typename TaggedHandleT::Type APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};




// ********** RemoteMonoObjectHandle (Input) **********

template <typename ABI, typename TaggedObjectHandleT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		TaggedObjectHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoObjectHandleTag, typename TaggedObjectHandleT::Type>
			&&	! tags::has_param_tag_v<TaggedObjectHandleT, tags::ParamOutTag>
			>
		>
{
	typedef TaggedObjectHandleT DefType;
	typedef typename TaggedObjectHandleT::Type APIType;
	typedef typename RMonoABITypeTraits<ABI>::IRMonoObjectPtrRaw RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_gchandle WrapType;
};




// ********** RemoteMonoObjectHandle (Output / Return) **********

template <typename ABI, typename TaggedObjectHandleT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		TaggedObjectHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoObjectHandleTag, typename TaggedObjectHandleT::Type>
			&&	tags::has_param_tag_v<TaggedObjectHandleT, tags::ParamOutTag>
			&&	! tags::has_param_tag_v<TaggedObjectHandleT, tags::ParamExceptionTag>
			>
		>
{
	typedef TaggedObjectHandleT DefType;
	typedef typename TaggedObjectHandleT::Type* APIType;
	typedef typename RMonoABITypeTraits<ABI>::IRMonoObjectPtrPtrRaw RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};

template <typename ABI, typename TaggedObjectHandleT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedObjectHandleT,
		std::enable_if_t<std::is_base_of_v<RMonoObjectHandleTag, typename TaggedObjectHandleT::Type>>
		>
{
	typedef TaggedObjectHandleT DefType;
	typedef typename TaggedObjectHandleT::Type APIType;
	typedef typename RMonoABITypeTraits<ABI>::IRMonoObjectPtrRaw RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_gchandle WrapType;
};




// ********** Exceptions (Output) **********

template <typename ABI, typename TaggedObjectHandleT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		TaggedObjectHandleT,
		std::enable_if_t <
				std::is_base_of_v<RMonoObjectHandleTag, typename TaggedObjectHandleT::Type>
			&&	tags::has_param_tag_v<TaggedObjectHandleT, tags::ParamExceptionTag>
			>
		>
{
	typedef TaggedObjectHandleT DefType;
	typedef bool APIType;
	typedef typename RMonoABITypeTraits<ABI>::IRMonoExceptionPtrPtrRaw RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};




// ********** Variants **********

// Input
template <typename ABI, typename VariantT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		VariantT,
		std::enable_if_t <
				std::is_base_of_v<RMonoVariant, typename VariantT::Type>
			&&	! tags::has_param_tag_v<VariantT, tags::ParamOutTag>
			>
		>
{
	typedef VariantT DefType;
	typedef const RMonoVariant& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};

// Output
template <typename ABI, typename VariantT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		VariantT,
		std::enable_if_t <
				std::is_base_of_v<RMonoVariant, typename VariantT::Type>
			&&	tags::has_param_tag_v<VariantT, tags::ParamOutTag>
			>
		>
{
	typedef VariantT DefType;
	typedef RMonoVariant& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};

// Return (NOTE: overridden by custom behavior for API & Wrap functions)
template <typename ABI, typename VariantT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		VariantT,
		std::enable_if_t<std::is_base_of_v<RMonoVariant, typename VariantT::Type>>
		>
{
	typedef VariantT DefType;
	typedef void APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef void WrapType;
};




// ********** Variant Arrays **********

// Input
template <typename ABI, typename VariantArrT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		VariantArrT,
		std::enable_if_t <
				std::is_base_of_v<RMonoVariantArray, typename VariantArrT::Type>
			&&	! tags::has_param_tag_v<VariantArrT, tags::ParamOutTag>
			&&	! tags::has_param_tag_v<VariantArrT, tags::ParamOvwrInOutTag>
			>
		>
{
	typedef VariantArrT DefType;
	typedef const RMonoVariantArray& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidpp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};

// Output
template <typename ABI, typename VariantArrT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		VariantArrT,
		std::enable_if_t <
				std::is_base_of_v<RMonoVariantArray, typename VariantArrT::Type>
			&&	(	tags::has_param_tag_v<VariantArrT, tags::ParamOutTag>
				||	tags::has_param_tag_v<VariantArrT, tags::ParamOvwrInOutTag>
				)
			>
		>
{
	typedef VariantArrT DefType;
	typedef RMonoVariantArray& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidpp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};




// ********** Strings (Input) **********

// ASCII / UTF-8
template <typename ABI, typename StringViewT>
struct RMonoAPIParamTypeAdapter <
			ABI,
			StringViewT,
			std::enable_if_t<std::is_base_of_v<std::string_view, typename StringViewT::Type>>
			>
{
	typedef std::string_view DefType;
	typedef const std::string_view& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_ccharp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_ccharp WrapType;
};

// UTF-16
template <typename ABI, typename StringViewT>
struct RMonoAPIParamTypeAdapter <
			ABI,
			StringViewT,
			std::enable_if_t<std::is_base_of_v<std::u16string_view, typename StringViewT::Type>>
			>
{
	typedef std::u16string_view DefType;
	typedef const std::u16string_view& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar2p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar2p WrapType;
};

// UTF-32
template <typename ABI, typename StringViewT>
struct RMonoAPIParamTypeAdapter <
			ABI,
			StringViewT,
			std::enable_if_t<std::is_base_of_v<std::u32string_view, typename StringViewT::Type>>
			>
{
	typedef std::u32string_view DefType;
	typedef const std::u32string_view& APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar4p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar4p WrapType;
};




// ********** Strings (Return, Non-Owning) **********

// ASCII / UTF-8
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::string, typename TaggedStringT::Type>
			&&	! tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_ccharp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_ccharp WrapType;
};

// UTF-16
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::u16string, typename TaggedStringT::Type>
			&&	! tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::u16string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar2p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar2p WrapType;
};

// UTF-32
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::u32string, typename TaggedStringT::Type>
			&&	! tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::u32string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar4p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_cunichar4p WrapType;
};




// ********** Strings (Return, Owning) **********

// ASCII / UTF-8
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::string, typename TaggedStringT::Type>
			&&	tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_charp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_charp WrapType;
};

// UTF-16
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::u16string, typename TaggedStringT::Type>
			&&	tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::u16string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_unichar2p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_unichar2p WrapType;
};

// UTF-32
template <typename ABI, typename TaggedStringT>
struct RMonoAPIReturnTypeAdapter <
		ABI,
		TaggedStringT,
		std::enable_if_t <
				std::is_base_of_v<std::u32string, typename TaggedStringT::Type>
			&&	tags::has_return_tag_v<TaggedStringT, tags::ReturnOwnTag>
			>
		>
{
	typedef TaggedStringT DefType;
	typedef std::u32string APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_unichar4p RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_unichar4p WrapType;
};




// ********** Value Type (Output) **********

template <typename ABI, typename ValT>
struct RMonoAPIParamTypeAdapter <
		ABI,
		ValT,
		std::enable_if_t <
				tags::has_param_tag_v<ValT, tags::ParamOutTag>
			&&	std::is_fundamental_v<typename ValT::Type>
			>
		>
{
	typedef ValT DefType;
	typedef typename ValT::Type* APIType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp RawType;
	typedef typename RMonoABITypeTraits<ABI>::irmono_voidp WrapType;
};


}
