// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_instance.h - WasmEdge C API ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about runtime instances (module instance,
/// function instance, table instance, memory instance, tag instance, and global
/// instance) in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_INSTANCE_H
#define WASMEDGE_C_API_INSTANCE_H

#include "wasmedge/wasmedge_basic.h"
#include "wasmedge/wasmedge_value.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge module instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_ModuleInstanceContext.
///
/// Create a module instance context with exported module name for host
/// instances. Developer can use this API to create a module instance for
/// collecting host functions, tables, memories, tags, and globals.
/// The caller owns the object and should call `WasmEdge_ModuleInstanceDelete`
/// to destroy it.
///
/// \param ModuleName the module name WasmEdge_String of this host module to
/// import.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreate(const WasmEdge_String ModuleName);

/// Creation of the WasmEdge_ModuleInstanceContext with host data.
///
/// Create a module instance context with exported module name, host data, and
/// host data finalizer for host instances. Developer can use this API to create
/// a module instance for collecting host functions, tables, memories, and
/// globals. When this created module instance being destroyed, the host data
/// finalizer will be invoked. The caller owns the object and should call
/// `WasmEdge_ModuleInstanceDelete` to destroy it.
///
/// \param ModuleName the module name WasmEdge_String of this host module to
/// import.
/// \param HostData the host data to set into the module instance. When calling
/// the finalizer, this pointer will become the argument of the finalizer
/// function.
/// \param Finalizer the function to finalize the host data.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreateWithData(const WasmEdge_String ModuleName,
                                      void *HostData,
                                      void (*Finalizer)(void *));

/// Creation of the WasmEdge_ModuleInstanceContext for the WASI specification.
///
/// This function will create a WASI host module that contains the WASI host
/// functions and initialize it. The caller owns the object and should call
/// `WasmEdge_ModuleInstanceDelete` to destroy it.
///
/// \param Args the command line arguments. The first argument suggests being
/// the program name. NULL if the length is 0.
/// \param ArgLen the length of the command line arguments.
/// \param Envs the environment variables in the format `ENV=VALUE`. NULL if the
/// length is 0.
/// \param EnvLen the length of the environment variables.
/// \param Preopens the directory paths to preopen. String format in
/// `GUEST_PATH:HOST_PATH` means the path mapping, or the same path will be
/// mapped. NULL if the length is 0.
/// \param PreopenLen the length of the directory paths to preopen.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreateWASI(const char *const *Args,
                                  const uint32_t ArgLen,
                                  const char *const *Envs,
                                  const uint32_t EnvLen,
                                  const char *const *Preopens,
                                  const uint32_t PreopenLen);

/// Same as WasmEdge_ModuleInstanceCreateWASI but with extended support for
/// File Descriptors
///
/// \param StdInFd  File descriptor to be mapped to WASI `stdin`.
/// \param StdOutFd File descriptor to be mapped to WASI `stdout`.
/// \param StdErrFd File descriptor to be mapped to WASI `stderr`.
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ModuleInstanceContext *
WasmEdge_ModuleInstanceCreateWASIWithFds(
    const char *const *Args, const uint32_t ArgLen, const char *const *Envs,
    const uint32_t EnvLen, const char *const *Preopens,
    const uint32_t PreopenLen, const int32_t StdInFd, const int32_t StdOutFd,
    const int32_t StdErrFd);
/// Initialize the WasmEdge_ModuleInstanceContext for the WASI specification.
///
/// This function will initialize the WASI host module with the parameters.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext of WASI import object.
/// \param Args the command line arguments. The first argument suggests being
/// the program name. NULL if the length is 0.
/// \param ArgLen the length of the command line arguments.
/// \param Envs the environment variables in the format `ENV=VALUE`. NULL if the
/// length is 0.
/// \param EnvLen the length of the environment variables.
/// \param Preopens the directory paths to preopen. String format in
/// `GUEST_PATH:HOST_PATH` means the path mapping, or the same path will be
/// mapped. NULL if the length is 0.
/// \param PreopenLen the length of the directory paths to preopen.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ModuleInstanceInitWASI(
    WasmEdge_ModuleInstanceContext *Cxt, const char *const *Args,
    const uint32_t ArgLen, const char *const *Envs, const uint32_t EnvLen,
    const char *const *Preopens, const uint32_t PreopenLen);

