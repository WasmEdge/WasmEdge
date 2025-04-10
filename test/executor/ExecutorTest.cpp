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

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <gtest/gtest.h>
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
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  T.onModule = [&VM](const std::string &ModName,
                     const std::string &Filename) -> Expect<void> {
    if (!ModName.empty()) {
      return VM.registerModule(ModName, Filename);
    } else {
      return VM.loadWasm(Filename)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename);
  };
  T.onValidate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename).and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  // Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
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
  T.onGet = [&VM](const std::string &ModName, const std::string &Field)
      -> Expect<std::pair<ValVariant, ValType>> {
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

class ValidatorSubtypeTest : public testing::Test {
protected:
  void SetUp() override {
    Conf = std::make_unique<WasmEdge::Configure>();
    Conf->addProposal(WasmEdge::Proposal::GC);
    LoadEngine = std::make_unique<WasmEdge::Loader::Loader>(*Conf);
    ValidEngine = std::make_unique<WasmEdge::Validator::Validator>(*Conf);
  }

  std::unique_ptr<WasmEdge::Configure> Conf;
  std::unique_ptr<WasmEdge::Loader::Loader> LoadEngine;
  std::unique_ptr<WasmEdge::Validator::Validator> ValidEngine;
};

uint32_t calculateSectionSize(const std::vector<uint8_t> &Wasm, size_t offset) {
  return Wasm.size() - offset;
}

TEST_F(ValidatorSubtypeTest, MaxSubtypeDepthExceeded) {
  std::vector<uint8_t> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  };

  Wasm.push_back(65);
  Wasm.push_back(0x60);
  Wasm.push_back(0x00);
  Wasm.push_back(0x00);

  for (int i = 1; i < 65; i++) {
    Wasm.push_back(0x63);
    Wasm.push_back(0x01);
    Wasm.push_back(i - 1);
    Wasm.push_back(0x50);
    Wasm.push_back(0x00);
    Wasm.push_back(0x00);
  }

  Wasm[9] = calculateSectionSize(Wasm, 10);

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, InvalidTypeIndex) {
  std::vector<uint8_t> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                               0x00, 0x01, 0x0C, 0x02, 0x60, 0x00, 0x00,
                               0x63, 0x01, 0xFF, 0x50, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, InvalidSupertypeIndex) {
  std::vector<uint8_t> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                               0x00, 0x01, 0x0C, 0x02, 0x60, 0x00, 0x00,
                               0x63, 0x01, 0x02, 0x50, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ErrorPropagationRecursive) {
  std::vector<uint8_t> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                               0x00, 0x01, 0x15, 0x03, 0x60, 0x00, 0x00,
                               0x63, 0x01, 0x02, 0x50, 0x00, 0x00, 0x63,
                               0x01, 0x01, 0x50, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ErrorPropagationInTypeHierarchy) {
  std::vector<uint8_t> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
                               0x01, 0x1E, 0x04, 0x60, 0x00, 0x00, 0x63, 0x01,
                               0x00, 0x50, 0x00, 0x00, 0x63, 0x01, 0x01, 0x50,
                               0x00, 0x00, 0x63, 0x01, 0x05, 0x50, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, SubtypeDepthCalculationError) {
  std::vector<uint8_t> Wasm = {0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00,
                               0x00, 0x01, 0x0C, 0x02, 0x60, 0x00, 0x00,
                               0x63, 0x01, 0x01, 0x50, 0x00, 0x00};

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_FALSE(ValidationResult);
  EXPECT_EQ(ValidationResult.error(), WasmEdge::ErrCode::Value::InvalidSubType);
}

TEST_F(ValidatorSubtypeTest, ExactMaxSubtypeDepth) {
  std::vector<uint8_t> Wasm = {
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  };

  Wasm.push_back(64);

  Wasm.push_back(0x60);
  Wasm.push_back(0x00);
  Wasm.push_back(0x00);

  for (int i = 1; i < 64; i++) {
    Wasm.push_back(0x63);
    Wasm.push_back(0x01);
    Wasm.push_back(i - 1);
    Wasm.push_back(0x50);
    Wasm.push_back(0x00);
    Wasm.push_back(0x00);
  }

  Wasm[9] = calculateSectionSize(Wasm, 10);

  auto Result = LoadEngine->parseModule(Wasm);
  ASSERT_TRUE(Result);

  auto ValidationResult = ValidEngine->validate(**Result);
  EXPECT_TRUE(ValidationResult);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
