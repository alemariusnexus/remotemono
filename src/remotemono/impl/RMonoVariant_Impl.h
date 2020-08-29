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

#include "RMonoVariant_Def.h"
#include "abi/RMonoABITypeTraits.h"

#include <exception>




namespace remotemono
{



bool RMonoVariant::isNullPointer() const
{
	if (type == TypeInvalid) {
		return true;
	}

	if ((flags & FlagOut)  ==  0) {
		if (type == TypeCustomValueCopy) {
			return cvc.p == nullptr;
		} else if (type == TypeCustomValueRef) {
			return cvr == 0;
		}
		return false;
	}

	switch (type) {
	case TypeInt8:
		return i8p == nullptr;
	case TypeUInt8:
		return u8p == nullptr;
	case TypeInt16:
		return i16p == nullptr;
	case TypeUInt16:
		return u16p == nullptr;
	case TypeInt32:
		return i32p == nullptr;
	case TypeUInt32:
		return u32p == nullptr;
	case TypeInt64:
		return i64p == nullptr;
	case TypeUInt64:
		return u64p == nullptr;
	case TypeFloat:
		return fp == nullptr;
	case TypeDouble:
		return dp == nullptr;
	case TypeBool:
		return bp == nullptr;
	case TypeCustomValueCopy:
		return cvc.p == nullptr;
	case TypeCustomValueRef:
		return cvrp == nullptr;
	case TypeMonoObjectPtr:
		return op == nullptr;
	}

	return true;
}


bool RMonoVariant::isNullValue() const
{
	if (isNullPointer()) {
		return true;
	}

	if ((flags & FlagOut)  !=  0) {
		switch (type) {
		case TypeCustomValueRef:
			return (*cvrp == 0);
		case TypeMonoObjectPtr:
			return ! (bool) (*op);
		}
	} else {
		switch (type) {
		case TypeMonoObjectPtr:
			return ! (bool) o;
		}
	}

	return false;
}


template <typename ABI>
size_t RMonoVariant::getRemoteMemorySize(ABI& abi) const
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if (isNullPointer()) {
		return 0;
	}

	switch (type) {
	case TypeInt8:
	case TypeUInt8:
		return 1;
	case TypeInt16:
	case TypeUInt16:
		return 2;
	case TypeInt32:
	case TypeUInt32:
	case TypeFloat:
		return 4;
	case TypeInt64:
	case TypeUInt64:
	case TypeDouble:
		return 8;
	case TypeBool:
		return sizeof(typename ABITypeTraits::irmono_bool);
	case TypeCustomValueCopy:
		return cvc.size;
	case TypeCustomValueRef:
		return sizeof(typename ABITypeTraits::irmono_voidp);
	case TypeMonoObjectPtr:
		return sizeof(typename ABITypeTraits::irmono_gchandle);
	default:
		assert(false);
		return 0;
	}
}