/// Same as WasmEdge_ModuleInstanceInitWASI but with extended support for File
/// Descriptors
///
/// \param StdInFd  File descriptor to be mapped to WASI `stdin`.
/// \param StdOutFd File descriptor to be mapped to WASI `stdout`.
/// \param StdErrFd File descriptor to be mapped to WASI `stderr`.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_ModuleInstanceInitWASIWithFds(
    WasmEdge_ModuleInstanceContext *Cxt, const char *const *Args,
    const uint32_t ArgLen, const char *const *Envs, const uint32_t EnvLen,
    const char *const *Preopens, const uint32_t PreopenLen,
    const int32_t StdInFd, const int32_t StdOutFd, const int32_t StdErrFd);

/// Get the WASI exit code.
///
/// This function will return the exit code after running the "_start" function
/// of a `wasm32-wasi` program.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext of WASI import object.
///
/// \returns the exit code after executing the "_start" function. Return
/// `EXIT_FAILURE` if the `Cxt` is NULL or not a WASI host module.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_ModuleInstanceWASIGetExitCode(
    const WasmEdge_ModuleInstanceContext *Cxt);

/// Get the native handler from the WASI mapped FD/Handler.
///
/// This function will return the raw FD/Handler from a given mapped Fd
/// or Handler.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext of WASI import object.
/// \param Fd the WASI mapped Fd.
/// \param [out] NativeHandler the raw Fd/Handler.
///
/// \returns the error code. Return `0` if the Native Handler is found.
/// Return `1` if the `Cxt` is `NULL`.
/// Return `2` if the given mapped Fd/handler is not found.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceWASIGetNativeHandler(
    const WasmEdge_ModuleInstanceContext *Cxt, int32_t Fd,
    uint64_t *NativeHandler);

/// Initialize the WasmEdge_ModuleInstanceContext for the wasmedge_process
/// specification.
///
/// This function will initialize the wasmedge_process host module with the
/// parameters.
///
/// \param AllowedCmds the allowed commands white list. NULL if the
/// length is 0.
/// \param CmdsLen the length of the allowed commands white list.
/// \param AllowAll the boolean value to allow all commands. `false` is
/// suggested. If this value is `true`, the allowed commands white list will not
/// be recorded and all commands can be executed by wasmedge_process.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceInitWasmEdgeProcess(const char *const *AllowedCmds,
                                           const uint32_t CmdsLen,
                                           const bool AllowAll);

/// Get the export module name of a module instance.
///
/// The returned string object is linked to the module name of the module
/// instance, and the caller should __NOT__ call the `WasmEdge_StringDelete`.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns string object. Length will be 0 and Buf will be NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_String
WasmEdge_ModuleInstanceGetModuleName(const WasmEdge_ModuleInstanceContext *Cxt);

/// Get the host data set into the module instance when creating.
///
/// The returned data is owned by the module instance, and will be passed into
/// the finalizer when deleting this module instance.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns host data. NULL if the module instance context is NULL or no host
/// data set into the module instance.
WASMEDGE_CAPI_EXPORT extern void *
WasmEdge_ModuleInstanceGetHostData(const WasmEdge_ModuleInstanceContext *Cxt);

/// Get the exported function instance context of a module instance.
///
/// The result function instance context links to the function instance in the
/// module instance context and owned by the module instance context, and the
/// caller should __NOT__ call the `WasmEdge_FunctionInstanceDelete`.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param Name the function name WasmEdge_String.
///
/// \returns pointer to the function instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_FunctionInstanceContext *
WasmEdge_ModuleInstanceFindFunction(const WasmEdge_ModuleInstanceContext *Cxt,
                                    const WasmEdge_String Name);

