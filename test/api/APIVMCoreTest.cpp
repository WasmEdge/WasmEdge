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

#include "helper.h"
#include "hostfunc_c.h"
#include "wasmedge/wasmedge.h"

#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// Binary Wasm module (Provider V1):
///
/// (module
///   (func $add (param i32 i32) (result i32)
///     local.get 0
///     local.get 1
///     i32.add)
///   (export "add_func" (func $add))
/// )
unsigned char provider_1_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0c, 0x01,
    0x08, 0x61, 0x64, 0x64, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b};
unsigned int provider_1_wasm_len = 46;

/// Binary Wasm module (Provider V2):
/// Adds 100 to the sum of two integers.
///
/// (module
///   (func $add (param i32 i32) (result i32)
///     local.get 0
///     local.get 1
///     i32.add
///     i32.const 100
///     i32.add)
///   (export "add_func" (func $add))
/// )
unsigned char provider_2_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07,
    0x01, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01,
    0x00, 0x07, 0x0c, 0x01, 0x08, 0x61, 0x64, 0x64, 0x5f, 0x66,
    0x75, 0x6e, 0x63, 0x00, 0x00, 0x0a, 0x0d, 0x01, 0x0b, 0x00,
    0x20, 0x00, 0x20, 0x01, 0x6a, 0x41, 0xe4, 0x00, 0x6a, 0x0b};
unsigned int provider_2_wasm_len = 50;

/// Binary Wasm module (Consumer):
/// Imports the add function from the provider and calls it.
///
/// (module
///   (import "provider" "add_func" (func $add (param i32 i32) (result i32)))
///
///   (func (export "call_add") (param i32 i32) (result i32)
///    local.get 0
///    local.get 1
///    call $add)
/// )
unsigned char consumer_wasm[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01, 0x60,
    0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x02, 0x15, 0x01, 0x08, 0x70, 0x72, 0x6f,
    0x76, 0x69, 0x64, 0x65, 0x72, 0x08, 0x61, 0x64, 0x64, 0x5f, 0x66, 0x75,
    0x6e, 0x63, 0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0c, 0x01, 0x08,
    0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x61, 0x64, 0x64, 0x00, 0x01, 0x0a, 0x0a,
    0x01, 0x08, 0x00, 0x20, 0x00, 0x20, 0x01, 0x10, 0x00, 0x0b};
unsigned int consumer_wasm_len = 70;

namespace {
using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  const auto &ConfRef = Conf;

  // Define context structure for C API
  struct TestContext {
    WasmEdge_VMContext *VM;
    WasmEdge_ModuleInstanceContext *TestModCxt;
    TestContext(const WasmEdge::Configure &C) {
      WasmEdge_ConfigureContext *ConfCxt = createConf(C);
      VM = WasmEdge_VMCreate(ConfCxt, nullptr);
      WasmEdge_ConfigureDelete(ConfCxt);
      TestModCxt = createSpecTestModule();
      WasmEdge_VMRegisterModuleFromImport(VM, TestModCxt);
    }
    ~TestContext() {
      WasmEdge_VMDelete(VM);
      WasmEdge_ModuleInstanceDelete(TestModCxt);
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
          WasmEdge_VMRegisterModuleFromImportWithAlias(Ctx->VM, AliasNameStr,
                                                       ModInst);
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
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    WasmEdge_Result Res;
    if (!ModName.empty()) {
      WasmEdge_String ModStr = WasmEdge_StringWrap(
          ModName.data(), static_cast<uint32_t>(ModName.length()));
      Res = WasmEdge_VMRegisterModuleFromFile(VM, ModStr, FileName.c_str());
    } else {
      Res = WasmEdge_VMLoadWasmFromFile(VM, FileName.c_str());
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
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_VMValidate(VM);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return {};
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    WasmEdge_LoaderContext *LoadCxt = WasmEdge_VMGetLoaderContext(VM);
    WasmEdge_ValidatorContext *ValidCxt = WasmEdge_VMGetValidatorContext(VM);
    WasmEdge_ASTModuleContext *ASTMod = nullptr;
    WasmEdge_Result Res =
        WasmEdge_LoaderParseFromFile(LoadCxt, &ASTMod, FileName.c_str());
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    Res = WasmEdge_ValidatorValidate(ValidCxt, ASTMod);
    if (!WasmEdge_ResultOK(Res)) {
      return Unexpect(convResult(Res));
    }
    return std::unique_ptr<AST::Module>(
        reinterpret_cast<AST::Module *>(ASTMod));
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
    auto *VM = static_cast<TestContext *>(Ctx)->VM;
    WasmEdge_Result Res = WasmEdge_VMLoadWasmFromFile(VM, FileName.c_str());
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

TEST(WasmEdgeVM, DeleteRegisteredModule) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, nullptr);

  uint32_t originalCount = WasmEdge_VMListRegisteredModuleLength(VMCxt);

  WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("test_module");
  WasmEdge_ModuleInstanceContext *ModInst =
      WasmEdge_ModuleInstanceCreate(ModuleName);

  WasmEdge_Result Res = WasmEdge_VMRegisterModuleFromImport(VMCxt, ModInst);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount + 1);

