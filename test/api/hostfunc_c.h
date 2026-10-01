// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/api/hostfunc_c.h - Spec test host functions for C API ==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The "spectest" host module used by the C API spec tests.
///
//===----------------------------------------------------------------------===//

#ifndef HOSTFUNC_C_H
#define HOSTFUNC_C_H

#include "wasmedge/wasmedge.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function type: {} -> {}
WasmEdge_Result SpecTestPrint(void *Data,
                              const WasmEdge_CallingFrameContext *CallFrameCxt,
                              const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {i32} -> {}
WasmEdge_Result
SpecTestPrintI32(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
                 const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {i64} -> {}
WasmEdge_Result
SpecTestPrintI64(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
                 const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {f32} -> {}
WasmEdge_Result
SpecTestPrintF32(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
                 const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {f64} -> {}
WasmEdge_Result
SpecTestPrintF64(void *Data, const WasmEdge_CallingFrameContext *CallFrameCxt,
                 const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {i32, f32} -> {}
WasmEdge_Result
SpecTestPrintI32F32(void *Data,
                    const WasmEdge_CallingFrameContext *CallFrameCxt,
                    const WasmEdge_Value *In, WasmEdge_Value *Out);

// Function type: {f64, f64} -> {}
WasmEdge_Result
SpecTestPrintF64F64(void *Data,
                    const WasmEdge_CallingFrameContext *CallFrameCxt,
                    const WasmEdge_Value *In, WasmEdge_Value *Out);

WasmEdge_ModuleInstanceContext *createSpecTestModule(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // HOSTFUNC_C_H