/// Get the exported table instance context of a module instance.
///
/// The result table instance context links to the table instance in the module
/// instance context and owned by the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_TableInstanceDelete`.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param Name the table name WasmEdge_String.
///
/// \returns pointer to the table instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_TableInstanceContext *
WasmEdge_ModuleInstanceFindTable(const WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name);

/// Get the exported memory instance context of a module instance.
///
/// The result memory instance context links to the memory instance in the
/// module instance context and owned by the module instance context, and the
/// caller should __NOT__ call the `WasmEdge_MemoryInstanceDelete`.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param Name the memory name WasmEdge_String.
///
/// \returns pointer to the memory instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_MemoryInstanceContext *
WasmEdge_ModuleInstanceFindMemory(const WasmEdge_ModuleInstanceContext *Cxt,
                                  const WasmEdge_String Name);

/// Get the exported tag instance context of a module instance.
///
/// The result tag instance context links to the tag instance in the
/// module instance context and owned by the module instance context.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param Name the tag name WasmEdge_String.
///
/// \returns pointer to the tag instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_TagInstanceContext *
WasmEdge_ModuleInstanceFindTag(const WasmEdge_ModuleInstanceContext *Cxt,
                               const WasmEdge_String Name);

/// Get the exported global instance context of a module instance.
///
/// The result global instance context links to the global instance in the
/// module instance context and owned by the module instance context, and the
/// caller should __NOT__ call the `WasmEdge_GlobalInstanceDelete`.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param Name the global name WasmEdge_String.
///
/// \returns pointer to the global instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_GlobalInstanceContext *
WasmEdge_ModuleInstanceFindGlobal(const WasmEdge_ModuleInstanceContext *Cxt,
                                  const WasmEdge_String Name);

/// Get the length of exported function list of a module instance.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns length of the exported function list.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_ModuleInstanceListFunctionLength(
    const WasmEdge_ModuleInstanceContext *Cxt);

/// List the exported function names of a module instance.
///
/// The returned function names filled into the `Names` array are linked to the
/// exported names of functions of the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the exported
/// function list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param [out] Names the output WasmEdge_String buffer of the function names.
/// \param Len the buffer length.
///
/// \returns actual exported function list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListFunction(const WasmEdge_ModuleInstanceContext *Cxt,
                                    WasmEdge_String *Names, const uint32_t Len);

/// Get the length of exported table list of a module instance.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns length of the exported table list.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_ModuleInstanceListTableLength(
    const WasmEdge_ModuleInstanceContext *Cxt);

/// List the exported table names of a module instance.
///
/// The returned table names filled into the `Names` array are linked to the
/// exported names of tables of the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the exported
/// table list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param [out] Names the output WasmEdge_String buffer of the table names.
/// \param Len the buffer length.
///
/// \returns actual exported table list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListTable(const WasmEdge_ModuleInstanceContext *Cxt,
                                 WasmEdge_String *Names, const uint32_t Len);

/// Get the length of exported memory list of a module instance.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns length of the exported memory list.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_ModuleInstanceListMemoryLength(
    const WasmEdge_ModuleInstanceContext *Cxt);

/// List the exported memory names of a module instance.
///
/// The returned memory names filled into the `Names` array are linked to the
/// exported names of memories of the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the exported
/// memory list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param [out] Names the output WasmEdge_String buffer of the memory names.
/// \param Len the buffer length.
///
/// \returns actual exported memory list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListMemory(const WasmEdge_ModuleInstanceContext *Cxt,
                                  WasmEdge_String *Names, const uint32_t Len);

/// Get the length of exported tag list of a module instance.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns length of the exported tag list.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListTagLength(const WasmEdge_ModuleInstanceContext *Cxt);

/// List the exported tag names of a module instance.
///
/// The returned tag names filled into the `Names` array are linked to the
/// exported names of tags of the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the exported
/// tag list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param [out] Names the output WasmEdge_String buffer of the tag names.
/// \param Len the buffer length.
///
/// \returns actual exported tag list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListTag(const WasmEdge_ModuleInstanceContext *Cxt,
                               WasmEdge_String *Names, const uint32_t Len);

