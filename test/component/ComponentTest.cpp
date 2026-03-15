// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/component/ComponentTest.cpp - Component spec tests --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wasm-tools.
/// Test Suits: https://github.com/WebAssembly/component-model/tree/main/test
/// wasm-tool: https://github.com/bytecodealliance/wasm-tools
///
//===----------------------------------------------------------------------===//

#include "common/spdlog.h"
#include "vm/vm.h"

#include "../spec/hostfunc.h"
#include "spectest.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
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
  T.onModule = [&](const std::string &ModName,
                   const std::string &FileName) -> Expect<void> {
    if (!ModName.empty()) {
      return VM.registerModule(ModName, FileName);
    } else {
      return VM.loadWasm(FileName)
          .and_then([&VM]() { return VM.validate(); })
          .and_then([&VM]() { return VM.instantiate(); });
    }
  };
  T.onLoad = [&](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName);
  };
  T.onValidate = [&](const std::string &FileName) -> Expect<void> {
    return VM.loadWasm(FileName).and_then([&VM]() { return VM.validate(); });
  };
  T.onModuleDefine = [&](const std::string &FileName)
      -> Expect<std::unique_ptr<AST::Component::Component>> {
    Loader::Loader &Loader = VM.getLoader();
    Validator::Validator &Validator = VM.getValidator();
    EXPECTED_TRY(auto ASTUnit, Loader.parseWasmUnit(FileName));
    auto *ASTComp =
        std::get_if<std::unique_ptr<AST::Component::Component>>(&ASTUnit);
    if (!ASTComp) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    EXPECTED_TRY(Validator.validate(*ASTComp->get()));
    return std::move(*ASTComp);
  };
  T.onInstanceFromDef = [&](const std::string &,
                            const AST::Component::Component &) -> Expect<void> {
    // TODO: implement this when component model instantiation supported.
    return {};
  };
  T.onInstantiate = [](const std::string &) -> Expect<void> {
    // TODO: implement this when component model instantiation supported.
    return {};
  };
  // Helper function to call functions.
  T.onInvoke = [](const std::string &, const std::string &,
                  const std::vector<ValVariant> &, const std::vector<ValType> &)
      -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
    // TODO: implement this when component model execution supported.
    return {};
  };
  // Helper function to get values.
  T.onGet = [](const std::string &,
               const std::string &) -> Expect<std::pair<ValVariant, ValType>> {
    // TODO: implement this when component model execution supported.
    return {};
  };

  T.run(Proposal, UnitName);
}

// Initiate test suite.
INSTANTIATE_TEST_SUITE_P(
    TestUnit, CoreTest,
    testing::ValuesIn(T.enumerate(SpecTest::TestMode::Interpreter)));

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
