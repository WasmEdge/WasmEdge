// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include "helper.h"
#include "hostfunc_c.h"
#include "wasmedge/wasmedge.h"

#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
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
  const auto &ConfRef = Conf;

  // Define context structure for C API steps
  struct TestContext {
    WasmEdge_StoreContext *StoreCxt;
    WasmEdge_StatisticsContext *StatCxt;
    WasmEdge_LoaderContext *LoadCxt;
    WasmEdge_ValidatorContext *ValidCxt;
    WasmEdge_ExecutorContext *ExecCxt;
    WasmEdge_ModuleInstanceContext *TestModCxt;
    WasmEdge_ModuleInstanceContext *ActiveModCxt;
    std::vector<WasmEdge_ModuleInstanceContext *> InstantiatedMod;

    TestContext(const WasmEdge::Configure &C) : ActiveModCxt(nullptr) {
      WasmEdge_ConfigureContext *ConfCxt = createConf(C);
      StoreCxt = WasmEdge_StoreCreate();
      StatCxt = WasmEdge_StatisticsCreate();
      LoadCxt = WasmEdge_LoaderCreate(ConfCxt);
      ValidCxt = WasmEdge_ValidatorCreate(ConfCxt);
      ExecCxt = WasmEdge_ExecutorCreate(ConfCxt, StatCxt);
      WasmEdge_ConfigureDelete(ConfCxt);
      TestModCxt = createSpecTestModule();
      WasmEdge_ExecutorRegisterImport(ExecCxt, StoreCxt, TestModCxt);
    }
    ~TestContext() {
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
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        WasmEdge_String ParentNameStr =
            WasmEdge_StringCreateByCString(ParentName.c_str());
        const WasmEdge_ModuleInstanceContext *ModInst =
            WasmEdge_StoreFindModule(P->StoreCxt, ParentNameStr);
        WasmEdge_StringDelete(ParentNameStr);
        if (ModInst != nullptr) {
          WasmEdge_String AliasNameStr =
              WasmEdge_StringCreateByCString(AliasName.c_str());
          WasmEdge_ExecutorRegisterImportWithAlias(Ctx->ExecCxt, Ctx->StoreCxt,
                                                   ModInst, AliasNameStr);
          WasmEdge_StringDelete(AliasNameStr);
        }
      }
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onModule = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_ASTModuleContext *ASTModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(TC->LoadCxt, &ASTModCxt, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(TC->ValidCxt, ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ASTModCxt);
      return Unexpect(convResult(Res));
    }
    WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    if (ModName.empty()) {
      Res = WasmEdge_ExecutorInstantiate(TC->ExecCxt, &ModCxt, TC->StoreCxt,
                                         ASTModCxt);
    } else {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      Res = WasmEdge_ExecutorRegister(TC->ExecCxt, &ModCxt, TC->StoreCxt,
                                      ASTModCxt, ModStr);
    }
    WasmEdge_ASTModuleDelete(ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    if (ModName.empty()) {
      TC->ActiveModCxt = ModCxt;
    }
    TC->InstantiatedMod.push_back(ModCxt);
    return {};
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(TC->LoadCxt, &ModCxt, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    WasmEdge_ASTModuleDelete(ModCxt);
    return {};
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_ASTModuleContext *ModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(TC->LoadCxt, &ModCxt, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(TC->ValidCxt, ModCxt);
    WasmEdge_ASTModuleDelete(ModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_ASTModuleContext *ASTMod = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(TC->LoadCxt, &ASTMod, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(TC->ValidCxt, ASTMod);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return std::unique_ptr<AST::Module>(
        reinterpret_cast<AST::Module *>(ASTMod));
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    const WasmEdge_ASTModuleContext *ASTModCxt =
        reinterpret_cast<const WasmEdge_ASTModuleContext *>(&ASTMod);
    WasmEdge_String ModStr = WasmEdge_StringWrap(
        ModName.data(), static_cast<uint32_t>(ModName.length()));
    WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    WasmEdge_Result Res = WasmEdge_ExecutorRegister(
        TC->ExecCxt, &ModCxt, TC->StoreCxt, ASTModCxt, ModStr);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    TC->InstantiatedMod.push_back(ModCxt);
    return {};
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_ASTModuleContext *ASTModCxt = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(TC->LoadCxt, &ASTModCxt, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(TC->ValidCxt, ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      WasmEdge_ASTModuleDelete(ASTModCxt);
      return Unexpect(convResult(Res));
    }
    WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    Res = WasmEdge_ExecutorInstantiate(TC->ExecCxt, &ModCxt, TC->StoreCxt,
                                       ASTModCxt);
    WasmEdge_ASTModuleDelete(ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    TC->ActiveModCxt = ModCxt;
    TC->InstantiatedMod.push_back(ModCxt);
    return {};
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto *TC = static_cast<TestContext *>(Ctx);
    WasmEdge_Result Res;
    std::vector<WasmEdge_Value> CParams = convFromValVec(Params, ParamTypes);
    std::vector<WasmEdge_Value> CReturns;
    WasmEdge_FunctionInstanceContext *FuncCxt = nullptr;
    WasmEdge_String FieldStr = WasmEdge_StringWrap(
        Field.data(), static_cast<uint32_t>(Field.length()));
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager. Get the function type to specify the return nums.
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      const WasmEdge_ModuleInstanceContext *ModCxt =
          WasmEdge_StoreFindModule(TC->StoreCxt, ModStr);
      FuncCxt = WasmEdge_ModuleInstanceFindFunction(ModCxt, FieldStr);
    } else {
      // Invoke function of current active module. Get function type to specify
      // the return nums.
      FuncCxt = WasmEdge_ModuleInstanceFindFunction(TC->ActiveModCxt, FieldStr);
    }

    if (FuncCxt == nullptr) {
      return Unexpect(ErrCode::Value::FuncNotFound);
    }
    const WasmEdge_FunctionTypeContext *FuncType =
        WasmEdge_FunctionInstanceGetFunctionType(FuncCxt);
    CReturns.resize(WasmEdge_FunctionTypeGetReturnsLength(FuncType));

    // Execute.
    Res = WasmEdge_ExecutorInvoke(TC->ExecCxt, FuncCxt, CParams.data(),
                                  static_cast<uint32_t>(CParams.size()),
                                  CReturns.data(),
                                  static_cast<uint32_t>(CReturns.size()));

    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return convToValVec(CReturns);
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto *TC = static_cast<TestContext *>(Ctx);
    // Get module instance.
    const WasmEdge_ModuleInstanceContext *ModCxt = nullptr;
    if (ModName.empty()) {
      ModCxt = TC->ActiveModCxt;
    } else {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      ModCxt = WasmEdge_StoreFindModule(TC->StoreCxt, ModStr);
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
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter,
                                  /* IncludeComponent */ false)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

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

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
