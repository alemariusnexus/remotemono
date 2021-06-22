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

#include "RMonoAPIFunctionAPI_Def.h"

#include <sstream>
#include "RMonoAPIBase_Def.h"
#include "RMonoAPI_Def.h"
#include "exception/RMonoRemoteException_Def.h"
#include "../log.h"
#include "../util.h"




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
		backend::RMonoProcess& process = mono->getProcess();

		constexpr size_t numArgs = sizeof...(ArgsT);

		size_t allocMinAlignment = process.getPageSize();
		assert(allocMinAlignment >= RMonoVariant::getMaxRequiredAlignment());

		backend::RMonoMemBlock dataBlock;
		std::unique_ptr<char[]> dataBlockBuf;
		size_t dataBlockSize = 0;
		size_t dataBlockRelAddr = 0; // Current address within data block

		InvokeContext ctx(args);


		// ********** Get Data Block Size **********

		{
			char* dataBlockBufPtr = nullptr; // This step doesn't write to the buffer, but WILL shift the pointer.
			irmono_voidp dataBlockAddr = 0; // For the first step, it's relative to the remote data buffer start.

			handleInvokeStep(ctx, StepDataBlockGetSize, &dataBlockBufPtr, &dataBlockAddr, nullptr, nullptr);

			dataBlockSize = (size_t) dataBlockAddr;
			assert(size_t(dataBlockBufPtr) == dataBlockSize);
		}


		// ********** Create and Fill Data Block **********

		if (dataBlockSize != 0) {
			// Allocate block (local and remote)
			dataBlockBuf = std::move(std::unique_ptr<char[]>(new char[dataBlockSize]));
			dataBlock = std::move(backend::RMonoMemBlock::alloc(&process, dataBlockSize, PAGE_READWRITE));

			// Zero-initialize
			memset(dataBlockBuf.get(), 0, dataBlockSize);

			assert(*dataBlock <= (std::numeric_limits<irmono_voidp>::max)());

			// Fill local data block
			{
				char* dataBlockBufPtr = dataBlockBuf.get();
				irmono_voidp dataBlockAddr = (irmono_voidp) *dataBlock;

				handleInvokeStep(ctx, StepDataBlockFill, &dataBlockBufPtr, &dataBlockAddr, nullptr, nullptr);

				assert(dataBlockBufPtr == dataBlockBuf.get() + dataBlockSize);
				assert(dataBlockAddr == ((irmono_voidp) *dataBlock) + dataBlockSize);
			}

			// Write local data block to remote buffer
			dataBlock.write(0, dataBlockSize, dataBlockBuf.get());
		} else {
			// Still need to call it, because it initializes ctx.wrapArgs as well!
			handleInvokeStep(ctx, StepDataBlockFill, nullptr, nullptr, nullptr, nullptr);
		}


		// ********** Debug Logging **********

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
						argsStr.data(), *dataBlock, (long long unsigned) dataBlockSize, dataBlockStr.data());
			} else {
				RMonoLogVerbose("Calling wrapper '%s'   -   Args (hex): [%s],   Data Block: NONE", funcName.data(), argsStr.data());
			}
		}


		// ********** Invoke Wrapper Function **********

		// Call the wrapper function
		WrapRetTypeOptional wrapRetval = std::apply([&](auto&... params) {
			if constexpr(!std::is_same_v<CommonWrapRetType, void>) {
				return static_cast<CommonT*>(this)->invokeWrap(params...);
			} else {
				static_cast<CommonT*>(this)->invokeWrap(params...);
				return 0;
			}
		}, ctx.wrapArgs);


		// ********** Handle Return Value and Output Parameters **********

		// Read back the remote data block
		if (dataBlockSize != 0) {
			dataBlock.read(0, dataBlock.size(), dataBlockBuf.get());
		}

		APIRetTypeOptional apiRetval;

		// Calculate return value and update output parameters
		{
			char* dataBlockBufPtr = dataBlockBuf.get();
			irmono_voidp dataBlockAddr = (irmono_voidp) *dataBlock;

			handleInvokeStep(ctx, StepDataBlockRead, &dataBlockBufPtr, &dataBlockAddr, &wrapRetval, &apiRetval);

			assert(dataBlockBufPtr == dataBlockBuf.get() + dataBlockSize);
			assert(dataBlockAddr == ((irmono_voidp) *dataBlock) + dataBlockSize);
		}

		if constexpr(!std::is_same_v<CommonAPIRetType, void>) {
			return apiRetval;
		}
	}

	return CommonAPIRetType();
}












