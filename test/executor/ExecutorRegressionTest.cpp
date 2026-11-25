// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/executor/ExecutorRegressionTest.cpp - Regression ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Regression and miscellaneous tests for the executor.
///
//===----------------------------------------------------------------------===//

#include "common/nan.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "experimental/bit.hpp"
#include "vm/vm.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <initializer_list>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {

using namespace WasmEdge;

/// Host function that records i32 values for later inspection.
class Check : public Runtime::HostFunction<Check> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t Value) {
    Values.push_back(Value);
    return {};
  }
  Span<const uint32_t> getValues() const noexcept { return Values; }

private:
  std::vector<uint32_t> Values;
};

/// Module that provides a "check" host function under the "gc" namespace.
class GCCheckModule : public Runtime::Instance::ModuleInstance {
public:
  GCCheckModule() : ModuleInstance("gc") {
    auto CP = std::make_unique<Check>();
    C = CP.get();
    addHostFunc("check", std::move(CP));
  }
  Span<const uint32_t> getValues() const noexcept { return C->getValues(); }

private:
  Check *C = nullptr;
};

/// Host function that records its i32 argument and returns it incremented, so a
/// caller can verify both the argument passed in and the value returned.
class AddOne : public Runtime::HostFunction<AddOne> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t Value) {
    Args.push_back(Value);
    return Value + 1;
  }
  Span<const uint32_t> getArgs() const noexcept { return Args; }

private:
  std::vector<uint32_t> Args;
};

/// Module that provides the "add_one" host function under the "host" namespace.
class TailCallHostModule : public Runtime::Instance::ModuleInstance {
public:
  TailCallHostModule() : ModuleInstance("host") {
    auto FP = std::make_unique<AddOne>();
    F = FP.get();
    addHostFunc("add_one", std::move(FP));
  }
  Span<const uint32_t> getArgs() const noexcept { return F->getArgs(); }

private:
  AddOne *F = nullptr;
};

/// Host function that does nothing.
class Nop : public Runtime::HostFunction<Nop> {
public:
  Expect<void> body(const Runtime::CallingFrame &) { return {}; }
};

/// After instantiation, read the exported table "t" at index 0 and verify the
/// null reference has been normalized to an abstract heap type. This catches
/// the root cause of #4757 without executing wasm that would segfault if the
/// fix is absent.
bool checkNullTableEntry(VM::VM &VMInst) {
  const auto *ModInst = VMInst.getActiveModule();
  if (!ModInst) {
    return false;
  }
  auto *TabInst = ModInst->findTableExports("t");
  if (!TabInst) {
    return false;
  }
  auto RefRes = TabInst->getRefAddr(0);
  if (!RefRes) {
    return false;
  }
  auto Ref = *RefRes;
  EXPECT_TRUE(Ref.isNull());
  EXPECT_TRUE(Ref.getType().isAbsHeapType())
      << "Null reference in concrete-typed table must be normalized to an "
         "abstract heap type (issue #4757)";
  return Ref.isNull() && Ref.getType().isAbsHeapType();
}

// clang-format off
/// Binary Wasm module with functions testing ref.test on externalized refs.
///
/// (module
///   (type (;0;) (func (param anyref) (result i32)))
///   (type (;1;) (func (param externref) (result i32)))
///
///   ;; Func 0: externalize anyref, then ref.test (ref null extern)
///   (func (;0;) (type 0) (param anyref) (result i32)
///     local.get 0
///     extern.convert_any
///     ref.test (ref null extern)
///   )
///
///   ;; Func 1: externalize anyref, then ref.test (ref extern)
///   (func (;1;) (type 0) (param anyref) (result i32)
///     local.get 0
///     extern.convert_any
///     ref.test (ref extern)
///   )
///
///   ;; Func 2: internalize externref, externalize, ref.test (ref null extern)
///   (func (;2;) (type 1) (param externref) (result i32)
///     local.get 0
///     any.convert_extern
///     extern.convert_any
///     ref.test (ref null extern)
///   )
///
///   ;; Func 3: internalize externref, externalize, ref.test (ref extern)
///   (func (;3;) (type 1) (param externref) (result i32)
///     local.get 0
///     any.convert_extern
///     extern.convert_any
///     ref.test (ref extern)
///   )
///
///   (export "test_nullable" (func 0))
///   (export "test_nonnull" (func 1))
///   (export "test_ext_nullable" (func 2))
///   (export "test_ext_nonnull" (func 3))
/// )
std::array<WasmEdge::Byte, 148> RefTestNullabilityWasm{
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0B, 0x02, 0x60,
    0x01, 0x6E, 0x01, 0x7F, 0x60, 0x01, 0x6F, 0x01, 0x7F, 0x03, 0x05, 0x04,
    0x00, 0x00, 0x01, 0x01, 0x07, 0x47, 0x04, 0x0D, 0x74, 0x65, 0x73, 0x74,
    0x5F, 0x6E, 0x75, 0x6C, 0x6C, 0x61, 0x62, 0x6C, 0x65, 0x00, 0x00, 0x0C,
    0x74, 0x65, 0x73, 0x74, 0x5F, 0x6E, 0x6F, 0x6E, 0x6E, 0x75, 0x6C, 0x6C,
    0x00, 0x01, 0x11, 0x74, 0x65, 0x73, 0x74, 0x5F, 0x65, 0x78, 0x74, 0x5F,
    0x6E, 0x75, 0x6C, 0x6C, 0x61, 0x62, 0x6C, 0x65, 0x00, 0x02, 0x10, 0x74,
    0x65, 0x73, 0x74, 0x5F, 0x65, 0x78, 0x74, 0x5F, 0x6E, 0x6F, 0x6E, 0x6E,
    0x75, 0x6C, 0x6C, 0x00, 0x03, 0x0A, 0x2D, 0x04, 0x09, 0x00, 0x20, 0x00,
    0xFB, 0x1B, 0xFB, 0x15, 0x6F, 0x0B, 0x09, 0x00, 0x20, 0x00, 0xFB, 0x1B,
    0xFB, 0x14, 0x6F, 0x0B, 0x0B, 0x00, 0x20, 0x00, 0xFB, 0x1A, 0xFB, 0x1B,
    0xFB, 0x15, 0x6F, 0x0B, 0x0B, 0x00, 0x20, 0x00, 0xFB, 0x1A, 0xFB, 0x1B,
    0xFB, 0x14, 0x6F, 0x0B};

/// Binary Wasm module: ref.test on null ref from concrete-typed table.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table (export "t") 1 (ref null $s))
///   (func (export "test")
///     i32.const 0
///     table.get 0
///     ref.test eqref
///     call $check
///     i32.const 0
///     table.get 0
///     ref.test (ref $s)
///     call $check))
std::array<WasmEdge::Byte, 106> NullRefTestWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x04, 0x05, 0x01, 0x63, 0x00, 0x00, 0x01, 0x07, 0x0c, 0x02,
    0x01, 0x74, 0x01, 0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a,
    0x16, 0x01, 0x14, 0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x15, 0x6d, 0x10,
    0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b, 0x00,
    0x15, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05, 0x63,
    0x68, 0x65, 0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: function returning null ref with concrete type,
/// then ref.test.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table (export "t") 1 (ref null $s))
///   (func $get_null (result (ref null $s))
///     i32.const 0
///     table.get 0)
///   (func (export "test")
///     call $get_null
///     ref.test eqref
///     call $check))
std::array<WasmEdge::Byte, 118> NullReturnConcreteWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x04, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x63, 0x00, 0x60, 0x00,
    0x00, 0x02, 0x0c, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63,
    0x6b, 0x00, 0x01, 0x03, 0x03, 0x02, 0x02, 0x03, 0x04, 0x05, 0x01, 0x63,
    0x00, 0x00, 0x01, 0x07, 0x0c, 0x02, 0x01, 0x74, 0x01, 0x00, 0x04, 0x74,
    0x65, 0x73, 0x74, 0x00, 0x02, 0x0a, 0x12, 0x02, 0x06, 0x00, 0x41, 0x00,
    0x25, 0x00, 0x0b, 0x09, 0x00, 0x10, 0x01, 0xfb, 0x15, 0x6d, 0x10, 0x00,
    0x0b, 0x00, 0x1f, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x12, 0x02, 0x00,
    0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x01, 0x08, 0x67, 0x65, 0x74, 0x5f,
    0x6e, 0x75, 0x6c, 0x6c, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: table.grow creates null entries, then ref.cast.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table $t (export "t") 0 (ref null $s))
///   (func (export "test")
///     ref.null $s
///     i32.const 1
///     table.grow $t
///     drop
///     i32.const 0
///     table.get $t
///     ref.cast eqref
///     ref.is_null
///     call $check))
std::array<WasmEdge::Byte, 112> NullTableGrowWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x04, 0x05, 0x01, 0x63, 0x00, 0x00, 0x00, 0x07, 0x0c, 0x02,
    0x01, 0x74, 0x01, 0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a,
    0x16, 0x01, 0x14, 0x00, 0xd0, 0x00, 0x41, 0x01, 0xfc, 0x0f, 0x00, 0x1a,
    0x41, 0x00, 0x25, 0x00, 0xfb, 0x17, 0x6d, 0xd1, 0x10, 0x00, 0x0b, 0x00,
    0x1b, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05, 0x63,
    0x68, 0x65, 0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73, 0x05, 0x04,
    0x01, 0x00, 0x01, 0x74};

/// Binary Wasm module: ref.cast eqref on null ref from concrete-typed table.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table (export "t") 1 (ref null $s))
///   (func (export "test")
///     i32.const 0
///     table.get 0
///     ref.cast eqref
///     ref.is_null
///     call $check))
std::array<WasmEdge::Byte, 98> NullRefCastWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03,
    0x5f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c,
    0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00,
    0x01, 0x03, 0x02, 0x01, 0x02, 0x04, 0x05, 0x01, 0x63, 0x00, 0x00,
    0x01, 0x07, 0x0c, 0x02, 0x01, 0x74, 0x01, 0x00, 0x04, 0x74, 0x65,
    0x73, 0x74, 0x00, 0x01, 0x0a, 0x0e, 0x01, 0x0c, 0x00, 0x41, 0x00,
    0x25, 0x00, 0xfb, 0x17, 0x6d, 0xd1, 0x10, 0x00, 0x0b, 0x00, 0x15,
    0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05, 0x63,
    0x68, 0x65, 0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: ref.cast (ref eq) on null -- non-nullable cast
/// should trap.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table (export "t") 1 (ref null $s))
///   (func (export "test")
///     i32.const 0
///     table.get 0
///     ref.cast (ref eq)
///     drop))
std::array<WasmEdge::Byte, 96> NullRefCastNonNullWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x04, 0x05, 0x01, 0x63, 0x00, 0x00, 0x01, 0x07, 0x0c, 0x02,
    0x01, 0x74, 0x01, 0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a,
    0x0c, 0x01, 0x0a, 0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x16, 0x6d, 0x1a,
    0x0b, 0x00, 0x15, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00,
    0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: br_on_cast / br_on_cast_fail with null ref from
/// concrete-typed table.
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (table (export "t") 1 (ref null $s))
///   (func (export "test")
///     (block $taken (result eqref)
///       i32.const 0
///       table.get 0
///       br_on_cast $taken eqref eqref
///       drop
///       i32.const 0
///       call $check
///       return)
///     ref.is_null
///     call $check
///     (block $fail_taken (result eqref)
///       i32.const 0
///       table.get 0
///       br_on_cast_fail $fail_taken eqref (ref eq)
///       drop
///       i32.const 0
///       call $check
///       return)
///     ref.is_null
///     call $check))
std::array<WasmEdge::Byte, 156> NullBrOnCastWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x04, 0x05, 0x01, 0x63, 0x00, 0x00, 0x01, 0x07, 0x0c, 0x02,
    0x01, 0x74, 0x01, 0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a,
    0x30, 0x01, 0x2e, 0x00, 0x02, 0x6d, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x18,
    0x03, 0x00, 0x6d, 0x6d, 0x1a, 0x41, 0x00, 0x10, 0x00, 0x0f, 0x0b, 0xd1,
    0x10, 0x00, 0x02, 0x6d, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x19, 0x01, 0x00,
    0x6d, 0x6d, 0x1a, 0x41, 0x00, 0x10, 0x00, 0x0f, 0x0b, 0xd1, 0x10, 0x00,
    0x0b, 0x00, 0x2d, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00,
    0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x03, 0x16, 0x01, 0x01, 0x02, 0x00,
    0x05, 0x74, 0x61, 0x6b, 0x65, 0x6e, 0x01, 0x0a, 0x66, 0x61, 0x69, 0x6c,
    0x5f, 0x74, 0x61, 0x6b, 0x65, 0x6e, 0x04, 0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: ref.test on default-initialized local with concrete
/// struct ref type (issue #4768).
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (func (export "test")
///     (local (ref null $s))
///     local.get 0
///     ref.test (ref null $s)
///     call $check
///     local.get 0
///     ref.test eqref
///     call $check
///     local.get 0
///     ref.test (ref $s)
///     call $check))
std::array<WasmEdge::Byte, 101> NullLocalStructWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01,
    0x0a, 0x1c, 0x01, 0x1a, 0x01, 0x01, 0x63, 0x00, 0x20, 0x00, 0xfb, 0x15,
    0x00, 0x10, 0x00, 0x20, 0x00, 0xfb, 0x15, 0x6d, 0x10, 0x00, 0x20, 0x00,
    0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x15, 0x04, 0x6e, 0x61, 0x6d,
    0x65, 0x01, 0x08, 0x01, 0x00, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x04,
    0x04, 0x01, 0x00, 0x01, 0x73};

/// Binary Wasm module: ref.test on default-initialized local with concrete
/// func ref type (issue #4768).
///
/// (module
///   (type $f (func))
///   (import "gc" "check" (func $check (param i32)))
///   (func (export "test")
///     (local (ref null $f))
///     local.get 0
///     ref.test (ref null func)
///     call $check
///     local.get 0
///     ref.test (ref $f)
///     call $check))
std::array<WasmEdge::Byte, 92> NullLocalFuncWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
    0x00, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x0c, 0x01, 0x02, 0x67, 0x63,
    0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02, 0x01, 0x00,
    0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a, 0x15,
    0x01, 0x13, 0x01, 0x01, 0x63, 0x00, 0x20, 0x00, 0xfb, 0x15, 0x70, 0x10,
    0x00, 0x20, 0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x15, 0x04,
    0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05, 0x63, 0x68, 0x65,
    0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x66};

/// Binary Wasm module: ref.test on default-initialized local with concrete
/// array ref type (issue #4768).
///
/// (module
///   (type $a (array i32))
///   (import "gc" "check" (func $check (param i32)))
///   (func (export "test")
///     (local (ref null $a))
///     local.get 0
///     ref.test eqref
///     call $check
///     local.get 0
///     ref.test (ref $a)
///     call $check))
std::array<WasmEdge::Byte, 95> NullLocalArrayWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x03, 0x5e,
    0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01,
    0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03,
    0x02, 0x01, 0x02, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00,
    0x01, 0x0a, 0x15, 0x01, 0x13, 0x01, 0x01, 0x63, 0x00, 0x20, 0x00, 0xfb,
    0x15, 0x6d, 0x10, 0x00, 0x20, 0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b,
    0x00, 0x15, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x08, 0x01, 0x00, 0x05,
    0x63, 0x68, 0x65, 0x63, 0x6b, 0x04, 0x04, 0x01, 0x00, 0x01, 0x61};

