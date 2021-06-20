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

#include "RMonoAPI_Def.h"

#include "mono/metadata/blob.h"
#include "mono/metadata/row-indexes.h"
#include "mono/metadata/tabledefs.h"
#include "../log.h"



#define REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(apiname)							\
	do {																		\
		apid->apply([&](auto& e) {												\
			checkAPIFunctionSupported(e.api.apiname);							\
		});																		\
	} while (0);




namespace remotemono
{



RMonoAPI::RMonoAPI(backend::RMonoProcess& process)
		: RMonoAPIBase(process), attached(false)
{
}


RMonoAPI::~RMonoAPI()
{
	detach();
}


void RMonoAPI::attach()
{
	if (attached) {
		return;
	}

	static bool versionPrinted = false;

	if (!versionPrinted) {
		RMonoLogInfo("RemoteMono version %u.%u.%u", REMOTEMONO_VERSION_MAJOR, REMOTEMONO_VERSION_MINOR, REMOTEMONO_VERSION_PATCH);
		versionPrinted = true;
	}

	RMonoLogInfo("Using backend: %s", process.getBackend()->getName().c_str());


	process.attach();

	selectABI();

	apid->apply([&](auto& e) {
		e.api.injectAPI(this, process);
	});

	attached = true;

	rootDomain = getRootDomain();

	monoThread = threadAttach(rootDomain);
}


void RMonoAPI::detach()
{
	if (!attached) {
		return;
	}

	rmono_gchandle monoThreadGchandle = *monoThread;
	monoThread.takeOwnership();

	size_t numRegisteredHandles = registeredHandles.size();

	if (numRegisteredHandles > 1) {
		RMonoLogDebug("%llu RemoteMonoHandles still reachable when detaching. Will force-delete them now.",
				(long long unsigned) (numRegisteredHandles-1));
	}

	for (RMonoHandleBackendBase* backend : registeredHandles) {
		backend->forceDelete();
	}
	registeredHandles.clear();

	threadDetach(monoThread);

	// TODO: It may be illegal to free the MonoThread's GCHandle (by calling gchandle_free(), as the destructor does) after we've already
	// detached from the thread. However, freeing it before detaching sounds even worse: That probably gives the GC an opportunity to
	// collect (or more likely: move) the MonoThread, which would leave us with an invalid MonoThreadPtrRaw so we can't safely detach.
	// I think this is the less dangerous option, because we likely don't even need to be attached to free a GCHandle.
	gchandleFree(monoThreadGchandle);
	monoThread.reset();

	apid->apply([&](auto& e) {
		e.api.uninjectAPI();
	});

	attached = false;
}


bool RMonoAPI::isAttached() const
{
	return attached;
}


bool RMonoAPI::isAPIFunctionSupported(const std::string& name) const
{
	return apid->apply([&](auto& e) {
		return e.api.isAPIFunctionSupported(name);
	});
}


void RMonoAPI::selectABI()
{
	SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);

    backend::RMonoProcessorArch arch = process.getProcessorArchitecture();

	apid->foreach([&](auto& e) {
		typedef decltype(e.abi) ABI;
		if (arch == backend::ProcessorArchX86_64  &&  sizeof(typename ABI::irmono_voidp) == 8) {
			apid->selectABI<ABI>();
		} else if (arch == backend::ProcessorArchX86  &&  sizeof(typename ABI::irmono_voidp) == 4) {
			apid->selectABI<ABI>();
		}
	});

	assert(apid->hasSelectedABI());

	apid->apply([&](auto& e) {
		RMonoLogDebug("Using Mono ABI: %s", typeid(e.abi).name());
	});
}


template <typename ABI>
backend::RMonoMemBlock RMonoAPI::prepareIterator()
{
	backend::RMonoMemBlock rIter = std::move(backend::RMonoMemBlock::alloc(&process,
			sizeof(typename ABI::irmono_voidp), PAGE_READWRITE));
	typename ABI::irmono_voidp nullPtr = 0;
	rIter.write(0, sizeof(typename ABI::irmono_voidp), &nullPtr);
	return std::move(rIter);
}


void RMonoAPI::checkAttached()
{
	if (!attached) {
		throw RMonoException("RMonoAPI is not attached.");
	}
}


template <typename FuncT>
void RMonoAPI::checkAPIFunctionSupported(const FuncT& f)
{
	if (!f) {
		throw RMonoUnsupportedAPIException(f.getName());
	}
}












// **************************************************************************
// *																		*
// *							MONO API WRAPPERS							*
// *																		*
// **************************************************************************


void RMonoAPI::free(rmono_voidp p)
{
	checkAttached();

	apid->apply([&](auto& e) {
		if (e.api.free) {
			e.api.free(e.abi.p2i_rmono_voidp(p));
		} else if (e.api.g_free) {
			e.api.g_free(e.abi.p2i_rmono_voidp(p));
		} else {
			throw RMonoUnsupportedAPIException("mono_free");
		}
	});
}



RMonoDomainPtr RMonoAPI::jitInit(const std::string_view& filename)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_init);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoDomainPtr(e.api.jit_init(filename));
	});
}


void RMonoAPI::jitCleanup(RMonoDomainPtr domain)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_cleanup);

	apid->apply([&](auto& e) {
		e.api.jit_cleanup(e.abi.p2i_RMonoDomainPtr(domain));
	});
}



RMonoDomainPtr RMonoAPI::getRootDomain()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_root_domain);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoDomainPtr(e.api.get_root_domain());
	});
}


bool RMonoAPI::domainSet(RMonoDomainPtr domain, bool force)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_set);

	return (bool) apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.domain_set(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_rmono_bool(force ? 1 : 0)));
	});
}


RMonoDomainPtr RMonoAPI::domainGet()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_get);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoDomainPtr(e.api.domain_get());
	});
}


std::vector<RMonoDomainPtr> RMonoAPI::domainList()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_foreach);

	std::vector<RMonoDomainPtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		std::vector<typename ABI::IRMonoDomainPtrRaw> iout;
		e.api.getIPCVector().vectorClear(e.api.getIPCVectorInstance());
		e.api.domain_foreach(e.abi.p2i_rmono_funcp((rmono_funcp) e.api.rmono_foreach_ipcvec_adapter.getAddress()),
				e.abi.p2i_rmono_voidp(e.api.getIPCVectorInstance()));
		e.api.getIPCVector().read(e.api.getIPCVectorInstance(), iout);
		for (auto p : iout) {
			out.push_back(e.abi.hi2p_RMonoDomainPtr(p, this, false));
		}
	});

	return out;
}


RMonoDomainPtr RMonoAPI::domainCreateAppdomain(const std::string_view& friendlyName, const std::string_view& configFile)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_create_appdomain);

	// TODO: Should be able to pass NULL for configFile
	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoDomainPtr(e.api.domain_create_appdomain(friendlyName, configFile));
	});
}


RMonoAssemblyPtr RMonoAPI::domainAssemblyOpen(RMonoDomainPtr domain, const std::string_view& name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_assembly_open);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoAssemblyPtr(e.api.domain_assembly_open(e.abi.p2i_RMonoDomainPtr(domain), name));
	});
}


void RMonoAPI::domainUnload(RMonoDomainPtr domain)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_unload);
	if (!domain) throw RMonoException("Invalid domain");

	apid->apply([&](auto& e) {
		e.api.domain_unload(e.abi.p2i_RMonoDomainPtr(domain));
	});
}


std::string RMonoAPI::domainGetFriendlyName(RMonoDomainPtr domain)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(domain_get_friendly_name);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		return e.api.domain_get_friendly_name(e.abi.p2i_RMonoDomainPtr(domain));
	});
}



RMonoThreadPtr RMonoAPI::threadAttach(RMonoDomainPtr domain)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(thread_attach);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoThreadPtr(e.api.thread_attach(e.abi.p2i_RMonoDomainPtr(domain)));
	});
}


void RMonoAPI::threadDetach(RMonoThreadPtr thread)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(thread_detach);
	if (!thread) throw RMonoException("Invalid thread");

	apid->apply([&](auto& e) {
		e.api.thread_detach(e.abi.p2i_RMonoThreadPtr(thread));
	});
}



