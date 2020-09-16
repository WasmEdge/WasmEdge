// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/core/coreTest.cpp - Wasm test suites --------------------===//
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

#include "../spec/spectest.h"
#include "support/filesystem.h"
#include "support/log.h"
#include "vm/configure.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using namespace std::literals;
using namespace SSVM;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

/// Parameterized testing class.
class CoreTest : public testing::TestWithParam<std::string> {};

TEST_P(CoreTest, TestSuites) {
  const std::string UnitName = GetParam();
  SSVM::VM::Configure Conf;
  SSVM::VM::VM VM(Conf);
  SSVM::SpecTestModule SpecTestMod;
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
  T.onValidate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename).and_then([&VM]() { return VM.validate(); });
  };
  T.onInstantiate = [&VM](const std::string &Filename) -> Expect<void> {
    return VM.loadWasm(Filename)
        .and_then([&VM]() { return VM.validate(); })
        .and_then([&VM]() { return VM.instantiate(); });
  };
  /// Helper function to call functions.
  T.onInvoke = [&VM](const std::string &ModName, const std::string &Field,
                     const std::vector<ValVariant> &Params)
      -> Expect<std::vector<ValVariant>> {
    if (!ModName.empty()) {
      /// Invoke function of named module. Named modules are registered in
      /// Store Manager.
      return VM.execute(ModName, Field, Params);
    } else {
      /// Invoke function of anonymous module. Anonymous modules are
      /// instantiated in VM.
      return VM.execute(Field, Params);
    }
  };
  /// Helper function to get values.
  T.onGet = [&VM](const std::string &ModName,
                  const std::string &Field) -> Expect<std::vector<ValVariant>> {
    /// Get module instance.
    auto &Store = VM.getStoreManager();
    SSVM::Runtime::Instance::ModuleInstance *ModInst = nullptr;
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

    return std::vector<SSVM::ValVariant>{GlobInst->getValue()};
  };
  T.onCompare =
      [](const std::vector<std::pair<std::string, std::string>> &Expected,
         const std::vector<ValVariant> &Got) -> bool {
    if (Expected.size() != Got.size()) {
      return false;
    }
    for (size_t I = 0; I < Expected.size(); ++I) {
      const auto &[Type, E] = Expected[I];
      const auto &G = Got[I];
      if (E.substr(0, 4) == "nan:"sv) {
        /// Handle NaN case
        /// TODO: nan:canonical and nan:arithmetic
        if (Type == "f32"sv) {
          const float F = std::get<float>(G);
          if (!std::isnan(F)) {
            return false;
          }
        } else if (Type == "f64"sv) {
          const double D = std::get<double>(G);
          if (!std::isnan(D)) {
            return false;
          }
        }
      } else if (Type == "externref"sv || Type == "funcref"sv) {
        /// Handle reference value case
        if (E == "null"sv) {
          return SSVM::isNullRef(G);
        } else {
          if (SSVM::isNullRef(G)) {
            return false;
          }
          uint32_t V1 = SSVM::retrieveRefIdx(G);
          uint32_t V2 = static_cast<uint32_t>(std::stoul(E));
          if (V1 != V2) {
            return false;
          }
        }
      } else if (Type == "i32"sv || Type == "f32"sv) {
        const uint32_t V1 = uint32_t(std::stoul(E));
        const uint32_t V2 = std::get<uint32_t>(G);
        if (V1 != V2) {
          return false;
        }
      } else if (Type == "i64"sv || Type == "f64"sv) {
        const uint64_t V2 = uint64_t(std::stoull(E));
        const uint64_t V1 = std::get<uint64_t>(G);
        if (V1 != V2) {
          return false;
        }
      } else {
        assert(false);
      }
    }
    return true;
  };
  T.onStringContains = [](const std::string &Expected,
                          const std::string &Got) -> bool {
    if (Expected.rfind(Got, 0) != 0) {
      std::cout << "   ##### expected text : " << Expected << '\n';
      std::cout << "   ######## error text : " << Got << '\n';
      return false;
    }
    return true;
  };

  T.run(UnitName);
}

/// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(TestUnit, CoreTest, testing::ValuesIn(T.enumerate()));
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  SSVM::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
