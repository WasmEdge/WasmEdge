// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_execution.h - WasmEdge C API --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about WASM execution (loader, validator,
/// executor, calling frame, store, and async) in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_EXECUTION_H
#define WASMEDGE_C_API_EXECUTION_H

#include "wasmedge/wasmedge_basic.h"
#include "wasmedge/wasmedge_value.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge loader functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_LoaderContext.
///
/// The caller owns the object and should call `WasmEdge_LoaderDelete` to
/// destroy it.
///
/// \param ConfCxt the WasmEdge_ConfigureContext as the configuration of Loader.
/// NULL for the default configuration.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_LoaderContext *
WasmEdge_LoaderCreate(const WasmEdge_ConfigureContext *ConfCxt);

/// Load and parse the WASM module from a WASM file into a
/// WasmEdge_ASTModuleContext.
///
/// Load and parse the WASM module from the file path, and return a
/// `WasmEdge_ASTModuleContext` as the result. The caller owns the
/// `WasmEdge_ASTModuleContext` object and should call
/// `WasmEdge_ASTModuleDelete` to destroy it.
///
/// \param Cxt the WasmEdge_LoaderContext.
/// \param [out] Module the output WasmEdge_ASTModuleContext if succeeded.
/// \param Path the NULL-terminated C string of the WASM file path.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_LoaderParseFromFile(WasmEdge_LoaderContext *Cxt,
                             WasmEdge_ASTModuleContext **Module,
                             const char *Path);

/// Load and parse the WASM module from a WasmEdge_Bytes into
/// WasmEdge_ASTModuleContext.
///
/// Load and parse the WASM module from a buffer, and return a
/// WasmEdge_ASTModuleContext as the result. The caller owns the
/// WasmEdge_ASTModuleContext object and should call `WasmEdge_ASTModuleDelete`
/// to destroy it.
///
/// \param Cxt the WasmEdge_LoaderContext.
/// \param [out] Module the output WasmEdge_ASTModuleContext if succeeded.
/// \param Bytes the WasmEdge_Bytes of WASM binary.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_LoaderParseFromBytes(WasmEdge_LoaderContext *Cxt,
                              WasmEdge_ASTModuleContext **Module,
                              const WasmEdge_Bytes Bytes);

/// Serialize the WasmEdge_ASTModuleContext into WASM binary.
///
/// Serialize the loaded WasmEdge_ASTModuleContext into the WASM binary format.
/// If the serialization succeeded, this API will allocate a new
/// `WasmEdge_Bytes` object and fill into the `Buf`. The caller owns the
/// `WasmEdge_Bytes` object and should call `WasmEdge_BytesDelete` to destroy
/// it.
///
/// \param Cxt the WasmEdge_LoaderContext.
/// \param ASTCxt the WasmEdge_ASTModuleContext to serialize.
/// \param [out] Buf the WasmEdge_Bytes to fill the serialized WASM binary.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_LoaderSerializeASTModule(WasmEdge_LoaderContext *Cxt,
                                  const WasmEdge_ASTModuleContext *ASTCxt,
                                  WasmEdge_Bytes *Buf);

/// Deletion of the WasmEdge_LoaderContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_LoaderContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_LoaderDelete(WasmEdge_LoaderContext *Cxt);

// <<<<<<<< WasmEdge loader functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge validator functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_ValidatorContext.
///
/// The caller owns the object and should call `WasmEdge_ValidatorDelete` to
/// destroy it.
///
/// \param ConfCxt the WasmEdge_ConfigureContext as the configuration of
/// Validator. NULL for the default configuration.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ValidatorContext *
WasmEdge_ValidatorCreate(const WasmEdge_ConfigureContext *ConfCxt);

/// Validate the WasmEdge AST Module.
///
/// \param Cxt the WasmEdge_ValidatorContext.
/// \param ASTCxt the WasmEdge_ASTModuleContext to validate.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_ValidatorValidate(WasmEdge_ValidatorContext *Cxt,
                           const WasmEdge_ASTModuleContext *ASTCxt);

/// Deletion of the WasmEdge_ValidatorContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_ValidatorContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ValidatorDelete(WasmEdge_ValidatorContext *Cxt);

