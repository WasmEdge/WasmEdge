// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/spectest.h - Wasm test suites ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parses and runs the Wasm test suites. The suites come from the
/// JSON that wast2json extracts, or directly from the .wast scripts through
/// the WAST parser of tree-sitter.
/// Test Suites: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "common/types.h"
#include "wast.h"

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
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
    Thread,
    Wait,
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

  enum class ParserMode { Json, Wast };

  explicit SpecTest(std::filesystem::path Root)
      : TestsuiteRoot(std::move(Root)) {}

  std::vector<std::string> enumerate(const TestMode Modes,
                                     bool IncludeComponent = true) const;
  std::tuple<std::string_view, WasmEdge::Configure, std::string>
  resolve(std::string_view Params) const;
  bool compareResult(const Wast::Result &Expected,
                     const std::pair<ValVariant, ValType> &Got) const;
  bool
  compareResults(const std::vector<Wast::ResultOrEither> &Expected,
                 const std::vector<std::pair<ValVariant, ValType>> &Got) const;
  bool stringContains(std::string_view Expected, std::string_view Got) const;

  void run(std::string_view Proposal, std::string_view UnitName);

  // This is an opaque handle to an execution context, such as a VM or an
  // Executor. Each test harness defines the type behind the pointer.
  using ContextHandle = void *;

  // Make a new execution context.
  //   Parent: The parent context. For the root context it is nullptr.
  //   SharedModules: The pairs of (export_name, alias_name) for the modules
  //     that this context can use. The harness must find each module by the
  //     first name in the parent store. Then it must register the module
  //     under the second name in the child store.
  using InitCallback = ContextHandle(
      ContextHandle Parent,
      const std::vector<std::pair<std::string, std::string>> &SharedModules);
  std::function<InitCallback> onInit;

  // Destroy an execution context and free its resources.
  using FiniCallback = void(ContextHandle Ctx);
  std::function<FiniCallback> onFini;

  using WasmUnit = std::variant<std::unique_ptr<AST::Component::Component>,
                                std::unique_ptr<AST::Module>>;

  // Parse the source into a WasmUnit. The source is WAT text, binary bytes, or
  // a file path.
  using ParseCallback = Expect<WasmUnit>(ContextHandle Ctx,
                                         std::string_view Source,
                                         Wast::ModuleType Type,
                                         const Configure &Conf);
  std::function<ParseCallback> onParse;

  // Validate a WasmUnit that the parser made.
  using ValidateCallback = Expect<void>(ContextHandle Ctx, WasmUnit &Unit);
  std::function<ValidateCallback> onValidate;

  // Instantiate a WasmUnit. The caller can also register it under ModName.
  using InstantiateCallback = Expect<void>(ContextHandle Ctx,
                                           const std::string &ModName,
                                           const WasmUnit &Unit);
  std::function<InstantiateCallback> onInstantiate;

  using InvokeCallback = Expect<std::vector<std::pair<ValVariant, ValType>>>(
      ContextHandle Ctx, const std::string &ModName, const std::string &Field,
      const std::vector<ValVariant> &Params,
      const std::vector<ValType> &ParamTypes);
  std::function<InvokeCallback> onInvoke;

  using GetCallback = Expect<std::pair<ValVariant, ValType>>(
      ContextHandle Ctx, const std::string &ModName, const std::string &Field);
  std::function<GetCallback> onGet;

  ParserMode Mode = ParserMode::Json;

  // If this flag is true, the callbacks mark a component as validated. The
  // instantiation can then continue without validation support. The flag is
  // thread_local, so that two concurrent spec-test threads do not overwrite
  // each other. Remove this flag after the component model is complete.
  static thread_local bool SkipComponentValidation;

private:
  // Run the WAST commands or JSON commands of the parser on a context.
  void executeCommands(Span<const Wast::ScriptCommand> Commands,
                       const Configure &Conf, bool IsComponent,
                       ContextHandle RootCtx, std::string TestFile);

  std::filesystem::path TestsuiteRoot;
};

} // namespace WasmEdge
