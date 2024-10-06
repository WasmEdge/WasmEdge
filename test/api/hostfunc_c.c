// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/api/hostfunc_c.c - Spec test host functions for C API ==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "hostfunc_c.h"
#include <stddef.h>

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#define MAYBE_UNUSED
#pragma warning(disable : 4100) // unreferenced formal parameter
#else
#define MAYBE_UNUSED __attribute__((unused))
#endif

WasmEdge_Result
SpecTestPrint(void *Data MAYBE_UNUSED,
              const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
              const WasmEdge_Value *In MAYBE_UNUSED,
              WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result
SpecTestPrintI32(void *Data MAYBE_UNUSED,
                 const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
                 const WasmEdge_Value *In MAYBE_UNUSED,
                 WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result
SpecTestPrintI64(void *Data MAYBE_UNUSED,
                 const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
                 const WasmEdge_Value *In MAYBE_UNUSED,
                 WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result
SpecTestPrintF32(void *Data MAYBE_UNUSED,
                 const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
                 const WasmEdge_Value *In MAYBE_UNUSED,
                 WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result
SpecTestPrintF64(void *Data MAYBE_UNUSED,
                 const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
                 const WasmEdge_Value *In MAYBE_UNUSED,
                 WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintI32F32(
    void *Data MAYBE_UNUSED,
    const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
    const WasmEdge_Value *In MAYBE_UNUSED, WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintF64F64(
    void *Data MAYBE_UNUSED,
    const WasmEdge_CallingFrameContext *CallFrameCxt MAYBE_UNUSED,
    const WasmEdge_Value *In MAYBE_UNUSED, WasmEdge_Value *Out MAYBE_UNUSED) {
  return WasmEdge_Result_Success;
}

WasmEdge_ModuleInstanceContext *createSpecTestModule(void) {
  WasmEdge_String HostName;
  WasmEdge_ModuleInstanceContext *HostMod = NULL;
  WasmEdge_FunctionTypeContext *HostFType = NULL;
  WasmEdge_TableTypeContext *HostTType = NULL;
  WasmEdge_MemoryTypeContext *HostMType = NULL;
  WasmEdge_GlobalTypeContext *HostGType = NULL;
  WasmEdge_FunctionInstanceContext *HostFunc = NULL;
  WasmEdge_TableInstanceContext *HostTable = NULL;
  WasmEdge_MemoryInstanceContext *HostMemory = NULL;
  WasmEdge_GlobalInstanceContext *HostGlobal = NULL;
  WasmEdge_Limit TabLimit;
  WasmEdge_Limit MemLimit;
  WasmEdge_Limit SharedMemLimit;
  WasmEdge_ValType Param[2];

  HostName = WasmEdge_StringCreateByCString("spectest");
  HostMod = WasmEdge_ModuleInstanceCreate(HostName);
  WasmEdge_StringDelete(HostName);

  // Add host function "print": {} -> {}
  HostFType = WasmEdge_FunctionTypeCreate(NULL, 0, NULL, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrint, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_i32": {i32} -> {}
  Param[0] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintI32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_i32");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_i64": {i64} -> {}
  Param[0] = WasmEdge_ValTypeGenI64();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintI64, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_i64");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_f32": {f32} -> {}
  Param[0] = WasmEdge_ValTypeGenF32();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintF32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f32");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_f64": {f64} -> {}
  Param[0] = WasmEdge_ValTypeGenF64();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintF64, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f64");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_i32_f32": {i32, f32} -> {}
  Param[0] = WasmEdge_ValTypeGenI32();
  Param[1] = WasmEdge_ValTypeGenF32();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintI32F32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_i32_f32");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "print_f64_f64": {f64, f64} -> {}
  Param[0] = WasmEdge_ValTypeGenF64();
  Param[1] = WasmEdge_ValTypeGenF64();
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, NULL, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, SpecTestPrintF64F64, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f64_f64");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host table "table"
  TabLimit.HasMax = true;
  TabLimit.Shared = false;
  TabLimit.Min = 10;
  TabLimit.Max = 20;
  HostTType = WasmEdge_TableTypeCreate(WasmEdge_ValTypeGenFuncRef(), TabLimit);
  HostTable = WasmEdge_TableInstanceCreate(HostTType);
  WasmEdge_TableTypeDelete(HostTType);
  HostName = WasmEdge_StringCreateByCString("table");
  WasmEdge_ModuleInstanceAddTable(HostMod, HostName, HostTable);
  WasmEdge_StringDelete(HostName);

  // Add host memory "memory"
  MemLimit.HasMax = true;
  MemLimit.Shared = false;
  MemLimit.Min = 1;
  MemLimit.Max = 2;
  HostMType = WasmEdge_MemoryTypeCreate(MemLimit);
  HostMemory = WasmEdge_MemoryInstanceCreate(HostMType);
  WasmEdge_MemoryTypeDelete(HostMType);
  HostName = WasmEdge_StringCreateByCString("memory");
  WasmEdge_ModuleInstanceAddMemory(HostMod, HostName, HostMemory);
  WasmEdge_StringDelete(HostName);

  // Add host memory "memory"
  SharedMemLimit.HasMax = true;
  SharedMemLimit.Shared = true;
  SharedMemLimit.Min = 1;
  SharedMemLimit.Max = 2;
  HostMType = WasmEdge_MemoryTypeCreate(SharedMemLimit);
  HostMemory = WasmEdge_MemoryInstanceCreate(HostMType);
  WasmEdge_MemoryTypeDelete(HostMType);
  HostName = WasmEdge_StringCreateByCString("shared_memory");
  WasmEdge_ModuleInstanceAddMemory(HostMod, HostName, HostMemory);
  WasmEdge_StringDelete(HostName);

  // Add host global "global_i32": const 666
  HostGType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenI32(),
                                        WasmEdge_Mutability_Const);
  HostGlobal =
      WasmEdge_GlobalInstanceCreate(HostGType, WasmEdge_ValueGenI32(666));
  WasmEdge_GlobalTypeDelete(HostGType);
  HostName = WasmEdge_StringCreateByCString("global_i32");
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  // Add host global "global_i64": const 666
  HostGType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenI64(),
                                        WasmEdge_Mutability_Const);
  HostGlobal =
      WasmEdge_GlobalInstanceCreate(HostGType, WasmEdge_ValueGenI64(666));
  WasmEdge_GlobalTypeDelete(HostGType);
  HostName = WasmEdge_StringCreateByCString("global_i64");
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  // Add host global "global_f32": const 666.0
  HostGType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenF32(),
                                        WasmEdge_Mutability_Const);
  HostGlobal = WasmEdge_GlobalInstanceCreate(
      HostGType, WasmEdge_ValueGenF32((float)666.6));
  WasmEdge_GlobalTypeDelete(HostGType);
  HostName = WasmEdge_StringCreateByCString("global_f32");
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  // Add host global "global_f64": const 666.0
  HostGType = WasmEdge_GlobalTypeCreate(WasmEdge_ValTypeGenF64(),
                                        WasmEdge_Mutability_Const);
  HostGlobal = WasmEdge_GlobalInstanceCreate(
      HostGType, WasmEdge_ValueGenF64((double)666.6));
  WasmEdge_GlobalTypeDelete(HostGType);
  HostName = WasmEdge_StringCreateByCString("global_f64");
  WasmEdge_ModuleInstanceAddGlobal(HostMod, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  return HostMod;
}