// <<<<<<<< WasmEdge validator functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge executor functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_ExecutorContext.
///
/// The caller owns the object and should call `WasmEdge_ExecutorDelete` to
/// delete it.
///
/// \param ConfCxt the WasmEdge_ConfigureContext as the configuration of
/// Executor. NULL for the default configuration.
/// \param StatCxt the WasmEdge_StatisticsContext as the statistics object set
/// into Executor. The statistics will refer to this context, and the life cycle
/// should be guaranteed until the executor context is deleted. NULL for not
/// doing the statistics.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ExecutorContext *
WasmEdge_ExecutorCreate(const WasmEdge_ConfigureContext *ConfCxt,
                        WasmEdge_StatisticsContext *StatCxt);

/// Instantiate an AST Module into a module instance.
///
/// Instantiate an AST Module, and return an instantiated module instance
/// context as the result. The caller owns the object and should call
/// `WasmEdge_ModuleInstanceDelete` to destroy it. Developers can use the
/// `WasmEdge_ModuleInstanceListFunction`,
/// `WasmEdge_ModuleInstanceFindFunction`, etc. APIs to retrieve the exported
/// instances from the result module instance.
///
/// \param Cxt the WasmEdge_ExecutorContext to instantiate the module.
/// \param [out] ModuleCxt the output WasmEdge_ModuleInstanceContext if
/// succeeded.
/// \param StoreCxt the WasmEdge_StoreContext to link the imports.
/// \param ASTCxt the WasmEdge AST Module context generated by loader or
/// compiler.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result WasmEdge_ExecutorInstantiate(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_ModuleInstanceContext **ModuleCxt,
    WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt);

/// Instantiate an AST Module into a named module instance and link into store.
///
/// Instantiate an AST Module with the module name, return the instantiated
/// module instance context as the result, and also register the module instance
/// to the store. The caller owns the object and should call
/// `WasmEdge_ModuleInstanceDelete` to destroy it.
/// Developers can use the `WasmEdge_ModuleInstanceListFunction`,
/// `WasmEdge_ModuleInstanceFindFunction`, etc. APIs to retrieve the exported
/// instances from the result module instance.
/// After calling this function, the output module instance will also be
/// registered into the store, and the other modules can import the exported
/// instances for linking when instantiation. Developers SHOULD guarantee the
/// life cycle of this output module instance, or the error will occur when in
/// execution after the module instance being destroyed if it has been imported
/// by other modules. That is, developers have the responsibility to delete the
/// output module instance even though the store being destroyed. When the
/// module instance is deleted, it will be unregistered to the store
/// automatically.
///
/// \param Cxt the WasmEdge_ExecutorContext to instantiate the module.
/// \param [out] ModuleCxt the output WasmEdge_ModuleInstanceContext if
/// succeeded.
/// \param StoreCxt the WasmEdge_StoreContext to link the imports.
/// \param ASTCxt the WasmEdge AST Module context generated by loader or
/// compiler.
/// \param ModuleName the module name WasmEdge_String for all exported
/// instances.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result WasmEdge_ExecutorRegister(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_ModuleInstanceContext **ModuleCxt,
    WasmEdge_StoreContext *StoreCxt, const WasmEdge_ASTModuleContext *ASTCxt,
    WasmEdge_String ModuleName);

/// Register a module instance into a store with exporting its module name.
///
/// Register an existing module into the store with its module name.
/// After calling this function, the existing module instance will be registered
/// into the store, and the other modules can import the exported instances for
/// linking when instantiation. Developers SHOULD guarantee the life cycle of
/// this existing module instance, or the error will occur when in execution
/// after the module instance being destroyed if it has been imported by other
/// modules. When the module instance is deleted, it will be unregistered to the
/// store automatically.
///
/// \param Cxt the WasmEdge_ExecutorContext to instantiate the module.
/// \param StoreCxt the WasmEdge_StoreContext to store the instantiated module.
/// \param ImportCxt the WasmEdge_ModuleInstanceContext to register.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result WasmEdge_ExecutorRegisterImport(
    WasmEdge_ExecutorContext *Cxt, WasmEdge_StoreContext *StoreCxt,
    const WasmEdge_ModuleInstanceContext *ImportCxt);

