// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/spec/spectest.h - Wasm test suites ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace WasmEdge {

class SpecTest {
public:
  enum class CommandID {
    Unknown,
    Module,
    ModuleDefinition,
    ModuleInstance,
    Action,
    Register,
    AssertReturn,
    AssertTrap,
    AssertExhaustion,
    AssertMalformed,
    AssertInvalid,
    AssertUnlinkable,
    AssertUninstantiable,
    AssertException,
  };

  enum class TestMode : uint8_t {
    Interpreter = 0x01U,
    AOT = 0x02U,
    AOT_Interpreter = 0x03U,
    JIT = 0x04U,
    JIT_Interpreter = 0x05U,
    JIT_AOT = 0x06U,
    All = 0x07U,
  };

  explicit SpecTest(std::filesystem::path Root)
      : TestsuiteRoot(std::move(Root)) {}

  std::vector<std::string> enumerate(const TestMode Mode) const;
  std::tuple<std::string_view, WasmEdge::Configure, std::string>
  resolve(std::string_view Params) const;
  bool compare(const std::pair<std::string, std::string> &Expected,
               const std::pair<ValVariant, ValType> &Got) const;
  bool
  compares(const std::vector<std::pair<std::string, std::string>> &Expected,
           const std::vector<std::pair<ValVariant, ValType>> &Got) const;
  bool stringContains(std::string_view Expected, std::string_view Got) const;

  void run(std::string_view Proposal, std::string_view UnitName);

  using ModuleCallback = Expect<void>(const std::string &Modname,
                                      const std::string &FileName);
  std::function<ModuleCallback> onModule;

  using LoadCallback = Expect<void>(const std::string &FileName);
  std::function<LoadCallback> onLoad;

  using ValidateCallback = Expect<void>(const std::string &FileName);
  std::function<ValidateCallback> onValidate;

  using ModuleDefineCallback =
      Expect<std::unique_ptr<AST::Module>>(const std::string &FileName);
  std::function<ModuleDefineCallback> onModuleDefine;

  using InstanceFromDefCallback = Expect<void>(const std::string &ModName,
                                               const AST::Module &ASTMod);
  std::function<InstanceFromDefCallback> onInstanceFromDef;

  using InstantiateCallback = Expect<void>(const std::string &FileName);
  std::function<InstantiateCallback> onInstantiate;

  using InvokeCallback = Expect<std::vector<std::pair<ValVariant, ValType>>>(
      const std::string &ModName, const std::string &Field,
      const std::vector<ValVariant> &Params,
      const std::vector<ValType> &ParamTypes);
  std::function<InvokeCallback> onInvoke;

  using GetCallback = Expect<std::pair<ValVariant, ValType>>(
      const std::string &ModName, const std::string &Field);
  std::function<GetCallback> onGet;

private:
  std::filesystem::path TestsuiteRoot;
};

} // namespace WasmEdge