void RMonoAPI::assemblyClose(RMonoAssemblyPtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_close);
	if (!assembly) throw RMonoException("Invalid assembly");

	apid->apply([&](auto& e) {
		e.api.assembly_close(e.abi.p2i_RMonoAssemblyPtr(assembly));
	});
}


std::vector<RMonoAssemblyPtr> RMonoAPI::assemblyList()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_foreach);

	std::vector<RMonoAssemblyPtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		std::vector<typename ABI::IRMonoAssemblyPtrRaw> iout;
		e.api.getIPCVector().vectorClear(e.api.getIPCVectorInstance());
		e.api.assembly_foreach(e.abi.p2i_rmono_funcp((rmono_funcp) e.api.rmono_foreach_ipcvec_adapter.getAddress()),
				e.abi.p2i_rmono_voidp(e.api.getIPCVectorInstance()));
		e.api.getIPCVector().read(e.api.getIPCVectorInstance(), iout);
		for (auto p : iout) {
			out.push_back(e.abi.hi2p_RMonoDomainPtr(p, this, false));
		}
	});

	return out;
}


RMonoImagePtr RMonoAPI::assemblyGetImage(RMonoAssemblyPtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_get_image);
	if (!assembly) throw RMonoException("Invalid assembly");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoImagePtr(e.api.assembly_get_image(e.abi.p2i_RMonoAssemblyPtr(assembly)));
	});
}


RMonoAssemblyNamePtr RMonoAPI::assemblyGetName(RMonoAssemblyPtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_get_name);
	if (!assembly) throw RMonoException("Invalid assembly");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoAssemblyNamePtr(e.api.assembly_get_name(e.abi.p2i_RMonoAssemblyPtr(assembly)));
	});
}


RMonoAssemblyNamePtr RMonoAPI::assemblyNameNew(const std::string_view& name)
{
	checkAttached();
	//REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_new);

	return apid->apply([&](auto& e) {
		if (e.api.assembly_name_new) {
			return e.abi.i2p_RMonoAssemblyNamePtr(e.api.assembly_name_new(name));
		} else if (e.api.assembly_name_parse) {
			backend::RMonoMemBlock block = std::move(backend::RMonoMemBlock::alloc(&process, 256, PAGE_READWRITE, false));
			RMonoAssemblyNamePtr aname((RMonoAssemblyNamePtrRaw) *block, this, true);
			if (!assemblyNameParse(name, aname)) {
				block.free();
				return RMonoAssemblyNamePtr();
			}
			return aname;
		} else {
			throw RMonoUnsupportedAPIException("assembly_name_new");
		}
	});
}


bool RMonoAPI::assemblyNameParse(const std::string_view& name, RMonoAssemblyNamePtr aname)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_parse);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.assembly_name_parse(name, e.abi.p2i_RMonoAssemblyNamePtr(aname))) != 0;
	});
}


void RMonoAPI::assemblyNameFree(RMonoAssemblyNamePtrRaw name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_free);

	apid->apply([&](auto& e) {
		e.api.assembly_name_free(e.abi.p2i_RMonoAssemblyNamePtrRaw(name));
	});
}


std::string RMonoAPI::assemblyNameGetName(RMonoAssemblyNamePtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_get_name);
	if (!assembly) throw RMonoException("Invalid assembly name");

	return apid->apply([&](auto& e) {
		return e.api.assembly_name_get_name(e.abi.p2i_RMonoAssemblyNamePtr(assembly));
	});
}


std::string RMonoAPI::assemblyNameGetCulture(RMonoAssemblyNamePtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_get_culture);
	if (!assembly) throw RMonoException("Invalid assembly name");

	return apid->apply([&](auto& e) {
		return e.api.assembly_name_get_culture(e.abi.p2i_RMonoAssemblyNamePtr(assembly));
	});
}


uint16_t RMonoAPI::assemblyNameGetVersion(RMonoAssemblyNamePtr assembly, uint16_t* minor, uint16_t* build, uint16_t* revision)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_name_get_version);
	if (!assembly) throw RMonoException("Invalid assembly name");

	return apid->apply([&](auto& e) {
		return e.api.assembly_name_get_version(e.abi.p2i_RMonoAssemblyNamePtr(assembly), minor, build, revision);
	});
}


std::string RMonoAPI::stringifyAssemblyName(RMonoAssemblyNamePtr assembly)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(stringify_assembly_name);
	if (!assembly) throw RMonoException("Invalid assembly name");

	return apid->apply([&](auto& e) {
		return e.api.stringify_assembly_name(e.abi.p2i_RMonoAssemblyNamePtr(assembly));
	});
}


std::string RMonoAPI::assemblyNameStringify(RMonoAssemblyNamePtr assembly)
{
	return stringifyAssemblyName(assembly);
}


RMonoAssemblyPtr RMonoAPI::assemblyLoaded(RMonoAssemblyNamePtr name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(assembly_loaded);
	if (!name) throw RMonoException("Invalid assembly name");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoAssemblyPtr(e.api.assembly_loaded(e.abi.p2i_RMonoAssemblyNamePtr(name)));
	});
}


RMonoAssemblyPtr RMonoAPI::assemblyLoaded(const std::string_view& name)
{
	return assemblyLoaded(assemblyNameNew(name));
}



std::string RMonoAPI::imageGetName(RMonoImagePtr image)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(image_get_name);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.api.image_get_name(e.abi.p2i_RMonoImagePtr(image));
	});
}


std::string RMonoAPI::imageGetFilename(RMonoImagePtr image)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(image_get_filename);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.api.image_get_filename(e.abi.p2i_RMonoImagePtr(image));
	});
}


RMonoTableInfoPtr RMonoAPI::imageGetTableInfo(RMonoImagePtr image, rmono_int tableID)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(image_get_table_info);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoTableInfoPtr(e.api.image_get_table_info(e.abi.p2i_RMonoImagePtr(image), e.abi.p2i_rmono_int(tableID)));
	});
}


rmono_int RMonoAPI::tableInfoGetRows(RMonoTableInfoPtr table)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(table_info_get_rows);
	if (!table) throw RMonoException("Invalid table info");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_int(e.api.table_info_get_rows(e.abi.p2i_RMonoTableInfoPtr(table)));
	});
}


rmono_voidp RMonoAPI::imageRVAMap(RMonoImagePtr image, uint32_t addr)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(image_rva_map);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_voidp(e.api.image_rva_map(e.abi.p2i_RMonoImagePtr(image), addr));
	});
}



uint32_t RMonoAPI::metadataDecodeRowCol(RMonoTableInfoPtr table, rmono_int idx, rmono_uint col)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_decode_row_col);
	if (!table) throw RMonoException("Invalid table info");

	return apid->apply([&](auto& e) {
		return e.api.metadata_decode_row_col(e.abi.p2i_RMonoTableInfoPtr(table), e.abi.p2i_rmono_int(idx), e.abi.p2i_rmono_uint(col));
	});
}


rmono_voidp RMonoAPI::metadataGuidHeap(RMonoImagePtr image, uint32_t idx, uint8_t* outGuid)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_guid_heap);
	if (!image) throw RMonoException("Invalid image");

	rmono_voidp p = apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_voidp(e.api.metadata_guid_heap(e.abi.p2i_RMonoImagePtr(image), idx));
	});

	if (outGuid) {
		process.readMemory(p, 16, outGuid);
	}

	return p;
}


std::string RMonoAPI::metadataStringHeap(RMonoImagePtr image, uint32_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_string_heap);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.api.metadata_string_heap(e.abi.p2i_RMonoImagePtr(image), idx);
	});
}


rmono_voidp RMonoAPI::metadataStringHeapRaw(RMonoImagePtr image, uint32_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_string_heap);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_voidp(e.api.metadata_string_heap.invokeRaw(e.abi.p2i_RMonoImagePtrRaw(*image), idx));
	});
}


rmono_voidp RMonoAPI::metadataBlobHeap(RMonoImagePtr image, uint32_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_blob_heap);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_voidp(e.api.metadata_blob_heap(e.abi.p2i_RMonoImagePtr(image), idx));
	});
}


std::string RMonoAPI::metadataUserString(RMonoImagePtr image, uint32_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_user_string);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.api.metadata_user_string(e.abi.p2i_RMonoImagePtr(image), idx);
	});
}