template <typename ABI>
void RMonoVariant::copyForRemoteMemory(ABI& abi, void* buf) const
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if (isNullPointer()) {
		return;
	}

	if ((flags & FlagOut) != 0) {
		switch (type) {
		case TypeInt8:
			*((int8_t*) buf) = i8p ? *i8p : 0;
			break;
		case TypeUInt8:
			*((uint8_t*) buf) = u8p ? *u8p : 0;
			break;
		case TypeInt16:
			*((int16_t*) buf) = i16p ? *i16p : 0;
			break;
		case TypeUInt16:
			*((uint16_t*) buf) = u16p ? *u16p : 0;
			break;
		case TypeInt32:
			*((int32_t*) buf) = i32p ? *i32p : 0;
			break;
		case TypeUInt32:
			*((uint32_t*) buf) = u32p ? *u32p : 0;
			break;
		case TypeInt64:
			*((int64_t*) buf) = i64p ? *i64p : 0;
			break;
		case TypeUInt64:
			*((uint64_t*) buf) = u64p ? *u64p : 0;
			break;
		case TypeFloat:
			*((float*) buf) = fp ? *fp : 0.0f;
			break;
		case TypeDouble:
			*((double*) buf) = dp ? *dp : 0.0;
			break;
		case TypeBool:
			*((typename ABITypeTraits::irmono_bool*) buf) = bp ? (*bp ? 1 : 0) : 0;
			break;
		case TypeCustomValueCopy:
			if (cvc.p) {
				memcpy(buf, cvc.p, cvc.size);
			}
			break;
		case TypeCustomValueRef:
			*((typename ABITypeTraits::irmono_voidp*) buf) = abi.p2i_rmono_voidp(*cvrp);
			break;
		case TypeMonoObjectPtr:
			*((typename ABITypeTraits::irmono_gchandle*) buf) = abi.hp2i_RMonoObjectPtr(*op);
			break;
		default:
			assert(false);
		}
	} else {
		switch (type) {
		case TypeInt8:
			*((int8_t*) buf) = i8;
			break;
		case TypeUInt8:
			*((uint8_t*) buf) = u8;
			break;
		case TypeInt16:
			*((int16_t*) buf) = i16;
			break;
		case TypeUInt16:
			*((uint16_t*) buf) = u16;
			break;
		case TypeInt32:
			*((int32_t*) buf) = i32;
			break;
		case TypeUInt32:
			*((uint32_t*) buf) = u32;
			break;
		case TypeInt64:
			*((int64_t*) buf) = i64;
			break;
		case TypeUInt64:
			*((uint64_t*) buf) = u64;
			break;
		case TypeFloat:
			*((float*) buf) = f;
			break;
		case TypeDouble:
			*((double*) buf) = d;
			break;
		case TypeBool:
			*((typename ABITypeTraits::irmono_bool*) buf) = b ? 1 : 0;
			break;
		case TypeCustomValueCopy:
			if (cvc.p) {
				memcpy(buf, cvc.p, cvc.size);
			}
			break;
		case TypeCustomValueRef:
			*((typename ABITypeTraits::irmono_voidp*) buf) = abi.p2i_rmono_voidp(cvr);
			break;
		case TypeMonoObjectPtr:
			*((typename ABITypeTraits::irmono_gchandle*) buf) = abi.hp2i_RMonoObjectPtr(o);
			break;
		}
	}
}


template <typename ABI>
void RMonoVariant::updateFromRemoteMemory(ABI& abi, RMonoAPIBase& mono, void* buf)
{
	typedef RMonoABITypeTraits<ABI> ABITypeTraits;

	if (isNullPointer()) {
		return;
	}

	if ((flags & FlagOut) != 0) {
		switch (type) {
		case TypeInt8:
			*i8p = *((int8_t*) buf);
			break;
		case TypeUInt8:
			*u8p = *((uint8_t*) buf);
			break;
		case TypeInt16:
			*i16p = *((int16_t*) buf);
			break;
		case TypeUInt16:
			*u16p = *((uint16_t*) buf);
			break;
		case TypeInt32:
			*i32p = *((int32_t*) buf);
			break;
		case TypeUInt32:
			*u32p = *((uint32_t*) buf);
			break;
		case TypeInt64:
			*i64p = *((int64_t*) buf);
			break;
		case TypeUInt64:
			*u64p = *((uint64_t*) buf);
			break;
		case TypeFloat:
			*fp = *((float*) buf);
			break;
		case TypeDouble:
			*dp = *((double*) buf);
			break;
		case TypeBool:
			*bp = *((rmono_bool*) buf) != 0;
			break;
		case TypeCustomValueCopy:
			memcpy(cvc.p, buf, cvc.size);
			break;
		case TypeCustomValueRef:
			*cvrp = abi.i2p_rmono_voidp(*((typename ABITypeTraits::irmono_voidp*) buf));
			break;
		case TypeMonoObjectPtr:
			*op = abi.hi2p_RMonoObjectPtr(*((typename ABITypeTraits::irmono_gchandle*) buf), &mono);
			break;
		default:
			assert(false);
		}
	}
}



};
