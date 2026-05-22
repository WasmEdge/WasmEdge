// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/component/canonical_abi.h"

#include "ast/component/type.h"
#include "ast/component/valtype.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/types.h"
#include "runtime/instance/memory.h"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using namespace WasmEdge;
using namespace WasmEdge::Executor::CanonicalABI;

namespace ASTComp = AST::Component;

ComponentValType prim(ComponentTypeCode C) noexcept { return ComponentValType(C); }

// Build a `DefValType` holding a record from the supplied component value
// types. Labels are auto-generated since they're not consulted by the
// alignment / elem_size paths.
ASTComp::DefValType
makeRecord(std::initializer_list<ComponentValType> Types) {
  ASTComp::RecordTy R;
  uint32_t I = 0;
  for (const auto &T : Types) {
    R.LabelTypes.push_back(
        ASTComp::LabelValType("f" + std::to_string(I++), T));
  }
  ASTComp::DefValType D;
  D.setRecord(std::move(R));
  return D;
}

ASTComp::DefValType
makeTuple(std::initializer_list<ComponentValType> Types) {
  ASTComp::TupleTy T;
  for (const auto &Ty : Types) {
    T.Types.push_back(Ty);
  }
  ASTComp::DefValType D;
  D.setTuple(std::move(T));
  return D;
}

ASTComp::DefValType makeVariant(
    std::initializer_list<std::optional<ComponentValType>> Cases) {
  ASTComp::VariantTy V;
  uint32_t I = 0;
  for (const auto &C : Cases) {
    V.Cases.emplace_back("c" + std::to_string(I++), C);
  }
  ASTComp::DefValType D;
  D.setVariant(std::move(V));
  return D;
}

ASTComp::DefValType makeFlags(uint32_t N) {
  ASTComp::FlagsTy F;
  for (uint32_t I = 0; I < N; ++I) {
    F.Labels.push_back("f" + std::to_string(I));
  }
  ASTComp::DefValType D;
  D.setFlags(std::move(F));
  return D;
}

ASTComp::DefValType makeOption(ComponentValType T) {
  ASTComp::DefValType D;
  ASTComp::OptionTy O;
  O.ValTy = T;
  D.setOption(std::move(O));
  return D;
}

ASTComp::DefValType makeResult(std::optional<ComponentValType> Ok,
                               std::optional<ComponentValType> Err) {
  ASTComp::DefValType D;
  ASTComp::ResultTy R;
  R.ValTy = Ok;
  R.ErrTy = Err;
  D.setResult(std::move(R));
  return D;
}

ASTComp::DefValType makeListNoLen(ComponentValType T) {
  ASTComp::DefValType D;
  ASTComp::ListTy L;
  L.ValTy = T;
  L.Len = std::nullopt;
  D.setList(std::move(L));
  return D;
}

ASTComp::DefValType makeEnum(uint32_t N) {
  ASTComp::EnumTy E;
  for (uint32_t I = 0; I < N; ++I) {
    E.Labels.push_back("e" + std::to_string(I));
  }
  ASTComp::DefValType D;
  D.setEnum(std::move(E));
  return D;
}

uint32_t alignDef(const ASTComp::DefValType &D) {
  CanonCtx Cx{};
  auto Res = alignmentDef(Cx, D);
  EXPECT_TRUE(Res.has_value()) << "alignmentDef returned an error";
  return Res.has_value() ? *Res : 0;
}

uint32_t alignVal(const ComponentValType &T) {
  CanonCtx Cx{};
  auto Res = alignment(Cx, T);
  EXPECT_TRUE(Res.has_value()) << "alignment returned an error";
  return Res.has_value() ? *Res : 0;
}

uint32_t sizeDef(const ASTComp::DefValType &D) {
  CanonCtx Cx{};
  auto Res = elemSizeDef(Cx, D);
  EXPECT_TRUE(Res.has_value()) << "elemSizeDef returned an error";
  return Res.has_value() ? *Res : 0;
}

uint32_t sizeVal(const ComponentValType &T) {
  CanonCtx Cx{};
  auto Res = elemSize(Cx, T);
  EXPECT_TRUE(Res.has_value()) << "elemSize returned an error";
  return Res.has_value() ? *Res : 0;
}

// =============================================================================
// discriminantSize — CanonicalABI.md L1951-1956
// =============================================================================

TEST(ComponentCanonABI, DiscriminantSize) {
  // 1..256 → 1 byte
  EXPECT_EQ(discriminantSize(1), 1u);
  EXPECT_EQ(discriminantSize(2), 1u);
  EXPECT_EQ(discriminantSize(256), 1u);
  // 257..65536 → 2 bytes
  EXPECT_EQ(discriminantSize(257), 2u);
  EXPECT_EQ(discriminantSize(65536), 2u);
  // 65537+ → 4 bytes
  EXPECT_EQ(discriminantSize(65537), 4u);
}

// =============================================================================
// alignment — CanonicalABI.md L1904-1985
// =============================================================================

TEST(ComponentCanonABI, AlignmentPrimitives) {
  // CanonicalABI.md L1908-1926 table.
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::Bool)), 1u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::S8)), 1u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::U8)), 1u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::S16)), 2u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::U16)), 2u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::S32)), 4u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::U32)), 4u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::F32)), 4u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::Char)), 4u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::S64)), 8u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::U64)), 8u);
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::F64)), 8u);
  // String alignment is the i32 ptr alignment (L1930).
  EXPECT_EQ(alignVal(prim(ComponentTypeCode::String)), 4u);
}

TEST(ComponentCanonABI, AlignmentRecord) {
  // empty record → 1
  EXPECT_EQ(alignDef(makeRecord({})), 1u);
  // record{u8} → 1
  EXPECT_EQ(alignDef(makeRecord({prim(ComponentTypeCode::U8)})), 1u);
  // record{u8, u32} → 4 (largest field)
  EXPECT_EQ(alignDef(makeRecord(
                {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)})),
            4u);
  // record{u64, u8} → 8
  EXPECT_EQ(alignDef(makeRecord(
                {prim(ComponentTypeCode::U64), prim(ComponentTypeCode::U8)})),
            8u);
}

TEST(ComponentCanonABI, AlignmentTuple) {
  EXPECT_EQ(alignDef(makeTuple(
                {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U64)})),
            8u);
}

TEST(ComponentCanonABI, AlignmentVariant) {
  // variant{u8 | u32} — 2 cases (disc 1B), max payload align 4 → 4
  EXPECT_EQ(alignDef(makeVariant({prim(ComponentTypeCode::U8),
                                  prim(ComponentTypeCode::U32)})),
            4u);
  // variant{u64} — 1 case (disc 1B), max payload align 8 → 8
  EXPECT_EQ(alignDef(makeVariant({prim(ComponentTypeCode::U64)})), 8u);
  // variant with 257 cases — discriminant alone is 2B
  std::vector<std::optional<ComponentValType>> Many;
  for (uint32_t I = 0; I < 257; ++I) {
    Many.emplace_back(std::nullopt);
  }
  ASTComp::VariantTy V;
  uint32_t I = 0;
  for (const auto &C : Many) {
    V.Cases.emplace_back("c" + std::to_string(I++), C);
  }
  ASTComp::DefValType D;
  D.setVariant(std::move(V));
  EXPECT_EQ(alignDef(D), 2u);
}

TEST(ComponentCanonABI, AlignmentOptionResult) {
  // option<u32> — 2-case variant, disc 1B, payload align 4 → 4
  EXPECT_EQ(alignDef(makeOption(prim(ComponentTypeCode::U32))), 4u);
  // option<u8> → max(1, 1) = 1
  EXPECT_EQ(alignDef(makeOption(prim(ComponentTypeCode::U8))), 1u);
  // result<u64, u8> → max(1, 8, 1) = 8
  EXPECT_EQ(alignDef(makeResult(prim(ComponentTypeCode::U64),
                                prim(ComponentTypeCode::U8))),
            8u);
  // result<_,_> with both none → just discriminant
  EXPECT_EQ(alignDef(makeResult(std::nullopt, std::nullopt)), 1u);
}

TEST(ComponentCanonABI, AlignmentList) {
  // list<u32> → 4 (ptr+len pair, alignment of i32 ptr).
  EXPECT_EQ(alignDef(makeListNoLen(prim(ComponentTypeCode::U32))), 4u);
}