rmono_voidp RMonoAPI::metadataUserStringRaw(RMonoImagePtr image, uint32_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_user_string);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_voidp(e.api.metadata_user_string.invokeRaw(e.abi.p2i_RMonoImagePtrRaw(*image), idx));
	});
}


uint32_t RMonoAPI::metadataDecodeBlobSize(rmono_voidp blobPtr, rmono_voidp* outBlobPtr)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(metadata_decode_blob_size);

	return apid->apply([&](auto& e) {
		typedef typename decltype(e.abi)::irmono_voidp irmono_voidp;

		irmono_voidp ip;
		uint32_t size = e.api.metadata_decode_blob_size(e.abi.p2i_rmono_voidp(blobPtr), &ip);

		*outBlobPtr = e.abi.i2p_rmono_voidp(ip);
		return size;
	});
}



RMonoClassPtr RMonoAPI::getObjectClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_object_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_object_class());
	});
}


RMonoClassPtr RMonoAPI::getInt16Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_int16_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_int16_class());
	});
}


RMonoClassPtr RMonoAPI::getInt32Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_int32_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_int32_class());
	});
}


RMonoClassPtr RMonoAPI::getInt64Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_int64_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_int64_class());
	});
}


RMonoClassPtr RMonoAPI::getDoubleClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_double_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_double_class());
	});
}


RMonoClassPtr RMonoAPI::getSingleClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_single_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_single_class());
	});
}


RMonoClassPtr RMonoAPI::getStringClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_string_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_string_class());
	});
}


RMonoClassPtr RMonoAPI::getThreadClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_thread_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_thread_class());
	});
}


RMonoClassPtr RMonoAPI::getUInt16Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_uint16_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_uint16_class());
	});
}


RMonoClassPtr RMonoAPI::getUInt32Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_uint32_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_uint32_class());
	});
}


RMonoClassPtr RMonoAPI::getUInt64Class()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_uint64_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_uint64_class());
	});
}


RMonoClassPtr RMonoAPI::getVoidClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_void_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_void_class());
	});
}


RMonoClassPtr RMonoAPI::getArrayClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_array_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_array_class());
	});
}


RMonoClassPtr RMonoAPI::getBooleanClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_boolean_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_boolean_class());
	});
}


RMonoClassPtr RMonoAPI::getByteClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_byte_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_byte_class());
	});
}


RMonoClassPtr RMonoAPI::getSByteClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_sbyte_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_sbyte_class());
	});
}


RMonoClassPtr RMonoAPI::getCharClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_char_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_char_class());
	});
}


RMonoClassPtr RMonoAPI::getExceptionClass()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(get_exception_class);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.get_exception_class());
	});
}



RMonoVTablePtr RMonoAPI::classVTable(RMonoDomainPtr domain, RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_vtable);
	if (!domain) throw RMonoException("Invalid domain");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoVTablePtr(e.api.class_vtable(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoClassPtr(cls)));
	});
}


RMonoVTablePtr RMonoAPI::classVTable(RMonoClassPtr cls)
{
	return classVTable(domainGet(), cls);
}


void RMonoAPI::runtimeClassInit(RMonoVTablePtr vtable)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(runtime_class_init);
	if (!vtable) throw RMonoException("Invalid vtable");

	apid->apply([&](auto& e) {
		e.api.runtime_class_init(e.abi.p2i_RMonoVTablePtr(vtable));
	});
}


RMonoClassPtr RMonoAPI::classGetParent(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_parent);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.class_get_parent(e.abi.p2i_RMonoClassPtr(cls)));
	});
}


RMonoTypePtr RMonoAPI::classGetType(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_type);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoTypePtr(e.api.class_get_type(e.abi.p2i_RMonoClassPtr(cls)));
	});
}


RMonoClassPtr RMonoAPI::classFromName(RMonoImagePtr image, const std::string_view& nameSpace, const std::string_view& name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_from_name);
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.class_from_name(e.abi.p2i_RMonoImagePtr(image), nameSpace, name));
	});
}


RMonoClassPtr RMonoAPI::classFromMonoType(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_from_mono_type);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.class_from_mono_type(e.abi.p2i_RMonoTypePtr(type)));
	});
}


std::string RMonoAPI::classGetName(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_name);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_get_name(e.abi.p2i_RMonoClassPtr(cls));
	});
}


std::string RMonoAPI::classGetNamespace(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_namespace);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_get_namespace(e.abi.p2i_RMonoClassPtr(cls));
	});
}


std::vector<RMonoClassFieldPtr> RMonoAPI::classGetFields(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_fields);
	if (!cls) throw RMonoException("Invalid class");

	std::vector<RMonoClassFieldPtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;

		backend::RMonoMemBlock rIter = prepareIterator<ABI>();
		typename ABI::irmono_voidpp iptr = (typename ABI::irmono_voidpp) *rIter;

		typename ABI::IRMonoClassFieldPtr field;
		typename ABI::IRMonoClassPtr icls = e.abi.p2i_RMonoClassPtr(cls);

		do {
			field = e.api.class_get_fields(icls, iptr);
			if (field) {
				out.push_back(e.abi.i2p_RMonoClassFieldPtr(field));
			}
		} while (field);
	});

	return out;
}


std::vector<RMonoMethodPtr> RMonoAPI::classGetMethods(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_methods);
	if (!cls) throw RMonoException("Invalid class");

	std::vector<RMonoMethodPtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;

		backend::RMonoMemBlock rIter = prepareIterator<ABI>();
		typename ABI::irmono_voidpp iptr = (typename ABI::irmono_voidpp) *rIter;

		typename ABI::IRMonoMethodPtr method;
		typename ABI::IRMonoClassPtr icls = e.abi.p2i_RMonoClassPtr(cls);

		do {
			method = e.api.class_get_methods(icls, iptr);
			if (method) {
				out.push_back(e.abi.i2p_RMonoMethodPtr(method));
			}
		} while (method);
	});

	return out;
}


std::vector<RMonoPropertyPtr> RMonoAPI::classGetProperties(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_properties);
	if (!cls) throw RMonoException("Invalid class");

	std::vector<RMonoPropertyPtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;

		backend::RMonoMemBlock rIter = prepareIterator<ABI>();
		typename ABI::irmono_voidpp iptr = (typename ABI::irmono_voidpp) *rIter;

		typename ABI::IRMonoPropertyPtr prop;
		typename ABI::IRMonoClassPtr icls = e.abi.p2i_RMonoClassPtr(cls);

		do {
			prop = e.api.class_get_properties(icls, iptr);
			if (prop) {
				out.push_back(e.abi.i2p_RMonoPropertyPtr(prop));
			}
		} while (prop);
	});

	return out;
}


RMonoClassFieldPtr RMonoAPI::classGetFieldFromName(RMonoClassPtr cls, const std::string_view& name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_field_from_name);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassFieldPtr(e.api.class_get_field_from_name(e.abi.p2i_RMonoClassPtr(cls), name));
	});
}


RMonoMethodPtr RMonoAPI::classGetMethodFromName(RMonoClassPtr cls, const std::string_view& name, int32_t paramCount)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_method_from_name);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.class_get_method_from_name(e.abi.p2i_RMonoClassPtr(cls), name, paramCount));
	});
}


RMonoPropertyPtr RMonoAPI::classGetPropertyFromName(RMonoClassPtr cls, const std::string_view& name)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_property_from_name);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoPropertyPtr(e.api.class_get_property_from_name(e.abi.p2i_RMonoClassPtr(cls), name));
	});
}


RMonoClassPtr RMonoAPI::classGetElementClass(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_element_class);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.class_get_element_class(e.abi.p2i_RMonoClassPtr(cls)));
	});
}


uint32_t RMonoAPI::classGetFlags(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_flags);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_get_flags(e.abi.p2i_RMonoClassPtr(cls));
	});
}


rmono_int RMonoAPI::classGetRank(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_get_rank);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_int(e.api.class_get_rank(e.abi.p2i_RMonoClassPtr(cls)));
	});
}


bool RMonoAPI::classIsValueType(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_is_valuetype);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.class_is_valuetype(e.abi.p2i_RMonoClassPtr(cls))) != 0;
	});
}