/// Get the length of exported global list of a module instance.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
///
/// \returns length of the exported global list.
WASMEDGE_CAPI_EXPORT extern uint32_t WasmEdge_ModuleInstanceListGlobalLength(
    const WasmEdge_ModuleInstanceContext *Cxt);

/// List the exported global names of a module instance.
///
/// The returned global names filled into the `Names` array are linked to the
/// exported names of globals of the module instance context, and the caller
/// should __NOT__ call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the exported
/// global list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext.
/// \param [out] Names the output WasmEdge_String buffer of the global names.
/// \param Len the buffer length.
///
/// \returns actual exported global list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_ModuleInstanceListGlobal(const WasmEdge_ModuleInstanceContext *Cxt,
                                  WasmEdge_String *Names, const uint32_t Len);

/// Add a function instance context into a WasmEdge_ModuleInstanceContext.
///
/// Export and move the ownership of the function instance into the module
/// instance. The caller should __NOT__ access or destroy the function instance
/// context after calling this function.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext to add the function instance.
/// \param Name the export function name WasmEdge_String.
/// \param FuncCxt the WasmEdge_FunctionInstanceContext to add.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceAddFunction(WasmEdge_ModuleInstanceContext *Cxt,
                                   const WasmEdge_String Name,
                                   WasmEdge_FunctionInstanceContext *FuncCxt);

/// Add a table instance context into a WasmEdge_ModuleInstanceContext.
///
/// Export and move the ownership of the table instance into the module
/// instance. The caller should __NOT__ access or destroy the table instance
/// context after calling this function.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext to add the table instance.
/// \param Name the export table name WasmEdge_String.
/// \param TableCxt the WasmEdge_TableInstanceContext to add.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceAddTable(WasmEdge_ModuleInstanceContext *Cxt,
                                const WasmEdge_String Name,
                                WasmEdge_TableInstanceContext *TableCxt);

/// Add a memory instance context into a WasmEdge_ModuleInstanceContext.
///
/// Export and move the ownership of the memory instance into the module
/// instance. The caller should __NOT__ access or destroy the memory instance
/// context after calling this function.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext to add the memory instance.
/// \param Name the export memory name WasmEdge_String.
/// \param MemoryCxt the WasmEdge_MemoryInstanceContext to add.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceAddMemory(WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name,
                                 WasmEdge_MemoryInstanceContext *MemoryCxt);

/// Add a global instance context into a WasmEdge_ModuleInstanceContext.
///
/// Export and move the ownership of the global instance into the module
/// instance. The caller should __NOT__ access or destroy the global instance
/// context after calling this function.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext to add the global instance.
/// \param Name the export global name WasmEdge_String.
/// \param GlobalCxt the WasmEdge_GlobalInstanceContext to add.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceAddGlobal(WasmEdge_ModuleInstanceContext *Cxt,
                                 const WasmEdge_String Name,
                                 WasmEdge_GlobalInstanceContext *GlobalCxt);

/// Deletion of the WasmEdge_ModuleInstanceContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
/// If the module instance has been registered into one or more store contexts,
/// it will be automatically unregistered.
///
/// \param Cxt the WasmEdge_ModuleInstanceContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ModuleInstanceDelete(WasmEdge_ModuleInstanceContext *Cxt);

// <<<<<<<< WasmEdge module instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge function instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

typedef WasmEdge_Result (*WasmEdge_HostFunc_t)(
    void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
    const WasmEdge_Value *Params, WasmEdge_Value *Returns);