TEST(ComponentCanonABI, AlignmentFlags) {
  // alignment_flags (L1971-1985): next_pow2(ceil(|labels|/8))
  EXPECT_EQ(alignDef(makeFlags(0)), 1u);  // empty → 1
  EXPECT_EQ(alignDef(makeFlags(1)), 1u);  // 1 byte
  EXPECT_EQ(alignDef(makeFlags(7)), 1u);  // 1 byte
  EXPECT_EQ(alignDef(makeFlags(8)), 1u);  // 1 byte
  EXPECT_EQ(alignDef(makeFlags(9)), 2u);  // 2 bytes (pow2)
  EXPECT_EQ(alignDef(makeFlags(16)), 2u); // 2 bytes
  EXPECT_EQ(alignDef(makeFlags(17)), 4u); // 3 bytes → pow2 = 4
  EXPECT_EQ(alignDef(makeFlags(32)), 4u); // 4 bytes
}

TEST(ComponentCanonABI, AlignmentEnum) {
  // Enum aligns to its discriminant.
  EXPECT_EQ(alignDef(makeEnum(1)), 1u);
  EXPECT_EQ(alignDef(makeEnum(256)), 1u);
  EXPECT_EQ(alignDef(makeEnum(257)), 2u);
}

TEST(ComponentCanonABI, AlignmentFixedLengthListRejected) {
  // 🔧 fixed-length list is gated and must be rejected.
  ASTComp::DefValType D;
  ASTComp::ListTy L;
  L.ValTy = prim(ComponentTypeCode::U32);
  L.Len = 4;
  D.setList(std::move(L));
  CanonCtx Cx{};
  auto Res = alignmentDef(Cx, D);
  ASSERT_FALSE(Res.has_value());
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentNotImplInstantiate);
}

TEST(ComponentCanonABI, AlignmentErrorContextRejected) {
  // 📝 error-context is gated and must be rejected.
  CanonCtx Cx{};
  auto Res = alignment(Cx, prim(ComponentTypeCode::ErrContext));
  ASSERT_FALSE(Res.has_value());
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentNotImplInstantiate);
}

// =============================================================================
// elem_size — CanonicalABI.md L1990-2040
// =============================================================================

TEST(ComponentCanonABI, ElemSizePrimitives) {
  // CanonicalABI.md L1994-2008 table.
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::Bool)), 1u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::S8)), 1u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::U8)), 1u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::S16)), 2u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::U16)), 2u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::S32)), 4u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::U32)), 4u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::F32)), 4u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::Char)), 4u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::S64)), 8u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::U64)), 8u);
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::F64)), 8u);
  // String is (ptr, len) → 8 bytes (L2009-2013).
  EXPECT_EQ(sizeVal(prim(ComponentTypeCode::String)), 8u);
}

TEST(ComponentCanonABI, ElemSizeRecordPadding) {
  // record{u8, u32}: off=0; u8 → off=1; align to 4 → off=4; +u32 → off=8.
  // record alignment=4; align_to(8,4)=8.
  EXPECT_EQ(sizeDef(makeRecord(
                {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)})),
            8u);
  // record{u32, u8}: u32 → off=4; +u8 → off=5; align 4 → 8.
  EXPECT_EQ(sizeDef(makeRecord(
                {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U8)})),
            8u);
  // record{u8} → 1
  EXPECT_EQ(sizeDef(makeRecord({prim(ComponentTypeCode::U8)})), 1u);
  // record{u64, u8}: off=8; +1=9; align 8 → 16.
  EXPECT_EQ(sizeDef(makeRecord(
                {prim(ComponentTypeCode::U64), prim(ComponentTypeCode::U8)})),
            16u);
}

TEST(ComponentCanonABI, ElemSizeTuple) {
  // tuple{u8, u64}: u8 → off=1; align 8 → 8; +u64 → 16; tuple align 8 → 16.
  EXPECT_EQ(sizeDef(makeTuple(
                {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U64)})),
            16u);
}

TEST(ComponentCanonABI, ElemSizeVariant) {
  // variant{u8 | u32}: disc 1B → s=1; align to max payload (4) → 4;
  // + max payload (4) → 8; variant align 4 → 8.
  EXPECT_EQ(sizeDef(makeVariant({prim(ComponentTypeCode::U8),
                                 prim(ComponentTypeCode::U32)})),
            8u);
  // variant{u64}: disc 1B → 1; align 8 → 8; +8 → 16; align 8 → 16.
  EXPECT_EQ(sizeDef(makeVariant({prim(ComponentTypeCode::U64)})), 16u);
}

TEST(ComponentCanonABI, ElemSizeOptionResult) {
  // option<u32>: 1 + 3 + 4 = 8.
  EXPECT_EQ(sizeDef(makeOption(prim(ComponentTypeCode::U32))), 8u);
  // option<u8>: 1 + 1 = 2, align 1 → 2.
  EXPECT_EQ(sizeDef(makeOption(prim(ComponentTypeCode::U8))), 2u);
  // result<u64,u8>: 1 + 7 + 8 = 16, align 8 → 16.
  EXPECT_EQ(sizeDef(makeResult(prim(ComponentTypeCode::U64),
                               prim(ComponentTypeCode::U8))),
            16u);
  // result<_,_>: just discriminant 1B.
  EXPECT_EQ(sizeDef(makeResult(std::nullopt, std::nullopt)), 1u);
}

TEST(ComponentCanonABI, ElemSizeList) {
  // list<u32> → 8 (ptr + len).
  EXPECT_EQ(sizeDef(makeListNoLen(prim(ComponentTypeCode::U32))), 8u);
}

TEST(ComponentCanonABI, ElemSizeFlags) {
  // ceil(N/8) aligned to alignment_flags.
  EXPECT_EQ(sizeDef(makeFlags(1)), 1u);
  EXPECT_EQ(sizeDef(makeFlags(8)), 1u);
  EXPECT_EQ(sizeDef(makeFlags(9)), 2u);
  EXPECT_EQ(sizeDef(makeFlags(16)), 2u);
  EXPECT_EQ(sizeDef(makeFlags(17)), 4u); // 3 bytes aligned to 4
  EXPECT_EQ(sizeDef(makeFlags(32)), 4u);
}

TEST(ComponentCanonABI, ElemSizeEnum) {
  EXPECT_EQ(sizeDef(makeEnum(1)), 1u);
  EXPECT_EQ(sizeDef(makeEnum(256)), 1u);
  EXPECT_EQ(sizeDef(makeEnum(257)), 2u);
}

TEST(ComponentCanonABI, ElemSizeFixedLengthListRejected) {
  // 🔧 fixed-length list deferred.
  ASTComp::DefValType D;
  ASTComp::ListTy L;
  L.ValTy = prim(ComponentTypeCode::U32);
  L.Len = 4;
  D.setList(std::move(L));
  CanonCtx Cx{};
  auto Res = elemSizeDef(Cx, D);
  ASSERT_FALSE(Res.has_value());
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentNotImplInstantiate);
}

// =============================================================================
// flatten_type — CanonicalABI.md L2860-2877
// =============================================================================

std::vector<TypeCode> flattenCodes(const ComponentValType &T) {
  CanonCtx Cx{};
  auto Res = flattenType(Cx, T);
  EXPECT_TRUE(Res.has_value());
  std::vector<TypeCode> Out;
  if (Res.has_value()) {
    for (const auto &V : *Res) {
      Out.push_back(V.getCode());
    }
  }
  return Out;
}

std::vector<TypeCode> flattenCodesDef(const ASTComp::DefValType &D) {
  CanonCtx Cx{};
  auto Res = flattenTypeDef(Cx, D);
  EXPECT_TRUE(Res.has_value());
  std::vector<TypeCode> Out;
  if (Res.has_value()) {
    for (const auto &V : *Res) {
      Out.push_back(V.getCode());
    }
  }
  return Out;
}