/// Binary Wasm module: ref.test on null global with concrete struct ref type
/// (issue #4768 — verifies globals are safe via ref.null init expression).
///
/// (module
///   (type $s (struct))
///   (import "gc" "check" (func $check (param i32)))
///   (global $g (ref null $s) ref.null $s)
///   (func (export "test")
///     global.get $g
///     ref.test eqref
///     call $check
///     global.get $g
///     ref.test (ref $s)
///     call $check))
std::array<WasmEdge::Byte, 106> NullGlobalStructWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x03, 0x5f,
    0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x02, 0x0c, 0x01, 0x02,
    0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00, 0x01, 0x03, 0x02,
    0x01, 0x02, 0x06, 0x07, 0x01, 0x63, 0x00, 0x00, 0xd0, 0x00, 0x0b, 0x07,
    0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x01, 0x0a, 0x12, 0x01,
    0x10, 0x00, 0x23, 0x00, 0xfb, 0x15, 0x6d, 0x10, 0x00, 0x23, 0x00, 0xfb,
    0x14, 0x00, 0x10, 0x00, 0x0b, 0x00, 0x1b, 0x04, 0x6e, 0x61, 0x6d, 0x65,
    0x01, 0x08, 0x01, 0x00, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x04, 0x04,
    0x01, 0x00, 0x01, 0x73, 0x07, 0x04, 0x01, 0x00, 0x01, 0x67};

/// Binary Wasm module: ref.test and ref.cast on default-initialized locals
/// with abstract ref types (issue #5343).
///
/// (module
///   (type $t (struct (field (mut structref))))
///   (type $a (array i32))
///   (type $f (func))
///   (type $r (func (result i32)))
///   (func (export "test_struct") (type $r) (local structref)
///     local.get 0 ref.test (ref null $t))
///   (func (export "test_struct_nonnull") (type $r) (local structref)
///     local.get 0 ref.test (ref $t))
///   (func (export "cast_struct") (type $r) (local structref)
///     local.get 0 ref.cast (ref null $t) drop i32.const 7)
///   (func (export "test_any") (type $r) (local anyref)
///     local.get 0 ref.test (ref null $t))
///   (func (export "test_i31") (type $r) (local i31ref)
///     local.get 0 ref.test (ref null $t))
///   (func (export "test_array") (type $r) (local arrayref)
///     local.get 0 ref.test (ref null $a))
///   (func (export "test_func") (type $r) (local funcref)
///     local.get 0 ref.test (ref null $f))
///   (func (export "test_func_nonnull") (type $r) (local funcref)
///     local.get 0 ref.test (ref $f))
///   (func (export "cast_func") (type $r) (local funcref)
///     local.get 0 ref.cast (ref null $f) drop i32.const 7)
///   (func (export "test_extern") (type $r) (local externref)
///     local.get 0 ref.test (ref null noextern)))
std::array<WasmEdge::Byte, 294> NullLocalAbstractWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x04, 0x5f,
    0x01, 0x6b, 0x01, 0x5e, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01,
    0x7f, 0x03, 0x0b, 0x0a, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x07, 0x90, 0x01, 0x0a, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x5f,
    0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x00, 0x13, 0x74, 0x65, 0x73,
    0x74, 0x5f, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x5f, 0x6e, 0x6f, 0x6e,
    0x6e, 0x75, 0x6c, 0x6c, 0x00, 0x01, 0x0b, 0x63, 0x61, 0x73, 0x74, 0x5f,
    0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x02, 0x08, 0x74, 0x65, 0x73,
    0x74, 0x5f, 0x61, 0x6e, 0x79, 0x00, 0x03, 0x08, 0x74, 0x65, 0x73, 0x74,
    0x5f, 0x69, 0x33, 0x31, 0x00, 0x04, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f,
    0x61, 0x72, 0x72, 0x61, 0x79, 0x00, 0x05, 0x09, 0x74, 0x65, 0x73, 0x74,
    0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x06, 0x11, 0x74, 0x65, 0x73, 0x74,
    0x5f, 0x66, 0x75, 0x6e, 0x63, 0x5f, 0x6e, 0x6f, 0x6e, 0x6e, 0x75, 0x6c,
    0x6c, 0x00, 0x07, 0x09, 0x63, 0x61, 0x73, 0x74, 0x5f, 0x66, 0x75, 0x6e,
    0x63, 0x00, 0x08, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x65, 0x78, 0x74,
    0x65, 0x72, 0x6e, 0x00, 0x09, 0x0a, 0x6b, 0x0a, 0x09, 0x01, 0x01, 0x6b,
    0x20, 0x00, 0xfb, 0x15, 0x00, 0x0b, 0x09, 0x01, 0x01, 0x6b, 0x20, 0x00,
    0xfb, 0x14, 0x00, 0x0b, 0x0c, 0x01, 0x01, 0x6b, 0x20, 0x00, 0xfb, 0x17,
    0x00, 0x1a, 0x41, 0x07, 0x0b, 0x09, 0x01, 0x01, 0x6e, 0x20, 0x00, 0xfb,
    0x15, 0x00, 0x0b, 0x09, 0x01, 0x01, 0x6c, 0x20, 0x00, 0xfb, 0x15, 0x00,
    0x0b, 0x09, 0x01, 0x01, 0x6a, 0x20, 0x00, 0xfb, 0x15, 0x01, 0x0b, 0x09,
    0x01, 0x01, 0x70, 0x20, 0x00, 0xfb, 0x15, 0x02, 0x0b, 0x09, 0x01, 0x01,
    0x70, 0x20, 0x00, 0xfb, 0x14, 0x02, 0x0b, 0x0c, 0x01, 0x01, 0x70, 0x20,
    0x00, 0xfb, 0x17, 0x02, 0x1a, 0x41, 0x07, 0x0b, 0x09, 0x01, 0x01, 0x6f,
    0x20, 0x00, 0xfb, 0x15, 0x72, 0x0b};

/// Binary Wasm module: throw_ref with null (ref null $point) payload;
/// struct.get on the surfaced value must trap on the null reference.
///
/// (module
///   (type $point (struct (field i32)))
///   (tag $e (param (ref null $point)))
///   (func (export "main") (result i32)
///     (local $exn exnref)
///     block $h (result (ref null $point) exnref)
///       try_table (catch_ref $e $h)
///         ref.null $point
///         throw $e
///       end
///       unreachable
///     end
///     local.set $exn
///     drop
///     block $h2 (result (ref null $point))
///       try_table (catch $e $h2)
///         local.get $exn
///         throw_ref
///       end
///       unreachable
///     end
///     struct.get $point 0))
std::array<WasmEdge::Byte, 139> ThrowRefNullPayloadWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x14, 0x04, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x01, 0x63, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f,
    0x60, 0x00, 0x02, 0x63, 0x00, 0x69, 0x03, 0x02, 0x01, 0x02, 0x0d, 0x03,
    0x01, 0x00, 0x01, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x69, 0x6e, 0x00,
    0x00, 0x0a, 0x2b, 0x01, 0x29, 0x01, 0x01, 0x69, 0x02, 0x03, 0x1f, 0x40,
    0x01, 0x01, 0x00, 0x00, 0xd0, 0x00, 0x08, 0x00, 0x0b, 0x00, 0x0b, 0x21,
    0x00, 0x1a, 0x02, 0x63, 0x00, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x20,
    0x00, 0x0a, 0x0b, 0x00, 0x0b, 0xfb, 0x02, 0x00, 0x00, 0x0b, 0x00, 0x2b,
    0x04, 0x6e, 0x61, 0x6d, 0x65, 0x02, 0x08, 0x01, 0x00, 0x01, 0x00, 0x03,
    0x65, 0x78, 0x6e, 0x03, 0x0a, 0x01, 0x00, 0x02, 0x00, 0x01, 0x68, 0x02,
    0x02, 0x68, 0x32, 0x04, 0x08, 0x01, 0x00, 0x05, 0x70, 0x6f, 0x69, 0x6e,
    0x74, 0x0b, 0x04, 0x01, 0x00, 0x01, 0x65};

/// Binary Wasm module: multi-param tag (i32 i64) round-tripped through
/// catch_ref + throw_ref + catch -- final result is 7 + 1000 == 1007.
///
/// (module
///   (tag $err (param i32 i64))
///   (func (export "main") (result i32)
///     (local $exn exnref)
///     block $h1 (result i32 i64 exnref)
///       try_table (catch_ref $err $h1)
///         i32.const 7
///         i64.const 1000
///         throw $err
///       end
///       unreachable
///     end
///     local.set $exn
///     drop
///     drop
///     block $h2 (result i32 i64)
///       try_table (catch $err $h2)
///         local.get $exn
///         throw_ref
///       end
///       unreachable
///     end
///     i32.wrap_i64
///     i32.add))
std::array<WasmEdge::Byte, 134> ThrowRefMultiParamWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x15, 0x04, 0x60,
    0x02, 0x7f, 0x7e, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x00, 0x03, 0x7f,
    0x7e, 0x69, 0x60, 0x00, 0x02, 0x7f, 0x7e, 0x03, 0x02, 0x01, 0x01, 0x0d,
    0x03, 0x01, 0x00, 0x00, 0x07, 0x08, 0x01, 0x04, 0x6d, 0x61, 0x69, 0x6e,
    0x00, 0x00, 0x0a, 0x2c, 0x01, 0x2a, 0x01, 0x01, 0x69, 0x02, 0x02, 0x1f,
    0x40, 0x01, 0x01, 0x00, 0x00, 0x41, 0x07, 0x42, 0xe8, 0x07, 0x08, 0x00,
    0x0b, 0x00, 0x0b, 0x21, 0x00, 0x1a, 0x1a, 0x02, 0x03, 0x1f, 0x40, 0x01,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x0a, 0x0b, 0x00, 0x0b, 0xa7, 0x6a, 0x0b,
    0x00, 0x24, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x02, 0x08, 0x01, 0x00, 0x01,
    0x00, 0x03, 0x65, 0x78, 0x6e, 0x03, 0x0b, 0x01, 0x00, 0x02, 0x00, 0x02,
    0x68, 0x31, 0x02, 0x02, 0x68, 0x32, 0x0b, 0x06, 0x01, 0x00, 0x03, 0x65,
    0x72, 0x72};

/// Binary Wasm module: call_indirect on a table whose element type is a
/// concrete struct reference, not a funcref subtype.
///
/// The table is `(ref null $s)` where `$s` is a struct type, so it is NOT a
/// subtype of funcref and call_indirect on it must be rejected at validation.
/// The old check used `ValType::isFuncRefType()`, which reported *any* concrete
/// type index as a funcref. The module therefore validated, and at runtime the
/// struct reference was reinterpreted as a `FunctionInstance *` -- a type
/// confusion in which the attacker-controlled v128 field bytes (here 0x41.. /
/// 0x49) stand in for function metadata. The fix validates the table type with
/// `TypeMatcher::matchType(.., FuncRef, ..)`, which resolves the concrete type
/// index and rejects the struct-typed table.
///
/// (module
///   (type $f (func))
///   (type $s (struct (field v128)))
///   (table 1 (ref null $s))
///   (func (export "_start")
///     i32.const 0
///     v128.const i32x4 0x41414141 0x41414141 0x41414149 0x41414141
///     struct.new $s
///     table.set 0
///     i32.const 0
///     call_indirect (type $f)))
std::array<WasmEdge::Byte, 77> CallIndirectStructTableWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x60,
    0x00, 0x00, 0x5f, 0x01, 0x7b, 0x00, 0x03, 0x02, 0x01, 0x00, 0x04, 0x05,
    0x01, 0x63, 0x01, 0x00, 0x01, 0x07, 0x0a, 0x01, 0x06, 0x5f, 0x73, 0x74,
    0x61, 0x72, 0x74, 0x00, 0x00, 0x0a, 0x22, 0x01, 0x20, 0x00, 0x41, 0x00,
    0xfd, 0x0c, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x49, 0x41,
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0xfb, 0x00, 0x01, 0x26, 0x00, 0x41,
    0x00, 0x11, 0x00, 0x00, 0x0b};
/// Binary Wasm module: catch_all must not leak the throw payload.
///
/// (module
///   (tag $e (param i32))
///   (func (export "test") (result i32)
///     (i32.const 1)
///     (block $L
///       (try_table (catch_all $L)
///         (i32.const 42)
///         (throw $e))
///       unreachable)))
std::array<WasmEdge::Byte, 60> CatchAllPayloadNotLeakedWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x01, 0x0d,
    0x03, 0x01, 0x00, 0x00, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74,
    0x00, 0x00, 0x0a, 0x14, 0x01, 0x12, 0x00, 0x41, 0x01, 0x02, 0x40, 0x1f,
    0x40, 0x01, 0x02, 0x00, 0x41, 0x2a, 0x08, 0x00, 0x0b, 0x00, 0x0b, 0x0b};

/// Binary Wasm module: catch_all_ref must not leak the throw payload alongside
/// the exnref.
///
/// (module
///   (tag $e (param i32))
///   (func (export "test") (result i32)
///     (i32.const 1)
///     (block $L (result exnref)
///       (try_table (catch_all_ref $L)
///         (i32.const 42)
///         (throw $e))
///       unreachable)
///     drop))
std::array<WasmEdge::Byte, 61> CatchAllRefPayloadNotLeakedWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60,
    0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x01, 0x0d,
    0x03, 0x01, 0x00, 0x00, 0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74,
    0x00, 0x00, 0x0a, 0x15, 0x01, 0x13, 0x00, 0x41, 0x01, 0x02, 0x69, 0x1f,
    0x40, 0x01, 0x03, 0x00, 0x41, 0x2a, 0x08, 0x00, 0x0b, 0x00, 0x0b, 0x1a,
    0x0b};

/// Binary Wasm module: a try_table handler left inactive by a br and then
/// buried under a newer try_table.
///
/// (module
///   (tag $tx)
///   (tag $ty (param i32 i32))
///   (func (export "go")
///     (block $outer
///       ;; raise the stack so try_table $A records a high VPos
///       (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1)
///       (try_table $A (catch $tx $outer)
///         (br $outer))              ;; leave $A without running its end: stale
///       unreachable)
///     ;; the stack is low again; try_table $B buries the stale handler $A
///     (try_table $B (catch $tx 0)
///       (i32.const 7) (i32.const 8)
///       (throw $ty))))              ;; uncaught: search reaches stale $A
std::array<WasmEdge::Byte, 78> BuriedStaleHandlerWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60,
    0x00, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x03, 0x02, 0x01, 0x00, 0x0d,
    0x05, 0x02, 0x00, 0x00, 0x00, 0x01, 0x07, 0x06, 0x01, 0x02, 0x67, 0x6f,
    0x00, 0x00, 0x0a, 0x26, 0x01, 0x24, 0x00, 0x02, 0x40, 0x41, 0x01, 0x41,
    0x01, 0x41, 0x01, 0x41, 0x01, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x0c,
    0x01, 0x0b, 0x00, 0x0b, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x41, 0x07,
    0x41, 0x08, 0x08, 0x01, 0x0b, 0x0b};

