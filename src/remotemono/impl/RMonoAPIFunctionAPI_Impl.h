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

#include "RMonoAPIFunctionAPI_Def.h"

#include <sstream>
#include "RMonoAPIBase_Def.h"
#include "RMonoAPI_Def.h"
#include "exception/RMonoRemoteException_Def.h"
#include "../log.h"
#include "../util.h"

using namespace blackbone;




namespace remotemono
{



template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
typename RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::CommonAPIRetType
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::invokeAPIInternal (
		CommonAPIArgsTuple&& args
) {
	if constexpr(!CommonT::needsWrapFunc()) {
		// Call the raw function directly
		return std::apply([&](auto&... params) {
			if constexpr(!std::is_same_v<CommonAPIRetType, void>) {
				return convertRawCallRet(static_cast<CommonT*>(this)->invokeRaw(convertRawCallArg<ArgsT>(params)...));
			} else {
				static_cast<CommonT*>(this)->invokeRaw(convertRawCallArg<ArgsT>(params)...);
			}
		}, args);
	} else {
		// Call the wrapper function

		ABI* abi = getABI();
		RMonoAPIBase* mono = getRemoteMonoAPI();
		Process& process = mono->getProcess();
		ProcessMemory& mem = process.memory();

		constexpr size_t numArgs = sizeof...(ArgsT);

		MemBlock dataBlock;
		std::unique_ptr<char[]> dataBlockBuf;
		size_t dataBlockSize = 0;

		InvokeContext ctx(args);

		remoteDataBlockGetSize(ctx, &dataBlockSize);

		// Allocate and fill remote data block
		if (dataBlockSize != 0) {
			dataBlockBuf = std::move(std::unique_ptr<char[]>(new char[dataBlockSize]));
			dataBlock = std::move(mem.Allocate(dataBlockSize, PAGE_READWRITE).result());

			memset(dataBlockBuf.get(), 0, dataBlockSize);

			assert(dataBlock.ptr() <= (std::numeric_limits<irmono_voidp>::max)());

			char* dataBlockBufPtr = dataBlockBuf.get();
			irmono_voidp dataBlockAddr = (irmono_voidp) dataBlock.ptr();

			remoteDataBlockFill(ctx, &dataBlockBufPtr, &dataBlockAddr);

			dataBlock.Write(0, dataBlockSize, dataBlockBuf.get());
		} else {
			// Still need to call it, because it initializes ctx.wrapArgs as well!
			remoteDataBlockFill(ctx, nullptr, nullptr);
		}

		// Log details about the call
		if (RMonoLogger::getInstance().isLogLevelActive(RMonoLogger::LOG_LEVEL_VERBOSE)) {
			std::string funcName = static_cast<CommonT*>(this)->getName();

			std::stringstream argsStrStream;
			argsStrStream << std::hex << std::uppercase;
			std::apply([&](auto&... params) {
				size_t argIdx = 0;
				((argsStrStream << (argIdx++ != 0 ? ", " : "") << params), ...);
			}, ctx.wrapArgs);
			std::string argsStr = argsStrStream.str();

			if (dataBlockSize != 0) {
				constexpr size_t maxDataBlockBytes = 128;

				std::string dataBlockStr;
				if (dataBlockSize <= maxDataBlockBytes) {
					dataBlockStr = DumpByteArray(dataBlockBuf.get(), dataBlockSize);
				} else {
					dataBlockStr = DumpByteArray(dataBlockBuf.get(), maxDataBlockBytes).append(" ...");
				}

				RMonoLogVerbose("Calling wrapper '%s'   -   Args (hex): [%s],   Data Block: %llX +%llX [%s]", funcName.data(),
						argsStr.data(), dataBlock.ptr(), (long long unsigned) dataBlockSize, dataBlockStr.data());
			} else {
				RMonoLogVerbose("Calling wrapper '%s'   -   Args (hex): [%s],   Data Block: NONE", funcName.data(), argsStr.data());
			}
		}

		irmono_voidp dataBlockAddr = (irmono_voidp) dataBlock.ptr();
		char* dataBlockBufPtr = dataBlockBuf.get();

		// Call the wrapper function
		WrapRetTypeOptional wrapRetval = std::apply([&](auto&... params) {
			if constexpr(!std::is_same_v<CommonWrapRetType, void>) {
				return static_cast<CommonT*>(this)->invokeWrap(params...);
			} else {
				static_cast<CommonT*>(this)->invokeWrap(params...);
				return 0;
			}
		}, ctx.wrapArgs);

		// Read back the remote data block
		if (dataBlockSize != 0) {
			dataBlock.Read(0, dataBlock.size(), dataBlockBuf.get());
		}

		// Parse the updated remote data block, calculating return value and updating output parameters
		APIRetTypeOptional retval = remoteDataBlockRead(ctx, wrapRetval, &dataBlockBufPtr);

		if constexpr(!std::is_same_v<CommonAPIRetType, void>) {
			return retval;
		}
	}

	return CommonAPIRetType();
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockGetSize (
		InvokeContext& ctx,
		size_t* size
) {
	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		if constexpr(sizeof...(ArgsT) != 0) {
			remoteDataBlockArgGetSize<1, 1, ArgsT...>(ctx, size, PackHelper<ArgsT>()...);
		}
	} else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		*size += sizeof(uint32_t);

		if constexpr(sizeof...(ArgsT) != 0) {
			remoteDataBlockArgGetSize<0, 1, ArgsT...>(ctx, size, PackHelper<ArgsT>()...);
		}
	} else {
		if constexpr(sizeof...(ArgsT) != 0) {
			remoteDataBlockArgGetSize<0, 0, ArgsT...>(ctx, size, PackHelper<ArgsT>()...);
		}
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockFill (
		InvokeContext& ctx,
		char** buf,
		irmono_voidp* rBufAddr
) {
	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		auto& outArg = std::get<0>(ctx.apiArgs);

		assert((outArg.getFlags() & Variant::FlagOut)  !=  0);

		// Fill param flags
		uint8_t flags = 0;
		if (outArg.getType() == Variant::TypeMonoObjectPtr) {
			flags |= ParamFlagMonoObjectPtr;
		} else if (outArg.getType() == Variant::TypeCustomValueRef) {
			flags |= ParamFlagDirectPtr;
		}
		if ((outArg.getFlags() & Variant::FlagOut)  !=  0) {
			flags |= ParamFlagOut;
		}

		std::get<0>(ctx.wrapArgs) = (irmono_voidp) flags;

		if constexpr(sizeof...(ArgsT) != 0) {
			remoteDataBlockArgFill<1, 1, ArgsT...>(ctx, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
		return;
	} else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		std::get<0>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;

		*((uint32_t*) buf) = 0; // Return string length
		*buf += sizeof(uint32_t);
		*rBufAddr += sizeof(uint32_t);

		remoteDataBlockArgFill<0, 1, ArgsT...>(ctx, buf, rBufAddr, PackHelper<ArgsT>()...);
	} else if constexpr(sizeof...(ArgsT) != 0) {
		remoteDataBlockArgFill<0, 0, ArgsT...>(ctx, buf, rBufAddr, PackHelper<ArgsT>()...);
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
typename RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::APIRetTypeOptional
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockRead (
		InvokeContext& ctx,
		WrapRetTypeOptional retval,
		char** buf
) {
	ABI* abi = getABI();
	RMonoAPI* mono = static_cast<RMonoAPI*>(getRemoteMonoAPI());
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();

	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {
		auto& outArg = std::get<0>(ctx.apiArgs);

		*buf += sizeof(uint8_t); // Flags

		if (outArg.getType() == Variant::TypeMonoObjectPtr) {
			outArg.updateFromRemoteMemory(*abi, *mono, &retval);
			remoteDataBlockArgRead<1, 1, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return 0;
		} else if (outArg.getType() == Variant::TypeCustomValueRef) {
			outArg.updateFromRemoteMemory(*abi, *mono, &retval);
			remoteDataBlockArgRead<1, 1, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return 0;
		} else {
			// TODO: There's a race condition here: The data pointed to by retval may already have become invalid
			// by the time we want to read it (e.g. if it's static data and the corresponding assembly has been
			// unloaded. It would be less of a race if we copied the data in the remote wrapper, but it would
			// still not be race-less. I guess there's no way to make it perfect?
			char* data = new char[outArg.getRemoteMemorySize(*abi)];
			mono->getProcess().memory().Read((ptr_t) retval, outArg.getRemoteMemorySize(*abi), data);
			outArg.updateFromRemoteMemory(*abi, *mono, data);
			delete[] data;

			remoteDataBlockArgRead<1, 1, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return 0;
		}
	} else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		if (retval != 0) {
			uint32_t size = *((uint32_t*) *buf);
			*buf += sizeof(uint32_t);

			CommonAPIRetType str(size, (typename RetT::Type::value_type) 0);
			mono->getProcess().memory().Read((ptr_t) retval, size*sizeof(typename RetT::Type::value_type), str.data());

			remoteDataBlockArgRead<0, 1, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);

			if constexpr(tags::has_return_tag_v<RetT, tags::ReturnOwnTag>) {
				mono->free(abi->i2p_rmono_voidp(retval));
			}

			return str;
		} else {
			remoteDataBlockArgRead<0, 1, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return CommonAPIRetType();
		}
	} else {
		if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename RetT::Type>) {
			remoteDataBlockArgRead<0, 0, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return abi->hi2p_RMonoObjectPtr(retval, mono);
		} else if constexpr(std::is_base_of_v<RMonoHandleTag, typename RetT::Type>) {
			remoteDataBlockArgRead<0, 0, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return typename RetT::Type(abi->i2p_rmono_voidp(retval), mono, tags::has_return_tag_v<RetT, tags::ReturnOwnTag>);
		} else if constexpr(! std::is_same_v<CommonWrapRetType, void>) {
			remoteDataBlockArgRead<0, 0, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
			return retval;
		}

		remoteDataBlockArgRead<0, 0, ArgsT...>(ctx, buf, PackHelper<ArgsT>()...);
		return 0;
	}
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t apiArgIdx, size_t wrapArgIdx, typename ArgT, typename... RestT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockArgGetSize (
		InvokeContext& ctx,
		size_t* size,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	ABI* abi = getABI();
	auto& arg = std::get<apiArgIdx>(ctx.apiArgs);

	if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		if (!arg.isNullPointer()) {
			*size += sizeof(uint8_t) + arg.getRemoteMemorySize(*abi); // Flags + payload
		}
	} else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		if (!arg.isNull()) {
			*size += sizeof(uint32_t); // Number of elements
			for (const Variant& v : arg) {
				*size += v.getRemoteMemorySize(*abi) + sizeof(irmono_voidp) + sizeof(uint8_t); // Payload + array entry + flags
			}
		}
	} else if constexpr (
			std::is_base_of_v<std::string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u16string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u32string_view, typename ArgT::Type>
	) {
		static_assert(!tags::has_param_tag_v<ArgT, tags::ParamOutTag>, "Output string are currently not supported.");

		assert((arg.size()+1)*sizeof(typename ArgT::Type::value_type) <= (std::numeric_limits<uint32_t>::max)());
		*size += (arg.size()+1)*sizeof(typename ArgT::Type::value_type); // String + null terminator
	} else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		static_assert (
				!tags::has_param_tag_v<ArgT, tags::ParamExceptionTag>  ||  tags::has_param_tag_v<ArgT, tags::ParamOutTag>,
				"Parameter tagged with ParamException<> was not tagged with ParamOut<>."
				);
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {
				*size += sizeof(irmono_gchandle);
			}
		}
	} else if constexpr(std::is_base_of_v<RMonoHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {
				// TODO: The handle behind a RMonoHandle might not always be a remote pointer, but currently there's
				// no way to get to the corresponding internal handle.
				*size += sizeof(irmono_voidp);
			}
		}
	} else if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
		*size += sizeof(typename ArgT::Type);
	}

	remoteDataBlockArgGetSize<apiArgIdx+1, wrapArgIdx+1, RestT...>(ctx, size, rest...);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t apiArgIdx, size_t wrapArgIdx, typename ArgT, typename... RestT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockArgFill (
		InvokeContext& ctx,
		char** buf,
		irmono_voidp* rBufAddr,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	ABI* abi = getABI();

	auto& arg = std::get<apiArgIdx>(ctx.apiArgs);

	if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		if (!arg.isNullPointer()) {
			assert(buf  &&  rBufAddr);

			std::get<wrapArgIdx>(ctx.wrapArgs) = *rBufAddr;

			size_t size = arg.getRemoteMemorySize(*abi);

			// Fill param flags
			uint8_t flags = 0;
			if (arg.getType() == Variant::TypeMonoObjectPtr) {
				flags |= ParamFlagMonoObjectPtr;
			} else if (arg.getType() == Variant::TypeCustomValueRef) {
				flags |= ParamFlagDirectPtr;
			}
			if ((arg.getFlags() & Variant::FlagOut)  !=  0) {
				flags |= ParamFlagOut;
			}
			*((uint8_t*) *buf) = flags;

			(*buf)++;
			(*rBufAddr)++;

			// Fill variant data
			if constexpr (
					!tags::has_param_tag_v<ArgT, tags::ParamOutTag>
				||	tags::has_param_tag_v<ArgT, tags::ParamInOutTag>
			) {
				arg.copyForRemoteMemory(*abi, *buf);
			} else {
				// Zero-initialize output parameter, otherwise we'll leak data from the remote process into remote memory.
				memset(*buf, 0, size);
			}
			*buf += size;
			*rBufAddr += (irmono_uintptr_t) size;
		} else {
			std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) 0;
		}
	} else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		if (!arg.isNull()) {
			assert(buf  &&  rBufAddr);

			std::vector<irmono_voidp> arrEntries;

			// Fill variant data
			for (const Variant& v : arg) {
				if (v.isNullPointer()) {
					arrEntries.push_back(0);
				} else {
					arrEntries.push_back(*rBufAddr);
				}

				size_t size = v.getRemoteMemorySize(*abi);

				if constexpr (
						!tags::has_param_tag_v<ArgT, tags::ParamOutTag>
					||	tags::has_param_tag_v<ArgT, tags::ParamInOutTag>
				) {
					v.copyForRemoteMemory(*abi, *buf);
				} else {
					// Zero-initialize output parameter, otherwise we'll leak data from the remote process into remote memory.
					memset(*buf, 0, size);
				}

				*buf += size;
				*rBufAddr += (irmono_uintptr_t) size;
			}

			// Yes, the argument pointer is supposed to point PAST the actual variant data, because the
			// wrapper generally doesn't know how that data is structured (i.e. how large it is), and it
			// can get any pointers it needs via the actual param array below.
			std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;

			// Fill element count
			*((uint32_t*) *buf) = (uint32_t) arg.size();
			*buf += sizeof(uint32_t);
			*rBufAddr += sizeof(uint32_t);

			// Fill param flags
			for (const Variant& v : arg) {
				uint8_t flags = 0;
				if (v.getType() == Variant::TypeMonoObjectPtr) {
					flags |= ParamFlagMonoObjectPtr;
				} else if (v.getType() == Variant::TypeCustomValueRef) {
					flags |= ParamFlagDirectPtr;
				}
				if ((v.getFlags() & Variant::FlagOut)  !=  0) {
					flags |= ParamFlagOut;
				}
				*((uint8_t*) *buf) = flags;
				(*buf)++;
				(*rBufAddr)++;
			}

			// Fill param array
			for (irmono_voidp entry : arrEntries) {
				*((irmono_voidp*) *buf) = entry;
				*buf += sizeof(irmono_voidp);
				*rBufAddr += sizeof(irmono_voidp);
			}
		} else {
			std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) 0;
		}
	} else if constexpr (
			std::is_base_of_v<std::string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u16string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u32string_view, typename ArgT::Type>
	) {
		assert(buf  &&  rBufAddr);

		std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;

		uint32_t size = uint32_t(arg.size() * sizeof(typename ArgT::Type::value_type));

		// Write string contents
		memcpy(*buf, arg.data(), size);
		*buf += size;
		*rBufAddr += size;

		// Write null terminator
		*((typename ArgT::Type::value_type*) *buf) = (typename ArgT::Type::value_type) 0;
		*buf += sizeof(typename ArgT::Type::value_type);
		*rBufAddr += sizeof(typename ArgT::Type::value_type);
	} else if constexpr(tags::has_param_tag_v<ArgT, tags::ParamExceptionTag>) {
		assert(buf  &&  rBufAddr);

		if (arg) {
			std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;

			*((irmono_gchandle*) *buf) = 0; // Exceptions are always outputs, so initial value doesn't matter much.
			*buf += sizeof(irmono_gchandle);
			*rBufAddr += sizeof(irmono_gchandle);
		} else {
			std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) 0;
		}
	} else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			assert(buf  &&  rBufAddr);

			if (arg) {
				std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;

				if constexpr (
						!tags::has_param_tag_v<ArgT, tags::ParamOutTag>
					||	tags::has_param_tag_v<ArgT, tags::ParamInOutTag>
				) {
					*((irmono_gchandle*) *buf) = abi->hp2i_RMonoObjectPtr(*arg);
				} else {
					*((irmono_gchandle*) *buf) = REMOTEMONO_GCHANDLE_INVALID;
				}

				*buf += sizeof(irmono_gchandle);
				*rBufAddr += sizeof(irmono_gchandle);
			} else {
				std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) 0;
			}
		} else {
			std::get<wrapArgIdx>(ctx.wrapArgs) = *arg;
		}
	} else if constexpr (std::is_base_of_v<RMonoHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			std::get<wrapArgIdx>(ctx.wrapArgs) = *rBufAddr;

			if constexpr(tags::has_param_tag_v<ArgT, tags::ParamInOutTag>) {
				*((irmono_voidp*) *buf) = abi->p2i_rmono_voidp(*arg);
			} else {
				*((irmono_voidp*) *buf) = 0;
			}
			*buf += sizeof(irmono_voidp);
			*rBufAddr += sizeof(irmono_voidp);
		} else {
			std::get<wrapArgIdx>(ctx.wrapArgs) = abi->p2i_rmono_voidp(*arg);
		}
	} else if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
		assert(buf  &&  rBufAddr);

		std::get<wrapArgIdx>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;
		if (arg) {
			if constexpr(tags::has_param_tag_v<ArgT, tags::ParamInOutTag>) {
				*((typename ArgT::Type*) *buf) = *arg;
			} else {
				*((typename ArgT::Type*) *buf) = typename ArgT::Type();
			}
		} else {
			*((typename ArgT::Type*) *buf) = typename ArgT::Type();
			*buf += sizeof(typename ArgT::Type);
		}
		*buf += sizeof(typename ArgT::Type);
		*rBufAddr += sizeof(typename ArgT::Type);
	} else {
		std::get<wrapArgIdx>(ctx.wrapArgs) = arg;
	}

	remoteDataBlockArgFill<apiArgIdx+1, wrapArgIdx+1, RestT...>(ctx, buf, rBufAddr, rest...);
}