TEST(ComponentCanonABI, FlattenPrimitives) {
  // CanonicalABI.md L2862-2870 table.
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::Bool)),
            (std::vector<TypeCode>{TypeCode::I32}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::S8)),
            (std::vector<TypeCode>{TypeCode::I32}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::U32)),
            (std::vector<TypeCode>{TypeCode::I32}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::Char)),
            (std::vector<TypeCode>{TypeCode::I32}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::S64)),
            (std::vector<TypeCode>{TypeCode::I64}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::U64)),
            (std::vector<TypeCode>{TypeCode::I64}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::F32)),
            (std::vector<TypeCode>{TypeCode::F32}));
  EXPECT_EQ(flattenCodes(prim(ComponentTypeCode::F64)),
            (std::vector<TypeCode>{TypeCode::F64}));
  EXPECT_EQ(
      flattenCodes(prim(ComponentTypeCode::String)),
      (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
}

TEST(ComponentCanonABI, FlattenRecord) {
  // record{u8, u32, u64} → [i32, i32, i64]
  EXPECT_EQ(flattenCodesDef(makeRecord({prim(ComponentTypeCode::U8),
                                        prim(ComponentTypeCode::U32),
                                        prim(ComponentTypeCode::U64)})),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32,
                                   TypeCode::I64}));
}

TEST(ComponentCanonABI, FlattenTuple) {
  // tuple{f32, f64} → [f32, f64]
  EXPECT_EQ(flattenCodesDef(makeTuple({prim(ComponentTypeCode::F32),
                                       prim(ComponentTypeCode::F64)})),
            (std::vector<TypeCode>{TypeCode::F32, TypeCode::F64}));
}

TEST(ComponentCanonABI, FlattenVariantJoin) {
  // variant{u8 | u32} → disc + join(i32, i32) = [i32, i32]
  EXPECT_EQ(flattenCodesDef(makeVariant({prim(ComponentTypeCode::U8),
                                         prim(ComponentTypeCode::U32)})),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
  // variant{f32 | s32} → disc + join(f32, i32) = [i32, i32] (per spec L2917)
  EXPECT_EQ(flattenCodesDef(makeVariant({prim(ComponentTypeCode::F32),
                                         prim(ComponentTypeCode::S32)})),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
  // variant{s64 | f64} → disc + join(i64, f64) = [i32, i64]
  EXPECT_EQ(flattenCodesDef(makeVariant({prim(ComponentTypeCode::S64),
                                         prim(ComponentTypeCode::F64)})),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I64}));
  // variant{s32 | f64} → disc + join(i32, f64) = [i32, i64]
  EXPECT_EQ(flattenCodesDef(makeVariant({prim(ComponentTypeCode::S32),
                                         prim(ComponentTypeCode::F64)})),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I64}));
}

TEST(ComponentCanonABI, FlattenOption) {
  EXPECT_EQ(flattenCodesDef(makeOption(prim(ComponentTypeCode::U32))),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
  EXPECT_EQ(flattenCodesDef(makeOption(prim(ComponentTypeCode::U64))),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I64}));
  EXPECT_EQ(flattenCodesDef(makeOption(prim(ComponentTypeCode::F64))),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::F64}));
}

TEST(ComponentCanonABI, FlattenResult) {
  // result<u32, u8> → [i32, i32] (join(i32, i32))
  EXPECT_EQ(flattenCodesDef(makeResult(prim(ComponentTypeCode::U32),
                                       prim(ComponentTypeCode::U8))),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
  // result<u64, _> → [i32, i64]
  EXPECT_EQ(
      flattenCodesDef(makeResult(prim(ComponentTypeCode::U64), std::nullopt)),
      (std::vector<TypeCode>{TypeCode::I32, TypeCode::I64}));
  // result<_, _> → just disc
  EXPECT_EQ(flattenCodesDef(makeResult(std::nullopt, std::nullopt)),
            (std::vector<TypeCode>{TypeCode::I32}));
}

TEST(ComponentCanonABI, FlattenFlagsEnumOwnBorrow) {
  EXPECT_EQ(flattenCodesDef(makeFlags(32)),
            (std::vector<TypeCode>{TypeCode::I32}));
  EXPECT_EQ(flattenCodesDef(makeEnum(3)),
            (std::vector<TypeCode>{TypeCode::I32}));
  ASTComp::DefValType OwnD;
  OwnD.setOwn(ASTComp::OwnTy{0});
  EXPECT_EQ(flattenCodesDef(OwnD), (std::vector<TypeCode>{TypeCode::I32}));
  ASTComp::DefValType BorrowD;
  BorrowD.setBorrow(ASTComp::BorrowTy{0});
  EXPECT_EQ(flattenCodesDef(BorrowD), (std::vector<TypeCode>{TypeCode::I32}));
}

TEST(ComponentCanonABI, FlattenList) {
  EXPECT_EQ(flattenCodesDef(makeListNoLen(prim(ComponentTypeCode::U32))),
            (std::vector<TypeCode>{TypeCode::I32, TypeCode::I32}));
}

// =============================================================================
// flatten_functype — CanonicalABI.md L2819-2832 (sync only)
// =============================================================================

ASTComp::FuncType
makeFunc(std::vector<ComponentValType> Params,
         std::vector<ComponentValType> Results) {
  std::vector<ASTComp::LabelValType> P;
  uint32_t I = 0;
  for (const auto &T : Params) {
    P.emplace_back("p" + std::to_string(I++), T);
  }
  std::vector<ASTComp::LabelValType> R;
  I = 0;
  for (const auto &T : Results) {
    R.emplace_back("r" + std::to_string(I++), T);
  }
  return ASTComp::FuncType{std::move(P), std::move(R)};
}

TEST(ComponentCanonABI, FlattenFuncTypeDirect) {
  CanonCtx Cx{};
  auto FT = makeFunc({prim(ComponentTypeCode::U32),
                      prim(ComponentTypeCode::U64)},
                     {prim(ComponentTypeCode::F32)});
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/true);
  ASSERT_TRUE(Res.has_value());
  EXPECT_EQ(Res->Params.size(), 2u);
  EXPECT_EQ(Res->Params[0].getCode(), TypeCode::I32);
  EXPECT_EQ(Res->Params[1].getCode(), TypeCode::I64);
  ASSERT_EQ(Res->Results.size(), 1u);
  EXPECT_EQ(Res->Results[0].getCode(), TypeCode::F32);
}

TEST(ComponentCanonABI, FlattenFuncTypeLiftIndirectResults) {
  // Two u32 results flatten to [i32, i32] — over MaxFlatResults (=1) — so
  // lift collapses to a single return-area pointer.
  CanonCtx Cx{};
  auto FT =
      makeFunc({}, {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U32)});
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/true);
  ASSERT_TRUE(Res.has_value());
  EXPECT_EQ(Res->Params.size(), 0u);
  ASSERT_EQ(Res->Results.size(), 1u);
  EXPECT_EQ(Res->Results[0].getCode(), TypeCode::I32);
}

TEST(ComponentCanonABI, FlattenFuncTypeLowerIndirectResultsAddsOutPtr) {
  // Lower side: results > MAX_FLAT_RESULTS (=1) collapses results=[] and
  // appends a trailing out-pointer to params (CanonicalABI.md L2829-2831).
  CanonCtx Cx{};
  auto FT =
      makeFunc({}, {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U32)});
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/false);
  ASSERT_TRUE(Res.has_value());
  ASSERT_EQ(Res->Params.size(), 1u);
  EXPECT_EQ(Res->Params[0].getCode(), TypeCode::I32);
  EXPECT_TRUE(Res->Results.empty());
}

TEST(ComponentCanonABI, FlattenFuncTypeLowerIndirectResultsAppendsToParams) {
  // Params + out-pointer combine on lower direction.
  CanonCtx Cx{};
  auto FT = makeFunc({prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U32)},
                     {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U32)});
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/false);
  ASSERT_TRUE(Res.has_value());
  ASSERT_EQ(Res->Params.size(), 3u);
  EXPECT_EQ(Res->Params[0].getCode(), TypeCode::I32);
  EXPECT_EQ(Res->Params[1].getCode(), TypeCode::I32);
  EXPECT_EQ(Res->Params[2].getCode(), TypeCode::I32); // out-ptr
  EXPECT_TRUE(Res->Results.empty());
}

