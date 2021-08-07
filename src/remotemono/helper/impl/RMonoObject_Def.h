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

#include "../../config.h"

#include <memory>
#include <unordered_map>
#include "RMonoHelperContext_Def.h"
#include "../../RMonoAPI.h"
#include "../../RMonoVariant.h"
#include "../../RMonoException.h"
#include "RMonoClass_Def.h"


namespace remotemono
{


class RMonoField;


class RMonoObject : public RMonoVariant::MonoObjectPtrWrapper
{
private:
	struct Data
	{
		Data(RMonoHelperContext* ctx, RMonoObjectPtr obj, const RMonoClass& cls)
				: ctx(ctx), mono(ctx->getMonoAPI()), obj(obj), cls(cls) {}

		RMonoHelperContext* ctx;
		RMonoAPI* mono;
		RMonoObjectPtr obj;
		RMonoClass cls;
	};

public:
	RMonoObject() {}
	RMonoObject(std::nullptr_t) {}
	RMonoObject(RMonoHelperContext* ctx)
			: d(std::make_shared<Data>(ctx, nullptr, nullptr)) {}
	RMonoObject(RMonoHelperContext* ctx, RMonoObjectPtr obj, const RMonoClass& cls)
			: d(std::make_shared<Data>(ctx, obj, cls)) {}
	RMonoObject(RMonoHelperContext* ctx, RMonoObjectPtr obj)
			: d(std::make_shared<Data>(ctx, obj, obj ? ctx->getCachedClass(ctx->getMonoAPI()->objectGetClass(obj)) : nullptr)) {}
	RMonoObject(const RMonoObject& other)
			: d(other.d) {}

	RMonoObject& operator=(const RMonoObject& other)
	{
		if (this != &other) {
			d = other.d;
		}
		return *this;
	}

	bool isValid() const { return ((bool) d)  &&  d->obj.isValid(); }
	bool isNull() const { return !isValid(); }
	operator bool() const { return isValid(); }

	RMonoObjectPtr operator*() const { return d ? d->obj : RMonoObjectPtr(); }
	operator RMonoObjectPtr() const { return **this; }

	bool operator==(const RMonoObject& other) const { return (d && other.d) ? (d->obj == other.d->obj) : true; }
	bool operator!=(const RMonoObject& other) const { return !(*this == other); }

	void reset() { d.reset(); }

	RMonoObjectPtr getWrappedMonoObjectPtr() const override { return d ? d->obj : RMonoObjectPtr(); }

	RMonoHelperContext* getContext() const { return d ? d->ctx : nullptr; }
	RMonoAPI* getMonoAPI() const { return d ? d->mono : nullptr; }

	RMonoClass getClass() const { assertValid(); return d->cls; }

	inline RMonoField field(const std::string& name) const;

	inline RMonoProperty property(const std::string& name) const;

	inline RMonoMethod method(const std::string& name, int32_t paramCount = -1) const;
	inline RMonoMethod methodDesc(const std::string& desc, bool includeNamespace = false) const;

	RMonoVariant forDirection(RMonoVariant::Direction dir, bool autoUnbox = true) const
	{
		if (dir != RMonoVariant::DirectionIn) {
			if (!d) {
				throw RMonoException("Attempted to call RMonoObject::forDirection() for an out-direction on an object "
						"that doesn't have access to a helper context. Did you create it from the default constructor?"
						);
			}

			RMonoVariant v(&const_cast<RMonoObject*>(this)->d->obj, autoUnbox);
			v.setDirection(dir);
			return v;
		} else {
			RMonoVariant v(d ? const_cast<RMonoObject*>(this)->d->obj : RMonoObjectPtr(), autoUnbox);
			v.setDirection(dir);
			return v;
		}
	}
	RMonoVariant in(bool autoUnbox = true) const { return forDirection(RMonoVariant::DirectionIn, autoUnbox); }
	RMonoVariant out(bool autoUnbox = true) const { return forDirection(RMonoVariant::DirectionOut, autoUnbox); }
	RMonoVariant inout(bool autoUnbox = true) const { return forDirection(RMonoVariant::DirectionInOut, autoUnbox); }

	template <typename T>
	T unbox() const { assertValid(); return d->mono->objectUnbox<T>(d->obj); }

	RMonoVariant unboxRaw() const { assertValid(); return d->mono->objectUnboxRaw(d->obj); }

	std::string toUTF8() const { assertValid(); return d->mono->stringToUTF8((RMonoStringPtr) d->obj); }
	std::string str() const { return toUTF8(); }

	RMonoStringPtr toString() const { assertValid(); return d->mono->objectToString(d->obj); }
	std::string toStringUTF8() const { assertValid(); return d->mono->objectToStringUTF8(d->obj); }

	bool instanceof(const RMonoClassPtr cls) const { assertValid(); return d->mono->objectIsInst(d->obj, cls); }

	// TODO: Add more array methods
	template <typename T = RMonoObject>
	std::vector<T> arrayAsVector() const;

	inline RMonoObject pin() const;
	RMonoObjectPtrRaw raw() const { return d ? d->obj.raw() : (RMonoObjectPtrRaw) 0; }

private:
	void assertValid() const
	{
		if (!isValid()) {
			throw RMonoException("Invalid object");
		}
	}

private:
	std::shared_ptr<Data> d;
};


}
