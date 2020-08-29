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

#include <cstdlib>
#include <stdint.h>
#include <string>
#include "RMonoTypes.h"
#include "RMonoHandle_Def.h"



namespace remotemono
{



class RMonoAPIBase;




/**
 * A special class that can encapsulate any instance of a Mono/.NET reference or value type. This means it can
 * hold any of the following:
 *
 * * A value of a fundamental type like `int`, `short`, `float`, `char`, `bool` etc.
 * * An instance of a reference type, i.e. a MonoObject* (in C# terms: an object of a class derived from System.Object).
 * * An instance of a custom value type (in C# terms: an instance of a struct derived from System.ValueType).
 *
 * This type is mostly used in certain places where the raw Mono API has a `void*` parameter or return value referring
 * to a managed value, e.g. the value in `mono_field_set_value()` or `mono_object_unbox()`. It is also used in functions like
 * `mono_runtime_invoke()` for the method parameters (as an element of a RMonoVariantArray).
 *
 * This class is necessary because simply keeping the raw `void*` type is impractical for two reasons:
 *
 * 1. For (custom or fundamental) value types, an additional copy operation to or from the remote memory would have
 *    to be made otherwise.
 * 2. For reference types (i.e. MonoObject*), we keep `rmono_gchandle`s instead of the raw pointers to be on the
 *    safe side of the Mono GC. Because we don't want to pin the GC handles, we *must* actually pass the GC handle itself
 *    to the remote process and only convert back to the raw pointer in the remote process. To do this automatically,
 *    RemoteMono's generated wrapper functions need the type information provided by this class.
 *
 * This documentation may reference C# types, but it is not actually specific to C# remotes.
 */
class RMonoVariant
{
public:
	typedef RMonoVariant Self;

public:
	enum Type
	{
		TypeInvalid,

		TypeInt8,
		TypeUInt8,
		TypeInt16,
		TypeUInt16,
		TypeInt32,
		TypeUInt32,
		TypeInt64,
		TypeUInt64,
		TypeFloat,
		TypeDouble,
		TypeBool,

		/**
		 * A value type instance that is kept in local memory, and always copied over on-demand when a Mono API function
		 * is invoked. Note that changes done by the Mono API on such instances will become lost.
		 */
		TypeCustomValueCopy,

		/**
		 * A remote pointer to a value type instance that is kept in remote memory (e.g. in a boxed object). This can
		 * be used if you want changes made by the Mono API to remain visible.
		 */
		TypeCustomValueRef,

		/**
		 * An instance of a reference type, i.e. the GC handle of a MonoObject*.
		 */
		TypeMonoObjectPtr
	};

	enum Flags
	{
		/**
		 * Flag for output values. If this flag is set, the new value will be read back from the remote process after a
		 * Mono API function is called. Used for output parameters like in `mono_field_get_value()`.
		 */
		FlagOut = 0x01
	};


	/**
	 * A constructor disambiguation tag used to create variants of type TypeCustomValueRef.
	 */
	static inline const struct CustomValueRef {} customValueRef = {};

public:
	/**
	 * Creates an invalid variant, passed ass a NULL `void*`.
	 */
	RMonoVariant() : type(TypeInvalid) {}

	/**
	 * Copy constructor.
	 */
	RMonoVariant(const Self& other)
	{
		if (other.type == TypeMonoObjectPtr  &&  (other.flags & FlagOut) == 0) {
			o = other.o;
			type = other.type;
			flags = other.flags;
		} else {
			memcpy(this, &other, sizeof(Self));
		}
	}
	~RMonoVariant() {}

	/**
	 * Creates a C# `sbyte` value.
	 */
	RMonoVariant(int8_t v) : i8(v), type(TypeInt8), flags(0) {}

	/**
	 * Creates a C# `byte` value.
	 */
	RMonoVariant(uint8_t v) : u8(v), type(TypeUInt8), flags(0) {}

	/**
	 * Creates a C# `short` value.
	 */
	RMonoVariant(int16_t v) : i16(v), type(TypeInt16), flags(0) {}

	/**
	 * Creates a C# `ushort` value.
	 */
	RMonoVariant(uint16_t v) : u16(v), type(TypeUInt16), flags(0) {}

	/**
	 * Creates a C# `int` value.
	 */
	RMonoVariant(int32_t v) : i32(v), type(TypeInt32), flags(0) {}