/// Invoke a WASM function by the function instance.
///
/// After instantiating a WASM module, developers can get the function instance
/// context from the module instance. Then developers can invoke the function
/// through this API.
///
/// \param Cxt the WasmEdge_ExecutorContext.
/// \param FuncCxt the function instance context to invoke.
/// \param Params the WasmEdge_Value buffer with the parameter values.
/// \param ParamLen the parameter buffer length.
/// \param [out] Returns the WasmEdge_Value buffer to fill the return values.
/// \param ReturnLen the return buffer length.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_ExecutorInvoke(WasmEdge_ExecutorContext *Cxt,
                        const WasmEdge_FunctionInstanceContext *FuncCxt,
                        const WasmEdge_Value *Params, const uint32_t ParamLen,
                        WasmEdge_Value *Returns, const uint32_t ReturnLen);

/// Asynchronous invoke a WASM function by the function instance.
///
/// After instantiating a WASM module, developers can get the function instance
/// context from the module instance. Then developers can invoke the function
/// asynchronously through this API.
///
/// \param Cxt the WasmEdge_ExecutorContext.
/// \param FuncCxt the function instance context to invoke.
/// \param Params the WasmEdge_Value buffer with the parameter values.
/// \param ParamLen the parameter buffer length.
///
/// \returns WasmEdge_Async. Call `WasmEdge_AsyncGet` for the result, and call
/// `WasmEdge_AsyncDelete` to destroy this object.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Async *
WasmEdge_ExecutorAsyncInvoke(WasmEdge_ExecutorContext *Cxt,
                             const WasmEdge_FunctionInstanceContext *FuncCxt,
                             const WasmEdge_Value *Params,
                             const uint32_t ParamLen);

/// Deletion of the WasmEdge_ExecutorContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_ExecutorContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ExecutorDelete(WasmEdge_ExecutorContext *Cxt);

// <<<<<<<< WasmEdge executor functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge store functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_StoreContext.
///
/// The caller owns the object and should call `WasmEdge_StoreDelete` to destroy
/// it.
/// The store is the linker for multiple WASM module instances. The store will
/// not own any module instance registered into it, and the module instances
/// will automatically be unregistered if they are destroyed.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_StoreContext *WasmEdge_StoreCreate(void);

/// Get the module instance context by the module name.
///
/// After registering a WASM module, developers can call this function to find
/// and get the registered module instance context by the module name.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_StoreContext.
/// \param Name the module name WasmEdge_String.
///
/// \returns pointer to the module instance context. NULL if not found.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_ModuleInstanceContext *
WasmEdge_StoreFindModule(const WasmEdge_StoreContext *Cxt,
                         const WasmEdge_String Name);

/// Get the length of registered module list in store.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_StoreContext.
///
/// \returns length of registered named module list.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_StoreListModuleLength(const WasmEdge_StoreContext *Cxt);

/// List the registered module names.
///
/// This function will list all registered module names.
/// The returned module names filled into the `Names` array are linked to the
/// registered module names in the store context, and the caller should __NOT__
/// call the `WasmEdge_StringDelete`.
/// If the `Names` buffer length is smaller than the result of the registered
/// named module list size, the overflowed return values will be discarded.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_StoreContext.
/// \param [out] Names the output names WasmEdge_String buffer of named modules.
/// \param Len the buffer length.
///
/// \returns actual registered named module list size.
WASMEDGE_CAPI_EXPORT extern uint32_t
WasmEdge_StoreListModule(const WasmEdge_StoreContext *Cxt,
                         WasmEdge_String *Names, const uint32_t Len);

/// Deletion of the WasmEdge_StoreContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
/// If there are module instances registered into this store context, they will
/// be automatically un-link to this store context.
///
/// \param Cxt the WasmEdge_StoreContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_StoreDelete(WasmEdge_StoreContext *Cxt);

