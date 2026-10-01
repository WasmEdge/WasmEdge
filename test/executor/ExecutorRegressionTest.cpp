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
#include <vector>

namespace {

using namespace std::literals;
using namespace WasmEdge;

// Host function that records i32 values for later inspection.
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

// Module that provides a "check" host function under the "gc" namespace.
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

// Host function that records its i32 argument and returns it incremented.
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

// Module that provides the "add_one" host function under the "host" namespace.
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

// Host function that does nothing.
class Nop : public Runtime::HostFunction<Nop> {
public:
  Expect<void> body(const Runtime::CallingFrame &) { return {}; }
};

// Packs the bit patterns of float lanes into a v128, Wasm lane 0 first.
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

// Expects each lane of Got to match Want. A NaN in Want accepts only an
// arithmetic NaN, which is a NaN with the quiet bit set.
template <typename T>
void expectLanes(std::string_view Mode, std::string_view Op, uint128_t Got,
                 std::initializer_list<FloatBits<T>> Want) {
  unsigned int Shift = 0;
  for (const auto WantBits : Want) {
    const auto GotBits =
        static_cast<FloatBits<T>>(static_cast<uint64_t>(Got >> Shift));
    if (std::isnan(cxx20::bit_cast<T>(WantBits))) {
      EXPECT_TRUE(std::isnan(cxx20::bit_cast<T>(GotBits)) &&
                  (GotBits & QuietBit<T>) != 0)
          << Mode << ' ' << Op << " lane bits " << std::hex << GotBits;
    } else {
      EXPECT_EQ(GotBits, WantBits) << Mode << ' ' << Op;
    }
    Shift += sizeof(T) * 8;
  }
}

// ref.test on externalized refs must keep the nullability of the source.
TEST(ExecutorRegressionTest, RefTestExternalizedNullability) {
  // (module
  //   (type (;0;) (func (param anyref) (result i32)))
  //   (type (;1;) (func (param externref) (result i32)))
  //
  //   ;; Func 0: externalize anyref, then ref.test (ref null extern)
  //   (func (;0;) (type 0) (param anyref) (result i32)
  //     local.get 0
  //     extern.convert_any
  //     ref.test (ref null extern)
  //   )
  //
  //   ;; Func 1: externalize anyref, then ref.test (ref extern)
  //   (func (;1;) (type 0) (param anyref) (result i32)
  //     local.get 0
  //     extern.convert_any
  //     ref.test (ref extern)
  //   )
  //
  //   ;; Func 2: internalize externref, externalize, ref.test (ref null extern)
  //   (func (;2;) (type 1) (param externref) (result i32)
  //     local.get 0
  //     any.convert_extern
  //     extern.convert_any
  //     ref.test (ref null extern)
  //   )
  //
  //   ;; Func 3: internalize externref, externalize, ref.test (ref extern)
  //   (func (;3;) (type 1) (param externref) (result i32)
  //     local.get 0
  //     any.convert_extern
  //     extern.convert_any
  //     ref.test (ref extern)
  //   )
  //
  //   (export "test_nullable" (func 0))
  //   (export "test_nonnull" (func 1))
  //   (export "test_ext_nullable" (func 2))
  //   (export "test_ext_nonnull" (func 3))
  // )
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

  WasmEdge::Configure Conf;
  WasmEdge::VM::VM VM(Conf);

  ASSERT_TRUE(VM.loadWasm(RefTestNullabilityWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // Non-null anyref: the pushed param is made non-nullable, so the externalized
  // value matches both targets.
  {
    int DummyObj = 42;
    RefVariant AnyRefVal(ValType(TypeCode::RefNull, TypeCode::AnyRef),
                         reinterpret_cast<void *>(&DummyObj));

    std::vector<ValVariant> Params = {ValVariant(AnyRefVal)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::AnyRef)};

    // extern.convert_any + ref.test (ref null extern)
    {
      auto Result = VM.execute("test_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Non-null externalized anyref should match (ref null extern)";
    }

    // extern.convert_any + ref.test (ref extern)
    {
      auto Result = VM.execute("test_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Non-null externalized anyref should match (ref extern)";
    }
  }

  // Null anyref: externalized to (ref null noextern) without the externalized
  // flag, so it matches only the nullable target.
  {
    RefVariant NullAnyRef(ValType(TypeCode::RefNull, TypeCode::NullRef));

    std::vector<ValVariant> Params = {ValVariant(NullAnyRef)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::AnyRef)};

    // extern.convert_any on null + ref.test (ref null extern)
    {
      auto Result = VM.execute("test_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Null externalized anyref should match (ref null extern)";
    }

    // extern.convert_any on null + ref.test (ref extern)
    {
      auto Result = VM.execute("test_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 0U)
          << "Null externalized anyref should NOT match (ref extern)";
    }
  }

  // Non-null externref round-trip: internalized to (ref any), externalized
  // back to (ref extern), so it matches both targets.
  {
    int DummyObj = 42;
    RefVariant ExternRefVal(&DummyObj);

    std::vector<ValVariant> Params = {ValVariant(ExternRefVal)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::ExternRef)};

    // internalize + externalize + ref.test (ref null extern)
    {
      auto Result = VM.execute("test_ext_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Round-trip externalized externref should match (ref null extern)";
    }

    // internalize + externalize + ref.test (ref extern)
    {
      auto Result = VM.execute("test_ext_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Round-trip externalized externref should match (ref extern)";
    }
  }

  // Null externref round-trip: (ref null none), then (ref null noextern),
  // without the externalized flag.
  {
    RefVariant NullExternRef(
        ValType(TypeCode::RefNull, TypeCode::NullExternRef));

    std::vector<ValVariant> Params = {ValVariant(NullExternRef)};
    std::vector<ValType> ParamTypes = {ValType(TypeCode::ExternRef)};

    // null externref round-trip + ref.test (ref null extern)
    {
      auto Result = VM.execute("test_ext_nullable", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U)
          << "Null round-trip externref should match (ref null extern)";
    }

    // null externref round-trip + ref.test (ref extern)
    {
      auto Result = VM.execute("test_ext_nonnull", Params, ParamTypes);
      ASSERT_TRUE(Result);
      ASSERT_EQ((*Result).size(), 1U);
      EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 0U)
          << "Null round-trip externref should NOT match (ref extern)";
    }
  }
}

// ref.test, ref.cast, and br_on_cast on a null from a concrete-typed table
// must not dereference the null object (issue #4757).
TEST(ExecutorRegressionTest, NullFromConcreteTable) {
  // (module
  //   (type $s (struct))
  //   (import "gc" "check" (func $check (param i32)))
  //   (table $t (export "t") 1 (ref null $s))
  //   (table $g 0 (ref null $s))
  //   (func $get_null (result (ref null $s))
  //     i32.const 0
  //     table.get $t)
  //   (func (export "ref_test")
  //     i32.const 0
  //     table.get $t
  //     ref.test eqref
  //     call $check
  //     i32.const 0
  //     table.get $t
  //     ref.test (ref $s)
  //     call $check
  //     call $get_null
  //     ref.test eqref
  //     call $check)
  //   (func (export "ref_cast")
  //     i32.const 0
  //     table.get $t
  //     ref.cast eqref
  //     ref.is_null
  //     call $check
  //     ref.null $s
  //     i32.const 1
  //     table.grow $g
  //     drop
  //     i32.const 0
  //     table.get $g
  //     ref.cast eqref
  //     ref.is_null
  //     call $check)
  //   (func (export "ref_cast_non_null")
  //     i32.const 0
  //     table.get $t
  //     ref.cast (ref eq)
  //     drop)
  //   (func (export "br_on_cast")
  //     (block $taken (result eqref)
  //       i32.const 0
  //       table.get $t
  //       br_on_cast $taken eqref eqref
  //       drop
  //       i32.const 0
  //       call $check
  //       return)
  //     ref.is_null
  //     call $check
  //     (block $fail_taken (result eqref)
  //       i32.const 0
  //       table.get $t
  //       br_on_cast_fail $fail_taken eqref (ref eq)
  //       drop
  //       i32.const 0
  //       call $check
  //       return)
  //     ref.is_null
  //     call $check))
  std::array<WasmEdge::Byte, 247> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x04, 0x5f,
      0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x63, 0x00, 0x60, 0x00,
      0x00, 0x02, 0x0c, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63,
      0x6b, 0x00, 0x01, 0x03, 0x06, 0x05, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04,
      0x09, 0x02, 0x63, 0x00, 0x00, 0x01, 0x63, 0x00, 0x00, 0x00, 0x07, 0x3c,
      0x05, 0x01, 0x74, 0x01, 0x00, 0x08, 0x72, 0x65, 0x66, 0x5f, 0x74, 0x65,
      0x73, 0x74, 0x00, 0x02, 0x08, 0x72, 0x65, 0x66, 0x5f, 0x63, 0x61, 0x73,
      0x74, 0x00, 0x03, 0x11, 0x72, 0x65, 0x66, 0x5f, 0x63, 0x61, 0x73, 0x74,
      0x5f, 0x6e, 0x6f, 0x6e, 0x5f, 0x6e, 0x75, 0x6c, 0x6c, 0x00, 0x04, 0x0a,
      0x62, 0x72, 0x5f, 0x6f, 0x6e, 0x5f, 0x63, 0x61, 0x73, 0x74, 0x00, 0x05,
      0x0a, 0x7d, 0x05, 0x06, 0x00, 0x41, 0x00, 0x25, 0x00, 0x0b, 0x1b, 0x00,
      0x41, 0x00, 0x25, 0x00, 0xfb, 0x15, 0x6d, 0x10, 0x00, 0x41, 0x00, 0x25,
      0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x10, 0x01, 0xfb, 0x15, 0x6d, 0x10,
      0x00, 0x0b, 0x1e, 0x00, 0x41, 0x00, 0x25, 0x00, 0xfb, 0x17, 0x6d, 0xd1,
      0x10, 0x00, 0xd0, 0x00, 0x41, 0x01, 0xfc, 0x0f, 0x01, 0x1a, 0x41, 0x00,
      0x25, 0x01, 0xfb, 0x17, 0x6d, 0xd1, 0x10, 0x00, 0x0b, 0x0a, 0x00, 0x41,
      0x00, 0x25, 0x00, 0xfb, 0x16, 0x6d, 0x1a, 0x0b, 0x2e, 0x00, 0x02, 0x6d,
      0x41, 0x00, 0x25, 0x00, 0xfb, 0x18, 0x03, 0x00, 0x6d, 0x6d, 0x1a, 0x41,
      0x00, 0x10, 0x00, 0x0f, 0x0b, 0xd1, 0x10, 0x00, 0x02, 0x6d, 0x41, 0x00,
      0x25, 0x00, 0xfb, 0x19, 0x01, 0x00, 0x6d, 0x6d, 0x1a, 0x41, 0x00, 0x10,
      0x00, 0x0f, 0x0b, 0xd1, 0x10, 0x00, 0x0b};

  Configure Conf;
  GCCheckModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  // The null in exported table "t" must have an abstract heap type, which
  // catches issue #4757 without running wasm that would segfault.
  const auto *ModInst = VM.getActiveModule();
  ASSERT_NE(ModInst, nullptr);
  auto *TabInst = ModInst->findTableExports("t");
  ASSERT_NE(TabInst, nullptr);
  auto RefRes = TabInst->getRefAddr(0);
  ASSERT_TRUE(RefRes);
  ASSERT_TRUE(RefRes->isNull());
  ASSERT_TRUE(RefRes->getType().isAbsHeapType())
      << "Null reference in concrete-typed table must be normalized to an "
         "abstract heap type (issue #4757)";

  // The null matches eqref but not (ref $s), also when returned by a call.
  ASSERT_TRUE(VM.execute("ref_test"));
  // ref.cast eqref passes the null through, also from a table.grow slot.
  ASSERT_TRUE(VM.execute("ref_cast"));
  // br_on_cast is taken for eqref, br_on_cast_fail is taken for (ref eq).
  ASSERT_TRUE(VM.execute("br_on_cast"));
  auto Values = GCMod.getValues();
  ASSERT_EQ(Values.size(), 7U);
  EXPECT_EQ(Values[0], 1U);
  EXPECT_EQ(Values[1], 0U);
  EXPECT_EQ(Values[2], 1U);
  EXPECT_EQ(Values[3], 1U);
  EXPECT_EQ(Values[4], 1U);
  EXPECT_EQ(Values[5], 1U);
  EXPECT_EQ(Values[6], 1U);

  // ref.cast (ref eq) on the null traps.
  auto Result = VM.execute("ref_cast_non_null");
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), ErrCode::Value::CastFailed);
}

// Null locals and globals of concrete (issue #4768) and abstract (issue #5343)
// ref types must be bottom-typed so that ref.test and ref.cast match.
TEST(ExecutorRegressionTest, NullLocalIsBottomTyped) {
  // (module
  //   (type $s (struct))
  //   (type $f (func))
  //   (type $a (array i32))
  //   (import "gc" "check" (func $check (param i32)))
  //   (global $g (ref null $s) ref.null $s)
  //   (func (export "struct_local") (local (ref null $s))
  //     local.get 0
  //     ref.test (ref null $s)
  //     call $check
  //     local.get 0
  //     ref.test eqref
  //     call $check
  //     local.get 0
  //     ref.test (ref $s)
  //     call $check)
  //   (func (export "func_local") (local (ref null $f))
  //     local.get 0
  //     ref.test (ref null func)
  //     call $check
  //     local.get 0
  //     ref.test (ref $f)
  //     call $check)
  //   (func (export "array_local") (local (ref null $a))
  //     local.get 0
  //     ref.test eqref
  //     call $check
  //     local.get 0
  //     ref.test (ref $a)
  //     call $check)
  //   (func (export "struct_global")
  //     global.get $g
  //     ref.test eqref
  //     call $check
  //     global.get $g
  //     ref.test (ref $s)
  //     call $check))
  std::array<WasmEdge::Byte, 201> ConcreteWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0d, 0x04, 0x5f,
      0x00, 0x60, 0x00, 0x00, 0x5e, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02,
      0x0c, 0x01, 0x02, 0x67, 0x63, 0x05, 0x63, 0x68, 0x65, 0x63, 0x6b, 0x00,
      0x03, 0x03, 0x05, 0x04, 0x01, 0x01, 0x01, 0x01, 0x06, 0x07, 0x01, 0x63,
      0x00, 0x00, 0xd0, 0x00, 0x0b, 0x07, 0x3b, 0x04, 0x0c, 0x73, 0x74, 0x72,
      0x75, 0x63, 0x74, 0x5f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x01, 0x0a,
      0x66, 0x75, 0x6e, 0x63, 0x5f, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00, 0x02,
      0x0b, 0x61, 0x72, 0x72, 0x61, 0x79, 0x5f, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
      0x00, 0x03, 0x0d, 0x73, 0x74, 0x72, 0x75, 0x63, 0x74, 0x5f, 0x67, 0x6c,
      0x6f, 0x62, 0x61, 0x6c, 0x00, 0x04, 0x0a, 0x55, 0x04, 0x1a, 0x01, 0x01,
      0x63, 0x00, 0x20, 0x00, 0xfb, 0x15, 0x00, 0x10, 0x00, 0x20, 0x00, 0xfb,
      0x15, 0x6d, 0x10, 0x00, 0x20, 0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b,
      0x13, 0x01, 0x01, 0x63, 0x01, 0x20, 0x00, 0xfb, 0x15, 0x70, 0x10, 0x00,
      0x20, 0x00, 0xfb, 0x14, 0x01, 0x10, 0x00, 0x0b, 0x13, 0x01, 0x01, 0x63,
      0x02, 0x20, 0x00, 0xfb, 0x15, 0x6d, 0x10, 0x00, 0x20, 0x00, 0xfb, 0x14,
      0x02, 0x10, 0x00, 0x0b, 0x10, 0x00, 0x23, 0x00, 0xfb, 0x15, 0x6d, 0x10,
      0x00, 0x23, 0x00, 0xfb, 0x14, 0x00, 0x10, 0x00, 0x0b};

  Configure Conf;
  GCCheckModule GCMod;
  VM::VM VM(Conf);
  VM.registerModule(GCMod);
  ASSERT_TRUE(VM.loadWasm(ConcreteWasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  ASSERT_TRUE(VM.execute("struct_local"));
  ASSERT_TRUE(VM.execute("func_local"));
  ASSERT_TRUE(VM.execute("array_local"));
  ASSERT_TRUE(VM.execute("struct_global"));
  // Each null matches the nullable target but not the non-nullable one.
  auto Values = GCMod.getValues();
  const std::vector<uint32_t> Expected = {1, 1, 0, 1, 0, 1, 0, 1, 0};
  EXPECT_EQ(std::vector<uint32_t>(Values.begin(), Values.end()), Expected);

  // (module
  //   (type $t (struct (field (mut structref))))
  //   (type $a (array i32))
  //   (type $f (func))
  //   (type $r (func (result i32)))
  //   (func (export "test_struct") (type $r) (local structref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_struct_nonnull") (type $r) (local structref)
  //     local.get 0 ref.test (ref $t))
  //   (func (export "cast_struct") (type $r) (local structref)
  //     local.get 0 ref.cast (ref null $t) drop i32.const 7)
  //   (func (export "test_any") (type $r) (local anyref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_i31") (type $r) (local i31ref)
  //     local.get 0 ref.test (ref null $t))
  //   (func (export "test_array") (type $r) (local arrayref)
  //     local.get 0 ref.test (ref null $a))
  //   (func (export "test_func") (type $r) (local funcref)
  //     local.get 0 ref.test (ref null $f))
  //   (func (export "test_func_nonnull") (type $r) (local funcref)
  //     local.get 0 ref.test (ref $f))
  //   (func (export "cast_func") (type $r) (local funcref)
  //     local.get 0 ref.cast (ref null $f) drop i32.const 7)
  //   (func (export "test_extern") (type $r) (local externref)
  //     local.get 0 ref.test (ref null noextern)))
  std::array<WasmEdge::Byte, 294> AbstractWasm{
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

  VM::VM AbstractVM(Conf);
  ASSERT_TRUE(AbstractVM.loadWasm(AbstractWasm));
  ASSERT_TRUE(AbstractVM.validate());
  ASSERT_TRUE(AbstractVM.instantiate());

  // A null structref local bottoms out at none: it matches (ref null $t) but
  // not (ref $t), and ref.cast to (ref null $t) must not trap.
  auto RTestStruct = AbstractVM.execute("test_struct");
  ASSERT_TRUE(RTestStruct);
  ASSERT_EQ(RTestStruct->size(), 1U);
  EXPECT_EQ((*RTestStruct)[0].first.get<uint32_t>(), 1U);

  auto RTestStructNonNull = AbstractVM.execute("test_struct_nonnull");
  ASSERT_TRUE(RTestStructNonNull);
  ASSERT_EQ(RTestStructNonNull->size(), 1U);
  EXPECT_EQ((*RTestStructNonNull)[0].first.get<uint32_t>(), 0U);

  auto RCastStruct = AbstractVM.execute("cast_struct");
  ASSERT_TRUE(RCastStruct) << "ref.cast trapped on a null structref local";
  ASSERT_EQ(RCastStruct->size(), 1U);
  EXPECT_EQ((*RCastStruct)[0].first.get<uint32_t>(), 7U);

  // The anyref, i31ref, and arrayref null locals bottom out at none as well.
  auto RTestAny = AbstractVM.execute("test_any");
  ASSERT_TRUE(RTestAny);
  ASSERT_EQ(RTestAny->size(), 1U);
  EXPECT_EQ((*RTestAny)[0].first.get<uint32_t>(), 1U);

  auto RTestI31 = AbstractVM.execute("test_i31");
  ASSERT_TRUE(RTestI31);
  ASSERT_EQ(RTestI31->size(), 1U);
  EXPECT_EQ((*RTestI31)[0].first.get<uint32_t>(), 1U);

  auto RTestArray = AbstractVM.execute("test_array");
  ASSERT_TRUE(RTestArray);
  ASSERT_EQ(RTestArray->size(), 1U);
  EXPECT_EQ((*RTestArray)[0].first.get<uint32_t>(), 1U);

  // A null funcref local bottoms out at nofunc: it matches (ref null $f) but
  // not (ref $f), and ref.cast to (ref null $f) must not trap.
  auto RTestFunc = AbstractVM.execute("test_func");
  ASSERT_TRUE(RTestFunc);
  ASSERT_EQ(RTestFunc->size(), 1U);
  EXPECT_EQ((*RTestFunc)[0].first.get<uint32_t>(), 1U);

  auto RTestFuncNonNull = AbstractVM.execute("test_func_nonnull");
  ASSERT_TRUE(RTestFuncNonNull);
  ASSERT_EQ(RTestFuncNonNull->size(), 1U);
  EXPECT_EQ((*RTestFuncNonNull)[0].first.get<uint32_t>(), 0U);

  auto RCastFunc = AbstractVM.execute("cast_func");
  ASSERT_TRUE(RCastFunc) << "ref.cast trapped on a null funcref local";
  ASSERT_EQ(RCastFunc->size(), 1U);
  EXPECT_EQ((*RCastFunc)[0].first.get<uint32_t>(), 7U);

  // A null externref local bottoms out at noextern.
  auto RTestExtern = AbstractVM.execute("test_extern");
  ASSERT_TRUE(RTestExtern);
  ASSERT_EQ(RTestExtern->size(), 1U);
  EXPECT_EQ((*RTestExtern)[0].first.get<uint32_t>(), 1U);
}

// throw_ref must rethrow the captured payload, and catch_all and
// catch_all_ref must drop it, leaving only the value pushed before the throw.
TEST(ExecutorRegressionTest, ExceptionPayload) {
  // (module
  //   (type $point (struct (field i32)))
  //   (tag $null (param (ref null $point)))
  //   (tag $pair (param i32 i64))
  //   (tag $one (param i32))
  //   (func (export "throw_ref_null") (result i32)
  //     (local $exn exnref)
  //     (block $h (result (ref null $point) exnref)
  //       (try_table (catch_ref $null $h)
  //         ref.null $point
  //         throw $null)
  //       unreachable)
  //     local.set $exn
  //     drop
  //     (block $h2 (result (ref null $point))
  //       (try_table (catch $null $h2)
  //         local.get $exn
  //         throw_ref)
  //       unreachable)
  //     struct.get $point 0)
  //   (func (export "throw_ref_pair") (result i32)
  //     (local $exn exnref)
  //     (block $h1 (result i32 i64 exnref)
  //       (try_table (catch_ref $pair $h1)
  //         i32.const 7
  //         i64.const 1000
  //         throw $pair)
  //       unreachable)
  //     local.set $exn
  //     drop
  //     drop
  //     (block $h2 (result i32 i64)
  //       (try_table (catch $pair $h2)
  //         local.get $exn
  //         throw_ref)
  //       unreachable)
  //     i32.wrap_i64
  //     i32.add)
  //   (func (export "catch_all") (result i32)
  //     i32.const 1
  //     (block $l
  //       (try_table (catch_all $l)
  //         i32.const 42
  //         throw $one)
  //       unreachable))
  //   (func (export "catch_all_ref") (result i32)
  //     i32.const 1
  //     (block $l (result exnref)
  //       (try_table (catch_all_ref $l)
  //         i32.const 42
  //         throw $one)
  //       unreachable)
  //     drop))
  std::array<WasmEdge::Byte, 258> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x28, 0x08, 0x5f,
      0x01, 0x7f, 0x00, 0x60, 0x01, 0x63, 0x00, 0x00, 0x60, 0x02, 0x7f, 0x7e,
      0x00, 0x60, 0x01, 0x7f, 0x00, 0x60, 0x00, 0x01, 0x7f, 0x60, 0x00, 0x02,
      0x63, 0x00, 0x69, 0x60, 0x00, 0x03, 0x7f, 0x7e, 0x69, 0x60, 0x00, 0x02,
      0x7f, 0x7e, 0x03, 0x05, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0d, 0x07, 0x03,
      0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x07, 0x3f, 0x04, 0x0e, 0x74, 0x68,
      0x72, 0x6f, 0x77, 0x5f, 0x72, 0x65, 0x66, 0x5f, 0x6e, 0x75, 0x6c, 0x6c,
      0x00, 0x00, 0x0e, 0x74, 0x68, 0x72, 0x6f, 0x77, 0x5f, 0x72, 0x65, 0x66,
      0x5f, 0x70, 0x61, 0x69, 0x72, 0x00, 0x01, 0x09, 0x63, 0x61, 0x74, 0x63,
      0x68, 0x5f, 0x61, 0x6c, 0x6c, 0x00, 0x02, 0x0d, 0x63, 0x61, 0x74, 0x63,
      0x68, 0x5f, 0x61, 0x6c, 0x6c, 0x5f, 0x72, 0x65, 0x66, 0x00, 0x03, 0x0a,
      0x7d, 0x04, 0x29, 0x01, 0x01, 0x69, 0x02, 0x05, 0x1f, 0x40, 0x01, 0x01,
      0x00, 0x00, 0xd0, 0x00, 0x08, 0x00, 0x0b, 0x00, 0x0b, 0x21, 0x00, 0x1a,
      0x02, 0x63, 0x00, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x0a,
      0x0b, 0x00, 0x0b, 0xfb, 0x02, 0x00, 0x00, 0x0b, 0x2a, 0x01, 0x01, 0x69,
      0x02, 0x06, 0x1f, 0x40, 0x01, 0x01, 0x01, 0x00, 0x41, 0x07, 0x42, 0xe8,
      0x07, 0x08, 0x01, 0x0b, 0x00, 0x0b, 0x21, 0x00, 0x1a, 0x1a, 0x02, 0x07,
      0x1f, 0x40, 0x01, 0x00, 0x01, 0x00, 0x20, 0x00, 0x0a, 0x0b, 0x00, 0x0b,
      0xa7, 0x6a, 0x0b, 0x12, 0x00, 0x41, 0x01, 0x02, 0x40, 0x1f, 0x40, 0x01,
      0x02, 0x00, 0x41, 0x2a, 0x08, 0x02, 0x0b, 0x00, 0x0b, 0x0b, 0x13, 0x00,
      0x41, 0x01, 0x02, 0x69, 0x1f, 0x40, 0x01, 0x03, 0x00, 0x41, 0x2a, 0x08,
      0x02, 0x0b, 0x00, 0x0b, 0x1a, 0x0b};

  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // The rethrown null payload traps at struct.get.
  auto NullPayload = VM.execute("throw_ref_null");
  ASSERT_FALSE(NullPayload);
  EXPECT_EQ(NullPayload.error(), ErrCode::Value::AccessNullStruct);
  // The rethrown (i32 i64) payload yields 7 + 1000.
  auto Pair = VM.execute("throw_ref_pair");
  ASSERT_TRUE(Pair);
  ASSERT_EQ(Pair->size(), 1U);
  EXPECT_EQ((*Pair)[0].first.get<uint32_t>(), 1007U);
  for (const auto Name : {"catch_all"sv, "catch_all_ref"sv}) {
    auto Result = VM.execute(Name);
    ASSERT_TRUE(Result) << Name;
    ASSERT_EQ(Result->size(), 1U) << Name;
    EXPECT_EQ((*Result)[0].first.get<uint32_t>(), 1U) << Name;
  }
}

// A throw reaching stale try_table handlers, buried by a later try_table or
// piled up by a loop, must not corrupt the value stack (use ASan to see it).
TEST(ExecutorRegressionTest, StaleTryTableHandlers) {
  // (module
  //   (tag $tx)
  //   (tag $ty (param i32 i32))
  //   (func (export "buried")
  //     (block $outer
  //       i32.const 1
  //       i32.const 1
  //       i32.const 1
  //       i32.const 1
  //       (try_table (catch $tx $outer)
  //         br $outer)
  //       unreachable)
  //     (try_table (catch $tx 0)
  //       i32.const 7
  //       i32.const 8
  //       throw $ty))
  //   (func (export "pile") (local $i i32)
  //     (loop $l
  //       (block $out
  //         i32.const 1
  //         i32.const 1
  //         i32.const 1
  //         i32.const 1
  //         (try_table (catch $tx $out)
  //           br $out)
  //         unreachable)
  //       (local.set $i (i32.add (local.get $i) (i32.const 1)))
  //       (br_if $l (i32.lt_u (local.get $i) (i32.const 3))))
  //     (try_table (catch $tx 0)
  //       i32.const 7
  //       i32.const 8
  //       throw $ty)))
  std::array<WasmEdge::Byte, 146> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02, 0x60,
      0x00, 0x00, 0x60, 0x02, 0x7f, 0x7f, 0x00, 0x03, 0x03, 0x02, 0x00, 0x00,
      0x0d, 0x05, 0x02, 0x00, 0x00, 0x00, 0x01, 0x07, 0x11, 0x02, 0x06, 0x62,
      0x75, 0x72, 0x69, 0x65, 0x64, 0x00, 0x00, 0x04, 0x70, 0x69, 0x6c, 0x65,
      0x00, 0x01, 0x0a, 0x5e, 0x02, 0x24, 0x00, 0x02, 0x40, 0x41, 0x01, 0x41,
      0x01, 0x41, 0x01, 0x41, 0x01, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x0c,
      0x01, 0x0b, 0x00, 0x0b, 0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x41, 0x07,
      0x41, 0x08, 0x08, 0x01, 0x0b, 0x0b, 0x37, 0x01, 0x01, 0x7f, 0x03, 0x40,
      0x02, 0x40, 0x41, 0x01, 0x41, 0x01, 0x41, 0x01, 0x41, 0x01, 0x1f, 0x40,
      0x01, 0x00, 0x00, 0x00, 0x0c, 0x01, 0x0b, 0x00, 0x0b, 0x20, 0x00, 0x41,
      0x01, 0x6a, 0x21, 0x00, 0x20, 0x00, 0x41, 0x03, 0x49, 0x0d, 0x00, 0x0b,
      0x1f, 0x40, 0x01, 0x00, 0x00, 0x00, 0x41, 0x07, 0x41, 0x08, 0x08, 0x01,
      0x0b, 0x0b};

  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());
  for (const auto Name : {"buried"sv, "pile"sv}) {
    auto Result = VM.execute(Name);
    ASSERT_FALSE(Result) << Name;
    EXPECT_EQ(Result.error(), ErrCode::Value::UncaughtException) << Name;
  }
}

// return_call to an imported host function must return to the caller's
// caller; it crashed or looped forever depending on the arity.
TEST(ExecutorRegressionTest, TailCallToHostImport) {
  // (module
  //   (import "host" "add_one" (func $h (param i32) (result i32)))
  //   (func $f2 (param i32 i32) (result i32)
  //     local.get 0
  //     return_call $h)
  //   (func $f1 (param i32) (result i32)
  //     local.get 0
  //     return_call $h)
  //   (func (export "run_more") (result i32)
  //     i32.const 41
  //     i32.const 99
  //     call $f2)
  //   (func (export "run_eq") (result i32)
  //     i32.const 7
  //     call $f1))
  std::array<WasmEdge::Byte, 129> Wasm{
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

  // The caller has more params than the callee (was a crash).
  {
    Configure Conf;
    TailCallHostModule HostMod;
    VM::VM VM(Conf);
    VM.registerModule(HostMod);
    ASSERT_TRUE(VM.loadWasm(Wasm));
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

  // The caller and the callee have equal arity (was an infinite loop).
  {
    Configure Conf;
    TailCallHostModule HostMod;
    VM::VM VM(Conf);
    VM.registerModule(HostMod);
    ASSERT_TRUE(VM.loadWasm(Wasm));
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

// Deep recursion must trap with CallStackExhausted instead of overflowing the
// host stack, both with MaxStackSize set and with the default limit.
TEST(ExecutorRegressionTest, CallStackLimit) {
  // f(n) = n ? f(n-1) : 0 without tail calls.
  // (module
  //   (type (;0;) (func (param i64) (result i64)))
  //   (export "f" (func $f))
  //   (func $f (;0;) (type 0) (param $n i64) (result i64)
  //     local.get $n
  //     i64.eqz
  //     if (result i64) ;; label = @1
  //       i64.const 0
  //     else
  //       local.get $n
  //       i64.const 1
  //       i64.sub
  //       call $f
  //     end
  //   )
  // )
  std::array<WasmEdge::Byte, 70> RecurseWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x06, 0x01, 0x60,
      0x01, 0x7e, 0x01, 0x7e, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05, 0x01, 0x01,
      0x66, 0x00, 0x00, 0x0a, 0x14, 0x01, 0x12, 0x00, 0x20, 0x00, 0x50, 0x04,
      0x7e, 0x42, 0x00, 0x05, 0x20, 0x00, 0x42, 0x01, 0x7d, 0x10, 0x00, 0x0b,
      0x0b, 0x00, 0x13, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x01, 0x04, 0x01, 0x00,
      0x01, 0x66, 0x02, 0x06, 0x01, 0x00, 0x01, 0x00, 0x01, 0x6e};

  const std::vector<ValType> ParamTypes = {ValType(TypeCode::I64)};
  {
    Configure Conf;
    Conf.getRuntimeConfigure().setMaxStackSize(UINT64_C(64) << 10);
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(RecurseWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    auto Result = VM.execute("f", {ValVariant(UINT64_C(5000))}, ParamTypes);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::CallStackExhausted);
    EXPECT_EQ(Result.error().getErrCodePhase(), WasmPhase::Execution);
  }
  // The default MaxStackSize of 0 recurses deeper than the compiled-code
  // default allows, but runaway recursion still traps.
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(RecurseWasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());
    EXPECT_TRUE(VM.execute("f", {ValVariant(UINT64_C(20000))}, ParamTypes));
    auto Result =
        VM.execute("f", {ValVariant(UINT64_C(100000000))}, ParamTypes);
    ASSERT_FALSE(Result);
    EXPECT_EQ(Result.error(), ErrCode::Value::CallStackExhausted);
  }
}

// A tag import must match the type of the exported tag both ways.
TEST(ExecutorRegressionTest, TagImportMatchesBothWays) {
  // (module
  //   (type $super (sub (func)))
  //   (type $sub (sub $super (func)))
  //   (tag (export "t") (type $sub)))
  std::array<WasmEdge::Byte, 34> ExportWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02, 0x50,
      0x00, 0x60, 0x00, 0x00, 0x50, 0x01, 0x00, 0x60, 0x00, 0x00, 0x0d, 0x03,
      0x01, 0x00, 0x01, 0x07, 0x05, 0x01, 0x01, 0x74, 0x04, 0x00};

  // The import declares the super type, which must not link.
  // (module
  //   (type $super (sub (func)))
  //   (import "M" "t" (tag (type $super))))
  std::array<WasmEdge::Byte, 26> SuperTypeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01,
      0x06, 0x01, 0x50, 0x00, 0x60, 0x00, 0x00, 0x02, 0x08,
      0x01, 0x01, 0x4d, 0x01, 0x74, 0x04, 0x00, 0x00};
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("M", ExportWasm));
    ASSERT_TRUE(VM.loadWasm(SuperTypeWasm));
    ASSERT_TRUE(VM.validate());
    auto Res = VM.instantiate();
    ASSERT_FALSE(Res);
    EXPECT_EQ(Res.error(), ErrCode::Value::IncompatibleImportType);
  }

  // The import declares an equivalent type, which links.
  // (module
  //   (type $super (sub (func)))
  //   (type $sub (sub $super (func)))
  //   (import "M" "t" (tag (type $sub))))
  std::array<WasmEdge::Byte, 32> SubTypeWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x02,
      0x50, 0x00, 0x60, 0x00, 0x00, 0x50, 0x01, 0x00, 0x60, 0x00, 0x00,
      0x02, 0x08, 0x01, 0x01, 0x4d, 0x01, 0x74, 0x04, 0x00, 0x01};
  {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("M", ExportWasm));
    ASSERT_TRUE(VM.loadWasm(SubTypeWasm));
    ASSERT_TRUE(VM.validate());
    EXPECT_TRUE(VM.instantiate());
  }
}

