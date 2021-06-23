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

#include <string>
#include <string_view>
#include "RMonoAPIBase_Def.h"
#include "RMonoTypes.h"
#include "RMonoHandle_Def.h"
#include "RMonoVariant_Def.h"
#include "RMonoVariantArray_Def.h"
#include "backend/RMonoProcess.h"
#include "backend/RMonoMemBlock.h"
#include "exception/RMonoException_Def.h"
#include "exception/RMonoUnsupportedAPIException_Def.h"



namespace remotemono
{



/**
 * This class provides the main public interface to the Mono API of a remote process. It is probably the class you're
 * most interested in when using RemoteMono.
 *
 * Before you can call any of the API wrapper methods provided by this class, you have to attach it to the remote
 * process by calling attach().
 *
 * Most of the methods provided by this class are simply convenience wrappers around the functions defined in
 * RMonoAPIBackend. This class uses RMonoAPIDispatcher to choose the backend that corresponds to the ABI that has
 * been selected for the remote process. If you want to call raw Mono API functions, or call them without the wrapping
 * provided by this class, you can use getAPIDispatcher() to get direct access to the RMonoAPIFunction instances.
 *
 * Most methods in here are not documented, because they simply call the corresponding Mono API functions in the remote
 * process. See the original Mono Embedded API documentation for details on what these functions do.
 */
class RMonoAPI : public RMonoAPIBase
{
public:
	///@name General
	///@{

	/**
	 * Create a new RMonoAPI for the given remote process. Note that this function **does not attach** to the remote.
	 * You need to call attach() afterwards before using the Mono API functions.
	 */
	inline RMonoAPI(backend::RMonoProcess& process);

	/**
	 * Destroy the RMonoAPI, automatically calling detach() if it was attached before.
	 */
	inline virtual ~RMonoAPI();


	/**
	 * Attach RemoteMono to the remote process. This will generate all the wrapper functions are upload their machine
	 * code to the remote, as well as create a Mono-attached worker thread (using threadAttach()) in which all the
	 * API functions will be executed.
	 */
	inline void attach();

	/**
	 * Detach RemoteMono from the remote process, freeing all the memory used for the generated wrapper functions.
	 */
	inline void detach();

	/**
	 * Determines whether this instance is currently attached using attach().
	 */
	inline bool isAttached() const;

	/**
	 * Determines whether the given Mono API function is supported on the remote. Note that this uses the full,
	 * original Mono API function name, e.g. *mono_runtime_invoke*.
	 *
	 * @param name The API function name to check for.
	 * @return true if supported, false otherwise
	 */
	inline bool isAPIFunctionSupported(const std::string& name) const;

	inline void setFreeBufferMaxCount(uint32_t maxCount);

	inline void flushFreeBuffers();

	///@}



	// ********** MONO API WRAPPERS **********

	inline void								free(rmono_voidp p);
	inline void								freeLater(rmono_voidp p);

	///@name Mono API - JIT Functions
	///@{
	inline RMonoDomainPtr					jitInit(const std::string_view& filename);
	inline void								jitCleanup(RMonoDomainPtr domain);
	///@}

	///@name Mono API - Domains
	///@{
	inline RMonoDomainPtr					getRootDomain();
	inline bool								domainSet(RMonoDomainPtr domain, bool force);
	inline RMonoDomainPtr					domainGet();
	inline std::vector<RMonoDomainPtr>		domainList();
	inline RMonoDomainPtr					domainCreateAppdomain (	const std::string_view& friendlyName,
																	const std::string_view& configFile = std::string_view());
	inline RMonoAssemblyPtr					domainAssemblyOpen(RMonoDomainPtr domain, const std::string_view& name);
	inline void								domainUnload(RMonoDomainPtr domain);
	inline std::string						domainGetFriendlyName(RMonoDomainPtr domain);
	///@}

	///@name Mono API - Threads
	///@{
	inline RMonoThreadPtr					threadAttach(RMonoDomainPtr domain);
	inline void								threadDetach(RMonoThreadPtr thread);
	///@}