TEST(ComponentCanonABI, FlattenFuncTypeTooManyParamsCollapses) {
  // 17 u32 params flatten to 17×i32 — over MaxFlatParams (=16). Indirect-params
  // path collapses to a single i32 (CanonicalABI.md L2823-2824); applies to
  // both lift and lower.
  CanonCtx Cx{};
  std::vector<ComponentValType> Ps(17, prim(ComponentTypeCode::U32));
  auto FT = makeFunc(std::move(Ps), {prim(ComponentTypeCode::U32)});
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/true);
  ASSERT_TRUE(Res.has_value());
  ASSERT_EQ(Res->Params.size(), 1u);
  EXPECT_EQ(Res->Params[0].getCode(), TypeCode::I32);
  ASSERT_EQ(Res->Results.size(), 1u);
  EXPECT_EQ(Res->Results[0].getCode(), TypeCode::I32);
}

TEST(ComponentCanonABI, FlattenFuncTypeAsyncRejected) {
  // 🔀 async deferred.
  CanonCtx Cx{};
  auto FT = makeFunc({}, {});
  FT.setAsync(true);
  auto Res = flattenFuncType(Cx, FT, /*IsLift=*/true);
  ASSERT_FALSE(Res.has_value());
  EXPECT_EQ(Res.error(), ErrCode::Value::ComponentNotImplInstantiate);
}

// =============================================================================
// load — CanonicalABI.md L2050-2288
// =============================================================================

class CanonABIMemFixture : public ::testing::Test {
protected:
  CanonABIMemFixture()
      : MemType(1U), Mem(MemType),
        Cx{nullptr, &Mem, nullptr, nullptr, {}} {}

  void writeBytes(uint32_t Off, const std::vector<uint8_t> &Bytes) {
    auto Res = Mem.setBytes(Bytes, Off, 0, Bytes.size());
    ASSERT_TRUE(Res.has_value());
  }

  template <typename T> void writeLE(uint32_t Off, T V) {
    Mem.storeValue<T>(V, Off);
  }

  AST::MemoryType MemType;
  Runtime::Instance::MemoryInstance Mem;
  CanonCtx Cx;
};

TEST_F(CanonABIMemFixture, LoadPrimU32) {
  writeLE<uint32_t>(16, 0xDEADBEEFu);
  auto V = load(Cx, 16, prim(ComponentTypeCode::U32));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<uint32_t>(*V), 0xDEADBEEFu);
}

TEST_F(CanonABIMemFixture, LoadPrimS8SignExtended) {
  writeBytes(8, {0xFF});
  auto V = load(Cx, 8, prim(ComponentTypeCode::S8));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<int8_t>(*V), -1);
}

TEST_F(CanonABIMemFixture, LoadPrimBool) {
  writeBytes(4, {0});
  writeBytes(5, {1});
  writeBytes(6, {42});
  // convert_int_to_bool: any non-zero → true.
  auto V0 = load(Cx, 4, prim(ComponentTypeCode::Bool));
  auto V1 = load(Cx, 5, prim(ComponentTypeCode::Bool));
  auto V2 = load(Cx, 6, prim(ComponentTypeCode::Bool));
  ASSERT_TRUE(V0.has_value());
  ASSERT_TRUE(V1.has_value());
  ASSERT_TRUE(V2.has_value());
  EXPECT_FALSE(std::get<bool>(*V0));
  EXPECT_TRUE(std::get<bool>(*V1));
  EXPECT_TRUE(std::get<bool>(*V2));
}

TEST_F(CanonABIMemFixture, LoadPrimCharValid) {
  writeLE<uint32_t>(12, 0x4A); // 'J'
  auto V = load(Cx, 12, prim(ComponentTypeCode::Char));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<uint32_t>(*V), 0x4Au);
}

TEST_F(CanonABIMemFixture, LoadPrimCharSurrogateTraps) {
  writeLE<uint32_t>(12, 0xD800); // UTF-16 surrogate
  auto V = load(Cx, 12, prim(ComponentTypeCode::Char));
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::ComponentTrap);
}

TEST_F(CanonABIMemFixture, LoadPrimCharOutOfRangeTraps) {
  writeLE<uint32_t>(12, 0x110000); // > 0x10FFFF
  auto V = load(Cx, 12, prim(ComponentTypeCode::Char));
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::ComponentTrap);
}

TEST_F(CanonABIMemFixture, LoadStringUTF8) {
  const std::string Hello = "hello";
  writeBytes(64, std::vector<uint8_t>(Hello.begin(), Hello.end()));
  writeLE<uint32_t>(16, 64u);                      // begin
  writeLE<uint32_t>(20, static_cast<uint32_t>(Hello.size())); // length
  auto V = load(Cx, 16, prim(ComponentTypeCode::String));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<std::string>(*V), Hello);
}

TEST_F(CanonABIMemFixture, LoadStringOOBTraps) {
  // Begin = near end of memory, length too large.
  const uint64_t MemSize = Mem.getSize();
  writeLE<uint32_t>(16, static_cast<uint32_t>(MemSize - 2));
  writeLE<uint32_t>(20, 10u);
  auto V = load(Cx, 16, prim(ComponentTypeCode::String));
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::MemoryOutOfBounds);
}

TEST_F(CanonABIMemFixture, LoadRecordU8U32WithPadding) {
  // record{u8, u32} laid out as: [u8 @0, pad @1..3, u32 @4..7].
  writeBytes(32, {0xAB, 0, 0, 0});                 // u8 + padding
  writeLE<uint32_t>(36, 0x12345678u);              // u32
  auto D = makeRecord(
      {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)});
  auto V = loadDef(Cx, 32, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  ASSERT_TRUE(VC);
  auto &R = std::get<RecordVal>(VC->V);
  ASSERT_EQ(R.Fields.size(), 2u);
  EXPECT_EQ(std::get<uint8_t>(R.Fields[0].second), 0xABu);
  EXPECT_EQ(std::get<uint32_t>(R.Fields[1].second), 0x12345678u);
}

TEST_F(CanonABIMemFixture, LoadVariantWithPayload) {
  // variant{u8 | u32}: disc 1B; max payload align 4 → payload @4..7.
  // Discriminant = 1 (second case → u32).
  writeBytes(48, {1, 0, 0, 0});            // disc + padding
  writeLE<uint32_t>(52, 0xCAFEBABEu);      // u32 payload
  auto D = makeVariant(
      {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)});
  auto V = loadDef(Cx, 48, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  ASSERT_TRUE(VC);
  auto &Vt = std::get<VariantVal>(VC->V);
  EXPECT_EQ(Vt.Case, 1u);
  ASSERT_TRUE(Vt.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Vt.Payload), 0xCAFEBABEu);
}

TEST_F(CanonABIMemFixture, LoadVariantDiscOutOfRangeTraps) {
  writeBytes(48, {99, 0, 0, 0});
  auto D = makeVariant(
      {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)});
  auto V = loadDef(Cx, 48, D);
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::ComponentTrap);
}