// A re-exported import is typed by its defining module, not by the forwarder.
TEST(ExecutorRegressionTest, ReExportedImportsMatch) {
  // (module
  //   (type $s (struct (field i32)))
  //   (type $ft (func (param i32)))
  //   (global (export "g") (ref null $s) (ref.null $s))
  //   (table (export "t") 1 (ref null $s))
  //   (tag (export "e") (type $ft)))
  std::array<WasmEdge::Byte, 55> DefineWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02,
      0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x04, 0x05, 0x01,
      0x63, 0x00, 0x00, 0x01, 0x0d, 0x03, 0x01, 0x00, 0x01, 0x06, 0x07,
      0x01, 0x63, 0x00, 0x00, 0xd0, 0x00, 0x0b, 0x07, 0x0d, 0x03, 0x01,
      0x67, 0x03, 0x00, 0x01, 0x74, 0x01, 0x00, 0x01, 0x65, 0x04, 0x00};

  // Re-exports them with its type indices shifted by one.
  // (module
  //   (type $f (func))
  //   (type $s (struct (field i32)))
  //   (type $ft (func (param i32)))
  //   (import "A" "g" (global (ref null $s)))
  //   (import "A" "t" (table 1 (ref null $s)))
  //   (import "A" "e" (tag (type $ft)))
  //   (export "g" (global 0))
  //   (export "t" (table 0))
  //   (export "e" (tag 0)))
  std::array<WasmEdge::Byte, 64> ForwardWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x0c, 0x03,
      0x60, 0x00, 0x00, 0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00,
      0x02, 0x19, 0x03, 0x01, 0x41, 0x01, 0x67, 0x03, 0x63, 0x01, 0x00,
      0x01, 0x41, 0x01, 0x74, 0x01, 0x63, 0x01, 0x00, 0x01, 0x01, 0x41,
      0x01, 0x65, 0x04, 0x00, 0x02, 0x07, 0x0d, 0x03, 0x01, 0x67, 0x03,
      0x00, 0x01, 0x74, 0x01, 0x00, 0x01, 0x65, 0x04, 0x00};

  // (module
  //   (type $s (struct (field i32)))
  //   (type $ft (func (param i32)))
  //   (import "B" "g" (global (ref null $s))))
  std::array<WasmEdge::Byte, 30> GlobalImportWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09,
      0x02, 0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02,
      0x09, 0x01, 0x01, 0x42, 0x01, 0x67, 0x03, 0x63, 0x00, 0x00};

  // (module
  //   (type $s (struct (field i32)))
  //   (type $ft (func (param i32)))
  //   (import "B" "t" (table 1 (ref null $s))))
  std::array<WasmEdge::Byte, 31> TableImportWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09, 0x02,
      0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02, 0x0a, 0x01,
      0x01, 0x42, 0x01, 0x74, 0x01, 0x63, 0x00, 0x00, 0x01};

  // (module
  //   (type $s (struct (field i32)))
  //   (type $ft (func (param i32)))
  //   (import "B" "e" (tag (type $ft))))
  std::array<WasmEdge::Byte, 29> TagImportWasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x09,
      0x02, 0x5f, 0x01, 0x7f, 0x00, 0x60, 0x01, 0x7f, 0x00, 0x02,
      0x08, 0x01, 0x01, 0x42, 0x01, 0x65, 0x04, 0x00, 0x01};

  for (auto Wasm :
       {Span<const Byte>(GlobalImportWasm), Span<const Byte>(TableImportWasm),
        Span<const Byte>(TagImportWasm)}) {
    Configure Conf;
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.registerModule("A", DefineWasm));
    ASSERT_TRUE(VM.registerModule("B", ForwardWasm));
    ASSERT_TRUE(VM.loadWasm(Wasm));
    ASSERT_TRUE(VM.validate());
    EXPECT_TRUE(VM.instantiate());
  }
}