uint32_t RMonoAPI::classDataSize(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_data_size);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_data_size(e.abi.p2i_RMonoClassPtr(cls));
	});
}


uint32_t RMonoAPI::classInstanceSize(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_instance_size);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_instance_size(e.abi.p2i_RMonoClassPtr(cls));
	});
}


int32_t RMonoAPI::classValueSize(RMonoClassPtr cls, uint32_t* align)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_value_size);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_value_size(e.abi.p2i_RMonoClassPtr(cls), align);
	});
}



RMonoReflectionTypePtr RMonoAPI::typeGetObject(RMonoDomainPtr domain, RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_get_object);
	if (!domain) throw RMonoException("Invalid domain");
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoReflectionTypePtr(e.api.type_get_object(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoTypePtr(type)));
	});
}


RMonoReflectionTypePtr RMonoAPI::typeGetObject(RMonoTypePtr type)
{
	return typeGetObject(domainGet(), type);
}


std::string RMonoAPI::typeGetName(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_get_name);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.api.type_get_name(e.abi.p2i_RMonoTypePtr(type));
	});
}


RMonoClassPtr RMonoAPI::typeGetClass(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_get_class);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.type_get_class(e.abi.p2i_RMonoTypePtr(type)));
	});
}


rmono_int RMonoAPI::typeGetType(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_get_type);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_int(e.api.type_get_type(e.abi.p2i_RMonoTypePtr(type)));
	});
}


bool RMonoAPI::typeIsByRef(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_is_byref);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.type_is_byref(e.abi.p2i_RMonoTypePtr(type))) != 0;
	});
}


bool RMonoAPI::typeIsPointer(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_is_pointer);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.type_is_pointer(e.abi.p2i_RMonoTypePtr(type))) != 0;
	});
}


bool RMonoAPI::typeIsReference(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_is_reference);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.type_is_reference(e.abi.p2i_RMonoTypePtr(type))) != 0;
	});
}


bool RMonoAPI::typeIsStruct(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_is_struct);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.type_is_struct(e.abi.p2i_RMonoTypePtr(type))) != 0;
	});
}


bool RMonoAPI::typeIsVoid(RMonoTypePtr type)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_is_void);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.type_is_void(e.abi.p2i_RMonoTypePtr(type))) != 0;
	});
}


rmono_int RMonoAPI::typeSize(RMonoTypePtr type, rmono_int* align)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_size);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		typedef typename decltype(e.abi)::irmono_int irmono_int;
		irmono_int ialign;
		rmono_int size = e.abi.i2p_rmono_int(e.api.type_size(e.abi.p2i_RMonoTypePtr(type), &ialign));
		if (align) {
			*align = e.abi.i2p_rmono_int(ialign);
		}
		return size;
	});
}


rmono_int RMonoAPI::typeStackSize(RMonoTypePtr type, rmono_int* align)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(type_stack_size);
	if (!type) throw RMonoException("Invalid type");

	return apid->apply([&](auto& e) {
		typedef typename decltype(e.abi)::irmono_int irmono_int;
		irmono_int ialign;
		rmono_int size = e.abi.i2p_rmono_int(e.api.type_stack_size(e.abi.p2i_RMonoTypePtr(type), &ialign));
		if (align) {
			*align = e.abi.i2p_rmono_int(ialign);
		}
		return size;
	});
}



RMonoClassPtr RMonoAPI::fieldGetParent(RMonoClassFieldPtr field)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_parent);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.field_get_parent(e.abi.p2i_RMonoClassFieldPtr(field)));
	});
}


RMonoTypePtr RMonoAPI::fieldGetType(RMonoClassFieldPtr field)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_type);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoTypePtr(e.api.field_get_type(e.abi.p2i_RMonoClassFieldPtr(field)));
	});
}


std::string RMonoAPI::fieldGetName(RMonoClassFieldPtr field)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_name);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.api.field_get_name(e.abi.p2i_RMonoClassFieldPtr(field));
	});
}


uint32_t RMonoAPI::fieldGetFlags(RMonoClassFieldPtr field)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_flags);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.api.field_get_flags(e.abi.p2i_RMonoClassFieldPtr(field));
	});
}


void RMonoAPI::fieldSetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, const RMonoVariant& val)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_set_value);
	if (!field) throw RMonoException("Invalid field");

	apid->apply([&](auto& e) {
		if (obj) {
			e.api.field_set_value(e.abi.p2i_RMonoObjectPtr(obj), e.abi.p2i_RMonoClassFieldPtr(field), val);
		} else {
			RMonoClassPtr cls = fieldGetParent(field);
			RMonoVTablePtr vtable = classVTable(domainGet(), cls);
			fieldStaticSetValue(vtable, field, val);
		}
	});
}


void RMonoAPI::fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, RMonoVariant& val)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_value);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		if (obj) {
			e.api.field_get_value(e.abi.p2i_RMonoObjectPtr(obj), e.abi.p2i_RMonoClassFieldPtr(field), val);
		} else {
			RMonoClassPtr cls = fieldGetParent(field);
			RMonoVTablePtr vtable = classVTable(domainGet(), cls);
			fieldStaticGetValue(vtable, field, val);
		}
	});
}


void RMonoAPI::fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, RMonoVariant&& val)
{
	fieldGetValue(obj, field, val);
}


template <typename T>
T RMonoAPI::fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field)
{
	T val = T();
	if constexpr(std::is_base_of_v<RMonoObjectHandleTag, T>) {
		fieldGetValue(obj, field, &val);
	} else {
		fieldGetValue(obj, field, RMonoVariant(&val));
	}
	return val;
}


RMonoObjectPtr RMonoAPI::fieldGetValueObject(RMonoDomainPtr domain, RMonoClassFieldPtr field, RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_value_object);
	if (!domain) throw RMonoException("Invalid domain");
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.field_get_value_object (
				e.abi.p2i_RMonoDomainPtr(domain),
				e.abi.p2i_RMonoClassFieldPtr(field),
				e.abi.p2i_RMonoObjectPtr(obj)
				));
	});
}


RMonoObjectPtr RMonoAPI::fieldGetValueObject(RMonoClassFieldPtr field, RMonoObjectPtr obj)
{
	return fieldGetValueObject(domainGet(), field, obj);
}


void RMonoAPI::fieldStaticSetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, const RMonoVariant& val)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_static_set_value);
	if (!vtable) throw RMonoException("Invalid vtable");
	if (!field) throw RMonoException("Invalid field");

	apid->apply([&](auto& e) {
		e.api.field_static_set_value(e.abi.p2i_RMonoVTablePtr(vtable), e.abi.p2i_RMonoClassFieldPtr(field), val);
	});
}


void RMonoAPI::fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, RMonoVariant& val)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_static_get_value);
	if (!vtable) throw RMonoException("Invalid vtable");
	if (!field) throw RMonoException("Invalid field");

	apid->apply([&](auto& e) {
		e.api.field_static_get_value(e.abi.p2i_RMonoVTablePtr(vtable), e.abi.p2i_RMonoClassFieldPtr(field), val);
	});
}


void RMonoAPI::fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, RMonoVariant&& val)
{
	fieldStaticGetValue(vtable, field, val);
}


template <typename T>
T RMonoAPI::fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field)
{
	T val = T();
	if constexpr(std::is_base_of_v<RMonoObjectHandleTag, T>) {
		fieldStaticGetValue(vtable, field, &val);
	} else {
		fieldStaticGetValue(vtable, field, RMonoVariant(&val));
	}
	return val;
}


uint32_t RMonoAPI::fieldGetOffset(RMonoClassFieldPtr field)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(field_get_offset);
	if (!field) throw RMonoException("Invalid field");

	return apid->apply([&](auto& e) {
		return e.api.field_get_offset(e.abi.p2i_RMonoClassFieldPtr(field));
	});
}



RMonoClassPtr RMonoAPI::methodGetClass(RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_get_class);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.method_get_class(e.abi.p2i_RMonoMethodPtr(method)));
	});
}


std::string RMonoAPI::methodGetName(RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_get_name);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.api.method_get_name(e.abi.p2i_RMonoMethodPtr(method));
	});
}


