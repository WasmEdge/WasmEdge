// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/api/APIAOTVMCoreTest.cpp - WasmEdge C API AOT core tests//
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
#include <fstream>
#include <functional>
#include <iterator>
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
class CoreCompileTest : public testing::TestWithParam<std::string> {};
class CoreCompileArrayTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreCompileTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  const auto &ConfRef = Conf;

  // Define context structure for C API AOT
  struct TestContext {
    WasmEdge_VMContext *VM;
    WasmEdge_ModuleInstanceContext *TestModCxt;
    WasmEdge_CompilerContext *CompilerCxt;
    TestContext(const WasmEdge::Configure &C) {
      WasmEdge_ConfigureContext *ConfCxt = createConf(C);
      VM = WasmEdge_VMCreate(ConfCxt, nullptr);
      WasmEdge_ConfigureCompilerSetOptimizationLevel(
          ConfCxt, WasmEdge_CompilerOptimizationLevel_O0);
      WasmEdge_ConfigureCompilerSetOutputFormat(
          ConfCxt, WasmEdge_CompilerOutputFormat_Native);
      CompilerCxt = WasmEdge_CompilerCreate(ConfCxt);
      WasmEdge_ConfigureDelete(ConfCxt);
      TestModCxt = createSpecTestModule();
      WasmEdge_VMRegisterModuleFromImport(VM, TestModCxt);
    }
    ~TestContext() {
      WasmEdge_VMDelete(VM);
      WasmEdge_ModuleInstanceDelete(TestModCxt);
      WasmEdge_CompilerDelete(CompilerCxt);
    }
    Expect<std::string> compile(const std::string &FileName) {
      auto Path = std::filesystem::u8path(FileName);
      Path.replace_extension(std::filesystem::u8path(WASMEDGE_LIB_EXTENSION));
      const auto SOPath = Path.u8string();
      WasmEdge_Result Res = WasmEdge_CompilerCompile(
          CompilerCxt, FileName.c_str(), SOPath.c_str());
      if (!WasmEdge_ResultOK(Res)) {
        return Unexpect(convResult(Res));
      }
      return SOPath;
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      WasmEdge_StoreContext *ParentStore = WasmEdge_VMGetStoreContext(P->VM);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        WasmEdge_String ParentNameStr =
            WasmEdge_StringCreateByCString(ParentName.c_str());
        const WasmEdge_ModuleInstanceContext *ModInst =
            WasmEdge_StoreFindModule(ParentStore, ParentNameStr);
        WasmEdge_StringDelete(ParentNameStr);
        if (ModInst != nullptr) {
          WasmEdge_String AliasNameStr =
              WasmEdge_StringCreateByCString(AliasName.c_str());
          WasmEdge_VMRegisterModuleFromImportWithAlias(Ctx->VM, ModInst,
                                                       AliasNameStr);
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
    auto *VM = TC->VM;
    return TC->compile(FileName).and_then(
        [VM, &ModName](const std::string &SOFileName) -> Expect<void> {
          WasmEdge_Result Res;
          if (!ModName.empty()) {
            WasmEdge_String ModStr = WasmEdge_StringWrap(
                ModName.data(), static_cast<uint32_t>(ModName.length()));
            Res = WasmEdge_VMRegisterModuleFromFile(VM, ModStr,
                                                    SOFileName.c_str());
          } else {
            Res = WasmEdge_VMLoadWasmFromFile(VM, SOFileName.c_str());
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
        });
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto *VM = TC->VM;
    return TC->compile(FileName).and_then(
        [VM](const std::string &SOFileName) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFileName.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto *VM = TC->VM;
    return TC->compile(FileName).and_then(
        [VM](const std::string &SOFileName) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFileName.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          Res = WasmEdge_VMValidate(VM);
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return {};
        });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto *VM = TC->VM;
    return TC->compile(FileName).and_then(
        [VM](const std::string &SOFileName) -> Expect<SpecTest::WasmUnit> {
          WasmEdge_LoaderContext *LoadCxt = WasmEdge_VMGetLoaderContext(VM);
          WasmEdge_ValidatorContext *ValidCxt =
              WasmEdge_VMGetValidatorContext(VM);
          WasmEdge_ASTModuleContext *ASTMod = nullptr;
          WasmEdge_Result Res = WasmEdge_LoaderParseFromFile(
              LoadCxt, &ASTMod, SOFileName.c_str());
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          Res = WasmEdge_ValidatorValidate(ValidCxt, ASTMod);
          if (!WasmEdge_ResultOK(Res)) {
            return Unexpect(convResult(Res));
          }
          return std::unique_ptr<AST::Module>(
              reinterpret_cast<AST::Module *>(ASTMod));
        });
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    const WasmEdge_ASTModuleContext *ASTModCxt =
        reinterpret_cast<const WasmEdge_ASTModuleContext *>(&ASTMod);
    WasmEdge_String ModStr = WasmEdge_StringWrap(
        ModName.data(), static_cast<uint32_t>(ModName.length()));
    WasmEdge_Result Res =
        WasmEdge_VMRegisterModuleFromASTModule(VM, ModStr, ASTModCxt);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto *VM = TC->VM;
    return TC->compile(FileName).and_then(
        [VM](const std::string &SOFileName) -> Expect<void> {
          WasmEdge_Result Res =
              WasmEdge_VMLoadWasmFromFile(VM, SOFileName.c_str());
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
        });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
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
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
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
    if (ModCxt == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
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
    TestUnit, CoreCompileTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT,
                                  /* IncludeComponent */ false)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
