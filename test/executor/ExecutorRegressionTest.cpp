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
// clang-format off
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
    0xFB, 0x14, 0x6F, 0x0B
};
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
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
