// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_compiler.h - WasmEdge C API ---------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about AOT compiler in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_COMPILER_H
#define WASMEDGE_C_API_COMPILER_H

#include "wasmedge/wasmedge_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge AOT compiler functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Creation of the WasmEdge_CompilerContext.
///
/// The caller owns the object and should call `WasmEdge_CompilerDelete` to
/// delete it.
///
/// \returns pointer to context, NULL if failed.
WASMEDGE_CAPI_EXPORT extern WasmEdge_CompilerContext *
WasmEdge_CompilerCreate(const WasmEdge_ConfigureContext *ConfCxt);

/// Compile the input WASM from the file path.
///
/// The compiler compiles the WASM from file path for the ahead-of-time mode and
/// store the result to the output file path.
///
/// \param Cxt the WasmEdge_CompilerContext.
/// \param InPath the input WASM file path.
/// \param OutPath the output WASM file path.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_CompilerCompile(WasmEdge_CompilerContext *Cxt, const char *InPath,
                         const char *OutPath);

/// Compile the input WASM from a WasmEdge_Bytes.
///
/// The compiler compiles the WASM from the WasmEdge_Bytes for the
/// ahead-of-time mode and store the result to the output file path.
///
/// \param Cxt the WasmEdge_CompilerContext.
/// \param Bytes the WasmEdge_Bytes of WASM binary.
/// \param OutPath the output WASM file path.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_CompilerCompileFromBytes(WasmEdge_CompilerContext *Cxt,
                                  const WasmEdge_Bytes Bytes,
                                  const char *OutPath);

/// Deletion of the WasmEdge_CompilerContext.
///
/// After calling this function, the context will be destroyed and should
/// __NOT__ be used.
///
/// \param Cxt the WasmEdge_CompilerContext to destroy.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_CompilerDelete(WasmEdge_CompilerContext *Cxt);

// <<<<<<<< WasmEdge AOT compiler functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> WasmEdge loader functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Load and parse the WASM module from a buffer into WasmEdge_ASTModuleContext.
///
/// CAUTION: This function will be deprecated and replaced by
/// `WasmEdge_LoaderParseFromBytes()` API in the future.
///
/// Load and parse the WASM module from a buffer, and return a
/// WasmEdge_ASTModuleContext as the result. The caller owns the
/// WasmEdge_ASTModuleContext object and should call `WasmEdge_ASTModuleDelete`
/// to destroy it.
///
/// \param Cxt the WasmEdge_LoaderContext.
/// \param [out] Module the output WasmEdge_ASTModuleContext if succeeded.
/// \param Buf the buffer of WASM binary.
/// \param BufLen the length of the buffer.
///
/// \returns WasmEdge_Result. Call `WasmEdge_ResultGetMessage` for the error
/// message.
WASMEDGE_CAPI_EXPORT extern WasmEdge_Result
WasmEdge_LoaderParseFromBuffer(WasmEdge_LoaderContext *Cxt,
                               WasmEdge_ASTModuleContext **Module,
                               const uint8_t *Buf, const uint32_t BufLen);

// <<<<<<<< WasmEdge loader functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_COMPILER_H
