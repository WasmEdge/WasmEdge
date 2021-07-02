// SPDX-License-Identifier: Apache-2.0
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

#include "api/wasmedge.h"

#include "../spec/spectest.h"
#include "helper.h"
#include "hostfunc_c.h"

#include "gtest/gtest.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

/// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge_ConfigureContext *ConfCxt = createConf(Conf);
  WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
  WasmEdge_StatisticsContext *StatCxt = WasmEdge_StatisticsCreate();
  WasmEdge_LoaderContext *LoadCxt = WasmEdge_LoaderCreate(ConfCxt);
  WasmEdge_ValidatorContext *ValidCxt = WasmEdge_ValidatorCreate(ConfCxt);
  WasmEdge_InterpreterContext *InterpCxt =
      WasmEdge_InterpreterCreate(ConfCxt, StatCxt);
  WasmEdge_ConfigureDelete(ConfCxt);

  WasmEdge_ImportObjectContext *TestModCxt = createSpecTestModule();
  WasmEdge_InterpreterRegisterImport(InterpCxt, StoreCxt, TestModCxt);

  T.onModule = [&](const std::string &ModName,
                   const std::string &Filename) -> Expect<void> {
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ModCxt);
      return Unexpect(convResult(Res));
    }
    if (!ModName.empty()) {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      Res = WasmEdge_InterpreterRegisterModule(InterpCxt, StoreCxt, ModCxt,
                                               ModStr);
    } else {
      Res = WasmEdge_InterpreterInstantiate(InterpCxt, StoreCxt, ModCxt);
    }
    WasmEdge_ASTModuleDelete(ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
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
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ModCxt);
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_InterpreterInstantiate(InterpCxt, StoreCxt, ModCxt);
    WasmEdge_ASTModuleDelete(ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  /// Helper function to call functions.
  T.onInvoke = [&](const std::string &ModName, const std::string &Field,
                   const std::vector<ValVariant> &Params,
                   const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<ValVariant>> {
    WasmEdge_Result Res;
    std::vector<WasmEdge_Value> CParams = convFromValVec(Params, ParamTypes);
    std::vector<WasmEdge_Value> CReturns;
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    if (!ModName.empty()) {
      /// Invoke function of named module. Named modules are registered in
      /// Store Manager.
      /// Get the function type to specify the return nums.
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      WasmEdge_FunctionInstanceContext *FuncCxt =
          WasmEdge_StoreFindFunctionRegistered(StoreCxt, ModStr, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res = WasmEdge_InterpreterInvokeRegistered(
          InterpCxt, StoreCxt, ModStr, FieldStr, &CParams[0],
          static_cast<uint32_t>(CParams.size()), &CReturns[0],
          static_cast<uint32_t>(CReturns.size()));
    } else {
      /// Invoke function of anonymous module. Anonymous modules are
      /// instantiated in VM.
      /// Get function type to specify the return nums.
      WasmEdge_FunctionInstanceContext *FuncCxt =
          WasmEdge_StoreFindFunction(StoreCxt, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      const WasmEdge_FunctionTypeContext *FuncType =
          WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res = WasmEdge_InterpreterInvoke(
          InterpCxt, StoreCxt, FieldStr, &CParams[0],
          static_cast<uint32_t>(CParams.size()), &CReturns[0],
          static_cast<uint32_t>(CReturns.size()));
    }
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  /// Helper function to get values.
  T.onGet = [&](const std::string &ModName,
                const std::string &Field) -> Expect<std::vector<ValVariant>> {
    /// Get global instance.
    WasmEdge_String ModStr = WasmEdge_StringWrap(
        ModName.data(), static_cast<uint32_t>(ModName.length()));
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    WasmEdge_GlobalInstanceContext *GlobCxt =
        WasmEdge_StoreFindGlobalRegistered(StoreCxt, ModStr, FieldStr);
    if (GlobCxt == nullptr) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return std::vector<ValVariant>{
        WasmEdge_GlobalInstanceGetValue(GlobCxt).Value};
  };

  T.run(Proposal, UnitName);

  WasmEdge_LoaderDelete(LoadCxt);
  WasmEdge_ValidatorDelete(ValidCxt);
  WasmEdge_InterpreterDelete(InterpCxt);
  WasmEdge_StoreDelete(StoreCxt);
  WasmEdge_StatisticsDelete(StatCxt);
  WasmEdge_ImportObjectDelete(TestModCxt);
}

/// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