std::string RMonoAPI::methodFullName(RMonoMethodPtr method, bool signature)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_full_name);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.api.method_full_name(e.abi.p2i_RMonoMethodPtr(method), e.abi.p2i_rmono_bool(signature ? 1 : 0));
	});
}


uint32_t RMonoAPI::methodGetFlags(RMonoMethodPtr method, uint32_t* iflags)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_get_flags);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.api.method_get_flags(e.abi.p2i_RMonoMethodPtr(method), iflags);
	});
}


RMonoMethodSignaturePtr RMonoAPI::methodSignature(RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_signature);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodSignaturePtr(e.api.method_signature(e.abi.p2i_RMonoMethodPtr(method)));
	});
}


RMonoMethodHeaderPtr RMonoAPI::methodGetHeader(RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_get_header);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodHeaderPtr(e.api.method_get_header(e.abi.p2i_RMonoMethodPtr(method)));
	});
}


rmono_funcp RMonoAPI::methodHeaderGetCode(RMonoMethodHeaderPtr header, uint32_t* codeSize, uint32_t* maxStack)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_header_get_code);
	if (!header) throw RMonoException("Invalid method header");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_funcp(e.api.method_header_get_code(e.abi.p2i_RMonoMethodHeaderPtr(header), codeSize, maxStack));
	});
}


RMonoMethodDescPtr RMonoAPI::methodDescNew(const std::string_view& name, bool includeNamespace)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_desc_new);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodDescPtr(e.api.method_desc_new(name, includeNamespace));
	});
}


void RMonoAPI::methodDescFree(RMonoMethodDescPtrRaw desc)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_desc_free);

	return apid->apply([&](auto& e) {
		e.api.method_desc_free(e.abi.p2i_RMonoMethodDescPtrRaw(desc));
	});
}


bool RMonoAPI::methodDescMatch(RMonoMethodDescPtr desc, RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_desc_match);
	if (!desc) throw RMonoException("Invalid method desc");
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.method_desc_match(e.abi.p2i_RMonoMethodDescPtr(desc), e.abi.p2i_RMonoMethodPtr(method))) != 0;
	});
}


RMonoMethodPtr RMonoAPI::methodDescSearchInClass(RMonoMethodDescPtr desc, RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_desc_search_in_class);
	if (!desc) throw RMonoException("Invalid method desc");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.method_desc_search_in_class(e.abi.p2i_RMonoMethodDescPtr(desc), e.abi.p2i_RMonoClassPtr(cls)));
	});
}


RMonoMethodPtr RMonoAPI::methodDescSearchInClass(const std::string_view& desc, bool includeNamespace, RMonoClassPtr cls)
{
	return methodDescSearchInClass(methodDescNew(desc, includeNamespace), cls);
}


RMonoMethodPtr RMonoAPI::methodDescSearchInImage(RMonoMethodDescPtr desc, RMonoImagePtr image)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(method_desc_search_in_image);
	if (!desc) throw RMonoException("Invalid method desc");
	if (!image) throw RMonoException("Invalid image");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.method_desc_search_in_image(e.abi.p2i_RMonoMethodDescPtr(desc), e.abi.p2i_RMonoImagePtr(image)));
	});
}


RMonoMethodPtr RMonoAPI::methodDescSearchInImage(const std::string_view& desc, bool includeNamespace, RMonoImagePtr image)
{
	return methodDescSearchInImage(methodDescNew(desc, includeNamespace), image);
}



std::string RMonoAPI::propertyGetName(RMonoPropertyPtr prop)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_name);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.api.property_get_name(e.abi.p2i_RMonoPropertyPtr(prop));
	});
}


uint32_t RMonoAPI::propertyGetFlags(RMonoPropertyPtr prop)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_flags);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.api.property_get_flags(e.abi.p2i_RMonoPropertyPtr(prop));
	});
}


RMonoClassPtr RMonoAPI::propertyGetParent(RMonoPropertyPtr prop)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_parent);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.property_get_parent(e.abi.p2i_RMonoPropertyPtr(prop)));
	});
}


RMonoMethodPtr RMonoAPI::propertyGetSetMethod(RMonoPropertyPtr prop)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_set_method);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.property_get_set_method(e.abi.p2i_RMonoPropertyPtr(prop)));
	});
}


RMonoMethodPtr RMonoAPI::propertyGetGetMethod(RMonoPropertyPtr prop)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_get_method);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.property_get_get_method(e.abi.p2i_RMonoPropertyPtr(prop)));
	});
}


RMonoObjectPtr RMonoAPI::propertyGetValue(RMonoPropertyPtr prop, const RMonoVariant& obj, RMonoVariantArray& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_value);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.property_get_value(e.abi.p2i_RMonoPropertyPtr(prop), obj, params, catchExceptions));
	});
}


RMonoObjectPtr RMonoAPI::propertyGetValue(RMonoPropertyPtr prop, const RMonoVariant& obj, RMonoVariantArray&& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_get_value);
	if (!prop) throw RMonoException("Invalid property");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.property_get_value(e.abi.p2i_RMonoPropertyPtr(prop), obj, params, catchExceptions));
	});
}


void RMonoAPI::propertySetValue(RMonoPropertyPtr prop, const RMonoVariant& obj, RMonoVariantArray& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_set_value);
	if (!prop) throw RMonoException("Invalid property");

	apid->apply([&](auto& e) {
		e.api.property_set_value(e.abi.p2i_RMonoPropertyPtr(prop), obj, params, catchExceptions);
	});
}


void RMonoAPI::propertySetValue(RMonoPropertyPtr prop, const RMonoVariant& obj, RMonoVariantArray&& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(property_set_value);
	if (!prop) throw RMonoException("Invalid property");

	apid->apply([&](auto& e) {
		e.api.property_set_value(e.abi.p2i_RMonoPropertyPtr(prop), obj, params, catchExceptions);
	});
}



RMonoTypePtr RMonoAPI::signatureGetReturnType(RMonoMethodSignaturePtr sig)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(signature_get_return_type);
	if (!sig) throw RMonoException("Invalid method signature");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoTypePtr(e.api.signature_get_return_type(e.abi.p2i_RMonoMethodSignaturePtr(sig)));
	});
}


uint32_t RMonoAPI::signatureGetCallConv(RMonoMethodSignaturePtr sig)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(signature_get_call_conv);
	if (!sig) throw RMonoException("Invalid method signature");

	return apid->apply([&](auto& e) {
		return e.api.signature_get_call_conv(e.abi.p2i_RMonoMethodSignaturePtr(sig));
	});
}


std::string RMonoAPI::signatureGetDesc(RMonoMethodSignaturePtr sig, bool includeNamespace)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(signature_get_desc);
	if (!sig) throw RMonoException("Invalid method signature");

	return apid->apply([&](auto& e) {
		return e.api.signature_get_desc(e.abi.p2i_RMonoMethodSignaturePtr(sig), e.abi.p2i_rmono_bool(includeNamespace ? 1 : 0));
	});
}


std::vector<RMonoTypePtr> RMonoAPI::signatureGetParams(RMonoMethodSignaturePtr sig)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(signature_get_params);
	if (!sig) throw RMonoException("Invalid method signature");

	std::vector<RMonoTypePtr> out;

	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;

		backend::RMonoMemBlock rIter = prepareIterator<ABI>();
		typename ABI::irmono_voidpp iptr = (typename ABI::irmono_voidpp) *rIter;

		typename ABI::IRMonoTypePtr param;
		typename ABI::IRMonoMethodSignaturePtr isig = e.abi.p2i_RMonoMethodSignaturePtr(sig);

		do {
			param = e.api.signature_get_params(isig, iptr);
			if (param) {
				out.push_back(e.abi.i2p_RMonoTypePtr(param));
			}
		} while (param);
	});

	return out;
}



RMonoClassPtr RMonoAPI::objectGetClass(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_get_class);
	if (!obj) throw RMonoException("Invalid object");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.object_get_class(e.abi.p2i_RMonoObjectPtr(obj)));
	});
}


RMonoObjectPtr RMonoAPI::objectNew(RMonoDomainPtr domain, RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_new);
	if (!domain) throw RMonoException("Invalid domain");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.object_new(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoClassPtr(cls)));
	});
}


