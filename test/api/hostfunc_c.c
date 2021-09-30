// SPDX-License-Identifier: Apache-2.0
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

WasmEdge_Result SpecTestPrint(void *Data __attribute__((unused)),
                              WasmEdge_MemoryInstanceContext *MemCxt
                              __attribute__((unused)),
                              const WasmEdge_Value *In __attribute__((unused)),
                              WasmEdge_Value *Out __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintI32(void *Data __attribute__((unused)),
                                 WasmEdge_MemoryInstanceContext *MemCxt
                                 __attribute__((unused)),
                                 const WasmEdge_Value *In
                                 __attribute__((unused)),
                                 WasmEdge_Value *Out __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintF32(void *Data __attribute__((unused)),
                                 WasmEdge_MemoryInstanceContext *MemCxt
                                 __attribute__((unused)),
                                 const WasmEdge_Value *In
                                 __attribute__((unused)),
                                 WasmEdge_Value *Out __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintF64(void *Data __attribute__((unused)),
                                 WasmEdge_MemoryInstanceContext *MemCxt
                                 __attribute__((unused)),
                                 const WasmEdge_Value *In
                                 __attribute__((unused)),
                                 WasmEdge_Value *Out __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintI32F32(void *Data __attribute__((unused)),
                                    WasmEdge_MemoryInstanceContext *MemCxt
                                    __attribute__((unused)),
                                    const WasmEdge_Value *In
                                    __attribute__((unused)),
                                    WasmEdge_Value *Out
                                    __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_Result SpecTestPrintF64F64(void *Data __attribute__((unused)),
                                    WasmEdge_MemoryInstanceContext *MemCxt
                                    __attribute__((unused)),
                                    const WasmEdge_Value *In
                                    __attribute__((unused)),
                                    WasmEdge_Value *Out
                                    __attribute__((unused))) {
  return WasmEdge_Result_Success;
}

WasmEdge_ImportObjectContext *createSpecTestModule(void) {
  WasmEdge_String HostName;
  WasmEdge_FunctionTypeContext *HostFType = NULL;
  WasmEdge_HostFunctionContext *HostFunc = NULL;
  WasmEdge_TableInstanceContext *HostTable = NULL;
  WasmEdge_MemoryInstanceContext *HostMemory = NULL;
  WasmEdge_GlobalInstanceContext *HostGlobal = NULL;
  enum WasmEdge_ValType Param[2];

  HostName = WasmEdge_StringCreateByCString("spectest");
  WasmEdge_ImportObjectContext *ImpObj = WasmEdge_ImportObjectCreate(HostName);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print": {} -> {}
  HostFType = WasmEdge_FunctionTypeCreate(NULL, 0, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrint, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print_i32": {i32} -> {}
  Param[0] = WasmEdge_ValType_I32;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrintI32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_i32");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print_f32": {f32} -> {}
  Param[0] = WasmEdge_ValType_F32;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrintF32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f32");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print_f64": {f64} -> {}
  Param[0] = WasmEdge_ValType_F64;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 1, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrintF64, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f64");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print_i32_f32": {i32, f32} -> {}
  Param[0] = WasmEdge_ValType_I32;
  Param[1] = WasmEdge_ValType_F32;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrintI32F32, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_i32_f32");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host function "print_f64_f64": {f64, f64} -> {}
  Param[0] = WasmEdge_ValType_F64;
  Param[1] = WasmEdge_ValType_F64;
  HostFType = WasmEdge_FunctionTypeCreate(Param, 2, NULL, 0);
  HostFunc = WasmEdge_HostFunctionCreate(HostFType, SpecTestPrintF64F64, NULL, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("print_f64_f64");
  WasmEdge_ImportObjectAddHostFunction(ImpObj, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  /// Add host table "table"
  WasmEdge_Limit TabLimit = {.HasMax = true, .Min = 10, .Max = 20};
  HostTable = WasmEdge_TableInstanceCreate(WasmEdge_RefType_FuncRef, TabLimit);
  HostName = WasmEdge_StringCreateByCString("table");
  WasmEdge_ImportObjectAddTable(ImpObj, HostName, HostTable);
  WasmEdge_StringDelete(HostName);

  /// Add host memory "memory"
  WasmEdge_Limit MemLimit = {.HasMax = true, .Min = 1, .Max = 2};
  HostMemory = WasmEdge_MemoryInstanceCreate(MemLimit);
  HostName = WasmEdge_StringCreateByCString("memory");
  WasmEdge_ImportObjectAddMemory(ImpObj, HostName, HostMemory);
  WasmEdge_StringDelete(HostName);

  /// Add host global "global_i32": const 666
  HostGlobal = WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenI32(666),
                                             WasmEdge_Mutability_Const);
  HostName = WasmEdge_StringCreateByCString("global_i32");
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  /// Add host global "global_i64": const 666
  HostGlobal = WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenI64(666),
                                             WasmEdge_Mutability_Const);
  HostName = WasmEdge_StringCreateByCString("global_i64");
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  /// Add host global "global_f32": const 666.0
  HostGlobal = WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenF32(666.0),
                                             WasmEdge_Mutability_Const);
  HostName = WasmEdge_StringCreateByCString("global_f32");
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  /// Add host global "global_f64": const 666.0
  HostGlobal = WasmEdge_GlobalInstanceCreate(WasmEdge_ValueGenF64(666.0),
                                             WasmEdge_Mutability_Const);
  HostName = WasmEdge_StringCreateByCString("global_f64");
  WasmEdge_ImportObjectAddGlobal(ImpObj, HostName, HostGlobal);
  WasmEdge_StringDelete(HostName);

  return ImpObj;
}