/// Binary Wasm module: a loop that leaves a stale handler behind on every
/// iteration, so the stale handlers pile up under one another before a newer
/// try_table buries them.
///
/// (module
///   (tag $tx)
///   (tag $ty (param i32 i32))
///   (func (export "go") (local $i i32)
///     (loop $L
///       (block $out
///         ;; raise the stack so try_table $A records a high VPos
///         (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1)
///         (try_table $A (catch $tx $out)
///           (br $out))            ;; leave $A without running its end: stale
///         unreachable)
///       ;; every iteration strands another handler, all sharing one Try
///       (local.set $i (i32.add (local.get $i) (i32.const 1)))
///       (br_if $L (i32.lt_u (local.get $i) (i32.const 3))))
///     (try_table $B (catch $tx 0)
///       (i32.const 7) (i32.const 8)
///       (throw $ty))))            ;; uncaught: search reaches the stale pile
std::array<WasmEdge::Byte, 97> StaleHandlerPileWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60,
    0x00, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x03, 0x02, 0x01, 0x00, 0x0d,
    0x05, 0x02, 0x00, 0x00, 0x00, 0x01, 0x07, 0x06, 0x01, 0x02, 0x67, 0x6f,
    0x00, 0x00, 0x0a, 0x39, 0x01, 0x37, 0x01, 0x01, 0x7f, 0x03, 0x40, 0x02,
    0x40, 0x41, 0x01, 0x41, 0x01, 0x41, 0x01, 0x41, 0x01, 0x1f, 0x40, 0x01,
    0x00, 0x00, 0x00, 0x0c, 0x01, 0x0b, 0x00, 0x0b, 0x20, 0x00, 0x41, 0x01,
    0x6a, 0x21, 0x00, 0x20, 0x00, 0x41, 0x03, 0x49, 0x0d, 0x00, 0x0b, 0x1f,
    0x40, 0x01, 0x00, 0x00, 0x00, 0x41, 0x07, 0x41, 0x08, 0x08, 0x01, 0x0b,
    0x0b};
/// Binary Wasm module: a wasm function tail-calls an imported host function.
/// $f2 has more params than the callee (the crashing shape) and $f1 has equal
/// arity (the hanging shape); both return_call the host "add_one", whose
/// result must propagate back to the exported caller.
///
/// (module
///   (import "host" "add_one" (func $h (param i32) (result i32)))
///   (func $f2 (param i32 i32) (result i32)
///     local.get 0
///     return_call $h)
///   (func $f1 (param i32) (result i32)
///     local.get 0
///     return_call $h)
///   (func (export "run_more") (result i32)
///     i32.const 41
///     i32.const 99
///     call $f2)
///   (func (export "run_eq") (result i32)
///     i32.const 7
///     call $f1))
std::array<WasmEdge::Byte, 129> TailCallHostImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x03, 0x60,
    0x01, 0x7f, 0x01, 0x7f, 0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x00,
    0x01, 0x7f, 0x02, 0x10, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x07, 0x61,
    0x64, 0x64, 0x5f, 0x6f, 0x6e, 0x65, 0x00, 0x00, 0x03, 0x05, 0x04, 0x01,
    0x00, 0x02, 0x02, 0x07, 0x15, 0x02, 0x08, 0x72, 0x75, 0x6e, 0x5f, 0x6d,
    0x6f, 0x72, 0x65, 0x00, 0x03, 0x06, 0x72, 0x75, 0x6e, 0x5f, 0x65, 0x71,
    0x00, 0x04, 0x0a, 0x20, 0x04, 0x06, 0x00, 0x20, 0x00, 0x12, 0x00, 0x0b,
    0x06, 0x00, 0x20, 0x00, 0x12, 0x00, 0x0b, 0x09, 0x00, 0x41, 0x29, 0x41,
    0xe3, 0x00, 0x10, 0x01, 0x0b, 0x06, 0x00, 0x41, 0x07, 0x10, 0x02, 0x0b,
    0x00, 0x13, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x0c, 0x03, 0x00, 0x01,
    0x68, 0x01, 0x02, 0x66, 0x32, 0x02, 0x02, 0x66, 0x31};

/// Binary Wasm module: Memory64 + Threads.
///
/// (module
///   (memory $m i64 1 1 shared)
///   (func (export "test")
///     (memory.atomic.notify (i64.const 4) (i32.const 1))
///     drop
///   )
/// )
std::array<WasmEdge::Byte, 49> NotifyMemory64Wasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
    0x00, 0x00, 0x03, 0x02, 0x01, 0x00, 0x05, 0x04, 0x01, 0x07, 0x01, 0x01,
    0x07, 0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x0a, 0x0d,
    0x01, 0x0b, 0x00, 0x42, 0x04, 0x41, 0x01, 0xfe, 0x00, 0x02, 0x00, 0x1a,
    0x0b};

/// Self-recursive f(n) = n ? f(n-1) : 0 (no tail-call); recursing deeper than
/// the configured MaxStackSize must trap with CallStackExhausted, not complete.
std::array<WasmEdge::Byte, 70> RecurseWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
    0x01, 0x7e, 0x01, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01,
    0x66, 0x00, 0x00, 0x0a, 0x14, 0x01, 0x12, 0x00, 0x20, 0x00, 0x50, 0x04,
    0x7e, 0x42, 0x00, 0x05, 0x20, 0x00, 0x42, 0x01, 0x7d, 0x10, 0x00, 0x0b,
    0x0b, 0x00, 0x13, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x04, 0x01, 0x00,
    0x01, 0x66, 0x02, 0x06, 0x01, 0x00, 0x01, 0x00, 0x01, 0x6e};

/// Binary Wasm module: exports a tag whose type has a super type.
///
/// (module
///   (type $super (sub (func)))
///   (type $sub (sub $super (func)))
///   (tag (export "t") (type $sub)))
std::array<WasmEdge::Byte, 34> TagSubTypeExportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02, 0x50,
    0x00, 0x60, 0x00, 0x00, 0x50, 0x01, 0x00, 0x60, 0x00, 0x00, 0x0d, 0x03,
    0x01, 0x00, 0x01, 0x07, 0x05, 0x01, 0x01, 0x74, 0x04, 0x00};

/// Binary Wasm module: imports the tag with its super type.
///
/// (module
///   (type $super (sub (func)))
///   (import "M" "t" (tag (type $super))))
std::array<WasmEdge::Byte, 26> TagSuperTypeImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x50,
    0x00, 0x60, 0x00, 0x00, 0x02, 0x08, 0x01, 0x01, 0x4d, 0x01, 0x74, 0x04,
    0x00, 0x00};

/// Binary Wasm module: imports the tag with an equivalent type.
///
/// (module
///   (type $super (sub (func)))
///   (type $sub (sub $super (func)))
///   (import "M" "t" (tag (type $sub))))
std::array<WasmEdge::Byte, 32> TagSubTypeImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02, 0x50,
    0x00, 0x60, 0x00, 0x00, 0x50, 0x01, 0x00, 0x60, 0x00, 0x00, 0x02, 0x08,
    0x01, 0x01, 0x4d, 0x01, 0x74, 0x04, 0x00, 0x01};

/// Binary Wasm module: defines and exports a global, a table, and a tag.
///
/// (module
///   (type $s (struct (field i32)))
///   (type $ft (func (param i32)))
///   (global (export "g") (ref null $s) (ref.null $s))
///   (table (export "t") 1 (ref null $s))
///   (tag (export "e") (type $ft)))
std::array<WasmEdge::Byte, 55> ReExportDefineWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x04, 0x05, 0x01, 0x63, 0x00,
    0x00, 0x01, 0x0d, 0x03, 0x01, 0x00, 0x01, 0x06, 0x07, 0x01, 0x63, 0x00,
    0x00, 0xd0, 0x00, 0x0b, 0x07, 0x0d, 0x03, 0x01, 0x67, 0x03, 0x00, 0x01,
    0x74, 0x01, 0x00, 0x01, 0x65, 0x04, 0x00};

/// Binary Wasm module: re-exports them with its type indices shifted by one.
///
/// (module
///   (type $f (func))
///   (type $s (struct (field i32)))
///   (type $ft (func (param i32)))
///   (import "A" "g" (global (ref null $s)))
///   (import "A" "t" (table 1 (ref null $s)))
///   (import "A" "e" (tag (type $ft)))
///   (export "g" (global 0))
///   (export "t" (table 0))
///   (export "e" (tag 0)))
std::array<WasmEdge::Byte, 64> ReExportForwardWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03, 0x60,
    0x00, 0x00, 0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x19,
    0x03, 0x01, 0x41, 0x01, 0x67, 0x03, 0x63, 0x01, 0x00, 0x01, 0x41, 0x01,
    0x74, 0x01, 0x63, 0x01, 0x00, 0x01, 0x01, 0x41, 0x01, 0x65, 0x04, 0x00,
    0x02, 0x07, 0x0d, 0x03, 0x01, 0x67, 0x03, 0x00, 0x01, 0x74, 0x01, 0x00,
    0x01, 0x65, 0x04, 0x00};

/// Binary Wasm module: imports the re-exported global.
///
/// (module
///   (type $s (struct (field i32)))
///   (type $ft (func (param i32)))
///   (import "B" "g" (global (ref null $s))))
std::array<WasmEdge::Byte, 30> ReExportGlobalImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x09, 0x01, 0x01, 0x42,
    0x01, 0x67, 0x03, 0x63, 0x00, 0x00};

/// Binary Wasm module: imports the re-exported table.
///
/// (module
///   (type $s (struct (field i32)))
///   (type $ft (func (param i32)))
///   (import "B" "t" (table 1 (ref null $s))))
std::array<WasmEdge::Byte, 31> ReExportTableImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x0a, 0x01, 0x01, 0x42,
    0x01, 0x74, 0x01, 0x63, 0x00, 0x00, 0x01};

/// Binary Wasm module: imports the re-exported tag.
///
/// (module
///   (type $s (struct (field i32)))
///   (type $ft (func (param i32)))
///   (import "B" "e" (tag (type $ft))))
std::array<WasmEdge::Byte, 29> ReExportTagImportWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x5f,
    0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x08, 0x01, 0x01, 0x42,
    0x01, 0x65, 0x04, 0x00, 0x01};

/// Binary Wasm module: calls and tests the function in its table.
///
/// (module
///   (type $f (func))
///   (type $g (sub (func)))
///   (table (export "tab") 1 funcref)
///   (func (export "call_f") (call_indirect (type $f) (i32.const 0)))
///   (func (export "call_g") (call_indirect (type $g) (i32.const 0)))
///   (func (export "test_f") (result i32)
///     (ref.test (ref $f) (table.get 0 (i32.const 0))))
///   (func (export "test_g") (result i32)
///     (ref.test (ref $g) (table.get 0 (i32.const 0)))))
std::array<WasmEdge::Byte, 120> HostFuncTableWasm{
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x60,
    0x00, 0x00, 0x50, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x03,
    0x05, 0x04, 0x00, 0x00, 0x02, 0x02, 0x04, 0x04, 0x01, 0x70, 0x00, 0x01,
    0x07, 0x2b, 0x05, 0x03, 0x74, 0x61, 0x62, 0x01, 0x00, 0x06, 0x63, 0x61,
    0x6c, 0x6c, 0x5f, 0x66, 0x00, 0x00, 0x06, 0x63, 0x61, 0x6c, 0x6c, 0x5f,
    0x67, 0x00, 0x01, 0x06, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x66, 0x00, 0x02,
    0x06, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x67, 0x00, 0x03, 0x0a, 0x25, 0x04,
    0x07, 0x00, 0x41, 0x00, 0x11, 0x00, 0x00, 0x0b, 0x07, 0x00, 0x41, 0x00,
    0x11, 0x01, 0x00, 0x0b, 0x09, 0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x14,
    0x00, 0x0b, 0x09, 0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x14, 0x01, 0x0b};
// clang-format on