template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t apiArgIdx, size_t wrapArgIdx, typename ArgT, typename... RestT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::remoteDataBlockArgRead (
		InvokeContext& ctx,
		char** buf,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	ABI* abi = getABI();
	RMonoAPIBase* mono = getRemoteMonoAPI();

	auto& arg = std::get<apiArgIdx>(ctx.apiArgs);

	if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		if (!arg.isNullPointer()) {
			if constexpr(tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
				arg.updateFromRemoteMemory(*abi, *mono, *buf + sizeof(uint8_t));
			}
			*buf += sizeof(uint8_t) + arg.getRemoteMemorySize(*abi); // Flags + payload
		}
	} else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		if (!arg.isNull()) {
			for (Variant& v : arg) {
				if constexpr(tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
					v.updateFromRemoteMemory(*abi, *mono, *buf);
				}
				*buf += v.getRemoteMemorySize(*abi);
			}

			*buf += sizeof(uint32_t) + arg.size() * (sizeof(uint8_t) + sizeof(irmono_voidp));
		}
	} else if constexpr(tags::has_param_tag_v<ArgT, tags::ParamExceptionTag>) {
		if (arg) {
			irmono_gchandle exHandle = *((irmono_gchandle*) *buf);
			*buf += sizeof(irmono_gchandle);

			if (exHandle != 0) {
				RMonoObjectPtr exObj = abi->hi2p_RMonoObjectPtr(exHandle, mono);
				throw RMonoRemoteException(exObj);
			}
		}
	} else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {
				*arg = abi->hi2p_RMonoObjectPtr(*((irmono_gchandle*) *buf), mono);
				*buf += sizeof(irmono_gchandle);
			}
		}
	} else if constexpr(std::is_base_of_v<RMonoHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {
				*arg = typename ArgT::Type(abi->i2p_rmono_voidp(*((irmono_voidp*) *buf)), mono, tags::has_param_tag_v<ArgT, tags::ParamOwnTag>);
				*buf += sizeof(irmono_voidp);
			}
		}
	} else if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
		if (arg) {
			*arg = *((typename ArgT::Type*) *buf);
		}
		*buf += sizeof(typename ArgT::Type);
	}

	remoteDataBlockArgRead<apiArgIdx+1, wrapArgIdx+1, RestT...>(ctx, buf, rest...);
}



}