	///@name Mono API - Assemblies
	///@{
	inline void								assemblyClose(RMonoAssemblyPtr assembly);
	inline std::vector<RMonoAssemblyPtr>	assemblyList();
	inline RMonoImagePtr					assemblyGetImage(RMonoAssemblyPtr assembly);
	inline RMonoAssemblyNamePtr				assemblyGetName(RMonoAssemblyPtr assembly);
	inline RMonoAssemblyNamePtr				assemblyNameNew(const std::string_view& name);
	inline bool								assemblyNameParse(const std::string_view& name, RMonoAssemblyNamePtr aname); // NOTE: Deprecated
	inline void								assemblyNameFree(RMonoAssemblyNamePtrRaw name);
	inline std::string						assemblyNameGetName(RMonoAssemblyNamePtr assembly);
	inline std::string						assemblyNameGetCulture(RMonoAssemblyNamePtr assembly);
	inline uint16_t							assemblyNameGetVersion (	RMonoAssemblyNamePtr assembly, uint16_t* minor,
																		uint16_t* build, uint16_t* revision);
	inline std::string						stringifyAssemblyName(RMonoAssemblyNamePtr assembly);
	inline std::string						assemblyNameStringify(RMonoAssemblyNamePtr assembly);
	inline RMonoAssemblyPtr					assemblyLoaded(RMonoAssemblyNamePtr name);
	inline RMonoAssemblyPtr					assemblyLoaded(const std::string_view& name);
	///@}

	///@name Mono API - Images
	///@{
	inline std::string						imageGetName(RMonoImagePtr image);
	inline std::string						imageGetFilename(RMonoImagePtr image);
	inline RMonoTableInfoPtr				imageGetTableInfo(RMonoImagePtr image, rmono_int tableID);
	inline rmono_int						tableInfoGetRows(RMonoTableInfoPtr table);
	inline rmono_voidp						imageRVAMap(RMonoImagePtr image, uint32_t addr);
	///@}

	///@name Mono API - Metadata Tables
	///@{
	inline uint32_t							metadataDecodeRowCol(RMonoTableInfoPtr table, rmono_int idx, rmono_uint col);
	inline rmono_voidp						metadataGuidHeap(RMonoImagePtr image, uint32_t idx, uint8_t* outGuid = nullptr);
	inline std::string						metadataStringHeap(RMonoImagePtr image, uint32_t idx);
	inline rmono_voidp						metadataStringHeapRaw(RMonoImagePtr image, uint32_t idx);
	inline rmono_voidp						metadataBlobHeap(RMonoImagePtr image, uint32_t idx);
	inline std::string						metadataUserString(RMonoImagePtr image, uint32_t idx);
	inline rmono_voidp						metadataUserStringRaw(RMonoImagePtr image, uint32_t idx);
	inline uint32_t							metadataDecodeBlobSize(rmono_voidp blobPtr, rmono_voidp* outBlobPtr);
	///@}

	///@name Mono API - Standard Classes
	///@{
	inline RMonoClassPtr					getObjectClass();
	inline RMonoClassPtr					getInt16Class();
	inline RMonoClassPtr					getInt32Class();
	inline RMonoClassPtr					getInt64Class();
	inline RMonoClassPtr					getDoubleClass();
	inline RMonoClassPtr					getSingleClass();
	inline RMonoClassPtr					getStringClass();
	inline RMonoClassPtr					getThreadClass();
	inline RMonoClassPtr					getUInt16Class();
	inline RMonoClassPtr					getUInt32Class();
	inline RMonoClassPtr					getUInt64Class();
	inline RMonoClassPtr					getVoidClass();
	inline RMonoClassPtr					getArrayClass();
	inline RMonoClassPtr					getBooleanClass();
	inline RMonoClassPtr					getByteClass();
	inline RMonoClassPtr					getSByteClass();
	inline RMonoClassPtr					getCharClass();
	inline RMonoClassPtr					getExceptionClass();
	///@}

