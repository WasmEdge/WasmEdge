// SPDX-License-Identifier: Apache-2.0
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

#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/types.h"

#include <functional>
#include <string_view>
#include <vector>

namespace WasmEdge {

class SpecTest {
public:
  enum class CommandID {
    Unknown,
    Module,
    Action,
    Register,
    AssertReturn,
    AssertTrap,
    AssertExhaustion,
    AssertMalformed,
    AssertInvalid,
    AssertUnlinkable,
    AssertUninstantiable,
  };

  explicit SpecTest(std::filesystem::path Root)
      : TestsuiteRoot(std::move(Root)) {}

  std::vector<std::string> enumerate() const;
  std::tuple<std::string_view, WasmEdge::Configure, std::string>
  resolve(std::string_view Params) const;
  bool compare(const std::vector<std::pair<std::string, std::string>> &Expected,
               const std::vector<ValVariant> &Got) const;
  bool stringContains(const std::string &Expected,
                      const std::string &Got) const;

  void run(std::string_view Proposal, std::string_view UnitName);

  using ModuleCallback = Expect<void>(const std::string &Modname,
                                      const std::string &Filename);
  std::function<ModuleCallback> onModule;

  using LoadCallback = Expect<void>(const std::string &Filename);
  std::function<LoadCallback> onLoad;

  using ValidateCallback = Expect<void>(const std::string &Filename);
  std::function<ValidateCallback> onValidate;

  using InstantiateCallback = Expect<void>(const std::string &Filename);
  std::function<InstantiateCallback> onInstantiate;

  using InvokeCallback = Expect<std::vector<ValVariant>>(
      const std::string &ModName, const std::string &Field,
      const std::vector<ValVariant> &Params,
      const std::vector<ValType> &ParamTypes);
  std::function<InvokeCallback> onInvoke;

  using GetCallback = Expect<std::vector<ValVariant>>(
      const std::string &ModName, const std::string &Field);
  std::function<GetCallback> onGet;

private:
  std::filesystem::path TestsuiteRoot;
};

} // namespace WasmEdge
