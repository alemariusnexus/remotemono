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

#include "RMonoHandle_Def.h"
#include "RMonoAPIBase_Def.h"
#include "backend/RMonoMemBlock.h"




namespace remotemono
{


template <typename PtrT>
void RMonoHandleAssemblyNamePtrDelete(PtrT p, RMonoAPIBase* mono)
{
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();
	apid->apply([&](auto& e) {
		// TODO: Some remotes (e.g. RedRunner) don't have mono_assembly_name_free(), but they do have
		// mono_assembly_name_parse(). How are we supposed to free then? I guess we'll just leak for now...
		if (e.api.assembly_name_free) {
			e.api.assembly_name_free(e.abi.p2i_RMonoAssemblyNamePtrRaw(p));
		}

		if (e.api.assembly_name_new) {
			if (e.api.free) {
				e.api.free(e.abi.p2i_rmono_voidp((rmono_voidp) p));
			} else if (e.api.g_free) {
				e.api.g_free(e.abi.p2i_rmono_voidp((rmono_voidp) p));
			} else {
				assert(false);
			}
		} else {
			backend::RMonoMemBlock block(&mono->getProcess(), static_cast<rmono_voidp>(p), true);
			block.free();
		}
	});
}


template <typename PtrT>
void RMonoHandleMethodDescPtrDelete(PtrT p, RMonoAPIBase* mono)
{
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();
	apid->apply([&](auto& e) {
		e.api.method_desc_free(e.abi.p2i_RMonoMethodDescPtrRaw(p));
	});
}


void RMonoObjectHandleDelete(rmono_gchandle gchandle, RMonoAPIBase* mono)
{
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();
	apid->apply([&](auto& e) {
		e.api.gchandle_free(e.abi.p2i_rmono_gchandle(gchandle));
	});
}





template<typename HandleT, void (*deleter)(HandleT, RMonoAPIBase*), HandleT invalidHandle>
void RMonoHandle<HandleT, deleter, invalidHandle>::Data::registerBackend()
{
	regIt = mono->registerMonoHandleBackend(this);
}


template<typename HandleT, void (*deleter)(HandleT, RMonoAPIBase*), HandleT invalidHandle>
void RMonoHandle<HandleT, deleter, invalidHandle>::Data::unregisterBackend()
{
	mono->unregisterMonoHandleBackend(regIt);
}





template <typename RawPtrT>
bool RMonoObjectHandle<RawPtrT>::operator==(const Self& other) const
{
	if (this == &other  ||  **this == *other) {
		return true;
	}
	// TODO: Maybe implement a custom remote function to do this more efficiently?
	auto pinnedThis = pin();
	return pinnedThis.raw() == other.raw();
}


template <typename RawPtrT>
typename RMonoObjectHandle<RawPtrT>::Self RMonoObjectHandle<RawPtrT>::pin() const
{
	if (!isValid()) {
		return *this;
	}
	RMonoAPIDispatcher* apid = d->mono->getAPIDispatcher();
	rmono_gchandle pinned = REMOTEMONO_GCHANDLE_INVALID;
	apid->apply([&](auto& e) {
		pinned = e.abi.i2p_rmono_gchandle(e.api.rmono_gchandle_pin(e.abi.p2i_rmono_gchandle(d->handle)));
	});
	return Self(pinned, d->mono);
}


template <typename RawPtrT>
typename RMonoObjectHandle<RawPtrT>::Self RMonoObjectHandle<RawPtrT>::clone() const
{
	if (!isValid()) {
		return *this;
	}
	RMonoAPIDispatcher* apid = d->mono->getAPIDispatcher();
	rmono_gchandle cloned = REMOTEMONO_GCHANDLE_INVALID;
	apid->apply([&](auto& e) {
		cloned = e.abi.i2p_rmono_gchandle(e.api.gchandle_new(*this, false));
	});
	return Self(cloned, d->mono);
}


template <typename RawPtrT>
typename RMonoObjectHandle<RawPtrT>::RawPtr RMonoObjectHandle<RawPtrT>::raw() const
{
	if (!isValid()) {
		return (RawPtr) 0;
	}
	RMonoAPIDispatcher* apid = d->mono->getAPIDispatcher();
	RawPtr p = (RawPtr) 0;
	apid->apply([&](auto& e) {
		p = e.abi.i2p_rmono_voidp(e.api.gchandle_get_target(e.abi.p2i_rmono_gchandle(d->handle)));
	});
	return p;
}


}
