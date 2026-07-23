// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "compiler/function_compiler.h"

#include <limits>

namespace WasmEdge {

Expect<void>
FunctionCompiler::compileNumericOp(const AST::Instruction &Instr) noexcept {
  switch (Instr.getOpCode()) {
  case OpCode::I32__eqz:
    stackPush(Builder.createZExt(
        Builder.createICmpEQ(stackPop(), LLContext.getInt32(0)),
        Context.Int32Ty));
    break;
  case OpCode::I64__eqz:
    stackPush(Builder.createZExt(
        Builder.createICmpEQ(stackPop(), LLContext.getInt64(0)),
        Context.Int32Ty));
    break;
  case OpCode::I32__clz:
    assuming(LLVM::Core::Ctlz != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::Ctlz, {Context.Int32Ty},
                                      {stackPop(), LLContext.getFalse()}));
    break;
  case OpCode::I64__clz:
    assuming(LLVM::Core::Ctlz != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::Ctlz, {Context.Int64Ty},
                                      {stackPop(), LLContext.getFalse()}));
    break;
  case OpCode::I32__ctz:
    assuming(LLVM::Core::Cttz != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::Cttz, {Context.Int32Ty},
                                      {stackPop(), LLContext.getFalse()}));
    break;
  case OpCode::I64__ctz:
    assuming(LLVM::Core::Cttz != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::Cttz, {Context.Int64Ty},
                                      {stackPop(), LLContext.getFalse()}));
    break;
  case OpCode::I32__popcnt:
  case OpCode::I64__popcnt:
    assuming(LLVM::Core::Ctpop != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Ctpop, stackPop()));
    break;
  case OpCode::F32__abs:
  case OpCode::F64__abs:
    assuming(LLVM::Core::Fabs != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Fabs, stackPop()));
    break;
  case OpCode::F32__neg:
  case OpCode::F64__neg:
    stackPush(Builder.createFNeg(stackPop()));
    break;
  case OpCode::F32__ceil:
  case OpCode::F64__ceil:
    assuming(LLVM::Core::Ceil != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Ceil, stackPop()));
    break;
  case OpCode::F32__floor:
  case OpCode::F64__floor:
    assuming(LLVM::Core::Floor != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Floor, stackPop()));
    break;
  case OpCode::F32__trunc:
  case OpCode::F64__trunc:
    assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Trunc, stackPop()));
    break;
  case OpCode::F32__nearest:
  case OpCode::F64__nearest: {
    const bool IsFloat = Instr.getOpCode() == OpCode::F32__nearest;
    LLVM::Value Value = stackPop();

#if LLVM_VERSION_MAJOR >= 12 && !defined(__s390x__)
    assuming(LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic);
    if (LLVM::Core::Roundeven != LLVM::Core::NotIntrinsic) {
      stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Roundeven, Value));
      break;
    }
#endif

    // The VectorSize is only used when SSE4_1 or NEON is supported.
    [[maybe_unused]] const uint32_t VectorSize = IsFloat ? 4 : 2;
#if defined(__x86_64__)
    if (Context.SupportSSE4_1) {
      auto Zero = LLContext.getInt64(0);
      auto VectorTy = LLVM::Type::getVectorType(Value.getType(), VectorSize);
      LLVM::Value Ret = LLVM::Value::getUndef(VectorTy);
      Ret = Builder.createInsertElement(Ret, Value, Zero);
      auto ID =
          IsFloat ? LLVM::Core::X86SSE41RoundSs : LLVM::Core::X86SSE41RoundSd;
      assuming(ID != LLVM::Core::NotIntrinsic);
      Ret = Builder.createIntrinsic(ID, {}, {Ret, Ret, LLContext.getInt32(8)});
      Ret = Builder.createExtractElement(Ret, Zero);
      stackPush(Ret);
      break;
    }
#endif

