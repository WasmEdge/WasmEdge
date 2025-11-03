// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_tools.h - WasmEdge C API ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the functions about driving WasmEdge tools in WasmEdge C
/// API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_TOOLS_H
#define WASMEDGE_C_API_TOOLS_H

#include "wasmedge/wasmedge_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge Driver functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
/// Convert UTF16 Args to UTF8 Args
///
/// This function is an argument converter for Windows platforms.
/// The caller owns the vector and should call `WasmEdge_Driver_ArgvDelete` to
/// destroy it.
///
/// \param Argc the argument count.
/// \param Argv the argument vector.
///
/// \returns Allocated argument vector.
WASMEDGE_CAPI_EXPORT extern const char **
WasmEdge_Driver_ArgvCreate(int Argc, const wchar_t *Argv[]);

/// Deletion of the argument vector
///
/// \param Argv the argument vector.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_Driver_ArgvDelete(const char *Argv[]);

/// Set console output code page to UTF-8 on windows.
WASMEDGE_CAPI_EXPORT extern void WasmEdge_Driver_SetConsoleOutputCPtoUTF8(void);
#endif

/// Entrypoint for the compiler tool.
///
/// This function provides an entrypoint to the WasmEdge AOT compiler tool with
/// the command line arguments.
///
/// \param Argc the argument count.
/// \param Argv the argument vector.
///
/// \returns the execution status.
WASMEDGE_CAPI_EXPORT extern int WasmEdge_Driver_Compiler(int Argc,
                                                         const char *Argv[]);

/// Entrypoint for the runtime tool.
///
/// This function provides an entrypoint to the WasmEdge runtime tool with the
/// command line arguments.
///
/// \param Argc the argument count.
/// \param Argv the argument vector.
///
/// \returns the execution status.
WASMEDGE_CAPI_EXPORT extern int WasmEdge_Driver_Tool(int Argc,
                                                     const char *Argv[]);

#ifdef WASMEDGE_BUILD_WASI_NN_RPC
/// Entrypoint for the WASI-NN RPC server tool.
///
/// This function provides an entrypoint to the WasmEdge WASI-NN RPC server tool
/// with the command line arguments.
///
/// \param Argc the argument count.
/// \param Argv the argument vector.
///
/// \returns the execution status.
WASMEDGE_CAPI_EXPORT extern int
WasmEdge_Driver_WasiNNRPCServer(int Argc, const char *Argv[]);
#endif

/// Entrypoint for the unified tool.
///
/// This function provides an entrypoint to the WasmEdge unified tool with the
/// command line arguments.
///
/// \param Argc the argument count.
/// \param Argv the argument vector.
///
/// \returns the execution status.
WASMEDGE_CAPI_EXPORT extern int WasmEdge_Driver_UniTool(int Argc,
                                                        const char *Argv[]);

// <<<<<<<< WasmEdge Driver functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_TOOLS_H
