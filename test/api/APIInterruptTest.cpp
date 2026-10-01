// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/api/APIInterruptTest.cpp - Interrupt ----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of interrupting the asynchronous executions
/// through the WasmEdge C API.
///
//===----------------------------------------------------------------------===//

#include "wasmedge/wasmedge.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace {

std::array<uint8_t, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

TEST(AsyncRunWsmFile, InterruptTest) {
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(nullptr, nullptr);
  {
    WasmEdge_Async *AsyncCxt = WasmEdge_VMAsyncRunWasmFromBuffer(
        VM, AsyncWasm.data(), static_cast<uint32_t>(AsyncWasm.size()),
        WasmEdge_StringWrap("_start", 6), nullptr, 0);
    EXPECT_NE(AsyncCxt, nullptr);
    EXPECT_FALSE(WasmEdge_AsyncWaitFor(AsyncCxt, 1));
    WasmEdge_AsyncCancel(AsyncCxt);
    WasmEdge_Result Res = WasmEdge_AsyncGet(AsyncCxt, nullptr, 0);
    EXPECT_FALSE(WasmEdge_ResultOK(Res));
    EXPECT_EQ(WasmEdge_ResultGetCode(Res), WasmEdge_ErrCode_Interrupted);
    WasmEdge_AsyncDelete(AsyncCxt);
  }
  WasmEdge_VMDelete(VM);
}

TEST(AsyncExecute, InterruptTest) {
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(nullptr, nullptr);
  ASSERT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromBuffer(
      VM, AsyncWasm.data(), static_cast<uint32_t>(AsyncWasm.size()))));
  ASSERT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VM)));
  ASSERT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VM)));
  {
    WasmEdge_Async *AsyncCxt = WasmEdge_VMAsyncExecute(
        VM, WasmEdge_StringWrap("_start", 6), nullptr, 0);
    EXPECT_NE(AsyncCxt, nullptr);
    EXPECT_FALSE(WasmEdge_AsyncWaitFor(AsyncCxt, 1));
    WasmEdge_AsyncCancel(AsyncCxt);
    WasmEdge_Result Res = WasmEdge_AsyncGet(AsyncCxt, nullptr, 0);
    EXPECT_FALSE(WasmEdge_ResultOK(Res));
    EXPECT_EQ(WasmEdge_ResultGetCode(Res), WasmEdge_ErrCode_Interrupted);
    WasmEdge_AsyncDelete(AsyncCxt);
  }
  WasmEdge_VMDelete(VM);
}

TEST(AsyncInvoke, InterruptTest) {
  WasmEdge_LoaderContext *Loader = WasmEdge_LoaderCreate(nullptr);
  WasmEdge_ValidatorContext *Validator = WasmEdge_ValidatorCreate(nullptr);
  WasmEdge_ExecutorContext *Executor =
      WasmEdge_ExecutorCreate(nullptr, nullptr);
  WasmEdge_StoreContext *Store = WasmEdge_StoreCreate();

  ASSERT_NE(Loader, nullptr);
  ASSERT_NE(Validator, nullptr);
  ASSERT_NE(Executor, nullptr);
  ASSERT_NE(Store, nullptr);

  WasmEdge_ASTModuleContext *AST = nullptr;
  ASSERT_TRUE(WasmEdge_ResultOK(
      WasmEdge_LoaderParseFromBuffer(Loader, &AST, AsyncWasm.data(),
                                     static_cast<uint32_t>(AsyncWasm.size()))));
  ASSERT_NE(AST, nullptr);
  ASSERT_TRUE(WasmEdge_ResultOK(WasmEdge_ValidatorValidate(Validator, AST)));
  WasmEdge_ModuleInstanceContext *Module = nullptr;
  ASSERT_TRUE(WasmEdge_ResultOK(
      WasmEdge_ExecutorInstantiate(Executor, &Module, Store, AST)));
  WasmEdge_ASTModuleDelete(AST);
  ASSERT_NE(Module, nullptr);
  WasmEdge_FunctionInstanceContext *FuncInst =
      WasmEdge_ModuleInstanceFindFunction(Module,
                                          WasmEdge_StringWrap("_start", 6));
  ASSERT_NE(FuncInst, nullptr);
  {
    WasmEdge_Async *AsyncCxt =
        WasmEdge_ExecutorAsyncInvoke(Executor, FuncInst, nullptr, 0);
    EXPECT_NE(AsyncCxt, nullptr);
    EXPECT_FALSE(WasmEdge_AsyncWaitFor(AsyncCxt, 1));
    WasmEdge_AsyncCancel(AsyncCxt);
    WasmEdge_Result Res = WasmEdge_AsyncGet(AsyncCxt, nullptr, 0);
    EXPECT_FALSE(WasmEdge_ResultOK(Res));
    EXPECT_EQ(WasmEdge_ResultGetCode(Res), WasmEdge_ErrCode_Interrupted);
    WasmEdge_AsyncDelete(AsyncCxt);
  }
  WasmEdge_LoaderDelete(Loader);
  WasmEdge_ValidatorDelete(Validator);
  WasmEdge_ExecutorDelete(Executor);
  WasmEdge_StoreDelete(Store);
  WasmEdge_ModuleInstanceDelete(Module);
}

} // namespace