	/**
	 * Creates a C# `uint` value
	 */
	RMonoVariant(uint32_t v) : u32(v), type(TypeUInt32), flags(0) {}

	/**
	 * Creates a C# `long` value.
	 */
	RMonoVariant(int64_t v) : i64(v), type(TypeInt64), flags(0) {}

	/**
	 * Creates a C# `ulong` value.
	 */
	RMonoVariant(uint64_t v) : u64(v), type(TypeUInt64), flags(0) {}

	/**
	 * Creates a C# `float`/`single` value.
	 */
	RMonoVariant(float v) : f(v), type(TypeFloat), flags(0) {}

	/**
	 * Creates a C# `double` value.
	 */
	RMonoVariant(double v) : d(v), type(TypeDouble), flags(0) {}

	/**
	 * Creates a C# `bool` value.
	 */
	RMonoVariant(bool v) : b(v), type(TypeBool), flags(0) {}

	/**
	 * Creates an input MonoObject* value, i.e. an object of a class derived from System.Object.
	 */
	RMonoVariant(const RMonoObjectPtr& v) : o(v), type(TypeMonoObjectPtr), flags(0) {}

	/**
	 * Creates an input MonoObject* value, i.e. an object of a class derived from System.Object.
	 */
	RMonoVariant(RMonoObjectPtr&& v) : o(v), type(TypeMonoObjectPtr), flags(0) {}

	/**
	 * Creates a `null` MonoObject* value.
	 */
	RMonoVariant(std::nullptr_t) : o(nullptr), type(TypeMonoObjectPtr), flags(0) {}


	/**
	 * Creates an output C# `sbyte` value.
	 */
	RMonoVariant(int8_t* vp) : i8p(vp), type(TypeInt8), flags(FlagOut) {}

	/**
	 * Creates an output C# `byte` value.
	 */
	RMonoVariant(uint8_t* vp) : u8p(vp), type(TypeUInt8), flags(FlagOut) {}

	/**
	 * Creates an output C# `short` value.
	 */
	RMonoVariant(int16_t* vp) : i16p(vp), type(TypeInt16), flags(FlagOut) {}

	/**
	 * Creates an output C# `ushort` value.
	 */
	RMonoVariant(uint16_t* vp) : u16p(vp), type(TypeUInt16), flags(FlagOut) {}

	/**
	 * Creates an output C# `int` value.
	 */
	RMonoVariant(int32_t* vp) : i32p(vp), type(TypeInt32), flags(FlagOut) {}

	/**
	 * Creates an output C# `uint` value.
	 */
	RMonoVariant(uint32_t* vp) : u32p(vp), type(TypeUInt32), flags(FlagOut) {}

	/**
	 * Creates an output C# `long` value.
	 */
	RMonoVariant(int64_t* vp) : i64p(vp), type(TypeInt64), flags(FlagOut) {}

	/**
	 * Creates an output C# `ulong` value.
	 */
	RMonoVariant(uint64_t* vp) : u64p(vp), type(TypeUInt64), flags(FlagOut) {}

	/**
	 * Creates an output C# `float`/`single` value.
	 */
	RMonoVariant(float* vp) : fp(vp), type(TypeFloat), flags(FlagOut) {}

	/**
	 * Creates an output C# `double` value.
	 */
	RMonoVariant(double* vp) : dp(vp), type(TypeDouble), flags(FlagOut) {}

	/**
	 * Creates an output C# `bool` value.
	 */
	RMonoVariant(bool* vp) : bp(vp), type(TypeBool), flags(FlagOut) {}

	/**
	 * Creates an output MonoObject* value, i.e. an object of a class derived from System.Object.
	 */
	RMonoVariant(RMonoObjectPtr* vp) : op(vp), type(TypeMonoObjectPtr), flags(FlagOut) {}


	/**
	 * Creates an object of a (custom) value type that keeps its memory in the local process and copies to the remote
	 * on-demand.
	 *
	 * @param p Pointer to the local memory holding the value. Must remain valid as long as the RMonoVariant object is used.
	 * @param size Size of the value type in bytes.
	 * @param out true if you want an output value (i.e. the new value is copied back to the local process after a Mono API
	 * 		function is called), false otherwise.
	 */
	RMonoVariant(void* p, size_t size, bool out = false) : cvc({p, size}), type(TypeCustomValueCopy), flags(out ? FlagOut : 0) {}

