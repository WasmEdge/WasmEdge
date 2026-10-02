// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/spec/hostfunc.h - Spec test host functions ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parses and runs tests of Wasm test suites extracted by wast2json.
/// Test Suites: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#pragma once

#include "ast/module.h"
#include "common/component_valtype.h"
#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"
#include "runtime/instance/module.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

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
    addHostTable("table64", std::make_unique<Runtime::Instance::TableInstance>(
                                AST::TableType(TypeCode::FuncRef,
                                               AST::Limit(10, 20, true, false)),
                                RefVariant(TypeCode::FuncRef)));

    addHostMemory("memory", std::make_unique<Runtime::Instance::MemoryInstance>(
                                AST::MemoryType(1, 2)));
    addHostMemory("shared_memory",
                  std::make_unique<Runtime::Instance::MemoryInstance>(
                      AST::MemoryType(AST::Limit(1, 2, false, true))));

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

// The host of the component-model suites, after the reference wast runner:
// the "host" instance with its resource, records, nested instance and core
// module, the instances the suites import by name, and the standalone
// functions "host-return-two" and "f".

/// The tag of the host resource `resource1`.
struct SpecTestResource {};

/// The drops of `resource1` the statics `drops` and `last-drop` report.
struct SpecTestResourceDrops {
  uint32_t Count = 0;
  uint32_t Last = 0;
};

// [constructor]resource1(r: u32) -> own<resource1>
class SpecTestResourceNew
    : public Runtime::Component::HostFunction<SpecTestResourceNew> {
public:
  static constexpr const char *ParamNames[] = {"r"};
  Expect<Runtime::Component::Own<SpecTestResource>>
  body(Runtime::Component::CallingFrame &, uint32_t Rep) {
    return Runtime::Component::Own<SpecTestResource>{Rep};
  }
};

// [static]resource1.assert(r: own<resource1>, rep: u32)
class SpecTestResourceAssert
    : public Runtime::Component::HostFunction<SpecTestResourceAssert> {
public:
  static constexpr const char *ParamNames[] = {"r", "rep"};
  Expect<void> body(Runtime::Component::CallingFrame &,
                    Runtime::Component::Own<SpecTestResource> Res,
                    uint32_t Rep) {
    if (Res.Rep != Rep) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    return {};
  }
};