#if defined(__aarch64__)
    if (Context.SupportNEON &&
        LLVM::Core::AArch64NeonFRIntN != LLVM::Core::NotIntrinsic) {
      auto Zero = LLContext.getInt64(0);
      auto VectorTy = LLVM::Type::getVectorType(Value.getType(), VectorSize);
      LLVM::Value Ret = LLVM::Value::getUndef(VectorTy);
      Ret = Builder.createInsertElement(Ret, Value, Zero);
      Ret = Builder.createUnaryIntrinsic(LLVM::Core::AArch64NeonFRIntN, Ret);
      Ret = Builder.createExtractElement(Ret, Zero);
      stackPush(Ret);
      break;
    }
#endif

    // Fallback case.
    // If the SSE4.1 is not supported on the x86_64 platform or
    // the NEON is not supported on the aarch64 platform,
    // then fallback to this.
    assuming(LLVM::Core::Nearbyint != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Nearbyint, Value));
    break;
  }
  case OpCode::F32__sqrt:
  case OpCode::F64__sqrt:
    assuming(LLVM::Core::Sqrt != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createUnaryIntrinsic(LLVM::Core::Sqrt, stackPop()));
    break;
  case OpCode::I32__wrap_i64:
    stackPush(Builder.createTrunc(stackPop(), Context.Int32Ty));
    break;
  case OpCode::I32__trunc_f32_s:
    compileSignedTrunc(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_f64_s:
    compileSignedTrunc(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_f32_u:
    compileUnsignedTrunc(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_f64_u:
    compileUnsignedTrunc(Context.Int32Ty);
    break;
  case OpCode::I64__extend_i32_s:
    stackPush(Builder.createSExt(stackPop(), Context.Int64Ty));
    break;
  case OpCode::I64__extend_i32_u:
    stackPush(Builder.createZExt(stackPop(), Context.Int64Ty));
    break;
  case OpCode::I64__trunc_f32_s:
    compileSignedTrunc(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_f64_s:
    compileSignedTrunc(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_f32_u:
    compileUnsignedTrunc(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_f64_u:
    compileUnsignedTrunc(Context.Int64Ty);
    break;
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i64_s:
    stackPush(Builder.createSIToFP(stackPop(), Context.FloatTy));
    break;
  case OpCode::F32__convert_i32_u:
  case OpCode::F32__convert_i64_u:
    stackPush(Builder.createUIToFP(stackPop(), Context.FloatTy));
    break;
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i64_s:
    stackPush(Builder.createSIToFP(stackPop(), Context.DoubleTy));
    break;
  case OpCode::F64__convert_i32_u:
  case OpCode::F64__convert_i64_u:
    stackPush(Builder.createUIToFP(stackPop(), Context.DoubleTy));
    break;
  case OpCode::F32__demote_f64:
    stackPush(Builder.createFPTrunc(stackPop(), Context.FloatTy));
    break;
  case OpCode::F64__promote_f32:
    stackPush(Builder.createFPExt(stackPop(), Context.DoubleTy));
    break;
  case OpCode::I32__reinterpret_f32:
    stackPush(Builder.createBitCast(stackPop(), Context.Int32Ty));
    break;
  case OpCode::I64__reinterpret_f64:
    stackPush(Builder.createBitCast(stackPop(), Context.Int64Ty));
    break;
  case OpCode::F32__reinterpret_i32:
    stackPush(Builder.createBitCast(stackPop(), Context.FloatTy));
    break;
  case OpCode::F64__reinterpret_i64:
    stackPush(Builder.createBitCast(stackPop(), Context.DoubleTy));
    break;
  case OpCode::I32__extend8_s:
    stackPush(Builder.createSExt(
        Builder.createTrunc(stackPop(), Context.Int8Ty), Context.Int32Ty));
    break;
  case OpCode::I32__extend16_s:
    stackPush(Builder.createSExt(
        Builder.createTrunc(stackPop(), Context.Int16Ty), Context.Int32Ty));
    break;
  case OpCode::I64__extend8_s:
    stackPush(Builder.createSExt(
        Builder.createTrunc(stackPop(), Context.Int8Ty), Context.Int64Ty));
    break;
  case OpCode::I64__extend16_s:
    stackPush(Builder.createSExt(
        Builder.createTrunc(stackPop(), Context.Int16Ty), Context.Int64Ty));
    break;
  case OpCode::I64__extend32_s:
    stackPush(Builder.createSExt(
        Builder.createTrunc(stackPop(), Context.Int32Ty), Context.Int64Ty));
    break;

  // Binary Numeric Instructions
  case OpCode::I32__eq:
  case OpCode::I64__eq: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpEQ(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__ne:
  case OpCode::I64__ne: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpNE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__lt_s:
  case OpCode::I64__lt_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpSLT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__lt_u:
  case OpCode::I64__lt_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpULT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__gt_s:
  case OpCode::I64__gt_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpSGT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__gt_u:
  case OpCode::I64__gt_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpUGT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__le_s:
  case OpCode::I64__le_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpSLE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__le_u:
  case OpCode::I64__le_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpULE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__ge_s:
  case OpCode::I64__ge_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpSGE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__ge_u:
  case OpCode::I64__ge_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createICmpUGE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__eq:
  case OpCode::F64__eq: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpOEQ(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__ne:
  case OpCode::F64__ne: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpUNE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__lt:
  case OpCode::F64__lt: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpOLT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__gt:
  case OpCode::F64__gt: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpOGT(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__le:
  case OpCode::F64__le: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpOLE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::F32__ge:
  case OpCode::F64__ge: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(
        Builder.createZExt(Builder.createFCmpOGE(LHS, RHS), Context.Int32Ty));
    break;
  }
  case OpCode::I32__add:
  case OpCode::I64__add: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createAdd(LHS, RHS));
    break;
  }
  // Wide Arithmetic Instructions
  case OpCode::I64__mul_wide_u:
  case OpCode::I64__mul_wide_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();

    LLVM::Type Int128Ty = Context.Int128Ty;
    LLVM::Value Shift64 = LLVM::Value::getConstInt(Int128Ty, 64, false);

    // Sign-extend for _s, Zero-extend for _u
    const bool IsSigned = Instr.getOpCode() == OpCode::I64__mul_wide_s;
    LLVM::Value ExtLHS = IsSigned ? Builder.createSExt(LHS, Int128Ty) : Builder.createZExt(LHS, Int128Ty);
    LLVM::Value ExtRHS = IsSigned ? Builder.createSExt(RHS, Int128Ty) : Builder.createZExt(RHS, Int128Ty);

    LLVM::Value Res128 = Builder.createMul(ExtLHS, ExtRHS);
    
    LLVM::Value Low64 = Builder.createTrunc(Res128, Context.Int64Ty);
    LLVM::Value High64 = Builder.createTrunc(Builder.createLShr(Res128, Shift64), Context.Int64Ty);

    // Spec: push lower 64 bits first, then upper 64 bits
    stackPush(Low64);
    stackPush(High64);
    break;
  }
  case OpCode::I64__add128:
  case OpCode::I64__sub128: {
    // Stack pops in reverse order: [high2, low2, high1, low1]
    LLVM::Value High2 = stackPop();
    LLVM::Value Low2 = stackPop();
    LLVM::Value High1 = stackPop();
    LLVM::Value Low1 = stackPop();

    LLVM::Type Int128Ty = Context.Int128Ty;
    LLVM::Value Shift64 = LLVM::Value::getConstInt(Int128Ty, 64, false);

    // Assemble LHS 128-bit value
    LLVM::Value ExtLow1 = Builder.createZExt(Low1, Int128Ty);
    LLVM::Value ExtHigh1 = Builder.createZExt(High1, Int128Ty);
    LLVM::Value Val1 = Builder.createOr(Builder.createShl(ExtHigh1, Shift64), ExtLow1);

    // Assemble RHS 128-bit value
    LLVM::Value ExtLow2 = Builder.createZExt(Low2, Int128Ty);
    LLVM::Value ExtHigh2 = Builder.createZExt(High2, Int128Ty);
    LLVM::Value Val2 = Builder.createOr(Builder.createShl(ExtHigh2, Shift64), ExtLow2);

    const bool IsAdd = Instr.getOpCode() == OpCode::I64__add128;
    LLVM::Value Res128 = IsAdd ? Builder.createAdd(Val1, Val2) : Builder.createSub(Val1, Val2);

    LLVM::Value ResLow = Builder.createTrunc(Res128, Context.Int64Ty);
    LLVM::Value ResHigh = Builder.createTrunc(Builder.createLShr(Res128, Shift64), Context.Int64Ty);

    // Spec: push lower 64 bits first, then upper 64 bits
    stackPush(ResLow);
    stackPush(ResHigh);
    break;
  }
  case OpCode::I32__sub:
  case OpCode::I64__sub: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();

    stackPush(Builder.createSub(LHS, RHS));
    break;
  }
  case OpCode::I32__mul:
  case OpCode::I64__mul: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createMul(LHS, RHS));
    break;
  }
  case OpCode::I32__div_s:
  case OpCode::I64__div_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    if constexpr (LLVM::kForceDivCheck) {
      const bool Is32 = Instr.getOpCode() == OpCode::I32__div_s;
      LLVM::Value IntZero =
          Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);
      LLVM::Value IntMinusOne =
          Is32 ? LLContext.getInt32(static_cast<uint32_t>(INT32_C(-1)))
               : LLContext.getInt64(static_cast<uint64_t>(INT64_C(-1)));
      LLVM::Value IntMin = Is32 ? LLContext.getInt32(static_cast<uint32_t>(
                                      std::numeric_limits<int32_t>::min()))
                                : LLContext.getInt64(static_cast<uint64_t>(
                                      std::numeric_limits<int64_t>::min()));

      auto NoZeroBB = LLVM::BasicBlock::create(LLContext, F.Fn, "div.nozero");
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "div.ok");

      auto IsNotZero = Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
      Builder.createCondBr(IsNotZero, NoZeroBB,
                           getTrapBB(ErrCode::Value::DivideByZero));

      Builder.positionAtEnd(NoZeroBB);
      auto NotOverflow = Builder.createLikely(
          Builder.createOr(Builder.createICmpNE(LHS, IntMin),
                           Builder.createICmpNE(RHS, IntMinusOne)));
      Builder.createCondBr(NotOverflow, OkBB,
                           getTrapBB(ErrCode::Value::IntegerOverflow));

      Builder.positionAtEnd(OkBB);
    }
    stackPush(Builder.createSDiv(LHS, RHS));
    break;
  }
  case OpCode::I32__div_u:
  case OpCode::I64__div_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    if constexpr (LLVM::kForceDivCheck) {
      const bool Is32 = Instr.getOpCode() == OpCode::I32__div_u;
      LLVM::Value IntZero =
          Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "div.ok");

      auto IsNotZero = Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
      Builder.createCondBr(IsNotZero, OkBB,
                           getTrapBB(ErrCode::Value::DivideByZero));
      Builder.positionAtEnd(OkBB);
    }
    stackPush(Builder.createUDiv(LHS, RHS));
    break;
  }
  case OpCode::I32__rem_s:
  case OpCode::I64__rem_s: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    // handle INT32_MIN % -1
    const bool Is32 = Instr.getOpCode() == OpCode::I32__rem_s;
    LLVM::Value IntMinusOne =
        Is32 ? LLContext.getInt32(static_cast<uint32_t>(INT32_C(-1)))
             : LLContext.getInt64(static_cast<uint64_t>(INT64_C(-1)));
    LLVM::Value IntMin =
        Is32 ? LLContext.getInt32(
                   static_cast<uint32_t>(std::numeric_limits<int32_t>::min()))
             : LLContext.getInt64(
                   static_cast<uint64_t>(std::numeric_limits<int64_t>::min()));
    LLVM::Value IntZero = Is32 ? LLContext.getInt32(0) : LLContext.getInt64(0);

    auto NoOverflowBB =
        LLVM::BasicBlock::create(LLContext, F.Fn, "no.overflow");
    auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "end.overflow");

    if constexpr (LLVM::kForceDivCheck) {
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rem.ok");

      auto IsNotZero = Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
      Builder.createCondBr(IsNotZero, OkBB,
                           getTrapBB(ErrCode::Value::DivideByZero));
      Builder.positionAtEnd(OkBB);
    }

    auto CurrBB = Builder.getInsertBlock();

    auto NotOverflow = Builder.createLikely(
        Builder.createOr(Builder.createICmpNE(LHS, IntMin),
                         Builder.createICmpNE(RHS, IntMinusOne)));
    Builder.createCondBr(NotOverflow, NoOverflowBB, EndBB);

    Builder.positionAtEnd(NoOverflowBB);
    auto Ret1 = Builder.createSRem(LHS, RHS);
    Builder.createBr(EndBB);

    Builder.positionAtEnd(EndBB);
    auto Ret = Builder.createPHI(Ret1.getType());
    Ret.addIncoming(Ret1, NoOverflowBB);
    Ret.addIncoming(IntZero, CurrBB);

    stackPush(Ret);
    break;
  }
  case OpCode::I32__rem_u:
  case OpCode::I64__rem_u: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    if constexpr (LLVM::kForceDivCheck) {
      LLVM::Value IntZero = Instr.getOpCode() == OpCode::I32__rem_u
                                ? LLContext.getInt32(0)
                                : LLContext.getInt64(0);
      auto OkBB = LLVM::BasicBlock::create(LLContext, F.Fn, "rem.ok");

      auto IsNotZero = Builder.createLikely(Builder.createICmpNE(RHS, IntZero));
      Builder.createCondBr(IsNotZero, OkBB,
                           getTrapBB(ErrCode::Value::DivideByZero));
      Builder.positionAtEnd(OkBB);
    }
    stackPush(Builder.createURem(LHS, RHS));
    break;
  }
  case OpCode::I32__and:
  case OpCode::I64__and: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createAnd(LHS, RHS));
    break;
  }
  case OpCode::I32__or:
  case OpCode::I64__or: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createOr(LHS, RHS));
    break;
  }
  case OpCode::I32__xor:
  case OpCode::I64__xor: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createXor(LHS, RHS));
    break;
  }
  case OpCode::I32__shl:
  case OpCode::I64__shl: {
    LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shl
                           ? LLContext.getInt32(31)
                           : LLContext.getInt64(63);
    LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createShl(LHS, RHS));
    break;
  }
  case OpCode::I32__shr_s:
  case OpCode::I64__shr_s: {
    LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shr_s
                           ? LLContext.getInt32(31)
                           : LLContext.getInt64(63);
    LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createAShr(LHS, RHS));
    break;
  }
  case OpCode::I32__shr_u:
  case OpCode::I64__shr_u: {
    LLVM::Value Mask = Instr.getOpCode() == OpCode::I32__shr_u
                           ? LLContext.getInt32(31)
                           : LLContext.getInt64(63);
    LLVM::Value RHS = Builder.createAnd(stackPop(), Mask);
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createLShr(LHS, RHS));
    break;
  }
  case OpCode::I32__rotl: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    assuming(LLVM::Core::FShl != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::FShl, {Context.Int32Ty},
                                      {LHS, LHS, RHS}));
    break;
  }
  case OpCode::I32__rotr: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    assuming(LLVM::Core::FShr != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::FShr, {Context.Int32Ty},
                                      {LHS, LHS, RHS}));
    break;
  }
  case OpCode::I64__rotl: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    assuming(LLVM::Core::FShl != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::FShl, {Context.Int64Ty},
                                      {LHS, LHS, RHS}));
    break;
  }
  case OpCode::I64__rotr: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    assuming(LLVM::Core::FShr != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::FShr, {Context.Int64Ty},
                                      {LHS, LHS, RHS}));
    break;
  }
  case OpCode::F32__add:
  case OpCode::F64__add: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createFAdd(LHS, RHS));
    break;
  }
  case OpCode::F32__sub:
  case OpCode::F64__sub: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createFSub(LHS, RHS));
    break;
  }
  case OpCode::F32__mul:
  case OpCode::F64__mul: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createFMul(LHS, RHS));
    break;
  }
  case OpCode::F32__div:
  case OpCode::F64__div: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    stackPush(Builder.createFDiv(LHS, RHS));
    break;
  }
  case OpCode::F32__min:
  case OpCode::F64__min: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    auto FpTy = Instr.getOpCode() == OpCode::F32__min ? Context.FloatTy
                                                      : Context.DoubleTy;
    auto IntTy = Instr.getOpCode() == OpCode::F32__min ? Context.Int32Ty
                                                       : Context.Int64Ty;

    auto UEQ = Builder.createFCmpUEQ(LHS, RHS);
    auto UNO = Builder.createFCmpUNO(LHS, RHS);

    auto LHSInt = Builder.createBitCast(LHS, IntTy);
    auto RHSInt = Builder.createBitCast(RHS, IntTy);
    auto OrInt = Builder.createOr(LHSInt, RHSInt);
    auto OrFp = Builder.createBitCast(OrInt, FpTy);

    auto AddFp = Builder.createFAdd(LHS, RHS);

    assuming(LLVM::Core::MinNum != LLVM::Core::NotIntrinsic);
    auto MinFp = Builder.createIntrinsic(LLVM::Core::MinNum, {LHS.getType()},
                                         {LHS, RHS});

    auto Ret = Builder.createSelect(UEQ, Builder.createSelect(UNO, AddFp, OrFp),
                                    MinFp);
    stackPush(Ret);
    break;
  }
  case OpCode::F32__max:
  case OpCode::F64__max: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    auto FpTy = Instr.getOpCode() == OpCode::F32__max ? Context.FloatTy
                                                      : Context.DoubleTy;
    auto IntTy = Instr.getOpCode() == OpCode::F32__max ? Context.Int32Ty
                                                       : Context.Int64Ty;

    auto UEQ = Builder.createFCmpUEQ(LHS, RHS);
    auto UNO = Builder.createFCmpUNO(LHS, RHS);

    auto LHSInt = Builder.createBitCast(LHS, IntTy);
    auto RHSInt = Builder.createBitCast(RHS, IntTy);
    auto AndInt = Builder.createAnd(LHSInt, RHSInt);
    auto AndFp = Builder.createBitCast(AndInt, FpTy);

    auto AddFp = Builder.createFAdd(LHS, RHS);

    assuming(LLVM::Core::MaxNum != LLVM::Core::NotIntrinsic);
    auto MaxFp = Builder.createIntrinsic(LLVM::Core::MaxNum, {LHS.getType()},
                                         {LHS, RHS});

    auto Ret = Builder.createSelect(
        UEQ, Builder.createSelect(UNO, AddFp, AndFp), MaxFp);
    stackPush(Ret);
    break;
  }
  case OpCode::F32__copysign:
  case OpCode::F64__copysign: {
    LLVM::Value RHS = stackPop();
    LLVM::Value LHS = stackPop();
    assuming(LLVM::Core::CopySign != LLVM::Core::NotIntrinsic);
    stackPush(Builder.createIntrinsic(LLVM::Core::CopySign, {LHS.getType()},
                                      {LHS, RHS}));
    break;
  }

  // Saturating Truncation Numeric Instructions
  case OpCode::I32__trunc_sat_f32_s:
    compileSignedTruncSat(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_sat_f32_u:
    compileUnsignedTruncSat(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_sat_f64_s:
    compileSignedTruncSat(Context.Int32Ty);
    break;
  case OpCode::I32__trunc_sat_f64_u:
    compileUnsignedTruncSat(Context.Int32Ty);
    break;
  case OpCode::I64__trunc_sat_f32_s:
    compileSignedTruncSat(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_sat_f32_u:
    compileUnsignedTruncSat(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_sat_f64_s:
    compileSignedTruncSat(Context.Int64Ty);
    break;
  case OpCode::I64__trunc_sat_f64_u:
    compileUnsignedTruncSat(Context.Int64Ty);
    break;

    // SIMD Memory Instructions
  default:
    assumingUnreachable();
  }
  return {};
}

void FunctionCompiler::compileSignedTrunc(LLVM::Type IntType) noexcept {
  auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.norm");
  auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.notmin");
  auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "strunc.notmax");
  auto Value = stackPop();
  const auto [Precise, MinFp, MaxFp] =
      [IntType, Value]() -> std::tuple<bool, LLVM::Value, LLVM::Value> {
    const auto BitWidth = IntType.getIntegerBitWidth();
    const auto [Min, Max] = [BitWidth]() -> std::tuple<int64_t, int64_t> {
      switch (BitWidth) {
      case 32:
        return {std::numeric_limits<int32_t>::min(),
                std::numeric_limits<int32_t>::max()};
      case 64:
        return {std::numeric_limits<int64_t>::min(),
                std::numeric_limits<int64_t>::max()};
      default:
        assumingUnreachable();
      }
    }();
    auto FPType = Value.getType();
    assuming(FPType.isFloatTy() || FPType.isDoubleTy());
    const auto FPWidth = FPType.getFPMantissaWidth();
    return {BitWidth <= FPWidth, LLVM::Value::getConstReal(FPType, Min),
            LLVM::Value::getConstReal(FPType, Max)};
  }();

  auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
  Builder.createCondBr(IsNotNan, NormBB,
                       getTrapBB(ErrCode::Value::InvalidConvToInt));

  Builder.positionAtEnd(NormBB);
  assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
  auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
  auto IsNotUnderflow =
      Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
  Builder.createCondBr(IsNotUnderflow, NotMinBB,
                       getTrapBB(ErrCode::Value::IntegerOverflow));

  Builder.positionAtEnd(NotMinBB);
  auto IsNotOverflow = Builder.createLikely(
      Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
  Builder.createCondBr(IsNotOverflow, NotMaxBB,
                       getTrapBB(ErrCode::Value::IntegerOverflow));

  Builder.positionAtEnd(NotMaxBB);
  stackPush(Builder.createFPToSI(Trunc, IntType));
}

void FunctionCompiler::compileSignedTruncSat(LLVM::Type IntType) noexcept {
  auto CurrBB = Builder.getInsertBlock();
  auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.norm");
  auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.notmin");
  auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.notmax");
  auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "ssat.end");
  auto Value = stackPop();
  const auto [Precise, MinInt, MaxInt, MinFp, MaxFp] = [IntType, Value]()
      -> std::tuple<bool, uint64_t, uint64_t, LLVM::Value, LLVM::Value> {
    const auto BitWidth = IntType.getIntegerBitWidth();
    const auto [Min, Max] = [BitWidth]() -> std::tuple<int64_t, int64_t> {
      switch (BitWidth) {
      case 32:
        return {std::numeric_limits<int32_t>::min(),
                std::numeric_limits<int32_t>::max()};
      case 64:
        return {std::numeric_limits<int64_t>::min(),
                std::numeric_limits<int64_t>::max()};
      default:
        assumingUnreachable();
      }
    }();
    auto FPType = Value.getType();
    assuming(FPType.isFloatTy() || FPType.isDoubleTy());
    const auto FPWidth = FPType.getFPMantissaWidth();
    return {BitWidth <= FPWidth, static_cast<uint64_t>(Min),
            static_cast<uint64_t>(Max), LLVM::Value::getConstReal(FPType, Min),
            LLVM::Value::getConstReal(FPType, Max)};
  }();

  auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
  Builder.createCondBr(IsNotNan, NormBB, EndBB);

  Builder.positionAtEnd(NormBB);
  assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
  auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
  auto IsNotUnderflow =
      Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
  Builder.createCondBr(IsNotUnderflow, NotMinBB, EndBB);

  Builder.positionAtEnd(NotMinBB);
  auto IsNotOverflow = Builder.createLikely(
      Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
  Builder.createCondBr(IsNotOverflow, NotMaxBB, EndBB);

  Builder.positionAtEnd(NotMaxBB);
  auto IntValue = Builder.createFPToSI(Trunc, IntType);
  Builder.createBr(EndBB);

  Builder.positionAtEnd(EndBB);
  auto PHIRet = Builder.createPHI(IntType);
  PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, 0, true), CurrBB);
  PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MinInt, true), NormBB);
  PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MaxInt, true), NotMinBB);
  PHIRet.addIncoming(IntValue, NotMaxBB);

  stackPush(PHIRet);
}

