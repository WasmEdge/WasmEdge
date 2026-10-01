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

#include "common/spdlog.h"
#include "common/types.h"
#include "vm/vm.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
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

// Checks that the null in exported table "t" has an abstract heap type, which
// catches issue #4757 without running wasm that would segfault.
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
  ASSERT_TRUE(checkNullTableEntry(VM));

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

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