	///@name Mono API - Classes
	///@{
	inline RMonoVTablePtr					classVTable(RMonoDomainPtr domain, RMonoClassPtr cls);
	inline RMonoVTablePtr					classVTable(RMonoClassPtr cls);
	inline void								runtimeClassInit(RMonoVTablePtr vtable);
	inline RMonoClassPtr					classGetParent(RMonoClassPtr cls);
	inline RMonoTypePtr						classGetType(RMonoClassPtr cls);
	inline RMonoClassPtr					classFromName(RMonoImagePtr image, const std::string_view& nameSpace, const std::string_view& name);
	inline RMonoClassPtr					classFromMonoType(RMonoTypePtr type);
	inline std::string						classGetName(RMonoClassPtr cls);
	inline std::string						classGetNamespace(RMonoClassPtr cls);
	inline std::vector<RMonoClassFieldPtr>	classGetFields(RMonoClassPtr cls);
	inline std::vector<RMonoMethodPtr>		classGetMethods(RMonoClassPtr cls);
	inline std::vector<RMonoPropertyPtr>	classGetProperties(RMonoClassPtr cls);
	inline RMonoClassFieldPtr				classGetFieldFromName(RMonoClassPtr cls, const std::string_view& name);
	inline RMonoMethodPtr					classGetMethodFromName(RMonoClassPtr cls, const std::string_view& name, int32_t paramCount = -1);
	inline RMonoPropertyPtr					classGetPropertyFromName(RMonoClassPtr cls, const std::string_view& name);
	inline RMonoClassPtr					classGetElementClass(RMonoClassPtr cls);
	inline uint32_t							classGetFlags(RMonoClassPtr cls);
	inline rmono_int						classGetRank(RMonoClassPtr cls);
	inline bool								classIsValueType(RMonoClassPtr cls);
	inline uint32_t							classDataSize(RMonoClassPtr cls);
	inline uint32_t							classInstanceSize(RMonoClassPtr cls);
	inline int32_t							classValueSize(RMonoClassPtr cls, uint32_t* align = nullptr);
	///@}

	///@name Mono API - Types
	///@{
	inline RMonoReflectionTypePtr			typeGetObject(RMonoDomainPtr domain, RMonoTypePtr type);
	inline RMonoReflectionTypePtr			typeGetObject(RMonoTypePtr type);
	inline std::string						typeGetName(RMonoTypePtr type);
	inline RMonoClassPtr					typeGetClass(RMonoTypePtr type);
	inline rmono_int						typeGetType(RMonoTypePtr type);
	inline bool								typeIsByRef(RMonoTypePtr type);
	inline bool								typeIsPointer(RMonoTypePtr type);
	inline bool								typeIsReference(RMonoTypePtr type);
	inline bool								typeIsStruct(RMonoTypePtr type);
	inline bool								typeIsVoid(RMonoTypePtr type);
	inline rmono_int						typeSize(RMonoTypePtr type, rmono_int* align = nullptr);
	inline rmono_int						typeStackSize(RMonoTypePtr type, rmono_int* align = nullptr);
	///@}