template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::handleInvokeStep (
		InvokeContext& ctx,
		InvokeStep step,
		char** buf,
		irmono_voidp* rBufAddr,
		WrapRetTypeOptional* wrapRetval,
		APIRetTypeOptional* apiRetval
) {
	ABI* abi = getABI();
	RMonoAPI* mono = static_cast<RMonoAPI*>(getRemoteMonoAPI());
	RMonoAPIDispatcher* apid = mono->getAPIDispatcher();

	backend::RMonoProcess& process = mono->getProcess();



	// **********************************************************************
	// *																	*
	// *					RETURN-TYPE-DEPENDENT LOGIC						*
	// *																	*
	// **********************************************************************


	// **********************************************************
	// *						VARIANT							*
	// **********************************************************

	if constexpr(std::is_base_of_v<Variant, typename RetT::Type>) {

		auto& outArg = std::get<0>(ctx.apiArgs);


		if (step == StepDataBlockFill) {
			variantflags_t flags = buildVariantFlags(outArg, true);
			std::get<0>(ctx.wrapArgs) = (irmono_voidp) flags;
		}


		else if (step == StepDataBlockRead) {
			if (outArg.getType() == Variant::TypeMonoObjectPtr) {
				outArg.updateFromRemoteMemory(*abi, *mono, wrapRetval);
			} else if (outArg.getType() == Variant::TypeRawPtr) {
				outArg.updateFromRemoteMemory(*abi, *mono, wrapRetval);
			} else {
				// TODO: There's a race condition here: The data pointed to by wrapRetval may already have become
				// invalid by the time we want to read it (e.g. if it's static data and the corresponding assembly
				// has been unloaded. It would be less of a race if we copied the data in the remote wrapper, but it
				// would still not be race-less. I guess there's no way to make it perfect?
				size_t valign;
				size_t vsize = outArg.getRemoteMemorySize(*abi, valign);
				char* data = new char[vsize];
				process.readMemory(static_cast<rmono_voidp>(*wrapRetval), vsize, data);
				outArg.updateFromRemoteMemory(*abi, *mono, data);
				delete[] data;
			}
		}


		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<1, 1, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}

	}



	// **********************************************************
	// *						STRING							*
	// **********************************************************

	else if constexpr (
			std::is_base_of_v<std::string, typename RetT::Type>
		||	std::is_base_of_v<std::u16string, typename RetT::Type>
		||	std::is_base_of_v<std::u32string, typename RetT::Type>
	) {
		alignBuf(buf, rBufAddr, sizeof(uint32_t));

		uint32_t* strLenPtr = (uint32_t*) *buf;


		if (step == StepDataBlockFill) {
			std::get<0>(ctx.wrapArgs) = (irmono_voidp) *rBufAddr;
			*strLenPtr = 0; // Return string length
		}


		else if (step == StepDataBlockRead) {
			if (*wrapRetval != 0) {
				uint32_t size = *strLenPtr;

				CommonAPIRetType str(size, (typename RetT::Type::value_type) 0);
				process.readMemory(static_cast<rmono_voidp>(*wrapRetval),
						size*sizeof(typename RetT::Type::value_type), str.data());

				if constexpr(tags::has_return_tag_v<RetT, tags::ReturnOwnTag>) {
					mono->freeLater(abi->i2p_rmono_voidp(*wrapRetval));
				}

				*apiRetval = std::move(str);
			} else {
				*apiRetval = CommonAPIRetType();
			}
		}


		shiftBuf(buf, rBufAddr, sizeof(uint32_t));

		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<0, 1, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
	}



	// **********************************************************
	// *					OBJECT HANDLE						*
	// **********************************************************

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename RetT::Type>) {
		if (step == StepDataBlockRead) {
			*apiRetval = abi->hi2p_RMonoObjectPtr(*wrapRetval, mono);
		}

		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<0, 0, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
	}



	// **********************************************************
	// *					MISC. HANDLE						*
	// **********************************************************

	else if constexpr(std::is_base_of_v<RMonoHandleTag, typename RetT::Type>) {
		if (step == StepDataBlockRead) {
			*apiRetval = typename RetT::Type(abi->i2p_rmono_voidp(*wrapRetval), mono,
					tags::has_return_tag_v<RetT, tags::ReturnOwnTag>);
		}

		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<0, 0, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
	}



	// **********************************************************
	// *					OTHER NON-VOID						*
	// **********************************************************

	else if constexpr(! std::is_same_v<CommonWrapRetType, void>) {
		if (step == StepDataBlockRead) {
			*apiRetval = *wrapRetval;
		}

		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<0, 0, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
	}



	// **********************************************************
	// *						VOID							*
	// **********************************************************

	else {
		if constexpr(sizeof...(ArgsT) != 0) {
			handleInvokeStepArg<0, 0, ArgsT...>(ctx, step, buf, rBufAddr, PackHelper<ArgsT>()...);
		}
	}
}