/// Creation of the WasmEdge_FunctionInstanceContext for host functions.
///
/// The caller owns the object and should call `WasmEdge_FunctionInstanceDelete`
/// to destroy it if the returned object is not added into a
/// `WasmEdge_ModuleInstanceContext`. The following is an example to create a
/// host function context.
/// ```c
/// WasmEdge_Result FuncAdd(void *Data,
///                         const WasmEdge_CallingFrameContext *CallFrameCxt,
///                         const WasmEdge_Value *In, WasmEdge_Value *Out) {
///   // Function to return A + B.
///   int32_t A = WasmEdge_ValueGetI32(In[0]);
///   int32_t B = WasmEdge_ValueGetI32(In[1]);
///   Out[0] = WasmEdge_ValueGenI32(A + B);
///   // Return execution status
///   return WasmEdge_Result_Success;
/// }
///
/// WasmEdge_ValType Params[2] = {WasmEdge_ValTypeGenI32(),
///                               WasmEdge_ValTypeGenI32()};
/// WasmEdge_ValType Returns[1] = {WasmEdge_ValTypeGenI32()};
/// WasmEdge_FunctionTypeContext *FuncType =
///     WasmEdge_FunctionTypeCreate(Params, 2, Returns, 1);
/// WasmEdge_FunctionInstanceContext *HostFunc =
///     WasmEdge_FunctionInstanceCreate(FuncType, FuncAdd, NULL, 0);
/// WasmEdge_FunctionTypeDelete(FuncType);
/// ...
/// ```
///
/// \param Type the function type context to describe the host function
/// signature.
/// \param HostFunc the host function pointer. The host function signature must
/// be as following:
/// ```c
/// typedef WasmEdge_Result (*WasmEdge_HostFunc_t)(
///     void *Data,
///     const WasmEdge_CallingFrameContext *CallFrameCxt,
///     const WasmEdge_Value *Params,
///     WasmEdge_Value *Returns);
/// ```
/// The `Params` is the input parameters array with length guaranteed to be the
/// same as the parameter types in the `Type`. The `Returns` is the output
/// results array with length guaranteed to be the same as the result types in
/// the `Type`. The return value is `WasmEdge_Result` for the execution status.
/// \param Data the additional object, such as the pointer to a data structure,
/// to set to this host function context. The caller should guarantee the life
/// cycle of the object. NULL if the additional data object is not needed.
/// \param Cost the function cost in statistics. Pass 0 if the calculation is
/// not needed.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_FunctionInstanceContext *
WasmEdge_FunctionInstanceCreate(const WasmEdge_FunctionTypeContext *Type,
                                WasmEdge_HostFunc_t HostFunc, void *Data,
                                const uint64_t Cost);

typedef WasmEdge_Result (*WasmEdge_WrapFunc_t)(
    void *This, void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
    const WasmEdge_Value *Params, const uint32_t ParamLen,
    WasmEdge_Value *Returns, const uint32_t ReturnLen);
