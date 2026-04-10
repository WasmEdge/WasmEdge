// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

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

#include "common/spdlog.h"
#include "common/types.h"
#include "vm/vm.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
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
///     i32.const 0  table.get 0  ref.test eqref   call $check
///     i32.const 0  table.get 0  ref.test (ref $s) call $check
///   ))
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
///     i32.const 0  table.get 0)
///   (func (export "test")
///     call $get_null  ref.test eqref  call $check
///   ))
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
///     ref.null $s  i32.const 1  table.grow $t  drop
///     i32.const 0  table.get $t  ref.cast eqref  ref.is_null
///     call $check
///   ))
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
///     i32.const 0  table.get 0  ref.cast eqref
///     ref.is_null  call $check
///   ))
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
///     i32.const 0  table.get 0  ref.cast (ref eq)  drop))
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
///       i32.const 0  table.get 0
///       br_on_cast $taken eqref eqref
///       drop  i32.const 0  call $check  return)
///     ref.is_null  call $check
///     (block $fail_taken (result eqref)
///       i32.const 0  table.get 0
///       br_on_cast_fail $fail_taken eqref (ref eq)
///       drop  i32.const 0  call $check  return)
///     ref.is_null  call $check
///   ))
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
///     local.get 0  ref.test (ref null $s)  call $check
///     local.get 0  ref.test eqref          call $check
///     local.get 0  ref.test (ref $s)       call $check
///   ))
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
///     local.get 0  ref.test (ref null func)  call $check
///     local.get 0  ref.test (ref $f)         call $check
///   ))
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
///     local.get 0  ref.test eqref    call $check
///     local.get 0  ref.test (ref $a) call $check
///   ))
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
///     global.get $g  ref.test eqref    call $check
///     global.get $g  ref.test (ref $s) call $check
///   ))
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
// clang-format on

/// Regression test for ref.test on externalized nullable references.
///
/// The bug: runRefTestOp always created non-nullable types for externalized
/// references (TypeCode::Ref), discarding the original nullability info.
/// The fix preserves nullability by checking isNullableRefType().
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
  // When a non-null anyref is externalized via extern.convert_any, the
  // externalized flag is set on the value. The ref.test instruction must
  // correctly handle this externalized type when matching against both
  // nullable and non-nullable extern targets.
  //
  // Note: The runtime parameter-pushing code converts non-null nullable refs
  // to non-nullable before execution. So the externalized type at ref.test
  // time will be non-nullable. Both ref.test targets should succeed.
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
    // non-nullable extern target (the externalized type is (ref extern)
    // because the runtime made it non-nullable before execution).
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
  // RefVariant with type (ref null noextern) without setting the externalized
  // flag. ref.test follows the normal (non-externalized) code path.
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
  // In this path, any.convert_extern (internalize) converts a non-null
  // externref to (ref any) (non-nullable), then extern.convert_any
  // (externalize) sets the externalized flag on this non-nullable type.
  // The fix correctly preserves this non-nullability. Both nullable and
  // non-nullable ref.test targets should match.
  {
    int DummyObj = 42;
    RefVariant ExternRefVal(&DummyObj);

    std::vector<ValVariant> Params = {ValVariant(ExternRefVal)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::ExternRef)};

    // Test 5: internalize + externalize + ref.test (ref null extern)
    // After internalize, type is (ref any) (non-nullable).
    // After externalize, type is (ref any) + externalized.
    // The fix normalizes this to (ref extern), matching (ref null extern).
    {
      auto Result = VM.execute("test_ext_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Round-trip externalized externref should match (ref null extern)";
    }

    // Test 6: internalize + externalize + ref.test (ref extern)
    // Same path, normalized to (ref extern) which matches (ref extern).
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
  // Neither step sets the externalized flag on the value.
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

  // --- Part 5: Direct ValType nullability preservation logic ---
  //
  // Verifies the same condition checked in runRefTestOp without going
  // through the full execution pipeline.
  {
    // Simulate a nullable externalized type (ref null any) + externalized flag.
    ValType NullableAny(TypeCode::RefNull, TypeCode::AnyRef);
    NullableAny.setExternalized();

    ASSERT_TRUE(NullableAny.isExternalized());
    ASSERT_TRUE(NullableAny.isNullableRefType());

    // The fix: construct the normalized type preserving nullability.
    ValType NormalizedNullable(
        NullableAny.isNullableRefType() ? TypeCode::RefNull : TypeCode::Ref,
        TypeCode::ExternRef);
    EXPECT_TRUE(NormalizedNullable.isNullableRefType())
        << "Normalized type should preserve nullable from source";
    EXPECT_FALSE(NormalizedNullable.isExternalized())
        << "Normalized type should not have externalize flag";

    // Simulate a non-nullable externalized type (ref any) + externalized flag.
    ValType NonNullableAny(TypeCode::Ref, TypeCode::AnyRef);
    NonNullableAny.setExternalized();

    ASSERT_TRUE(NonNullableAny.isExternalized());
    ASSERT_FALSE(NonNullableAny.isNullableRefType());

    // The fix: non-nullable should remain non-nullable.
    ValType NormalizedNonNullable(
        NonNullableAny.isNullableRefType() ? TypeCode::RefNull : TypeCode::Ref,
        TypeCode::ExternRef);
    EXPECT_FALSE(NormalizedNonNullable.isNullableRefType())
        << "Normalized type should preserve non-nullable from source";

    // Old code (the bug) would always use TypeCode::Ref:
    ValType OldBugResult(TypeCode::Ref, TypeCode::ExternRef);
    EXPECT_FALSE(OldBugResult.isNullableRefType())
        << "Old code always produced non-nullable";

    // Verify the difference: for a nullable input, old and new code differ.
    EXPECT_NE(NormalizedNullable.isNullableRefType(),
              OldBugResult.isNullableRefType())
        << "Fix should produce different result than bug for nullable input";

    // Verify: for a non-nullable input, old and new code agree.
    EXPECT_EQ(NormalizedNonNullable.isNullableRefType(),
              OldBugResult.isNullableRefType())
        << "Fix should produce same result as bug for non-nullable input";
  }

  // --- Part 6: ref.test on null from concrete-typed table (issue #4757) ---
  //
  // table.get on a concrete-typed table (e.g. `(table N (ref null $s))`)
  // returned null references that retained the concrete type. When ref.test
  // received such a reference, the code entered the `!VT.isAbsHeapType()`
  // branch and dereferenced the null object pointer, causing a segfault.
  {
    Configure Conf6;
    VM::VM VM6(Conf6);
    GCCheckModule GCMod;
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

  // --- Part 7: function returning null ref with concrete type ---
  {
    Configure Conf7;
    VM::VM VM7(Conf7);
    GCCheckModule GCMod;
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

  // --- Part 8: table.grow creates null entries, then ref.cast ---
  {
    Configure Conf8;
    VM::VM VM8(Conf8);
    GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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
  VM::VM VM(Conf);
  GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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
    VM::VM VM(Conf);
    GCCheckModule GCMod;
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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
