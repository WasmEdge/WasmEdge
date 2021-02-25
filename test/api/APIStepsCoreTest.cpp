// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/api/APIStepsCoreTest.cpp - SSVM C API core tests --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "api/ssvm.h"

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
using namespace SSVM;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

/// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  SSVM_ConfigureContext *ConfCxt = createConf(Conf);
  SSVM_StoreContext *StoreCxt = SSVM_StoreCreate();
  SSVM_StatisticsContext *StatCxt = SSVM_StatisticsCreate();
  SSVM_LoaderContext *LoadCxt = SSVM_LoaderCreate(ConfCxt);
  SSVM_ValidatorContext *ValidCxt = SSVM_ValidatorCreate(ConfCxt);
  SSVM_InterpreterContext *InterpCxt = SSVM_InterpreterCreate(ConfCxt, StatCxt);
  SSVM_ConfigureDelete(ConfCxt);

  SSVM_ImportObjectContext *TestModCxt = createSpecTestModule();
  SSVM_InterpreterRegisterImport(InterpCxt, StoreCxt, TestModCxt);

  T.onModule = [&](const std::string &ModName,
                   const std::string &Filename) -> Expect<void> {
    SSVM_ASTModuleContext *ModCxt = nullptr;
    SSVM_Result Res =
        SSVM_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = SSVM_ValidatorValidate(ValidCxt, ModCxt);
    if (!SSVM_ResultOK(Res)) {
      SSVM_ASTModuleDelete(ModCxt);
      return Unexpect(convResult(Res));
    }
    if (!ModName.empty()) {
      SSVM_String ModStr = SSVM_StringWrap(ModName.data(), ModName.length());
      Res = SSVM_InterpreterRegisterModule(InterpCxt, StoreCxt, ModCxt, ModStr);
    } else {
      Res = SSVM_InterpreterInstantiate(InterpCxt, StoreCxt, ModCxt);
    }
    SSVM_ASTModuleDelete(ModCxt);
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onValidate = [&](const std::string &Filename) -> Expect<void> {
    SSVM_ASTModuleContext *ModCxt = nullptr;
    SSVM_Result Res =
        SSVM_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = SSVM_ValidatorValidate(ValidCxt, ModCxt);
    SSVM_ASTModuleDelete(ModCxt);
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onInstantiate = [&](const std::string &Filename) -> Expect<void> {
    SSVM_ASTModuleContext *ModCxt = nullptr;
    SSVM_Result Res =
        SSVM_LoaderParseFromFile(LoadCxt, &ModCxt, Filename.c_str());
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = SSVM_ValidatorValidate(ValidCxt, ModCxt);
    if (!SSVM_ResultOK(Res)) {
      SSVM_ASTModuleDelete(ModCxt);
      return Unexpect(convResult(Res));
    }
    Res = SSVM_InterpreterInstantiate(InterpCxt, StoreCxt, ModCxt);
    SSVM_ASTModuleDelete(ModCxt);
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  /// Helper function to call functions.
  T.onInvoke = [&](const std::string &ModName, const std::string &Field,
                   const std::vector<ValVariant> &Params,
                   const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<ValVariant>> {
    SSVM_Result Res;
    std::vector<SSVM_Value> CParams = convFromValVec(Params, ParamTypes);
    std::vector<SSVM_Value> CReturns;
    SSVM_String FieldStr = SSVM_StringWrap(Field.data(), Field.length());
    if (!ModName.empty()) {
      /// Invoke function of named module. Named modules are registered in
      /// Store Manager.
      /// Get the function type to specify the return nums.
      SSVM_String ModStr = SSVM_StringWrap(ModName.data(), ModName.length());
      SSVM_FunctionInstanceContext *FuncCxt =
          SSVM_StoreFindFunctionRegistered(StoreCxt, ModStr, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      const SSVM_FunctionTypeContext *FuncType =
          SSVM_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(SSVM_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res = SSVM_InterpreterInvokeRegistered(
          InterpCxt, StoreCxt, ModStr, FieldStr, &CParams[0], CParams.size(),
          &CReturns[0], CReturns.size());
    } else {
      /// Invoke function of anonymous module. Anonymous modules are
      /// instantiated in VM.
      /// Get function type to specify the return nums.
      SSVM_FunctionInstanceContext *FuncCxt =
          SSVM_StoreFindFunction(StoreCxt, FieldStr);
      if (FuncCxt == nullptr) {
        return Unexpect(ErrCode::FuncNotFound);
      }
      const SSVM_FunctionTypeContext *FuncType =
          SSVM_FunctionInstanceGetFunctionType(FuncCxt);
      CReturns.resize(SSVM_FunctionTypeGetReturnsLength(FuncType));
      /// Execute.
      Res =
          SSVM_InterpreterInvoke(InterpCxt, StoreCxt, FieldStr, &CParams[0],
                                 CParams.size(), &CReturns[0], CReturns.size());
    }
    if (!SSVM_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  /// Helper function to get values.
  T.onGet = [&](const std::string &ModName,
                const std::string &Field) -> Expect<std::vector<ValVariant>> {
    /// Get global instance.
    SSVM_String ModStr = SSVM_StringWrap(ModName.data(), ModName.length());
    SSVM_String FieldStr = SSVM_StringWrap(Field.data(), Field.length());
    SSVM_GlobalInstanceContext *GlobCxt =
        SSVM_StoreFindGlobalRegistered(StoreCxt, ModStr, FieldStr);
    if (GlobCxt == nullptr) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return std::vector<ValVariant>{SSVM_GlobalInstanceGetValue(GlobCxt).Value};
  };

  T.run(Proposal, UnitName);

  SSVM_LoaderDelete(LoadCxt);
  SSVM_ValidatorDelete(ValidCxt);
  SSVM_InterpreterDelete(InterpCxt);
  SSVM_StoreDelete(StoreCxt);
  SSVM_StatisticsDelete(StatCxt);
  SSVM_ImportObjectDelete(TestModCxt);
}

/// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  SSVM_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
