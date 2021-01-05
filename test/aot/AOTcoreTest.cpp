// SPDX-License-Identifier: Apache-2.0
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

#include "common/configure.h"
#include "common/filesystem.h"
#include "common/log.h"

#include "aot/compiler.h"
#include "validator/validator.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"
#include "gtest/gtest.h"

#include <fstream>
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
  WasmEdge::VM::VM VM(Conf);
  WasmEdge::SpecTestModule SpecTestMod;
  VM.registerModule(SpecTestMod);
  auto Compile = [&, Conf = std::cref(Conf)](
                     const std::string &Filename) -> Expect<std::string> {
    WasmEdge::Loader::Loader Loader(Conf);
    WasmEdge::Validator::Validator ValidatorEngine(Conf);
    WasmEdge::AOT::Compiler Compiler;
    Compiler.setOptimizationLevel(
        WasmEdge::AOT::Compiler::OptimizationLevel::O0);
    Compiler.setDumpIR(true);
    auto Path = std::filesystem::u8path(Filename);
    Path.replace_extension(std::filesystem::u8path(".so"sv));
    const auto SOPath = Path.u8string();
    auto Data = *Loader.loadFile(Filename);
    auto Module = *Loader.parseModule(Data);
    if (auto Res = ValidatorEngine.validate(*Module); !Res) {
      return Unexpect(Res);
    }
    if (auto Res = Compiler.compile(Data, *Module, SOPath); !Res) {
      return Unexpect(Res);
    }
    return SOPath;
  };
  T.onModule = [&VM, &Compile](const std::string &ModName,
                               const std::string &Filename) -> Expect<void> {
    return Compile(Filename).and_then(
        [&VM, &ModName](const std::string &SOFilename) -> Expect<void> {
          if (!ModName.empty()) {
            return VM.registerModule(ModName, SOFilename);
          } else {
            return VM.loadWasm(SOFilename)
                .and_then([&VM]() { return VM.validate(); })
                .and_then([&VM]() { return VM.instantiate(); });
          }
        });
  };
  T.onLoad = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename);
  };
  T.onValidate = [&VM, &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM,
                     &Compile](const std::string &Filename) -> Expect<void> {
    return Compile(Filename)
        .and_then([&](const std::string &SOFilename) -> Expect<void> {
          return VM.loadWasm(SOFilename);
        })
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  /// Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params,
                     const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<ValVariant>> {
    if (!ModName.empty()) {
      /// Invoke function of named module. Named modules are registered in
      /// Store Manager.
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      /// Invoke function of anonymous module. Anonymous modules are
      /// instantiated in VM.
      return VM.execute(Field, Params, ParamTypes);
    }
  };
  /// Helper function to get values.
  T.onGet = [&VM](const std::string &ModName,
                  const std::string &Field) -> Expect<std::vector<ValVariant>> {
    /// Get module instance.
    auto &Store = VM.getStoreManager();
    WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = *Store.getActiveModule();
    } else {
      if (auto Res = Store.findModule(ModName)) {
        ModInst = *Res;
      } else {
        return Unexpect(Res);
      }
    }

    /// Get global instance.
    auto &Globs = ModInst->getGlobalExports();
    if (Globs.find(Field) == Globs.cend()) {
      return Unexpect(ErrCode::IncompatibleImportType);
    }
    uint32_t GlobAddr = Globs.find(Field)->second;
    auto *GlobInst = *Store.getGlobal(GlobAddr);

    return std::vector<WasmEdge::ValVariant>{GlobInst->getValue()};
  };

  T.run(Proposal, UnitName);
}

/// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
