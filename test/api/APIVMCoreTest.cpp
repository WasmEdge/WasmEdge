// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/api/APIVMCoreTest.cpp - WasmEdge C API core tests ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "wasmedge/wasmedge.h"

#include "../spec/spectest.h"
#include "helper.h"
#include "hostfunc_c.h"

#include <array>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge_ConfigureContext *ConfCxt = createConf(Conf);
  WasmEdge_VMContext *VM = WasmEdge_VMCreate(ConfCxt, nullptr);
  WasmEdge_ConfigureDelete(ConfCxt);
  WasmEdge_ModuleInstanceContext *TestModCxt = createSpecTestModule();
  WasmEdge_VMRegisterModuleFromImport(VM, TestModCxt);

  T.onModule = [&VM](const std::string &ModName,
                     const std::string &Filename) -> Expect<void> {
    WasmEdge_Result Res;
    if (!ModName.empty()) {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      Res = WasmEdge_VMRegisterModuleFromFile(VM, ModStr, Filename.c_str());
    } else {
      Res = WasmEdge_VMLoadWasmFromFile(VM, Filename.c_str());
      if (!WasmEdge_ResultOK(Res)) {
        return Unexpect(convResult(Res));
      }
      Res = WasmEdge_VMValidate(VM);
      if (!WasmEdge_ResultOK(Res)) {
        return Unexpect(convResult(Res));
      }
      Res = WasmEdge_VMInstantiate(VM);
    }
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onValidate = [&VM](const std::string &Filename) -> Expect<void> {
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_VMValidate(VM);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onInstantiate = [&VM](const std::string &Filename) -> Expect<void> {
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_VMValidate(VM);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_VMInstantiate(VM);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  // Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    WasmEdge_Result Res;
    std::vector<WasmEdge_Value> CParams = convFromValVec(Params, ParamTypes);
    std::vector<WasmEdge_Value> CReturns;
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager. Get the function type to specify the return nums.
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_VMGetFunctionTypeRegistered(VM, ModStr, FieldStr);
      if (FuncType == nullptr) {
        return Unexpect(ErrCode::Value::FuncNotFound);
      }
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      // Execute.
      Res = WasmEdge_VMExecuteRegistered(
          VM, ModStr, FieldStr, CParams.data(),
          static_cast<uint32_t>(CParams.size()), CReturns.data(),
          static_cast<uint32_t>(CReturns.size()));
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM. Get function type to specify the return nums.
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_VMGetFunctionType(VM, FieldStr);
      if (FuncType == nullptr) {
        return Unexpect(ErrCode::Value::FuncNotFound);
      }
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      // Execute.
      Res = WasmEdge_VMExecute(
          VM, FieldStr, CParams.data(), static_cast<uint32_t>(CParams.size()),
          CReturns.data(), static_cast<uint32_t>(CReturns.size()));
    }
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  // Helper function to get values.
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
    // Get module instance.
    const WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VM);
    if (ModName.empty()) {
      ModCxt = WasmEdge_VMGetActiveModule(VM);
    } else {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      ModCxt = WasmEdge_StoreFindModule(StoreCxt, ModStr);
    }

    // Get global instance.
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    WasmEdge_GlobalInstanceContext *GlobCxt =
        WasmEdge_ModuleInstanceFindGlobal(ModCxt, FieldStr);
    if (GlobCxt == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return convToVal(WasmEdge_GlobalInstanceGetValue(GlobCxt));
  };

  T.run(Proposal, UnitName);

  WasmEdge_VMDelete(VM);
  WasmEdge_ModuleInstanceDelete(TestModCxt);
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
