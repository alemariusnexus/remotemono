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

#include <string>
#include <memory>
#include "RMonoHelperContext_Def.h"
#include "RMonoClass_Def.h"
#include "RMonoObject_Def.h"
#include "../../RMonoAPI.h"
#include "../../RMonoException.h"
#include "../../impl/mono/metadata/tabledefs.h"


namespace remotemono
{


class RMonoField
{
protected:
	struct Data
	{
		Data(RMonoHelperContext* ctx, RMonoClassFieldPtr field, const RMonoClass& cls)
				: ctx(ctx), mono(ctx->getMonoAPI()), field(field), cls(cls), vtable(0), flags(0)
		{
			if (field) {
				vtable = mono->classVTable(mono->domainGet(), cls);
				flags = mono->fieldGetFlags(field);
			}
		}

		RMonoHelperContext* ctx;
		RMonoAPI* mono;
		RMonoClassFieldPtr field;
		RMonoClass cls;
		RMonoVTablePtr vtable;
		uint32_t flags;
	};

	struct InstData
	{
		InstData(const RMonoObject& obj)
				: obj(obj) {}

		RMonoObject obj;
	};

public:
	RMonoField() {}
	RMonoField(std::nullptr_t) {}
	RMonoField(RMonoHelperContext* ctx, RMonoClassFieldPtr field, const RMonoClass& cls, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, field, cls)), id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoField(RMonoHelperContext* ctx, RMonoClassFieldPtr field, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, field, ctx->getCachedClass(ctx->getMonoAPI()->fieldGetParent(field)))),
			  id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoField(const RMonoField& other, const RMonoObject& obj)
			: d(other.d), id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoField(const RMonoField& other)
			: d(other.d), id(other.id) {}

	RMonoField& operator=(const RMonoField& other)
	{
		if (this != &other) {
			d = other.d;
			id = other.id;
		}
		return *this;
	}

	bool isValid() const { return ((bool) d)  &&  d->field.isValid(); }
	bool isNull() const { return !isValid(); }
	operator bool() const { return isValid(); }

	RMonoClassFieldPtr operator*() const { return d ? d->field : RMonoClassFieldPtr(); }
	operator RMonoClassFieldPtr() const { return **this; }

	bool operator==(const RMonoField& other) const { return (d && other.d) ? (d->field == other.d->field) : true; }
	bool operator!=(const RMonoField& other) const { return !(*this == other); }

	RMonoHelperContext* getContext() const { return d ? d->ctx : nullptr; }
	RMonoAPI* getMonoAPI() const { return d ? d->mono : nullptr; }

	RMonoField inst(const RMonoObject& obj) { return RMonoField(*this, obj); }

	bool isInstanced() const { return (bool) id; }

	RMonoClass getClass() const { assertValid(); return d->cls; }

	bool isStatic() const { assertValid(); return (d->flags & FIELD_ATTRIBUTE_STATIC) != 0; }

	uint32_t getFlags() const { assertValid(); return d->flags; }

	void set(const RMonoVariant& val)
	{
		if (isStatic()) {
			d->mono->fieldStaticSetValue(d->vtable, d->field, val);
		} else {
			if (!isInstanced()) {
				throw RMonoException("Field is non-static but RMonoField object is non-instanced.");
			}
			if (!id->obj) {
				throw RMonoException("Field is non-static but instance is invalid.");
			}
			d->mono->fieldSetValue(id->obj, d->field, val);
		}
	}

	void get(RMonoVariant& val)
	{
		if (isStatic()) {
			d->mono->fieldStaticGetValue(d->vtable, d->field, val);
		} else {
			if (!isInstanced()) {
				throw RMonoException("Field is non-static but RMonoField object is non-instanced.");
			}
			if (!id->obj) {
				throw RMonoException("Field is non-static but instance is invalid.");
			}
			d->mono->fieldGetValue(id->obj, d->field, val);
		}
	}
	void get(RMonoVariant&& val) { get(val); }
	template <typename T = RMonoObject>
		T get()
	{
		if constexpr(std::is_same_v<T, RMonoObject>) {
			return getBoxed();
		} else if constexpr(std::is_same_v<T, RMonoObjectPtr>) {
			return *getBoxed();
		} else {
			if (isStatic()) {
				return d->mono->fieldStaticGetValue<T>(d->vtable, d->field);
			} else {
				if (!isInstanced()) {
					throw RMonoException("Field is non-static but RMonoField object is non-instanced.");
				}
				if (!id->obj) {
					throw RMonoException("Field is non-static but instance is invalid.");
				}
				return d->mono->fieldGetValue<T>(id->obj, d->field);
			}
		}
	}
	inline RMonoObject getBoxed();

	RMonoTypePtr type() const { assertValid(); return d->mono->fieldGetType(d->field); }
	RMonoReflectionTypePtr typeObject() const { assertValid(); return d->mono->typeGetObject(type()); }
	uint32_t offset() const { assertValid(); return d->mono->fieldGetOffset(d->field); }

	std::string name() const { assertValid(); return d->mono->fieldGetName(d->field); }

private:
	void assertValid() const
	{
		if (!isValid()) {
			throw RMonoException("Invalid field");
		}
	}

protected:
	std::shared_ptr<Data> d;
	std::shared_ptr<InstData> id;
};


}