	/**
	 * Creates an variant that references an existing value type object in the remote memory. The pointer is passed directly
	 * to any Mono API functions and nothing is copied back to local memory.
	 *
	 * Use RMonoVariant::customValueRef for the second parameter. It is just a constructor overload disambiguation tag.
	 */
	explicit RMonoVariant(rmono_voidp v, CustomValueRef) : cvr(v), type(TypeCustomValueRef), flags(0) {}

	/**
	 * Creates an variant that references an existing value type object in the remote memory. The pointer is passed directly
	 * to any Mono API functions and nothing is copied back to local memory. This version creates an output variant, which
	 * is currently no different an input variant for TypeCustomValueRef, but certain parts of RemoteMono may assert if an
	 * input variant is used where an output is expected.
	 *
	 * Use RMonoVariant::customValueRef for the second parameter. It is just a constructor overload disambiguation tag.
	 */
	explicit RMonoVariant(rmono_voidp* v, CustomValueRef) : cvrp(v), type(TypeCustomValueRef), flags(FlagOut) {}


	/**
	 * Determine if this is a valid variant. Note that things like null pointers are still considered valid. Only variants
	 * of type TypeInvalid are considered invalid.
	 */
	bool isValid() const { return type != TypeInvalid; }


	/**
	 * Return this variant's type.
	 */
	Type getType() const { return type; }

	/**
	 * Return this variant's flags.
	 */
	uint8_t getFlags() const { return flags; }


	/**
	 * Determine if this variant is a null pointer. Note that this can only be true for output value type variants, or
	 * for input custom value type variants. A variant of type TypeMonoObjectPtr that is null will return false.
	 *
	 * @see isNullValue()
	 */
	inline bool isNullPointer() const;

	/**
	 * Determine if this variant represents a null value. All null pointers are considered null values, as determined
	 * by isNullPointer(), but variants of type TypeMonoObjectPtr that are null will also return true here.
	 */
	inline bool isNullValue() const;


	/**
	 * Return the underlying RMonoObjectPtr. You may only call this if the variant is of type TypeMonoObjectPtr.
	 */
	RMonoObjectPtr getMonoObjectPtr() const { assert(type == TypeMonoObjectPtr); return ((flags & FlagOut) != 0) ? (op ? *op : nullptr) : o; }

	/**
	 * Return the underlying raw remote pointer for the custom value type. You may only call this if the variant is of
	 * type TypeCustomValueRef. In particular, you **may not** call it for TypeCustomValueCopy.
	 */
	rmono_voidp getCustomValueRef() const { assert(type == TypeCustomValueRef); return ((flags & FlagOut) != 0) ? (cvrp ? *cvrp : 0) : cvr; }


	/**
	 * Get the number of bytes required to hold the this variant's value in the remote process.
	 *
	 * @param abi The ABI to use for determining the size.
	 * @return The value size in bytes.
	 */
	template <typename ABI>
	size_t getRemoteMemorySize(ABI& abi) const;

	/**
	 * Copy this variant's value to a buffer that can then be transferred to remote memory. The buffer must have at least
	 * `getRemoteMemorySIze(abi)` bytes of free memory.
	 *
	 * @param abi The ABI to use for converting the value.
	 * @param buf The buffer to copy the value into.
	 */
	template <typename ABI>
	void copyForRemoteMemory(ABI& abi, void* buf) const;

	/**
	 * Updates this variant's value from the given buffer as it may be obtained from remote memory.
	 *
	 * @param abi The ABI to use for converting the value.
	 * @param mono The Mono API to be used, e.g. for creating RMonoObjectHandle instances.
	 * @param buf The buffer to read the raw data from.
	 */
	template <typename ABI>
	void updateFromRemoteMemory(ABI& abi, RMonoAPIBase& mono, void* buf);

private:
	union
	{
		int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f;
		double d;
		bool b;
		struct {
			void* p;
			size_t size;
		} cvc;
		rmono_voidp cvr;

		int8_t* i8p;
		uint8_t* u8p;
		int16_t* i16p;
		uint16_t* u16p;
		int32_t* i32p;
		uint32_t* u32p;
		int64_t* i64p;
		uint64_t* u64p;
		float* fp;
		double* dp;
		bool* bp;
		RMonoObjectPtr* op;
		rmono_voidp* cvrp;
	};

	RMonoObjectPtr o;

	Type type;
	uint8_t flags;
};


};





#include "RMonoVariant_Impl.h"