TEST_F(CanonABIMemFixture, LoadOptionSome) {
  // option<u32>: disc 1B (1=some), payload aligned to 4.
  writeBytes(96, {1, 0, 0, 0});
  writeLE<uint32_t>(100, 7u);
  auto D = makeOption(prim(ComponentTypeCode::U32));
  auto V = loadDef(Cx, 96, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &O = std::get<OptionVal>(VC->V);
  ASSERT_TRUE(O.Value.has_value());
  EXPECT_EQ(std::get<uint32_t>(*O.Value), 7u);
}

TEST_F(CanonABIMemFixture, LoadOptionNone) {
  writeBytes(96, {0, 0, 0, 0});
  auto D = makeOption(prim(ComponentTypeCode::U32));
  auto V = loadDef(Cx, 96, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &O = std::get<OptionVal>(VC->V);
  EXPECT_FALSE(O.Value.has_value());
}

TEST_F(CanonABIMemFixture, LoadResultOk) {
  // result<u64, u8>: disc 1B (0=ok), payload aligned to 8.
  writeBytes(128, {0, 0, 0, 0, 0, 0, 0, 0});
  uint64_t Big = 0x1122334455667788ull;
  Mem.storeValue<uint64_t>(Big, 136);
  auto D = makeResult(prim(ComponentTypeCode::U64),
                      prim(ComponentTypeCode::U8));
  auto V = loadDef(Cx, 128, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &R = std::get<ResultVal>(VC->V);
  EXPECT_TRUE(R.IsOk);
  ASSERT_TRUE(R.Payload.has_value());
  EXPECT_EQ(std::get<uint64_t>(*R.Payload), Big);
}

TEST_F(CanonABIMemFixture, LoadFlags17) {
  // 17 labels → 3 bytes raw, aligned to 4 → 4 bytes (per A.2 test).
  // We pack bits 0, 4, 16 → raw = 0x00010011.
  writeLE<uint32_t>(160, 0x00010011u);
  auto D = makeFlags(17);
  auto V = loadDef(Cx, 160, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &F = std::get<FlagsVal>(VC->V);
  ASSERT_EQ(F.Bits.size(), 17u);
  EXPECT_TRUE(F.Bits[0]);
  EXPECT_FALSE(F.Bits[1]);
  EXPECT_TRUE(F.Bits[4]);
  EXPECT_TRUE(F.Bits[16]);
}

TEST_F(CanonABIMemFixture, LoadEnumValid) {
  // 3-case enum → disc 1B.
  writeBytes(176, {2});
  auto D = makeEnum(3);
  auto V = loadDef(Cx, 176, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &E = std::get<EnumVal>(VC->V);
  EXPECT_EQ(E.Case, 2u);
}

TEST_F(CanonABIMemFixture, LoadEnumOutOfRange) {
  writeBytes(176, {7});
  auto D = makeEnum(3);
  auto V = loadDef(Cx, 176, D);
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::ComponentTrap);
}

TEST_F(CanonABIMemFixture, LoadListU16) {
  // list<u16>: ptr + len at 192. Backing data at 256: [10, 20, 30].
  writeLE<uint32_t>(192, 256u);
  writeLE<uint32_t>(196, 3u);
  Mem.storeValue<uint32_t, 2>(10u, 256);
  Mem.storeValue<uint32_t, 2>(20u, 258);
  Mem.storeValue<uint32_t, 2>(30u, 260);
  auto D = makeListNoLen(prim(ComponentTypeCode::U16));
  auto V = loadDef(Cx, 192, D);
  ASSERT_TRUE(V.has_value());
  auto VC = std::get<std::shared_ptr<ValComp>>(*V);
  auto &L = std::get<ListVal>(VC->V);
  ASSERT_EQ(L.Elements.size(), 3u);
  EXPECT_EQ(std::get<uint16_t>(L.Elements[0]), 10u);
  EXPECT_EQ(std::get<uint16_t>(L.Elements[1]), 20u);
  EXPECT_EQ(std::get<uint16_t>(L.Elements[2]), 30u);
}

TEST_F(CanonABIMemFixture, LoadListMisalignedTraps) {
  writeLE<uint32_t>(192, 257u); // misaligned for u16
  writeLE<uint32_t>(196, 3u);
  auto D = makeListNoLen(prim(ComponentTypeCode::U16));
  auto V = loadDef(Cx, 192, D);
  ASSERT_FALSE(V.has_value());
  EXPECT_EQ(V.error(), ErrCode::Value::ComponentTrap);
}

// =============================================================================
// store / round-trip — CanonicalABI.md L2360-2735
// =============================================================================

template <typename T>
void roundTripPrim(CanonCtx &Cx, ComponentTypeCode TC, T Expected,
                   uint32_t Off) {
  auto Ty = prim(TC);
  EXPECT_TRUE(store(Cx, ComponentValVariant{Expected}, Ty, Off).has_value())
      << "store " << static_cast<int>(TC);
  auto R = load(Cx, Off, Ty);
  ASSERT_TRUE(R.has_value()) << "load " << static_cast<int>(TC);
  EXPECT_EQ(std::get<T>(*R), Expected) << "round-trip " << static_cast<int>(TC);
}

TEST_F(CanonABIMemFixture, RoundTripPrimitivesAll) {
  roundTripPrim<bool>(Cx, ComponentTypeCode::Bool, true, 0);
  roundTripPrim<int8_t>(Cx, ComponentTypeCode::S8, -7, 8);
  roundTripPrim<uint8_t>(Cx, ComponentTypeCode::U8, 200, 16);
  roundTripPrim<int16_t>(Cx, ComponentTypeCode::S16, -12345, 24);
  roundTripPrim<uint16_t>(Cx, ComponentTypeCode::U16, 60000, 32);
  roundTripPrim<int32_t>(Cx, ComponentTypeCode::S32, -2000000000, 40);
  roundTripPrim<uint32_t>(Cx, ComponentTypeCode::U32, 0xDEADBEEFu, 48);
  // -(1 << 40): use multiplication to avoid UB on shifting a negative literal.
  const int64_t BigNeg = -(int64_t{1} << 40);
  roundTripPrim<int64_t>(Cx, ComponentTypeCode::S64, BigNeg, 56);
  roundTripPrim<uint64_t>(Cx, ComponentTypeCode::U64,
                          uint64_t{0x0123456789ABCDEFull}, 64);
  roundTripPrim<float>(Cx, ComponentTypeCode::F32, 3.14f, 72);
  roundTripPrim<double>(Cx, ComponentTypeCode::F64, 2.718281828, 80);
  // 😀 (U+1F600).
  roundTripPrim<uint32_t>(Cx, ComponentTypeCode::Char, 0x1F600u, 88);
}

TEST_F(CanonABIMemFixture, RoundTripRecord) {
  auto D = makeRecord(
      {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)});
  // Build the expected record value.
  RecordVal R;
  R.Fields.emplace_back("f0", ComponentValVariant{static_cast<uint8_t>(0xAB)});
  R.Fields.emplace_back("f1", ComponentValVariant{uint32_t{0x12345678u}});
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(R);
  ComponentValVariant CVV{std::move(VC)};
  EXPECT_TRUE(storeDef(Cx, CVV, D, 32).has_value());
  auto Loaded = loadDef(Cx, 32, D);
  ASSERT_TRUE(Loaded.has_value());
  auto &LR = std::get<RecordVal>(
      std::get<std::shared_ptr<ValComp>>(*Loaded)->V);
  ASSERT_EQ(LR.Fields.size(), 2u);
  EXPECT_EQ(std::get<uint8_t>(LR.Fields[0].second), 0xABu);
  EXPECT_EQ(std::get<uint32_t>(LR.Fields[1].second), 0x12345678u);
}

TEST_F(CanonABIMemFixture, RoundTripVariant) {
  auto D = makeVariant(
      {prim(ComponentTypeCode::U8), prim(ComponentTypeCode::U32)});
  VariantVal Vv;
  Vv.Case = 1;
  Vv.Payload = ComponentValVariant{uint32_t{0xCAFEBABEu}};
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(Vv);
  ComponentValVariant CVV{std::move(VC)};
  EXPECT_TRUE(storeDef(Cx, CVV, D, 48).has_value());
  auto Loaded = loadDef(Cx, 48, D);
  ASSERT_TRUE(Loaded.has_value());
  auto &LV =
      std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*Loaded)->V);
  EXPECT_EQ(LV.Case, 1u);
  ASSERT_TRUE(LV.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*LV.Payload), 0xCAFEBABEu);
}

TEST_F(CanonABIMemFixture, RoundTripOptionResult) {
  auto OD = makeOption(prim(ComponentTypeCode::U32));
  OptionVal Ov;
  Ov.Value = ComponentValVariant{uint32_t{42u}};
  auto OVC = std::make_shared<ValComp>();
  OVC->V = std::move(Ov);
  EXPECT_TRUE(storeDef(Cx, ComponentValVariant{std::move(OVC)}, OD, 96)
                  .has_value());
  auto OL = loadDef(Cx, 96, OD);
  ASSERT_TRUE(OL.has_value());
  auto &OO = std::get<OptionVal>(std::get<std::shared_ptr<ValComp>>(*OL)->V);
  ASSERT_TRUE(OO.Value.has_value());
  EXPECT_EQ(std::get<uint32_t>(*OO.Value), 42u);

  auto RD = makeResult(prim(ComponentTypeCode::U64),
                       prim(ComponentTypeCode::U8));
  ResultVal Rv;
  Rv.IsOk = false;
  Rv.Payload = ComponentValVariant{static_cast<uint8_t>(99)};
  auto RVC = std::make_shared<ValComp>();
  RVC->V = std::move(Rv);
  EXPECT_TRUE(storeDef(Cx, ComponentValVariant{std::move(RVC)}, RD, 128)
                  .has_value());
  auto RL = loadDef(Cx, 128, RD);
  ASSERT_TRUE(RL.has_value());
  auto &RR = std::get<ResultVal>(std::get<std::shared_ptr<ValComp>>(*RL)->V);
  EXPECT_FALSE(RR.IsOk);
  ASSERT_TRUE(RR.Payload.has_value());
  EXPECT_EQ(std::get<uint8_t>(*RR.Payload), 99u);
}