template <typename CommonT, typename ABI, typename RetT, typename... ArgsT>
template <size_t apiArgIdx, size_t wrapArgIdx, typename ArgT, typename... RestT>
void
RMonoAPIFunctionAPI<CommonT, ABI, RetT, ArgsT...>::handleInvokeStepArg (
		InvokeContext& ctx,
		InvokeStep step,
		char** buf,
		irmono_voidp* rBufAddr,
		PackHelper<ArgT>,
		PackHelper<RestT>... rest
) {
	ABI* abi = getABI();
	RMonoAPIBase* mono = getRemoteMonoAPI();

	auto& arg = std::get<apiArgIdx>(ctx.apiArgs);
	auto& wrapArg = std::get<wrapArgIdx>(ctx.wrapArgs);



	// **********************************************************************
	// *																	*
	// *					ARGUMENT-TYPE-DEPENDENT LOGIC					*
	// *																	*
	// **********************************************************************


	// **********************************************************
	// *						VARIANT							*
	// **********************************************************

	if constexpr(std::is_base_of_v<Variant, typename ArgT::Type>) {
		if (!arg.isNullPointer()) {

			//	struct DataBlockVariant {
			//		__align variantflags_t flags;
			//		__align char payload[*];			// <-- !!! Wrap argument points here !!!
			//	};

			irmono_voidp rBufAddrStart = *rBufAddr;
			variantflags_t* flagsPtr;
			char* payload;
			irmono_voidp payloadAddr;

			size_t valign;
			size_t vsize = arg.getRemoteMemorySize(*abi, valign);

			RMonoVariant::Direction vdir = getVariantDirectionForArg<ArgT>(arg);


			*rBufAddr = align(irmono_voidp(*rBufAddr + sizeof(variantflags_t)), sizeof(variantflags_t));
			*rBufAddr = align(*rBufAddr, valign);

			*buf += *rBufAddr - rBufAddrStart;

			flagsPtr = (variantflags_t*) (*buf - sizeof(variantflags_t));

			payload = *buf;
			payloadAddr = *rBufAddr;

			shiftBuf(buf, rBufAddr, vsize);


			// ********** GET SIZE **********

			if (step == StepDataBlockGetSize) {
			}


			// ********** FILL DATA **********

			else if (step == StepDataBlockFill) {
				wrapArg = payloadAddr;

				// Fill param flags
				*flagsPtr = buildVariantFlags(arg, (vdir == RMonoVariant::DirectionOut ||  vdir == RMonoVariant::DirectionInOut));

				// Fill variant data
				if (vdir == RMonoVariant::DirectionIn  ||  vdir == RMonoVariant::DirectionInOut) {
					// In/InOut parameter
					arg.copyForRemoteMemory(*abi, payload);
				} else {
					if (arg.getType() == Variant::TypeRawPtr) {
						// For raw pointer variants, we still need to pass the raw pointer itself in the data buffer, even for
						// output-only variants.
						arg.copyForRemoteMemory(*abi, payload);
					} else if (arg.getType() == Variant::TypeMonoObjectPtr) {
						// For output MonoObjectPtr variants, we still have to pass the variant's GCHandle in order to support
						// auto-unboxing, where a valid boxed object is passed that is detected by the wrapper function. There's
						// little risk of leaking uninitialized memory to the remote, because RMonoObjectHandle's constructors
						// always initialize the GCHandle (e.g. to 0).
						arg.copyForRemoteMemory(*abi, payload);
					} else {
						// Zero-initialize output parameter, otherwise we'll leak data from the local process into remote memory.
						memset(payload, 0, vsize);
					}
				}
			}


			// ********** READ DATA **********

			else if (step == StepDataBlockRead) {
				if (vdir == RMonoVariant::DirectionOut  ||  vdir == RMonoVariant::DirectionInOut) {
					const_cast<Variant&>(arg).updateFromRemoteMemory(*abi, *mono, payload);
				}
			}

		} else {
			// Null pointer -> directly pass NULL value

			if (step == StepDataBlockFill) {
				wrapArg = (irmono_voidp) 0;
			}
		}
	}



	// **********************************************************
	// *					VARIANT ARRAY						*
	// **********************************************************

	else if constexpr(std::is_base_of_v<VariantArray, typename ArgT::Type>) {
		if (!arg.isNull()) {

			//	struct DataBlockVariantArray {
			//		__align uint32_t numElems;						// <-- Wrap argument points here
			//		__align irmono_voidp arrEntries[numElems];
			//		__align variantflags_t flags[numElems];
			//		__align char payload[*];
			//	};

			uint32_t numElems = (uint32_t) arg.size();

			irmono_voidp startAddr;
			uint32_t* numElemsPtr;
			irmono_voidp* arrEntriesPtr;
			variantflags_t* flagsPtr;


			// ***** Element count *****

			alignBuf(buf, rBufAddr, sizeof(numElems));

			startAddr = *rBufAddr;
			numElemsPtr = (uint32_t*) *buf;

			shiftBuf(buf, rBufAddr, sizeof(numElems));


			// ***** Array Entries *****

			alignBuf(buf, rBufAddr, sizeof(irmono_voidp));

			arrEntriesPtr = (irmono_voidp*) *buf;

			shiftBuf(buf, rBufAddr, numElems*sizeof(irmono_voidp));


			// ***** Flags *****

			alignBuf(buf, rBufAddr, sizeof(variantflags_t));

			flagsPtr = (variantflags_t*) *buf;

			shiftBuf(buf, rBufAddr, numElems*sizeof(variantflags_t));


			// ********** GET SIZE **********

			if (step == StepDataBlockGetSize) {
				for (const Variant& v : arg) {
					size_t valign;
					size_t vsize = v.getRemoteMemorySize(*abi, valign);

					alignBuf(buf, rBufAddr, valign);
					shiftBuf(buf, rBufAddr, vsize);
				}
			}


			// ********** FILL DATA **********

			else if (step == StepDataBlockFill) {
				wrapArg = startAddr;
				*numElemsPtr = numElems;

				for (uint32_t i = 0 ; i < numElems ; i++) {
					const Variant& v = arg[i];

					size_t valign;
					size_t vsize = v.getRemoteMemorySize(*abi, valign);

					RMonoVariant::Direction vdir = getVariantDirectionForArg<ArgT>(v);


					// ***** Flags *****

					flagsPtr[i] = buildVariantFlags(v, (vdir == RMonoVariant::DirectionOut  ||  vdir == RMonoVariant::DirectionInOut));

					if (i == numElems-1) {
						// This is used by the wrapper function code to determine when to stop iterating without having to
						// carry around a counter variable and a variable holding the number of elements (saves registers).
						flagsPtr[i] |= ParamFlagLastArrayElement;
					}


					// ***** Payload *****

					alignBuf(buf, rBufAddr, valign);

					// Fill array entry
					if (v.isNullPointer()) {
						arrEntriesPtr[i] = 0;
					} else {
						arrEntriesPtr[i] = *rBufAddr;
					}

					if (vdir == RMonoVariant::DirectionIn  ||  vdir == RMonoVariant::DirectionInOut) {
						// In/InOut variant
						v.copyForRemoteMemory(*abi, *buf);
					} else {
						if (v.getType() == Variant::TypeRawPtr) {
							// For raw pointer variants, we still need to pass the raw pointer itself in the data buffer, even for
							// output-only variants.
							v.copyForRemoteMemory(*abi, *buf);
						} else if (v.getType() == Variant::TypeMonoObjectPtr) {
							// For output MonoObjectPtr variants, we still have to pass the variant's GCHandle in order to support
							// auto-unboxing, where a valid boxed object is passed that is detected by the wrapper function. There's
							// little risk of leaking uninitialized memory to the remote, because RMonoObjectHandle's constructors
							// always initialize the GCHandle (e.g. to 0).
							v.copyForRemoteMemory(*abi, *buf);
						} else {
							// Zero-initialize output parameter, otherwise we'll leak data from the remote process into remote memory.
							memset(*buf, 0, vsize);
						}
					}

					shiftBuf(buf, rBufAddr, vsize);
				}
			}


			// ********** READ DATA **********

			else if (step == StepDataBlockRead) {
				for (uint32_t i = 0 ; i < numElems ; i++) {
					Variant& v = const_cast<Variant&>(arg[i]);

					size_t valign;
					size_t vsize = v.getRemoteMemorySize(*abi, valign);

					RMonoVariant::Direction vdir = getVariantDirectionForArg<ArgT>(v);

					alignBuf(buf, rBufAddr, valign);

					if (vdir == RMonoVariant::DirectionOut  ||  vdir == RMonoVariant::DirectionInOut) {
						v.updateFromRemoteMemory(*abi, *mono, *buf);
					}

					shiftBuf(buf, rBufAddr, vsize);
				}
			}

		} else {
			// Null pointer -> directly pass NULL value

			if (step == StepDataBlockFill) {
				wrapArg = (irmono_voidp) 0;
			}
		}
	}



	// **********************************************************
	// *						STRING							*
	// **********************************************************

	else if constexpr (
			std::is_base_of_v<std::string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u16string_view, typename ArgT::Type>
		||	std::is_base_of_v<std::u32string_view, typename ArgT::Type>
	) {

		static_assert(!tags::has_param_tag_v<ArgT, tags::ParamOutTag>, "Output string are currently not supported.");

		uint32_t strSize = uint32_t(arg.size() * sizeof(typename ArgT::Type::value_type));

		irmono_voidp startAddr;
		typename ArgT::Type::value_type* strPtr;


		alignBuf(buf, rBufAddr, sizeof(typename ArgT::Type::value_type));

		startAddr = (irmono_voidp) *rBufAddr;
		strPtr = (typename ArgT::Type::value_type*) *buf;

		shiftBuf(buf, rBufAddr, strSize + sizeof(typename ArgT::Type::value_type));


		// ********** GET SIZE **********

		if (step == StepDataBlockGetSize) {
		}


		// ********** FILL DATA **********

		else if (step == StepDataBlockFill) {
			wrapArg = startAddr;

			// Write string contents
			memcpy(strPtr, arg.data(), strSize);

			// Write null terminator
			strPtr[arg.size()] = (typename ArgT::Type::value_type) 0;
		}


		// ********** READ DATA **********

		else if (step == StepDataBlockRead) {
		}

	}



	// **********************************************************
	// *					OBJECT HANDLE						*
	// **********************************************************

	// Including exceptions

	else if constexpr(std::is_base_of_v<RMonoObjectHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {

				irmono_voidp startAddr;
				irmono_gchandle* gchandlePtr;


				alignBuf(buf, rBufAddr, sizeof(irmono_gchandle));

				startAddr = *rBufAddr;
				gchandlePtr = (irmono_gchandle*) *buf;

				shiftBuf(buf, rBufAddr, sizeof(irmono_gchandle));


				// ********** GET SIZE **********

				if (step == StepDataBlockGetSize) {
				}


				// ********** FILL DATA **********

				else if (step == StepDataBlockFill) {
					wrapArg = startAddr;

					if constexpr (
							!tags::has_param_tag_v<ArgT, tags::ParamOutTag>
						||	tags::has_param_tag_v<ArgT, tags::ParamInOutTag>
					) {
						// InOut parameter
						*gchandlePtr = abi->hp2i_RMonoObjectPtr(*arg);
					} else {
						// Out parameter
						*gchandlePtr = REMOTEMONO_GCHANDLE_INVALID;
					}
				}


				// ********** READ DATA **********

				else if (step == StepDataBlockRead) {
					if constexpr(tags::has_param_tag_v<ArgT, tags::ParamExceptionTag>) {
						if (*gchandlePtr != 0) {
							RMonoObjectPtr exObj = abi->hi2p_RMonoObjectPtr(*gchandlePtr, mono);
							RMonoLogVerbose("Caught remote exception.");
							throw RMonoRemoteException(exObj);
						}
					} else {
						*arg = abi->hi2p_RMonoObjectPtr(*gchandlePtr, mono);
					}
				}

			} else {
				// Null object (or false for exceptions) -> pass raw NULL pointer

				if (step == StepDataBlockFill) {
					wrapArg = (irmono_voidp) 0;
				}
			}

		} else {
			// Input parameter -> pass GCHandle directly to wrapper

			if (step == StepDataBlockFill) {
				wrapArg = *arg;
			}
		}
	}



	// **********************************************************
	// *					MISC. HANDLE						*
	// **********************************************************

	else if constexpr(std::is_base_of_v<RMonoHandleTag, typename ArgT::Type>) {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {

			irmono_voidp startAddr;
			irmono_voidp* handlePtr;


			alignBuf(buf, rBufAddr, sizeof(irmono_voidp));

			startAddr = *rBufAddr;
			handlePtr = (irmono_voidp*) *buf;

			shiftBuf(buf, rBufAddr, sizeof(irmono_voidp));


			// ********** GET SIZE **********

			if (step == StepDataBlockGetSize) {
			}


			// ********** FILL DATA **********

			else if (step == StepDataBlockFill) {
				wrapArg = startAddr;

				if constexpr(tags::has_param_tag_v<ArgT, tags::ParamInOutTag>) {
					// InOut parameter
					*handlePtr = abi->p2i_rmono_voidp(*arg);
				} else {
					// Out parameter
					*handlePtr = 0;
				}
			}


			// ********** READ DATA **********

			else if (step == StepDataBlockRead) {
				*arg = typename ArgT::Type(abi->i2p_rmono_voidp(*handlePtr), mono, tags::has_param_tag_v<ArgT, tags::ParamOwnTag>);
			}

		} else {
			// Input parameter -> pass raw handle directly to wrapper

			if (step == StepDataBlockFill) {
				wrapArg = abi->p2i_rmono_voidp(*arg);
			}
		}
	}



	// **********************************************************
	// *						OTHER							*
	// **********************************************************

	else {
		if constexpr (tags::has_param_tag_v<ArgT, tags::ParamOutTag>) {
			if (arg) {

				irmono_voidp startAddr;
				typename ArgT::Type* dataPtr;


				// TODO: Alignment might be different for structs.
				alignBuf(buf, rBufAddr, sizeof(typename ArgT::Type));

				startAddr = (irmono_voidp) *rBufAddr;
				dataPtr = (typename ArgT::Type*) *buf;

				shiftBuf(buf, rBufAddr, sizeof(typename ArgT::Type));


				// ********** GET SIZE **********

				if (step == StepDataBlockGetSize) {
				}


				// ********** FILL DATA **********

				else if (step == StepDataBlockFill) {
					wrapArg = startAddr;

					if (arg) {
						// A non-null pointer
						if constexpr(tags::has_param_tag_v<ArgT, tags::ParamInOutTag>) {
							// InOut parameter
							*dataPtr = *arg;
						} else {
							// Out parameter
							*dataPtr = typename ArgT::Type();
						}
					} else {
						// A null pointer was passed. We will NOT pass a NULL pointer directly, because some Mono API functions
						// may not handle NULL for output parameters properly. Instead, we'll just pass a proper pointer and
						// then ignore anything written to it.
						*dataPtr = typename ArgT::Type();
					}
				}


				// ********** READ DATA **********

				else if (step == StepDataBlockRead) {
					*arg = *dataPtr;
				}

			} else {
				// Null pointer -> pass raw NULL pointer

				if (step == StepDataBlockFill) {
					wrapArg = (irmono_voidp) 0;
				}
			}
		} else {
			// Input parameter -> pass directly

			if (step == StepDataBlockFill) {
				wrapArg = arg;
			}
		}
	}



	handleInvokeStepArg<apiArgIdx+1, wrapArgIdx+1, RestT...>(ctx, step, buf, rBufAddr, rest...);
}



}
