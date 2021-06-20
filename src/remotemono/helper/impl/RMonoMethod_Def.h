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

#include <memory>
#include <unordered_map>
#include "RMonoHelperContext_Def.h"
#include "../../RMonoAPI.h"
#include "../../RMonoVariant.h"
#include "RMonoClass_Def.h"
#include "RMonoObject_Def.h"


namespace remotemono
{


class RMonoMethod
{
protected:
	struct Data
	{
		Data(RMonoHelperContext* ctx, RMonoMethodPtr method, const RMonoClass& cls)
				: ctx(ctx), mono(ctx->getMonoAPI()), method(method), cls(cls), flags(0)
		{
			if (method) {
				flags = mono->methodGetFlags(method);
			}
		}

		RMonoHelperContext* ctx;
		RMonoAPI* mono;
		RMonoMethodPtr method;
		RMonoClass cls;
		uint32_t flags;
	};

	struct InstData
	{
		InstData(const RMonoObject& obj)
				: obj(obj) {}

		RMonoObject obj;
	};

public:
	RMonoMethod() {}
	RMonoMethod(std::nullptr_t) {}
	RMonoMethod(RMonoHelperContext* ctx, RMonoMethodPtr method, const RMonoClass& cls, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, method, cls)), id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoMethod(RMonoHelperContext* ctx, RMonoMethodPtr method, const RMonoObject& obj = RMonoObject())
			: d(std::make_shared<Data>(ctx, method, ctx->getCachedClass(ctx->getMonoAPI()->methodGetClass(method)))),
			  id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoMethod(const RMonoMethod& other, const RMonoObject& obj)
			: d(other.d), id(obj ? std::make_shared<InstData>(obj) : std::shared_ptr<InstData>()) {}
	RMonoMethod(const RMonoMethod& other)
			: d(other.d), id(other.id) {}

	RMonoMethod& operator=(const RMonoMethod& other)
	{
		if (this != &other) {
			d = other.d;
			id = other.id;
		}
		return *this;
	}

	bool isValid() const { return ((bool) d)  &&  d->method.isValid(); }
	bool isNull() const { return !isValid(); }
	operator bool() const { return isValid(); }

	RMonoMethodPtr operator*() const { return d ? d->method : RMonoMethodPtr(); }
	operator RMonoMethodPtr() const { return **this; }

	bool operator==(const RMonoMethod& other) const { return (d && other.d) ? (d->method == other.d->method) : true; }
	bool operator!=(const RMonoMethod& other) const { return !(*this == other); }

	RMonoHelperContext* getContext() const { return d ? d->ctx : nullptr; }
	RMonoAPI* getMonoAPI() const { return d ? d->mono : nullptr; }

	RMonoMethod inst(const RMonoObject& obj) { return RMonoMethod(*this, obj); }

	bool isInstanced() const { return (bool) id; }

	RMonoClass getClass() const { assertValid(); return d->cls; }

	bool isStatic() const { assertValid(); return (d->flags & METHOD_ATTRIBUTE_STATIC) != 0; }

	uint32_t getFlags() const { assertValid(); return d->flags; }

	inline RMonoObject invoke(RMonoVariantArray& args);
	inline RMonoObject invoke(RMonoVariantArray&& args);

	template <typename... VariantT>
	inline RMonoObject invoke(VariantT... args);

	inline RMonoObject operator()(RMonoVariantArray& args);
	inline RMonoObject operator()(RMonoVariantArray&& args);

	template <typename... VariantT>
	inline RMonoObject operator()(VariantT... args);

	rmono_funcp compile() const { assertValid(); return d->mono->compileMethod(d->method); }

private:
	void assertValid() const
	{
		if (!isValid()) {
			throw RMonoException("Invalid method");
		}
	}

private:
	std::shared_ptr<Data> d;
	std::shared_ptr<InstData> id;
};


}
