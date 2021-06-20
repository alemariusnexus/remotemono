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
#include "RMonoClass_Def.h"
#include "RMonoObject_Def.h"
#include "RMonoMethod_Def.h"


namespace remotemono
{


class RMonoProperty
{
protected:
	struct Data
	{
		Data(RMonoHelperContext* ctx, RMonoPropertyPtr prop, const RMonoClass& cls)
				: ctx(ctx), mono(ctx->getMonoAPI()), prop(prop), cls(cls), staticFlag(false)
		{
			if (prop) {
				getter = RMonoMethod(ctx, mono->propertyGetGetMethod(prop), cls);
				setter = RMonoMethod(ctx, mono->propertyGetSetMethod(prop), cls);

				// TODO: Is it possible that only one of them is static?
				staticFlag = (getter.isValid()  &&  getter.isStatic())
						||  (setter.isValid()  &&  setter.isStatic());
			}
		}

		RMonoHelperContext* ctx;
		RMonoAPI* mono;
		RMonoPropertyPtr prop;
		RMonoClass cls;
		RMonoMethod getter;
		RMonoMethod setter;
		bool staticFlag;
	};

	struct InstData
	{
		InstData(const RMonoObject& obj, const Data& d)
				: obj(obj), getter(RMonoMethod(d.getter, obj)), setter(RMonoMethod(d.setter, obj))
		{
		}

		RMonoObject obj;
		RMonoMethod getter;
		RMonoMethod setter;
	};

public:
	RMonoProperty() {}
	RMonoProperty(std::nullptr_t) {}
	RMonoProperty(RMonoHelperContext* ctx, RMonoPropertyPtr prop, const RMonoClass& cls, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, prop, cls)), id(obj ? std::make_shared<InstData>(obj, *d) : std::shared_ptr<InstData>()) {}
	RMonoProperty(RMonoHelperContext* ctx, RMonoPropertyPtr prop, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, prop, ctx->getCachedClass(ctx->getMonoAPI()->propertyGetParent(prop)))),
			  id(obj ? std::make_shared<InstData>(obj, *d) : std::shared_ptr<InstData>()) {}
	RMonoProperty(const RMonoProperty& other, const RMonoObject& obj)
			: d(other.d), id(obj ? std::make_shared<InstData>(obj, *d) : std::shared_ptr<InstData>()) {}
	RMonoProperty(const RMonoProperty& other)
			: d(other.d), id(other.id) {}

	RMonoProperty& operator=(const RMonoProperty& other)
	{
		if (this != &other) {
			d = other.d;
			id = other.id;
		}
		return *this;
	}

	bool isValid() const { return ((bool) d)  &&  d->prop.isValid(); }
	bool isNull() const { return !isValid(); }
	operator bool() const { return isValid(); }

	RMonoPropertyPtr operator*() const { return d ? d->prop : RMonoPropertyPtr(); }
	operator RMonoPropertyPtr() const { return **this; }

	bool operator==(const RMonoProperty& other) const { return (d && other.d) ? (d->prop == other.d->prop) : true; }
	bool operator!=(const RMonoProperty& other) const { return !(*this == other); }

	RMonoHelperContext* getContext() const { return d ? d->ctx : nullptr; }
	RMonoAPI* getMonoAPI() const { return d ? d->mono : nullptr; }

	RMonoProperty inst(const RMonoObject& obj) { return RMonoProperty(*this, obj); }

	bool isInstanced() const { return (bool) id; }

	RMonoClass getClass() const { assertValid(); return d->cls; }

	bool isStatic() const { assertValid(); return d->staticFlag; }

	inline RMonoMethod getter() const;
	inline RMonoMethod setter() const;

	inline RMonoObject get(RMonoVariantArray& args) const;
	inline RMonoObject get(RMonoVariantArray&& args = RMonoVariantArray()) const;

	template <typename... VariantT>
	inline RMonoObject get(VariantT... args) const;

	template <typename T>
	inline std::enable_if_t<!std::is_same_v<T, RMonoObject>, T> get() const;

	inline RMonoObject set(RMonoVariantArray& args) const;
	inline RMonoObject set(RMonoVariantArray&& args = RMonoVariantArray()) const;

	template <typename... VariantT>
	inline RMonoObject set(VariantT... args) const;

private:
	void assertValid() const
	{
		if (!isValid()) {
			throw RMonoException("Invalid property");
		}
	}

private:
	std::shared_ptr<Data> d;
	std::shared_ptr<InstData> id;
};


}