void FunctionCompiler::compileUnsignedTrunc(LLVM::Type IntType) noexcept {
  auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.norm");
  auto NotMinBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.notmin");
  auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "utrunc.notmax");
  auto Value = stackPop();
  const auto [Precise, MinFp, MaxFp] =
      [IntType, Value]() -> std::tuple<bool, LLVM::Value, LLVM::Value> {
    const auto BitWidth = IntType.getIntegerBitWidth();
    const auto [Min, Max] = [BitWidth]() -> std::tuple<uint64_t, uint64_t> {
      switch (BitWidth) {
      case 32:
        return {std::numeric_limits<uint32_t>::min(),
                std::numeric_limits<uint32_t>::max()};
      case 64:
        return {std::numeric_limits<uint64_t>::min(),
                std::numeric_limits<uint64_t>::max()};
      default:
        assumingUnreachable();
      }
    }();
    auto FPType = Value.getType();
    assuming(FPType.isFloatTy() || FPType.isDoubleTy());
    const auto FPWidth = FPType.getFPMantissaWidth();
    return {BitWidth <= FPWidth, LLVM::Value::getConstReal(FPType, Min),
            LLVM::Value::getConstReal(FPType, Max)};
  }();

  auto IsNotNan = Builder.createLikely(Builder.createFCmpORD(Value, Value));
  Builder.createCondBr(IsNotNan, NormBB,
                       getTrapBB(ErrCode::Value::InvalidConvToInt));

  Builder.positionAtEnd(NormBB);
  assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
  auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
  auto IsNotUnderflow =
      Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
  Builder.createCondBr(IsNotUnderflow, NotMinBB,
                       getTrapBB(ErrCode::Value::IntegerOverflow));

  Builder.positionAtEnd(NotMinBB);
  auto IsNotOverflow = Builder.createLikely(
      Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
  Builder.createCondBr(IsNotOverflow, NotMaxBB,
                       getTrapBB(ErrCode::Value::IntegerOverflow));

  Builder.positionAtEnd(NotMaxBB);
  stackPush(Builder.createFPToUI(Trunc, IntType));
}