TEST_F(CanonABIMemFixture, RoundTripFlags17) {
  auto D = makeFlags(17);
  FlagsVal F;
  F.Bits.resize(17);
  F.Bits[0] = true;
  F.Bits[4] = true;
  F.Bits[16] = true;
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(F);
  EXPECT_TRUE(storeDef(Cx, ComponentValVariant{std::move(VC)}, D, 160).has_value());
  auto L = loadDef(Cx, 160, D);
  ASSERT_TRUE(L.has_value());
  auto &LF = std::get<FlagsVal>(std::get<std::shared_ptr<ValComp>>(*L)->V);
  ASSERT_EQ(LF.Bits.size(), 17u);
  EXPECT_TRUE(LF.Bits[0]);
  EXPECT_FALSE(LF.Bits[1]);
  EXPECT_TRUE(LF.Bits[4]);
  EXPECT_TRUE(LF.Bits[16]);
}

TEST_F(CanonABIMemFixture, StoreEmptyStringSkipsRealloc) {
  // Empty string never invokes realloc — short-circuit lets us cover the
  // store(String) path here without a real Executor. Expect (ptr=0, len=0)
  // written at the target offset. CanonicalABI.md L2483-2487.
  auto R = store(Cx, ComponentValVariant{std::string()},
                 prim(ComponentTypeCode::String), 32);
  ASSERT_TRUE(R.has_value());
  uint32_t Ptr = 0xFFFFFFFFu;
  uint32_t Len = 0xFFFFFFFFu;
  ASSERT_TRUE(Mem.loadValue<uint32_t>(Ptr, 32).has_value());
  ASSERT_TRUE(Mem.loadValue<uint32_t>(Len, 36).has_value());
  EXPECT_EQ(Ptr, 0u);
  EXPECT_EQ(Len, 0u);
}

TEST_F(CanonABIMemFixture, StoreEmptyListSkipsRealloc) {
  // Same idea: zero-length list writes (ptr=0, len=0) without realloc.
  auto D = makeListNoLen(prim(ComponentTypeCode::U16));
  ListVal L;
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(L);
  auto R = storeDef(Cx, ComponentValVariant{std::move(VC)}, D, 32);
  ASSERT_TRUE(R.has_value());
  uint32_t Ptr = 0xFFFFFFFFu;
  uint32_t Len = 0xFFFFFFFFu;
  ASSERT_TRUE(Mem.loadValue<uint32_t>(Ptr, 32).has_value());
  ASSERT_TRUE(Mem.loadValue<uint32_t>(Len, 36).has_value());
  EXPECT_EQ(Ptr, 0u);
  EXPECT_EQ(Len, 0u);
}

// =============================================================================
// lift_flat — CanonicalABI.md L2957-3084
// =============================================================================

TEST_F(CanonABIMemFixture, LiftFlatPrimitiveU32) {
  // A single u32 → 1 i32 flat value.
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0xCAFEBABEu})};
  FlatIter It(Flat);
  auto V = liftFlat(Cx, It, prim(ComponentTypeCode::U32));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<uint32_t>(*V), 0xCAFEBABEu);
  EXPECT_TRUE(It.done());
}

TEST_F(CanonABIMemFixture, LiftFlatPrimitiveS8FromI32) {
  // s8 flattens to i32; lift narrows back to int8_t with sign extension.
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0x000000FFu})}; // -1 (s8)
  FlatIter It(Flat);
  auto V = liftFlat(Cx, It, prim(ComponentTypeCode::S8));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<int8_t>(*V), -1);
}

TEST_F(CanonABIMemFixture, LiftFlatPrimitiveBool) {
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0u}),
                               ValVariant(uint32_t{1u}),
                               ValVariant(uint32_t{99u})};
  FlatIter It(Flat);
  auto V0 = liftFlat(Cx, It, prim(ComponentTypeCode::Bool));
  auto V1 = liftFlat(Cx, It, prim(ComponentTypeCode::Bool));
  auto V2 = liftFlat(Cx, It, prim(ComponentTypeCode::Bool));
  ASSERT_TRUE(V0.has_value() && V1.has_value() && V2.has_value());
  EXPECT_FALSE(std::get<bool>(*V0));
  EXPECT_TRUE(std::get<bool>(*V1));
  EXPECT_TRUE(std::get<bool>(*V2));
}

TEST_F(CanonABIMemFixture, LiftFlatString) {
  // Write payload to memory then lift via flat (ptr, len).
  const std::string Msg = "hello, world";
  writeBytes(256, std::vector<uint8_t>(Msg.begin(), Msg.end()));
  std::vector<ValVariant> Flat{
      ValVariant(uint32_t{256u}),
      ValVariant(uint32_t{static_cast<uint32_t>(Msg.size())})};
  FlatIter It(Flat);
  auto V = liftFlat(Cx, It, prim(ComponentTypeCode::String));
  ASSERT_TRUE(V.has_value());
  EXPECT_EQ(std::get<std::string>(*V), Msg);
}

TEST_F(CanonABIMemFixture, LiftFlatRecordU8) {
  // record{u8} → flat [i32].
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0xABu})};
  FlatIter It(Flat);
  auto D = makeRecord({prim(ComponentTypeCode::U8)});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &R = std::get<RecordVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  ASSERT_EQ(R.Fields.size(), 1u);
  EXPECT_EQ(std::get<uint8_t>(R.Fields[0].second), 0xABu);
}

TEST_F(CanonABIMemFixture, LiftFlatFlags) {
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0b10101u})};
  FlatIter It(Flat);
  auto D = makeFlags(5);
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &F = std::get<FlagsVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_TRUE(F.Bits[0]);
  EXPECT_FALSE(F.Bits[1]);
  EXPECT_TRUE(F.Bits[2]);
  EXPECT_FALSE(F.Bits[3]);
  EXPECT_TRUE(F.Bits[4]);
}

TEST_F(CanonABIMemFixture, LiftFlatEnum) {
  std::vector<ValVariant> Flat{ValVariant(uint32_t{2u})};
  FlatIter It(Flat);
  auto D = makeEnum(4);
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &E = std::get<EnumVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(E.Case, 2u);
}

TEST_F(CanonABIMemFixture, LiftFlatVariantNoPayloadCases) {
  // variant{a | b | c} (all-none payloads) → flat [i32] (disc only).
  std::vector<ValVariant> Flat{ValVariant(uint32_t{1u})};
  FlatIter It(Flat);
  auto D = makeVariant({std::nullopt, std::nullopt, std::nullopt});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &Vv = std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(Vv.Case, 1u);
  EXPECT_FALSE(Vv.Payload.has_value());
}

TEST_F(CanonABIMemFixture, LiftFlatVariantWithPayloadU32) {
  // variant{a | u32}: flat = [i32 disc, i32 payload]. Case 1 + payload 42.
  std::vector<ValVariant> Flat{ValVariant(uint32_t{1u}),
                               ValVariant(uint32_t{42u})};
  FlatIter It(Flat);
  auto D = makeVariant({std::nullopt, prim(ComponentTypeCode::U32)});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &Vv = std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(Vv.Case, 1u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Vv.Payload), 42u);
  EXPECT_TRUE(It.done());
}

TEST_F(CanonABIMemFixture, LiftFlatVariantNoPayloadCaseDrainsPadding) {
  // variant{none | u64}: flat = [i32 disc, i64 join]. Case 0 has no payload,
  // but the i64 join slot must still be drained.
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0u}),
                               ValVariant(uint64_t{0xCAFEBABEDEADBEEFull})};
  FlatIter It(Flat);
  auto D = makeVariant({std::nullopt, prim(ComponentTypeCode::U64)});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &Vv = std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(Vv.Case, 0u);
  EXPECT_FALSE(Vv.Payload.has_value());
  EXPECT_TRUE(It.done());
}

