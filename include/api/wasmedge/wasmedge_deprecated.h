// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_deprecated.h - WasmEdge C API -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the deprecated functions in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_DEPRECATED_H
#define WASMEDGE_C_API_DEPRECATED_H

#include "wasmedge/wasmedge_basic.h"
#include "wasmedge/wasmedge_value.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge AOT compiler functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Compile the input WASM from the given buffer.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_CompilerCompileFromBytes()` API in the future.
///
/// The compiler compiles the WASM from the given buffer for the
/// ahead-of-time mode and store the result to the output file path.
///
/// \param Cxt the WasmEdge_CompilerContext.
/// \param InBuffer the input WASM binary buffer.
/// \param InBufferLen the length of the input WASM binary buffer.
/// \param OutPath the output WASM file path.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result WasmEdge_CompilerCompileFromBuffer(
    WasmEdge_CompilerContext *Cxt, const uint8_t *InBuffer,
    const uint64_t InBufferLen, const char *OutPath);

// <<<<<<<< WasmEdge AOT compiler functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge VM functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Register and instantiate WASM into the store in VM from a buffer.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_VMRegisterModuleFromBytes()` API in the future.
///
/// Load a WASM module from a buffer, and register all exported instances and
/// instantiate them into the store into the VM with their exported name and
/// module name.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_VMContext which contains the store.
/// \param ModuleName the WasmEdge_String of module name for all exported
/// instances.
/// \param Buf the buffer of WASM binary.
/// \param BufLen the length of the buffer.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_VMRegisterModuleFromBuffer(WasmEdge_VMContext *Cxt,
                                    const WasmEdge_String ModuleName,
                                    const uint8_t *Buf, const uint32_t BufLen);

/// Instantiate the WASM module from a buffer and invoke a function by name.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_VMRunWasmFromBytes()` API in the future.
///
/// This is the function to invoke a WASM function rapidly.
/// Load and instantiate the WASM module from a buffer, and then invoke a
/// function by name and parameters. If the `Returns` buffer length is smaller
/// than the arity of the function, the overflowed return values will be
/// discarded.
/// After calling this function, a new anonymous module instance owned by VM is
/// instantiated, and the old one will be destroyed.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_VMContext.
/// \param Buf the buffer of WASM binary.
/// \param BufLen the length of the buffer.
/// \param FuncName the function name WasmEdge_String.
/// \param Params the WasmEdge_Value buffer with the parameter values.
/// \param ParamLen the parameter buffer length.
/// \param [out] Returns the WasmEdge_Value buffer to fill the return values.
/// \param ReturnLen the return buffer length.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result WasmEdge_VMRunWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen, WasmEdge_Value *Returns, const uint32_t ReturnLen);

/// Instantiate the WASM module from a buffer and asynchronous invoke a function
/// by name.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_VMAsyncRunWasmFromBytes()` API in the future.
///
/// This is the function to invoke a WASM function rapidly.
/// Load and instantiate the WASM module from a buffer, and then invoke a
/// function by name and parameters. If the `Returns` buffer length is smaller
/// than the arity of the function, the overflowed return values will be
/// discarded.
/// After calling this function, a new anonymous module instance owned by VM is
/// instantiated, and the old one will be destroyed.
///
/// The caller owns the object and should call `WasmEdge_AsyncDelete` to destroy
/// it.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_VMContext.
/// \param Buf the buffer of WASM binary.
/// \param BufLen the length of the buffer.
/// \param FuncName the function name WasmEdge_String.
/// \param Params the WasmEdge_Value buffer with the parameter values.
/// \param ParamLen the parameter buffer length.
///
/// \returns WasmEdge_Async. Call `WasmEdge_AsyncGet` for the result, and call
/// `WasmEdge_AsyncDelete` to destroy this object.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Async *WasmEdge_VMAsyncRunWasmFromBuffer(
    WasmEdge_VMContext *Cxt, const uint8_t *Buf, const uint32_t BufLen,
    const WasmEdge_String FuncName, const WasmEdge_Value *Params,
    const uint32_t ParamLen);

/// Load the WASM module from a buffer.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_VMLoadWasmFromBytes()` API in the future.
///
/// This is the first step to invoke a WASM function step by step.
/// Load and parse the WASM module from a buffer. You can then call
/// `WasmEdge_VMValidate` for the next step.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_VMContext.
/// \param Buf the buffer of WASM binary.
/// \param BufLen the length of the buffer.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_VMLoadWasmFromBuffer(WasmEdge_VMContext *Cxt, const uint8_t *Buf,
                              const uint32_t BufLen);

// <<<<<<<< WasmEdge VM functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_DEPRECATED_H
