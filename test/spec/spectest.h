// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/spec/spectest.h - Wasm test suites ----------------------===//
//
// Part of the SSVM Project.
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

#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/proposal.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include "runtime/instance/memory.h"
#include <functional>
#include <string_view>
#include <vector>

namespace SSVM {

class SpecTestPrint : public Runtime::HostFunction<SpecTestPrint> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst) { return {}; }
};

class SpecTestPrintI32 : public Runtime::HostFunction<SpecTestPrintI32> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Val) {
    return {};
  }
};

class SpecTestPrintF32 : public Runtime::HostFunction<SpecTestPrintF32> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, float Val) {
    return {};
  }
};

class SpecTestPrintF64 : public Runtime::HostFunction<SpecTestPrintF64> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, double Val) {
    return {};
  }
};

class SpecTestPrintI32F32 : public Runtime::HostFunction<SpecTestPrintI32F32> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, uint32_t Val1,
                    float Val2) {
    return {};
  }
};

class SpecTestPrintF64F64 : public Runtime::HostFunction<SpecTestPrintF64F64> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, double IVal,
                    double Val2) {
    return {};
  }
};

class SpecTestModule : public Runtime::ImportObject {
public:
  SpecTestModule() : ImportObject("spectest") {
    addHostFunc("print", std::make_unique<SpecTestPrint>());
    addHostFunc("print_i32", std::make_unique<SpecTestPrintI32>());
    addHostFunc("print_f32", std::make_unique<SpecTestPrintF32>());
    addHostFunc("print_f64", std::make_unique<SpecTestPrintF64>());
    addHostFunc("print_i32_f32", std::make_unique<SpecTestPrintI32F32>());
    addHostFunc("print_f64_f64", std::make_unique<SpecTestPrintF64F64>());

    AST::Limit TabLimit(10, 20);
    addHostTable("table", std::make_unique<Runtime::Instance::TableInstance>(
                              RefType::FuncRef, TabLimit));

    AST::Limit MemLimit(1, 2);
    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                                MemLimit));

    addHostGlobal("global_i32",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      ValType::I32, ValMut::Const, uint32_t(666)));
    addHostGlobal("global_i64",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      ValType::I64, ValMut::Const, uint64_t(666)));
    addHostGlobal("global_f32",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      ValType::F32, ValMut::Const, float(666)));
    addHostGlobal("global_f64",
                  std::make_unique<Runtime::Instance::GlobalInstance>(
                      ValType::F64, ValMut::Const, double(666)));
  }
  virtual ~SpecTestModule() = default;
};

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
  std::tuple<std::string_view, SSVM::ProposalConfigure, std::string>
  resolve(std::string_view Params) const;

  void run(std::string_view Proposal, std::string_view UnitName);

  using ModuleCallback = Expect<void>(const std::string &Modname,
                                      const std::string &Filename);
  std::function<ModuleCallback> onModule;

  using ValidateCallback = Expect<void>(const std::string &Filename);
  std::function<ValidateCallback> onValidate;

  using InstantiateCallback = Expect<void>(const std::string &Filename);
  std::function<InstantiateCallback> onInstantiate;

  using InvokeCallback = Expect<std::vector<ValVariant>>(
      const std::string &ModName, const std::string &Field,
      const std::vector<ValVariant> &Params);
  std::function<InvokeCallback> onInvoke;

  using GetCallback = Expect<std::vector<ValVariant>>(
      const std::string &ModName, const std::string &Field);
  std::function<GetCallback> onGet;

  /// Helper function to compare return values with expected values.
  using CompareCallback =
      bool(const std::vector<std::pair<std::string, std::string>> &Expected,
           const std::vector<ValVariant> &Got);
  std::function<CompareCallback> onCompare;

  using StringContainsCallback = bool(const std::string &Expected,
                                      const std::string &Got);
  std::function<StringContainsCallback> onStringContains;

private:
  std::filesystem::path TestsuiteRoot;
};

} // namespace SSVM