	///@name Mono API - Fields
	///@{
	inline RMonoClassPtr					fieldGetParent(RMonoClassFieldPtr field);
	inline RMonoTypePtr						fieldGetType(RMonoClassFieldPtr field);
	inline std::string						fieldGetName(RMonoClassFieldPtr field);
	inline uint32_t							fieldGetFlags(RMonoClassFieldPtr field);
	inline void								fieldSetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, const RMonoVariant& val);
	inline void								fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, RMonoVariant& val);
	inline void								fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field, RMonoVariant&& val);
	template <typename T> T					fieldGetValue(RMonoObjectPtr obj, RMonoClassFieldPtr field);
	inline RMonoObjectPtr					fieldGetValueObjectWithRetCls(RMonoClassPtr& retCls, RMonoDomainPtr domain, RMonoClassFieldPtr field, RMonoObjectPtr obj = nullptr);
	inline RMonoObjectPtr					fieldGetValueObjectWithRetCls(RMonoClassPtr& retCls, RMonoClassFieldPtr field, RMonoObjectPtr obj = nullptr);
	inline RMonoObjectPtr					fieldGetValueObject(RMonoDomainPtr domain, RMonoClassFieldPtr field, RMonoObjectPtr obj = nullptr);
	inline RMonoObjectPtr					fieldGetValueObject(RMonoClassFieldPtr field, RMonoObjectPtr obj = nullptr);
	inline void								fieldStaticSetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, const RMonoVariant& val);
	inline void								fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, RMonoVariant& val);
	inline void								fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field, RMonoVariant&& val);
	template <typename T> T					fieldStaticGetValue(RMonoVTablePtr vtable, RMonoClassFieldPtr field);
	inline uint32_t							fieldGetOffset(RMonoClassFieldPtr field);
	///@}

	///@name Mono API - Methods
	///@{
	inline RMonoClassPtr					methodGetClass(RMonoMethodPtr method);
	inline std::string						methodGetName(RMonoMethodPtr method);
	inline std::string						methodFullName(RMonoMethodPtr method, bool signature);
	inline uint32_t							methodGetFlags(RMonoMethodPtr method, uint32_t* iflags = nullptr);
	inline RMonoMethodSignaturePtr			methodSignature(RMonoMethodPtr method);
	inline RMonoMethodHeaderPtr				methodGetHeader(RMonoMethodPtr method);
	inline rmono_funcp						methodHeaderGetCode(RMonoMethodHeaderPtr header, uint32_t* codeSize, uint32_t* maxStack);
	inline RMonoMethodDescPtr				methodDescNew(const std::string_view& name, bool includeNamespace);
	inline void								methodDescFree(RMonoMethodDescPtrRaw desc);
	inline bool								methodDescMatch(RMonoMethodDescPtr desc, RMonoMethodPtr method);
	inline RMonoMethodPtr					methodDescSearchInClass(RMonoMethodDescPtr desc, RMonoClassPtr cls);
	inline RMonoMethodPtr					methodDescSearchInClass(const std::string_view& desc, bool includeNamespace, RMonoClassPtr cls);
	inline RMonoMethodPtr					methodDescSearchInImage(RMonoMethodDescPtr desc, RMonoImagePtr image);
	inline RMonoMethodPtr					methodDescSearchInImage(const std::string_view& desc, bool includeNamespace, RMonoImagePtr image);
	inline RMonoObjectPtr					runtimeInvokeWithRetCls (	RMonoClassPtr& retCls,
																		RMonoMethodPtr method,
																		const RMonoVariant& obj,
																		RMonoVariantArray& params,
																		bool catchExceptions = true);
	inline RMonoObjectPtr					runtimeInvokeWithRetCls (	RMonoClassPtr& retCls,
																		RMonoMethodPtr method,
																		const RMonoVariant& obj = nullptr,
																		RMonoVariantArray&& params = RMonoVariantArray(),
																		bool catchExceptions = true);
	inline RMonoObjectPtr					runtimeInvoke (	RMonoMethodPtr method, const RMonoVariant& obj,
															RMonoVariantArray& params, bool catchExceptions = true);
	inline RMonoObjectPtr					runtimeInvoke (	RMonoMethodPtr method, const RMonoVariant& obj = nullptr,
															RMonoVariantArray&& params = RMonoVariantArray(), bool catchExceptions = true);
	inline rmono_funcp						compileMethod(RMonoMethodPtr method);
	///@}

	///@name Mono API - Properties
	///@{
	inline std::string						propertyGetName(RMonoPropertyPtr prop);
	inline uint32_t							propertyGetFlags(RMonoPropertyPtr prop);
	inline RMonoClassPtr					propertyGetParent(RMonoPropertyPtr prop);
	inline RMonoMethodPtr					propertyGetSetMethod(RMonoPropertyPtr prop);
	inline RMonoMethodPtr					propertyGetGetMethod(RMonoPropertyPtr prop);
	inline RMonoObjectPtr					propertyGetValueWithRetCls (	RMonoClassPtr& retCls,
																			RMonoPropertyPtr prop,
																			const RMonoVariant& obj,
																			RMonoVariantArray& params,
																			bool catchExceptions = true);
	inline RMonoObjectPtr					propertyGetValueWithRetCls (	RMonoClassPtr& retCls,
																			RMonoPropertyPtr prop,
																			const RMonoVariant& obj = nullptr,
																			RMonoVariantArray&& params = RMonoVariantArray(),
																			bool catchExceptions = true);
	inline RMonoObjectPtr					propertyGetValue (	RMonoPropertyPtr prop, const RMonoVariant& obj,
																RMonoVariantArray& params, bool catchExceptions = true);
	inline RMonoObjectPtr					propertyGetValue (	RMonoPropertyPtr prop, const RMonoVariant& obj = nullptr,
																RMonoVariantArray&& params = RMonoVariantArray(), bool catchExceptions = true);
	inline void								propertySetValue (	RMonoPropertyPtr prop, const RMonoVariant& obj,
																RMonoVariantArray& params, bool catchExceptions = true);
	inline void								propertySetValue (	RMonoPropertyPtr prop, const RMonoVariant& obj = nullptr,
																RMonoVariantArray&& params = RMonoVariantArray(), bool catchExceptions = true);
	///@}

	///@name Mono API - Method Signatures
	///@{
	inline RMonoTypePtr						signatureGetReturnType(RMonoMethodSignaturePtr sig);
	inline uint32_t							signatureGetCallConv(RMonoMethodSignaturePtr sig);
	inline std::string						signatureGetDesc(RMonoMethodSignaturePtr sig, bool includeNamespace);
	inline std::vector<RMonoTypePtr>		signatureGetParams(RMonoMethodSignaturePtr sig);
	///@}

	///@name Mono API - Objects
	///@{
	inline RMonoClassPtr					objectGetClass(RMonoObjectPtr obj);
	inline RMonoObjectPtr					objectNew(RMonoDomainPtr domain, RMonoClassPtr cls);
	inline RMonoObjectPtr					objectNew(RMonoClassPtr cls);
	inline void								runtimeObjectInit(const RMonoVariant& obj);
	template <typename T> T					objectUnbox(RMonoObjectPtr obj);
	inline RMonoVariant						objectUnboxRaw(RMonoObjectPtr obj);
	inline RMonoObjectPtr					valueBox(RMonoDomainPtr domain, RMonoClassPtr cls, const RMonoVariant& val);
	inline RMonoObjectPtr					valueBox(RMonoClassPtr cls, const RMonoVariant& val);
	inline RMonoStringPtr					objectToString(const RMonoVariant& obj, bool catchExceptions = true);
	inline RMonoObjectPtr					objectClone(RMonoObjectPtr obj);
	inline RMonoDomainPtr					objectGetDomain(RMonoObjectPtr obj);
	inline RMonoMethodPtr					objectGetVirtualMethod(RMonoObjectPtr obj, RMonoMethodPtr method);
	inline RMonoObjectPtr					objectIsInst(RMonoObjectPtr obj, RMonoClassPtr cls);
	inline rmono_uint						objectGetSize(RMonoObjectPtr obj);
	///@}

	///@name Mono API - Strings
	///@{
	inline RMonoStringPtr					stringNew(RMonoDomainPtr domain, const std::string_view& str);
	inline RMonoStringPtr					stringNew(const std::string_view& str);
	inline RMonoStringPtr					stringNewUTF16(RMonoDomainPtr domain, const std::u16string_view& str);
	inline RMonoStringPtr					stringNewUTF16(const std::u16string_view& str);
	inline RMonoStringPtr					stringNewUTF32(RMonoDomainPtr domain, const std::u32string_view& str);
	inline RMonoStringPtr					stringNewUTF32(const std::u32string_view& str);
	inline std::string						stringToUTF8(RMonoStringPtr str);
	inline std::u16string					stringToUTF16(RMonoStringPtr str);
	inline std::u32string					stringToUTF32(RMonoStringPtr str);
	inline std::u16string					stringChars(RMonoStringPtr str);
	inline int32_t							stringLength(RMonoStringPtr str);
	inline bool								stringEqual(RMonoStringPtr a, RMonoStringPtr b);
	///@}

	///@name Mono API - Arrays
	///@{
	inline RMonoArrayPtr					arrayNew(RMonoDomainPtr domain, RMonoClassPtr cls, rmono_uintptr_t n);
	inline RMonoArrayPtr					arrayNew(RMonoClassPtr cls, rmono_uintptr_t n);
	inline RMonoArrayPtr					arrayNewFull (	RMonoDomainPtr domain, RMonoClassPtr cls, const std::vector<rmono_uintptr_t>& lengths,
															const std::vector<rmono_intptr_t>& lowerBounds = {});
	inline RMonoArrayPtr					arrayNewFull (	RMonoClassPtr cls, const std::vector<rmono_uintptr_t>& lengths,
															const std::vector<rmono_intptr_t>& lowerBounds = {});
	inline RMonoClassPtr					arrayClassGet(RMonoClassPtr cls, uint32_t rank);
	inline rmono_voidp						arrayAddrWithSize(RMonoArrayPtr arr, rmono_int size, rmono_uintptr_t idx);
	inline rmono_uintptr_t					arrayLength(RMonoArrayPtr arr);
	inline int32_t							arrayElementSize(RMonoClassPtr cls);
	inline int32_t							classArrayElementSize(RMonoClassPtr cls);
	inline RMonoArrayPtr					arrayClone(RMonoArrayPtr arr);
	template <typename T> T					arrayGet(RMonoArrayPtr arr, rmono_uintptr_t idx);
	inline void								arraySet(RMonoArrayPtr arr, rmono_uintptr_t idx, const RMonoVariant& val);
	///@}

	///@name Mono API - GC Handles
	///@{
	inline rmono_gchandle					gchandleNew(RMonoObjectPtr obj, bool pinned);
	inline rmono_gchandle					gchandleNewRaw(RMonoObjectPtrRaw obj, bool pinned);
	inline rmono_gchandle					gchandleNewWeakref(RMonoObjectPtr obj, bool trackResurrection);
	inline rmono_gchandle					gchandleNewWeakrefRaw(RMonoObjectPtrRaw obj, bool trackResurrection);
	inline RMonoObjectPtrRaw				gchandleGetTarget(rmono_gchandle gchandle);
	inline void								gchandleFree(rmono_gchandle gchandle);
	inline void								gchandleFreeLater(rmono_gchandle gchandle);
	///@}

	///@name Mono API - Garbage Collector (GC)
	///@{
	inline void								gcCollect(rmono_int generation);
	inline rmono_int						gcMaxGeneration();
	inline rmono_int						gcGetGeneration(RMonoObjectPtr obj);
	///@}

	///@name Mono API - JIT Info
	///@{
	inline RMonoJitInfoPtr					jitInfoTableFind(RMonoDomainPtr domain, rmono_voidp addr);
	inline RMonoJitInfoPtr					jitInfoTableFind(rmono_voidp addr);
	inline rmono_funcp						jitInfoGetCodeStart(RMonoJitInfoPtr jinfo);
	inline int32_t							jitInfoGetCodeSize(RMonoJitInfoPtr jinfo);
	inline RMonoMethodPtr					jitInfoGetMethod(RMonoJitInfoPtr jinfo);
	///@}

	///@name Mono API - Miscellaneous
	///@{
	inline std::string						disasmCode(RMonoDisHelperPtr helper, RMonoMethodPtr method, rmono_voidp ip, rmono_voidp end);
	inline std::string						pmip(rmono_voidp ip);
	///@}




	// ********** UTILITY METHODS **********

	///@name Utilities
	///@{

	/**
	 * List all classes in the given image. The classes are queried from the metadata table MONO_TABLE_TYPEDEF.
	 *
	 * @param image The image for which to list classes.
	 * @return All classes in the image.
	 */
	inline std::vector<RMonoClassPtr>		listClasses(RMonoImagePtr image);

	/**
	 * A small convenience wrapper that combines objectToString() with stringToUTF8().
	 */
	inline std::string						objectToStringUTF8(RMonoObjectPtr obj, bool catchExceptions = true);

	template <typename T>
		std::vector<T>						arraySlice(RMonoArrayPtr arr, rmono_uintptr_t start, rmono_uintptr_t end);

	/**
	 * Converts the given MonoArray to a local std::vector. Can be used for both value types (e.g. `arrayAsVector<int32_t>(arr)`)
	 * and reference types (e.g. `arrayAsVector<RMonoObjectPtr>(arr)`). Note that this will always return a flat vector, even
	 * for multidimensional arrays.
	 */
	template <typename T>
		std::vector<T>						arrayAsVector(RMonoArrayPtr arr);

	/**
	 * Creates a new MonoArray from the values in the given std::vector. Can be used for both value types (e.g.
	 * `arrayFromVector<int32_t>(getInt32Class(), {1,2,3})` and reference types (e.g.
	 * `arrayFromVector<RMonoStringPtr>(getStringClass(), {mono.stringNew("A"), mono.stringNew("B")})`).
	 */
	template <typename T>
		RMonoArrayPtr						arrayFromVector(RMonoDomainPtr domain, RMonoClassPtr cls, const std::vector<T>& vec);

	/**
	 * Creates a new MonoArray from the values in the given std::vector. Can be used for both value types (e.g.
	 * `arrayFromVector<int32_t>(getInt32Class(), {1,2,3})` and reference types (e.g.
	 * `arrayFromVector<RMonoStringPtr>(getStringClass(), {mono.stringNew("A"), mono.stringNew("B")})`).
	 */
	template <typename T>
		RMonoArrayPtr						arrayFromVector(RMonoClassPtr cls, const std::vector<T>& vec);

	/**
	 * Return a new GC handle for the same raw pointer as the parameter, but in its pinned version (see mono_gchandle_new()).
	 */
	inline rmono_gchandle					gchandlePin(rmono_gchandle gchandle);

	///@}



private:
	inline void selectABI();

	template <typename ABI>
	backend::RMonoMemBlock prepareIterator();

	inline void checkAttached();

	template <typename FuncT>
	void checkAPIFunctionSupported(const FuncT& f);


private:
	bool attached;
	RMonoDomainPtr rootDomain;
	RMonoThreadPtr monoThread;
};



}