// <<<<<<<< WasmEdge store functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge calling frame functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Get the executor context from the current calling frame.
///
/// \param Cxt the WasmEdge_CallingFrameContext.
///
/// \returns the executor context, NULL if the Cxt is NULL.
WASMEDGE_CAPI_EXPORT extern WasmEdge_ExecutorContext *
WasmEdge_CallingFrameGetExecutor(const WasmEdge_CallingFrameContext *Cxt);

/// Get the module instance of the current calling frame.
///
/// When a WASM function is executing and start to call a host function, a frame
/// with the module instance which the WASM function belongs to will be pushed
/// onto the stack. And therefore the calling frame context will record that
/// module instance.
/// So in one case that the module instance will be `NULL`: developers execute
/// the function instance which is a host function and not added into a module
/// instance.
///
/// \param Cxt the WasmEdge_CallingFrameContext.
///
/// \returns the module instance of the current calling frame.
WASMEDGE_CAPI_EXPORT extern const WasmEdge_ModuleInstanceContext *
WasmEdge_CallingFrameGetModuleInstance(const WasmEdge_CallingFrameContext *Cxt);

/// Get the memory instance by index from the module instance of the current
/// calling frame.
///
/// By default, a WASM module only have one memory instance after instantiation.
/// Therefore, developers can use:
///   `WasmEdge_CallingFrameGetMemoryInstance(Cxt, 0)`
/// to get the memory instance in host function body.
/// This extension is for the WASM multiple memories proposal. After enabling
/// the proposal, there may be greater than 1 memory instances in a WASM module.
/// So developers can use this function to access the memory instances which are
/// not in 0 index.
///
/// \param Cxt the WasmEdge_CallingFrameContext.
/// \param Idx the index of memory instance in the module instance.
///
/// \returns the memory instance, NULL if not found.
WASMEDGE_CAPI_EXPORT extern WasmEdge_MemoryInstanceContext *
WasmEdge_CallingFrameGetMemoryInstance(const WasmEdge_CallingFrameContext *Cxt,
                                       const uint32_t Idx);

// <<<<<<<< WasmEdge calling frame functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge Async functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Wait a WasmEdge_Async execution.
///
/// \param Cxt the WasmEdge_ASync.
WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncWait(const WasmEdge_Async *Cxt);

/// Wait a WasmEdge_Async execution with timeout.
///
/// \param Cxt the WasmEdge_ASync.
/// \param Milliseconds times to wait.
///
/// \returns Result of waiting, true for execution ended, false for timeout
/// occurred.
WASMEDGE_CAPI_EXPORT bool WasmEdge_AsyncWaitFor(const WasmEdge_Async *Cxt,
                                                uint64_t Milliseconds);

/// Cancel a WasmEdge_Async execution.
///
/// \param Cxt the WasmEdge_ASync.
WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncCancel(WasmEdge_Async *Cxt);

/// Wait and get the return list length of the WasmEdge_Async execution.
///
/// This function will wait until the execution finished and return the return
/// value list length of the executed function. This function will return 0 if
/// the `Cxt` is NULL, the execution was failed, or the execution was canceled.
/// Developers can call the `WasmEdge_AsyncGet` to get the execution status and
/// the return values.
///
/// \param Cxt the WasmEdge_ASync.
///
/// \returns the return list length of the executed function.
WASMEDGE_CAPI_EXPORT uint32_t
WasmEdge_AsyncGetReturnsLength(const WasmEdge_Async *Cxt);

/// Wait and get the result of WasmEdge_Async execution.
///
/// This function will wait until the execution finished and return the
/// execution status and the return values.
/// If the `Returns` buffer length is smaller than the arity of the function,
/// the overflowed return values will be discarded.
///
/// \param Cxt the WasmEdge_ASync.
/// \param [out] Returns the WasmEdge_Value buffer to fill the return values.
/// \param ReturnLen the return buffer length.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT WasmEdge_Result
WasmEdge_AsyncGet(const WasmEdge_Async *Cxt, WasmEdge_Value *Returns,
                  const uint32_t ReturnLen);

/// Deletion of the WasmEdge_Async.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_ASync to destroy.
WASMEDGE_CAPI_EXPORT void WasmEdge_AsyncDelete(WasmEdge_Async *Cxt);

// <<<<<<<< WasmEdge Async functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_EXECUTION_H
