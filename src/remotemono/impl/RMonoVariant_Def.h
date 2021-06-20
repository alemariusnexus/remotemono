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
 * * A value of a built-in type like `int`, `short`, `float`, `char`, `bool` etc.
 * * An instance of a custom value type (in C# terms: an instance of a struct derived from System.ValueType).
 * * An instance of a reference type, i.e. a MonoObject* (in C# terms: an object of a class derived from System.Object).
 *
 * This type is mostly used in certain places where the raw Mono API has a `void*` parameter or return value referring
 * to a managed value, e.g. the value in `mono_field_set_value()` or `mono_object_unbox()`. It is also used in functions like
 * `mono_runtime_invoke()` for the method parameters (as an element of a RMonoVariantArray).
 *
 * We use object of this class instead of simple `void*` values for multiple reasons:
 *
 * 1. It contains info about what kind of type is stored in it (value type, reference type or raw pointer). This is
 *    useful e.g. for reference types (i.e. MonoObject*): Here we keep `rmono_gchandle`s instead of the raw pointers to be
 *    on the safe side of the Mono GC. Because we don't want to pin the GC handles, we *must* actually pass the GC handle
 *    itself to the remote process and convert back to the raw pointer just-in-time. To do this, RemoteMono's generated
 *    wrapper functions need to know if the value it receives is such a GC handle or not.
 * 2. For value types, it stores them together with info about their size. We need that when copying the values to and from
 *    remote memory.
 * 3. It contains additional metadata necessary for calling certain Mono API functions. E.g. for mono_runtime_invoke(), we
 *    need to know if each parameter is an input, output, or both. That's because mono_runtime_invoke() needs parameters
 *    (specifically reference types) to be passed differently depending on this directionality. This class can provide this
 *    information (see #forDirection()).
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
		TypeInvalid = 0,

		/**
		 * A value type instance that is kept in local memory, and always copied over on-demand when a Mono API function is
		 * invoked. Used for both built-in value types (e.g. C# int, byte, float) and custom value types (e.g. C# structs).
		 */
		TypeValue = 1,

		/**
		 * An instance of a reference type, i.e. the GC handle of a MonoObject*.
		 */
		TypeMonoObjectPtr = 2,

		/**
		 * A raw pointer in remote memory. Gets passed directly to any Mono API functions.
		 */
		TypeRawPtr = 3
	};

	/**
	 * Direction of the value contained in the variant.
	 *
	 * This is only useful when calling Mono API functions whose parameters are not always of a fixed directionality. One such
	 * example is mono_runtime_invoke(), where the directionality of each parameter depends on how the method was defined (e.g.
	 * in C# whether the `out` or `ref` modifiers were used for a parameter). In this case, you **must** specify the direction
	 * if it differs from the default (see the parameter tags in the function definition in RMonoAPIBackend).
	 *
	 * For example: For mono_runtime_invoke(), you **must** pass out-params (C# `out`) and inout-params (C# `ref`) using
	 * DirectionOut and DirectionInOut, respectively. Not doing so (even if you don't care about the result of an out-param)
	 * will lead to crashes.
	 *
	 * @see forDirection()
	 * @see in()
	 * @see out()
	 * @see inout()
	 */
	enum Direction
	{
		/**
		 * Direction not specified.
		 *
		 * This is the default for all variants and means that the variant uses the direction that is the default for whatever
		 * Mono API function it's used in.
		 */
		DirectionDefault = 0 << 3,

		/**
		 * Input direction (local value is passed to remote Mono API function, but is not read back).
		 */
		DirectionIn = 1 << 3,

		/**
		 * Output direction (undefined value is passed to remote Mono API function, but the new value after the call is read back).
		 */
		DirectionOut = 2 << 3,

		/**
		 * Both directions.
		 */
		DirectionInOut = 3 << 3
	};

	/**
	 * A constructor disambiguation tag used to create variants of type TypeRawPtr.
	 */
	static inline const struct RawPtr {} rawPtr = {};

	class MonoObjectPtrWrapper
	{
	public:
		virtual RMonoObjectPtr getWrappedMonoObjectPtr() const = 0;
	};

private:
	enum Flags
	{
		FlagMaskType = 0x0007,
		FlagMaskDirection = 0x0018,

		/**
		 * Normally for TypeMonoObjectPtr variants, the wrapper function will check if the object is a boxed value type,
		 * and automatically unbox it in that case. This is because most (all?) Mono API functions expect a pointer to
		 * the raw data for value type objects instead of a boxed object. However, for convenience RemoteMono allows
		 * passing boxed objects anyway and handles unboxing on-the-fly in the wrapper.
		 *
		 * This flag can be used to disable this automatic unboxing. With it set, boxed value type objects will be passed
		 * as-is. This will make most Mono API functions malfunction for boxed value types, but it's possible that there
		 * is some Mono API function that actually expects the boxed objects, in which case this flag will be useful.
		 */
		FlagDisableAutoUnbox = 0x0100,

		/**
		 * Whether this object owns the memory it references. If not, it will simply store a pointer to user-provided
		 * memory and the user must ensure that this memory remains valid.
		 */
		FlagOwnMemory = 0x0200,

		/**
		 * Whether this object is simply an alias for another RMonoVariant object. When an alias is read or written,
		 * it will simply read from or write to to object it aliases. Alias objects are created e.g. by #forDirection(),
		 * where the alias will have an different Direction value than the aliased object.
		 */
		FlagIsAlias = 0x0400
	};

public:
	/**
	 * Return the maximum alignment that any value could possibly require in remote memory.
	 */
	static constexpr size_t getMaxRequiredAlignment()
	{
		// TODO: I have no clue what value this should actually be.
		return 16;
	}

public:
	/**
	 * Creates an invalid variant, passed as a NULL pointer to Mono API functions.
	 */
	RMonoVariant() : flags(TypeInvalid) {}

	/**
	 * Copy constructor.
	 */
	RMonoVariant(const Self& other)
	{
		copyFromOther(other);
	}

	/**
	 * Creates an object of a value type.
	 *
	 * This is equivalent to calling `RMonoVariant(&val, sizeof(T), true)`, i.e. it copies the data.
	 *
	 * @param val The value.
	 * @see RMonoVariant(void*, size_t, bool)
	 */
	template <typename T, typename Enable = std::enable_if_t<!std::is_base_of_v<MonoObjectPtrWrapper, T>>>
	RMonoVariant(T val)
			: flags(TypeValue | FlagOwnMemory)
	{
		if constexpr(sizeof(T) <= sizeof(v.sdata)) {
			*((T*) v.sdata) = val;
		} else {
			v.ddata = new char[sizeof(T)];
			*((T*) v.ddata) = val;
		}
		v.size = sizeof(T);
	}

	/**
	 * Creates an object of a value type in user-provided memory.
	 *
	 * This is equivalent to calling `RMonoVariant(val, sizeof(T), false)`, i.e. it does **not** copy the data, but stores the
	 * pointer directly.
	 *
	 * @param val Pointer to the value.
	 * @see RMonoVariant(void*, size_t, bool)
	 */
	template <typename T>
	RMonoVariant(T* val)
			: flags(TypeValue)
	{
		v.ddata = val;
		v.size = sizeof(T);
	}

	/**
	 * Creates an object of a value type from a buffer.
	 *
	 * Can be used for two purposes:
	 *
	 * 1. For C# built-in value types (e.g. int, float, byte), in which case you can just pass a pointer to a variable of the
	 *    corresponding C++ type (e.g. int32_t, float, uint8_t).
	 * 2. For any custom value types, in which case you can pass a pointer to the equivalent C++ struct (be aware of alignment
	 *    and other details), or to a raw buffer that you received from another Mono API call.
	 *
	 * The data can either be copied into the variant object itself, or the object can simply store a pointer to the
	 * user-supplied memory. In the latter case, the user has to ensure that the data remains valid for as long as the
	 * RMonoVariant object is used. Storing a direct pointer can be useful for getting output values into user-supplied
	 * variables without unnecessary copying.
	 *
	 * If the variant is used for an output-only value, you can also pass a pointer to uninitialized memory, as long as the
	 * memory is large enough to hold the expected value type.
	 *
	 * @param data Pointer to the value type object's data. May be NULL, in which case a raw NULL pointer will be passed
	 * 		to the Mono API.
	 * @param size Size of the value type object's data in bytes.
	 * @param copy true if the object should make a copy of the data, false if it should store the pointer directly.
	 */
	RMonoVariant(void* data, size_t size, bool copy = false)
			: flags(TypeValue)
	{
		if (data  &&  copy) {
			flags |= FlagOwnMemory;
			if (size <= sizeof(v.sdata)) {
				memcpy(v.sdata, data, size);
			} else {
				v.ddata = new char[size];
				memcpy(v.ddata, data, size);
			}
		} else {
			v.ddata = data;
		}
		v.size = size;
	}

	/**
	 * Creates a variant containing an RMonoObjectPtr, i.e. an object of a class derived from System.Object.
	 *
	 * This version will copy the RMonoObjectPtr.
	 *
	 * @param v The object pointer.
	 * @param autoUnbox Whether to enable auto-unboxing of boxed value types. See #FlagDisableAutoUnbox.
	 */
	RMonoVariant(const RMonoObjectPtr& v, bool autoUnbox = true)
			: flags(TypeMonoObjectPtr | FlagOwnMemory | (autoUnbox ? 0 : FlagDisableAutoUnbox)), o(v) {}

	/**
	 * Creates a variant containing an RMonoObjectPtr, i.e. an object of a class derived from System.Object.
	 *
	 * This version will copy the RMonoObjectPtr.
	 *
	 * @param v The object pointer.
	 * @param autoUnbox Whether to enable auto-unboxing of boxed value types. See #FlagDisableAutoUnbox.
	 */
	RMonoVariant(RMonoObjectPtr&& v, bool autoUnbox = true)
			: flags(TypeMonoObjectPtr | FlagOwnMemory | (autoUnbox ? 0 : FlagDisableAutoUnbox)), o(v) {}

	/**
	 * Creates a variant containing an RMonoObjectPtr, i.e. an object of a class derived from System.Object.
	 *
	 * This version will **not** copy the RMonoObjectPtr, but keep store the pointer directly. The user has to ensure that
	 * the memory remains valid for as long as the RMonoVariant object is used.
	 *
	 * @param v Pointer to the object pointer.
	 * @param autoUnbox Whether to enable auto-unboxing of boxed value types. See #FlagDisableAutoUnbox.
	 */
	RMonoVariant(RMonoObjectPtr* v, bool autoUnbox = true)
			: flags(TypeMonoObjectPtr | (autoUnbox ? 0 : FlagDisableAutoUnbox)), op(v) {}

	RMonoVariant(const MonoObjectPtrWrapper& v, bool autoUnbox = true)
			: flags(TypeMonoObjectPtr | FlagOwnMemory | (autoUnbox ? 0 : FlagDisableAutoUnbox)), o(v.getWrappedMonoObjectPtr()) {}

	/**
	 * Creates a `null` value.
	 *
	 * This will just pass a raw NULL pointer to the Mono API functions.
	 */
	RMonoVariant(std::nullptr_t) : flags(TypeRawPtr | FlagOwnMemory), p(0) {}

	/**
	 * Creates a variant that represents a raw pointer in remote memory. The pointer is passed directly to any Mono API
	 * functions. The caller must ensure that it is valid in the context of the remote process.
	 *
	 * Use RMonoVariant::rawPtr for the second parameter. It is just a constructor overload disambiguation tag.
	 *
	 * @param v The raw pointer in remote memory.
	 */
	explicit RMonoVariant(rmono_voidp v, RawPtr) : flags(TypeRawPtr | FlagOwnMemory), p(v) {}

	/**
	 * Creates a variant that represents a raw pointer in remote memory. The pointer is passed directly to any Mono API
	 * functions. The caller must ensure that it is valid in the context of the remote process.
	 *
	 * This version of the constructor takes a pointer to the raw remote pointer, and can be used to receive raw pointers
	 * returned from a Mono API function.
	 *
	 * Use RMonoVariant::rawPtr for the second parameter. It is just a constructor overload disambiguation tag.
	 *
	 * @param v Pointer to the raw pointer in remote memory.
	 */
	explicit RMonoVariant(rmono_voidp* v, RawPtr) : flags(TypeRawPtr), pp(v) {}

	~RMonoVariant()
	{
		if (getType() == TypeValue  &&  (flags & FlagOwnMemory)  !=  0  &&  !usesStaticValueData()) {
			delete[] v.ddata;
		}
	}




	/**
	 * Assignment operator.
	 */
	Self& operator=(const Self& other)
	{
		if (this != &other) {
			copyFromOther(other);
		}
		return *this;
	}


	/**
	 * Return an alias of this object that has the given explicit directionality.
	 *
	 * @see Direction
	 * @see in()
	 * @see out()
	 * @see inout()
	 */
	Self forDirection(Direction dir) const { return Self(const_cast<Self*>(this), dir); }


	/**
	 * Return a DirectionIn alias of this object.
	 */
	Self in() const { return forDirection(DirectionIn); }

	/**
	 * Return a DirectionOut alias of this object.
	 */
	Self out() const { return forDirection(DirectionOut); }

	/**
	 * Return a DirectionInOut alias of this object.
	 */
	Self inout() const { return forDirection(DirectionInOut); }


	/**
	 * Determine if this is a valid variant. Note that things like null pointers are still considered valid. Only variants
	 * of type TypeInvalid are considered invalid.
	 */
	bool isValid() const { return getType() != TypeInvalid; }


	/**
	 * Return this variant's type.
	 */
	Type getType() const { return (Type) (flags & FlagMaskType); }

	Direction getDirection() const { return (Direction) (flags & FlagMaskDirection); }
	void setDirection(Direction dir) { flags = (flags & ~FlagMaskDirection) | dir; }

	/**
	 * Enable or disable automatic unboxing of boxed value type objects.
	 */
	void setAutoUnboxEnabled(bool autoUnbox)
	{
		if ((flags & FlagIsAlias)  !=  0) {
			alias->setAutoUnboxEnabled(autoUnbox);
		}
		if (autoUnbox) {
			flags &= ~FlagDisableAutoUnbox;
		} else {
			flags |= FlagDisableAutoUnbox;
		}
	}

	/**
	 * Determines whether automatic unboxing of boxed value type objects is enabled.
	 */
	bool isAutoUnboxEnabled() const
	{
		return (flags & FlagDisableAutoUnbox) == 0;
	}

	/**
	 * Determine if this variant is a null pointer. Note that this can only be true for output value type variants, or
	 * for input custom value type variants. A variant of type TypeMonoObjectPtr that is null will return false.
	 *
	 * @see isNullValue()
	 */
	inline bool isNullPointer() const;


	/**
	 * Get the size of the stored value type, in bytes.
	 *
	 * This may only be called if the variant is of type TypeValue.
	 */
	size_t getValueSize() const
	{
		assert(getType() == TypeValue);
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->getValueSize();
		}
		return v.size;
	}

	/**
	 * Get a pointer to the stored value type data.
	 *
	 * This may only be called if the variant is of type TypeValue.
	 */
	void* getValueData()
	{
		assert(getType() == TypeValue);
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->getValueData();
		}
		return usesStaticValueData() ? v.sdata : v.ddata;
	}

	/**
	 * Get a pointer to the stored value type data.
	 *
	 * This may only be called if the variant is of type TypeValue.
	 */
	const void* getValueData() const
	{
		assert(getType() == TypeValue);
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->getValueData();
		}
		return usesStaticValueData() ? v.sdata : v.ddata;
	}

	/**
	 * Get a copy of the stored value type data (reinterpret-casted to T).
	 *
	 * This may only be called if the variant is of type TypeValue.
	 */
	template <typename T>
	T getValue() const
	{
		assert(getType() == TypeValue);
		return *((T*) getValueData());
	}

	/**
	 * Return the underlying RMonoObjectPtr.
	 *
	 * You may only call this if the variant is of type TypeMonoObjectPtr.
	 */
	RMonoObjectPtr getMonoObjectPtr() const
	{
		assert(getType() == TypeMonoObjectPtr);
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->getMonoObjectPtr();
		}
		if ((flags & FlagOwnMemory)  !=  0) {
			return o;
		} else {
			return (op ? *op : RMonoObjectPtr());
		}
	}

	/**
	 * Return the underlying raw remote pointer.
	 *
	 * You may only call this if the variant is of type TypeRawPtr.
	 */
	rmono_voidp getRawPtr() const
	{
		assert(getType() == TypeRawPtr);
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->getRawPtr();
		}
		if ((flags & FlagOwnMemory)  !=  0) {
			return p;
		} else {
			return (pp ? *pp : 0);
		}
	}


	/**
	 * Get the number of bytes required to hold this variant's value in the remote process.
	 *
	 * @param abi The ABI to use for determining the size.
	 * @param alignment The minimum alignment for the value.
	 * @return The value size in bytes.
	 */
	template <typename ABI>
	size_t getRemoteMemorySize(ABI& abi, size_t& alignment) const;

	/**
	 * Copy this variant's value to a buffer that can then be transferred to remote memory. The buffer must have at least
	 * `getRemoteMemorySize(abi)` bytes of free memory.
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
	explicit RMonoVariant::RMonoVariant(RMonoVariant* other, Direction dir)
			: flags((other->flags & ~FlagMaskDirection) | FlagIsAlias | dir), alias(other) {}

	void copyFromOther(const Self& other)
	{
		if ((other.flags & FlagIsAlias)  !=  0) {
			flags = other.flags;
			alias = other.alias;
		} else if (other.getType() == TypeMonoObjectPtr  &&  (other.flags & FlagOwnMemory) != 0) {
			flags = other.flags;
			o = other.o;
		} else if (other.getType() == TypeValue  &&  (other.flags & FlagOwnMemory) != 0  &&  !other.usesStaticValueData()) {
			v.size = other.v.size;
			v.ddata = new char[v.size];
			memcpy(v.ddata, other.v.ddata, v.size);
			flags = other.flags;
		} else {
			memcpy(this, &other, sizeof(Self));
		}
	}

	bool usesStaticValueData() const
	{
		if ((flags & FlagIsAlias)  !=  0) {
			return alias->usesStaticValueData();
		}
		return ((flags & FlagOwnMemory) != 0)  &&  v.size <= sizeof(v.sdata);
	}

private:
	uint16_t flags;

	union {
		struct {
			union
			{
				char sdata[32];
				void* ddata;
			};
			size_t size;
		} v;
		RMonoObjectPtr* op;
		rmono_voidp p;
		rmono_voidp* pp;
		RMonoVariant* alias;
	};
	RMonoObjectPtr o;
};


};





#include "RMonoVariant_Impl.h"