RMonoObjectPtr RMonoAPI::objectNew(RMonoClassPtr cls)
{
	return objectNew(domainGet(), cls);
}


void RMonoAPI::runtimeObjectInit(const RMonoVariant& obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(runtime_object_init);

	apid->apply([&](auto& e) {
		e.api.runtime_object_init(obj);
	});
}


template <typename T>
T RMonoAPI::objectUnbox(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_unbox);
	if (!obj) throw RMonoException("Invalid object");

	T res;
	RMonoVariant var(&res);
	apid->apply([&](auto& e) {
		e.api.object_unbox(var, e.abi.p2i_RMonoObjectPtr(obj));
	});
	return res;
}


RMonoVariant RMonoAPI::objectUnboxRaw(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_unbox);
	if (!obj) throw RMonoException("Invalid object");

	rmono_voidp p;
	RMonoVariant var(&p, RMonoVariant::rawPtr);
	apid->apply([&](auto& e) {
		e.api.object_unbox(var, e.abi.p2i_RMonoObjectPtr(obj));
	});
	return RMonoVariant(p, RMonoVariant::rawPtr);
}


RMonoObjectPtr RMonoAPI::valueBox(RMonoDomainPtr domain, RMonoClassPtr cls, const RMonoVariant& val)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(value_box);
	if (!domain) throw RMonoException("Invalid domain");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.value_box(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoClassPtr(cls), val));
	});
}


RMonoObjectPtr RMonoAPI::valueBox(RMonoClassPtr cls, const RMonoVariant& val)
{
	return valueBox(domainGet(), cls, val);
}


RMonoStringPtr RMonoAPI::objectToString(const RMonoVariant& obj, bool catchExceptions)
{
	checkAttached();

	return apid->apply([&](auto& e) {
		if (e.api.object_to_string) {
			return e.abi.i2p_RMonoStringPtr(e.api.object_to_string(obj, catchExceptions));
		} else {
			// TODO: Maybe support value types here?
			//		 UPDATE: Now that we're using objectGetVirtualMethod(), maybe value types will work out of the box?
			assert(obj.getType() == RMonoVariant::TypeMonoObjectPtr);

			auto toStr = classGetMethodFromName(getObjectClass(), "ToString", 0);
			auto virtualToStr = objectGetVirtualMethod(obj.getMonoObjectPtr(), toStr);
			return (RMonoStringPtr) runtimeInvoke(virtualToStr, obj.getMonoObjectPtr(), {}, catchExceptions);
		}
	});
}


RMonoObjectPtr RMonoAPI::objectClone(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_clone);
	if (!obj) throw RMonoException("Invalid object");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.object_clone(e.abi.p2i_RMonoObjectPtr(obj)));
	});
}


RMonoDomainPtr RMonoAPI::objectGetDomain(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_get_domain);
	if (!obj) throw RMonoException("Invalid object");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoDomainPtr(e.api.object_get_domain(e.abi.p2i_RMonoObjectPtr(obj)));
	});
}


RMonoMethodPtr RMonoAPI::objectGetVirtualMethod(RMonoObjectPtr obj, RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_get_virtual_method);
	if (!obj) throw RMonoException("Invalid object");
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.object_get_virtual_method(e.abi.p2i_RMonoObjectPtr(obj), e.abi.p2i_RMonoMethodPtr(method)));
	});
}


RMonoObjectPtr RMonoAPI::objectIsInst(RMonoObjectPtr obj, RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_isinst);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.object_isinst(e.abi.p2i_RMonoObjectPtr(obj), e.abi.p2i_RMonoClassPtr(cls)));
	});
}


rmono_uint RMonoAPI::objectGetSize(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(object_get_size);
	if (!obj) throw RMonoException("Invalid object");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_uint(e.api.object_get_size(e.abi.p2i_RMonoObjectPtr(obj)));
	});
}



RMonoStringPtr RMonoAPI::stringNew(RMonoDomainPtr domain, const std::string_view& str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_new_len);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		return e.abi.i2p_RMonoStringPtr(e.api.string_new_len(e.abi.p2i_RMonoDomainPtr(domain), str, (typename ABI::irmono_uint) str.size()));
	});
}


RMonoStringPtr RMonoAPI::stringNew(const std::string_view& str)
{
	return stringNew(domainGet(), str);
}


RMonoStringPtr RMonoAPI::stringNewUTF16(RMonoDomainPtr domain, const std::u16string_view& str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_new_utf16);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		return e.abi.i2p_RMonoStringPtr(e.api.string_new_utf16(e.abi.p2i_RMonoDomainPtr(domain), str, (int32_t) str.size()));
	});
}


RMonoStringPtr RMonoAPI::stringNewUTF16(const std::u16string_view& str)
{
	return stringNewUTF16(domainGet(), str);
}


RMonoStringPtr RMonoAPI::stringNewUTF32(RMonoDomainPtr domain, const std::u32string_view& str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_new_utf32);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		return e.abi.i2p_RMonoStringPtr(e.api.string_new_utf32(e.abi.p2i_RMonoDomainPtr(domain), str, (int32_t) str.size()));
	});
}


RMonoStringPtr RMonoAPI::stringNewUTF32(const std::u32string_view& str)
{
	return stringNewUTF32(domainGet(), str);
}


std::string RMonoAPI::stringToUTF8(RMonoStringPtr str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_to_utf8);

	return apid->apply([&](auto& e) {
		return e.api.string_to_utf8(e.abi.p2i_RMonoStringPtr(str));
	});
}


std::u16string RMonoAPI::stringToUTF16(RMonoStringPtr str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_to_utf16);

	return apid->apply([&](auto& e) {
		return e.api.string_to_utf16(e.abi.p2i_RMonoStringPtr(str));
	});
}


std::u32string RMonoAPI::stringToUTF32(RMonoStringPtr str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_to_utf32);

	return apid->apply([&](auto& e) {
		return e.api.string_to_utf32(e.abi.p2i_RMonoStringPtr(str));
	});
}


std::u16string RMonoAPI::stringChars(RMonoStringPtr str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_chars);

	return apid->apply([&](auto& e) {
		return e.api.string_chars(e.abi.p2i_RMonoStringPtr(str));
	});
}


int32_t RMonoAPI::stringLength(RMonoStringPtr str)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_length);
	if (!str) throw RMonoException("Invalid string");

	return apid->apply([&](auto& e) {
		return e.api.string_length(e.abi.p2i_RMonoStringPtr(str));
	});
}


bool RMonoAPI::stringEqual(RMonoStringPtr a, RMonoStringPtr b)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(string_equal);
	if (!a  ||  !b) throw RMonoException("Invalid string");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_bool(e.api.string_equal(e.abi.p2i_RMonoStringPtr(a), e.abi.p2i_RMonoStringPtr(b))) != 0;
	});
}



RMonoArrayPtr RMonoAPI::arrayNew(RMonoDomainPtr domain, RMonoClassPtr cls, rmono_uintptr_t n)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_new);
	if (!domain) throw RMonoException("Invalid domain");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoArrayPtr(e.api.array_new(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoClassPtr(cls),
				e.abi.p2i_rmono_uintptr_t(n)));
	});
}


RMonoArrayPtr RMonoAPI::arrayNew(RMonoClassPtr cls, rmono_uintptr_t n)
{
	return arrayNew(domainGet(), cls, n);
}