/// Regression test for ref.test on externalized references.
///
/// This test exercises the externalized reference code path in ref.test via
/// the extern.convert_any + ref.test pattern, verifying correct behavior for
/// both nullable and non-nullable ref.test targets, as well as null and
/// non-null input values.
TEST(ExecutorRegression, RefTestNullAbility) {
  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(RefTestNullabilityWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // --- Part 1: Non-null anyref input ---
  //
  // When a non-null anyref is externalized via extern.convert_any, the value
  // stays as it is and is seen as an extern reference. The ref.test
  // instruction must match it against both nullable and non-nullable extern
  // targets.
  //
  // Note: The type of a reference value comes from the value, so a non-null
  // reference is non-nullable. Both ref.test targets should succeed.
  {
    int DummyObj = 42;
    RefVariant AnyRefVal(ValType(TypeCode::RefNull, TypeCode::AnyRef),
                         reinterpret_cast<void *>(&DummyObj));

    std::vector<ValVariant> Params = {ValVariant(AnyRefVal)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::AnyRef)};

    // Test 1: extern.convert_any + ref.test (ref null extern)
    // Non-null externalized ref with non-nullable type should match
    // nullable extern target.
    {
      auto Result = VM.execute("test_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Non-null externalized anyref should match (ref null extern)";
    }

    // Test 2: extern.convert_any + ref.test (ref extern)
    // Non-null externalized ref with non-nullable type should also match
    // non-nullable extern target.
    {
      auto Result = VM.execute("test_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Non-null externalized anyref should match (ref extern)";
    }
  }

  // --- Part 2: Null anyref input ---
  //
  // When a null anyref is externalized, extern.convert_any creates a new
  // RefVariant with type (ref null noextern).
  // A null value should match nullable target but not non-nullable.
  {
    RefVariant NullAnyRef(ValType(TypeCode::RefNull, TypeCode::NullRef));

    std::vector<ValVariant> Params = {ValVariant(NullAnyRef)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::AnyRef)};

    // Test 3: extern.convert_any on null + ref.test (ref null extern)
    // A null externalized value becomes (ref null noextern), which should
    // match (ref null extern) since noextern <: extern.
    {
      auto Result = VM.execute("test_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Null externalized anyref should match (ref null extern)";
    }

    // Test 4: extern.convert_any on null + ref.test (ref extern)
    // A null value should not match a non-nullable target since
    // (ref null noextern) is nullable.
    {
      auto Result = VM.execute("test_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 0U)
          << "Null externalized anyref should NOT match (ref extern)";
    }
  }

  // --- Part 3: Non-null externref round-trip ---
  //
  // In this path, any.convert_extern (internalize) and then
  // extern.convert_any (externalize) keep a non-null externref as it is.
  // Both nullable and non-nullable ref.test targets should match.
  {
    int DummyObj = 42;
    RefVariant ExternRefVal(&DummyObj);

    std::vector<ValVariant> Params = {ValVariant(ExternRefVal)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::ExternRef)};

    // Test 5: internalize + externalize + ref.test (ref null extern)
    // The value is seen as (ref extern), matching (ref null extern).
    {
      auto Result = VM.execute("test_ext_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Round-trip externalized externref should match (ref null extern)";
    }

    // Test 6: internalize + externalize + ref.test (ref extern)
    // Same path, seen as (ref extern) which matches (ref extern).
    {
      auto Result = VM.execute("test_ext_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Round-trip externalized externref should match (ref extern)";
    }
  }

  // --- Part 4: Null externref round-trip ---
  //
  // When a null externref is internalized, it becomes (ref null none).
  // When externalized, a null value becomes (ref null noextern).
  {
    RefVariant NullExternRef(
        ValType(TypeCode::RefNull, TypeCode::NullExternRef));

    std::vector<ValVariant> Params = {ValVariant(NullExternRef)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::ExternRef)};

    // Test 7: null externref round-trip + ref.test (ref null extern)
    // Should match since null noextern <: null extern.
    {
      auto Result = VM.execute("test_ext_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Null round-trip externref should match (ref null extern)";
    }

    // Test 8: null externref round-trip + ref.test (ref extern)
    // Should NOT match since null value cannot be non-nullable.
    {
      auto Result = VM.execute("test_ext_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 0U)
          << "Null round-trip externref should NOT match (ref extern)";
    }
  }

  // --- Part 5: ref.test on null from concrete-typed table (issue #4757) ---
  //
  // table.get on a concrete-typed table (e.g. `(table N (ref null $s))`)
  // returned null references that retained the concrete type. When ref.test
  // received such a reference, the code entered the `!VT.isAbsHeapType()`
  // branch and dereferenced the null object pointer, causing a segfault.
  {
    Configure Conf6;
    GCCheckModule GCMod;
    VM::VM VM6(Conf6);
    VM6.registerModule(GCMod);
    ASSERT_TRUE(VM6.loadWasm(NullRefTestWasm));
    ASSERT_TRUE(VM6.validate());
    ASSERT_TRUE(VM6.instantiate());
    ASSERT_TRUE(checkNullTableEntry(VM6));
    VM6.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 2);
    EXPECT_EQ(Values[0], 1); // null matches nullable eqref
    EXPECT_EQ(Values[1], 0); // null doesn't match non-nullable (ref $s)
  }

  // --- Part 6: function returning null ref with concrete type ---
  {
    Configure Conf7;
    GCCheckModule GCMod;
    VM::VM VM7(Conf7);
    VM7.registerModule(GCMod);
    ASSERT_TRUE(VM7.loadWasm(NullReturnConcreteWasm));
    ASSERT_TRUE(VM7.validate());
    ASSERT_TRUE(VM7.instantiate());
    ASSERT_TRUE(checkNullTableEntry(VM7));
    VM7.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 1);
    EXPECT_EQ(Values[0], 1); // null from concrete return matches eqref
  }

  // --- Part 7: table.grow creates null entries, then ref.cast ---
  {
    Configure Conf8;
    GCCheckModule GCMod;
    VM::VM VM8(Conf8);
    VM8.registerModule(GCMod);
    ASSERT_TRUE(VM8.loadWasm(NullTableGrowWasm));
    ASSERT_TRUE(VM8.validate());
    ASSERT_TRUE(VM8.instantiate());
    VM8.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 1);
    EXPECT_EQ(Values[0], 1); // null from grown slot passed through cast
  }
}

/// Regression test for ref.cast on null refs from concrete-typed tables
/// (issue #4757).
///
/// When ref.cast received a null reference with a concrete type, the code
/// entered the `!VT.isAbsHeapType()` branch and dereferenced the null object
/// pointer before checking for null, causing a segfault.
///
/// This test covers:
///   - ref.cast eqref on null (nullable cast should pass through)
///   - ref.cast (ref eq) on null (non-nullable cast should trap)
TEST(ExecutorRegression, RefCastNullAbility) {
  // --- Part 1: ref.cast eqref on null --- nullable cast passes through
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullRefCastWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    ASSERT_TRUE(checkNullTableEntry(VM));
    VM.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 1);
    EXPECT_EQ(Values[0], 1); // null passed through nullable cast
  }

  // --- Part 2: ref.cast (ref eq) on null --- non-nullable cast must trap
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullRefCastNonNullWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    ASSERT_TRUE(checkNullTableEntry(VM));
    auto Result = VM.execute("test");
    EXPECT_FALSE(Result); // should trap with CastFailed
  }
}

/// Regression test for br_on_cast on null refs from concrete-typed tables
/// (issue #4757).
///
/// When br_on_cast / br_on_cast_fail received a null reference with a
/// concrete type, the same null-dereference bug as ref.cast could occur in
/// runBrOnCastOp.
///
/// This test covers:
///   - br_on_cast to eqref: null matches nullable, branch taken
///   - br_on_cast_fail to (ref eq): null doesn't match non-nullable,
///     fail branch taken
TEST(ExecutorRegression, BrOnCastNullAbility) {
  Configure Conf;
  GCCheckModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(NullBrOnCastWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  ASSERT_TRUE(checkNullTableEntry(VM));
  VM.execute("test");
  auto Values = GCMod.getValues();
  ASSERT_EQ(Values.size(), 2);
  EXPECT_EQ(Values[0], 1); // br_on_cast taken for nullable match
  EXPECT_EQ(Values[1], 1); // br_on_cast_fail taken for non-nullable mismatch
}

/// Regression test for ref.test on default-initialized locals with concrete
/// ref types (issue #4768).
///
/// The bug: ValueFromType initialized reference-typed locals with
/// RefVariant(Type), passing the raw concrete type (e.g. (ref null $s)).
/// This created a null RefVariant whose ValType had HTCode = TypeIndex,
/// violating the invariant that null references always carry abstract heap
/// types. When ref.test later checked the type, it entered the
/// !VT.isAbsHeapType() branch and dereferenced the null object pointer.
///
/// The fix normalizes concrete type indices to their abstract bottom types
/// (NullFuncRef for func types, NullRef for struct/array types) when
/// initializing locals in enterFunction.
TEST(ExecutorRegression, NullLocalConcreteType) {
  // --- Part 1: local (ref null $s) where $s is a struct type ---
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullLocalStructWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    VM.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 3);
    EXPECT_EQ(Values[0], 1); // null matches nullable (ref null $s)
    EXPECT_EQ(Values[1], 1); // null matches nullable eqref
    EXPECT_EQ(Values[2], 0); // null doesn't match non-nullable (ref $s)
  }

  // --- Part 2: local (ref null $f) where $f is a func type ---
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullLocalFuncWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    VM.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 2);
    EXPECT_EQ(Values[0], 1); // null matches nullable (ref null func)
    EXPECT_EQ(Values[1], 0); // null doesn't match non-nullable (ref $f)
  }

  // --- Part 3: local (ref null $a) where $a is an array type ---
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullLocalArrayWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    VM.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 2);
    EXPECT_EQ(Values[0], 1); // null matches nullable eqref
    EXPECT_EQ(Values[1], 0); // null doesn't match non-nullable (ref $a)
  }

  // --- Part 4: global (ref null $s) initialized with ref.null $s ---
  // Verifies the global init path is safe (ref.null already calls
  // toBottomType).
  {
    Configure Conf;
    GCCheckModule GCMod;
    VM::VM VM(Conf);
    VM.registerModule(GCMod);
    ASSERT_TRUE(VM.loadWasm(NullGlobalStructWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    VM.execute("test");
    auto Values = GCMod.getValues();
    ASSERT_EQ(Values.size(), 2);
    EXPECT_EQ(Values[0], 1); // null matches nullable eqref
    EXPECT_EQ(Values[1], 0); // null doesn't match non-nullable (ref $s)
  }
}

/// Regression test for issue #5343: the null locals of the abstract ref types
/// must be bottom-typed so that ref.test/ref.cast match a concrete type.
TEST(ExecutorRegression, NullLocalAbstractType) {
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(NullLocalAbstractWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // A null structref local bottoms out at none: it matches (ref null $t) but
  // not (ref $t), and ref.cast to (ref null $t) must not trap.
  auto RTestStruct = VM.execute("test_struct");
  ASSERT_TRUE(RTestStruct);
  ASSERT_EQ(RTestStruct->size(), 1U);
  EXPECT_EQ((*RTestStruct)[0].first.get<uint32_t>(), 1U);

  auto RTestStructNonNull = VM.execute("test_struct_nonnull");
  ASSERT_TRUE(RTestStructNonNull);
  ASSERT_EQ(RTestStructNonNull->size(), 1U);
  EXPECT_EQ((*RTestStructNonNull)[0].first.get<uint32_t>(), 0U);

  auto RCastStruct = VM.execute("cast_struct");
  ASSERT_TRUE(RCastStruct) << "ref.cast trapped on a null structref local";
  ASSERT_EQ(RCastStruct->size(), 1U);
  EXPECT_EQ((*RCastStruct)[0].first.get<uint32_t>(), 7U);

  // The anyref, i31ref, and arrayref null locals bottom out at none as well.
  auto RTestAny = VM.execute("test_any");
  ASSERT_TRUE(RTestAny);
  ASSERT_EQ(RTestAny->size(), 1U);
  EXPECT_EQ((*RTestAny)[0].first.get<uint32_t>(), 1U);

  auto RTestI31 = VM.execute("test_i31");
  ASSERT_TRUE(RTestI31);
  ASSERT_EQ(RTestI31->size(), 1U);
  EXPECT_EQ((*RTestI31)[0].first.get<uint32_t>(), 1U);

  auto RTestArray = VM.execute("test_array");
  ASSERT_TRUE(RTestArray);
  ASSERT_EQ(RTestArray->size(), 1U);
  EXPECT_EQ((*RTestArray)[0].first.get<uint32_t>(), 1U);

  // A null funcref local bottoms out at nofunc: it matches (ref null $f) but
  // not (ref $f), and ref.cast to (ref null $f) must not trap.
  auto RTestFunc = VM.execute("test_func");
  ASSERT_TRUE(RTestFunc);
  ASSERT_EQ(RTestFunc->size(), 1U);
  EXPECT_EQ((*RTestFunc)[0].first.get<uint32_t>(), 1U);

  auto RTestFuncNonNull = VM.execute("test_func_nonnull");
  ASSERT_TRUE(RTestFuncNonNull);
  ASSERT_EQ(RTestFuncNonNull->size(), 1U);
  EXPECT_EQ((*RTestFuncNonNull)[0].first.get<uint32_t>(), 0U);

  auto RCastFunc = VM.execute("cast_func");
  ASSERT_TRUE(RCastFunc) << "ref.cast trapped on a null funcref local";
  ASSERT_EQ(RCastFunc->size(), 1U);
  EXPECT_EQ((*RCastFunc)[0].first.get<uint32_t>(), 7U);

  // A null externref local bottoms out at noextern.
  auto RTestExtern = VM.execute("test_extern");
  ASSERT_TRUE(RTestExtern);
  ASSERT_EQ(RTestExtern->size(), 1U);
  EXPECT_EQ((*RTestExtern)[0].first.get<uint32_t>(), 1U);
}

/// Regression test for throw_ref payload preservation.
///
/// The bug: exnref stored only a TagInstance*, dropping the captured payload.
/// When throw_ref re-walked handlers the operand stack carried garbage in
/// place of the original args, so the surfaced "(ref null $point)" was not
/// actually null and struct.get failed to trap.
TEST(ExecutorRegression, ThrowRefPreservesPayload) {
  // --- Part 1: throw_ref of (ref null $point) must trap at struct.get ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(ThrowRefNullPayloadWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("main");
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::AccessNullStruct);
  }

  // --- Part 2: multi-param `(i32 i64)` round-trip yields 1007 ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(ThrowRefMultiParamWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("main");
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1007U);
  }
}

/// Regression test for catch_all / catch_all_ref payload stack corruption.
///
/// The bug: popTopHandler always preserves AssocValSize values (the throw
/// payload) on the value stack, but the jump descriptor computed at validation
/// time uses Arity=0 for catch_all clauses and therefore does not erase them.
/// The result is that the payload (42) clobbers the value pushed before the
/// throw (1), so the function returns 42 instead of 1 in the interpreter.
///
/// This test covers:
///   - plain catch_all: function must return the pre-throw stack value (1),
///     not the payload (42)
///   - catch_all_ref: same, plus the exnref is dropped and only the pre-throw
///     value (1) remains
TEST(ExecutorRegression, CatchAllPayloadNotLeaked) {
  // --- Part 1: catch_all must discard the payload ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(CatchAllPayloadNotLeakedWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("test");
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
        << "catch_all must not leak the throw payload onto the value stack";
  }

  // --- Part 2: catch_all_ref must discard the payload (exnref is dropped) ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(CatchAllRefPayloadNotLeakedWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("test");
    ASSERT_TRUE(Result);
    ASSERT_EQ((*Result).size(), 1U);
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
        << "catch_all_ref must not leak the throw payload alongside the exnref";
  }
}

/// Regression test for a try_table handler buried by a later try_table.
///
/// A br out of a try_table leaves its handler inactive without popping it, and
/// a second try_table buries it. An uncaught throw then reached the buried
/// handler, whose VPos was recorded above the current stack height, and
/// popTopHandler erased the value stack with first > last, moving values past
/// the end of the buffer -- a guest-controlled out-of-bounds write. Execution
/// must instead report an orderly uncaught exception.
///
/// The write is only visible in a release build under one heap layout, so this
/// case must be run with assertions enabled or under ASan, where it aborts in
/// popTopHandler before returning.
TEST(ExecutorRegression, BuriedStaleTryTableHandler) {
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(BuriedStaleHandlerWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto Result = VM.execute("go");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::UncaughtException)
      << "the buried stale handler must not corrupt the value stack";
}

/// Regression test for stale try_table handlers piling up across loop
/// iterations.
///
/// A loop that branches out of a try_table strands one handler per iteration,
/// all sharing the same Try pointer, so a scan looking for inactive handlers
/// cannot tell them apart from an active one. The pile both grew without bound
/// and left the oldest entries reachable with a stale VPos. Reaping at branch
/// time keeps the stack bounded, so execution must report an orderly uncaught
/// exception.
TEST(ExecutorRegression, StaleTryTableHandlerPile) {
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(StaleHandlerPileWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto Result = VM.execute("go");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::UncaughtException)
      << "handlers stranded by a loop must not survive to corrupt the stack";
}

/// Regression test for call_indirect on a non-funcref table (type confusion).
///
/// A table whose element type is a concrete struct reference is not a funcref
/// subtype, so call_indirect on it must be rejected by validation. Before the
/// fix, ValType::isFuncRefType() accepted any concrete type index, so the
/// module passed validation and the executor reinterpreted the struct
/// reference as a FunctionInstance pointer -- with the v128 struct field bytes
/// acting as a forged function pointer. The validator now resolves the
/// concrete type via TypeMatcher and rejects the module before it can run.
TEST(ExecutorRegression, CallIndirectNonFuncTable) {
  Configure Conf;
  VM::VM VM(Conf);
  // The module is well-formed and loads successfully...
  ASSERT_TRUE(VM.loadWasm(CallIndirectStructTableWasm));
  // ... but validation must reject call_indirect on the (ref null $s) table
  // instead of letting the struct reference reach the executor as a function.
  auto Result = VM.validate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::TypeCheckFailed);
}

