// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_context.h - WasmEdge C API ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of contexts in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_CONTEXT_H
#define WASMEDGE_C_API_CONTEXT_H

/// Opaque struct of WasmEdge configure.
typedef struct WasmEdge_ConfigureContext WasmEdge_ConfigureContext;

/// Opaque struct of WasmEdge statistics.
typedef struct WasmEdge_StatisticsContext WasmEdge_StatisticsContext;

/// Opaque struct of WasmEdge AST module.
typedef struct WasmEdge_ASTModuleContext WasmEdge_ASTModuleContext;

/// Opaque struct of WasmEdge limit type.
typedef struct WasmEdge_LimitContext WasmEdge_LimitContext;

/// Opaque struct of WasmEdge function type.
typedef struct WasmEdge_FunctionTypeContext WasmEdge_FunctionTypeContext;

/// Opaque struct of WasmEdge memory type.
typedef struct WasmEdge_MemoryTypeContext WasmEdge_MemoryTypeContext;

/// Opaque struct of WasmEdge table type.
typedef struct WasmEdge_TableTypeContext WasmEdge_TableTypeContext;

/// Opaque struct of WasmEdge tag type.
typedef struct WasmEdge_TagTypeContext WasmEdge_TagTypeContext;

/// Opaque struct of WasmEdge global type.
typedef struct WasmEdge_GlobalTypeContext WasmEdge_GlobalTypeContext;

/// Opaque struct of WasmEdge import type.
typedef struct WasmEdge_ImportTypeContext WasmEdge_ImportTypeContext;

/// Opaque struct of WasmEdge export type.
typedef struct WasmEdge_ExportTypeContext WasmEdge_ExportTypeContext;

/// Opaque struct of WasmEdge AOT compiler.
typedef struct WasmEdge_CompilerContext WasmEdge_CompilerContext;

/// Opaque struct of WasmEdge loader.
typedef struct WasmEdge_LoaderContext WasmEdge_LoaderContext;

/// Opaque struct of WasmEdge validator.
typedef struct WasmEdge_ValidatorContext WasmEdge_ValidatorContext;

/// Opaque struct of WasmEdge executor.
typedef struct WasmEdge_ExecutorContext WasmEdge_ExecutorContext;

/// Opaque struct of WasmEdge store.
typedef struct WasmEdge_StoreContext WasmEdge_StoreContext;

/// Opaque struct of WasmEdge module instance.
typedef struct WasmEdge_ModuleInstanceContext WasmEdge_ModuleInstanceContext;

/// Opaque struct of WasmEdge function instance.
typedef struct WasmEdge_FunctionInstanceContext
    WasmEdge_FunctionInstanceContext;

/// Opaque struct of WasmEdge table instance.
typedef struct WasmEdge_TableInstanceContext WasmEdge_TableInstanceContext;

/// Opaque struct of WasmEdge memory instance.
typedef struct WasmEdge_MemoryInstanceContext WasmEdge_MemoryInstanceContext;

/// Opaque struct of WasmEdge tag instance.
typedef struct WasmEdge_TagInstanceContext WasmEdge_TagInstanceContext;

/// Opaque struct of WasmEdge global instance.
typedef struct WasmEdge_GlobalInstanceContext WasmEdge_GlobalInstanceContext;

/// Opaque struct of WasmEdge calling frame.
typedef struct WasmEdge_CallingFrameContext WasmEdge_CallingFrameContext;

/// Opaque struct of WasmEdge asynchronous result.
typedef struct WasmEdge_Async WasmEdge_Async;

/// Opaque struct of WasmEdge VM.
typedef struct WasmEdge_VMContext WasmEdge_VMContext;

/// Opaque struct of WasmEdge Plugin.
typedef struct WasmEdge_PluginContext WasmEdge_PluginContext;

#endif /// WASMEDGE_C_API_CONTEXT_H