RMonoArrayPtr RMonoAPI::arrayNewFull (
		RMonoDomainPtr domain,
		RMonoClassPtr cls,
		const std::vector<rmono_uintptr_t>& lengths,
		const std::vector<rmono_intptr_t>& lowerBounds
) {
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_new_full);
	if (!domain) throw RMonoException("Invalid domain");
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;
		typedef typename ABI::irmono_voidp irmono_voidp;
		typedef typename ABI::irmono_uintptr_t irmono_uintptr_t;
		typedef typename ABI::irmono_intptr_t irmono_intptr_t;

		size_t blockSize = lengths.size() * sizeof(irmono_voidp);
		if (!lowerBounds.empty()) {
			blockSize += lowerBounds.size() * sizeof(irmono_voidp);
		}

		char* data = new char[blockSize];
		char* dataPtr = data;

		// NOTE: The API for mono_array_new_full() was changed in Mono 2.8. It used to be the following:
		//
		//		MonoArray* mono_array_new_full(MonoDomain *domain, MonoClass *array_class,
		//				mono_array_size_t *lengths, mono_array_size_t *lower_bounds);
		//
		// Crucially, the element type of lengths and lower_bounds (mono_array_size_t) used to be typedef'd to either
		// guint32 or (if MONO_BIG_ARRAYS was defined) guint64. This means we can't use the new signature when the
		// remote uses the old Mono API, and for some miraculous reason, even some recent Unity games STILL use it...
		// So we detect the new API by looking for mono_free(), which was also introduced in 2.8, and change types
		// accordingly.
		// See: https://www.mono-project.com/docs/advanced/embedding/#updates-for-mono-version-28
		// TODO: Find a way to support old remotes with MONO_BIG_ARRAYS. How do we detect that?
		// TODO: Is looking for mono_free() really a robust way to detect new vs. old API?
		bool newApi = (bool) e.api.free;

		if (newApi) {
			for (rmono_uintptr_t len : lengths) {
				*((irmono_uintptr_t*) dataPtr) = e.abi.p2i_rmono_uintptr_t(len);
				dataPtr += sizeof(irmono_uintptr_t);
			}
			for (rmono_intptr_t bounds : lowerBounds) {
				*((irmono_intptr_t*) dataPtr) = e.abi.p2i_rmono_intptr_t(bounds);
				dataPtr += sizeof(irmono_intptr_t);
			}
		} else {
			for (rmono_uintptr_t len : lengths) {
				*((uint32_t*) dataPtr) = uint32_t(len);
				dataPtr += sizeof(uint32_t);
			}
			for (rmono_intptr_t bounds : lowerBounds) {
				*((uint32_t*) dataPtr) = uint32_t(bounds);
				dataPtr += sizeof(uint32_t);
			}
		}

		backend::RMonoMemBlock block = std::move(backend::RMonoMemBlock::alloc(&process, blockSize, PAGE_READWRITE));
		block.write(0, blockSize, data);

		delete[] data;

		irmono_voidp lengthsPtr = irmono_voidp(*block);
		irmono_voidp lowerBoundsPtr = lowerBounds.empty() ? 0 : irmono_voidp(lengthsPtr + lengths.size()*sizeof(irmono_voidp));

		RMonoArrayPtr arr = e.abi.i2p_RMonoArrayPtr(e.api.array_new_full(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_RMonoClassPtr(cls),
				lengthsPtr, lowerBoundsPtr));

		block.free();

		return arr;
	});
}


RMonoArrayPtr RMonoAPI::arrayNewFull (
		RMonoClassPtr cls,
		const std::vector<rmono_uintptr_t>& lengths,
		const std::vector<rmono_intptr_t>& lowerBounds
) {
	return arrayNewFull(cls, lengths, lowerBounds);
}


RMonoClassPtr RMonoAPI::arrayClassGet(RMonoClassPtr cls, uint32_t rank)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_class_get);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoClassPtr(e.api.array_class_get(e.abi.p2i_RMonoClassPtr(cls), rank));
	});
}


rmono_voidp RMonoAPI::arrayAddrWithSize(RMonoArrayPtr arr, rmono_int size, rmono_uintptr_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_addr_with_size);
	if (!arr) throw RMonoException("Invalid array");

	rmono_voidp addr;
	apid->apply([&](auto& e) {
		e.api.array_addr_with_size(RMonoVariant(&addr, RMonoVariant::rawPtr), e.abi.p2i_RMonoArrayPtr(arr),
				e.abi.p2i_rmono_int(size), e.abi.p2i_rmono_uintptr_t(idx));
	});
	return addr;
}


rmono_uintptr_t RMonoAPI::arrayLength(RMonoArrayPtr arr)
{
	checkAttached();
	if (!arr) throw RMonoException("Invalid array");

	return apid->apply([&](auto& e) {
		if (e.api.array_length) {
			return e.abi.i2p_rmono_uintptr_t(e.api.array_length(e.abi.p2i_RMonoArrayPtr(arr)));
		} else {
			RMonoClassPtr cls = objectGetClass(arr);
			RMonoPropertyPtr lenProp = classGetPropertyFromName(cls, "Length");
			RMonoObjectPtr lenObj = propertyGetValue(lenProp, arr);
			return (rmono_uintptr_t) objectUnbox<int32_t>(lenObj);
		}
	});
}


int32_t RMonoAPI::arrayElementSize(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_element_size);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.array_element_size(e.abi.p2i_RMonoClassPtr(cls));
	});
}


int32_t RMonoAPI::classArrayElementSize(RMonoClassPtr cls)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(class_array_element_size);
	if (!cls) throw RMonoException("Invalid class");

	return apid->apply([&](auto& e) {
		return e.api.class_array_element_size(e.abi.p2i_RMonoClassPtr(cls));
	});
}


RMonoArrayPtr RMonoAPI::arrayClone(RMonoArrayPtr arr)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_clone);
	if (!arr) throw RMonoException("Invalid array");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoArrayPtr(e.api.array_clone(e.abi.p2i_RMonoArrayPtr(arr)));
	});
}


template <typename T>
T RMonoAPI::arrayGet(RMonoArrayPtr arr, rmono_uintptr_t idx)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(array_addr_with_size);
	if (!arr) throw RMonoException("Invalid array");

	T val;
	apid->apply([&](auto& e) {
		typedef decltype(e.abi) ABI;

		// TODO: What about custom value types? Should probably provide a version with RMonoVariant output parameter instead
		// of templated return type.

		// NOTE: Mono's original macros for mono_array_get() and mono_array_set*() directly use sizeof() to determine
		// the element size, so it seems safe to do the same here, and it's certainly much faster.
		if constexpr(std::is_base_of_v<RMonoObjectHandleTag, T>) {
			e.api.array_addr_with_size(RMonoVariant(&val), e.abi.p2i_RMonoArrayPtr(arr),
					e.abi.p2i_rmono_int((rmono_int) sizeof(typename ABI::IRMonoObjectPtrRaw)), e.abi.p2i_rmono_uintptr_t(idx));
		} else {
			e.api.array_addr_with_size(RMonoVariant(&val), e.abi.p2i_RMonoArrayPtr(arr),
					e.abi.p2i_rmono_int((rmono_int) sizeof(T)), e.abi.p2i_rmono_uintptr_t(idx));
		}
	});
	return val;
}


void RMonoAPI::arraySet(RMonoArrayPtr arr, rmono_uintptr_t idx, const RMonoVariant& val)
{
	checkAttached();
	if (!arr) throw RMonoException("Invalid array");

	apid->apply([&](auto& e) {
		// TODO: Maybe some auto-unboxing support? Probably just need to add it to rmono_array_setref().

		if (val.getType() == RMonoVariant::TypeMonoObjectPtr) {
			e.api.rmono_array_setref(e.abi.p2i_rmono_gchandle(*arr), e.abi.p2i_rmono_uintptr_t(idx),
					e.abi.p2i_rmono_gchandle(*val.getMonoObjectPtr()));
		} else if (val.getType() == RMonoVariant::TypeRawPtr) {
			RMonoClassPtr arrCls = objectGetClass(arr);
			rmono_int size = (rmono_int) arrayElementSize(arrCls);
			rmono_voidp p = arrayAddrWithSize(arr, size, idx);
			char* data = new char[size];
			process.readMemory(val.getRawPtr(), size, data);
			process.writeMemory(p, size, data);
			delete[] data;
		} else {
			size_t align;
			rmono_int size = (rmono_int) val.getRemoteMemorySize(e.abi, align);
			rmono_voidp p = arrayAddrWithSize(arr, size, idx);
			char* data = new char[size];
			val.copyForRemoteMemory(e.abi, data);
			process.writeMemory(p, size, data);
			delete[] data;
		}
	});
}



rmono_gchandle RMonoAPI::gchandleNew(RMonoObjectPtr obj, bool pinned)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_new);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_gchandle(e.api.gchandle_new(e.abi.p2i_RMonoObjectPtr(obj), e.abi.p2i_rmono_bool(pinned ? 1 : 0)));
	});
}