/// Creation of the WasmEdge_FunctionInstanceContext for host functions.
///
/// This function is for the languages which cannot pass the function pointer of
/// the host function into this shared library directly. The caller owns the
/// object and should call `WasmEdge_FunctionInstanceDelete` to destroy it if
/// the returned object is not added into a `WasmEdge_ModuleInstanceContext`.
/// The following is an example to create a host function context for other
/// languages.
/// ```c
/// // `RealFunc` is the pointer to the function in other languages.
///
/// WasmEdge_Result FuncAddWrap(
///     void *This, void *Data,
///     const WasmEdge_CallingFrameContext *CallFrameCxt,
///     const WasmEdge_Value *In, const uint32_t InLen, WasmEdge_Value *Out,
///     const uint32_t OutLen) {
///   // Wrapper function of host function to return A + B.
///
///   // `This` is the same as `RealFunc`.
///   int32_t A = WasmEdge_ValueGetI32(In[0]);
///   int32_t B = WasmEdge_ValueGetI32(In[1]);
///
///   // Call the function of `This` in the host language ...
///   int32_t Result = ...;
///
///   Out[0] = Result;
///   // Return the execution status.
///   return WasmEdge_Result_Success;
/// }
///
/// WasmEdge_ValType Params[2] = {WasmEdge_ValTypeGenI32(),
///                               WasmEdge_ValTypeGenI32()};
/// WasmEdge_ValType Returns[1] = {WasmEdge_ValTypeGenI32()};
/// WasmEdge_FunctionTypeContext *FuncType =
///     WasmEdge_FunctionTypeCreate(Params, 2, Returns, 1);
/// WasmEdge_FunctionInstanceContext *HostFunc =
///     WasmEdge_FunctionInstanceCreateBinding(
///         FuncType, FuncAddWrap, RealFunc, NULL, 0);
/// WasmEdge_FunctionTypeDelete(FuncType);
/// ...
/// ```
///
/// \param Type the function type context to describe the host function
/// signature.
/// \param WrapFunc the wrapper function pointer. The wrapper function signature
/// must be as following:
/// ```c
/// typedef WasmEdge_Result (*WasmEdge_WrapFunc_t)(
///     void *This,
///     void *Data,
///     WasmEdge_CallingFrameContext *FrameCxt,
///     const WasmEdge_Value *Params,
///     const uint32_t ParamLen,
///     WasmEdge_Value *Returns,
///     const uint32_t ReturnLen);
/// ```
/// The `This` is the pointer the same as the `Binding` parameter of this
/// function. The `Params` is the input parameters array with length guaranteed
/// to be the same as the parameter types in the `Type`, and the `ParamLen` is
/// the length of the array. The `Returns` is the output results array with
/// length guaranteed to be the same as the result types in the `Type`, and the
/// `ReturnLen` is the length of the array. The return value is
/// `WasmEdge_Result` for the execution status.
/// \param Binding the `this` pointer of the host function target or the
/// function indexing maintained by the caller which can specify the host
/// function. When invoking the host function, this pointer will be the first
/// argument of the wrapper function.
/// \param Data the additional object, such as the pointer to a data structure,
/// to set to this host function context. The caller should guarantee the life
/// cycle of the object. NULL if the additional data object is not needed.
/// \param Cost the function cost in statistics. Pass 0 if the calculation is
/// not needed.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_FunctionInstanceContext *
WasmEdge_FunctionInstanceCreateBinding(const WasmEdge_FunctionTypeContext *Type,
                                       WasmEdge_WrapFunc_t WrapFunc,
                                       void *Binding, void *Data,
                                       const uint64_t Cost);

/// Get the function data field of the function instance.
///
/// The function data is passed when creating the FunctionInstance.
///
/// \param Cxt the WasmEdge_FunctionInstanceContext.
///
/// \returns pointer to Data, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const void *
WasmEdge_FunctionInstanceGetData(const WasmEdge_FunctionInstanceContext *Cxt);

/// Get the function type context of the function instance.
///
/// The function type context links to the function type in the function
/// instance context and owned by the context. The caller should __NOT__ call
/// the `WasmEdge_FunctionTypeDelete`.
///
/// \param Cxt the WasmEdge_FunctionInstanceContext.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_FunctionTypeContext *
WasmEdge_FunctionInstanceGetFunctionType(
    const WasmEdge_FunctionInstanceContext *Cxt);

/// Deletion of the WasmEdge_FunctionInstanceContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_FunctionInstanceContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_FunctionInstanceDelete(WasmEdge_FunctionInstanceContext *Cxt);

// <<<<<<<< WasmEdge function instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge table instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_TableInstanceContext.
///
/// The caller owns the object and should call `WasmEdge_TableInstanceDelete` to
/// destroy it if the returned object is not added into a
/// `WasmEdge_ModuleInstanceContext`.
/// The default value of the elements in the output table instance will be null
/// references with the same reference type in the table type when table grows.
/// If the reference type of the input table type is a non-nullable value type,
/// a non-null default init value is required. In this case, please use the
/// `WasmEdge_TableInstanceCreateWithInit` API instead.
///
/// \param TabType the table type context to initialize the table instance
/// context.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreate(const WasmEdge_TableTypeContext *TabType);