/// Regression test for return_call to an imported host function.
///
/// The bug: return_call to an imported host function popped the caller's frame
/// but returned its continuation under the interpreter's `-1` convention while
/// the host branch consumed it with the host convention, so execution resumed
/// on the caller's call site instead of after it. Depending on the arity
/// mismatch this crashed (caller has more params) or looped forever (equal
/// arity), with a guest-controlled out-of-bounds access underneath.
///
/// The fix keeps the host branch returning after the caller's call site, so the
/// host result propagates back to the exported caller as an ordinary return.
TEST(ExecutorRegression, TailCallToHostImport) {
  // --- Part 1: caller has more params than callee (was a crash) ---
  {
    Configure Conf;
    TailCallHostModule HostMod;
    VM::VM VM(Conf);
    VM.registerModule(HostMod);
    ASSERT_TRUE(VM.loadWasm(TailCallHostImportWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Res = VM.execute("run_more");
    ASSERT_TRUE(Res);
    ASSERT_EQ(Res->size(), 1);
    EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 42);
    auto Args = HostMod.getArgs();
    ASSERT_EQ(Args.size(), 1);
    EXPECT_EQ(Args[0], 41);
  }

  // --- Part 2: caller and callee have equal arity (was an infinite loop) ---
  {
    Configure Conf;
    TailCallHostModule HostMod;
    VM::VM VM(Conf);
    VM.registerModule(HostMod);
    ASSERT_TRUE(VM.loadWasm(TailCallHostImportWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Res = VM.execute("run_eq");
    ASSERT_TRUE(Res);
    ASSERT_EQ(Res->size(), 1);
    EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 8);
    auto Args = HostMod.getArgs();
    ASSERT_EQ(Args.size(), 1);
    EXPECT_EQ(Args[0], 7);
  }
}

/// Regression test for memory.atomic.notify with Memory64 (issue #5310).
TEST(ExecutorRegression, MemoryAtomicNotifyMemory64) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(NotifyMemory64Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // Offset 4 is 4-byte aligned. It should NOT trap even on Memory64 where
  // AddrType is i64.
  auto Result = VM.execute("test");
  EXPECT_TRUE(Result) << "memory.atomic.notify should succeed with 4-byte "
                         "alignment on Memory64";
}

/// Deeply recursive wasm must trap with CallStackExhausted once MaxStackSize
/// is exceeded, instead of completing or overflowing the host stack.
TEST(ExecutorRegression, CallStackExhaustion) {
  Configure Conf;
  Conf.getRuntimeConfigure().setMaxStackSize(UINT64_C(64) << 10);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(RecurseWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const std::vector<ValVariant> Params = {ValVariant(UINT64_C(5000))};
  const std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
  auto Result = VM.execute("f", Params, ParamTypes);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CallStackExhausted);
  EXPECT_EQ(Result.error().getErrCodePhase(), WasmPhase::Execution);
}

/// The default MaxStackSize of 0 lets the interpreter recurse deeper than the
/// compiled-code default allows, but runaway recursion still traps.
TEST(ExecutorRegression, CallStackDefaultLimit) {
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(RecurseWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
  const std::vector<ValVariant> DeepParams = {ValVariant(UINT64_C(20000))};
  EXPECT_TRUE(VM.execute("f", DeepParams, ParamTypes));
  const std::vector<ValVariant> RunawayParams = {
      ValVariant(UINT64_C(100000000))};
  auto Result = VM.execute("f", RunawayParams, ParamTypes);
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CallStackExhausted);
}

/// Regression test for tag imports, whose types must match both ways.
TEST(ExecutorRegression, TagImportMatchesBothWays) {
  // --- Part 1: the import declares the super type (must not link) ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("M", TagSubTypeExportWasm));
    ASSERT_TRUE(VM.loadWasm(TagSuperTypeImportWasm));
    ASSERT_TRUE(VM.validate());
    auto Res = VM.instantiate();
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::IncompatibleImportType);
  }

  // --- Part 2: the import declares an equivalent type (links) ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("M", TagSubTypeExportWasm));
    ASSERT_TRUE(VM.loadWasm(TagSubTypeImportWasm));
    ASSERT_TRUE(VM.validate());
    EXPECT_TRUE(VM.instantiate());
  }

  // --- Part 3: the AST module the VM copied is gone (must not link) ---
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("M", TagSubTypeExportWasm));
    auto Mod = VM.getLoader().parseModule(TagSuperTypeImportWasm);
    ASSERT_TRUE(Mod);
    ASSERT_TRUE(VM.loadWasm(**Mod));
    Mod->reset();
    ASSERT_TRUE(VM.validate());
    auto Res = VM.instantiate();
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::IncompatibleImportType);
  }
}

/// Regression test for re-exported imports, typed by their defining module.
TEST(ExecutorRegression, ReExportedImportsMatch) {
  for (auto Wasm : {Span<const Byte>(ReExportGlobalImportWasm),
                    Span<const Byte>(ReExportTableImportWasm),
                    Span<const Byte>(ReExportTagImportWasm)}) {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("A", ReExportDefineWasm));
    ASSERT_TRUE(VM.registerModule("B", ReExportForwardWasm));
    ASSERT_TRUE(VM.loadWasm(Wasm));
    ASSERT_TRUE(VM.validate());
    EXPECT_TRUE(VM.instantiate());
  }
}

/// Regression test for a host function out of any module, typed as the final
/// `(func)`: it matches `$f` but not the non-final `$g`.
TEST(ExecutorRegression, HostFuncMatchesItsDefinedType) {
  Runtime::Instance::FunctionInstance HostFunc(std::make_unique<Nop>());
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(HostFuncTableWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto *TabInst = VM.getActiveModule()->findTableExports("tab");
  ASSERT_NE(TabInst, nullptr);
  ASSERT_TRUE(TabInst->setRefAddr(0, RefVariant(&HostFunc)));

  EXPECT_TRUE(VM.execute("call_f"));
  auto CallRes = VM.execute("call_g");
  ASSERT_FALSE(CallRes);
  EXPECT_EQ(CallRes.error(), ErrCode::Value::IndirectCallTypeMismatch);

  auto TestRes = VM.execute("test_f");
  ASSERT_TRUE(TestRes);
  ASSERT_EQ(TestRes->size(), 1U);
  EXPECT_EQ(TestRes->at(0).first.get<uint32_t>(), 1U);
  TestRes = VM.execute("test_g");
  ASSERT_TRUE(TestRes);
  ASSERT_EQ(TestRes->size(), 1U);
  EXPECT_EQ(TestRes->at(0).first.get<uint32_t>(), 0U);
}

/// A reference from the host is typed by its value: non-null when it refers
/// to something, and of the bottom type when it is null.
void checkHostRefValueType(const Configure &Conf) {
  // (module
  //   (table (export "tab") 2 externref)
  //   (func (export "test") (param i32) (result i32)
  //     (ref.test (ref extern) (table.get 0 (local.get 0))))
  //   (func (export "test_bottom") (param i32) (result i32)
  //     (ref.test (ref null noextern) (table.get 0 (local.get 0))))
  //   (func (export "non_null") (param i32) (result i32)
  //     (ref.test (ref extern) (ref.as_non_null (table.get 0 (local.get 0)))))
  //   (func (export "branch") (param i32) (result i32)
  //     (block (result (ref extern))
  //       (br_on_cast 0 externref (ref extern) (table.get 0 (local.get 0)))
  //       (drop)
  //       (return (i32.const 0)))
  //     (drop)
  //     (i32.const 1)))
  std::array<WasmEdge::Byte, 137> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
      0x01, 0x7f, 0x01, 0x7f, 0x03, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04,
      0x04, 0x01, 0x6f, 0x00, 0x02, 0x07, 0x30, 0x05, 0x03, 0x74, 0x61, 0x62,
      0x01, 0x00, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x0b, 0x74, 0x65,
      0x73, 0x74, 0x5f, 0x62, 0x6f, 0x74, 0x74, 0x6f, 0x6d, 0x00, 0x01, 0x08,
      0x6e, 0x6f, 0x6e, 0x5f, 0x6e, 0x75, 0x6c, 0x6c, 0x00, 0x02, 0x06, 0x62,
      0x72, 0x61, 0x6e, 0x63, 0x68, 0x00, 0x03, 0x0a, 0x38, 0x04, 0x09, 0x00,
      0x20, 0x00, 0x25, 0x00, 0xfb, 0x14, 0x6f, 0x0b, 0x09, 0x00, 0x20, 0x00,
      0x25, 0x00, 0xfb, 0x15, 0x72, 0x0b, 0x0a, 0x00, 0x20, 0x00, 0x25, 0x00,
      0xd4, 0xfb, 0x14, 0x6f, 0x0b, 0x17, 0x00, 0x02, 0x64, 0x6f, 0x20, 0x00,
      0x25, 0x00, 0xfb, 0x18, 0x01, 0x00, 0x6f, 0x6f, 0x1a, 0x41, 0x00, 0x0f,
      0x0b, 0x1a, 0x41, 0x01, 0x0b};

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  EXPECT_EQ(VM.getActiveModule()->findFuncExports("test")->isCompiledFunction(),
            Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT);

  // Both references carry the nullable `externref` type of the host.
  int Object = 0;
  auto *TabInst = VM.getActiveModule()->findTableExports("tab");
  ASSERT_NE(TabInst, nullptr);
  ASSERT_TRUE(TabInst->setRefAddr(0, RefVariant(&Object)));
  ASSERT_TRUE(TabInst->setRefAddr(1, RefVariant(ValType(TypeCode::ExternRef))));

  const auto Run = [&VM](std::string_view Name, uint32_t Index) -> uint32_t {
    auto Result =
        VM.execute(Name, {ValVariant(Index)}, {ValType(TypeCode::I32)});
    EXPECT_TRUE(Result) << Name;
    return Result ? (*Result)[0].first.get<uint32_t>() : UINT32_MAX;
  };
  EXPECT_EQ(Run("test", 0U), 1U);
  EXPECT_EQ(Run("test", 1U), 0U);
  EXPECT_EQ(Run("test_bottom", 0U), 0U);
  EXPECT_EQ(Run("test_bottom", 1U), 1U);
  EXPECT_EQ(Run("non_null", 0U), 1U);
  EXPECT_EQ(Run("branch", 0U), 1U);
  EXPECT_EQ(Run("branch", 1U), 0U);
}

TEST(ExecutorRegression, HostRefValueType) {
  checkHostRefValueType(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, HostRefValueTypeJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkHostRefValueType(Conf);
}
#endif

/// An externalized reference handed to the host internalizes to itself.
void checkExternalizedRefThroughHost(const Configure &Conf) {
  // (module
  //   (type (struct (field i32)))
  //   (func (export "wrap_struct") (result externref)
  //     (extern.convert_any (struct.new 0 (i32.const 42))))
  //   (func (export "wrap_i31") (result externref)
  //     (extern.convert_any (ref.i31 (i32.const 7))))
  //   (func (export "unwrap_struct") (param externref) (result i32)
  //     (struct.get 0 0 (ref.cast (ref 0) (any.convert_extern (local.get 0)))))
  //   (func (export "unwrap_i31") (param externref) (result i32)
  //     (i31.get_u (ref.cast (ref i31) (any.convert_extern (local.get 0))))))
  std::array<WasmEdge::Byte, 136> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0e, 0x03, 0x5f,
      0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x6f, 0x60, 0x01, 0x6f, 0x01, 0x7f,
      0x03, 0x05, 0x04, 0x01, 0x01, 0x02, 0x02, 0x07, 0x37, 0x04, 0x0b, 0x77,
      0x72, 0x61, 0x70, 0x5f, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x00,
      0x08, 0x77, 0x72, 0x61, 0x70, 0x5f, 0x69, 0x33, 0x31, 0x00, 0x01, 0x0d,
      0x75, 0x6e, 0x77, 0x72, 0x61, 0x70, 0x5f, 0x73, 0x74, 0x72, 0x75, 0x63,
      0x74, 0x00, 0x02, 0x0a, 0x75, 0x6e, 0x77, 0x72, 0x61, 0x70, 0x5f, 0x69,
      0x33, 0x31, 0x00, 0x03, 0x0a, 0x2e, 0x04, 0x09, 0x00, 0x41, 0x2a, 0xfb,
      0x00, 0x00, 0xfb, 0x1b, 0x0b, 0x08, 0x00, 0x41, 0x07, 0xfb, 0x1c, 0xfb,
      0x1b, 0x0b, 0x0d, 0x00, 0x20, 0x00, 0xfb, 0x1a, 0xfb, 0x16, 0x00, 0xfb,
      0x02, 0x00, 0x00, 0x0b, 0x0b, 0x00, 0x20, 0x00, 0xfb, 0x1a, 0xfb, 0x16,
      0x6c, 0xfb, 0x1e, 0x0b};

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  EXPECT_EQ(VM.getActiveModule()
                ->findFuncExports("wrap_struct")
                ->isCompiledFunction(),
            Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT);

  // The host gets `(ref extern)` and passes the value back as it is.
  auto Wrapped = VM.execute("wrap_struct");
  ASSERT_TRUE(Wrapped);
  ASSERT_EQ(Wrapped->size(), 1U);
  EXPECT_EQ(Wrapped->at(0).second, ValType(TypeCode::Ref, TypeCode::ExternRef));
  auto Res = VM.execute("unwrap_struct", {Wrapped->at(0).first},
                        {Wrapped->at(0).second});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 42U);

  Wrapped = VM.execute("wrap_i31");
  ASSERT_TRUE(Wrapped);
  ASSERT_EQ(Wrapped->size(), 1U);
  EXPECT_EQ(Wrapped->at(0).second, ValType(TypeCode::Ref, TypeCode::ExternRef));
  Res =
      VM.execute("unwrap_i31", {Wrapped->at(0).first}, {Wrapped->at(0).second});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 7U);
}

