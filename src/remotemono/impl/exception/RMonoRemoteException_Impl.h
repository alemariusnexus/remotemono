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

#include "RMonoRemoteException_Def.h"

#include "../RMonoAPI_Def.h"



namespace remotemono
{


void RMonoRemoteException::fetchRemoteData() noexcept
{
	if (dataFetched) {
		return;
	}

	try {
		auto mono = static_cast<RMonoAPI*>(ex.getMonoAPI());

		if (mono) {
			auto cls = mono->objectGetClass(ex);

			auto msgProp = mono->classGetPropertyFromName(cls, "Message");
			auto msgGet = mono->propertyGetGetMethod(msgProp);
			auto msgStr = mono->runtimeInvoke(msgGet, ex);
			message = mono->stringToUTF8(msgStr);

			toStrRes = mono->objectToStringUTF8(ex);
		}
	} catch (std::exception& ex) {
		RMonoLogError("RMonoRemoteException::fetchRemoteData() caught an exception: %s", ex.what());
	} catch (...) {
		RMonoLogError("RMonoRemoteException::fetchRemoteData() caught an exception.");
	}

	dataFetched = true;
}


}
