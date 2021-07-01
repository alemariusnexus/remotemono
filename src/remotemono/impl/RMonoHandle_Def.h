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

#include <cstddef>
#include "RMonoHandle_FwdDef.h"
#include "RMonoTypes.h"



namespace remotemono
{




class RMonoAPIBase;


class RMonoHandleBackendBase
{
public:
	virtual void forceDelete() = 0;
};





inline void RMonoObjectHandleDelete(rmono_gchandle gchandle, RMonoAPIBase* mono);




struct RMonoHandleTag {};

/**
 * A wrapper around a handle in the remote process. RMonoHandle is mostly used to wrap various pointer types to Mono
 * data structures (e.g. MonoAssembly*, MonoImage*). It allows for simple automatic memory management by using shared
 * pointers internally, so that you normally don't have to free resources returned by the Mono API functions. Note that
 * MonoObject* (and derived types like MonoString*, MonoException* etc.) will use the subclass RMonoObjectHandle.
 */
template <
	typename HandleT,
	void (*deleter)(HandleT, RMonoAPIBase*),
	HandleT invalidHandle
	>
class RMonoHandle : public RMonoHandleTag
{
public:
	typedef HandleT HandleType;
	typedef RMonoHandle<HandleType, deleter, invalidHandle> Self;

private:
	struct Data : public RMonoHandleBackendBase
	{
		enum Flags
		{
			FlagOwned = 0x01
		};

		Data(HandleT handle, RMonoAPIBase* mono, bool owned)
				: handle(handle), mono(mono), flags(owned ? FlagOwned : 0)
		{
			if (owned) {
				registerBackend();
			}
		}

		~Data()
		{
			if (mono) {
				if ((flags & FlagOwned)  !=  0) {
					unregisterBackend();
					deleter(handle, mono);
				}
			}
		}

		inline void registerBackend();
		inline void unregisterBackend();

		bool takeOwnership()
		{
			if ((flags & FlagOwned)  !=  0) {
				unregisterBackend();
				flags &= ~FlagOwned;
				return true;
			}
			return false;
		}

		virtual void forceDelete()
		{
			if ((flags & FlagOwned)  !=  0) {
				deleter(handle, mono);
				flags = 0;
				mono = nullptr;
				handle = invalidHandle;
			}
		}

		HandleT handle;
		RMonoAPIBase* mono;
		uint8_t flags;
		typename std::list<RMonoHandleBackendBase*>::iterator regIt; // TODO: Find a way to get this type from RMonoAPIBase
	};

public:
	/**
	 * Create an invalid handle (e.g. a null pointer)
	 */
	RMonoHandle() {}

	/**
	 * Create an invalid handle (e.g. a null pointer)
	 */
	RMonoHandle(std::nullptr_t) {}

	/**
	 * Main constructor for creating an RMonoHandle from a raw handle (e.g. a raw remote pointer).
	 *
	 * @param handle The raw remote handle, e.g. a raw remote pointer or a GC handle.
	 * @param mono The Mono API from which the handle originates.
	 * @param owned true if the RMonoHandle should own the remote resources associated with the raw handle. Owning the
	 * 		resources means that the deleter function will be called when the internal shared pointer expires.
	 */
	RMonoHandle(HandleType handle, RMonoAPIBase* mono, bool owned)
			: d(handle == invalidHandle ? std::shared_ptr<Data>() : std::make_shared<Data>(handle, mono, owned)) {}

	/**
	 * Copy constructor.
	 */
	RMonoHandle(const Self& other) : d(other.d) {}

	/**
	 * Move constructor.
	 */
	RMonoHandle(Self&& other) : d(std::move(other.d)) {}


	/**
	 * Assignment operator.
	 */
	Self& operator=(const Self& other)
	{
		if (this != &other)
			d = other.d;
		return *this;
	}

	/**
	 * Move-assignment operator.
	 */
	Self& operator=(Self&& other)
	{
		if (this != &other)
			d = std::move(other.d);
		return *this;
	}


	/**
	 * Returns true if the raw handles of the two RMonoHandle objects are the same.
	 */
	bool operator==(const Self& other) const { return **this == *other; }

	/**
	 * Returns true if the raw handles of the two RMonoHandle objects are different.
	 */
	bool operator!=(const Self& other) const { return !(*this == other); }


	/**
	 * Returns the raw handle.
	 */
	HandleType operator*() const { return d ? d->handle : invalidHandle; }

	/**
	 * Returns the Mono API that this handle belongs to.
	 */
	RMonoAPIBase* getMonoAPI() { return d ? d->mono : nullptr; }


	/**
	 * Lets the caller take ownership of the remote resources behind the handle. After calling this method, the caller will
	 * be responsible for freeing any associated resources.
	 *
	 * @return true if this object had ownership before, false otherwise.
	 */
	bool takeOwnership()
	{
		return d ? d->takeOwnership() : false;
	}


	/**
	 * Resets this RMonoHandle to an invalid handle.
	 */
	void reset() { d.reset(); }


	/**
	 * @return true if this is a valid handle, i.e. a handle not created with the invalid raw handle or one of the
	 * invalid-handle constructors.
	 */
	bool isValid() const { return (bool) d; }

	/**
	 * The opposite of isValid().
	 */
	bool isNull() const { return !isValid(); }