TEST(ExecutorRegression, ExternalizedRefThroughHost) {
  checkExternalizedRefThroughHost(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, ExternalizedRefThroughHostJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkExternalizedRefThroughHost(Conf);
}
#endif

/// An argument of an invocation matches the parameter type by its value.
void checkInvokeArgumentValueType(const Configure &Conf) {
  // (module
  //   (type (struct (field i32)))
  //   (type (array i32))
  //   (type (func (result i32)))
  //   (table (export "tab") 1 externref)
  //   (func (export "one") (type 2)
  //     (i32.const 1))
  //   (func (export "new_struct") (result (ref 0))
  //     (struct.new 0 (i32.const 42)))
  //   (func (export "get_tab") (result externref)
  //     (table.get 0 (i32.const 0)))
  //   (func (export "get_one") (result funcref)
  //     (ref.func 0))
  //   (func (export "get_new") (result funcref)
  //     (ref.func 1))
  //   (func (export "test_struct") (param anyref) (result i32)
  //     (ref.test (ref 0) (local.get 0)))
  //   (func (export "take_extern") (param externref) (result i32)
  //     (ref.is_null (local.get 0)))
  //   (func (export "take_func") (param (ref null 2)) (result i32)
  //     (ref.is_null (local.get 0)))
  //   (func (export "take_non_null") (param (ref extern)))
  //   (func (export "take_array") (param (ref 1))))
  std::array<WasmEdge::Byte, 266> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x33, 0x0b, 0x5f,
      0x01, 0x7f, 0x00, 0x5e, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x00,
      0x01, 0x64, 0x00, 0x60, 0x00, 0x01, 0x6f, 0x60, 0x00, 0x01, 0x70, 0x60,
      0x01, 0x6e, 0x01, 0x7f, 0x60, 0x01, 0x6f, 0x01, 0x7f, 0x60, 0x01, 0x63,
      0x02, 0x01, 0x7f, 0x60, 0x01, 0x64, 0x6f, 0x00, 0x60, 0x01, 0x64, 0x01,
      0x00, 0x03, 0x0b, 0x0a, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x04, 0x04, 0x01, 0x6f, 0x00, 0x01, 0x07, 0x7d, 0x0b, 0x03,
      0x74, 0x61, 0x62, 0x01, 0x00, 0x03, 0x6f, 0x6e, 0x65, 0x00, 0x00, 0x0a,
      0x6e, 0x65, 0x77, 0x5f, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x01,
      0x07, 0x67, 0x65, 0x74, 0x5f, 0x74, 0x61, 0x62, 0x00, 0x02, 0x07, 0x67,
      0x65, 0x74, 0x5f, 0x6f, 0x6e, 0x65, 0x00, 0x03, 0x07, 0x67, 0x65, 0x74,
      0x5f, 0x6e, 0x65, 0x77, 0x00, 0x04, 0x0b, 0x74, 0x65, 0x73, 0x74, 0x5f,
      0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x00, 0x05, 0x0b, 0x74, 0x61, 0x6b,
      0x65, 0x5f, 0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x00, 0x06, 0x09, 0x74,
      0x61, 0x6b, 0x65, 0x5f, 0x66, 0x75, 0x6e, 0x63, 0x00, 0x07, 0x0d, 0x74,
      0x61, 0x6b, 0x65, 0x5f, 0x6e, 0x6f, 0x6e, 0x5f, 0x6e, 0x75, 0x6c, 0x6c,
      0x00, 0x08, 0x0a, 0x74, 0x61, 0x6b, 0x65, 0x5f, 0x61, 0x72, 0x72, 0x61,
      0x79, 0x00, 0x09, 0x0a, 0x39, 0x0a, 0x04, 0x00, 0x41, 0x01, 0x0b, 0x07,
      0x00, 0x41, 0x2a, 0xfb, 0x00, 0x00, 0x0b, 0x06, 0x00, 0x41, 0x00, 0x25,
      0x00, 0x0b, 0x04, 0x00, 0xd2, 0x00, 0x0b, 0x04, 0x00, 0xd2, 0x01, 0x0b,
      0x07, 0x00, 0x20, 0x00, 0xfb, 0x14, 0x00, 0x0b, 0x05, 0x00, 0x20, 0x00,
      0xd1, 0x0b, 0x05, 0x00, 0x20, 0x00, 0xd1, 0x0b, 0x02, 0x00, 0x0b, 0x02,
      0x00, 0x0b};

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  EXPECT_EQ(VM.getActiveModule()
                ->findFuncExports("test_struct")
                ->isCompiledFunction(),
            Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT);

  int Object = 0;
  auto *TabInst = VM.getActiveModule()->findTableExports("tab");
  ASSERT_NE(TabInst, nullptr);
  ASSERT_TRUE(TabInst->setRefAddr(0, RefVariant(&Object)));

  // A result goes back as it was returned, and keeps its own type.
  auto Made = VM.execute("new_struct");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  auto Res =
      VM.execute("test_struct", {Made->at(0).first}, {Made->at(0).second});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 1U);

  Made = VM.execute("get_tab");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("take_extern", {Made->at(0).first}, {Made->at(0).second});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 0U);

  Made = VM.execute("get_one");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("take_func", {Made->at(0).first}, {Made->at(0).second});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 0U);

  // A null reference fails for a non-null parameter type.
  Res = VM.execute("take_non_null",
                   {ValVariant(RefVariant(ValType(TypeCode::ExternRef)))},
                   {ValType(TypeCode::ExternRef)});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::NonNullRequired);

  // A value out of the parameter type fails under a type that contains it.
  Made = VM.execute("new_struct");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("take_array", {Made->at(0).first},
                   {ValType(TypeCode::AnyRef)});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::FuncSigMismatch);

  Made = VM.execute("get_new");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("take_func", {Made->at(0).first},
                   {ValType(TypeCode::FuncRef)});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::FuncSigMismatch);

  // A reference is seen as the type passed with it: an extern reference does
  // not fit an internal parameter, and the same value passed as an any does.
  Made = VM.execute("get_tab");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("test_struct", {Made->at(0).first}, {Made->at(0).second});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::FuncSigMismatch);
  Res = VM.execute("test_struct", {Made->at(0).first},
                   {ValType(TypeCode::AnyRef)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 0U);

  // An internal reference does not fit an extern parameter, and the same value
  // passed as an extern reference does.
  Made = VM.execute("new_struct");
  ASSERT_TRUE(Made);
  ASSERT_EQ(Made->size(), 1U);
  Res = VM.execute("take_extern", {Made->at(0).first}, {Made->at(0).second});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::FuncSigMismatch);
  Res = VM.execute("take_extern", {Made->at(0).first},
                   {ValType(TypeCode::ExternRef)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 0U);
}

TEST(ExecutorRegression, InvokeArgumentValueType) {
  checkInvokeArgumentValueType(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, InvokeArgumentValueTypeJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkInvokeArgumentValueType(Conf);
}
#endif

/// Only a reference to a function instance is retrieved as a function.
void checkStructRefNotFuncRef(const Configure &Conf) {
  // (module
  //   (type (struct (field i32)))
  //   (global (export "struct") (mut anyref) (ref.null any))
  //   (global (export "func") (mut funcref) (ref.null func))
  //   (func (export "run")
  //     (global.set 0 (struct.new 0 (i32.const 1)))
  //     (global.set 1 (ref.func 0))))
  std::array<WasmEdge::Byte, 77> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02,
      0x5f, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x00, 0x03, 0x02, 0x01, 0x01,
      0x06, 0x0b, 0x02, 0x6e, 0x01, 0xd0, 0x6e, 0x0b, 0x70, 0x01, 0xd0,
      0x70, 0x0b, 0x07, 0x17, 0x03, 0x06, 0x73, 0x74, 0x72, 0x75, 0x63,
      0x74, 0x03, 0x00, 0x04, 0x66, 0x75, 0x6e, 0x63, 0x03, 0x01, 0x03,
      0x72, 0x75, 0x6e, 0x00, 0x00, 0x0a, 0x0f, 0x01, 0x0d, 0x00, 0x41,
      0x01, 0xfb, 0x00, 0x00, 0x24, 0x00, 0xd2, 0x00, 0x24, 0x01, 0x0b};

  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  const auto *ModInst = VM.getActiveModule();
  const auto *FuncInst = ModInst->findFuncExports("run");
  ASSERT_NE(FuncInst, nullptr);
  EXPECT_EQ(FuncInst->isCompiledFunction(),
            Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT);
  ASSERT_TRUE(VM.execute("run"));

  const auto &Struct =
      ModInst->findGlobalExports("struct")->getValue().get<RefVariant>();
  EXPECT_FALSE(Struct.isNull());
  EXPECT_EQ(retrieveFuncRef(Struct), nullptr);
  const auto &Func =
      ModInst->findGlobalExports("func")->getValue().get<RefVariant>();
  EXPECT_EQ(retrieveFuncRef(Func), FuncInst);
}

TEST(ExecutorRegression, StructRefNotFuncRef) {
  checkStructRefNotFuncRef(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, StructRefNotFuncRefJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkStructRefNotFuncRef(Conf);
}
#endif

#ifdef WASMEDGE_USE_LLVM
/// A compiled function left in an imported table by an instantiation that
/// failed while initializing the memory still reads the global of its module.
TEST(ExecutorRegression, FailedInstantiationKeepsContextJIT) {
  // (module
  //   (type (func (result i32)))
  //   (table (export "tab") 2 funcref)
  //   (func (export "read") (param i32) (result i32)
  //     (call_indirect (type 0) (local.get 0))))
  std::array<Byte, 57> SharedWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x02, 0x60,
      0x00, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x01,
      0x04, 0x04, 0x01, 0x70, 0x00, 0x02, 0x07, 0x0e, 0x02, 0x03, 0x74, 0x61,
      0x62, 0x01, 0x00, 0x04, 0x72, 0x65, 0x61, 0x64, 0x00, 0x00, 0x0a, 0x09,
      0x01, 0x07, 0x00, 0x20, 0x00, 0x11, 0x00, 0x00, 0x0b};
  // (module
  //   (import "shared" "tab" (table 2 funcref))
  //   (global (mut i32) (i32.const 7))
  //   (memory 1)
  //   (func (result i32) (global.get 0))
  //   (elem (i32.const 0) func 0)
  //   (data (i32.const 65536) "d"))
  std::array<Byte, 78> DataOutOfBoundsWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7f, 0x02, 0x10, 0x01, 0x06, 0x73, 0x68, 0x61, 0x72, 0x65,
      0x64, 0x03, 0x74, 0x61, 0x62, 0x01, 0x70, 0x00, 0x02, 0x03, 0x02, 0x01,
      0x00, 0x05, 0x03, 0x01, 0x00, 0x01, 0x06, 0x06, 0x01, 0x7f, 0x01, 0x41,
      0x07, 0x0b, 0x09, 0x07, 0x01, 0x00, 0x41, 0x00, 0x0b, 0x01, 0x00, 0x0a,
      0x06, 0x01, 0x04, 0x00, 0x23, 0x00, 0x0b, 0x0b, 0x09, 0x01, 0x00, 0x41,
      0x80, 0x80, 0x04, 0x0b, 0x01, 0x64};

  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule("shared", SharedWasm));
  ASSERT_TRUE(VM.loadWasm(DataOutOfBoundsWasm));
  ASSERT_TRUE(VM.validate());
  auto InstRes = VM.instantiate();
  ASSERT_FALSE(InstRes);
  EXPECT_EQ(InstRes.error(), ErrCode::Value::MemoryOutOfBounds);

  auto Res = VM.execute("shared", "read", {ValVariant(UINT32_C(0))},
                        {ValType(TypeCode::I32)});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(Res->at(0).first.get<uint32_t>(), 7U);
}
#endif

// clang-format off
/// Binary Wasm module: vector min, max, and rounding on caller-supplied lanes.
///
/// (module
///   (func (export "f32x4.min") (param v128 v128) (result v128)
///     (f32x4.min (local.get 0) (local.get 1)))
///   (func (export "f32x4.max") (param v128 v128) (result v128)
///     (f32x4.max (local.get 0) (local.get 1)))
///   (func (export "f64x2.min") (param v128 v128) (result v128)
///     (f64x2.min (local.get 0) (local.get 1)))
///   (func (export "f64x2.max") (param v128 v128) (result v128)
///     (f64x2.max (local.get 0) (local.get 1)))
///   (func (export "f32x4.ceil") (param v128) (result v128)
///     (f32x4.ceil (local.get 0)))
///   (func (export "f32x4.floor") (param v128) (result v128)
///     (f32x4.floor (local.get 0)))
///   (func (export "f32x4.trunc") (param v128) (result v128)
///     (f32x4.trunc (local.get 0)))
///   (func (export "f32x4.nearest") (param v128) (result v128)
///     (f32x4.nearest (local.get 0)))
///   (func (export "f64x2.ceil") (param v128) (result v128)
///     (f64x2.ceil (local.get 0)))
///   (func (export "f64x2.floor") (param v128) (result v128)
///     (f64x2.floor (local.get 0)))
///   (func (export "f64x2.trunc") (param v128) (result v128)
///     (f64x2.trunc (local.get 0)))
///   (func (export "f64x2.nearest") (param v128) (result v128)
///     (f64x2.nearest (local.get 0))))
std::array<WasmEdge::Byte, 303> SimdNaNWasm{
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0C, 0x02, 0x60,
    0x02, 0x7B, 0x7B, 0x01, 0x7B, 0x60, 0x01, 0x7B, 0x01, 0x7B, 0x03, 0x0D,
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x07, 0xA3, 0x01, 0x0C, 0x09, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2E,
    0x6D, 0x69, 0x6E, 0x00, 0x00, 0x09, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2E,
    0x6D, 0x61, 0x78, 0x00, 0x01, 0x09, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2E,
    0x6D, 0x69, 0x6E, 0x00, 0x02, 0x09, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2E,
    0x6D, 0x61, 0x78, 0x00, 0x03, 0x0A, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2E,
    0x63, 0x65, 0x69, 0x6C, 0x00, 0x04, 0x0B, 0x66, 0x33, 0x32, 0x78, 0x34,
    0x2E, 0x66, 0x6C, 0x6F, 0x6F, 0x72, 0x00, 0x05, 0x0B, 0x66, 0x33, 0x32,
    0x78, 0x34, 0x2E, 0x74, 0x72, 0x75, 0x6E, 0x63, 0x00, 0x06, 0x0D, 0x66,
    0x33, 0x32, 0x78, 0x34, 0x2E, 0x6E, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74,
    0x00, 0x07, 0x0A, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2E, 0x63, 0x65, 0x69,
    0x6C, 0x00, 0x08, 0x0B, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2E, 0x66, 0x6C,
    0x6F, 0x6F, 0x72, 0x00, 0x09, 0x0B, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2E,
    0x74, 0x72, 0x75, 0x6E, 0x63, 0x00, 0x0A, 0x0D, 0x66, 0x36, 0x34, 0x78,
    0x32, 0x2E, 0x6E, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00, 0x0B, 0x0A,
    0x62, 0x0C, 0x09, 0x00, 0x20, 0x00, 0x20, 0x01, 0xFD, 0xE8, 0x01, 0x0B,
    0x09, 0x00, 0x20, 0x00, 0x20, 0x01, 0xFD, 0xE9, 0x01, 0x0B, 0x09, 0x00,
    0x20, 0x00, 0x20, 0x01, 0xFD, 0xF4, 0x01, 0x0B, 0x09, 0x00, 0x20, 0x00,
    0x20, 0x01, 0xFD, 0xF5, 0x01, 0x0B, 0x06, 0x00, 0x20, 0x00, 0xFD, 0x67,
    0x0B, 0x06, 0x00, 0x20, 0x00, 0xFD, 0x68, 0x0B, 0x06, 0x00, 0x20, 0x00,
    0xFD, 0x69, 0x0B, 0x06, 0x00, 0x20, 0x00, 0xFD, 0x6A, 0x0B, 0x06, 0x00,
    0x20, 0x00, 0xFD, 0x74, 0x0B, 0x06, 0x00, 0x20, 0x00, 0xFD, 0x75, 0x0B,
    0x06, 0x00, 0x20, 0x00, 0xFD, 0x7A, 0x0B, 0x07, 0x00, 0x20, 0x00, 0xFD,
    0x94, 0x01, 0x0B};
// clang-format on

/// Packs the bit patterns of float lanes into a v128, Wasm lane 0 first.
template <typename T>
uint128_t v128Of(std::initializer_list<FloatBits<T>> Lanes) {
  uint128_t V = 0U;
  unsigned int Shift = 0;
  for (const auto Lane : Lanes) {
    V |= uint128_t(Lane) << Shift;
    Shift += sizeof(T) * 8;
  }
  return V;
}