TEST_F(CanonABIMemFixture, LiftFlatVariantJoinI32F32ReadsAsF32) {
  // variant{u32 | f32}: join slot = i32 (f32 and i32 join → i32).
  // Pick case 1 (f32), so the i32-shaped slot must be bit-reinterpreted to f32.
  float Pi = 3.14f;
  uint32_t Bits = 0;
  std::memcpy(&Bits, &Pi, sizeof(Bits));
  std::vector<ValVariant> Flat{ValVariant(uint32_t{1u}),
                               ValVariant(Bits)};
  FlatIter It(Flat);
  auto D = makeVariant(
      {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::F32)});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &Vv = std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(Vv.Case, 1u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<float>(*Vv.Payload), Pi);
}

TEST_F(CanonABIMemFixture, LiftFlatVariantJoinI64WrapsToI32) {
  // variant{u32 | u64}: join slot = i64 (i32 & i64 → i64). For case 0 (u32),
  // the joined i64 slot is wrapped back to i32 by CoerceValueIter.
  std::vector<ValVariant> Flat{
      ValVariant(uint32_t{0u}),
      ValVariant(uint64_t{0x1122334455667788ull})};
  FlatIter It(Flat);
  auto D = makeVariant(
      {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U64)});
  auto V = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(V.has_value());
  auto &Vv = std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*V)->V);
  EXPECT_EQ(Vv.Case, 0u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Vv.Payload), 0x55667788u);
}

// =============================================================================
// lower_flat (round-trip with lift_flat) — CanonicalABI.md L3086-3192
// =============================================================================

namespace {

// Helper: round-trip `V` of type `T` through lowerFlat → liftFlat, asserting
// the holds_alternative and equality of the resulting ComponentValVariant.
// Only valid for primitive shapes that don't need realloc (no string/list).
template <typename T>
void roundTripPrim(const CanonCtx &Cx, const ComponentValType &CT,
                   const T &Original) {
  ComponentValVariant CV{Original};
  auto Lowered = lowerFlat(Cx, CV, CT);
  ASSERT_TRUE(Lowered.has_value());
  FlatIter It(*Lowered);
  auto Lifted = liftFlat(Cx, It, CT);
  ASSERT_TRUE(Lifted.has_value());
  EXPECT_TRUE(std::holds_alternative<T>(*Lifted));
  EXPECT_EQ(std::get<T>(*Lifted), Original);
  EXPECT_TRUE(It.done());
}

} // namespace

TEST_F(CanonABIMemFixture, LowerLiftRoundTripPrimitives) {
  roundTripPrim<uint32_t>(Cx, prim(ComponentTypeCode::U32), 0xDEADBEEFu);
  roundTripPrim<uint64_t>(Cx, prim(ComponentTypeCode::U64),
                          0xFEEDFACECAFEBEEFull);
  roundTripPrim<int8_t>(Cx, prim(ComponentTypeCode::S8), -42);
  roundTripPrim<int16_t>(Cx, prim(ComponentTypeCode::S16), -12345);
  roundTripPrim<int32_t>(Cx, prim(ComponentTypeCode::S32), -1);
  roundTripPrim<int64_t>(Cx, prim(ComponentTypeCode::S64),
                         -9000000000000000000ll);
  roundTripPrim<float>(Cx, prim(ComponentTypeCode::F32), 3.14f);
  roundTripPrim<double>(Cx, prim(ComponentTypeCode::F64), 2.718281828);
  roundTripPrim<bool>(Cx, prim(ComponentTypeCode::Bool), true);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripCharValid) {
  // U+1F600 (😀).
  uint32_t Code = 0x1F600u;
  ComponentValVariant CV{Code};
  auto Lowered = lowerFlat(Cx, CV, prim(ComponentTypeCode::Char));
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 1u);
  FlatIter It(*Lowered);
  auto Lifted = liftFlat(Cx, It, prim(ComponentTypeCode::Char));
  ASSERT_TRUE(Lifted.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Lifted), Code);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripTupleOfPrimitives) {
  // tuple<u32, s16, bool>.
  AST::Component::TupleTy Tup;
  Tup.Types.push_back(prim(ComponentTypeCode::U32));
  Tup.Types.push_back(prim(ComponentTypeCode::S16));
  Tup.Types.push_back(prim(ComponentTypeCode::Bool));
  AST::Component::DefValType D;
  D.setTuple(std::move(Tup));

  TupleVal Tv;
  Tv.Values.emplace_back(uint32_t{0xABCD1234u});
  Tv.Values.emplace_back(int16_t{-1000});
  Tv.Values.emplace_back(true);
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(Tv);

  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  EXPECT_EQ(Lowered->size(), 3u); // three i32s

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &LiftedTup =
      std::get<TupleVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  ASSERT_EQ(LiftedTup.Values.size(), 3u);
  EXPECT_EQ(std::get<uint32_t>(LiftedTup.Values[0]), 0xABCD1234u);
  EXPECT_EQ(std::get<int16_t>(LiftedTup.Values[1]), int16_t{-1000});
  EXPECT_EQ(std::get<bool>(LiftedTup.Values[2]), true);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripFlags17) {
  // 17-label flags pack into a single i32 (Preview 2 cap = 32).
  AST::Component::FlagsTy F;
  for (uint32_t I = 0; I < 17; ++I) {
    F.Labels.push_back("f" + std::to_string(I));
  }
  AST::Component::DefValType D;
  D.setFlags(std::move(F));

  FlagsVal Fv;
  Fv.Bits.assign(17, false);
  Fv.Bits[0] = true;
  Fv.Bits[5] = true;
  Fv.Bits[16] = true;
  auto VC = std::make_shared<ValComp>();
  VC->V = std::move(Fv);

  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 1u);
  EXPECT_EQ(Lowered->at(0).get<uint32_t>(),
            (1u << 0) | (1u << 5) | (1u << 16));

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &LiftedF =
      std::get<FlagsVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  ASSERT_EQ(LiftedF.Bits.size(), 17u);
  EXPECT_TRUE(LiftedF.Bits[0]);
  EXPECT_TRUE(LiftedF.Bits[5]);
  EXPECT_TRUE(LiftedF.Bits[16]);
  EXPECT_FALSE(LiftedF.Bits[3]);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripEnum) {
  AST::Component::EnumTy E;
  E.Labels = {"a", "b", "c", "d"};
  AST::Component::DefValType D;
  D.setEnum(std::move(E));

  auto VC = std::make_shared<ValComp>();
  VC->V = EnumVal{2u};

  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 1u);
  EXPECT_EQ(Lowered->at(0).get<uint32_t>(), 2u);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  EXPECT_EQ(std::get<EnumVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V)
                .Case,
            2u);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripVariantWithPayload) {
  // variant{none | u32}: joined flat = [i32 disc, i32 payload].
  auto D = makeVariant({std::nullopt, prim(ComponentTypeCode::U32)});
  auto VC = std::make_shared<ValComp>();
  VC->V = VariantVal{1u, ComponentValVariant{uint32_t{99u}}};
  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 2u);
  EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 1u);
  EXPECT_EQ((*Lowered)[1].get<uint32_t>(), 99u);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &Vv =
      std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  EXPECT_EQ(Vv.Case, 1u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Vv.Payload), 99u);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripVariantJoinI64) {
  // variant{u32 | u64}: i32 widens to occupy the joined i64 slot (spec L3171).
  auto D = makeVariant(
      {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::U64)});
  auto VC = std::make_shared<ValComp>();
  VC->V = VariantVal{0u, ComponentValVariant{uint32_t{0xCAFEu}}};
  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 2u);
  EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 0u);
  EXPECT_EQ((*Lowered)[1].get<uint64_t>(), 0xCAFEull);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &Vv =
      std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  EXPECT_EQ(Vv.Case, 0u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<uint32_t>(*Vv.Payload), 0xCAFEu);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripVariantJoinF32AsI32) {
  // variant{u32 | f32}: f32 reinterprets to i32 slot (spec L3170).
  auto D = makeVariant(
      {prim(ComponentTypeCode::U32), prim(ComponentTypeCode::F32)});
  auto VC = std::make_shared<ValComp>();
  VC->V = VariantVal{1u, ComponentValVariant{1.5f}};
  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 2u);
  EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 1u);
  uint32_t Bits = 0;
  float V15 = 1.5f;
  std::memcpy(&Bits, &V15, sizeof(Bits));
  EXPECT_EQ((*Lowered)[1].get<uint32_t>(), Bits);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &Vv =
      std::get<VariantVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  EXPECT_EQ(Vv.Case, 1u);
  ASSERT_TRUE(Vv.Payload.has_value());
  EXPECT_EQ(std::get<float>(*Vv.Payload), 1.5f);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripOptionF64) {
  // option<f64>: joined flat = [i32 disc, f64]. Lower some(2.71) → check f64
  // slot; roundtrip back to OptionVal{some}.
  auto D = makeOption(prim(ComponentTypeCode::F64));
  auto VC = std::make_shared<ValComp>();
  VC->V = OptionVal{ComponentValVariant{2.71828}};
  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 2u);
  EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 1u);
  EXPECT_EQ((*Lowered)[1].get<double>(), 2.71828);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &Ov =
      std::get<OptionVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  ASSERT_TRUE(Ov.Value.has_value());
  EXPECT_EQ(std::get<double>(*Ov.Value), 2.71828);
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripOptionNoneDrainsPadding) {
  // option<f64> with none — disc = 0, joined slot zero-padded to f64.
  auto D = makeOption(prim(ComponentTypeCode::F64));
  auto VC = std::make_shared<ValComp>();
  VC->V = OptionVal{};
  auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
  ASSERT_TRUE(Lowered.has_value());
  ASSERT_EQ(Lowered->size(), 2u);
  EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 0u);
  EXPECT_EQ((*Lowered)[1].get<double>(), 0.);

  FlatIter It(*Lowered);
  auto Lifted = liftFlatDef(Cx, It, D);
  ASSERT_TRUE(Lifted.has_value());
  auto &Ov =
      std::get<OptionVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
  EXPECT_FALSE(Ov.Value.has_value());
}

