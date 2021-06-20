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
#include <unordered_map>
#include <tuple>
#include "RMonoHelperContext_Def.h"
#include "../../RMonoAPI.h"
#include "../../util.h"


namespace remotemono
{


class RMonoField;
class RMonoProperty;
class RMonoMethod;
class RMonoObject;


class RMonoClass
{
private:
	struct MethodNameWithParamCount
	{
		MethodNameWithParamCount(const std::string& name, int32_t paramCount)
				: name(name), paramCount(paramCount >= 0 ? paramCount : -1) {}

		bool operator==(const MethodNameWithParamCount& o) const
		{
			return name == o.name  &&  paramCount == o.paramCount;
		}

		std::string name;
		int32_t paramCount;
	};
	struct MethodDesc
	{
		MethodDesc(const std::string& desc, bool includeNamespace)
				: desc(desc), includeNamespace(includeNamespace) {}

		bool operator==(const MethodDesc& o) const
		{
			return desc == o.desc  &&  includeNamespace == o.includeNamespace;
		}

		std::string desc;
		bool includeNamespace;
	};

	struct MethodNameWithParamCountHash
	{
		size_t operator()(MethodNameWithParamCount const& v) const
		{
			size_t h = 0;
			hash_combine(h, v.name);
			hash_combine(h, v.paramCount);
			return h;
		}
	};
	struct MethodDescHash
	{
		size_t operator()(MethodDesc const& v) const
		{
			size_t h = 0;
			hash_combine(h, v.desc);
			hash_combine(h, v.includeNamespace);
			return h;
		}
	};

	struct Data
	{
		Data(RMonoHelperContext* ctx, RMonoClassPtr cls)
				: ctx(ctx), mono(ctx->getMonoAPI()), cls(cls) {}

		RMonoHelperContext* ctx;
		RMonoAPI* mono;
		RMonoClassPtr cls;

		std::unordered_map<std::string, RMonoField> fieldsByName;
		std::unordered_map<std::string, RMonoProperty> propsByName;
		std::unordered_map<MethodNameWithParamCount, RMonoMethod, MethodNameWithParamCountHash> methodsByName;
		std::unordered_map<MethodDesc, RMonoMethod, MethodDescHash> methodsByDesc;
	};

public:
	RMonoClass() {}
	RMonoClass(std::nullptr_t) {}
	RMonoClass(RMonoHelperContext* ctx, RMonoClassPtr cls)
			: d(std::make_shared<Data>(ctx, cls)) {}
	RMonoClass(RMonoHelperContext* ctx, RMonoImagePtr image, const std::string& nameSpace, const std::string& name)
			: d(std::make_shared<Data>(ctx, ctx->getMonoAPI()->classFromName(image, nameSpace, name))) {}
	RMonoClass(const RMonoClass& other)
			: d(other.d) {}

	RMonoClass& operator=(const RMonoClass& other)
	{
		if (this != &other) {
			d = other.d;
		}
		return *this;
	}

	bool isValid() const { return ((bool) d)  &&  d->cls.isValid(); }
	bool isNull() const { return !isValid(); }
	operator bool() const { return isValid(); }

	RMonoClassPtr operator*() const { return d ? d->cls : RMonoClassPtr(); }
	operator RMonoClassPtr() const { return **this; }

	bool operator==(const RMonoClass& other) const { return (d && other.d) ? (d->cls == other.d->cls) : true; }
	bool operator!=(const RMonoClass& other) const { return !(*this == other); }

	RMonoHelperContext* getContext() const { return d ? d->ctx : nullptr; }
	RMonoAPI* getMonoAPI() const { return d ? d->mono : nullptr; }

	std::string getName() const { assertValid(); return d->mono->classGetName(d->cls); }
	std::string getNamespace() const { assertValid(); return d->mono->classGetNamespace(d->cls); }

	inline RMonoField field(const std::string& name) const;

	inline RMonoProperty property(const std::string& name) const;

	inline RMonoMethod method(const std::string& name, int32_t paramCount = -1) const;
	inline RMonoMethod methodDesc(const std::string& desc, bool includeNamespace = false) const;

	inline RMonoObject allocObject();

	inline RMonoObject newObject(RMonoVariantArray& args);
	inline RMonoObject newObject(RMonoVariantArray&& args = RMonoVariantArray());

	template <typename... VariantT>
	inline RMonoObject newObject(VariantT... args);

	inline RMonoObject newObjectDesc(const std::string_view& argsDesc, RMonoVariantArray& args);
	inline RMonoObject newObjectDesc(const std::string_view& argsDesc, RMonoVariantArray&& args = RMonoVariantArray());

	template <typename... VariantT>
	inline RMonoObject newObjectDesc(const std::string_view& argsDesc, VariantT... args);

	RMonoVTablePtr vtable() const { assertValid(); return d->mono->classVTable(d->cls); }
	RMonoTypePtr type() const { assertValid(); return d->mono->classGetType(d->cls); }
	RMonoReflectionTypePtr typeObject() const { assertValid(); return d->mono->typeGetObject(type()); }
	bool isValueType() const { assertValid(); return d->mono->classIsValueType(d->cls); }
	int32_t valueSize(uint32_t* align = nullptr) const { assertValid(); return d->mono->classValueSize(d->cls, align); }

private:
	void assertValid() const
	{
		if (!isValid()) {
			throw RMonoException("Invalid class");
		}
	}

private:
	std::shared_ptr<Data> d;
};


}
