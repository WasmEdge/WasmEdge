// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/spec/hostfunc.h - Spec test host functions ----------===//
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

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

namespace WasmEdge {

class SpecTestPrint : public Runtime::HostFunction<SpecTestPrint> {
public:
  Expect<void> body(const Runtime::CallingFrame &) { return {}; }
};

class SpecTestPrintI32 : public Runtime::HostFunction<SpecTestPrintI32> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) { return {}; }
};

class SpecTestPrintI64 : public Runtime::HostFunction<SpecTestPrintI64> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint64_t) { return {}; }
};

class SpecTestPrintF32 : public Runtime::HostFunction<SpecTestPrintF32> {
public:
  Expect<void> body(const Runtime::CallingFrame &, float) { return {}; }
};

class SpecTestPrintF64 : public Runtime::HostFunction<SpecTestPrintF64> {
public:
  Expect<void> body(const Runtime::CallingFrame &, double) { return {}; }
};

class SpecTestPrintI32F32 : public Runtime::HostFunction<SpecTestPrintI32F32> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t, float) {
    return {};
  }
};

class SpecTestPrintF64F64 : public Runtime::HostFunction<SpecTestPrintF64F64> {
public:
  Expect<void> body(const Runtime::CallingFrame &, double, double) {
    return {};
  }
};

class SpecTestModule : public Runtime::Instance::ModuleInstance {
public:
  SpecTestModule() : ModuleInstance("spectest") {
    addHostFunc("print", std::make_unique<SpecTestPrint>());
    addHostFunc("print_i32", std::make_unique<SpecTestPrintI32>());
    addHostFunc("print_i64", std::make_unique<SpecTestPrintI64>());
    addHostFunc("print_f32", std::make_unique<SpecTestPrintF32>());
    addHostFunc("print_f64", std::make_unique<SpecTestPrintF64>());
    addHostFunc("print_i32_f32", std::make_unique<SpecTestPrintI32F32>());
    addHostFunc("print_f64_f64", std::make_unique<SpecTestPrintF64F64>());

    addHostTable("table", std::make_unique<Runtime::Instance::TableInstance>(
                              AST::TableType(TypeCode::FuncRef, 10, 20),
                              RefVariant(TypeCode::FuncRef)));

    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                                AST::MemoryType(1, 2)));

    addHostMemory("shared_memory",
                  std::make_unique<Runtime::Instance::MemoryInstance>(
                      AST::MemoryType(1, 2, true)));

    addHostGlobal(
        "global_i32",
        std::make_unique<Runtime::Instance::GlobalInstance>(
            AST::GlobalType(TypeCode::I32, ValMut::Const), uint32_t(666)));
    addHostGlobal(
        "global_i64",
        std::make_unique<Runtime::Instance::GlobalInstance>(
            AST::GlobalType(TypeCode::I64, ValMut::Const), uint64_t(666)));
    addHostGlobal(
        "global_f32",
        std::make_unique<Runtime::Instance::GlobalInstance>(
            AST::GlobalType(TypeCode::F32, ValMut::Const), float(666.6)));
    addHostGlobal(
        "global_f64",
        std::make_unique<Runtime::Instance::GlobalInstance>(
            AST::GlobalType(TypeCode::F64, ValMut::Const), double(666.6)));
  }
  ~SpecTestModule() noexcept override = default;
};

} // namespace WasmEdge