TEST_F(CanonABIMemFixture, LowerLiftRoundTripResultOkErrAsymPayloads) {
  // result<u8, u64>: ok payload u8 (flat [i32]) + err payload u64 (flat [i64]).
  // Joined slot = i64. Ok path widens i32→i64; err path is a direct i64.
  auto D = makeResult(prim(ComponentTypeCode::U8),
                      prim(ComponentTypeCode::U64));
  // Ok case
  {
    auto VC = std::make_shared<ValComp>();
    VC->V = ResultVal{/*IsOk=*/true,
                      ComponentValVariant{static_cast<uint8_t>(0xAB)}};
    auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
    ASSERT_TRUE(Lowered.has_value());
    ASSERT_EQ(Lowered->size(), 2u);
    EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 0u);
    EXPECT_EQ((*Lowered)[1].get<uint64_t>(), 0xABull);

    FlatIter It(*Lowered);
    auto Lifted = liftFlatDef(Cx, It, D);
    ASSERT_TRUE(Lifted.has_value());
    auto &Rv =
        std::get<ResultVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
    EXPECT_TRUE(Rv.IsOk);
    ASSERT_TRUE(Rv.Payload.has_value());
    EXPECT_EQ(std::get<uint8_t>(*Rv.Payload), 0xABu);
  }
  // Err case
  {
    auto VC = std::make_shared<ValComp>();
    VC->V = ResultVal{/*IsOk=*/false,
                      ComponentValVariant{uint64_t{0xDEADBEEF12345678ull}}};
    auto Lowered = lowerFlatDef(Cx, ComponentValVariant{VC}, D);
    ASSERT_TRUE(Lowered.has_value());
    ASSERT_EQ(Lowered->size(), 2u);
    EXPECT_EQ((*Lowered)[0].get<uint32_t>(), 1u);
    EXPECT_EQ((*Lowered)[1].get<uint64_t>(), 0xDEADBEEF12345678ull);

    FlatIter It(*Lowered);
    auto Lifted = liftFlatDef(Cx, It, D);
    ASSERT_TRUE(Lifted.has_value());
    auto &Rv =
        std::get<ResultVal>(std::get<std::shared_ptr<ValComp>>(*Lifted)->V);
    EXPECT_FALSE(Rv.IsOk);
    ASSERT_TRUE(Rv.Payload.has_value());
    EXPECT_EQ(std::get<uint64_t>(*Rv.Payload), 0xDEADBEEF12345678ull);
  }
}

// =============================================================================
// lift_flat_values / lower_flat_values — CanonicalABI.md L3193-3232
// =============================================================================

TEST_F(CanonABIMemFixture, LiftFlatValuesDirect) {
  // 2 u32 types, MaxFlat=4 → direct path, both lifted from the iter.
  std::vector<ValVariant> Flat{ValVariant(uint32_t{0xAAAAu}),
                               ValVariant(uint32_t{0xBBBBu})};
  std::vector<ComponentValType> Types{prim(ComponentTypeCode::U32),
                                      prim(ComponentTypeCode::U32)};
  FlatIter It(Flat);
  auto R = liftFlatValues(Cx, It, Types, /*MaxFlat=*/4u);
  ASSERT_TRUE(R.has_value());
  ASSERT_EQ(R->size(), 2u);
  EXPECT_EQ(std::get<uint32_t>((*R)[0]), 0xAAAAu);
  EXPECT_EQ(std::get<uint32_t>((*R)[1]), 0xBBBBu);
  EXPECT_TRUE(It.done());
}

TEST_F(CanonABIMemFixture, LowerFlatValuesDirect) {
  // 2 u32 values → direct path → 2 i32 flats.
  std::vector<ComponentValVariant> Vs{ComponentValVariant{uint32_t{1u}},
                                      ComponentValVariant{uint32_t{2u}}};
  std::vector<ComponentValType> Types{prim(ComponentTypeCode::U32),
                                      prim(ComponentTypeCode::U32)};
  auto R = lowerFlatValues(Cx, Vs, Types, /*MaxFlat=*/4u);
  ASSERT_TRUE(R.has_value());
  ASSERT_EQ(R->size(), 2u);
  EXPECT_EQ((*R)[0].get<uint32_t>(), 1u);
  EXPECT_EQ((*R)[1].get<uint32_t>(), 2u);
}

TEST_F(CanonABIMemFixture, LiftFlatValuesIndirectFromMemory) {
  // 2 u32 types, MaxFlat=1 → indirect. Pre-populate memory at offset 64 with
  // {0x11, 0x22} u32s, then lift.
  writeLE<uint32_t>(64, 0x11u);
  writeLE<uint32_t>(68, 0x22u);
  std::vector<ValVariant> Flat{ValVariant(uint32_t{64u})};
  std::vector<ComponentValType> Types{prim(ComponentTypeCode::U32),
                                      prim(ComponentTypeCode::U32)};
  FlatIter It(Flat);
  auto R = liftFlatValues(Cx, It, Types, /*MaxFlat=*/1u);
  ASSERT_TRUE(R.has_value());
  ASSERT_EQ(R->size(), 2u);
  EXPECT_EQ(std::get<uint32_t>((*R)[0]), 0x11u);
  EXPECT_EQ(std::get<uint32_t>((*R)[1]), 0x22u);
}

TEST_F(CanonABIMemFixture, LowerFlatValuesIndirectWithOutParam) {
  // 2 u32 values, MaxFlat=1, OutParam = 128 → store to memory at 128, return
  // empty vector (no realloc needed because OutParam is provided).
  std::vector<ComponentValVariant> Vs{ComponentValVariant{uint32_t{0x33u}},
                                      ComponentValVariant{uint32_t{0x44u}}};
  std::vector<ComponentValType> Types{prim(ComponentTypeCode::U32),
                                      prim(ComponentTypeCode::U32)};
  auto R = lowerFlatValues(Cx, Vs, Types, /*MaxFlat=*/1u,
                           /*OutParam=*/std::optional<uint32_t>{128u});
  ASSERT_TRUE(R.has_value());
  EXPECT_TRUE(R->empty());
  uint32_t V0 = 0, V1 = 0;
  ASSERT_TRUE(Mem.loadValue<uint32_t>(V0, 128).has_value());
  ASSERT_TRUE(Mem.loadValue<uint32_t>(V1, 132).has_value());
  EXPECT_EQ(V0, 0x33u);
  EXPECT_EQ(V1, 0x44u);
}

} // namespace