/// Expects each lane of Got to match Want. A NaN in Want accepts only an
/// arithmetic NaN, which is a NaN with the quiet bit set.
template <typename T>
void expectLanes(std::string_view Op, uint128_t Got,
                 std::initializer_list<FloatBits<T>> Want) {
  unsigned int Shift = 0;
  for (const auto WantBits : Want) {
    const auto GotBits =
        static_cast<FloatBits<T>>(static_cast<uint64_t>(Got >> Shift));
    if (std::isnan(cxx20::bit_cast<T>(WantBits))) {
      EXPECT_TRUE(std::isnan(cxx20::bit_cast<T>(GotBits)) &&
                  (GotBits & QuietBit<T>) != 0)
          << Op << " lane bits " << std::hex << GotBits;
    } else {
      EXPECT_EQ(GotBits, WantBits) << Op;
    }
    Shift += sizeof(T) * 8;
  }
}

/// Vector min, max, and rounding must return an arithmetic NaN for a NaN
/// operand, including a signaling one. The spec test harness accepts any NaN,
/// so it cannot catch a signaling NaN that passes through unchanged.
void checkSimdNaNResults(const Configure &Conf) {
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(SimdNaNWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  const auto Run = [&VM](std::string_view Name,
                         std::vector<ValVariant> Args) -> uint128_t {
    const std::vector<ValType> Types(Args.size(), ValType(TypeCode::V128));
    auto Result = VM.execute(Name, Args, Types);
    EXPECT_TRUE(Result) << Name;
    return Result ? (*Result)[0].first.get<uint128_t>() : uint128_t(0U);
  };

  constexpr uint32_t NaN32 = UINT32_C(0x7FC00000);
  // Lanes: +sNaN, -1.5, -sNaN, 2.5 and 1.0, +sNaN, -sNaN, 3.0.
  const auto A32 = v128Of<float>({UINT32_C(0x7FA00000), UINT32_C(0xBFC00000),
                                  UINT32_C(0xFFA00001), UINT32_C(0x40200000)});
  const auto B32 = v128Of<float>({UINT32_C(0x3F800000), UINT32_C(0x7F800001),
                                  UINT32_C(0xFF800002), UINT32_C(0x40400000)});
  for (const auto &[L, R] : {std::pair{A32, B32}, std::pair{B32, A32}}) {
    expectLanes<float>("f32x4.min", Run("f32x4.min", {L, R}),
                       {NaN32, NaN32, NaN32, UINT32_C(0x40200000)});
    expectLanes<float>("f32x4.max", Run("f32x4.max", {L, R}),
                       {NaN32, NaN32, NaN32, UINT32_C(0x40400000)});
  }
  expectLanes<float>(
      "f32x4.ceil", Run("f32x4.ceil", {A32}),
      {NaN32, UINT32_C(0xBF800000), NaN32, UINT32_C(0x40400000)});
  expectLanes<float>(
      "f32x4.floor", Run("f32x4.floor", {A32}),
      {NaN32, UINT32_C(0xC0000000), NaN32, UINT32_C(0x40000000)});
  expectLanes<float>(
      "f32x4.trunc", Run("f32x4.trunc", {A32}),
      {NaN32, UINT32_C(0xBF800000), NaN32, UINT32_C(0x40000000)});
  expectLanes<float>(
      "f32x4.nearest", Run("f32x4.nearest", {A32}),
      {NaN32, UINT32_C(0xC0000000), NaN32, UINT32_C(0x40000000)});

  constexpr uint64_t NaN64 = UINT64_C(0x7FF8000000000000);
  // Lanes: +sNaN, -1.5 and 1.0, -sNaN.
  const auto A64 = v128Of<double>(
      {UINT64_C(0x7FF4000000000000), UINT64_C(0xBFF8000000000000)});
  const auto B64 = v128Of<double>(
      {UINT64_C(0x3FF0000000000000), UINT64_C(0xFFF0000000000001)});
  for (const auto &[L, R] : {std::pair{A64, B64}, std::pair{B64, A64}}) {
    expectLanes<double>("f64x2.min", Run("f64x2.min", {L, R}), {NaN64, NaN64});
    expectLanes<double>("f64x2.max", Run("f64x2.max", {L, R}), {NaN64, NaN64});
  }
  expectLanes<double>("f64x2.ceil", Run("f64x2.ceil", {A64}),
                      {NaN64, UINT64_C(0xBFF0000000000000)});
  expectLanes<double>("f64x2.floor", Run("f64x2.floor", {B64}),
                      {UINT64_C(0x3FF0000000000000), NaN64});
  expectLanes<double>("f64x2.trunc", Run("f64x2.trunc", {A64}),
                      {NaN64, UINT64_C(0xBFF0000000000000)});
  expectLanes<double>("f64x2.nearest", Run("f64x2.nearest", {A64}),
                      {NaN64, UINT64_C(0xC000000000000000)});
}

TEST(ExecutorRegression, SimdNaNResultsAreArithmetic) {
  checkSimdNaNResults(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, SimdNaNResultsAreArithmeticJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkSimdNaNResults(Conf);
}
#endif

// clang-format off
/// Binary Wasm module: scalar rounding on caller-supplied bit patterns.
///
/// (module
///   (func (export "f32.ceil") (param i32) (result i32)
///     (i32.reinterpret_f32 (f32.ceil (f32.reinterpret_i32 (local.get 0)))))
///   (func (export "f32.floor") (param i32) (result i32)
///     (i32.reinterpret_f32 (f32.floor (f32.reinterpret_i32 (local.get 0)))))
///   (func (export "f32.trunc") (param i32) (result i32)
///     (i32.reinterpret_f32 (f32.trunc (f32.reinterpret_i32 (local.get 0)))))
///   (func (export "f32.nearest") (param i32) (result i32)
///     (i32.reinterpret_f32 (f32.nearest (f32.reinterpret_i32 (local.get 0)))))
///   (func (export "f64.ceil") (param i64) (result i64)
///     (i64.reinterpret_f64 (f64.ceil (f64.reinterpret_i64 (local.get 0)))))
///   (func (export "f64.floor") (param i64) (result i64)
///     (i64.reinterpret_f64 (f64.floor (f64.reinterpret_i64 (local.get 0)))))
///   (func (export "f64.trunc") (param i64) (result i64)
///     (i64.reinterpret_f64 (f64.trunc (f64.reinterpret_i64 (local.get 0)))))
///   (func (export "f64.nearest") (param i64) (result i64)
///     (i64.reinterpret_f64 (f64.nearest (f64.reinterpret_i64 (local.get 0))))))
std::array<WasmEdge::Byte, 200> ScalarNaNWasm{
    0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0B, 0x02, 0x60,
    0x01, 0x7F, 0x01, 0x7F, 0x60, 0x01, 0x7E, 0x01, 0x7E, 0x03, 0x09, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x07, 0x63, 0x08, 0x08,
    0x66, 0x33, 0x32, 0x2E, 0x63, 0x65, 0x69, 0x6C, 0x00, 0x00, 0x09, 0x66,
    0x33, 0x32, 0x2E, 0x66, 0x6C, 0x6F, 0x6F, 0x72, 0x00, 0x01, 0x09, 0x66,
    0x33, 0x32, 0x2E, 0x74, 0x72, 0x75, 0x6E, 0x63, 0x00, 0x02, 0x0B, 0x66,
    0x33, 0x32, 0x2E, 0x6E, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00, 0x03,
    0x08, 0x66, 0x36, 0x34, 0x2E, 0x63, 0x65, 0x69, 0x6C, 0x00, 0x04, 0x09,
    0x66, 0x36, 0x34, 0x2E, 0x66, 0x6C, 0x6F, 0x6F, 0x72, 0x00, 0x05, 0x09,
    0x66, 0x36, 0x34, 0x2E, 0x74, 0x72, 0x75, 0x6E, 0x63, 0x00, 0x06, 0x0B,
    0x66, 0x36, 0x34, 0x2E, 0x6E, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00,
    0x07, 0x0A, 0x41, 0x08, 0x07, 0x00, 0x20, 0x00, 0xBE, 0x8D, 0xBC, 0x0B,
    0x07, 0x00, 0x20, 0x00, 0xBE, 0x8E, 0xBC, 0x0B, 0x07, 0x00, 0x20, 0x00,
    0xBE, 0x8F, 0xBC, 0x0B, 0x07, 0x00, 0x20, 0x00, 0xBE, 0x90, 0xBC, 0x0B,
    0x07, 0x00, 0x20, 0x00, 0xBF, 0x9B, 0xBD, 0x0B, 0x07, 0x00, 0x20, 0x00,
    0xBF, 0x9C, 0xBD, 0x0B, 0x07, 0x00, 0x20, 0x00, 0xBF, 0x9D, 0xBD, 0x0B,
    0x07, 0x00, 0x20, 0x00, 0xBF, 0x9E, 0xBD, 0x0B};
// clang-format on

/// Scalar rounding must also return an arithmetic NaN for a signaling NaN.
void checkScalarNaNResults(const Configure &Conf) {
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(ScalarNaNWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  for (const auto Name :
       {"f32.ceil", "f32.floor", "f32.trunc", "f32.nearest"}) {
    for (const uint32_t In : {UINT32_C(0x7FA00000), UINT32_C(0xFFA00001)}) {
      auto Result =
          VM.execute(Name, {ValVariant(In)}, {ValType(TypeCode::I32)});
      ASSERT_TRUE(Result) << Name;
      const auto Got = (*Result)[0].first.get<uint32_t>();
      EXPECT_TRUE(std::isnan(cxx20::bit_cast<float>(Got)) &&
                  (Got & QuietBit<float>) != 0)
          << Name << " bits " << std::hex << Got;
    }
  }
  for (const auto Name :
       {"f64.ceil", "f64.floor", "f64.trunc", "f64.nearest"}) {
    for (const uint64_t In :
         {UINT64_C(0x7FF4000000000000), UINT64_C(0xFFF0000000000001)}) {
      auto Result =
          VM.execute(Name, {ValVariant(In)}, {ValType(TypeCode::I64)});
      ASSERT_TRUE(Result) << Name;
      const auto Got = (*Result)[0].first.get<uint64_t>();
      EXPECT_TRUE(std::isnan(cxx20::bit_cast<double>(Got)) &&
                  (Got & QuietBit<double>) != 0)
          << Name << " bits " << std::hex << Got;
    }
  }
}

TEST(ExecutorRegression, ScalarNaNResultsAreArithmetic) {
  checkScalarNaNResults(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, ScalarNaNResultsAreArithmeticJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  checkScalarNaNResults(Conf);
}
#endif

} // namespace

/// A table whose initial size cannot be allocated must fail instantiation.
TEST(ExecutorRegression, TableAllocationFailure) {
  // (module (table i64 0x4000000000000000 funcref))
  std::array<WasmEdge::Byte, 22> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x04, 0x0c, 0x01,
      0x70, 0x04, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x40};
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  auto Result = VM.instantiate();
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::AllocationFailed);
}

/// A table.grow whose new size cannot be allocated must return -1.
TEST(ExecutorRegression, TableGrowAllocationFailure) {
  // (module
  //   (table i64 1 funcref)
  //   (func (export "grow") (result i64)
  //     ref.null func
  //     i64.const 0x4000000000000000
  //     table.grow 0))
  std::array<WasmEdge::Byte, 57> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05, 0x01, 0x60,
      0x00, 0x01, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x04, 0x04, 0x01, 0x70, 0x04,
      0x01, 0x07, 0x08, 0x01, 0x04, 0x67, 0x72, 0x6f, 0x77, 0x00, 0x00, 0x0a,
      0x14, 0x01, 0x12, 0x00, 0xd0, 0x70, 0x42, 0x80, 0x80, 0x80, 0x80, 0x80,
      0x80, 0x80, 0x80, 0xc0, 0x00, 0xfc, 0x0f, 0x00, 0x0b};
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  auto Result = VM.execute("grow");
  ASSERT_TRUE(Result);
  ASSERT_EQ(Result->size(), 1U);
  EXPECT_EQ((*Result)[0].first.get<uint64_t>(), UINT64_MAX);
}

/// A name registered by another party while a named module is being
/// instantiated must fail the instantiation instead of being ignored.
TEST(ExecutorRegression, RegisterModuleLateNameConflict) {
  class RegisterOther : public Runtime::HostFunction<RegisterOther> {
  public:
    RegisterOther(Runtime::StoreManager &S,
                  const Runtime::Instance::ModuleInstance &M)
        : Store(S), Other(M) {}
    Expect<void> body(const Runtime::CallingFrame &) {
      return Store.registerModule(&Other, "dup");
    }

  private:
    Runtime::StoreManager &Store;
    const Runtime::Instance::ModuleInstance &Other;
  };

  // (module
  //   (import "host" "f" (func))
  //   (start 0))
  std::array<WasmEdge::Byte, 29> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04,
      0x01, 0x60, 0x00, 0x00, 0x02, 0x0a, 0x01, 0x04, 0x68, 0x6f,
      0x73, 0x74, 0x01, 0x66, 0x00, 0x00, 0x08, 0x01, 0x00};
  Configure Conf;
  Loader::Loader Loader(Conf);
  Validator::Validator Validator(Conf);
  Executor::Executor Executor(Conf);
  Runtime::Instance::ModuleInstance Other("other");
  Runtime::Instance::ModuleInstance HostMod("host");
  Runtime::StoreManager Store;
  HostMod.addHostFunc("f", std::make_unique<RegisterOther>(Store, Other));
  ASSERT_TRUE(Executor.registerModule(Store, HostMod));

  auto Mod = Loader.parseModule(Wasm);
  ASSERT_TRUE(Mod);
  ASSERT_TRUE(Validator.validate(**Mod));
  auto Res = Executor.registerModule(Store, **Mod, "dup");
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::ModuleNameConflict);
  EXPECT_EQ(Store.findModule("dup"), &Other);
}

/// Registering a component under a name that is already taken must fail.
TEST(ExecutorRegression, RegisterComponentNameConflict) {
  // (component)
  std::array<WasmEdge::Byte, 8> Wasm{0x00, 0x61, 0x73, 0x6d,
                                     0x0d, 0x00, 0x01, 0x00};
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Loader(Conf);
  Validator::Validator Validator(Conf);
  Executor::Executor Executor(Conf);
  Runtime::StoreManager Store;

  auto Unit = Loader.parseWasmUnit(Wasm);
  ASSERT_TRUE(Unit);
  using CompPtr = std::unique_ptr<AST::Component::Component>;
  ASSERT_TRUE(std::holds_alternative<CompPtr>(*Unit));
  const auto &Comp = *std::get<CompPtr>(*Unit);
  ASSERT_TRUE(Validator.validate(Comp));
  auto First = Executor.registerComponent(Store, Comp, "dup");
  ASSERT_TRUE(First);
  auto Second = Executor.registerComponent(Store, Comp, "dup");
  ASSERT_FALSE(Second);
  EXPECT_EQ(Second.error(), ErrCode::Value::ModuleNameConflict);
  EXPECT_EQ(Store.findComponent("dup"), First->get());
}

