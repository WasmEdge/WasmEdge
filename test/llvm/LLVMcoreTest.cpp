// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/aot/AOTcoreTest.cpp - Wasm test suites --------------===//
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

#include "common/defines.h"
#include "common/spdlog.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <map>
#include <memory>
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
class NativeCoreTest : public testing::TestWithParam<std::string> {};
class CustomWasmCoreTest : public testing::TestWithParam<std::string> {};
class JITCoreTest : public testing::TestWithParam<std::string> {};

TEST_P(NativeCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::Configure ModifiedConf = Conf;
  ModifiedConf.getRuntimeConfigure().setMaxCallDepth(1024);
  WasmEdge::VM::VM VM(ModifiedConf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  auto Compile = [&, ConfWrap = std::cref(ModifiedConf)](
                     const std::string &FileName) -> Expect<std::string> {
    WasmEdge::Configure CopyConf = ConfWrap.get();
    WasmEdge::Loader::Loader Loader(ConfWrap);
    WasmEdge::Validator::Validator ValidatorEngine(ConfWrap);
    CopyConf.getCompilerConfigure().setOutputFormat(
        CompilerConfigure::OutputFormat::Native);
    CopyConf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
    WasmEdge::LLVM::Compiler Compiler(CopyConf);
    WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
    auto Path = std::filesystem::u8path(FileName);
    Path.replace_extension(std::filesystem::u8path(WASMEDGE_LIB_EXTENSION));
    const auto SOPath = Path.u8string();
    std::vector<WasmEdge::Byte> Data;
    std::unique_ptr<WasmEdge::AST::Module> Module;
    return Loader.loadFile(FileName)
        .and_then([&](auto Result) noexcept {
          Data = std::move(Result);
          return Loader.parseModule(Data);
        })
        .and_then([&](auto Result) noexcept {
          Module = std::move(Result);
          return ValidatorEngine.validate(*Module);
        })
        .and_then([&]() noexcept { return Compiler.compile(*Module); })
        .and_then([&](auto Result) noexcept {
          return CodeGen.codegen(Data, std::move(Result), SOPath);
        })
        .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &FileName) -> Expect<void> {
    return Compile(FileName).and_then(
        [&VM, &ModName](const std::string &SOFileName) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFileName);
          } else {
            return VM.loadWasm(SOFileName)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [&VM](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName);
  };
  T.onValidate = [&VM, &Compile](const std::string &FileName) -> Expect<void> {
    return Compile(FileName)
        .and_then([&](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [&VM, &Compile](
          const std::string &FileName) -> Expect<std::unique_ptr<AST::Module>> {
    return Compile(FileName).and_then(
        [&VM](const std::string &SOFileName)
            -> Expect<std::unique_ptr<AST::Module>> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [&VM](const std::string &ModName,
                              const AST::Module &ASTMod) -> Expect<void> {
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &FileName) -> Expect<void> {
    return Compile(FileName)
        .and_then([&](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
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

TEST_P(CustomWasmCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::Configure ModifiedConf = Conf;
  ModifiedConf.getRuntimeConfigure().setMaxCallDepth(1024);
  WasmEdge::VM::VM VM(ModifiedConf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  auto Compile = [&, ConfWrap = std::cref(ModifiedConf)](
                     const std::string &FileName) -> Expect<std::string> {
    WasmEdge::Configure CopyConf = ConfWrap.get();
    WasmEdge::Loader::Loader Loader(ConfWrap);
    WasmEdge::Validator::Validator ValidatorEngine(ConfWrap);
    CopyConf.getCompilerConfigure().setOptimizationLevel(
        WasmEdge::CompilerConfigure::OptimizationLevel::O0);
    WasmEdge::LLVM::Compiler Compiler(CopyConf);
    WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
    auto Path = std::filesystem::u8path(FileName);
    Path.replace_extension(std::filesystem::u8path(".aot.wasm"));
    const auto SOPath = Path.u8string();
    std::vector<WasmEdge::Byte> Data;
    std::unique_ptr<WasmEdge::AST::Module> Module;
    return Loader.loadFile(FileName)
        .and_then([&](auto Result) noexcept {
          Data = std::move(Result);
          return Loader.parseModule(Data);
        })
        .and_then([&](auto Result) noexcept {
          Module = std::move(Result);
          return ValidatorEngine.validate(*Module);
        })
        .and_then([&]() noexcept { return Compiler.compile(*Module); })
        .and_then([&](auto Result) noexcept {
          return CodeGen.codegen(Data, std::move(Result), SOPath);
        })
        .and_then([&]() noexcept { return Expect<std::string>{SOPath}; });
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &FileName) -> Expect<void> {
    return Compile(FileName).and_then(
        [&VM, &ModName](const std::string &SOFileName) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFileName);
          } else {
            return VM.loadWasm(SOFileName)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [&VM](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName);
  };
  T.onValidate = [&VM, &Compile](const std::string &FileName) -> Expect<void> {
    return Compile(FileName)
        .and_then([&](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [&VM, &Compile](
          const std::string &FileName) -> Expect<std::unique_ptr<AST::Module>> {
    return Compile(FileName).and_then(
        [&VM](const std::string &SOFileName)
            -> Expect<std::unique_ptr<AST::Module>> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [&VM](const std::string &ModName,
                              const AST::Module &ASTMod) -> Expect<void> {
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &FileName) -> Expect<void> {
    return Compile(FileName)
        .and_then([&](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
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

TEST_P(JITCoreTest, TestSuites) {
  const auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  WasmEdge::Configure CopyConf = Conf;
  CopyConf.getRuntimeConfigure().setMaxCallDepth(1024);
  CopyConf.getRuntimeConfigure().setEnableJIT(true);
  CopyConf.getCompilerConfigure().setOptimizationLevel(
      WasmEdge::CompilerConfigure::OptimizationLevel::O0);
  WasmEdge::VM::VM VM(CopyConf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  T.onModule = [&VM](const std::string &ModName,
                     const std::string &FileName) -> Expect<void> {
    if (!ModName.empty()) {
      return VM.registerModule(ModName, FileName);
    } else {
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [&VM](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName);
  };
  T.onValidate = [&VM](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [&VM](
          const std::string &FileName) -> Expect<std::unique_ptr<AST::Module>> {
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTMod, Loader.parseModule(FileName));
    EXPECTED_TRY(Validator.validate(*ASTMod.get()));
    return ASTMod;
  };
  T.onInstanceFromDef = [&VM](const std::string &ModName,
                              const AST::Module &ASTMod) -> Expect<void> {
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [&VM](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName)
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
    TestUnit, NativeCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CustomWasmCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, JITCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::JIT)));

std::array<WasmEdge::Byte, 46> AsyncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x00, 0x0a,
    0x09, 0x01, 0x07, 0x00, 0x03, 0x40, 0x0c, 0x00, 0x0b, 0x0b};

TEST(AsyncRunWsmFile, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, NativeInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest" WASMEDGE_LIB_EXTENSION);
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  ASSERT_TRUE(VM.loadWasm(Path));
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
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncRunWsmFile, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  {
    auto Timeout =
        std::chrono::system_clock::now() + std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitUntil(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  {
    auto Timeout = std::chrono::milliseconds(1);
    auto AsyncResult = VM.asyncRunWasmFile(Path, "_start");
    EXPECT_FALSE(AsyncResult.waitFor(Timeout));
    AsyncResult.cancel();
    auto Result = AsyncResult.get();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::Interrupted);
  }
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(AsyncExecute, CustomWasmInterruptTest) {
  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setInterruptible(true);
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Wasm);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);
  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("AOTcoreTest.aot.wasm");
  auto Module = *Loader.parseModule(AsyncWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(AsyncWasm, std::move(*Data), Path));
  ASSERT_TRUE(VM.loadWasm(Path));
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
  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

TEST(Configure, ConfigureTest) {
  {
    WasmEdge::Configure Conf;
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_TRUE(Result);
  }
  {
    WasmEdge::Configure Conf;
    Conf.addProposal(Proposal::Annotations);
    WasmEdge::LLVM::Compiler Compiler(Conf);
    auto Result = Compiler.checkConfigure();
    EXPECT_FALSE(Result);
    EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::InvalidAOTConfigure);
  }
}

// Test for f32x4.max NaN handling (Issue #4257)
// This test verifies that f32x4.max correctly returns the RHS NaN when both
// operands are NaN, as per the WebAssembly SIMD spec.
//
// WAT source for SIMDNaNTestWasm:
// (module
//   (func (export "test_f32x4_max_nan") (result v128)
//     ;; LHS: v128.const with NaN values (0x7fc00001 in each lane)
//     v128.const i32x4 0x7fc00001 0x7fc00001 0x7fc00001 0x7fc00001
//     ;; RHS: v128.const with NaN values (0x7fc00000 in each lane)
//     v128.const i32x4 0x7fc00000 0x7fc00000 0x7fc00000 0x7fc00000
//     ;; f32x4.max should return RHS NaN (0x7fc00000) per spec
//     f32x4.max
//   )
// )
TEST(SIMDNaN, F32x4MaxNaNHandling) {
  // clang-format off
  std::array<WasmEdge::Byte, 88> SIMDNaNTestWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7b, 0x03, 0x02, 0x01, 0x00, 0x07, 0x16, 0x01, 0x12, 0x74,
      0x65, 0x73, 0x74, 0x5f, 0x66, 0x33, 0x32, 0x78, 0x34, 0x5f, 0x6d, 0x61,
      0x78, 0x5f, 0x6e, 0x61, 0x6e, 0x00, 0x00, 0x0a, 0x2b, 0x01, 0x29, 0x00,
      0xfd, 0x0c, 0x01, 0x00, 0xc0, 0x7f, 0x01, 0x00, 0xc0, 0x7f, 0x01, 0x00,
      0xc0, 0x7f, 0x01, 0x00, 0xc0, 0x7f, 0xfd, 0x0c, 0x00, 0x00, 0xc0, 0x7f,
      0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0xc0, 0x7f, 0x00, 0x00, 0xc0, 0x7f,
      0xfd, 0xe9, 0x01, 0x0b};
  // clang-format on

  WasmEdge::Configure Conf;
  Conf.getCompilerConfigure().setOutputFormat(
      CompilerConfigure::OutputFormat::Native);

  WasmEdge::VM::VM VM(Conf);
  WasmEdge::Loader::Loader Loader(Conf);
  WasmEdge::Validator::Validator ValidatorEngine(Conf);
  WasmEdge::LLVM::Compiler Compiler(Conf);
  WasmEdge::LLVM::CodeGen CodeGen(Conf);

  auto Path = std::filesystem::temp_directory_path() /
              std::filesystem::u8path("SIMDNaNTest" WASMEDGE_LIB_EXTENSION);

  auto Module = *Loader.parseModule(SIMDNaNTestWasm);
  ASSERT_TRUE(ValidatorEngine.validate(*Module));
  auto Data = Compiler.compile(*Module);
  ASSERT_TRUE(Data);
  ASSERT_TRUE(CodeGen.codegen(SIMDNaNTestWasm, std::move(*Data), Path));

  ASSERT_TRUE(VM.loadWasm(Path));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  auto Result = VM.execute("test_f32x4_max_nan");
  ASSERT_TRUE(Result);
  ASSERT_EQ((*Result).size(), 1U);

  auto ResultVal = (*Result)[0].first.get<WasmEdge::uint128_t>();

  uint32_t Lanes[4];
  std::copy_n(reinterpret_cast<const uint32_t *>(&ResultVal), 4, Lanes);

  // Per SIMD spec, f32x4.max with two NaN inputs should return RHS NaN
  const uint32_t ExpectedNaN = 0x7fc00000;
  EXPECT_EQ(Lanes[0], ExpectedNaN)
      << "Lane 0: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[0];
  EXPECT_EQ(Lanes[1], ExpectedNaN)
      << "Lane 1: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[1];
  EXPECT_EQ(Lanes[2], ExpectedNaN)
      << "Lane 2: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[2];
  EXPECT_EQ(Lanes[3], ExpectedNaN)
      << "Lane 3: Expected RHS NaN (0x7fc00000), got 0x" << std::hex
      << Lanes[3];

  VM.cleanup();
  EXPECT_NO_THROW(std::filesystem::remove(Path));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
