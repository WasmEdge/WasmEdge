// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/executor/ExecutorTest.cpp - Wasm test suites --------===//
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

#include "common/spdlog.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
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

  // Define context structure
  struct TestContext {
    WasmEdge::VM::VM VM;
    WasmEdge::SpecTestModule SpecTestMod;
    TestContext(const WasmEdge::Configure &C) : VM(C) {
      VM.registerModule(SpecTestMod);
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    // Always create VM with own Store to avoid module name conflicts
    // from built-in host modules being re-registered in a shared Store.
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          // Register the shared module under the alias name so that
          // the thread's wasm modules can import it by the expected name.
          Ctx->VM.registerModule(*ModInst, AliasName);
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
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      return VM.registerModule(ModName, FileName);
    } else if (T.SkipComponentValidation) {
      // For component-model tests where validation is not yet supported,
      // skip validation by force-setting the stage as validated.
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.forceValidateForComponent(); })
          .and_then([&VM]() { return VM.instantiate(); });
    } else {
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTUnit, Loader.parseWasmUnit(FileName));
    if (std::holds_alternative<std::unique_ptr<AST::Module>>(ASTUnit)) {
      auto &ASTMod = std::get<std::unique_ptr<AST::Module>>(ASTUnit);
      EXPECTED_TRY(Validator.validate(*ASTMod.get()));
    } else if (!T.SkipComponentValidation) {
      auto &ASTComp =
          std::get<std::unique_ptr<AST::Component::Component>>(ASTUnit);
      EXPECTED_TRY(Validator.validate(*ASTComp.get()));
    }
    return ASTUnit;
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      // Invoke function of named module. Named modules are registered in Store
      // Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      // Invoke function of anonymous module. Anonymous modules are instantiated
      // in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  // Helper function to get values.
  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    // Get module instance.
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    // Get global instance.
    WasmEdge::Runtime::Instance::GlobalInstance *GlobInst =
        ModInst->findGlobalExports(Field);
    if (unlikely(GlobInst == nullptr)) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
    return std::make_pair(GlobInst->getValue(),
                          GlobInst->getGlobalType().getValType());
  };

  T.run(Proposal, UnitName);
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
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(AsyncWasm, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(AsyncWasm, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(AsyncExecute, InterruptTest) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(AsyncWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncExecute("_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(AsyncInvoke, InterruptTest) {
  WasmEdge::Configure Conf;
  WasmEdge::Loader::Loader LoadEngine(Conf);
  WasmEdge::Validator::Validator ValidEngine(Conf);
  WasmEdge::Executor::Executor ExecEngine(Conf);
  WasmEdge::Runtime::StoreManager Store;

  auto AST = LoadEngine.parseModule(AsyncWasm);
  ASSERT_TRUE(AST);
  ASSERT_TRUE(ValidEngine.validate(**AST));
  auto Module = ExecEngine.instantiateModule(Store, **AST);
  ASSERT_TRUE(Module);
  auto FuncInst = (*Module)->findFuncExports("_start");
  ASSERT_NE(FuncInst, nullptr);
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = ExecEngine.asyncInvoke(FuncInst, {}, {});
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = ExecEngine.asyncInvoke(FuncInst, {}, {});
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
}

TEST(VM, MultipleVM) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM1(Conf);
  WasmEdge::VM::VM VM2(Conf);
  std::array<WasmEdge::Byte, 36> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x07, 0x0a, 0x01, 0x06, 0x5f, 0x73,
      0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a, 0x04, 0x01, 0x02, 0x00, 0x0b};
  ASSERT_TRUE(VM1.loadWasm(Wasm));
  ASSERT_TRUE(VM1.validate());
  ASSERT_TRUE(VM1.instantiate());
  ASSERT_TRUE(VM2.loadWasm(Wasm));
  ASSERT_TRUE(VM2.validate());
  ASSERT_TRUE(VM2.instantiate());
  auto Result1 = VM1.execute("_start");
  auto Result2 = VM2.execute("_start");
  EXPECT_TRUE(Result1);
  EXPECT_TRUE(Result2);
}

TEST(Coredump, generateCoredump) {
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setEnableCoredump(true);
  Conf.getRuntimeConfigure().setCoredumpWasmgdb(false);
  WasmEdge::VM::VM VM(Conf);
  std::array<WasmEdge::Byte, 70> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
      0x1e, 0x02, 0x03, 0x6d, 0x65, 0x6d, 0x02, 0x00, 0x14, 0x61, 0x63, 0x63,
      0x65, 0x73, 0x73, 0x5f, 0x6f, 0x75, 0x74, 0x5f, 0x6f, 0x66, 0x5f, 0x62,
      0x6f, 0x75, 0x6e, 0x64, 0x73, 0x00, 0x00, 0x0a, 0x0d, 0x01, 0x0b, 0x00,
      0x41, 0xf0, 0xa2, 0x04, 0x41, 0x00, 0x36, 0x02, 0x00, 0x0b};
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  VM.execute("access_out_of_bounds");
  bool FindCoredump = false;
  for (const auto &Entry : std::filesystem::directory_iterator("./")) {
    if (Entry.path().string().find("coredump.") != std::string::npos) {
      FindCoredump = true;
      break;
    }
  }
  EXPECT_TRUE(FindCoredump);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