/// Host function that stops the executor that calls it.
class StopExecutor : public Runtime::HostFunction<StopExecutor> {
public:
  Expect<void> body(const Runtime::CallingFrame &Frame) {
    Frame.getExecutor()->stop();
    return {};
  }
};

/// Host function that calls the "inner" and then the "probe" export of the
/// calling module, and handles the errors of both calls.
class CallInnerAndProbe : public Runtime::HostFunction<CallInnerAndProbe> {
public:
  Expect<void> body(const Runtime::CallingFrame &Frame) {
    auto *Executor = Frame.getExecutor();
    const auto *Module = Frame.getModule();
    auto Inner = Executor->invoke(Module->findFuncExports("inner"), {}, {});
    InnerInterrupted = !Inner && Inner.error() == ErrCode::Value::Interrupted;
    auto Probe = Executor->invoke(Module->findFuncExports("probe"), {}, {});
    ProbeInterrupted = !Probe && Probe.error() == ErrCode::Value::Interrupted;
    return {};
  }
  bool InnerInterrupted = false;
  bool ProbeInterrupted = false;
};

/// Module "host" with the host functions of the stop tests.
class StopHostModule : public Runtime::Instance::ModuleInstance {
public:
  StopHostModule() : ModuleInstance("host") {
    addHostFunc("stop", std::make_unique<StopExecutor>());
    auto CP = std::make_unique<CallInnerAndProbe>();
    C = CP.get();
    addHostFunc("call_inner_and_probe", std::move(CP));
  }
  const CallInnerAndProbe &callback() const noexcept { return *C; }

private:
  CallInnerAndProbe *C = nullptr;
};

void checkNestedCallKeepsStop(Configure Conf) {
  // (module
  //   (import "host" "stop" (func $stop))
  //   (import "host" "call_inner_and_probe" (func $cb))
  //   (func $inner (export "inner") (call $stop) (block (br 0)))
  //   (func $probe (export "probe") (block))
  //   (func (export "outer") (call $cb) (call $probe)))
  std::array<WasmEdge::Byte, 116> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x29, 0x02, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x04, 0x73,
      0x74, 0x6f, 0x70, 0x00, 0x00, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x14, 0x63,
      0x61, 0x6c, 0x6c, 0x5f, 0x69, 0x6e, 0x6e, 0x65, 0x72, 0x5f, 0x61, 0x6e,
      0x64, 0x5f, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x03, 0x04, 0x03,
      0x00, 0x00, 0x00, 0x07, 0x19, 0x03, 0x05, 0x69, 0x6e, 0x6e, 0x65, 0x72,
      0x00, 0x02, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x03, 0x05, 0x6f,
      0x75, 0x74, 0x65, 0x72, 0x00, 0x04, 0x0a, 0x18, 0x03, 0x09, 0x00, 0x10,
      0x00, 0x02, 0x40, 0x0c, 0x00, 0x0b, 0x0b, 0x05, 0x00, 0x02, 0x40, 0x0b,
      0x0b, 0x06, 0x00, 0x10, 0x01, 0x10, 0x03, 0x0b};
  StopHostModule HostMod;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.registerModule(HostMod));
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  ASSERT_EQ(
      VM.getActiveModule()->findFuncExports("inner")->isCompiledFunction(),
      Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT);
  auto Res = VM.execute("outer");
  EXPECT_TRUE(HostMod.callback().InnerInterrupted);
  EXPECT_TRUE(HostMod.callback().ProbeInterrupted);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::Interrupted);
  EXPECT_TRUE(VM.execute("probe"));
}

/// A stop request stays set for the whole run. A nested call that a
/// checkpoint inside the function interrupts does not clear it, so the next
/// nested call and the caller see it too.
TEST(ExecutorRegression, NestedCallKeepsStop) {
  checkNestedCallKeepsStop(Configure());
}

#ifdef WASMEDGE_USE_LLVM
TEST(ExecutorRegression, NestedCallKeepsStopJIT) {
  Configure Conf;
  Conf.getRuntimeConfigure().setRunMode(RunMode::JIT);
  Conf.getCompilerConfigure().setInterruptible(true);
  checkNestedCallKeepsStop(Conf);
}
#endif

/// Host function that calls an exported function on another executor.
class CallOther : public Runtime::HostFunction<CallOther> {
public:
  CallOther(Executor::Executor &E) : Exec(E) {}
  Expect<void> body(const Runtime::CallingFrame &) {
    return Exec.invoke(Func, {}, {}).map([](auto &&) {});
  }
  Executor::Executor &Exec;
  const Runtime::Instance::FunctionInstance *Func = nullptr;
};

/// Host function that stops another executor, calls an exported function on
/// it, and handles the error of that call.
class StopOtherAndCall : public Runtime::HostFunction<StopOtherAndCall> {
public:
  StopOtherAndCall(Executor::Executor &E) : Exec(E) {}
  Expect<void> body(const Runtime::CallingFrame &) {
    Exec.stop();
    auto Res = Exec.invoke(Func, {}, {});
    NestedInterrupted = !Res && Res.error() == ErrCode::Value::Interrupted;
    return {};
  }
  Executor::Executor &Exec;
  const Runtime::Instance::FunctionInstance *Func = nullptr;
  bool NestedInterrupted = false;
};

/// A call of executor A, through executor B, back into A joins the run of
/// the outer call of A, so a stop of A reaches it.
TEST(ExecutorRegression, ReentryThroughOtherExecutorSharesRun) {
  // (module
  //   (import "host" "call_b" (func $call_b))
  //   (func $probe (export "probe") (block))
  //   (func (export "outer") (call $call_b) (call $probe)))
  std::array<WasmEdge::Byte, 71> WasmA{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x0f, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x06, 0x63,
      0x61, 0x6c, 0x6c, 0x5f, 0x62, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00,
      0x07, 0x11, 0x02, 0x05, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x01, 0x05,
      0x6f, 0x75, 0x74, 0x65, 0x72, 0x00, 0x02, 0x0a, 0x0e, 0x02, 0x05, 0x00,
      0x02, 0x40, 0x0b, 0x0b, 0x06, 0x00, 0x10, 0x00, 0x10, 0x01, 0x0b};
  // (module
  //   (import "host" "stop_a_and_probe" (func $f))
  //   (func (export "run") (call $f)))
  std::array<WasmEdge::Byte, 62> WasmB{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01,
      0x60, 0x00, 0x00, 0x02, 0x19, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74,
      0x10, 0x73, 0x74, 0x6f, 0x70, 0x5f, 0x61, 0x5f, 0x61, 0x6e, 0x64,
      0x5f, 0x70, 0x72, 0x6f, 0x62, 0x65, 0x00, 0x00, 0x03, 0x02, 0x01,
      0x00, 0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x01, 0x0a,
      0x06, 0x01, 0x04, 0x00, 0x10, 0x00, 0x0b};
  Configure Conf;
  Loader::Loader Loader(Conf);
  Validator::Validator Validator(Conf);
  Executor::Executor ExecA(Conf);
  Executor::Executor ExecB(Conf);
  Runtime::StoreManager StoreA;
  Runtime::StoreManager StoreB;

  auto CallBP = std::make_unique<CallOther>(ExecB);
  auto &CallB = *CallBP;
  Runtime::Instance::ModuleInstance HostA("host");
  HostA.addHostFunc("call_b", std::move(CallBP));
  ASSERT_TRUE(ExecA.registerModule(StoreA, HostA));
  auto StopAP = std::make_unique<StopOtherAndCall>(ExecA);
  auto &StopA = *StopAP;
  Runtime::Instance::ModuleInstance HostB("host");
  HostB.addHostFunc("stop_a_and_probe", std::move(StopAP));
  ASSERT_TRUE(ExecB.registerModule(StoreB, HostB));

  auto ModA = Loader.parseModule(WasmA);
  ASSERT_TRUE(ModA);
  ASSERT_TRUE(Validator.validate(**ModA));
  auto InstA = ExecA.instantiateModule(StoreA, **ModA);
  ASSERT_TRUE(InstA);
  auto ModB = Loader.parseModule(WasmB);
  ASSERT_TRUE(ModB);
  ASSERT_TRUE(Validator.validate(**ModB));
  auto InstB = ExecB.instantiateModule(StoreB, **ModB);
  ASSERT_TRUE(InstB);
  CallB.Func = (*InstB)->findFuncExports("run");
  StopA.Func = (*InstA)->findFuncExports("probe");

  auto Res = ExecA.invoke((*InstA)->findFuncExports("outer"), {}, {});
  EXPECT_TRUE(StopA.NestedInterrupted);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::Interrupted);
}

/// Host function that returns 7, and stops the executor that calls it when
/// Stop is set.
class ReturnSeven : public Runtime::HostFunction<ReturnSeven> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame) {
    if (Stop) {
      Frame.getExecutor()->stop();
    }
    return UINT32_C(7);
  }
  bool Stop = false;
};

/// Host function that counts its calls.
class CountCalls : public Runtime::HostFunction<CountCalls> {
public:
  Expect<void> body(const Runtime::CallingFrame &, uint32_t) {
    ++Count;
    return {};
  }
  uint32_t Count = 0;
};

/// A call of a component function is one run. A stop request from its core
/// function makes the call fail, and the post-return function does not run.
TEST(ExecutorRegression, ComponentCallIsOneRun) {
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Executor::Executor Executor(Conf);
  Runtime::Instance::ModuleInstance HostMod("host");
  auto CoreP = std::make_unique<ReturnSeven>();
  auto &Core = *CoreP;
  HostMod.addHostFunc("core", std::move(CoreP));
  auto PostReturnP = std::make_unique<CountCalls>();
  auto &PostReturn = *PostReturnP;
  HostMod.addHostFunc("post_return", std::move(PostReturnP));
  const AST::Component::FuncType Type({},
                                      ComponentValType(ComponentTypeCode::U32));
  const Runtime::Instance::Component::FunctionInstance Func(
      Type, HostMod.findFuncExports("core"), nullptr, nullptr, nullptr,
      HostMod.findFuncExports("post_return"));

  auto Res = Executor.invoke(&Func, {}, {});
  ASSERT_TRUE(Res);
  ASSERT_EQ(Res->size(), 1U);
  EXPECT_EQ(std::get<uint32_t>((*Res)[0].first), 7U);
  EXPECT_EQ(PostReturn.Count, 1U);

  Core.Stop = true;
  Res = Executor.invoke(&Func, {}, {});
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::Interrupted);
  EXPECT_EQ(PostReturn.Count, 1U);
}

/// Host function that counts its calls.
class CountMarks : public Runtime::HostFunction<CountMarks> {
public:
  Expect<void> body(const Runtime::CallingFrame &) {
    ++Count;
    return {};
  }
  uint32_t Count = 0;
};

/// Instantiation of a component is one run. A stop request from the start
/// function of one core instance keeps the start function of the next core
/// instance from running.
TEST(ExecutorRegression, ComponentInstantiationIsOneRun) {
  // (component
  //   (type (instance (type (func)) (export "stop" (func (type 0)))
  //                   (export "mark" (func (type 0)))))
  //   (import "host" (instance (type 0)))
  //   (alias export 0 "stop" (func))
  //   (alias export 0 "mark" (func))
  //   (core func (canon lower (func 0)))
  //   (core func (canon lower (func 1)))
  //   (core instance (export "stop" (func 0)) (export "mark" (func 1)))
  //   (core module (import "host" "stop" (func $s)) (start $s))
  //   (core module (import "host" "mark" (func $m)) (start $m))
  //   (core instance (instantiate 0 (with "host" (instance 0))))
  //   (core instance (instantiate 1 (with "host" (instance 0)))))
  std::array<WasmEdge::Byte, 187> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x0d, 0x00, 0x01, 0x00, 0x07, 0x1a, 0x01, 0x42,
      0x03, 0x01, 0x40, 0x00, 0x01, 0x00, 0x04, 0x00, 0x04, 0x73, 0x74, 0x6f,
      0x70, 0x01, 0x00, 0x04, 0x00, 0x04, 0x6d, 0x61, 0x72, 0x6b, 0x01, 0x00,
      0x0a, 0x09, 0x01, 0x00, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x05, 0x00, 0x06,
      0x11, 0x02, 0x01, 0x00, 0x00, 0x04, 0x73, 0x74, 0x6f, 0x70, 0x01, 0x00,
      0x00, 0x04, 0x6d, 0x61, 0x72, 0x6b, 0x08, 0x09, 0x02, 0x01, 0x00, 0x00,
      0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x11, 0x01, 0x01, 0x02, 0x04, 0x73,
      0x74, 0x6f, 0x70, 0x00, 0x00, 0x04, 0x6d, 0x61, 0x72, 0x6b, 0x00, 0x01,
      0x01, 0x20, 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04,
      0x01, 0x60, 0x00, 0x00, 0x02, 0x0d, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74,
      0x04, 0x73, 0x74, 0x6f, 0x70, 0x00, 0x00, 0x08, 0x01, 0x00, 0x01, 0x20,
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x04, 0x01, 0x60,
      0x00, 0x00, 0x02, 0x0d, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x04, 0x6d,
      0x61, 0x72, 0x6b, 0x00, 0x00, 0x08, 0x01, 0x00, 0x02, 0x15, 0x02, 0x00,
      0x00, 0x01, 0x04, 0x68, 0x6f, 0x73, 0x74, 0x12, 0x00, 0x00, 0x01, 0x01,
      0x04, 0x68, 0x6f, 0x73, 0x74, 0x12, 0x00};
  Configure Conf;
  Conf.addProposal(Proposal::Component);
  Loader::Loader Loader(Conf);
  Validator::Validator Validator(Conf);
  Executor::Executor Executor(Conf);
  Runtime::StoreManager Store;

  Runtime::Instance::ModuleInstance HostMod("host");
  HostMod.addHostFunc("stop", std::make_unique<StopExecutor>());
  auto MarkP = std::make_unique<CountMarks>();
  auto &Mark = *MarkP;
  HostMod.addHostFunc("mark", std::move(MarkP));
  const AST::Component::FuncType Type({}, std::nullopt);
  Runtime::Instance::ComponentInstance HostComp("host");
  HostComp.addFunction(
      std::make_unique<Runtime::Instance::Component::FunctionInstance>(
          Type, HostMod.findFuncExports("stop"), nullptr, nullptr, nullptr));
  HostComp.exportFunction("stop", 0);
  HostComp.addFunction(
      std::make_unique<Runtime::Instance::Component::FunctionInstance>(
          Type, HostMod.findFuncExports("mark"), nullptr, nullptr, nullptr));
  HostComp.exportFunction("mark", 1);
  ASSERT_TRUE(Executor.registerComponent(Store, HostComp));

  auto Unit = Loader.parseWasmUnit(Wasm);
  ASSERT_TRUE(Unit);
  using CompPtr = std::unique_ptr<AST::Component::Component>;
  ASSERT_TRUE(std::holds_alternative<CompPtr>(*Unit));
  const auto &Comp = *std::get<CompPtr>(*Unit);
  ASSERT_TRUE(Validator.validate(Comp));
  auto Res = Executor.instantiateComponent(Store, Comp);
  ASSERT_FALSE(Res);
  EXPECT_EQ(Res.error(), ErrCode::Value::Interrupted);
  EXPECT_EQ(Mark.Count, 0U);
}

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
