// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/SpecAOTTest.cpp - Spec tests -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suites: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "common/defines.h"
#include "common/filesystem.h"
#include "common/spdlog.h"
#include "executor/executor.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "llvm/codegen.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"

#include "hostfunc.h"
#include "spectest.h"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(u8path("testSuites"sv));

// Parameterized testing class.
class NativeCoreTest : public testing::TestWithParam<std::string> {};
class CustomWasmCoreTest : public testing::TestWithParam<std::string> {};

TEST_P(NativeCoreTest, TestSuites) {
  auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  // Native AOT spec test: explicitly opt into RunMode::AOT so the runtime
  // load step uses the produced .so as AOT, instead of falling back to
  // interpreter under the new default mode.
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    WasmEdge::Configure Conf;
    TestContext(const WasmEdge::Configure &C) : VM(C), Conf(C) {
      VM.registerModule(SpecTestMod);
    }
    Expect<std::string> compile(const std::string &FileName) {
      WasmEdge::Configure CopyConf = Conf;
      WasmEdge::Loader::Loader Loader(Conf);
      WasmEdge::Validator::Validator ValidatorEngine(Conf);
      CopyConf.getCompilerConfigure().setOutputFormat(
          CompilerConfigure::OutputFormat::Native);
      CopyConf.getCompilerConfigure().setOptimizationLevel(
          WasmEdge::CompilerConfigure::OptimizationLevel::O0);
      CopyConf.getCompilerConfigure().setDumpIR(true);
      WasmEdge::LLVM::Compiler Compiler(CopyConf);
      WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
      auto Path = u8path(FileName);
      Path.replace_extension(u8path(WASMEDGE_LIB_EXTENSION));
      const auto SOPath = u8string(Path);
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
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
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
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
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
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM](const std::string &SOFileName) -> Expect<SpecTest::WasmUnit> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
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
      // in the VM.
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

TEST_P(CustomWasmCoreTest, TestSuites) {
  auto [Proposal, Conf, UnitName] = T.resolve(GetParam());
  // Universal-WASM AOT spec test: produced files are .aot.wasm (universal
  // WASM with an AOT custom section). Opt into RunMode::AOT so the
  // runtime load step actually loads the AOT section.
  Conf.getRuntimeConfigure().setRunMode(WasmEdge::RunMode::AOT);
  const auto &ConfRef = Conf;

  // Define context structure
  struct TestContext {
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    WasmEdge::Configure Conf;
    TestContext(const WasmEdge::Configure &C) : VM(C), Conf(C) {
      VM.registerModule(SpecTestMod);
    }
    Expect<std::string> compile(const std::string &FileName) {
      WasmEdge::Configure CopyConf = Conf;
      WasmEdge::Loader::Loader Loader(Conf);
      WasmEdge::Validator::Validator ValidatorEngine(Conf);
      CopyConf.getCompilerConfigure().setOptimizationLevel(
          WasmEdge::CompilerConfigure::OptimizationLevel::O0);
      CopyConf.getCompilerConfigure().setDumpIR(true);
      WasmEdge::LLVM::Compiler Compiler(CopyConf);
      WasmEdge::LLVM::CodeGen CodeGen(CopyConf);
      auto Path = u8path(FileName);
      Path.replace_extension(u8path(".aot.wasm"));
      const auto SOPath = u8string(Path);
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
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr && !SharedModules.empty()) {
      auto *P = static_cast<TestContext *>(Parent);
      for (const auto &[ParentName, AliasName] : SharedModules) {
        const auto *ModInst = P->VM.getStoreManager().findModule(ParentName);
        if (ModInst != nullptr) {
          Ctx->VM.registerModule(AliasName, *ModInst);
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
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
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
  T.onLoad = [](SpecTest::ContextHandle Ctx,
                const std::string &FileName) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.loadWasm(FileName);
  };
  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine =
      [](SpecTest::ContextHandle Ctx,
         const std::string &FileName) -> Expect<SpecTest::WasmUnit> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName).and_then(
        [&VM](const std::string &SOFileName) -> Expect<SpecTest::WasmUnit> {
          Loader::Loader &Loader = VM.getLoader();
          Validator::Validator &Validator = VM.getValidator();
          EXPECTED_TRY(auto ASTMod, Loader.parseModule(SOFileName));
          EXPECTED_TRY(Validator.validate(*ASTMod.get()));
          return ASTMod;
        });
  };
  T.onInstanceFromDef = [](SpecTest::ContextHandle Ctx,
                           const std::string &ModName,
                           const AST::Module &ASTMod) -> Expect<void> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    return VM.registerModule(ModName, ASTMod);
  };
  T.onInstantiate = [](SpecTest::ContextHandle Ctx,
                       const std::string &FileName) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &VM = TC->VM;
    return TC->compile(FileName)
        .and_then([&VM](const std::string &SOFileName) -> Expect<void> {
          return VM.loadWasm(SOFileName);
        })
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
      // in the VM.
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
    TestUnit, NativeCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CustomWasmCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::AOT)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
