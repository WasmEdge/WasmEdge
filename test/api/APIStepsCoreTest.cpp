// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/test/api/APIStepsCoreTest.cpp - WasmEdge C API core tests ===//
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

#include "../spec/spectest.h"
#include "helper.h"
#include "hostfunc_c.h"
#include "wasmedge/wasmedge.h"

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
  WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
  WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
  WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(ConfCxt);
  WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(ConfCxt);
  WasmEdge_ExecutorContext *ExecCxt = WasmEdge_ExecutorCreate(ConfCxt, StatCxt);
  WasmEdge_ModuleInstanceContext *ActiveModCxt = nullptr;
  std::vector<WasmEdge_ModuleInstanceContext *> InstantiatedMod;

  WasmEdge_ConfigureDelete(ConfCxt);

  WasmEdge_ModuleInstanceContext *TestModCxt = createSpecTestModule();
  WasmEdge_ExecutorRegisterImport(ExecCxt, StoreCxt, TestModCxt);

  T.onModule = [&](const std::string &ModName,
                   const std::string &Filename) -> Expect<void> {
    WasmEdge_ASTModuleContext *ASTModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ASTModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ASTModCxt);
      return Unexpect(convResult(Res));
    }
    WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    if (ModName.empty()) {
      Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, StoreCxt, ASTModCxt);
    } else {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      Res = WasmEdge_ExecutorRegister(ExecCxt, &ModCxt, StoreCxt, ASTModCxt,
                                      ModStr);
    }
    WasmEdge_ASTModuleDelete(ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    if (ModName.empty()) {
      ActiveModCxt = ModCxt;
    }
    InstantiatedMod.push_back(ModCxt);
    return {};
  };
  T.onLoad = [&](const std::string &Filename) -> Expect<void> {
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    WasmEdge_ASTModuleDelete(ModCxt);
    return {};
  };
  T.onValidate = [&](const std::string &Filename) -> Expect<void> {
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ModCxt);
    WasmEdge_ASTModuleDelete(ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onInstantiate = [&](const std::string &Filename) -> Expect<void> {
    WasmEdge_ASTModuleContext *ASTModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ASTModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ASTModCxt);
      return Unexpect(convResult(Res));
    }
    WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    Res = WasmEdge_ExecutorInstantiate(ExecCxt, &ModCxt, StoreCxt, ASTModCxt);
    WasmEdge_ASTModuleDelete(ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    ActiveModCxt = ModCxt;
    InstantiatedMod.push_back(ModCxt);
    return {};
  };
  // Helper function to call functions.
  T.onInvoke = [&](const std::string &ModName, const std::string &Field,
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
      const WasmEdge_ModuleInstanceContext *ModCxt =
          WasmEdge_StoreFindModule(StoreCxt, ModStr);
      WasmEdge_FunctionInstanceContext *FuncCxt =
          WasmEdge_ModuleInstanceFindFunction(ModCxt, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::Value::FuncNotFound);
      }
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      // Execute.
      Res = WasmEdge_ExecutorInvoke(
          ExecCxt, FuncCxt, &CParams[0], static_cast<uint32_t>(CParams.size()),
          &CReturns[0], static_cast<uint32_t>(CReturns.size()));
    } else {
      // Invoke function of current active module. Get function type to specify
      // the return nums.
      WasmEdge_FunctionInstanceContext *FuncCxt =
          WasmEdge_ModuleInstanceFindFunction(ActiveModCxt, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::Value::FuncNotFound);
      }
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      // Execute.
      Res = WasmEdge_ExecutorInvoke(
          ExecCxt, FuncCxt, &CParams[0], static_cast<uint32_t>(CParams.size()),
          &CReturns[0], static_cast<uint32_t>(CReturns.size()));
    }
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  // Helper function to get values.
  T.onGet =
      [&](const std::string &ModName,
          const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    // Get module instance.
    const WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    if (ModName.empty()) {
      ModCxt = ActiveModCxt;
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
    WasmEdge_Value Val = WasmEdge_GlobalInstanceGetValue(GlobCxt);
#if defined(__x86_64__) || defined(__aarch64__)
    return std::make_pair(ValVariant(Val.Value),
                          static_cast<ValType>(Val.Type));
#else
    return std::make_pair(
        ValVariant(WasmEdge::uint128_t(Val.Value.High, Val.Value.Low)),
        static_cast<ValType>(Val.Type));
#endif
  };

  T.run(Proposal, UnitName);

  WasmEdge_LoaderDelete(LoadCxt);
  WasmEdge_ValidatorDelete(ValidCxt);
  WasmEdge_ExecutorDelete(ExecCxt);
  WasmEdge_StoreDelete(StoreCxt);
  WasmEdge_StatisticsDelete(StatCxt);
  WasmEdge_ModuleInstanceDelete(TestModCxt);
  for (auto &&ModCxt : InstantiatedMod) {
    WasmEdge_ModuleInstanceDelete(ModCxt);
  }
  InstantiatedMod.clear();
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