  WasmEdge_VMDeleteRegisteredModule(VMCxt, ModuleName);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount);

  // Added check to ensure module is no longer accessible
  WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
  const WasmEdge_ModuleInstanceContext *FindResult =
      WasmEdge_StoreFindModule(StoreCxt, ModuleName);
  EXPECT_EQ(FindResult, nullptr);

  // Cleanup
  WasmEdge_StringDelete(ModuleName);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteNonExistentModule) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, nullptr);
  uint32_t originalCount = WasmEdge_VMListRegisteredModuleLength(VMCxt);
  WasmEdge_String ModuleName =
      WasmEdge_StringCreateByCString("nonexistent_module");

  // Try deleting a module that doesn’t exist — should not crash
  WasmEdge_VMDeleteRegisteredModule(VMCxt, ModuleName);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt),
            originalCount); // No change

  // Cleanup
  WasmEdge_StringDelete(ModuleName);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteInvalidInput) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_String ModuleName = WasmEdge_StringCreateByCString("test_module");
  WasmEdge_String EmptyName = WasmEdge_StringCreateByCString("");

  // Test null VM context, should not crash
  WasmEdge_VMDeleteRegisteredModule(nullptr, ModuleName);
  // Test empty module name, should not crash
  WasmEdge_VMDeleteRegisteredModule(VMCxt, EmptyName);
  uint32_t originalCount = WasmEdge_VMListRegisteredModuleLength(VMCxt);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount);

  // Cleanup
  WasmEdge_StringDelete(ModuleName);
  WasmEdge_StringDelete(EmptyName);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteProviderModule) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);
  WasmEdge_Result Res;

  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];

  // Register Provider 1
  WasmEdge_String PName = WasmEdge_StringCreateByCString("provider");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, PName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  // Register Consumer 1 Linked to Provider 1
  WasmEdge_String CName1 = WasmEdge_StringCreateByCString("consumer_1");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, CName1, consumer_wasm,
                                            consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  uint32_t originalCount = WasmEdge_VMListRegisteredModuleLength(VMCxt);

  // Delete Provider 1 (It Becomes Zombie)
  WasmEdge_VMDeleteRegisteredModule(VMCxt, PName);
  EXPECT_EQ(WasmEdge_StoreFindModule(StoreCxt, PName), nullptr);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount - 1);

  // Test Consumer 1 still works (Calling Zombie Provider 1)
  Res = WasmEdge_VMExecuteRegistered(VMCxt, CName1, FuncName, Params, 2,
                                     Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  // Register Provider 2 with the same name
  Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, PName, provider_2_wasm,
                                            provider_2_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount);

  // Register Consumer 2 Linked to Provider 2
  WasmEdge_String CName2 = WasmEdge_StringCreateByCString("consumer_2");
  WasmEdge_VMRegisterModuleFromBuffer(VMCxt, CName2, consumer_wasm,
                                      consumer_wasm_len);

  // Unregister Provder 2
  WasmEdge_VMDeleteRegisteredModule(VMCxt, PName);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount);

  // Test Consumer 2 uses Provider 2 (Result 130)
  Res = WasmEdge_VMExecuteRegistered(VMCxt, CName2, FuncName, Params, 2,
                                     Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 130);

  // Cleanup Consumer 1 and Consumer 2
  WasmEdge_VMDeleteRegisteredModule(VMCxt, CName1);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount - 1);
  WasmEdge_VMDeleteRegisteredModule(VMCxt, CName2);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount - 2);

  // Cleanup
  WasmEdge_StringDelete(PName);
  WasmEdge_StringDelete(CName1);
  WasmEdge_StringDelete(CName2);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteProviderModuleWithAnonymousConsumer) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(Conf, nullptr);
  WasmEdge_StoreContext *StoreCxt = WasmEdge_VMGetStoreContext(VMCxt);

  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];
  WasmEdge_Result Res;

  // Register Provider 1
  WasmEdge_String PName = WasmEdge_StringCreateByCString("provider");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, PName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  // Instantiate Anonymous Consumer 1 Links to Provider 1
  Res = WasmEdge_VMLoadWasmFromBuffer(VMCxt, consumer_wasm, consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  Res = WasmEdge_VMValidate(VMCxt);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  Res = WasmEdge_VMInstantiate(VMCxt);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  uint32_t originalCount = WasmEdge_VMListRegisteredModuleLength(VMCxt);

  // Unregister Provider 1 (It becomes Zombie)
  WasmEdge_VMDeleteRegisteredModule(VMCxt, PName);
  EXPECT_EQ(WasmEdge_StoreFindModule(StoreCxt, PName), nullptr);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount - 1);

  // Consumer 1 should still work (Result 30)
  Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  // Register Provider 2 with the same name
  Res = WasmEdge_VMRegisterModuleFromBuffer(VMCxt, PName, provider_2_wasm,
                                            provider_2_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount);

  // Instantiate Anonymous Consumer 2
  // This replaces Consumer 1 as the "Active Module" and links to Provider 2
  Res = WasmEdge_VMLoadWasmFromBuffer(VMCxt, consumer_wasm, consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  Res = WasmEdge_VMValidate(VMCxt);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  Res = WasmEdge_VMInstantiate(VMCxt);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  // Unregister Provider 2 (It becomes Zombie)
  WasmEdge_VMDeleteRegisteredModule(VMCxt, PName);
  EXPECT_EQ(WasmEdge_StoreFindModule(StoreCxt, PName), nullptr);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VMCxt), originalCount - 1);

  // Consumer 2 should still work (Result 130)
  Res = WasmEdge_VMExecute(VMCxt, FuncName, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 130);

  // Cleanup
  WasmEdge_StringDelete(PName);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteSharedProviderModule) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *SharedStore = WasmEdge_StoreCreate();
  WasmEdge_VMContext *VM1 = WasmEdge_VMCreate(Conf, SharedStore);
  WasmEdge_VMContext *VM2 = WasmEdge_VMCreate(Conf, SharedStore);

  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];
  WasmEdge_Result Res;

  WasmEdge_String PName = WasmEdge_StringCreateByCString("provider");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VM1, PName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  WasmEdge_String CName1 = WasmEdge_StringCreateByCString("consumer_1");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VM2, CName1, consumer_wasm,
                                            consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  EXPECT_NE(WasmEdge_StoreFindModule(SharedStore, PName), nullptr);
  EXPECT_NE(WasmEdge_StoreFindModule(SharedStore, CName1), nullptr);

  WasmEdge_VMDeleteRegisteredModule(VM1, PName);
  EXPECT_EQ(WasmEdge_StoreFindModule(SharedStore, PName), nullptr);

  Res = WasmEdge_VMExecuteRegistered(VM2, CName1, FuncName, Params, 2, Returns,
                                     1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  WasmEdge_StringDelete(PName);
  WasmEdge_StringDelete(CName1);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_VMDelete(VM1);
  WasmEdge_VMDelete(VM2);
  WasmEdge_StoreDelete(SharedStore);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteSharedModuleViaBorrower) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *SharedStore = WasmEdge_StoreCreate();
  WasmEdge_VMContext *VM1 = WasmEdge_VMCreate(Conf, SharedStore);
  WasmEdge_VMContext *VM2 = WasmEdge_VMCreate(Conf, SharedStore);

  WasmEdge_ExecutorContext *Exec = WasmEdge_ExecutorCreate(Conf, nullptr);
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("add_func");
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];
  WasmEdge_Result Res;

  uint32_t originalCount1 = WasmEdge_VMListRegisteredModuleLength(VM1);
  uint32_t originalCount2 = WasmEdge_VMListRegisteredModuleLength(VM2);

  // Register the same module in both VMs, it should be shared in the Store
  WasmEdge_String MName = WasmEdge_StringCreateByCString("module_shared");
  Res = WasmEdge_VMRegisterModuleFromBuffer(VM1, MName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VM1), originalCount1 + 1);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VM2), originalCount2 + 1);
  EXPECT_NE(WasmEdge_StoreFindModule(SharedStore, MName), nullptr);

  // Get the module instance from VM1 and find the function, it should be valid
  const WasmEdge_ModuleInstanceContext *ModInst =
      WasmEdge_VMGetRegisteredModule(VM1, MName);
  EXPECT_NE(ModInst, nullptr);
  const WasmEdge_FunctionInstanceContext *FuncInst =
      WasmEdge_ModuleInstanceFindFunction(ModInst, FuncName);
  EXPECT_NE(FuncInst, nullptr);

  // Deleting the module from VM2
  WasmEdge_VMDeleteRegisteredModule(VM2, MName);

  // We can't find the module in the Store, but it should still be alive in VM1
  EXPECT_EQ(WasmEdge_StoreFindModule(SharedStore, MName), nullptr);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VM1), originalCount1);
  EXPECT_EQ(WasmEdge_VMListRegisteredModuleLength(VM2), originalCount2);

  // We still can execute functions from the module in VM1, proving it’s still
  // alive
  Res = WasmEdge_ExecutorInvoke(Exec, FuncInst, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  WasmEdge_StringDelete(MName);
  WasmEdge_VMDelete(VM1);
  WasmEdge_VMDelete(VM2);
  WasmEdge_StoreDelete(SharedStore);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteOwnerVMWithDependency) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *SharedStore = WasmEdge_StoreCreate();
  WasmEdge_VMContext *VM1 = WasmEdge_VMCreate(Conf, SharedStore);
  WasmEdge_VMContext *VM2 = WasmEdge_VMCreate(Conf, SharedStore);

  WasmEdge_String PName = WasmEdge_StringCreateByCString("provider");
  WasmEdge_String CName = WasmEdge_StringCreateByCString("consumer");
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];
  WasmEdge_Result Res;

  Res = WasmEdge_VMRegisterModuleFromBuffer(VM1, PName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  Res = WasmEdge_VMRegisterModuleFromBuffer(VM2, CName, consumer_wasm,
                                            consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  EXPECT_NE(WasmEdge_StoreFindModule(SharedStore, PName), nullptr);
  EXPECT_NE(WasmEdge_StoreFindModule(SharedStore, CName), nullptr);

  WasmEdge_VMDelete(VM1);
  EXPECT_EQ(WasmEdge_StoreFindModule(SharedStore, PName), nullptr);

  Res =
      WasmEdge_VMExecuteRegistered(VM2, CName, FuncName, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  WasmEdge_StringDelete(PName);
  WasmEdge_StringDelete(CName);
  WasmEdge_StringDelete(FuncName);
  WasmEdge_VMDelete(VM2);
  WasmEdge_StoreDelete(SharedStore);
  WasmEdge_ConfigureDelete(Conf);
}

TEST(WasmEdgeVM, DeleteSharedStoreWithDependency) {
  WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *SharedStore = WasmEdge_StoreCreate();
  WasmEdge_VMContext *VM1 = WasmEdge_VMCreate(Conf, SharedStore);
  WasmEdge_VMContext *VM2 = WasmEdge_VMCreate(Conf, SharedStore);

  WasmEdge_String PName = WasmEdge_StringCreateByCString("provider");
  WasmEdge_String CName = WasmEdge_StringCreateByCString("consumer");
  WasmEdge_String FuncName = WasmEdge_StringCreateByCString("call_add");
  WasmEdge_ExecutorContext *Exec = WasmEdge_ExecutorCreate(Conf, nullptr);
  WasmEdge_Value Params[2] = {WasmEdge_ValueGenI32(10),
                              WasmEdge_ValueGenI32(20)};
  WasmEdge_Value Returns[1];
  WasmEdge_Result Res;

  Res = WasmEdge_VMRegisterModuleFromBuffer(VM1, PName, provider_1_wasm,
                                            provider_1_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  Res = WasmEdge_VMRegisterModuleFromBuffer(VM2, CName, consumer_wasm,
                                            consumer_wasm_len);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));

  const WasmEdge_ModuleInstanceContext *ModInst =
      WasmEdge_VMGetRegisteredModule(VM2, CName);
  const WasmEdge_FunctionInstanceContext *FuncInst =
      WasmEdge_ModuleInstanceFindFunction(ModInst, FuncName);
  ASSERT_NE(FuncInst, nullptr);

  WasmEdge_StoreDelete(SharedStore);

  Res = WasmEdge_ExecutorInvoke(Exec, FuncInst, Params, 2, Returns, 1);
  EXPECT_TRUE(WasmEdge_ResultOK(Res));
  EXPECT_EQ(WasmEdge_ValueGetI32(Returns[0]), 30);

  WasmEdge_ExecutorDelete(Exec);
  WasmEdge_VMDelete(VM1);
  WasmEdge_VMDelete(VM2);
  WasmEdge_ConfigureDelete(Conf);
  WasmEdge_StringDelete(PName);
  WasmEdge_StringDelete(CName);
  WasmEdge_StringDelete(FuncName);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