rmono_gchandle RMonoAPI::gchandleNewRaw(RMonoObjectPtrRaw obj, bool pinned)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_new);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_gchandle(e.api.gchandle_new.invokeRaw(e.abi.p2i_RMonoObjectPtrRaw(obj), e.abi.p2i_rmono_bool(pinned ? 1 : 0)));
	});
}


rmono_gchandle RMonoAPI::gchandleNewWeakref(RMonoObjectPtr obj, bool trackResurrection)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_new_weakref);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_gchandle(e.api.gchandle_new_weakref(e.abi.p2i_RMonoObjectPtr(obj),
				e.abi.p2i_rmono_bool(trackResurrection ? 1 : 0)));
	});
}


rmono_gchandle RMonoAPI::gchandleNewWeakrefRaw(RMonoObjectPtrRaw obj, bool trackResurrection)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_new_weakref);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_gchandle(e.api.gchandle_new_weakref.invokeRaw(e.abi.p2i_RMonoObjectPtrRaw(obj),
				e.abi.p2i_rmono_bool(trackResurrection ? 1 : 0)));
	});
}


RMonoObjectPtrRaw RMonoAPI::gchandleGetTarget(rmono_gchandle gchandle)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_get_target);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtrRaw(e.api.gchandle_get_target(e.abi.p2i_rmono_gchandle(gchandle)));
	});
}


void RMonoAPI::gchandleFree(rmono_gchandle gchandle)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gchandle_free);

	return apid->apply([&](auto& e) {
		e.api.gchandle_free(e.abi.p2i_rmono_gchandle(gchandle));
	});
}


void RMonoAPI::gcCollect(rmono_int generation)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gc_collect);

	apid->apply([&](auto& e) {
		e.api.gc_collect(e.abi.p2i_rmono_int(generation));
	});
}


rmono_int RMonoAPI::gcMaxGeneration()
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gc_max_generation);

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_int(e.api.gc_max_generation());
	});
}


rmono_int RMonoAPI::gcGetGeneration(RMonoObjectPtr obj)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(gc_get_generation);
	if (!obj) throw RMonoException("Invalid object");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_int(e.api.gc_get_generation(e.abi.p2i_RMonoObjectPtr(obj)));
	});
}



RMonoObjectPtr RMonoAPI::runtimeInvoke(RMonoMethodPtr method, const RMonoVariant& obj, RMonoVariantArray& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(runtime_invoke);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.runtime_invoke(e.abi.p2i_RMonoMethodPtr(method), obj, params, catchExceptions));
	});
}


RMonoObjectPtr RMonoAPI::runtimeInvoke(RMonoMethodPtr method, const RMonoVariant& obj, RMonoVariantArray&& params, bool catchExceptions)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(runtime_invoke);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoObjectPtr(e.api.runtime_invoke(e.abi.p2i_RMonoMethodPtr(method), obj, params, catchExceptions));
	});
}



rmono_funcp RMonoAPI::compileMethod(RMonoMethodPtr method)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(compile_method);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_funcp(e.api.compile_method(e.abi.p2i_RMonoMethodPtr(method)));
	});
}



RMonoJitInfoPtr RMonoAPI::jitInfoTableFind(RMonoDomainPtr domain, rmono_voidp addr)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_info_table_find);
	if (!domain) throw RMonoException("Invalid domain");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoJitInfoPtr(e.api.jit_info_table_find(e.abi.p2i_RMonoDomainPtr(domain), e.abi.p2i_rmono_voidp(addr)));
	});
}


RMonoJitInfoPtr RMonoAPI::jitInfoTableFind(rmono_voidp addr)
{
	return jitInfoTableFind(domainGet(), addr);
}


rmono_funcp RMonoAPI::jitInfoGetCodeStart(RMonoJitInfoPtr jinfo)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_info_get_code_start);
	if (!jinfo) throw RMonoException("Invalid jit info");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_funcp(e.api.jit_info_get_code_start(e.abi.p2i_RMonoJitInfoPtr(jinfo)));
	});
}


int32_t RMonoAPI::jitInfoGetCodeSize(RMonoJitInfoPtr jinfo)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_info_get_code_size);
	if (!jinfo) throw RMonoException("Invalid jit info");

	return apid->apply([&](auto& e) {
		return e.api.jit_info_get_code_size(e.abi.p2i_RMonoJitInfoPtr(jinfo));
	});
}


RMonoMethodPtr RMonoAPI::jitInfoGetMethod(RMonoJitInfoPtr jinfo)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(jit_info_get_method);
	if (!jinfo) throw RMonoException("Invalid jit info");

	return apid->apply([&](auto& e) {
		return e.abi.i2p_RMonoMethodPtr(e.api.jit_info_get_method(e.abi.p2i_RMonoJitInfoPtr(jinfo)));
	});
}



std::string RMonoAPI::disasmCode(RMonoDisHelperPtr helper, RMonoMethodPtr method, rmono_voidp ip, rmono_voidp end)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(disasm_code);
	if (!method) throw RMonoException("Invalid method");

	return apid->apply([&](auto& e) {
		return e.api.disasm_code(e.abi.p2i_RMonoDisHelperPtr(helper), e.abi.p2i_RMonoMethodPtr(method), e.abi.p2i_rmono_voidp(ip),
				e.abi.p2i_rmono_voidp(end));
	});
}


std::string RMonoAPI::pmip(rmono_voidp ip)
{
	checkAttached();
	REMOTEMONO_RMONOAPI_CHECK_SUPPORTED(pmip);

	return apid->apply([&](auto& e) {
		return e.api.pmip(e.abi.p2i_rmono_voidp(ip));
	});
}













// **************************************************************************
// *																		*
// *							UTILITY METHODS								*
// *																		*
// **************************************************************************


std::vector<RMonoClassPtr> RMonoAPI::listClasses(RMonoImagePtr image)
{
	std::vector<RMonoClassPtr> out;

	RMonoTableInfoPtr tableInfo = imageGetTableInfo(image, MONO_TABLE_TYPEDEF);
	rmono_int rows = tableInfoGetRows(tableInfo);

	for (rmono_int i = 0 ; i < rows ; i++) {
		uint32_t nameGuid = metadataDecodeRowCol(tableInfo, i, MONO_TYPEDEF_NAME);
		uint32_t nameSpaceGuid = metadataDecodeRowCol(tableInfo, i, MONO_TYPEDEF_NAMESPACE);
		std::string name = metadataStringHeap(image, nameGuid);
		std::string nameSpace = metadataStringHeap(image, nameSpaceGuid);

		RMonoClassPtr cls = classFromName(image, nameSpace, name);

		if (cls) {
			out.push_back(cls);
		}
	}

	return out;
}



std::string RMonoAPI::objectToStringUTF8(RMonoObjectPtr obj, bool catchExceptions)
{
	return stringToUTF8(objectToString(obj, catchExceptions));
}


template <typename T>
std::vector<T> RMonoAPI::arrayAsVector(RMonoArrayPtr arr)
{
	std::vector<T> out;

	rmono_uintptr_t len = arrayLength(arr);
	for (rmono_uintptr_t i = 0 ; i < len ; i++) {
		out.push_back(arrayGet<T>(arr, i));
	}

	return out;
}


template <typename T>
RMonoArrayPtr RMonoAPI::arrayFromVector(RMonoDomainPtr domain, RMonoClassPtr cls, const std::vector<T>& vec)
{
	RMonoArrayPtr arr = arrayNew(domain, cls, vec.size());

	for (size_t i = 0 ; i < vec.size() ; i++) {
		arraySet(arr, (rmono_uintptr_t) i, RMonoVariant(vec[i]));
	}

	return arr;
}


template <typename T>
RMonoArrayPtr RMonoAPI::arrayFromVector(RMonoClassPtr cls, const std::vector<T>& vec)
{
	return arrayFromVector(domainGet(), cls, vec);
}



rmono_gchandle RMonoAPI::gchandlePin(rmono_gchandle gchandle)
{
	return apid->apply([&](auto& e) {
		return e.abi.i2p_rmono_gchandle(e.api.rmono_gchandle_pin(e.abi.p2i_rmono_gchandle(gchandle)));
	});
}





}
