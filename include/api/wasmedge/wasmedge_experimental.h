// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//===-- wasmedge/wasmedge_experimental.h - WasmEdge C API -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the experimental functions in WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_EXPERIMENTAL_H
#define WASMEDGE_C_API_EXPERIMENTAL_H

#include "wasmedge/wasmedge_basic.h"

#ifdef __cplusplus
extern "C" {
#endif

// >>>>>>>> WasmEdge Experimental functions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// Register a host function that will be invoked before executing any host
/// functions.
///
/// There is only one pre-host-function. After calling this function, the
/// previous registered host function will be replaced. This is a experimental
/// feature. Use it at your own risk.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_ExecutorContext.
/// \param Data the host data to set into the given host function. When calling
/// the Func, this pointer will be the argument of the Func function.
/// \param Func the function to be invoked before executing any other host
/// functions.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ExecutorExperimentalRegisterPreHostFunction(
    WasmEdge_ExecutorContext *Cxt, void *Data, void (*Func)(void *));

/// Register a host function that will be invoked after executing any host
/// functions.
///
/// There is only one post-host-function. After calling this function, the
/// previous registered host function will be replaced. This is a experimental
/// feature. Use it at your own risk.
///
/// This function is thread-safe.
///
/// \param Cxt the WasmEdge_VMContext.
/// \param Data the host data to set into the given host function. When calling
/// the Func, this pointer will be the argument of the Func function.
/// \param Func the function to be invoked after executing any other host
/// functions.
WASMEDGE_CAPI_EXPORT extern void
WasmEdge_ExecutorExperimentalRegisterPostHostFunction(
    WasmEdge_ExecutorContext *Cxt, void *Data, void (*Func)(void *));

// <<<<<<<< WasmEdge Experimental Functions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#ifdef __cplusplus
} /// extern "C"
#endif

#endif /// WASMEDGE_C_API_EXPERIMENTAL_H