/// Creation of the WasmEdge_TableInstanceContext with the default init value.
///
/// The caller owns the object and should call `WasmEdge_TableInstanceDelete` to
/// destroy it if the returned object is not added into a
/// `WasmEdge_ModuleInstanceContext`.
/// The value type of the default init value should compatible with the
/// reference type of the input table type, otherwise this function will fail.
/// If the reference type of the input table type is a non-nullable value type,
/// this function will fail if the default init value is a null reference.
///
/// \param TabType the table type context to initialize the table instance
/// context.
/// \param Value the default init value for the table element when table
/// grows.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_TableInstanceContext *
WasmEdge_TableInstanceCreateWithInit(const WasmEdge_TableTypeContext *TabType,
                                     const WasmEdge_Value Value);

/// Get the table type context from a table instance.
///
/// The table type context links to the table type in the table instance context
/// and owned by the context. The caller should __NOT__ call the
/// `WasmEdge_TableTypeDelete`.
///
/// \param Cxt the WasmEdge_TableInstanceContext.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TableTypeContext *
WasmEdge_TableInstanceGetTableType(const WasmEdge_TableInstanceContext *Cxt);

/// Get the reference value in a table instance.
///
/// \param Cxt the WasmEdge_TableInstanceContext.
/// \param [out] Data the result reference value.
/// \param Offset the reference value offset (index) in the table instance.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_TableInstanceGetData(const WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value *Data, const uint32_t Offset);

/// Set the reference value into a table instance.
///
/// \param Cxt the WasmEdge_TableInstanceContext.
/// \param Data the reference value to set into the table instance.
/// \param Offset the reference value offset (index) in the table instance.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_TableInstanceSetData(WasmEdge_TableInstanceContext *Cxt,
                              WasmEdge_Value Data, const uint32_t Offset);

/// Get the size of a table instance.
///
/// \param Cxt the WasmEdge_TableInstanceContext.
///
/// \returns the size of the table instance.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_TableInstanceGetSize(const WasmEdge_TableInstanceContext *Cxt);

/// Grow a table instance with a size.
///
/// \param Cxt the WasmEdge_TableInstanceContext.
/// \param Size the count of reference values to grow in the table instance.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_TableInstanceGrow(WasmEdge_TableInstanceContext *Cxt,
                           const uint32_t Size);

/// Deletion of the WasmEdge_TableInstanceContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_TableInstanceContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_TableInstanceDelete(WasmEdge_TableInstanceContext *Cxt);

// <<<<<<<< WasmEdge table instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge memory instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_MemoryInstanceContext.
///
/// The caller owns the object and should call `WasmEdge_MemoryInstanceDelete`
/// to destroy it if the returned object is not added into a
/// `WasmEdge_ModuleInstanceContext`.
///
/// \param MemType the memory type context to initialize the memory instance
/// context.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_MemoryInstanceContext *
WasmEdge_MemoryInstanceCreate(const WasmEdge_MemoryTypeContext *MemType);

/// Get the memory type context from a memory instance.
///
/// The memory type context links to the memory type in the memory instance
/// context and owned by the context. The caller should __NOT__ call the
/// `WasmEdge_MemoryTypeDelete`.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_MemoryTypeContext *
WasmEdge_MemoryInstanceGetMemoryType(const WasmEdge_MemoryInstanceContext *Cxt);

/// Copy the data to the output buffer from a memory instance.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
/// \param [out] Data the result data buffer of copying destination.
/// \param Offset the data start offset in the memory instance.
/// \param Length the requested data length. If the `Offset + Length` is larger
/// than the data size in the memory instance, this function will failed.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_MemoryInstanceGetData(const WasmEdge_MemoryInstanceContext *Cxt,
                               uint8_t *Data, const uint32_t Offset,
                               const uint32_t Length);

/// Copy the data into a memory instance from the input buffer.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
/// \param Data the data buffer to copy.
/// \param Offset the data start offset in the memory instance.
/// \param Length the data buffer length. If the `Offset + Length` is larger
/// than the data size in the memory instance, this function will failed.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_MemoryInstanceSetData(WasmEdge_MemoryInstanceContext *Cxt,
                               const uint8_t *Data, const uint32_t Offset,
                               const uint32_t Length);