	/**
	 * An alias for isValid().
	 */
	operator bool() const { return isValid(); }

protected:
	std::shared_ptr<Data> d;
};





struct RMonoObjectHandleTag {};

/**
 * A wrapper around MonoObject* and derived types like MonoString*, MonoException* etc.
 *
 * This class does not hold the raw remote pointer, but holds a rmono_gchandle (created by mono_gchandle_new()) to it.
 * Storing raw remote pointers to such objects in the local process is dangerous, because these objects are managed
 * by Mono's garbage collector, so they could be deleted at any moment as soon as the remote process itself does not
 * hold references to it anymore. And even if the remote process does still hold a reference to it, the GC is allowed
 * to move the objects in memory at any time. There are only three situations in which it is safe to hold references
 * to such objects in native code:
 *
 * 1. If the raw pointer to the object is kept in a processor register or on the stack of any Mono-attached
 *    thread.
 * 2. If the object is referenced via a rmono_gchandle obtained through mono_gchandle_new(), instead of via
 *    the raw MonoObject*.
 * 3. The object is referenced via a raw MonoObject* that belongs to a pinned rmono_gchandle. See the 'pinned'
 *    parameter of mono_gchandle_new().
 *
 * The first option can be ruled out for RemoteMono, because we want to keep references in the local process, not just
 * on the remote. The third option should be strongly avoided, because it will very quickly lead to severe performance
 * degradation because the GC can't do its job properly. This is why RemoteMono always keeps references to MonoObject*
 * as rmono_gchandle.
 *
 * It is possible to get the raw MonoObject* by calling raw(), but note that this is only safe to use if the
 * RMonoObjectHandle has been pinned by calling pin(), and only as long as that pinned RMonoObjectHandle remains valid.
 */
template <typename RawPtrT>
class RMonoObjectHandle :
		public RMonoHandle<rmono_gchandle, &RMonoObjectHandleDelete, REMOTEMONO_GCHANDLE_INVALID>,
		public RMonoObjectHandleTag
{
public:
	typedef RMonoHandle<rmono_gchandle, &RMonoObjectHandleDelete, REMOTEMONO_GCHANDLE_INVALID> Base;
	typedef RMonoObjectHandle<RawPtrT> Self;

	typedef RawPtrT RawPtr;

public:
	/**
	 * Constructs a null pointer handle.
	 */
	RMonoObjectHandle() : RMonoHandle() {}

	/**
	 * Constructs a null pointer handle
	 */
	RMonoObjectHandle(std::nullptr_t) : RMonoHandle(nullptr) {}

	/**
	 * Constructs an RMonoObjectHandle from a rmono_gchandle.
	 *
	 * @param gchandle The GC handle obtained from the remote process by mono_gchandle_new().
	 * @param mono The RMonoAPI instance that the object belongs to
	 * @param owned true if the handle should own the rmono_gchandle and free it using mono_gchandle_free() when
	 * 		the internal shared pointer expires.
	 */
	RMonoObjectHandle(rmono_gchandle gchandle, RMonoAPIBase* mono, bool owned = true) : RMonoHandle(gchandle, mono, owned) {}

	/**
	 * Copy constructor.
	 */
	RMonoObjectHandle(const Self& other) : RMonoHandle(other) {}

	/**
	 * Move constructor.
	 */
	RMonoObjectHandle(Self&& other) : RMonoHandle(std::move(other)) {}


	/**
	 * Assignment operator.
	 */
	Self& operator=(const Self& other) { Base::operator=(other); return *this; }

	/**
	 * Move-assignment operator
	 */
	Self& operator=(Self&& other) { Base::operator=(std::move(other)); return *this; }


	/**
	 * Returns true if the two RMonoObjectHandle instances refer to the same raw remote pointer, i.e. the same
	 * remote MonoObject. Note that this does not compare the rmono_gchandles: Two different GC handles may
	 * refer to the same MonoObject, and this method would return true in those cases.
	 */
	inline bool operator==(const Self& other) const;

	/**
	 * Returns true if the two RMonoObjectHandle instances refer to different raw remote pointers.
	 *
	 * @see operator==()
	 */
	bool operator!=(const Self& other) const { return !(*this == other); }


	/**
	 * Return a new RMonoObjectHandle pointing to the same object that this handle points to, but pinned down.
	 */
	inline Self pin() const;

	/**
	 * Return a new RMonoObjectHandle with a separate rmono_gchandle referencing the same raw pointer. The new
	 * handle will not be pinned.
	 */
	inline Self clone() const;

	/**
	 * Return the raw remote pointer behind this RMonoObjectHandle. This calls mono_gchandle_get_target(). Note that
	 * this the raw pointer is only safe to use as long as a pinned GC handle to it exists somewhere.
	 */
	inline RawPtr raw() const;
};



}




namespace std
{


template <
	typename HandleT,
	void (*deleter)(HandleT, remotemono::RMonoAPIBase*),
	HandleT invalidHandle
>
struct hash<typename remotemono::RMonoHandle<HandleT, deleter, invalidHandle>>
{
	typedef typename remotemono::RMonoHandle<HandleT, deleter, invalidHandle> HandleType;

	std::size_t operator()(HandleType const& h) const noexcept
	{
		return hh(*h);
	}

private:
	std::hash<typename HandleType::HandleType> hh;
};


}
