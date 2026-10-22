// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/executor/WastExecutorTest.cpp - WAST spec tests -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This suite runs the Wasm spec tests directly from the .wast files. It uses
/// the WAST parser of tree-sitter.
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "../spec/spectest.h"
#include "../spec/vmcallback.h"
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;
static SpecTest T(u8path("../spec/testSuites"sv), SpecTest::ParserMode::Wast);

class WastCoreTest : public testing::TestWithParam<std::string> {};

TEST_P(WastCoreTest, TestSuites) {
  const auto [Proposal, Config, UnitName] = T.resolve(GetParam());
  const auto &ConfRef = Config;

  struct TestContext {
    // SpecTestMod must come before VM. The destructor of a class destroys the
    // members in the reverse order of their declaration. Every module imports
    // "spectest", so VM keeps SpecTestMod as a dependent. A destructor call on
    // SpecTestMod with a dependent that stays breaks the
    // assuming(!hasDependents()) test in the destructor of ModuleInstance.
    // For more information, see include/runtime/instance/module.h.
    WasmEdge::SpecTestModule SpecTestMod;
    WasmEdge::VM::VM VM;
    TestContext(const WasmEdge::Configure &C) : VM(C) {
      VM.registerModule(SpecTestMod);
    }
  };

  T.onInit = [&ConfRef](SpecTest::ContextHandle Parent,
                        const std::vector<std::pair<std::string, std::string>>
                            &SharedModules) -> SpecTest::ContextHandle {
    auto *Ctx = new TestContext(ConfRef);
    if (Parent != nullptr) {
      SpecTestVM::shareModules(Ctx->VM, static_cast<TestContext *>(Parent)->VM,
                               SharedModules);
    }
    return Ctx;
  };

  T.onFini = [](SpecTest::ContextHandle Ctx) {
    delete static_cast<TestContext *>(Ctx);
  };

  T.onParse = [](SpecTest::ContextHandle, std::string_view Source,
                 Wast::ModuleType Type, const WasmEdge::Configure &Conf) {
    return SpecTestVM::parse(Source, Type, Conf);
  };

  T.onValidate = [](SpecTest::ContextHandle Ctx,
                    SpecTest::WasmUnit &Unit) -> Expect<void> {
    return SpecTestVM::validate(static_cast<TestContext *>(Ctx)->VM, Unit);
  };

  T.onInstantiate = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                       const SpecTest::WasmUnit &Unit) -> Expect<void> {
    return SpecTestVM::instantiate(
        static_cast<TestContext *>(Ctx)->VM, ModName,
        *std::get<std::unique_ptr<AST::Module>>(Unit));
  };

  T.onInvoke = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
                  const std::string &Field,
                  const std::vector<ValVariant> &Params,
                  const std::vector<ValType> &ParamTypes) {
    return SpecTestVM::invoke(static_cast<TestContext *>(Ctx)->VM, ModName,
                              Field, Params, ParamTypes);
  };

  T.onGet = [](SpecTest::ContextHandle Ctx, const std::string &ModName,
               const std::string &Field) {
    return SpecTestVM::get(static_cast<TestContext *>(Ctx)->VM, ModName, Field);
  };

  T.run(Proposal, UnitName);
}

INSTANTIATE_TEST_SUITE_P(
    TestUnit, WastCoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter, false)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