// [method]resource1.simple(self: borrow<resource1>, rep: u32)
class SpecTestResourceSimple
    : public Runtime::Component::HostFunction<SpecTestResourceSimple> {
public:
  static constexpr const char *ParamNames[] = {"self", "rep"};
  Expect<void> body(Runtime::Component::CallingFrame &,
                    Runtime::Component::Borrow<SpecTestResource> Self,
                    uint32_t Rep) {
    if (Self.Rep != Rep) {
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    return {};
  }
};

// [method]resource1.take-own(self: borrow<resource1>, b: own<resource1>):
// the host keeps the resource it is given, which counts as a drop.
class SpecTestResourceTakeOwn
    : public Runtime::Component::HostFunction<SpecTestResourceTakeOwn> {
public:
  static constexpr const char *ParamNames[] = {"self", "b"};
  explicit SpecTestResourceTakeOwn(SpecTestResourceDrops &Record)
      : Drops(Record) {}
  Expect<void> body(Runtime::Component::CallingFrame &,
                    Runtime::Component::Borrow<SpecTestResource>,
                    Runtime::Component::Own<SpecTestResource> Taken) {
    Drops.Count += 1;
    Drops.Last = static_cast<uint32_t>(Taken.Rep);
    return {};
  }

private:
  SpecTestResourceDrops &Drops;
};

// [method]resource1.take-borrow(self: borrow<resource1>, b: borrow<resource1>)
class SpecTestResourceTakeBorrow
    : public Runtime::Component::HostFunction<SpecTestResourceTakeBorrow> {
public:
  static constexpr const char *ParamNames[] = {"self", "b"};
  Expect<void> body(Runtime::Component::CallingFrame &,
                    Runtime::Component::Borrow<SpecTestResource>,
                    Runtime::Component::Borrow<SpecTestResource>) {
    return {};
  }
};

// [static]resource1.drops() -> u32
class SpecTestResourceDropCount
    : public Runtime::Component::HostFunction<SpecTestResourceDropCount> {
public:
  explicit SpecTestResourceDropCount(const SpecTestResourceDrops &Record)
      : Drops(Record) {}
  Expect<uint32_t> body(Runtime::Component::CallingFrame &) {
    return Drops.Count;
  }

private:
  const SpecTestResourceDrops &Drops;
};

// [static]resource1.last-drop() -> u32
class SpecTestResourceLastDrop
    : public Runtime::Component::HostFunction<SpecTestResourceLastDrop> {
public:
  explicit SpecTestResourceLastDrop(const SpecTestResourceDrops &Record)
      : Drops(Record) {}
  Expect<uint32_t> body(Runtime::Component::CallingFrame &) {
    return Drops.Last;
  }

private:
  const SpecTestResourceDrops &Drops;
};

// A function returning the u32 it was built with: return-three, return-four,
// host-return-two.
class SpecTestReturnU32
    : public Runtime::Component::HostFunction<SpecTestReturnU32> {
public:
  explicit SpecTestReturnU32(uint32_t Val) : Value(Val) {}
  Expect<uint32_t> body(Runtime::Component::CallingFrame &) { return Value; }

private:
  uint32_t Value;
};

// f()
class SpecTestNop : public Runtime::Component::HostFunction<SpecTestNop> {
public:
  Expect<void> body(Runtime::Component::CallingFrame &) { return {}; }
};

/// The "host" instance. The core module `simple-module` it exports is parsed
/// by the caller: f() -> 101, g = 100.
class SpecTestComponent : public Runtime::Instance::ComponentInstance {
public:
  explicit SpecTestComponent(std::unique_ptr<AST::Module> Module)
      : ComponentInstance("host"), SimpleModule(std::move(Module)) {
    const uint32_t Resource1 =
        addHostResourceType<SpecTestResource>([this](uint64_t Rep) {
          Drops.Count += 1;
          Drops.Last = static_cast<uint32_t>(Rep);
        });
    exportType("resource1", Resource1);
    exportType("resource1-again", Resource1);
    exportType("resource2", addHostResourceType(nullptr));
    addHostFunc("[constructor]resource1",
                std::make_unique<SpecTestResourceNew>());
    addHostFunc("[static]resource1.assert",
                std::make_unique<SpecTestResourceAssert>());
    addHostFunc("[method]resource1.simple",
                std::make_unique<SpecTestResourceSimple>());
    addHostFunc("[method]resource1.take-own",
                std::make_unique<SpecTestResourceTakeOwn>(Drops));
    addHostFunc("[method]resource1.take-borrow",
                std::make_unique<SpecTestResourceTakeBorrow>());
    addHostFunc("[static]resource1.drops",
                std::make_unique<SpecTestResourceDropCount>(Drops));
    addHostFunc("[static]resource1.last-drop",
                std::make_unique<SpecTestResourceLastDrop>(Drops));
    addHostFunc("return-three", std::make_unique<SpecTestReturnU32>(3));

    const Runtime::Component::TypeMinter Mint = getTypeMinter();
    const ComponentValType X = Runtime::Component::WitRecord::type(
        Mint, {{"x", Runtime::Component::Wit<uint32_t>::type(Mint)}});
    exportType("x", X.getTypeIndex());
    const ComponentValType Rec = Runtime::Component::WitRecord::type(
        Mint,
        {{"x", X}, {"y", Runtime::Component::Wit<std::string>::type(Mint)}});
    exportType("rec", Rec.getTypeIndex());
    exportType("some-record", Rec.getTypeIndex());

    auto Nested = std::make_unique<ComponentInstance>("");
    Nested->addHostFunc("return-four", std::make_unique<SpecTestReturnU32>(4));
    exportComponentInstance("nested", addComponentInstance(std::move(Nested)));

    addModule(*SimpleModule);
    exportCoreModule("simple-module", 0);
  }
  ~SpecTestComponent() noexcept override = default;

private:
  SpecTestResourceDrops Drops;
  std::unique_ptr<AST::Module> SimpleModule;
};

/// The instance "a": the type u64 under the names "a" and "b".
class SpecTestU64Component : public Runtime::Instance::ComponentInstance {
public:
  SpecTestU64Component() : ComponentInstance("a") {
    AST::Component::DefValType DefValTy;
    DefValTy.setPrimValType(PrimValType::U64);
    const uint32_t U64 =
        getTypeMinter().defValType(std::move(DefValTy)).getTypeIndex();
    exportType("a", U64);
    exportType("b", U64);
  }
  ~SpecTestU64Component() noexcept override = default;
};

/// The instance "demo:component/types": the enum "baz" and the record "foo".
class SpecTestTypesComponent : public Runtime::Instance::ComponentInstance {
public:
  SpecTestTypesComponent() : ComponentInstance("demo:component/types") {
    const Runtime::Component::TypeMinter Mint = getTypeMinter();
    AST::Component::EnumTy Enum;
    Enum.Labels.emplace_back("qux");
    AST::Component::DefValType DefValTy;
    DefValTy.setEnum(std::move(Enum));
    const ComponentValType Baz = Mint.defValType(std::move(DefValTy));
    exportType("baz", Baz.getTypeIndex());
    exportType("foo", Runtime::Component::WitRecord::type(Mint, {{"bar", Baz}})
                          .getTypeIndex());
  }
  ~SpecTestTypesComponent() noexcept override = default;
};

/// The instance "not-provided-by-the-host2": an empty instance "x".
class SpecTestEmptyExportComponent
    : public Runtime::Instance::ComponentInstance {
public:
  SpecTestEmptyExportComponent()
      : ComponentInstance("not-provided-by-the-host2") {
    exportComponentInstance(
        "x", addComponentInstance(std::make_unique<ComponentInstance>("")));
  }
  ~SpecTestEmptyExportComponent() noexcept override = default;
};

/// The owner of the functions the suites import by name: "host-return-two"
/// and "f".
class SpecTestFuncsComponent : public Runtime::Instance::ComponentInstance {
public:
  SpecTestFuncsComponent() : ComponentInstance("") {
    addHostFunc("host-return-two", std::make_unique<SpecTestReturnU32>(2));
    addHostFunc("f", std::make_unique<SpecTestNop>());
  }
  ~SpecTestFuncsComponent() noexcept override = default;
};

} // namespace WasmEdge
