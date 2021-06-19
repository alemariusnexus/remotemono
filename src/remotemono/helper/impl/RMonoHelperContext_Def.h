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

#include <string>
#include <unordered_map>
#include "../../RMonoAPI.h"



namespace remotemono
{


class RMonoClass;
class RMonoObject;


class RMonoHelperContext
{
private:
	enum Flags {
		FlagEnableExtendedVerification = 0x01
	};

public:
	RMonoHelperContext(RMonoAPI* mono)
			: mono(mono), flags(0)
	{
		init();
	}

	RMonoAPI* getMonoAPI() const { return mono; }

	void setExtendedVerificationEnabled(bool enabled);
	bool isExtendedVerificationEnabled() const { return (flags & FlagEnableExtendedVerification) != 0; }

	inline RMonoClass getCachedClass(RMonoClassPtr cls);

	inline RMonoClass classFromName(RMonoImagePtr image, const std::string& nameSpace, const std::string& name);

	inline RMonoObject str(const std::string_view& s);

	inline RMonoClass classObject();
	inline RMonoClass classString();

private:
	inline void init();

private:
	RMonoAPI* mono;

	int flags;

	std::unordered_map<RMonoClassPtr, RMonoClass> classesByPtr;

	RMonoClass* clsObject;
	RMonoClass* clsInt16;
	RMonoClass* clsInt32;
	RMonoClass* clsInt64;
	RMonoClass* clsDouble;
	RMonoClass* clsSingle;
	RMonoClass* clsString;
	RMonoClass* clsThread;
	RMonoClass* clsUInt16;
	RMonoClass* clsUInt32;
	RMonoClass* clsUInt64;
	RMonoClass* clsVoid;
	RMonoClass* clsArray;
	RMonoClass* clsBoolean;
	RMonoClass* clsByte;
	RMonoClass* clsSByte;
	RMonoClass* clsChar;
	RMonoClass* clsException;
};


}