// A host function outside any module has the final type `(func)`, which
// matches `$f` but not the non-final `$g`.
TEST(ExecutorRegressionTest, HostFuncMatchesItsDefinedType) {
  // (module
  //   (type $f (func))
  //   (type $g (sub (func)))
  //   (table (export "tab") 1 funcref)
  //   (func (export "call_f") (call_indirect (type $f) (i32.const 0)))
  //   (func (export "call_g") (call_indirect (type $g) (i32.const 0)))
  //   (func (export "test_f") (result i32)
  //     (ref.test (ref $f) (table.get 0 (i32.const 0))))
  //   (func (export "test_g") (result i32)
  //     (ref.test (ref $g) (table.get 0 (i32.const 0)))))
  std::array<WasmEdge::Byte, 120> Wasm{
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

  Runtime::Instance::FunctionInstance HostFunc(std::make_unique<Nop>());
  Configure Conf;
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
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

// The wide arithmetic instructions take their i64 operand pairs from under the
// rest of the stack and return the low and the high half of the result.
TEST(ExecutorRegressionTest, WideArithmetic) {
  // (module
  //   (func (export "add128") (param i64 i64 i64 i64) (result i64 i64 i64)
  //     i64.const 0x9e3779b97f4a7c15
  //     local.get 0 local.get 1 local.get 2 local.get 3
  //     i64.add128)
  //   (func (export "sub128") (param i64 i64 i64 i64) (result i64 i64 i64)
  //     i64.const 0x9e3779b97f4a7c15
  //     local.get 0 local.get 1 local.get 2 local.get 3
  //     i64.sub128)
  //   (func (export "mul_wide_s") (param i64 i64) (result i64 i64 i64)
  //     i64.const 0x9e3779b97f4a7c15
  //     local.get 0 local.get 1
  //     i64.mul_wide_s)
  //   (func (export "mul_wide_u") (param i64 i64) (result i64 i64 i64)
  //     i64.const 0x9e3779b97f4a7c15
  //     local.get 0 local.get 1
  //     i64.mul_wide_u))
  std::array<WasmEdge::Byte, 174> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x13, 0x02, 0x60,
      0x04, 0x7e, 0x7e, 0x7e, 0x7e, 0x03, 0x7e, 0x7e, 0x7e, 0x60, 0x02, 0x7e,
      0x7e, 0x03, 0x7e, 0x7e, 0x7e, 0x03, 0x05, 0x04, 0x00, 0x00, 0x01, 0x01,
      0x07, 0x2d, 0x04, 0x06, 0x61, 0x64, 0x64, 0x31, 0x32, 0x38, 0x00, 0x00,
      0x06, 0x73, 0x75, 0x62, 0x31, 0x32, 0x38, 0x00, 0x01, 0x0a, 0x6d, 0x75,
      0x6c, 0x5f, 0x77, 0x69, 0x64, 0x65, 0x5f, 0x73, 0x00, 0x02, 0x0a, 0x6d,
      0x75, 0x6c, 0x5f, 0x77, 0x69, 0x64, 0x65, 0x5f, 0x75, 0x00, 0x03, 0x0a,
      0x59, 0x04, 0x17, 0x00, 0x42, 0x95, 0xf8, 0xa9, 0xfa, 0x97, 0xb7, 0xde,
      0x9b, 0x9e, 0x7f, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0x20, 0x03, 0xfc,
      0x13, 0x0b, 0x17, 0x00, 0x42, 0x95, 0xf8, 0xa9, 0xfa, 0x97, 0xb7, 0xde,
      0x9b, 0x9e, 0x7f, 0x20, 0x00, 0x20, 0x01, 0x20, 0x02, 0x20, 0x03, 0xfc,
      0x14, 0x0b, 0x13, 0x00, 0x42, 0x95, 0xf8, 0xa9, 0xfa, 0x97, 0xb7, 0xde,
      0x9b, 0x9e, 0x7f, 0x20, 0x00, 0x20, 0x01, 0xfc, 0x15, 0x0b, 0x13, 0x00,
      0x42, 0x95, 0xf8, 0xa9, 0xfa, 0x97, 0xb7, 0xde, 0x9b, 0x9e, 0x7f, 0x20,
      0x00, 0x20, 0x01, 0xfc, 0x16, 0x0b};
  Configure Conf;
  Conf.addProposal(Proposal::WideArithmetic);
  VM::VM VM(Conf);
  ASSERT_TRUE(VM.loadWasm(Wasm));
  ASSERT_TRUE(VM.validate());
  ASSERT_TRUE(VM.instantiate());

  // Each export returns the sentinel it pushed under the operands, then the
  // low and the high half of the result.
  constexpr uint64_t Sentinel = UINT64_C(0x9e3779b97f4a7c15);
  auto Check = [&](std::string_view Name, std::vector<ValVariant> Params,
                   uint64_t Low, uint64_t High) {
    const std::vector<ValType> Types(Params.size(), ValType(TypeCode::I64));
    auto Result = VM.execute(Name, Params, Types);
    ASSERT_TRUE(Result) << Name;
    ASSERT_EQ(Result->size(), 3U) << Name;
    EXPECT_EQ((*Result)[0].first.get<uint64_t>(), Sentinel) << Name;
    EXPECT_EQ((*Result)[1].first.get<uint64_t>(), Low) << Name;
    EXPECT_EQ((*Result)[2].first.get<uint64_t>(), High) << Name;
  };
  constexpr uint64_t Max = UINT64_MAX;
  constexpr uint64_t SignBit = UINT64_C(0x8000000000000000);
  constexpr uint64_t SignedMax = UINT64_C(0x7fffffffffffffff);

  // Carry from the low half.
  Check("add128", {Max, UINT64_C(0), UINT64_C(1), UINT64_C(0)}, UINT64_C(0),
        UINT64_C(1));
  // Carry wraps the full 128-bit result.
  Check("add128", {Max, Max, UINT64_C(1), UINT64_C(0)}, UINT64_C(0),
        UINT64_C(0));
  // Add both halves without a carry.
  Check("add128", {UINT64_C(2), UINT64_C(3), UINT64_C(5), UINT64_C(7)},
        UINT64_C(7), UINT64_C(10));
  // Adding the carry to the rhs high half also wraps.
  Check("add128", {Max, Max, Max, Max}, Max - 1U, Max);
  // The high half wraps without a low-half carry.
  Check("add128", {UINT64_C(0), Max, UINT64_C(0), UINT64_C(1)}, UINT64_C(0),
        UINT64_C(0));
  // Crossing bit 63 does not carry into the high half.
  Check("add128", {SignedMax, UINT64_C(3), UINT64_C(1), UINT64_C(7)}, SignBit,
        UINT64_C(10));

  // Borrow from the high half.
  Check("sub128", {UINT64_C(0), UINT64_C(1), UINT64_C(1), UINT64_C(0)}, Max,
        UINT64_C(0));
  // Borrow wraps the full 128-bit result.
  Check("sub128", {UINT64_C(0), UINT64_C(0), UINT64_C(1), UINT64_C(0)}, Max,
        Max);
  // Subtract both halves without a borrow.
  Check("sub128", {UINT64_C(9), UINT64_C(7), UINT64_C(2), UINT64_C(3)},
        UINT64_C(7), UINT64_C(4));
  // Adding the borrow to the rhs high half also wraps.
  Check("sub128", {UINT64_C(0), UINT64_C(0), Max, Max}, UINT64_C(1),
        UINT64_C(0));
  // The high half wraps without a low-half borrow.
  Check("sub128", {Max, UINT64_C(0), Max, UINT64_C(1)}, UINT64_C(0), Max);
  // Crossing bit 63 does not borrow from the high half.
  Check("sub128", {SignBit, UINT64_C(7), UINT64_C(1), UINT64_C(3)}, SignedMax,
        UINT64_C(4));

  // Multiplying minus one by two preserves the negative high half.
  Check("mul_wide_s", {Max, UINT64_C(2)}, Max - 1U, Max);
  // Multiplying two negative operands produces a positive result.
  Check("mul_wide_s", {Max, Max}, UINT64_C(1), UINT64_C(0));
  // Negating the minimum signed value produces a zero high half.
  Check("mul_wide_s", {SignBit, Max}, SignBit, UINT64_C(0));
  // Multiplying two minimum signed values produces a positive high half.
  Check("mul_wide_s", {SignBit, SignBit}, UINT64_C(0),
        UINT64_C(0x4000000000000000));
  // Zero times a negative operand produces zero.
  Check("mul_wide_s", {UINT64_C(0), SignBit}, UINT64_C(0), UINT64_C(0));
  // The left negative operand is sign-extended.
  Check("mul_wide_s", {SignBit, UINT64_C(1)}, SignBit, Max);
  // The right negative operand is sign-extended.
  Check("mul_wide_s", {UINT64_C(1), SignBit}, SignBit, Max);
  // Doubling the minimum signed value clears the low half.
  Check("mul_wide_s", {SignBit, UINT64_C(2)}, UINT64_C(0), Max);
  // Multiplying two maximum signed values fills both result halves.
  Check("mul_wide_s", {SignedMax, SignedMax}, UINT64_C(1),
        UINT64_C(0x3fffffffffffffff));
  // Multiplying opposite signed extremes produces a negative result.
  Check("mul_wide_s", {SignedMax, SignBit}, SignBit,
        UINT64_C(0xc000000000000000));

  // Multiplying the maximum unsigned value by two carries into the high half.
  Check("mul_wide_u", {Max, UINT64_C(2)}, Max - 1U, UINT64_C(1));
  // Multiplying two maximum unsigned values fills both result halves.
  Check("mul_wide_u", {Max, Max}, UINT64_C(1), Max - 1U);
  // Bit 63 is part of each unsigned operand's magnitude.
  Check("mul_wide_u", {SignBit, SignBit}, UINT64_C(0),
        UINT64_C(0x4000000000000000));
  // Zero times the maximum unsigned value produces zero.
  Check("mul_wide_u", {UINT64_C(0), Max}, UINT64_C(0), UINT64_C(0));
}

// Rounding, min, and max must return an arithmetic NaN for a NaN operand,
// including a signaling one, which the spec tests accept any NaN for.
TEST(ExecutorRegressionTest, NaNResultsAreArithmetic) {
  // (module
  //   (func (export "f32.ceil") (param i32) (result i32)
  //     (i32.reinterpret_f32 (f32.ceil (f32.reinterpret_i32 (local.get 0)))))
  //   (func (export "f32.floor") (param i32) (result i32)
  //     (i32.reinterpret_f32 (f32.floor (f32.reinterpret_i32 (local.get 0)))))
  //   (func (export "f32.trunc") (param i32) (result i32)
  //     (i32.reinterpret_f32 (f32.trunc (f32.reinterpret_i32 (local.get 0)))))
  //   (func (export "f32.nearest") (param i32) (result i32)
  //     (i32.reinterpret_f32 (f32.nearest (f32.reinterpret_i32 (local.get
  //     0)))))
  //   (func (export "f64.ceil") (param i64) (result i64)
  //     (i64.reinterpret_f64 (f64.ceil (f64.reinterpret_i64 (local.get 0)))))
  //   (func (export "f64.floor") (param i64) (result i64)
  //     (i64.reinterpret_f64 (f64.floor (f64.reinterpret_i64 (local.get 0)))))
  //   (func (export "f64.trunc") (param i64) (result i64)
  //     (i64.reinterpret_f64 (f64.trunc (f64.reinterpret_i64 (local.get 0)))))
  //   (func (export "f64.nearest") (param i64) (result i64)
  //     (i64.reinterpret_f64 (f64.nearest (f64.reinterpret_i64 (local.get
  //     0)))))
  //   (func (export "f32x4.min") (param v128 v128) (result v128)
  //     (f32x4.min (local.get 0) (local.get 1)))
  //   (func (export "f32x4.max") (param v128 v128) (result v128)
  //     (f32x4.max (local.get 0) (local.get 1)))
  //   (func (export "f64x2.min") (param v128 v128) (result v128)
  //     (f64x2.min (local.get 0) (local.get 1)))
  //   (func (export "f64x2.max") (param v128 v128) (result v128)
  //     (f64x2.max (local.get 0) (local.get 1)))
  //   (func (export "f32x4.ceil") (param v128) (result v128)
  //     (f32x4.ceil (local.get 0)))
  //   (func (export "f32x4.floor") (param v128) (result v128)
  //     (f32x4.floor (local.get 0)))
  //   (func (export "f32x4.trunc") (param v128) (result v128)
  //     (f32x4.trunc (local.get 0)))
  //   (func (export "f32x4.nearest") (param v128) (result v128)
  //     (f32x4.nearest (local.get 0)))
  //   (func (export "f64x2.ceil") (param v128) (result v128)
  //     (f64x2.ceil (local.get 0)))
  //   (func (export "f64x2.floor") (param v128) (result v128)
  //     (f64x2.floor (local.get 0)))
  //   (func (export "f64x2.trunc") (param v128) (result v128)
  //     (f64x2.trunc (local.get 0)))
  //   (func (export "f64x2.nearest") (param v128) (result v128)
  //     (f64x2.nearest (local.get 0))))
  std::array<WasmEdge::Byte, 484> Wasm{
      0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x16, 0x04, 0x60,
      0x01, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7e, 0x01, 0x7e, 0x60, 0x02, 0x7b,
      0x7b, 0x01, 0x7b, 0x60, 0x01, 0x7b, 0x01, 0x7b, 0x03, 0x15, 0x14, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03,
      0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x07, 0x85, 0x02, 0x14, 0x08,
      0x66, 0x33, 0x32, 0x2e, 0x63, 0x65, 0x69, 0x6c, 0x00, 0x00, 0x09, 0x66,
      0x33, 0x32, 0x2e, 0x66, 0x6c, 0x6f, 0x6f, 0x72, 0x00, 0x01, 0x09, 0x66,
      0x33, 0x32, 0x2e, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x00, 0x02, 0x0b, 0x66,
      0x33, 0x32, 0x2e, 0x6e, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00, 0x03,
      0x08, 0x66, 0x36, 0x34, 0x2e, 0x63, 0x65, 0x69, 0x6c, 0x00, 0x04, 0x09,
      0x66, 0x36, 0x34, 0x2e, 0x66, 0x6c, 0x6f, 0x6f, 0x72, 0x00, 0x05, 0x09,
      0x66, 0x36, 0x34, 0x2e, 0x74, 0x72, 0x75, 0x6e, 0x63, 0x00, 0x06, 0x0b,
      0x66, 0x36, 0x34, 0x2e, 0x6e, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00,
      0x07, 0x09, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2e, 0x6d, 0x69, 0x6e, 0x00,
      0x08, 0x09, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2e, 0x6d, 0x61, 0x78, 0x00,
      0x09, 0x09, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2e, 0x6d, 0x69, 0x6e, 0x00,
      0x0a, 0x09, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2e, 0x6d, 0x61, 0x78, 0x00,
      0x0b, 0x0a, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2e, 0x63, 0x65, 0x69, 0x6c,
      0x00, 0x0c, 0x0b, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2e, 0x66, 0x6c, 0x6f,
      0x6f, 0x72, 0x00, 0x0d, 0x0b, 0x66, 0x33, 0x32, 0x78, 0x34, 0x2e, 0x74,
      0x72, 0x75, 0x6e, 0x63, 0x00, 0x0e, 0x0d, 0x66, 0x33, 0x32, 0x78, 0x34,
      0x2e, 0x6e, 0x65, 0x61, 0x72, 0x65, 0x73, 0x74, 0x00, 0x0f, 0x0a, 0x66,
      0x36, 0x34, 0x78, 0x32, 0x2e, 0x63, 0x65, 0x69, 0x6c, 0x00, 0x10, 0x0b,
      0x66, 0x36, 0x34, 0x78, 0x32, 0x2e, 0x66, 0x6c, 0x6f, 0x6f, 0x72, 0x00,
      0x11, 0x0b, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2e, 0x74, 0x72, 0x75, 0x6e,
      0x63, 0x00, 0x12, 0x0d, 0x66, 0x36, 0x34, 0x78, 0x32, 0x2e, 0x6e, 0x65,
      0x61, 0x72, 0x65, 0x73, 0x74, 0x00, 0x13, 0x0a, 0xa2, 0x01, 0x14, 0x07,
      0x00, 0x20, 0x00, 0xbe, 0x8d, 0xbc, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbe,
      0x8e, 0xbc, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbe, 0x8f, 0xbc, 0x0b, 0x07,
      0x00, 0x20, 0x00, 0xbe, 0x90, 0xbc, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf,
      0x9b, 0xbd, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf, 0x9c, 0xbd, 0x0b, 0x07,
      0x00, 0x20, 0x00, 0xbf, 0x9d, 0xbd, 0x0b, 0x07, 0x00, 0x20, 0x00, 0xbf,
      0x9e, 0xbd, 0x0b, 0x09, 0x00, 0x20, 0x00, 0x20, 0x01, 0xfd, 0xe8, 0x01,
      0x0b, 0x09, 0x00, 0x20, 0x00, 0x20, 0x01, 0xfd, 0xe9, 0x01, 0x0b, 0x09,
      0x00, 0x20, 0x00, 0x20, 0x01, 0xfd, 0xf4, 0x01, 0x0b, 0x09, 0x00, 0x20,
      0x00, 0x20, 0x01, 0xfd, 0xf5, 0x01, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xfd,
      0x67, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xfd, 0x68, 0x0b, 0x06, 0x00, 0x20,
      0x00, 0xfd, 0x69, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xfd, 0x6a, 0x0b, 0x06,
      0x00, 0x20, 0x00, 0xfd, 0x74, 0x0b, 0x06, 0x00, 0x20, 0x00, 0xfd, 0x75,
      0x0b, 0x06, 0x00, 0x20, 0x00, 0xfd, 0x7a, 0x0b, 0x07, 0x00, 0x20, 0x00,
      0xfd, 0x94, 0x01, 0x0b};
  std::vector<RunMode> Modes{RunMode::Interpreter};
#ifdef WASMEDGE_USE_LLVM
  Modes.push_back(RunMode::JIT);
#endif
  for (const auto Mode : Modes) {
    const std::string_view ModeName =
        Mode == RunMode::JIT ? "JIT"sv : "interpreter"sv;
    Configure Conf;
    Conf.getRuntimeConfigure().setRunMode(Mode);
    VM::VM VM(Conf);
    ASSERT_TRUE(VM.loadWasm(Wasm));
    ASSERT_TRUE(VM.validate());
    ASSERT_TRUE(VM.instantiate());

    // The scalar exports take and return the bit pattern of their float.
    for (const auto Name :
         {"f32.ceil", "f32.floor", "f32.trunc", "f32.nearest"}) {
      for (const uint32_t In : {UINT32_C(0x7FA00000), UINT32_C(0xFFA00001)}) {
        auto Result =
            VM.execute(Name, {ValVariant(In)}, {ValType(TypeCode::I32)});
        ASSERT_TRUE(Result) << ModeName << ' ' << Name;
        const auto Got = (*Result)[0].first.get<uint32_t>();
        EXPECT_TRUE(std::isnan(cxx20::bit_cast<float>(Got)) &&
                    (Got & QuietBit<float>) != 0)
            << ModeName << ' ' << Name << " bits " << std::hex << Got;
      }
    }
    for (const auto Name :
         {"f64.ceil", "f64.floor", "f64.trunc", "f64.nearest"}) {
      for (const uint64_t In :
           {UINT64_C(0x7FF4000000000000), UINT64_C(0xFFF0000000000001)}) {
        auto Result =
            VM.execute(Name, {ValVariant(In)}, {ValType(TypeCode::I64)});
        ASSERT_TRUE(Result) << ModeName << ' ' << Name;
        const auto Got = (*Result)[0].first.get<uint64_t>();
        EXPECT_TRUE(std::isnan(cxx20::bit_cast<double>(Got)) &&
                    (Got & QuietBit<double>) != 0)
            << ModeName << ' ' << Name << " bits " << std::hex << Got;
      }
    }

    const auto Run = [&](std::string_view Name,
                         std::vector<ValVariant> Args) -> uint128_t {
      const std::vector<ValType> Types(Args.size(), ValType(TypeCode::V128));
      auto Result = VM.execute(Name, Args, Types);
      EXPECT_TRUE(Result) << ModeName << ' ' << Name;
      return Result ? (*Result)[0].first.get<uint128_t>() : uint128_t(0U);
    };

    constexpr uint32_t NaN32 = UINT32_C(0x7FC00000);
    // Lanes: +sNaN, -1.5, -sNaN, 2.5 and 1.0, +sNaN, -sNaN, 3.0.
    const auto A32 =
        v128Of<float>({UINT32_C(0x7FA00000), UINT32_C(0xBFC00000),
                       UINT32_C(0xFFA00001), UINT32_C(0x40200000)});
    const auto B32 =
        v128Of<float>({UINT32_C(0x3F800000), UINT32_C(0x7F800001),
                       UINT32_C(0xFF800002), UINT32_C(0x40400000)});
    for (const auto &[L, R] : {std::pair{A32, B32}, std::pair{B32, A32}}) {
      expectLanes<float>(ModeName, "f32x4.min", Run("f32x4.min", {L, R}),
                         {NaN32, NaN32, NaN32, UINT32_C(0x40200000)});
      expectLanes<float>(ModeName, "f32x4.max", Run("f32x4.max", {L, R}),
                         {NaN32, NaN32, NaN32, UINT32_C(0x40400000)});
    }
    expectLanes<float>(
        ModeName, "f32x4.ceil", Run("f32x4.ceil", {A32}),
        {NaN32, UINT32_C(0xBF800000), NaN32, UINT32_C(0x40400000)});
    expectLanes<float>(
        ModeName, "f32x4.floor", Run("f32x4.floor", {A32}),
        {NaN32, UINT32_C(0xC0000000), NaN32, UINT32_C(0x40000000)});
    expectLanes<float>(
        ModeName, "f32x4.trunc", Run("f32x4.trunc", {A32}),
        {NaN32, UINT32_C(0xBF800000), NaN32, UINT32_C(0x40000000)});
    expectLanes<float>(
        ModeName, "f32x4.nearest", Run("f32x4.nearest", {A32}),
        {NaN32, UINT32_C(0xC0000000), NaN32, UINT32_C(0x40000000)});

    constexpr uint64_t NaN64 = UINT64_C(0x7FF8000000000000);
    // Lanes: +sNaN, -1.5 and 1.0, -sNaN.
    const auto A64 = v128Of<double>(
        {UINT64_C(0x7FF4000000000000), UINT64_C(0xBFF8000000000000)});
    const auto B64 = v128Of<double>(
        {UINT64_C(0x3FF0000000000000), UINT64_C(0xFFF0000000000001)});
    for (const auto &[L, R] : {std::pair{A64, B64}, std::pair{B64, A64}}) {
      expectLanes<double>(ModeName, "f64x2.min", Run("f64x2.min", {L, R}),
                          {NaN64, NaN64});
      expectLanes<double>(ModeName, "f64x2.max", Run("f64x2.max", {L, R}),
                          {NaN64, NaN64});
    }
    expectLanes<double>(ModeName, "f64x2.ceil", Run("f64x2.ceil", {A64}),
                        {NaN64, UINT64_C(0xBFF0000000000000)});
    expectLanes<double>(ModeName, "f64x2.floor", Run("f64x2.floor", {B64}),
                        {UINT64_C(0x3FF0000000000000), NaN64});
    expectLanes<double>(ModeName, "f64x2.trunc", Run("f64x2.trunc", {A64}),
                        {NaN64, UINT64_C(0xBFF0000000000000)});
    expectLanes<double>(ModeName, "f64x2.nearest", Run("f64x2.nearest", {A64}),
                        {NaN64, UINT64_C(0xC000000000000000)});
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