void FunctionCompiler::compileUnsignedTruncSat(LLVM::Type IntType) noexcept {
  auto CurrBB = Builder.getInsertBlock();
  auto NormBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.norm");
  auto NotMaxBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.notmax");
  auto EndBB = LLVM::BasicBlock::create(LLContext, F.Fn, "usat.end");
  auto Value = stackPop();
  const auto [Precise, MinInt, MaxInt, MinFp, MaxFp] = [IntType, Value]()
      -> std::tuple<bool, uint64_t, uint64_t, LLVM::Value, LLVM::Value> {
    const auto BitWidth = IntType.getIntegerBitWidth();
    const auto [Min, Max] = [BitWidth]() -> std::tuple<uint64_t, uint64_t> {
      switch (BitWidth) {
      case 32:
        return {std::numeric_limits<uint32_t>::min(),
                std::numeric_limits<uint32_t>::max()};
      case 64:
        return {std::numeric_limits<uint64_t>::min(),
                std::numeric_limits<uint64_t>::max()};
      default:
        assumingUnreachable();
      }
    }();
    auto FPType = Value.getType();
    assuming(FPType.isFloatTy() || FPType.isDoubleTy());
    const auto FPWidth = FPType.getFPMantissaWidth();
    return {BitWidth <= FPWidth, Min, Max,
            LLVM::Value::getConstReal(FPType, Min),
            LLVM::Value::getConstReal(FPType, Max)};
  }();

  assuming(LLVM::Core::Trunc != LLVM::Core::NotIntrinsic);
  auto Trunc = Builder.createUnaryIntrinsic(LLVM::Core::Trunc, Value);
  auto IsNotUnderflow =
      Builder.createLikely(Builder.createFCmpOGE(Trunc, MinFp));
  Builder.createCondBr(IsNotUnderflow, NormBB, EndBB);

  Builder.positionAtEnd(NormBB);
  auto IsNotOverflow = Builder.createLikely(
      Builder.createFCmp(Precise ? LLVMRealOLE : LLVMRealOLT, Trunc, MaxFp));
  Builder.createCondBr(IsNotOverflow, NotMaxBB, EndBB);

  Builder.positionAtEnd(NotMaxBB);
  auto IntValue = Builder.createFPToUI(Trunc, IntType);
  Builder.createBr(EndBB);

  Builder.positionAtEnd(EndBB);
  auto PHIRet = Builder.createPHI(IntType);
  PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MinInt), CurrBB);
  PHIRet.addIncoming(LLVM::Value::getConstInt(IntType, MaxInt), NormBB);
  PHIRet.addIncoming(IntValue, NotMaxBB);

  stackPush(PHIRet);
}

} // namespace WasmEdge
