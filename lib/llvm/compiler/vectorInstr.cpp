// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

#include <array>
#include <limits>
#include <numeric>

namespace WasmEdge {

Expect<void>
FunctionCompiler::compileVectorOp(const AST::Instruction &Instr) noexcept {
  switch (Instr.getOpCode()) {
  case OpCode::V128__load:
    compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int128x1Ty);
    break;
  case OpCode::V128__load8x8_s:
    compileVectorLoadOp(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        LLVM::Type::getVectorType(Context.Int8Ty, 8), Context.Int16x8Ty, true);
    break;
  case OpCode::V128__load8x8_u:
    compileVectorLoadOp(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        LLVM::Type::getVectorType(Context.Int8Ty, 8), Context.Int16x8Ty, false);
    break;
  case OpCode::V128__load16x4_s:
    compileVectorLoadOp(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        LLVM::Type::getVectorType(Context.Int16Ty, 4), Context.Int32x4Ty, true);
    break;
  case OpCode::V128__load16x4_u:
    compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(),
                        LLVM::Type::getVectorType(Context.Int16Ty, 4),
                        Context.Int32x4Ty, false);
    break;
  case OpCode::V128__load32x2_s:
    compileVectorLoadOp(
        Instr.getTargetIndex(), Instr.getMemoryOffset(), Instr.getMemoryAlign(),
        LLVM::Type::getVectorType(Context.Int32Ty, 2), Context.Int64x2Ty, true);
    break;
  case OpCode::V128__load32x2_u:
    compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(),
                        LLVM::Type::getVectorType(Context.Int32Ty, 2),
                        Context.Int64x2Ty, false);
    break;
  case OpCode::V128__load8_splat:
    compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int8Ty,
                       Context.Int8x16Ty);
    break;
  case OpCode::V128__load16_splat:
    compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int16Ty,
                       Context.Int16x8Ty);
    break;
  case OpCode::V128__load32_splat:
    compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int32Ty,
                       Context.Int32x4Ty);
    break;
  case OpCode::V128__load64_splat:
    compileSplatLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Context.Int64Ty,
                       Context.Int64x2Ty);
    break;
  case OpCode::V128__load32_zero:
    compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int32Ty,
                        Context.Int128Ty, false);
    break;
  case OpCode::V128__load64_zero:
    compileVectorLoadOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                        Instr.getMemoryAlign(), Context.Int64Ty,
                        Context.Int128Ty, false);
    break;
  case OpCode::V128__store:
    compileStoreOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                   Instr.getMemoryAlign(), Context.Int128x1Ty, false, true);
    break;
  case OpCode::V128__load8_lane:
    compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Instr.getMemoryLane(),
                      Context.Int8Ty, Context.Int8x16Ty);
    break;
  case OpCode::V128__load16_lane:
    compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Instr.getMemoryLane(),
                      Context.Int16Ty, Context.Int16x8Ty);
    break;
  case OpCode::V128__load32_lane:
    compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Instr.getMemoryLane(),
                      Context.Int32Ty, Context.Int32x4Ty);
    break;
  case OpCode::V128__load64_lane:
    compileLoadLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                      Instr.getMemoryAlign(), Instr.getMemoryLane(),
                      Context.Int64Ty, Context.Int64x2Ty);
    break;
  case OpCode::V128__store8_lane:
    compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Instr.getMemoryLane(),
                       Context.Int8Ty, Context.Int8x16Ty);
    break;
  case OpCode::V128__store16_lane:
    compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Instr.getMemoryLane(),
                       Context.Int16Ty, Context.Int16x8Ty);
    break;
  case OpCode::V128__store32_lane:
    compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Instr.getMemoryLane(),
                       Context.Int32Ty, Context.Int32x4Ty);
    break;
  case OpCode::V128__store64_lane:
    compileStoreLaneOp(Instr.getTargetIndex(), Instr.getMemoryOffset(),
                       Instr.getMemoryAlign(), Instr.getMemoryLane(),
                       Context.Int64Ty, Context.Int64x2Ty);
    break;

  // SIMD Const Instructions
  case OpCode::V128__const: {
    const auto Value = Instr.getNum().get<uint64x2_t>();
    auto Vector =
        LLVM::Value::getConstVector64(LLContext, {Value[0], Value[1]});
    stackPush(Builder.createBitCast(Vector, Context.Int64x2Ty));
    break;
  }

  // SIMD Shuffle Instructions
  case OpCode::I8x16__shuffle: {
    auto V2 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
    auto V1 = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
    const auto V3 = Instr.getNum().get<uint128_t>();
    std::array<uint8_t, 16> Mask;
    for (size_t I = 0; I < 16; ++I) {
      auto Num = static_cast<uint8_t>(V3 >> (I * 8));
      if constexpr (Endian::native == Endian::little) {
        Mask[I] = Num;
      } else {
        Mask[15 - I] = Num < 16 ? 15 - Num : 47 - Num;
      }
    }
    stackPush(Builder.createBitCast(
        Builder.createShuffleVector(
            V1, V2, LLVM::Value::getConstVector8(LLContext, Mask)),
        Context.Int64x2Ty));
    break;
  }

  // SIMD Lane Instructions
  case OpCode::I8x16__extract_lane_s:
    compileExtractLaneOp(Context.Int8x16Ty, Instr.getMemoryLane(),
                         Context.Int32Ty, true);
    break;
  case OpCode::I8x16__extract_lane_u:
    compileExtractLaneOp(Context.Int8x16Ty, Instr.getMemoryLane(),
                         Context.Int32Ty, false);
    break;
  case OpCode::I8x16__replace_lane:
    compileReplaceLaneOp(Context.Int8x16Ty, Instr.getMemoryLane());
    break;
  case OpCode::I16x8__extract_lane_s:
    compileExtractLaneOp(Context.Int16x8Ty, Instr.getMemoryLane(),
                         Context.Int32Ty, true);
    break;
  case OpCode::I16x8__extract_lane_u:
    compileExtractLaneOp(Context.Int16x8Ty, Instr.getMemoryLane(),
                         Context.Int32Ty, false);
    break;
  case OpCode::I16x8__replace_lane:
    compileReplaceLaneOp(Context.Int16x8Ty, Instr.getMemoryLane());
    break;
  case OpCode::I32x4__extract_lane:
    compileExtractLaneOp(Context.Int32x4Ty, Instr.getMemoryLane());
    break;
  case OpCode::I32x4__replace_lane:
    compileReplaceLaneOp(Context.Int32x4Ty, Instr.getMemoryLane());
    break;
  case OpCode::I64x2__extract_lane:
    compileExtractLaneOp(Context.Int64x2Ty, Instr.getMemoryLane());
    break;
  case OpCode::I64x2__replace_lane:
    compileReplaceLaneOp(Context.Int64x2Ty, Instr.getMemoryLane());
    break;
  case OpCode::F32x4__extract_lane:
    compileExtractLaneOp(Context.Floatx4Ty, Instr.getMemoryLane());
    break;
  case OpCode::F32x4__replace_lane:
    compileReplaceLaneOp(Context.Floatx4Ty, Instr.getMemoryLane());
    break;
  case OpCode::F64x2__extract_lane:
    compileExtractLaneOp(Context.Doublex2Ty, Instr.getMemoryLane());
    break;
  case OpCode::F64x2__replace_lane:
    compileReplaceLaneOp(Context.Doublex2Ty, Instr.getMemoryLane());
    break;

  // SIMD Numeric Instructions
  case OpCode::I8x16__swizzle:
    compileVectorSwizzle();
    break;
  case OpCode::I8x16__splat:
    compileSplatOp(Context.Int8x16Ty);
    break;
  case OpCode::I16x8__splat:
    compileSplatOp(Context.Int16x8Ty);
    break;
  case OpCode::I32x4__splat:
    compileSplatOp(Context.Int32x4Ty);
    break;
  case OpCode::I64x2__splat:
    compileSplatOp(Context.Int64x2Ty);
    break;
  case OpCode::F32x4__splat:
    compileSplatOp(Context.Floatx4Ty);
    break;
  case OpCode::F64x2__splat:
    compileSplatOp(Context.Doublex2Ty);
    break;
  case OpCode::I8x16__eq:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntEQ);
    break;
  case OpCode::I8x16__ne:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntNE);
    break;
  case OpCode::I8x16__lt_s:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSLT);
    break;
  case OpCode::I8x16__lt_u:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntULT);
    break;
  case OpCode::I8x16__gt_s:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSGT);
    break;
  case OpCode::I8x16__gt_u:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntUGT);
    break;
  case OpCode::I8x16__le_s:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSLE);
    break;
  case OpCode::I8x16__le_u:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntULE);
    break;
  case OpCode::I8x16__ge_s:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntSGE);
    break;
  case OpCode::I8x16__ge_u:
    compileVectorCompareOp(Context.Int8x16Ty, LLVMIntUGE);
    break;
  case OpCode::I16x8__eq:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntEQ);
    break;
  case OpCode::I16x8__ne:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntNE);
    break;
  case OpCode::I16x8__lt_s:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSLT);
    break;
  case OpCode::I16x8__lt_u:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntULT);
    break;
  case OpCode::I16x8__gt_s:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSGT);
    break;
  case OpCode::I16x8__gt_u:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntUGT);
    break;
  case OpCode::I16x8__le_s:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSLE);
    break;
  case OpCode::I16x8__le_u:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntULE);
    break;
  case OpCode::I16x8__ge_s:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntSGE);
    break;
  case OpCode::I16x8__ge_u:
    compileVectorCompareOp(Context.Int16x8Ty, LLVMIntUGE);
    break;
  case OpCode::I32x4__eq:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntEQ);
    break;
  case OpCode::I32x4__ne:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntNE);
    break;
  case OpCode::I32x4__lt_s:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSLT);
    break;
  case OpCode::I32x4__lt_u:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntULT);
    break;
  case OpCode::I32x4__gt_s:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSGT);
    break;
  case OpCode::I32x4__gt_u:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntUGT);
    break;
  case OpCode::I32x4__le_s:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSLE);
    break;
  case OpCode::I32x4__le_u:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntULE);
    break;
  case OpCode::I32x4__ge_s:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntSGE);
    break;
  case OpCode::I32x4__ge_u:
    compileVectorCompareOp(Context.Int32x4Ty, LLVMIntUGE);
    break;
  case OpCode::I64x2__eq:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntEQ);
    break;
  case OpCode::I64x2__ne:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntNE);
    break;
  case OpCode::I64x2__lt_s:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSLT);
    break;
  case OpCode::I64x2__gt_s:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSGT);
    break;
  case OpCode::I64x2__le_s:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSLE);
    break;
  case OpCode::I64x2__ge_s:
    compileVectorCompareOp(Context.Int64x2Ty, LLVMIntSGE);
    break;
  case OpCode::F32x4__eq:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOEQ, Context.Int32x4Ty);
    break;
  case OpCode::F32x4__ne:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealUNE, Context.Int32x4Ty);
    break;
  case OpCode::F32x4__lt:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLT, Context.Int32x4Ty);
    break;
  case OpCode::F32x4__gt:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGT, Context.Int32x4Ty);
    break;
  case OpCode::F32x4__le:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOLE, Context.Int32x4Ty);
    break;
  case OpCode::F32x4__ge:
    compileVectorCompareOp(Context.Floatx4Ty, LLVMRealOGE, Context.Int32x4Ty);
    break;
  case OpCode::F64x2__eq:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOEQ, Context.Int64x2Ty);
    break;
  case OpCode::F64x2__ne:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealUNE, Context.Int64x2Ty);
    break;
  case OpCode::F64x2__lt:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOLT, Context.Int64x2Ty);
    break;
  case OpCode::F64x2__gt:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOGT, Context.Int64x2Ty);
    break;
  case OpCode::F64x2__le:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOLE, Context.Int64x2Ty);
    break;
  case OpCode::F64x2__ge:
    compileVectorCompareOp(Context.Doublex2Ty, LLVMRealOGE, Context.Int64x2Ty);
    break;
  case OpCode::V128__not:
    Stack.back() = Builder.createNot(Stack.back());
    break;
  case OpCode::V128__and: {
    auto RHS = stackPop();
    auto LHS = stackPop();
    stackPush(Builder.createAnd(LHS, RHS));
    break;
  }
  case OpCode::V128__andnot: {
    auto RHS = stackPop();
    auto LHS = stackPop();
    stackPush(Builder.createAnd(LHS, Builder.createNot(RHS)));
    break;
  }
  case OpCode::V128__or: {
    auto RHS = stackPop();
    auto LHS = stackPop();
    stackPush(Builder.createOr(LHS, RHS));
    break;
  }
  case OpCode::V128__xor: {
    auto RHS = stackPop();
    auto LHS = stackPop();
    stackPush(Builder.createXor(LHS, RHS));
    break;
  }
  case OpCode::V128__bitselect: {
    auto C = stackPop();
    auto V2 = stackPop();
    auto V1 = stackPop();
    stackPush(
        Builder.createXor(Builder.createAnd(Builder.createXor(V1, V2), C), V2));
    break;
  }
  case OpCode::V128__any_true:
    compileVectorAnyTrue();
    break;
  case OpCode::I8x16__abs:
    compileVectorAbs(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__neg:
    compileVectorNeg(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__popcnt:
    compileVectorPopcnt();
    break;
  case OpCode::I8x16__all_true:
    compileVectorAllTrue(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__bitmask:
    compileVectorBitMask(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__narrow_i16x8_s:
    compileVectorNarrow(Context.Int16x8Ty, true);
    break;
  case OpCode::I8x16__narrow_i16x8_u:
    compileVectorNarrow(Context.Int16x8Ty, false);
    break;
  case OpCode::I8x16__shl:
    compileVectorShl(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__shr_s:
    compileVectorAShr(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__shr_u:
    compileVectorLShr(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__add:
    compileVectorVectorAdd(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__add_sat_s:
    compileVectorVectorAddSat(Context.Int8x16Ty, true);
    break;
  case OpCode::I8x16__add_sat_u:
    compileVectorVectorAddSat(Context.Int8x16Ty, false);
    break;
  case OpCode::I8x16__sub:
    compileVectorVectorSub(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__sub_sat_s:
    compileVectorVectorSubSat(Context.Int8x16Ty, true);
    break;
  case OpCode::I8x16__sub_sat_u:
    compileVectorVectorSubSat(Context.Int8x16Ty, false);
    break;
  case OpCode::I8x16__min_s:
    compileVectorVectorSMin(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__min_u:
    compileVectorVectorUMin(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__max_s:
    compileVectorVectorSMax(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__max_u:
    compileVectorVectorUMax(Context.Int8x16Ty);
    break;
  case OpCode::I8x16__avgr_u:
    compileVectorVectorUAvgr(Context.Int8x16Ty);
    break;
  case OpCode::I16x8__abs:
    compileVectorAbs(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__neg:
    compileVectorNeg(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__all_true:
    compileVectorAllTrue(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__bitmask:
    compileVectorBitMask(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__narrow_i32x4_s:
    compileVectorNarrow(Context.Int32x4Ty, true);
    break;
  case OpCode::I16x8__narrow_i32x4_u:
    compileVectorNarrow(Context.Int32x4Ty, false);
    break;
  case OpCode::I16x8__extend_low_i8x16_s:
    compileVectorExtend(Context.Int8x16Ty, true, true);
    break;
  case OpCode::I16x8__extend_high_i8x16_s:
    compileVectorExtend(Context.Int8x16Ty, true, false);
    break;
  case OpCode::I16x8__extend_low_i8x16_u:
    compileVectorExtend(Context.Int8x16Ty, false, true);
    break;
  case OpCode::I16x8__extend_high_i8x16_u:
    compileVectorExtend(Context.Int8x16Ty, false, false);
    break;
  case OpCode::I16x8__shl:
    compileVectorShl(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__shr_s:
    compileVectorAShr(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__shr_u:
    compileVectorLShr(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__add:
    compileVectorVectorAdd(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__add_sat_s:
    compileVectorVectorAddSat(Context.Int16x8Ty, true);
    break;
  case OpCode::I16x8__add_sat_u:
    compileVectorVectorAddSat(Context.Int16x8Ty, false);
    break;
  case OpCode::I16x8__sub:
    compileVectorVectorSub(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__sub_sat_s:
    compileVectorVectorSubSat(Context.Int16x8Ty, true);
    break;
  case OpCode::I16x8__sub_sat_u:
    compileVectorVectorSubSat(Context.Int16x8Ty, false);
    break;
  case OpCode::I16x8__mul:
    compileVectorVectorMul(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__min_s:
    compileVectorVectorSMin(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__min_u:
    compileVectorVectorUMin(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__max_s:
    compileVectorVectorSMax(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__max_u:
    compileVectorVectorUMax(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__avgr_u:
    compileVectorVectorUAvgr(Context.Int16x8Ty);
    break;
  case OpCode::I16x8__extmul_low_i8x16_s:
    compileVectorExtMul(Context.Int8x16Ty, true, true);
    break;
  case OpCode::I16x8__extmul_high_i8x16_s:
    compileVectorExtMul(Context.Int8x16Ty, true, false);
    break;
  case OpCode::I16x8__extmul_low_i8x16_u:
    compileVectorExtMul(Context.Int8x16Ty, false, true);
    break;
  case OpCode::I16x8__extmul_high_i8x16_u:
    compileVectorExtMul(Context.Int8x16Ty, false, false);
    break;
  case OpCode::I16x8__q15mulr_sat_s:
    compileVectorVectorQ15MulSat();
    break;
  case OpCode::I16x8__extadd_pairwise_i8x16_s:
    compileVectorExtAddPairwise(Context.Int8x16Ty, true);
    break;
  case OpCode::I16x8__extadd_pairwise_i8x16_u:
    compileVectorExtAddPairwise(Context.Int8x16Ty, false);
    break;
  case OpCode::I32x4__abs:
    compileVectorAbs(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__neg:
    compileVectorNeg(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__all_true:
    compileVectorAllTrue(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__bitmask:
    compileVectorBitMask(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__extend_low_i16x8_s:
    compileVectorExtend(Context.Int16x8Ty, true, true);
    break;
  case OpCode::I32x4__extend_high_i16x8_s:
    compileVectorExtend(Context.Int16x8Ty, true, false);
    break;
  case OpCode::I32x4__extend_low_i16x8_u:
    compileVectorExtend(Context.Int16x8Ty, false, true);
    break;
  case OpCode::I32x4__extend_high_i16x8_u:
    compileVectorExtend(Context.Int16x8Ty, false, false);
    break;
  case OpCode::I32x4__shl:
    compileVectorShl(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__shr_s:
    compileVectorAShr(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__shr_u:
    compileVectorLShr(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__add:
    compileVectorVectorAdd(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__sub:
    compileVectorVectorSub(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__mul:
    compileVectorVectorMul(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__min_s:
    compileVectorVectorSMin(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__min_u:
    compileVectorVectorUMin(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__max_s:
    compileVectorVectorSMax(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__max_u:
    compileVectorVectorUMax(Context.Int32x4Ty);
    break;
  case OpCode::I32x4__extmul_low_i16x8_s:
    compileVectorExtMul(Context.Int16x8Ty, true, true);
    break;
  case OpCode::I32x4__extmul_high_i16x8_s:
    compileVectorExtMul(Context.Int16x8Ty, true, false);
    break;
  case OpCode::I32x4__extmul_low_i16x8_u:
    compileVectorExtMul(Context.Int16x8Ty, false, true);
    break;
  case OpCode::I32x4__extmul_high_i16x8_u:
    compileVectorExtMul(Context.Int16x8Ty, false, false);
    break;
  case OpCode::I32x4__extadd_pairwise_i16x8_s:
    compileVectorExtAddPairwise(Context.Int16x8Ty, true);
    break;
  case OpCode::I32x4__extadd_pairwise_i16x8_u:
    compileVectorExtAddPairwise(Context.Int16x8Ty, false);
    break;
  case OpCode::I32x4__dot_i16x8_s: {
    auto ExtendTy = Context.Int16x8Ty.getExtendedElementVectorType();
    auto Undef = LLVM::Value::getUndef(ExtendTy);
    auto LHS = Builder.createSExt(
        Builder.createBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
    auto RHS = Builder.createSExt(
        Builder.createBitCast(stackPop(), Context.Int16x8Ty), ExtendTy);
    auto M = Builder.createMul(LHS, RHS);
    auto L = Builder.createShuffleVector(
        M, Undef, LLVM::Value::getConstVector32(LLContext, {0U, 2U, 4U, 6U}));
    auto R = Builder.createShuffleVector(
        M, Undef, LLVM::Value::getConstVector32(LLContext, {1U, 3U, 5U, 7U}));
    auto V = Builder.createAdd(L, R);
    stackPush(Builder.createBitCast(V, Context.Int64x2Ty));
    break;
  }
  case OpCode::I64x2__abs:
    compileVectorAbs(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__neg:
    compileVectorNeg(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__all_true:
    compileVectorAllTrue(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__bitmask:
    compileVectorBitMask(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__extend_low_i32x4_s:
    compileVectorExtend(Context.Int32x4Ty, true, true);
    break;
  case OpCode::I64x2__extend_high_i32x4_s:
    compileVectorExtend(Context.Int32x4Ty, true, false);
    break;
  case OpCode::I64x2__extend_low_i32x4_u:
    compileVectorExtend(Context.Int32x4Ty, false, true);
    break;
  case OpCode::I64x2__extend_high_i32x4_u:
    compileVectorExtend(Context.Int32x4Ty, false, false);
    break;
  case OpCode::I64x2__shl:
    compileVectorShl(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__shr_s:
    compileVectorAShr(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__shr_u:
    compileVectorLShr(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__add:
    compileVectorVectorAdd(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__sub:
    compileVectorVectorSub(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__mul:
    compileVectorVectorMul(Context.Int64x2Ty);
    break;
  case OpCode::I64x2__extmul_low_i32x4_s:
    compileVectorExtMul(Context.Int32x4Ty, true, true);
    break;
  case OpCode::I64x2__extmul_high_i32x4_s:
    compileVectorExtMul(Context.Int32x4Ty, true, false);
    break;
  case OpCode::I64x2__extmul_low_i32x4_u:
    compileVectorExtMul(Context.Int32x4Ty, false, true);
    break;
  case OpCode::I64x2__extmul_high_i32x4_u:
    compileVectorExtMul(Context.Int32x4Ty, false, false);
    break;
  case OpCode::F32x4__abs:
    compileVectorFAbs(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__neg:
    compileVectorFNeg(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__sqrt:
    compileVectorFSqrt(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__add:
    compileVectorVectorFAdd(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__sub:
    compileVectorVectorFSub(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__mul:
    compileVectorVectorFMul(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__div:
    compileVectorVectorFDiv(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__min:
    compileVectorVectorFMin(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__max:
    compileVectorVectorFMax(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__pmin:
    compileVectorVectorFPMin(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__pmax:
    compileVectorVectorFPMax(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__ceil:
    compileVectorFCeil(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__floor:
    compileVectorFFloor(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__trunc:
    compileVectorFTrunc(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__nearest:
    compileVectorFNearest(Context.Floatx4Ty);
    break;
  case OpCode::F64x2__abs:
    compileVectorFAbs(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__neg:
    compileVectorFNeg(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__sqrt:
    compileVectorFSqrt(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__add:
    compileVectorVectorFAdd(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__sub:
    compileVectorVectorFSub(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__mul:
    compileVectorVectorFMul(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__div:
    compileVectorVectorFDiv(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__min:
    compileVectorVectorFMin(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__max:
    compileVectorVectorFMax(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__pmin:
    compileVectorVectorFPMin(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__pmax:
    compileVectorVectorFPMax(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__ceil:
    compileVectorFCeil(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__floor:
    compileVectorFFloor(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__trunc:
    compileVectorFTrunc(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__nearest:
    compileVectorFNearest(Context.Doublex2Ty);
    break;
  case OpCode::I32x4__trunc_sat_f32x4_s:
    compileVectorTruncSatS32(Context.Floatx4Ty, false);
    break;
  case OpCode::I32x4__trunc_sat_f32x4_u:
    compileVectorTruncSatU32(Context.Floatx4Ty, false);
    break;
  case OpCode::F32x4__convert_i32x4_s:
    compileVectorConvertS(Context.Int32x4Ty, Context.Floatx4Ty, false);
    break;
  case OpCode::F32x4__convert_i32x4_u:
    compileVectorConvertU(Context.Int32x4Ty, Context.Floatx4Ty, false);
    break;
  case OpCode::I32x4__trunc_sat_f64x2_s_zero:
    compileVectorTruncSatS32(Context.Doublex2Ty, true);
    break;
  case OpCode::I32x4__trunc_sat_f64x2_u_zero:
    compileVectorTruncSatU32(Context.Doublex2Ty, true);
    break;
  case OpCode::F64x2__convert_low_i32x4_s:
    compileVectorConvertS(Context.Int32x4Ty, Context.Doublex2Ty, true);
    break;
  case OpCode::F64x2__convert_low_i32x4_u:
    compileVectorConvertU(Context.Int32x4Ty, Context.Doublex2Ty, true);
    break;
  case OpCode::F32x4__demote_f64x2_zero:
    compileVectorDemote();
    break;
  case OpCode::F64x2__promote_low_f32x4:
    compileVectorPromote();
    break;

  // Relaxed SIMD Instructions
  case OpCode::I8x16__relaxed_swizzle:
    compileVectorSwizzle();
    break;
  case OpCode::I32x4__relaxed_trunc_f32x4_s:
    compileVectorTruncSatS32(Context.Floatx4Ty, false);
    break;
  case OpCode::I32x4__relaxed_trunc_f32x4_u:
    compileVectorTruncSatU32(Context.Floatx4Ty, false);
    break;
  case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
    compileVectorTruncSatS32(Context.Doublex2Ty, true);
    break;
  case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
    compileVectorTruncSatU32(Context.Doublex2Ty, true);
    break;
  case OpCode::F32x4__relaxed_madd:
    compileVectorVectorMAdd(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__relaxed_nmadd:
    compileVectorVectorNMAdd(Context.Floatx4Ty);
    break;
  case OpCode::F64x2__relaxed_madd:
    compileVectorVectorMAdd(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__relaxed_nmadd:
    compileVectorVectorNMAdd(Context.Doublex2Ty);
    break;
  case OpCode::I8x16__relaxed_laneselect:
  case OpCode::I16x8__relaxed_laneselect:
  case OpCode::I32x4__relaxed_laneselect:
  case OpCode::I64x2__relaxed_laneselect: {
    auto C = stackPop();
    auto V2 = stackPop();
    auto V1 = stackPop();
    stackPush(
        Builder.createXor(Builder.createAnd(Builder.createXor(V1, V2), C), V2));
    break;
  }
  case OpCode::F32x4__relaxed_min:
    compileVectorVectorFMin(Context.Floatx4Ty);
    break;
  case OpCode::F32x4__relaxed_max:
    compileVectorVectorFMax(Context.Floatx4Ty);
    break;
  case OpCode::F64x2__relaxed_min:
    compileVectorVectorFMin(Context.Doublex2Ty);
    break;
  case OpCode::F64x2__relaxed_max:
    compileVectorVectorFMax(Context.Doublex2Ty);
    break;
  case OpCode::I16x8__relaxed_q15mulr_s:
    compileVectorVectorQ15MulSat();
    break;
  case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
    compileVectorRelaxedIntegerDotProduct();
    break;
  case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
    compileVectorRelaxedIntegerDotProductAdd();
    break;

    // Atomic Instructions
  default:
    assumingUnreachable();
  }
  return {};
}

void FunctionCompiler::compileExtractLaneOp(LLVM::Type VectorTy,
                                            unsigned Index) noexcept {
  auto Vector = Builder.createBitCast(Stack.back(), VectorTy);
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - Index - 1;
  }
  Stack.back() =
      Builder.createExtractElement(Vector, LLContext.getInt64(Index));
}

void FunctionCompiler::compileExtractLaneOp(LLVM::Type VectorTy, unsigned Index,
                                            LLVM::Type ExtendTy,
                                            bool Signed) noexcept {
  compileExtractLaneOp(VectorTy, Index);
  if (Signed) {
    Stack.back() = Builder.createSExt(Stack.back(), ExtendTy);
  } else {
    Stack.back() = Builder.createZExt(Stack.back(), ExtendTy);
  }
}

void FunctionCompiler::compileLoadLaneOp(unsigned MemoryIndex, uint64_t Offset,
                                         unsigned Alignment, unsigned Index,
                                         LLVM::Type LoadTy,
                                         LLVM::Type VectorTy) noexcept {
  auto Vector = stackPop();
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - 1 - Index;
  }
  auto Value = Stack.back();
  Stack.back() = Builder.createBitCast(
      Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                  Value, LLContext.getInt64(Index)),
      Context.Int64x2Ty);
}

void FunctionCompiler::compileReplaceLaneOp(LLVM::Type VectorTy,
                                            unsigned Index) noexcept {
  auto Value = Builder.createTrunc(stackPop(), VectorTy.getElementType());
  auto Vector = Stack.back();
  if constexpr (Endian::native == Endian::big) {
    Index = VectorTy.getVectorSize() - Index - 1;
  }
  Stack.back() = Builder.createBitCast(
      Builder.createInsertElement(Builder.createBitCast(Vector, VectorTy),
                                  Value, LLContext.getInt64(Index)),
      Context.Int64x2Ty);
}

void FunctionCompiler::compileSplatLoadOp(unsigned MemoryIndex, uint64_t Offset,
                                          unsigned Alignment, LLVM::Type LoadTy,
                                          LLVM::Type VectorTy) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  compileSplatOp(VectorTy);
}

void FunctionCompiler::compileSplatOp(LLVM::Type VectorTy) noexcept {
  auto Undef = LLVM::Value::getUndef(VectorTy);
  auto Zeros = LLVM::Value::getConstNull(
      LLVM::Type::getVectorType(Context.Int32Ty, VectorTy.getVectorSize()));
  auto Value = Builder.createTrunc(Stack.back(), VectorTy.getElementType());
  auto Vector =
      Builder.createInsertElement(Undef, Value, LLContext.getInt64(0));
  Vector = Builder.createShuffleVector(Vector, Undef, Zeros);

  Stack.back() = Builder.createBitCast(Vector, Context.Int64x2Ty);
}

void FunctionCompiler::compileStoreLaneOp(uint32_t MemoryIndex, uint64_t Offset,
                                          uint32_t Alignment, uint8_t Index,
                                          LLVM::Type LoadTy,
                                          LLVM::Type VectorTy) noexcept {
  auto Vector = Stack.back();
  if constexpr (Endian::native == Endian::big) {
    Index = static_cast<uint8_t>(VectorTy.getVectorSize() - Index - 1);
  }
  Stack.back() = Builder.createExtractElement(
      Builder.createBitCast(Vector, VectorTy), LLContext.getInt64(Index));
  compileStoreOp(MemoryIndex, Offset, Alignment, LoadTy);
}

void FunctionCompiler::compileVectorAbs(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    return Builder.createIntrinsic(LLVM::Core::Abs, {V.getType()},
                                   {V, LLContext.getFalse()});
  });
}

void FunctionCompiler::compileVectorAllTrue(LLVM::Type VectorTy) noexcept {
  compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto IntType = LLContext.getIntNTy(Size);
    auto Zero = LLVM::Value::getConstNull(VectorTy);
    auto Cmp = Builder.createBitCast(Builder.createICmpEQ(V, Zero), IntType);
    auto CmpZero = LLVM::Value::getConstInt(IntType, 0);
    return Builder.createICmpEQ(Cmp, CmpZero);
  });
}

void FunctionCompiler::compileVectorAnyTrue() noexcept {
  compileVectorReduceIOp(Context.Int128x1Ty, [this](auto V) noexcept {
    auto Zero = LLVM::Value::getConstNull(Context.Int128x1Ty);
    return Builder.createBitCast(Builder.createICmpNE(V, Zero),
                                 LLContext.getInt1Ty());
  });
}

void FunctionCompiler::compileVectorAShr(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createAShr(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorBitMask(LLVM::Type VectorTy) noexcept {
  compileVectorReduceIOp(VectorTy, [this, VectorTy](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto IntType = LLContext.getIntNTy(Size);
    auto Zero = LLVM::Value::getConstNull(VectorTy);
    return Builder.createBitCast(Builder.createICmpSLT(V, Zero), IntType);
  });
}

void FunctionCompiler::compileVectorCompareOp(
    LLVM::Type VectorTy, LLVMIntPredicate Predicate) noexcept {
  auto RHS = stackPop();
  auto LHS = stackPop();
  auto Result = Builder.createSExt(
      Builder.createICmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                         Builder.createBitCast(RHS, VectorTy)),
      VectorTy);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorCompareOp(LLVM::Type VectorTy,
                                              LLVMRealPredicate Predicate,
                                              LLVM::Type ResultTy) noexcept {
  auto RHS = stackPop();
  auto LHS = stackPop();
  auto Result = Builder.createSExt(
      Builder.createFCmp(Predicate, Builder.createBitCast(LHS, VectorTy),
                         Builder.createBitCast(RHS, VectorTy)),
      ResultTy);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorConvertS(LLVM::Type VectorTy,
                                             LLVM::Type FPVectorTy,
                                             bool Low) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
    if (Low) {
      const auto Size = VectorTy.getVectorSize() / 2;
      std::vector<uint32_t> Mask(Size);
      if constexpr (Endian::native == Endian::little) {
        std::iota(Mask.begin(), Mask.end(), 0);
      } else {
        std::iota(Mask.begin(), Mask.end(), Size);
      }
      V = Builder.createShuffleVector(
          V, LLVM::Value::getUndef(VectorTy),
          LLVM::Value::getConstVector32(LLContext, Mask));
    }
    return Builder.createSIToFP(V, FPVectorTy);
  });
}

void FunctionCompiler::compileVectorConvertU(LLVM::Type VectorTy,
                                             LLVM::Type FPVectorTy,
                                             bool Low) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, FPVectorTy, Low](auto V) noexcept {
    if (Low) {
      const auto Size = VectorTy.getVectorSize() / 2;
      std::vector<uint32_t> Mask(Size);
      if constexpr (Endian::native == Endian::little) {
        std::iota(Mask.begin(), Mask.end(), 0);
      } else {
        std::iota(Mask.begin(), Mask.end(), Size);
      }
      V = Builder.createShuffleVector(
          V, LLVM::Value::getUndef(VectorTy),
          LLVM::Value::getConstVector32(LLContext, Mask));
    }
    return Builder.createUIToFP(V, FPVectorTy);
  });
}

void FunctionCompiler::compileVectorDemote() noexcept {
  compileVectorOp(Context.Doublex2Ty, [this](auto V) noexcept {
    auto Demoted =
        Builder.createFPTrunc(V, LLVM::Type::getVectorType(Context.FloatTy, 2));
    auto ZeroV = LLVM::Value::getConstNull(Demoted.getType());
    if constexpr (Endian::native == Endian::little) {
      return Builder.createShuffleVector(
          Demoted, ZeroV,
          LLVM::Value::getConstVector32(LLContext, {0u, 1u, 2u, 3u}));
    } else {
      return Builder.createShuffleVector(
          Demoted, ZeroV,
          LLVM::Value::getConstVector32(LLContext, {3u, 2u, 1u, 0u}));
    }
  });
}

void FunctionCompiler::compileVectorExtAddPairwise(LLVM::Type VectorTy,
                                                   bool Signed) noexcept {
  compileVectorOp(
      VectorTy, [this, VectorTy, Signed](auto V) noexcept -> LLVM::Value {
        auto ExtTy =
            VectorTy.getExtendedElementVectorType().getHalfElementsVectorType();
#if defined(__x86_64__)
        const auto Count = VectorTy.getVectorSize();
        if (Context.SupportXOP) {
          const auto ID = [Count, Signed]() noexcept {
            switch (Count) {
            case 8:
              return Signed ? LLVM::Core::X86XOpVPHAddWD
                            : LLVM::Core::X86XOpVPHAddUWD;
            case 16:
              return Signed ? LLVM::Core::X86XOpVPHAddBW
                            : LLVM::Core::X86XOpVPHAddUBW;
            default:
              assumingUnreachable();
            }
          }();
          assuming(ID != LLVM::Core::NotIntrinsic);
          return Builder.createUnaryIntrinsic(ID, V);
        }
        if (Context.SupportSSSE3 && Count == 16) {
          assuming(LLVM::Core::X86SSSE3PMAddUbSw128 !=
                   LLVM::Core::NotIntrinsic);
          if (Signed) {
            return Builder.createIntrinsic(
                LLVM::Core::X86SSSE3PMAddUbSw128, {},
                {Builder.createVectorSplat(16, LLContext.getInt8(1)), V});
          } else {
            return Builder.createIntrinsic(
                LLVM::Core::X86SSSE3PMAddUbSw128, {},
                {V, Builder.createVectorSplat(16, LLContext.getInt8(1))});
          }
        }
        if (Context.SupportSSE2 && Count == 8) {
          assuming(LLVM::Core::X86SSE2PMAddWd != LLVM::Core::NotIntrinsic);
          if (Signed) {
            return Builder.createIntrinsic(
                LLVM::Core::X86SSE2PMAddWd, {},
                {V, Builder.createVectorSplat(8, LLContext.getInt16(1))});
          } else {
            V = Builder.createXor(
                V, Builder.createVectorSplat(8, LLContext.getInt16(0x8000)));
            V = Builder.createIntrinsic(
                LLVM::Core::X86SSE2PMAddWd, {},
                {V, Builder.createVectorSplat(8, LLContext.getInt16(1))});
            return Builder.createAdd(
                V, Builder.createVectorSplat(4, LLContext.getInt32(0x10000)));
          }
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          const auto ID = Signed ? LLVM::Core::AArch64NeonSAddLP
                                 : LLVM::Core::AArch64NeonUAddLP;
          assuming(ID != LLVM::Core::NotIntrinsic);
          return Builder.createIntrinsic(ID, {ExtTy, VectorTy}, {V});
        }
#endif

        // Fallback case.
        // If the XOP, SSSE3, or SSE2 is not supported on the x86_64 platform
        // or the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto Width = LLVM::Value::getConstInt(
            ExtTy.getElementType(),
            VectorTy.getElementType().getIntegerBitWidth());
        Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
        auto EV = Builder.createBitCast(V, ExtTy);
        LLVM::Value L, R;
        if (Signed) {
          L = Builder.createAShr(EV, Width);
          R = Builder.createAShr(Builder.createShl(EV, Width), Width);
        } else {
          L = Builder.createLShr(EV, Width);
          R = Builder.createLShr(Builder.createShl(EV, Width), Width);
        }
        return Builder.createAdd(L, R);
      });
}

void FunctionCompiler::compileVectorExtend(LLVM::Type FromTy, bool Signed,
                                           bool Low) noexcept {
  auto ExtTy = FromTy.getExtendedElementVectorType();
  const auto Count = FromTy.getVectorSize();
  std::vector<uint32_t> Mask(Count / 2);
  if constexpr (Endian::native == Endian::big) {
    Low = !Low;
  }
  std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
  auto R = Builder.createBitCast(Stack.back(), FromTy);
  if (Signed) {
    R = Builder.createSExt(R, ExtTy);
  } else {
    R = Builder.createZExt(R, ExtTy);
  }
  R = Builder.createShuffleVector(
      R, LLVM::Value::getUndef(ExtTy),
      LLVM::Value::getConstVector32(LLContext, Mask));
  Stack.back() = Builder.createBitCast(R, Context.Int64x2Ty);
}

void FunctionCompiler::compileVectorExtMul(LLVM::Type FromTy, bool Signed,
                                           bool Low) noexcept {
  auto ExtTy = FromTy.getExtendedElementVectorType();
  const auto Count = FromTy.getVectorSize();
  std::vector<uint32_t> Mask(Count / 2);
  std::iota(Mask.begin(), Mask.end(), Low ? 0 : Count / 2);
  auto Extend = [this, FromTy, Signed, ExtTy, &Mask](LLVM::Value R) noexcept {
    R = Builder.createBitCast(R, FromTy);
    if (Signed) {
      R = Builder.createSExt(R, ExtTy);
    } else {
      R = Builder.createZExt(R, ExtTy);
    }
    return Builder.createShuffleVector(
        R, LLVM::Value::getUndef(ExtTy),
        LLVM::Value::getConstVector32(LLContext, Mask));
  };
  auto RHS = Extend(stackPop());
  auto LHS = Extend(stackPop());
  stackPush(
      Builder.createBitCast(Builder.createMul(RHS, LHS), Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorFAbs(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Fabs != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Fabs, V);
  });
}

void FunctionCompiler::compileVectorFCeil(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Ceil != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Ceil, V);
  });
}

void FunctionCompiler::compileVectorFFloor(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Floor != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Floor, V);
  });
}

void FunctionCompiler::compileVectorFNearest(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [&](auto V) noexcept {
#if LLVM_VERSION_MAJOR >= 12 && !defined(__s390x__)
    assuming(LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic);
    if (LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic) {
      return Builder.createUnaryIntrinsic(LLVM::Core::Roundeven, V);
    }
#endif

#if defined(__x86_64__)
    if (Context.SupportSSE4_1) {
      const bool IsFloat = VectorTy.getElementType().isFloatTy();
      auto ID =
          IsFloat ? LLVM::Core::X86SSE41RoundPs : LLVM::Core::X86SSE41RoundPd;
      assuming(ID != LLVM::Core::NotIntrinsic);
      return Builder.createIntrinsic(ID, {}, {V, LLContext.getInt32(8)});
    }
#endif

#if defined(__aarch64__)
    if (Context.SupportNEON &&
        LLVM::Core::AArch64NeonFRIntN != LLVM::Core::NotIntrinsic) {
      return Builder.createUnaryIntrinsic(LLVM::Core::AArch64NeonFRIntN, V);
    }
#endif

    // Fallback case.
    // If the SSE4.1 is not supported on the x86_64 platform or
    // the NEON is not supported on the aarch64 platform,
    // then fallback to this.
    assuming(LLVM::Core::Nearbyint != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Nearbyint, V);
  });
}

void FunctionCompiler::compileVectorFNeg(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy,
                  [this](auto V) noexcept { return Builder.createFNeg(V); });
}

void FunctionCompiler::compileVectorFSqrt(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Sqrt != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Sqrt, V);
  });
}

void FunctionCompiler::compileVectorFTrunc(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy, [this](auto V) noexcept {
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Trunc, V);
  });
}

void FunctionCompiler::compileVectorLoadOp(unsigned MemoryIndex,
                                           uint64_t Offset, unsigned Alignment,
                                           LLVM::Type LoadTy) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy);
  Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
}

void FunctionCompiler::compileVectorLoadOp(unsigned MemoryIndex,
                                           uint64_t Offset, unsigned Alignment,
                                           LLVM::Type LoadTy,
                                           LLVM::Type ExtendTy,
                                           bool Signed) noexcept {
  compileLoadOp(MemoryIndex, Offset, Alignment, LoadTy, ExtendTy, Signed);
  Stack.back() = Builder.createBitCast(Stack.back(), Context.Int64x2Ty);
}

void FunctionCompiler::compileVectorLShr(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createLShr(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorNarrow(LLVM::Type FromTy,
                                           bool Signed) noexcept {
  auto [MinInt,
        MaxInt] = [&]() noexcept -> std::tuple<LLVM::Value, LLVM::Value> {
    switch (FromTy.getElementType().getIntegerBitWidth()) {
    case 16: {
      const auto Min =
          static_cast<int16_t>(Signed ? std::numeric_limits<int8_t>::min()
                                      : std::numeric_limits<uint8_t>::min());
      const auto Max =
          static_cast<int16_t>(Signed ? std::numeric_limits<int8_t>::max()
                                      : std::numeric_limits<uint8_t>::max());
      return {LLContext.getInt16(static_cast<uint16_t>(Min)),
              LLContext.getInt16(static_cast<uint16_t>(Max))};
    }
    case 32: {
      const auto Min =
          static_cast<int32_t>(Signed ? std::numeric_limits<int16_t>::min()
                                      : std::numeric_limits<uint16_t>::min());
      const auto Max =
          static_cast<int32_t>(Signed ? std::numeric_limits<int16_t>::max()
                                      : std::numeric_limits<uint16_t>::max());
      return {LLContext.getInt32(static_cast<uint32_t>(Min)),
              LLContext.getInt32(static_cast<uint32_t>(Max))};
    }
    default:
      assumingUnreachable();
    }
  }();
  const auto Count = FromTy.getVectorSize();
  auto VMin = Builder.createVectorSplat(Count, MinInt);
  auto VMax = Builder.createVectorSplat(Count, MaxInt);

  auto TruncTy = FromTy.getTruncatedElementVectorType();

  auto F2 = Builder.createBitCast(stackPop(), FromTy);
  F2 = Builder.createSelect(Builder.createICmpSLT(F2, VMin), VMin, F2);
  F2 = Builder.createSelect(Builder.createICmpSGT(F2, VMax), VMax, F2);
  F2 = Builder.createTrunc(F2, TruncTy);

  auto F1 = Builder.createBitCast(stackPop(), FromTy);
  F1 = Builder.createSelect(Builder.createICmpSLT(F1, VMin), VMin, F1);
  F1 = Builder.createSelect(Builder.createICmpSGT(F1, VMax), VMax, F1);
  F1 = Builder.createTrunc(F1, TruncTy);

  std::vector<uint32_t> Mask(Count * 2);
  std::iota(Mask.begin(), Mask.end(), 0);
  auto V = Endian::native == Endian::little
               ? Builder.createShuffleVector(
                     F1, F2, LLVM::Value::getConstVector32(LLContext, Mask))
               : Builder.createShuffleVector(
                     F2, F1, LLVM::Value::getConstVector32(LLContext, Mask));
  stackPush(Builder.createBitCast(V, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorNeg(LLVM::Type VectorTy) noexcept {
  compileVectorOp(VectorTy,
                  [this](auto V) noexcept { return Builder.createNeg(V); });
}

void FunctionCompiler::compileVectorPopcnt() noexcept {
  compileVectorOp(Context.Int8x16Ty, [this](auto V) noexcept {
    assuming(LLVM::Core::Ctpop != LLVM::Core::NotIntrinsic);
    return Builder.createUnaryIntrinsic(LLVM::Core::Ctpop, V);
  });
}

void FunctionCompiler::compileVectorPromote() noexcept {
  compileVectorOp(Context.Floatx4Ty, [this](auto V) noexcept {
    auto UndefV = LLVM::Value::getUndef(V.getType());
    auto Low = Builder.createShuffleVector(
        V, UndefV, LLVM::Value::getConstVector32(LLContext, {0u, 1u}));
    return Builder.createFPExt(Low,
                               LLVM::Type::getVectorType(Context.DoubleTy, 2));
  });
}

void FunctionCompiler::compileVectorRelaxedIntegerDotProduct() noexcept {
  auto OriTy = Context.Int8x16Ty;
  auto ExtTy = Context.Int16x8Ty;
  auto RHS = Builder.createBitCast(stackPop(), OriTy);
  auto LHS = Builder.createBitCast(stackPop(), OriTy);
#if defined(__x86_64__)
  if (Context.SupportSSSE3) {
    assuming(LLVM::Core::X86SSSE3PMAddUbSw128 != LLVM::Core::NotIntrinsic);
    // WebAssembly Relaxed SIMD spec: signed(LHS) * unsigned/signed(RHS)
    // But PMAddUbSw128 is unsigned(LHS) * signed(RHS). Therefore swap both
    // side to match the WebAssembly spec
    return stackPush(Builder.createBitCast(
        Builder.createIntrinsic(LLVM::Core::X86SSSE3PMAddUbSw128, {},
                                {RHS, LHS}),
        Context.Int64x2Ty));
  }
#endif
  auto Width = LLVM::Value::getConstInt(
      ExtTy.getElementType(), OriTy.getElementType().getIntegerBitWidth());
  Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
  auto EA = Builder.createBitCast(LHS, ExtTy);
  auto EB = Builder.createBitCast(RHS, ExtTy);

  LLVM::Value AL, AR, BL, BR;
  AL = Builder.createAShr(EA, Width);
  AR = Builder.createAShr(Builder.createShl(EA, Width), Width);
  BL = Builder.createAShr(EB, Width);
  BR = Builder.createAShr(Builder.createShl(EB, Width), Width);

  return stackPush(Builder.createBitCast(
      Builder.createAdd(Builder.createMul(AL, BL), Builder.createMul(AR, BR)),
      Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorRelaxedIntegerDotProductAdd() noexcept {
  auto OriTy = Context.Int8x16Ty;
  auto ExtTy = Context.Int16x8Ty;
  auto FinTy = Context.Int32x4Ty;
  auto VC = Builder.createBitCast(stackPop(), FinTy);
  auto RHS = Builder.createBitCast(stackPop(), OriTy);
  auto LHS = Builder.createBitCast(stackPop(), OriTy);
  LLVM::Value IM;
#if defined(__x86_64__)
  if (Context.SupportSSSE3) {
    assuming(LLVM::Core::X86SSSE3PMAddUbSw128 != LLVM::Core::NotIntrinsic);
    // WebAssembly Relaxed SIMD spec: signed(LHS) * unsigned/signed(RHS)
    // But PMAddUbSw128 is unsigned(LHS) * signed(RHS). Therefore swap both
    // side to match the WebAssembly spec
    IM = Builder.createIntrinsic(LLVM::Core::X86SSSE3PMAddUbSw128, {},
                                 {RHS, LHS});
  } else
#endif
  {
    auto Width = LLVM::Value::getConstInt(
        ExtTy.getElementType(), OriTy.getElementType().getIntegerBitWidth());
    Width = Builder.createVectorSplat(ExtTy.getVectorSize(), Width);
    auto EA = Builder.createBitCast(LHS, ExtTy);
    auto EB = Builder.createBitCast(RHS, ExtTy);

    LLVM::Value AL, AR, BL, BR;
    AL = Builder.createAShr(EA, Width);
    AR = Builder.createAShr(Builder.createShl(EA, Width), Width);
    BL = Builder.createAShr(EB, Width);
    BR = Builder.createAShr(Builder.createShl(EB, Width), Width);
    IM =
        Builder.createAdd(Builder.createMul(AL, BL), Builder.createMul(AR, BR));
  }

  auto Width = LLVM::Value::getConstInt(
      FinTy.getElementType(), ExtTy.getElementType().getIntegerBitWidth());
  Width = Builder.createVectorSplat(FinTy.getVectorSize(), Width);
  auto IME = Builder.createBitCast(IM, FinTy);
  auto L = Builder.createAShr(IME, Width);
  auto R = Builder.createAShr(Builder.createShl(IME, Width), Width);

  return stackPush(Builder.createBitCast(
      Builder.createAdd(Builder.createAdd(L, R), VC), Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorShl(LLVM::Type VectorTy) noexcept {
  compileVectorShiftOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createShl(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorSwizzle() noexcept {
  auto Index = Builder.createBitCast(stackPop(), Context.Int8x16Ty);
  auto Vector = Builder.createBitCast(stackPop(), Context.Int8x16Ty);

#if defined(__x86_64__)
  if (Context.SupportSSSE3) {
    auto Magic = Builder.createVectorSplat(16, LLContext.getInt8(112));
    auto Added = Builder.createAdd(Index, Magic);
    auto NewIndex = Builder.createSelect(
        Builder.createICmpUGT(Index, Added),
        LLVM::Value::getConstAllOnes(Context.Int8x16Ty), Added);
    assuming(LLVM::Core::X86SSSE3PShufB128 != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createBitCast(
        Builder.createIntrinsic(LLVM::Core::X86SSSE3PShufB128, {},
                                {Vector, NewIndex}),
        Context.Int64x2Ty));
    return;
  }
#endif

#if defined(__aarch64__)
  if (Context.SupportNEON) {
    assuming(LLVM::Core::AArch64NeonTbl1 != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createBitCast(
        Builder.createIntrinsic(LLVM::Core::AArch64NeonTbl1,
                                {Context.Int8x16Ty}, {Vector, Index}),
        Context.Int64x2Ty));
    return;
  }
#endif

  auto Mask = Builder.createVectorSplat(16, LLContext.getInt8(15));
  auto Zero = Builder.createVectorSplat(16, LLContext.getInt8(0));

#if defined(__s390x__)
  assuming(LLVM::Core::S390VPerm != LLVM::Core::NotIntrinsic);
  auto Exceed = Builder.createICmpULE(Index, Mask);
  Index = Builder.createSub(Mask, Index);
  auto Result =
      Builder.createIntrinsic(LLVM::Core::S390VPerm, {}, {Vector, Zero, Index});
  Result = Builder.createSelect(Exceed, Result, Zero);
  stackPush(Builder.createBitCast(Result, Context.Int64x2Ty));
  return;
#endif

  // Fallback case.
  // If the SSSE3 is not supported on the x86_64 platform or
  // the NEON is not supported on the aarch64 platform,
  // then fallback to this.
  auto IsOver = Builder.createICmpUGT(Index, Mask);
  auto InboundIndex = Builder.createAnd(Index, Mask);
  auto Array = Builder.createArray(16, 1);
  for (size_t I = 0; I < 16; ++I) {
    Builder.createStore(
        Builder.createExtractElement(Vector, LLContext.getInt64(I)),
        Builder.createInBoundsGEP1(Context.Int8Ty, Array,
                                   LLContext.getInt64(I)));
  }
  LLVM::Value Ret = LLVM::Value::getUndef(Context.Int8x16Ty);
  for (size_t I = 0; I < 16; ++I) {
    auto Idx =
        Builder.createExtractElement(InboundIndex, LLContext.getInt64(I));
    auto Value = Builder.createLoad(
        Context.Int8Ty, Builder.createInBoundsGEP1(Context.Int8Ty, Array, Idx));
    Ret = Builder.createInsertElement(Ret, Value, LLContext.getInt64(I));
  }
  Ret = Builder.createSelect(IsOver, Zero, Ret);
  stackPush(Builder.createBitCast(Ret, Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorTruncSatS32(LLVM::Type VectorTy,
                                                bool PadZero) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, PadZero](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto FPTy = VectorTy.getElementType();
    auto IntMin = LLContext.getInt32(
        static_cast<uint32_t>(std::numeric_limits<int32_t>::min()));
    auto IntMax = LLContext.getInt32(
        static_cast<uint32_t>(std::numeric_limits<int32_t>::max()));
    auto IntMinV = Builder.createVectorSplat(Size, IntMin);
    auto IntMaxV = Builder.createVectorSplat(Size, IntMax);
    auto IntZeroV = LLVM::Value::getConstNull(IntMinV.getType());
    auto FPMin = Builder.createSIToFP(IntMin, FPTy);
    auto FPMax = Builder.createSIToFP(IntMax, FPTy);
    auto FPMinV = Builder.createVectorSplat(Size, FPMin);
    auto FPMaxV = Builder.createVectorSplat(Size, FPMax);

    auto Normal = Builder.createFCmpORD(V, V);
    auto NotUnder = Builder.createFCmpUGE(V, FPMinV);
    auto NotOver = Builder.createFCmpULT(V, FPMaxV);
    V = Builder.createFPToSI(
        V, LLVM::Type::getVectorType(LLContext.getInt32Ty(), Size));
    V = Builder.createSelect(Normal, V, IntZeroV);
    V = Builder.createSelect(NotUnder, V, IntMinV);
    V = Builder.createSelect(NotOver, V, IntMaxV);
    if (PadZero) {
      std::vector<uint32_t> Mask(Size * 2);
      std::iota(Mask.begin(), Mask.end(), 0);
      if constexpr (Endian::native == Endian::little) {
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      } else {
        V = Builder.createShuffleVector(
            IntZeroV, V, LLVM::Value::getConstVector32(LLContext, Mask));
      }
    }
    return V;
  });
}

void FunctionCompiler::compileVectorTruncSatU32(LLVM::Type VectorTy,
                                                bool PadZero) noexcept {
  compileVectorOp(VectorTy, [this, VectorTy, PadZero](auto V) noexcept {
    const auto Size = VectorTy.getVectorSize();
    auto FPTy = VectorTy.getElementType();
    auto IntMin = LLContext.getInt32(std::numeric_limits<uint32_t>::min());
    auto IntMax = LLContext.getInt32(std::numeric_limits<uint32_t>::max());
    auto IntMinV = Builder.createVectorSplat(Size, IntMin);
    auto IntMaxV = Builder.createVectorSplat(Size, IntMax);
    auto FPMin = Builder.createUIToFP(IntMin, FPTy);
    auto FPMax = Builder.createUIToFP(IntMax, FPTy);
    auto FPMinV = Builder.createVectorSplat(Size, FPMin);
    auto FPMaxV = Builder.createVectorSplat(Size, FPMax);

    auto NotUnder = Builder.createFCmpOGE(V, FPMinV);
    auto NotOver = Builder.createFCmpULT(V, FPMaxV);
    V = Builder.createFPToUI(
        V, LLVM::Type::getVectorType(LLContext.getInt32Ty(), Size));
    V = Builder.createSelect(NotUnder, V, IntMinV);
    V = Builder.createSelect(NotOver, V, IntMaxV);
    if (PadZero) {
      auto IntZeroV = LLVM::Value::getConstNull(IntMinV.getType());
      std::vector<uint32_t> Mask(Size * 2);
      std::iota(Mask.begin(), Mask.end(), 0);
      if constexpr (Endian::native == Endian::little) {
        V = Builder.createShuffleVector(
            V, IntZeroV, LLVM::Value::getConstVector32(LLContext, Mask));
      } else {
        V = Builder.createShuffleVector(
            IntZeroV, V, LLVM::Value::getConstVector32(LLContext, Mask));
      }
    }
    return V;
  });
}

void FunctionCompiler::compileVectorVectorAdd(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createAdd(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorAddSat(LLVM::Type VectorTy,
                                                 bool Signed) noexcept {
  auto ID = Signed ? LLVM::Core::SAddSat : LLVM::Core::UAddSat;
  assuming(ID != LLVM::Core::NotIntrinsic);
  compileVectorVectorOp(
      VectorTy, [this, VectorTy, ID](auto LHS, auto RHS) noexcept {
        return Builder.createIntrinsic(ID, {VectorTy}, {LHS, RHS});
      });
}

void FunctionCompiler::compileVectorVectorFAdd(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFAdd(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFDiv(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFDiv(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto LNaN = Builder.createFCmpUNO(LHS, LHS);
    auto RNaN = Builder.createFCmpUNO(RHS, RHS);
    auto OLT = Builder.createFCmpOLT(LHS, RHS);
    auto OGT = Builder.createFCmpOGT(LHS, RHS);
    auto Ret = Builder.createBitCast(
        Builder.createAnd(Builder.createBitCast(LHS, Context.Int64x2Ty),
                          Builder.createBitCast(RHS, Context.Int64x2Ty)),
        LHS.getType());
    Ret = Builder.createSelect(OLT, RHS, Ret);
    Ret = Builder.createSelect(OGT, LHS, Ret);
    Ret = Builder.createSelect(LNaN, LHS, Ret);
    Ret = Builder.createSelect(RNaN, RHS, Ret);
    return Ret;
  });
}

void FunctionCompiler::compileVectorVectorFMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto LNaN = Builder.createFCmpUNO(LHS, LHS);
    auto RNaN = Builder.createFCmpUNO(RHS, RHS);
    auto OLT = Builder.createFCmpOLT(LHS, RHS);
    auto OGT = Builder.createFCmpOGT(LHS, RHS);
    auto Ret = Builder.createBitCast(
        Builder.createOr(Builder.createBitCast(LHS, Context.Int64x2Ty),
                         Builder.createBitCast(RHS, Context.Int64x2Ty)),
        LHS.getType());
    Ret = Builder.createSelect(OGT, RHS, Ret);
    Ret = Builder.createSelect(OLT, LHS, Ret);
    Ret = Builder.createSelect(LNaN, LHS, Ret);
    Ret = Builder.createSelect(RNaN, RHS, Ret);
    return Ret;
  });
}

void FunctionCompiler::compileVectorVectorFMul(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFMul(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorFPMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto Cmp = Builder.createFCmpOGT(RHS, LHS);
    return Builder.createSelect(Cmp, RHS, LHS);
  });
}

void FunctionCompiler::compileVectorVectorFPMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    auto Cmp = Builder.createFCmpOLT(RHS, LHS);
    return Builder.createSelect(Cmp, RHS, LHS);
  });
}

void FunctionCompiler::compileVectorVectorFSub(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createFSub(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorMAdd(LLVM::Type VectorTy) noexcept {
  auto C = Builder.createBitCast(stackPop(), VectorTy);
  auto RHS = Builder.createBitCast(stackPop(), VectorTy);
  auto LHS = Builder.createBitCast(stackPop(), VectorTy);
  stackPush(Builder.createBitCast(
      Builder.createFAdd(Builder.createFMul(LHS, RHS), C), Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorVectorMul(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createMul(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorNMAdd(LLVM::Type VectorTy) noexcept {
  auto C = Builder.createBitCast(stackPop(), VectorTy);
  auto RHS = Builder.createBitCast(stackPop(), VectorTy);
  auto LHS = Builder.createBitCast(stackPop(), VectorTy);
  stackPush(Builder.createBitCast(
      Builder.createFAdd(Builder.createFMul(Builder.createFNeg(LHS), RHS), C),
      Context.Int64x2Ty));
}

void FunctionCompiler::compileVectorVectorQ15MulSat() noexcept {
  compileVectorVectorOp(
      Context.Int16x8Ty, [this](auto LHS, auto RHS) noexcept -> LLVM::Value {
#if defined(__x86_64__)
        if (Context.SupportSSSE3) {
          assuming(LLVM::Core::X86SSSE3PMulHrSw128 != LLVM::Core::NotIntrinsic);
          auto Result = Builder.createIntrinsic(LLVM::Core::X86SSSE3PMulHrSw128,
                                                {}, {LHS, RHS});
          auto IntMaxV = Builder.createVectorSplat(
              8, LLContext.getInt16(UINT16_C(0x8000)));
          auto NotOver = Builder.createSExt(
              Builder.createICmpEQ(Result, IntMaxV), Context.Int16x8Ty);
          return Builder.createXor(Result, NotOver);
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          assuming(LLVM::Core::AArch64NeonSQRDMulH != LLVM::Core::NotIntrinsic);
          return Builder.createBinaryIntrinsic(LLVM::Core::AArch64NeonSQRDMulH,
                                               LHS, RHS);
        }
#endif

        // Fallback case.
        // If the SSSE3 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto ExtTy = Context.Int16x8Ty.getExtendedElementVectorType();
        auto Offset =
            Builder.createVectorSplat(8, LLContext.getInt32(UINT32_C(0x4000)));
        auto Shift =
            Builder.createVectorSplat(8, LLContext.getInt32(UINT32_C(15)));
        auto ExtLHS = Builder.createSExt(LHS, ExtTy);
        auto ExtRHS = Builder.createSExt(RHS, ExtTy);
        auto Result = Builder.createTrunc(
            Builder.createAShr(
                Builder.createAdd(Builder.createMul(ExtLHS, ExtRHS), Offset),
                Shift),
            Context.Int16x8Ty);
        auto IntMaxV =
            Builder.createVectorSplat(8, LLContext.getInt16(UINT16_C(0x8000)));
        auto NotOver = Builder.createSExt(Builder.createICmpEQ(Result, IntMaxV),
                                          Context.Int16x8Ty);
        return Builder.createXor(Result, NotOver);
      });
}

void FunctionCompiler::compileVectorVectorSMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::SMax, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorSMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::SMin, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorSub(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createSub(LHS, RHS);
  });
}

void FunctionCompiler::compileVectorVectorSubSat(LLVM::Type VectorTy,
                                                 bool Signed) noexcept {
  auto ID = Signed ? LLVM::Core::SSubSat : LLVM::Core::USubSat;
  assuming(ID != LLVM::Core::NotIntrinsic);
  compileVectorVectorOp(
      VectorTy, [this, VectorTy, ID](auto LHS, auto RHS) noexcept {
        return Builder.createIntrinsic(ID, {VectorTy}, {LHS, RHS});
      });
}

void FunctionCompiler::compileVectorVectorUAvgr(LLVM::Type VectorTy) noexcept {
  auto ExtendTy = VectorTy.getExtendedElementVectorType();
  compileVectorVectorOp(
      VectorTy,
      [this, VectorTy, ExtendTy](auto LHS, auto RHS) noexcept -> LLVM::Value {
#if defined(__x86_64__)
        if (Context.SupportSSE2) {
          const auto ID = [VectorTy]() noexcept {
            switch (VectorTy.getElementType().getIntegerBitWidth()) {
            case 8:
              return LLVM::Core::X86SSE2PAvgB;
            case 16:
              return LLVM::Core::X86SSE2PAvgW;
            default:
              assumingUnreachable();
            }
          }();
          assuming(ID != LLVM::Core::NotIntrinsic);
          return Builder.createIntrinsic(ID, {}, {LHS, RHS});
        }
#endif

#if defined(__aarch64__)
        if (Context.SupportNEON) {
          assuming(LLVM::Core::AArch64NeonURHAdd != LLVM::Core::NotIntrinsic);
          return Builder.createBinaryIntrinsic(LLVM::Core::AArch64NeonURHAdd,
                                               LHS, RHS);
        }
#endif

        // Fallback case.
        // If the SSE2 is not supported on the x86_64 platform or
        // the NEON is not supported on the aarch64 platform,
        // then fallback to this.
        auto EL = Builder.createZExt(LHS, ExtendTy);
        auto ER = Builder.createZExt(RHS, ExtendTy);
        auto One = Builder.createZExt(
            Builder.createVectorSplat(ExtendTy.getVectorSize(),
                                      LLContext.getTrue()),
            ExtendTy);
        return Builder.createTrunc(
            Builder.createLShr(
                Builder.createAdd(Builder.createAdd(EL, ER), One), One),
            VectorTy);
      });
}

void FunctionCompiler::compileVectorVectorUMax(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::UMax, {LHS.getType()},
                                   {LHS, RHS});
  });
}

void FunctionCompiler::compileVectorVectorUMin(LLVM::Type VectorTy) noexcept {
  compileVectorVectorOp(VectorTy, [this](auto LHS, auto RHS) noexcept {
    return Builder.createIntrinsic(LLVM::Core::UMin, {LHS.getType()},
                                   {LHS, RHS});
  });
}

} // namespace WasmEdge