/// Get the data pointer in a memory instance.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
/// \param Offset the data start offset in the memory instance.
/// \param Length the requested data length. If the `Offset + Length` is larger
/// than the data size in the memory instance, this function will return NULL.
///
/// \returns the pointer to data with the start offset. NULL if failed.
WASMEDGE_CAPI_EXPORT extern uint8_t *
WasmEdge_MemoryInstanceGetPointer(WasmEdge_MemoryInstanceContext *Cxt,
                                  const uint32_t Offset, const uint32_t Length);

/// Get the const data pointer in a const memory instance.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
/// \param Offset the data start offset in the memory instance.
/// \param Length the requested data length. If the `Offset + Length` is larger
/// than the data size in the memory instance, this function will return NULL.
///
/// \returns the pointer to data with the start offset. NULL if failed.
WASMEDGE_CAPI_EXPORT extern const uint8_t *
WasmEdge_MemoryInstanceGetPointerConst(
    const WasmEdge_MemoryInstanceContext *Cxt, const uint32_t Offset,
    const uint32_t Length);

/// Get the current page size (64 KiB of each page) of a memory instance.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
///
/// \returns the page size of the memory instance.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_MemoryInstanceGetPageSize(const WasmEdge_MemoryInstanceContext *Cxt);

/// Grow a memory instance with a page size.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext.
/// \param Page the page count to grow in the memory instance.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_MemoryInstanceGrowPage(WasmEdge_MemoryInstanceContext *Cxt,
                                const uint32_t Page);

/// Deletion of the WasmEdge_MemoryInstanceContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_MemoryInstanceContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_MemoryInstanceDelete(WasmEdge_MemoryInstanceContext *Cxt);

// <<<<<<<< WasmEdge memory instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge tag instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the tag type context from a tag instance.
///
/// The tag type context links to the tag type in the tag instance
/// context and owned by the context.
///
/// \param Cxt the WasmEdge_TagInstanceContext.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_TagTypeContext *
WasmEdge_TagInstanceGetTagType(const WasmEdge_TagInstanceContext *Cxt);

// <<<<<<<< WasmEdge tag instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge global instance functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_GlobalInstanceContext.
///
/// The caller owns the object and should call `WasmEdge_GlobalInstanceDelete`
/// to destroy it if the returned object is not added into a
/// `WasmEdge_ModuleInstanceContext`.
///
/// \param GlobType the global type context to initialize the global instance
/// context.
/// \param Value the initial value with its value type of the global instance.
/// This function will fail if the value type of `GlobType` and `Value` are not
/// the same.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_GlobalInstanceContext *
WasmEdge_GlobalInstanceCreate(const WasmEdge_GlobalTypeContext *GlobType,
                              const WasmEdge_Value Value);

/// Get the global type context from a global instance.
///
/// The global type context links to the global type in the global instance
/// context and owned by the context. The caller should __NOT__ call the
/// `WasmEdge_GlobalTypeDelete`.
///
/// \param Cxt the WasmEdge_GlobalInstanceContext.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_GlobalTypeContext *
WasmEdge_GlobalInstanceGetGlobalType(const WasmEdge_GlobalInstanceContext *Cxt);

/// Get the value from a global instance.
///
/// \param Cxt the WasmEdge_GlobalInstanceContext.
///
/// \returns the current value of the global instance.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Value
WasmEdge_GlobalInstanceGetValue(const WasmEdge_GlobalInstanceContext *Cxt);

/// Set the value into a global instance.
///
/// This function will return error if the global context is set as the `Const`
/// mutation or the value type not matched.
///
/// \param Cxt the WasmEdge_GlobalInstanceContext.
/// \param Value the value to set into the global context.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_GlobalInstanceSetValue(WasmEdge_GlobalInstanceContext *Cxt,
                                const WasmEdge_Value Value);

/// Deletion of the WasmEdge_GlobalInstanceContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_GlobalInstanceContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_GlobalInstanceDelete(WasmEdge_GlobalInstanceContext *Cxt);

// <<<<<<<< WasmEdge global instance functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_INSTANCE_H
