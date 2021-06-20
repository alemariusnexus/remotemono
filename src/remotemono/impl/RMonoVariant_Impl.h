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

#include "RMonoVariant_Def.h"
#include "abi/RMonoABITypeTraits.h"

#include <exception>




namespace remotemono
{



bool RMonoVariant::isNullPointer() const
{
	if ((flags & FlagIsAlias)  !=  0) {
		return alias->isNullPointer();
	}

	switch (getType()) {
	case TypeInvalid:
		return true;
	case TypeValue:
		return ((flags & FlagOwnMemory) == 0)  &&  v.ddata == nullptr;
	case TypeMonoObjectPtr:
		return ((flags & FlagOwnMemory) == 0)  &&  op == nullptr;
	case TypeRawPtr:
		return ((flags & FlagOwnMemory) == 0)  &&  pp == nullptr;
	default:
		assert(false);
	}

	return true;
}


template <typename ABI>
size_t RMonoVariant::getRemoteMemorySize(ABI& abi, size_t& alignment) const
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if ((flags & FlagIsAlias)  !=  0) {
		return alias->getRemoteMemorySize(abi, alignment);
	}

	size_t size;

	if (isNullPointer()) {
		alignment = 1;
		size = 0;
	} else {
		// TODO: Actually, I have no clue what the minimum alignments for those types are. It's just a wild guess that errs
		// somewhat on the safe side...

		switch (getType()) {
		case TypeValue:
			if (v.size <= 1) {
				alignment = 1;
			} else if (v.size <= 2) {
				alignment = 2;
			} else if (v.size <= 4) {
				alignment = 4;
			} else if (v.size <= 8) {
				alignment = 8;
			} else {
				alignment = 16; // Might be necessary for SIMD instructions
			}
			size = v.size;
			break;
		case TypeMonoObjectPtr:
			alignment = sizeof(typename ABITypeTraits::irmono_gchandle);
			size = sizeof(typename ABITypeTraits::irmono_gchandle);
			break;
		case TypeRawPtr:
			alignment = sizeof(typename ABITypeTraits::irmono_voidp);
			size = sizeof(typename ABITypeTraits::irmono_voidp);
			break;
		default:
			assert(false);
			alignment = 1;
			size = 0;
		}
	}

	assert(alignment <= getMaxRequiredAlignment());

	return size;
}


template <typename ABI>
void RMonoVariant::copyForRemoteMemory(ABI& abi, void* buf) const
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if ((flags & FlagIsAlias)  !=  0) {
		alias->copyForRemoteMemory(abi, buf);
		return;
	}

	if (isNullPointer()) {
		return;
	}

	switch (getType()) {
	case TypeValue:
		memcpy(buf, getValueData(), v.size);
		break;
	case TypeMonoObjectPtr:
		*((typename ABITypeTraits::irmono_gchandle*) buf) = abi.hp2i_RMonoObjectPtr(getMonoObjectPtr());
		break;
	case TypeRawPtr:
		*((typename ABITypeTraits::irmono_voidp*) buf) = abi.p2i_rmono_voidp(getRawPtr());
		break;
	}
}


template <typename ABI>
void RMonoVariant::updateFromRemoteMemory(ABI& abi, RMonoAPIBase& mono, void* buf)
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if ((flags & FlagIsAlias)  !=  0) {
		alias->updateFromRemoteMemory(abi, mono, buf);
		return;
	}

	if (isNullPointer()) {
		return;
	}

	switch (getType()) {
	case TypeValue:
		memcpy(getValueData(), buf, v.size);
		break;
	case TypeMonoObjectPtr:
		if ((flags & FlagOwnMemory)  !=  0) {
			o = abi.hi2p_RMonoObjectPtr(*((typename ABITypeTraits::irmono_gchandle*) buf), &mono);
		} else {
			*op = abi.hi2p_RMonoObjectPtr(*((typename ABITypeTraits::irmono_gchandle*) buf), &mono);
		}
		break;
	case TypeRawPtr:
		if ((flags & FlagOwnMemory)  !=  0) {
			p = abi.i2p_rmono_voidp(*((typename ABITypeTraits::irmono_voidp*) buf));
		} else {
			*pp = abi.i2p_rmono_voidp(*((typename ABITypeTraits::irmono_voidp*) buf));
		}
		break;
	default:
		assert(false);
	}
}



};
