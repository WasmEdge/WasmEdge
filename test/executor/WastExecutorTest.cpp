// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/test/executor/WastExecutorTest.cpp - WAST spec tests -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file runs Wasm spec tests directly from .wast files, using the
/// tree-sitter WAST parser to parse the test scripts.
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "loader/loader.h"
#include "validator/validator.h"
#include "vm/vm.h"
#include "wat/parser.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(std::filesystem::u8path("../spec/testSuites"sv));

// Parameterized testing class.
class WastCoreTest : public testing::TestWithParam<std::string> {};

TEST_P(WastCoreTest, TestSuites) {
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

  T.Mode = SpecTest::ParserMode::Wast;

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

  T.onParse =
      [](SpecTest::ContextHandle Ctx, std::string_view Source,
         Wast::ModuleType Type,
         const WasmEdge::Configure &Conf) -> Expect<SpecTest::WasmUnit> {
    (void)Ctx;
    if (Type == Wast::ModuleType::Text) {
      // Inline WAT source text.
      EXPECTED_TRY(auto ASTMod, WAT::parseWat(Source, Conf));
      return SpecTest::WasmUnit(
          std::make_unique<AST::Module>(std::move(ASTMod)));
    } else if (Type == Wast::ModuleType::TextFile) {
      // File path to a .wat file - read content and parse.
      std::ifstream Ifs{std::string(Source)};
      std::stringstream SS;
      SS << Ifs.rdbuf();
      std::string Content = SS.str();
      EXPECTED_TRY(auto ASTMod, WAT::parseWat(std::string_view(Content), Conf));
      return SpecTest::WasmUnit(
          std::make_unique<AST::Module>(std::move(ASTMod)));
    } else if (Type == Wast::ModuleType::BinaryFile) {
      // File path to a .wasm binary.
      WasmEdge::Loader::Loader Ld(Conf);
      EXPECTED_TRY(auto ModPtr,
                   Ld.parseModule(std::filesystem::path(std::string(Source))));
      return SpecTest::WasmUnit(std::move(ModPtr));
    } else {
      // Binary source: raw bytes.
      WasmEdge::Loader::Loader Ld(Conf);
      EXPECTED_TRY(auto ModPtr,
                   Ld.parseModule(Span<const uint8_t>(
                       reinterpret_cast<const uint8_t *>(Source.data()),
                       Source.size())));
      return SpecTest::WasmUnit(std::move(ModPtr));
    }
  };

  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    SpecTest::WasmUnit &Unit) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &Mod = *std::get<std::unique_ptr<AST::Module>>(Unit);
    WasmEdge::Validator::Validator Valid(TC->VM.getExecutor().getConfigure());
    return Valid.validate(Mod);
  };

  T.onInstantiate = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                       const SpecTest::WasmUnit &Unit) -> Expect<void> {
    auto *TC = static_cast<TestContext *>(Ctx);
    auto &Mod = *std::get<std::unique_ptr<AST::Module>>(Unit);
    if (!ModName.empty()) {
      return TC->VM.registerModule(ModName, Mod);
    }
    return TC->VM.loadWasm(Mod)
        .and_then([TC]() { return TC->VM.validate(); })
        .and_then([TC]() { return TC->VM.instantiate(); });
  };

  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    if (!ModName.empty()) {
      return VM.execute(ModName, Field, Params, ParamTypes);
    } else {
      return VM.execute(Field, Params, ParamTypes);
    }
  };

  T.onGet =
      [](SpecTest::ContextHandle Ctx, const std::string &ModName,
         const std::string &Field) -> Expect<std::pair<ValVariant, ValType>> {
    auto &VM = static_cast<TestContext *>(Ctx)->VM;
    const WasmEdge::Runtime::Instance::ModuleInstance *ModInst = nullptr;
    if (ModName.empty()) {
      ModInst = VM.getActiveModule();
    } else {
      ModInst = VM.getStoreManager().findModule(ModName);
    }
    if (ModInst == nullptr) {
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }
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
    TestUnit, WastCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter, false)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
