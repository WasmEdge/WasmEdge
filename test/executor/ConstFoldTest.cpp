// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/instruction.h"
#include "common/enum_ast.hpp"
#include "common/types.h"
#include "executor/engine/const_fold.h"

#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

using namespace WasmEdge;

// ---------------------------------------------------------------------------
// Helpers to build instruction vectors for testing
// ---------------------------------------------------------------------------

namespace {

AST::Instruction makeConst(OpCode Code, ValVariant Val) {
  AST::Instruction I(Code, 0);
  I.setNum(Val);
  return I;
}

AST::Instruction makeI32Const(uint32_t V) {
  ValVariant Val;
  Val.emplace<uint32_t>(V);
  return makeConst(OpCode::I32__const, Val);
}

AST::Instruction makeI64Const(uint64_t V) {
  ValVariant Val;
  Val.emplace<uint64_t>(V);
  return makeConst(OpCode::I64__const, Val);
}

AST::Instruction makeF32Const(float V) {
  ValVariant Val;
  Val.emplace<float>(V);
  return makeConst(OpCode::F32__const, Val);
}

AST::Instruction makeF64Const(double V) {
  ValVariant Val;
  Val.emplace<double>(V);
  return makeConst(OpCode::F64__const, Val);
}

AST::Instruction makeOp(OpCode Code) { return AST::Instruction(Code, 0); }

uint32_t getI32(const AST::Instruction &I) {
  return I.getNum().get<uint32_t>();
}
uint64_t getI64(const AST::Instruction &I) {
  return I.getNum().get<uint64_t>();
}
float getF32(const AST::Instruction &I) { return I.getNum().get<float>(); }
double getF64(const AST::Instruction &I) { return I.getNum().get<double>(); }

} // namespace

// ---------------------------------------------------------------------------
// Integer binary folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, I32BinaryAdd) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(3));
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeOp(OpCode::I32__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs.size(), 3u);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 8u);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, I32BinarySub) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(10));
  Instrs.push_back(makeI32Const(7));
  Instrs.push_back(makeOp(OpCode::I32__sub));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 3u);
}

TEST(ConstFoldTest, I32BinaryMul) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(6));
  Instrs.push_back(makeI32Const(7));
  Instrs.push_back(makeOp(OpCode::I32__mul));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(getI32(Instrs[0]), 42u);
}

TEST(ConstFoldTest, I32BinaryBitwise) {
  // AND
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0xFF00));
    Instrs.push_back(makeI32Const(0x0FF0));
    Instrs.push_back(makeOp(OpCode::I32__and));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0x0F00u);
  }
  // OR
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0xFF00));
    Instrs.push_back(makeI32Const(0x00FF));
    Instrs.push_back(makeOp(OpCode::I32__or));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0xFFFFu);
  }
  // XOR
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0xAAAA));
    Instrs.push_back(makeI32Const(0x5555));
    Instrs.push_back(makeOp(OpCode::I32__xor));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0xFFFFu);
  }
}

TEST(ConstFoldTest, I32BinaryShifts) {
  // SHL
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeI32Const(4));
    Instrs.push_back(makeOp(OpCode::I32__shl));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 16u);
  }
  // SHR_U
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0x80000000));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__shr_u));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0x40000000u);
  }
  // SHR_S
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(static_cast<uint32_t>(-8)));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__shr_s));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), -4);
  }
}

TEST(ConstFoldTest, I32Rotate) {
  // ROTL
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0x80000001));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__rotl));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0x00000003u);
  }
  // ROTR
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0x80000001));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__rotr));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0xC0000000u);
  }
}

TEST(ConstFoldTest, I64BinaryAdd) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(1000000000000)));
  Instrs.push_back(makeI64Const(UINT64_C(2000000000000)));
  Instrs.push_back(makeOp(OpCode::I64__add));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(3000000000000));
}

// ---------------------------------------------------------------------------
// Division/remainder safety
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, DivByZeroNotFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(10));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__div_u));
  Executor::optimizeConstantExpressions(Instrs);
  // Must NOT be folded -- runtime must trap
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__div_u);
}

TEST(ConstFoldTest, SignedDivOverflowNotFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(
      makeI32Const(static_cast<uint32_t>(std::numeric_limits<int32_t>::min())));
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I32__div_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__div_s);
}

TEST(ConstFoldTest, SafeDivIsFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(42));
  Instrs.push_back(makeI32Const(7));
  Instrs.push_back(makeOp(OpCode::I32__div_u));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 6u);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, RemByZeroNotFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(10));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__rem_u));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__rem_u);
}

TEST(ConstFoldTest, SignedRemMinByNeg1) {
  // Wasm spec: i32.rem_s(INT_MIN, -1) = 0 (no trap, result is 0)
  AST::InstrVec Instrs;
  Instrs.push_back(
      makeI32Const(static_cast<uint32_t>(std::numeric_limits<int32_t>::min())));
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I32__rem_s));
  Executor::optimizeConstantExpressions(Instrs);
  // rem_s does NOT trap for this case, divisor is non-zero so it can fold
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

// ---------------------------------------------------------------------------
// Float binary folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, F32BinaryAdd) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(1.5f));
  Instrs.push_back(makeF32Const(2.5f));
  Instrs.push_back(makeOp(OpCode::F32__add));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_FLOAT_EQ(getF32(Instrs[0]), 4.0f);
}

TEST(ConstFoldTest, F32MinNaN) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeF32Const(1.0f));
  Instrs.push_back(makeOp(OpCode::F32__min));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_TRUE(std::isnan(getF32(Instrs[0])));
  // Verify canonical NaN bit pattern
  uint32_t Bits;
  float R = getF32(Instrs[0]);
  std::memcpy(&Bits, &R, sizeof(Bits));
  EXPECT_EQ(Bits, UINT32_C(0x7FC00000));
}

TEST(ConstFoldTest, F32MinSignedZero) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(0.0f));
  Instrs.push_back(makeF32Const(-0.0f));
  Instrs.push_back(makeOp(OpCode::F32__min));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  float R = getF32(Instrs[0]);
  EXPECT_EQ(R, 0.0f);
  EXPECT_TRUE(std::signbit(R)); // min(+0, -0) = -0
}

TEST(ConstFoldTest, F64MaxSignedZero) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__max));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  double R = getF64(Instrs[0]);
  EXPECT_EQ(R, 0.0);
  EXPECT_FALSE(std::signbit(R)); // max(-0, +0) = +0
}

TEST(ConstFoldTest, F64DivByZero) {
  // Float div by zero is well-defined (produces Inf), so it CAN be folded.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__div));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isinf(getF64(Instrs[0])));
}

// ---------------------------------------------------------------------------
// Unary folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, I32Eqz) {
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0));
    Instrs.push_back(makeOp(OpCode::I32__eqz));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
    EXPECT_EQ(getI32(Instrs[0]), 1u);
  }
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(42));
    Instrs.push_back(makeOp(OpCode::I32__eqz));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0u);
  }
}

TEST(ConstFoldTest, I32Clz) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeOp(OpCode::I32__clz));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(getI32(Instrs[0]), 31u);
}

TEST(ConstFoldTest, I32Ctz) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x80000000));
  Instrs.push_back(makeOp(OpCode::I32__ctz));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(getI32(Instrs[0]), 31u);
}

TEST(ConstFoldTest, I32Popcnt) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0xFF));
  Instrs.push_back(makeOp(OpCode::I32__popcnt));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(getI32(Instrs[0]), 8u);
}

TEST(ConstFoldTest, F64Sqrt) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(25.0));
  Instrs.push_back(makeOp(OpCode::F64__sqrt));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_DOUBLE_EQ(getF64(Instrs[0]), 5.0);
}

TEST(ConstFoldTest, F32Neg) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(3.14f));
  Instrs.push_back(makeOp(OpCode::F32__neg));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_FLOAT_EQ(getF32(Instrs[0]), -3.14f);
}

// ---------------------------------------------------------------------------
// Relational folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, I32Eq) {
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(5));
    Instrs.push_back(makeI32Const(5));
    Instrs.push_back(makeOp(OpCode::I32__eq));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
    EXPECT_EQ(getI32(Instrs[0]), 1u);
  }
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(5));
    Instrs.push_back(makeI32Const(6));
    Instrs.push_back(makeOp(OpCode::I32__eq));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getI32(Instrs[0]), 0u);
  }
}

TEST(ConstFoldTest, F64NaNComparison) {
  AST::InstrVec Instrs;
  double Nan = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeF64Const(Nan));
  Instrs.push_back(makeF64Const(Nan));
  Instrs.push_back(makeOp(OpCode::F64__eq));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u); // NaN != NaN
}

// ---------------------------------------------------------------------------
// Type conversion folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, I32WrapI64) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0x1FFFFFFFF)));
  Instrs.push_back(makeOp(OpCode::I32__wrap_i64));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), UINT32_MAX);
}

TEST(ConstFoldTest, I64ExtendI32S) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I64__extend_i32_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(static_cast<int64_t>(getI64(Instrs[0])), -1);
}

TEST(ConstFoldTest, I64ExtendI32U) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(UINT32_MAX));
  Instrs.push_back(makeOp(OpCode::I64__extend_i32_u));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xFFFFFFFF));
}

TEST(ConstFoldTest, F64PromoteF32) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(1.5f));
  Instrs.push_back(makeOp(OpCode::F64__promote_f32));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(getF64(Instrs[0]), 1.5);
}

TEST(ConstFoldTest, Reinterpret) {
  // f32 -> i32
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF32Const(1.0f));
    Instrs.push_back(makeOp(OpCode::I32__reinterpret_f32));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
    EXPECT_EQ(getI32(Instrs[0]), UINT32_C(0x3F800000)); // IEEE 754 1.0f
  }
  // i32 -> f32
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(UINT32_C(0x3F800000)));
    Instrs.push_back(makeOp(OpCode::F32__reinterpret_i32));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
    EXPECT_FLOAT_EQ(getF32(Instrs[0]), 1.0f);
  }
}

// ---------------------------------------------------------------------------
// Truncation safety
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, TruncNaNNotFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f32_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f32_s);
}

TEST(ConstFoldTest, TruncInfNotFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::I64__trunc_f64_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I64__trunc_f64_s);
}

TEST(ConstFoldTest, TruncSafeFolded) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(42.9));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), 42);
}

TEST(ConstFoldTest, TruncSatNaN) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_sat_f32_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, TruncSatInfPositive) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_sat_f64_u));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), UINT32_MAX);
}

// ---------------------------------------------------------------------------
// Identity elimination
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, IdentityAddZero) {
  // local.get $x, i32.const 0, i32.add -> local.get $x, nop, nop
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get)); // placeholder for stack value
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::Local__get);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, IdentityMulOne) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeOp(OpCode::I32__mul));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, IdentityAndAllOnes) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeI32Const(UINT32_MAX));
  Instrs.push_back(makeOp(OpCode::I32__and));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, IdentityOrZero) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__or));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, IdentityShlZero) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__shl));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

// ---------------------------------------------------------------------------
// Chained folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, ChainedFolding) {
  // i32.const 1, i32.const 2, i32.add, i32.const 4, i32.mul
  // First fold: const 3, nop, nop, const 4, mul
  // Second fold: const 12, nop, nop, nop, nop
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeI32Const(2));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Instrs.push_back(makeI32Const(4));
  Instrs.push_back(makeOp(OpCode::I32__mul));
  Executor::optimizeConstantExpressions(Instrs);

  // Find the surviving const
  uint32_t Result = 0;
  for (auto &I : Instrs) {
    if (I.getOpCode() == OpCode::I32__const) {
      Result = getI32(I);
      break;
    }
  }
  EXPECT_EQ(Result, 12u);
}

TEST(ConstFoldTest, ChainedUnaryAndBinary) {
  // f64.const 9.0, f64.sqrt -> f64.const 3.0
  // then: f64.const 3.0, f64.const 2.0, f64.mul -> f64.const 6.0
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(9.0));
  Instrs.push_back(makeOp(OpCode::F64__sqrt));
  Instrs.push_back(makeF64Const(2.0));
  Instrs.push_back(makeOp(OpCode::F64__mul));
  Executor::optimizeConstantExpressions(Instrs);

  double Result = 0.0;
  for (auto &I : Instrs) {
    if (I.getOpCode() == OpCode::F64__const) {
      Result = getF64(I);
    }
  }
  EXPECT_DOUBLE_EQ(Result, 6.0);
}

// ---------------------------------------------------------------------------
// Jump offset preservation
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, JumpOffsetsPreserved) {
  // Ensure the vector size doesn't change after optimization.
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Block)); // block with jump
  Instrs.push_back(makeI32Const(3));
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Instrs.push_back(makeOp(OpCode::End));

  size_t OrigSize = Instrs.size();
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs.size(), OrigSize);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::Block);
  EXPECT_EQ(Instrs[4].getOpCode(), OpCode::End);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, EmptyInstructions) {
  AST::InstrVec Instrs;
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_TRUE(Instrs.empty());
}

TEST(ConstFoldTest, SingleInstruction) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Nop));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs.size(), 1u);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, NoFoldablePatterns) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::Local__get);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Local__get);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__add);
}

TEST(ConstFoldTest, ConstFollowedByNonFoldable) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeOp(OpCode::Local__get));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Executor::optimizeConstantExpressions(Instrs);
  // const + local_get is not const+const, so no fold
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Local__get);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__add);
}

// ---------------------------------------------------------------------------
// Sign-extension folding
// ---------------------------------------------------------------------------

TEST(ConstFoldTest, I32Extend8S) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0xFF)); // -1 as signed int8
  Instrs.push_back(makeOp(OpCode::I32__extend8_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), -1);
}

TEST(ConstFoldTest, I32Extend16S) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0xFFFE)); // -2 as signed int16
  Instrs.push_back(makeOp(OpCode::I32__extend16_s));
  Executor::optimizeConstantExpressions(Instrs);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), -2);
}

// ---------------------------------------------------------------------------
// SIMD / V128 constant folding
// ---------------------------------------------------------------------------

namespace {

// Build a V128__const instruction from a uint128_t raw value.
AST::Instruction makeV128Const(uint128_t V) {
  ValVariant Val;
  Val.emplace<uint128_t>(V);
  return makeConst(OpCode::V128__const, Val);
}

// Read the raw uint128_t back from a V128__const instruction.
uint128_t getV128(const AST::Instruction &I) {
  return I.getNum().get<uint128_t>();
}

// Build a uint128_t from four uint32_t lanes (little-endian order).
uint128_t makeI32x4(uint32_t L0, uint32_t L1, uint32_t L2, uint32_t L3) {
  uint32_t Lanes[4] = {L0, L1, L2, L3};
  uint128_t V;
  std::memcpy(&V, Lanes, 16);
  return V;
}

// Build a uint128_t from two uint64_t lanes.
uint128_t makeI64x2(uint64_t L0, uint64_t L1) {
  uint64_t Lanes[2] = {L0, L1};
  uint128_t V;
  std::memcpy(&V, Lanes, 16);
  return V;
}

// Build a uint128_t from four float lanes.
uint128_t makeF32x4(float L0, float L1, float L2, float L3) {
  float Lanes[4] = {L0, L1, L2, L3};
  uint128_t V;
  std::memcpy(&V, Lanes, 16);
  return V;
}

// Build a uint128_t from two double lanes.
uint128_t makeF64x2(double L0, double L1) {
  double Lanes[2] = {L0, L1};
  uint128_t V;
  std::memcpy(&V, Lanes, 16);
  return V;
}

// Extract four uint32_t lanes from a V128 result.
void extractI32x4(uint128_t V, uint32_t (&Out)[4]) {
  std::memcpy(Out, &V, 16);
}

// Extract four float lanes from a V128 result.
void extractF32x4(uint128_t V, float (&Out)[4]) {
  std::memcpy(Out, &V, 16);
}

// Extract two double lanes from a V128 result.
void extractF64x2(uint128_t V, double (&Out)[2]) {
  std::memcpy(Out, &V, 16);
}

} // namespace

// V128 / SIMD folds are only available on non-MSVC builds.  On MSVC the
// Executor uses std::array-loop overrides that aren't shared with the fold
// pass, so these tests are built out.
#if !defined(_MSC_VER) || defined(__clang__)

TEST(ConstFoldTest, V128BitwiseAnd) {
  // (v128.const [0xFF00FF00 x4]) & (v128.const [0x0F0F0F0F x4])
  //   = v128.const [0x0F000F00 x4]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0xFF00FF00u, 0xFF00FF00u,
                                            0xFF00FF00u, 0xFF00FF00u)));
  Instrs.push_back(makeV128Const(makeI32x4(0x0F0F0F0Fu, 0x0F0F0F0Fu,
                                            0x0F0F0F0Fu, 0x0F0F0F0Fu)));
  Instrs.push_back(makeOp(OpCode::V128__and));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0x0F000F00u);
  EXPECT_EQ(Out[1], 0x0F000F00u);
  EXPECT_EQ(Out[2], 0x0F000F00u);
  EXPECT_EQ(Out[3], 0x0F000F00u);
}

TEST(ConstFoldTest, V128BitwiseOr) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0xFF000000u, 0x00FF0000u,
                                            0x0000FF00u, 0x000000FFu)));
  Instrs.push_back(makeV128Const(makeI32x4(0x00FF0000u, 0xFF000000u,
                                            0x000000FFu, 0x0000FF00u)));
  Instrs.push_back(makeOp(OpCode::V128__or));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0xFFFF0000u);
  EXPECT_EQ(Out[1], 0xFFFF0000u);
  EXPECT_EQ(Out[2], 0x0000FFFFu);
  EXPECT_EQ(Out[3], 0x0000FFFFu);
}

TEST(ConstFoldTest, V128BitwiseXor) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0xAAAAAAAAu, 0xAAAAAAAAu,
                                            0xAAAAAAAAu, 0xAAAAAAAAu)));
  Instrs.push_back(makeV128Const(makeI32x4(0xFFFFFFFFu, 0xFFFFFFFFu,
                                            0xFFFFFFFFu, 0xFFFFFFFFu)));
  Instrs.push_back(makeOp(OpCode::V128__xor));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0x55555555u);
  EXPECT_EQ(Out[1], 0x55555555u);
  EXPECT_EQ(Out[2], 0x55555555u);
  EXPECT_EQ(Out[3], 0x55555555u);
}

TEST(ConstFoldTest, V128BitwiseAndNot) {
  // andnot(A, B) = A & ~B
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0xFFFFFFFFu, 0xFFFFFFFFu,
                                            0xFFFFFFFFu, 0xFFFFFFFFu)));
  Instrs.push_back(makeV128Const(makeI32x4(0x0F0F0F0Fu, 0x0F0F0F0Fu,
                                            0x0F0F0F0Fu, 0x0F0F0F0Fu)));
  Instrs.push_back(makeOp(OpCode::V128__andnot));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0xF0F0F0F0u);
}

TEST(ConstFoldTest, V128Not) {
  // v128.not flips all bits
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0x00FF00FFu, 0xFF00FF00u,
                                            0x00000000u, 0xFFFFFFFFu)));
  Instrs.push_back(makeOp(OpCode::V128__not));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0xFF00FF00u);
  EXPECT_EQ(Out[1], 0x00FF00FFu);
  EXPECT_EQ(Out[2], 0xFFFFFFFFu);
  EXPECT_EQ(Out[3], 0x00000000u);
}

TEST(ConstFoldTest, I32x4Add) {
  // Lane-wise add: [1,2,3,4] + [10,20,30,40] = [11,22,33,44]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(1, 2, 3, 4)));
  Instrs.push_back(makeV128Const(makeI32x4(10, 20, 30, 40)));
  Instrs.push_back(makeOp(OpCode::I32x4__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 11u);
  EXPECT_EQ(Out[1], 22u);
  EXPECT_EQ(Out[2], 33u);
  EXPECT_EQ(Out[3], 44u);
}

TEST(ConstFoldTest, I32x4MulWrap) {
  // Overflow wraps: 0xFFFFFFFF * 2 = 0xFFFFFFFE
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(0xFFFFFFFFu, 0x80000000u, 5, 1)));
  Instrs.push_back(makeV128Const(makeI32x4(2, 2, 3, 7)));
  Instrs.push_back(makeOp(OpCode::I32x4__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 0xFFFFFFFEu); // wrap
  EXPECT_EQ(Out[1], 0u);          // 0x80000000 * 2 wraps to 0
  EXPECT_EQ(Out[2], 15u);
  EXPECT_EQ(Out[3], 7u);
}

TEST(ConstFoldTest, I32x4MinMax) {
  AST::InstrVec Instrs;
  // min_s: [-1, 2, -3, 4] min_s [0, 1, -2, 5] = [-1, 1, -3, 4]
  auto A = makeI32x4(static_cast<uint32_t>(-1), 2u,
                     static_cast<uint32_t>(-3), 4u);
  auto B = makeI32x4(0u, 1u, static_cast<uint32_t>(-2), 5u);

  Instrs.push_back(makeV128Const(A));
  Instrs.push_back(makeV128Const(B));
  Instrs.push_back(makeOp(OpCode::I32x4__min_s));
  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(static_cast<int32_t>(Out[0]), -1);
  EXPECT_EQ(static_cast<int32_t>(Out[1]),  1);
  EXPECT_EQ(static_cast<int32_t>(Out[2]), -3);
  EXPECT_EQ(static_cast<int32_t>(Out[3]),  4);
}

TEST(ConstFoldTest, I64x2Add) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI64x2(UINT64_MAX, 0)));
  Instrs.push_back(makeV128Const(makeI64x2(1, 42)));
  Instrs.push_back(makeOp(OpCode::I64x2__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint64_t Out[2];
  uint128_t R = getV128(Instrs[0]);
  std::memcpy(Out, &R, 16);
  EXPECT_EQ(Out[0], 0u);  // UINT64_MAX + 1 wraps to 0
  EXPECT_EQ(Out[1], 42u);
}

TEST(ConstFoldTest, F32x4Add) {
  // Per-lane float add: [1.0, 2.0, 3.0, 4.0] + [0.5, 0.5, 0.5, 0.5]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(1.0f, 2.0f, 3.0f, 4.0f)));
  Instrs.push_back(makeV128Const(makeF32x4(0.5f, 0.5f, 0.5f, 0.5f)));
  Instrs.push_back(makeOp(OpCode::F32x4__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 1.5f);
  EXPECT_FLOAT_EQ(Out[1], 2.5f);
  EXPECT_FLOAT_EQ(Out[2], 3.5f);
  EXPECT_FLOAT_EQ(Out[3], 4.5f);
}

TEST(ConstFoldTest, F64x2Mul) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF64x2(3.14159, 2.71828)));
  Instrs.push_back(makeV128Const(makeF64x2(2.0, 3.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_DOUBLE_EQ(Out[0], 3.14159 * 2.0);
  EXPECT_DOUBLE_EQ(Out[1], 2.71828 * 3.0);
}

TEST(ConstFoldTest, F32x4MinNaN) {
  // f32x4.min with one NaN lane: NaN propagates (B's NaN wins when both NaN)
  AST::InstrVec Instrs;
  float NaN1 = std::numeric_limits<float>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF32x4(NaN1, 1.0f, 3.0f, 5.0f)));
  Instrs.push_back(makeV128Const(makeF32x4(2.0f, NaN1, 2.0f, 4.0f)));
  Instrs.push_back(makeOp(OpCode::F32x4__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_TRUE(std::isnan(Out[0])); // lane 0: A=NaN, B=2.0 → NaN from A
  EXPECT_TRUE(std::isnan(Out[1])); // lane 1: A=1.0, B=NaN → NaN from B
  EXPECT_FLOAT_EQ(Out[2], 2.0f);  // lane 2: min(3.0, 2.0) = 2.0
  EXPECT_FLOAT_EQ(Out[3], 4.0f);  // lane 3: min(5.0, 4.0) = 4.0
}

TEST(ConstFoldTest, F32x4PMin) {
  // pmin is pseudo-min: B if B < A else A, no NaN handling
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(5.0f, 1.0f, 3.0f, 2.0f)));
  Instrs.push_back(makeV128Const(makeF32x4(3.0f, 4.0f, 3.0f, 1.0f)));
  Instrs.push_back(makeOp(OpCode::F32x4__pmin));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 3.0f);
  EXPECT_FLOAT_EQ(Out[1], 1.0f);
  EXPECT_FLOAT_EQ(Out[2], 3.0f);
  EXPECT_FLOAT_EQ(Out[3], 1.0f);
}

TEST(ConstFoldTest, I32x4Neg) {
  // neg([1, -1, INT32_MIN, 0]) = [-1, 1, INT32_MIN, 0] (wrap on MIN)
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI32x4(1u,
      static_cast<uint32_t>(-1),
      static_cast<uint32_t>(std::numeric_limits<int32_t>::min()),
      0u)));
  Instrs.push_back(makeOp(OpCode::I32x4__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(static_cast<int32_t>(Out[0]), -1);
  EXPECT_EQ(static_cast<int32_t>(Out[1]),  1);
  EXPECT_EQ(Out[2], static_cast<uint32_t>(std::numeric_limits<int32_t>::min())); // wrap
  EXPECT_EQ(Out[3], 0u);
}

TEST(ConstFoldTest, F32x4Abs) {
  // abs preserves NaN, clears sign bit for finite values
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(-1.0f, 2.0f, -0.0f, 0.0f)));
  Instrs.push_back(makeOp(OpCode::F32x4__abs));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 1.0f);
  EXPECT_FLOAT_EQ(Out[1], 2.0f);
  EXPECT_EQ(std::signbit(Out[2]), false); // -0.0 → +0.0
  EXPECT_EQ(std::signbit(Out[3]), false);
}

TEST(ConstFoldTest, F64x2Sqrt) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF64x2(4.0, 9.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__sqrt));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_DOUBLE_EQ(Out[0], 2.0);
  EXPECT_DOUBLE_EQ(Out[1], 3.0);
}

TEST(ConstFoldTest, I32x4Splat) {
  // i32.const 42, i32x4.splat → v128.const [42, 42, 42, 42]
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(42));
  Instrs.push_back(makeOp(OpCode::I32x4__splat));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 42u);
  EXPECT_EQ(Out[1], 42u);
  EXPECT_EQ(Out[2], 42u);
  EXPECT_EQ(Out[3], 42u);
}

TEST(ConstFoldTest, I64x2Splat) {
  // i64.const UINT64_MAX, i64x2.splat → v128.const [UINT64_MAX, UINT64_MAX]
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_MAX));
  Instrs.push_back(makeOp(OpCode::I64x2__splat));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint64_t Out[2];
  uint128_t R = getV128(Instrs[0]);
  std::memcpy(Out, &R, 16);
  EXPECT_EQ(Out[0], UINT64_MAX);
  EXPECT_EQ(Out[1], UINT64_MAX);
}

TEST(ConstFoldTest, F32x4Splat) {
  // f32.const 3.14f, f32x4.splat → v128.const [3.14f x4]
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(3.14f));
  Instrs.push_back(makeOp(OpCode::F32x4__splat));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 3.14f);
  EXPECT_FLOAT_EQ(Out[1], 3.14f);
  EXPECT_FLOAT_EQ(Out[2], 3.14f);
  EXPECT_FLOAT_EQ(Out[3], 3.14f);
}

TEST(ConstFoldTest, F64x2Splat) {
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(2.71828));
  Instrs.push_back(makeOp(OpCode::F64x2__splat));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_DOUBLE_EQ(Out[0], 2.71828);
  EXPECT_DOUBLE_EQ(Out[1], 2.71828);
}

TEST(ConstFoldTest, I8x16SplatTruncates) {
  // i8x16.splat takes low 8 bits of the i32 operand
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x1234));
  Instrs.push_back(makeOp(OpCode::I8x16__splat));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint8_t Out[16];
  uint128_t R = getV128(Instrs[0]);
  std::memcpy(Out, &R, 16);
  for (uint8_t Lane : Out)
    EXPECT_EQ(Lane, uint8_t{0x34}); // low 8 bits of 0x1234
}

TEST(ConstFoldTest, V128SplatChainedWithBinary) {
  // Splat then binary: (i32x4.splat 3) add (i32x4.splat 7) = [10, 10, 10, 10]
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(3));
  Instrs.push_back(makeOp(OpCode::I32x4__splat));
  Instrs.push_back(makeI32Const(7));
  Instrs.push_back(makeOp(OpCode::I32x4__splat));
  Instrs.push_back(makeOp(OpCode::I32x4__add));

  Executor::optimizeConstantExpressions(Instrs);

  // After first pass: splats fold to two v128.const.
  // After second pass: the two v128.const + i32x4.add fold to one v128.const.
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint32_t Out[4];
  extractI32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], 10u);
  EXPECT_EQ(Out[1], 10u);
  EXPECT_EQ(Out[2], 10u);
  EXPECT_EQ(Out[3], 10u);
}

#endif // !defined(_MSC_VER) || defined(__clang__)

// ---------------------------------------------------------------------------
// Wrong-code regression tests — derived from GCC and LLVM bug reports for
// IPCP / constant-folding miscompilations.  Each test documents the original
// compiler bug category, the WASM translation, and the expected behaviour of
// our constant-folding pass.
// ---------------------------------------------------------------------------

// --- Category: Shift by width -----------------------------------------------
//
// GCC and LLVM had bugs where a shift-by-width was mis-evaluated using C
// undefined-behaviour semantics (yielding 0) instead of the target language's
// shift-masking rule.  In WebAssembly, i32.shl/shr_s/shr_u mask the shift
// amount by 31 (amount & 0x1f) and i64.shl/shr* mask by 63 (amount & 0x3f).
//
// Real confirmed bugs:
//   GCC: "ifcombine may move shift so it shifts more than bitwidth"
//        https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106884
//   LLVM: "wrong code for bit shifting integer promotion at -O1 and above"
//         https://github.com/llvm/llvm-project/issues/18349

TEST(ConstFoldTest, WrongCode_I32ShlByWidth) {
  // i32.shl(1, 32): WASM masks 32 & 31 = 0, result = 1.
  // A naive C fold would compute 1 << 32 = UB (typically 0 on x86).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeI32Const(32));
  Instrs.push_back(makeOp(OpCode::I32__shl));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 1u); // 1 << (32 & 31) = 1 << 0 = 1
}

TEST(ConstFoldTest, WrongCode_I32ShrsByOne) {
  // i32.shr_s(-1, 33): WASM masks 33 & 31 = 1, result = -1 (arithmetic shr).
  // Bug category: unsigned-shift vs signed-shift confusion under masking.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeI32Const(33));
  Instrs.push_back(makeOp(OpCode::I32__shr_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  // -1 arithmetic-right-shifted by 1 is -1 (all ones).
  EXPECT_EQ(getI32(Instrs[0]), static_cast<uint32_t>(-1));
}

TEST(ConstFoldTest, WrongCode_I32ShruByWidth) {
  // i32.shr_u(0x8000_0000, 32): WASM masks 32 & 31 = 0, result = 0x8000_0000.
  // A wrong fold using C UB would give 0 (logical shift by 32).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x80000000u));
  Instrs.push_back(makeI32Const(32));
  Instrs.push_back(makeOp(OpCode::I32__shr_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0x80000000u);
}

TEST(ConstFoldTest, WrongCode_I64ShlByWidth) {
  // i64.shl(1, 64): WASM masks 64 & 63 = 0, result = 1.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(1));
  Instrs.push_back(makeI64Const(64));
  Instrs.push_back(makeOp(OpCode::I64__shl));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 1ull);
}

TEST(ConstFoldTest, WrongCode_I64ShruByWidth) {
  // i64.shr_u(UINT64_MAX, 64): WASM masks 64 & 63 = 0, result = UINT64_MAX.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_MAX));
  Instrs.push_back(makeI64Const(64));
  Instrs.push_back(makeOp(OpCode::I64__shr_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_MAX);
}

TEST(ConstFoldTest, WrongCode_I64ShrsByMasked) {
  // i64.shr_s(INT64_MIN, 65): WASM masks 65 & 63 = 1, result = INT64_MIN/2
  // (arithmetic shift, so high bit preserved → 0xC000_0000_0000_0000).
  AST::InstrVec Instrs;
  uint64_t IntMin64 = uint64_t(1) << 63;
  Instrs.push_back(makeI64Const(IntMin64));
  Instrs.push_back(makeI64Const(65));
  Instrs.push_back(makeOp(OpCode::I64__shr_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  // INT64_MIN >> 1 (arithmetic) = 0xC000000000000000
  EXPECT_EQ(getI64(Instrs[0]), 0xC000000000000000ull);
}

// --- Category: Division / remainder by zero ---------------------------------
//
// LLVM's constant propagation had bugs where X/0 was incorrectly folded to 0
// or the dividend instead of preserving the trap.  In WASM, integer
// division/rem by zero always traps — the fold must be suppressed entirely.
//
// Real confirmed bugs:
//   LLVM: "Clang silently eliminates division-by-zero error detection when
//          optimizations are enabled"
//         https://github.com/llvm/llvm-project/issues/136679
//   LLVM: "Clang stop detecting UBs after a divide by zero"
//         https://github.com/llvm/llvm-project/issues/45469

TEST(ConstFoldTest, WrongCode_DivByZeroI32NotFolded) {
  // i32.div_u(5, 0) must NOT fold — it traps in WASM.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__div_u));

  Executor::optimizeConstantExpressions(Instrs);

  // All three instructions must remain unchanged.
  ASSERT_EQ(Instrs.size(), 3u);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__div_u);
}

TEST(ConstFoldTest, WrongCode_DivByZeroI32SignedNotFolded) {
  // i32.div_s(5, 0) must NOT fold — it traps.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__div_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__div_s);
}

TEST(ConstFoldTest, WrongCode_RemByZeroI32NotFolded) {
  // i32.rem_u(5, 0) must NOT fold — it traps.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__rem_u));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__rem_u);
}

TEST(ConstFoldTest, WrongCode_RemByZeroI32SignedNotFolded) {
  // i32.rem_s(5, 0) must NOT fold — it traps.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__rem_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__rem_s);
}

TEST(ConstFoldTest, WrongCode_DivByZeroI64NotFolded) {
  // i64.div_u(99, 0) must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(99));
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__div_u));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I64__div_u);
}

TEST(ConstFoldTest, WrongCode_ZeroDividendFolds) {
  // 0 / non-zero = 0 and is safe to fold.  Some buggy passes skipped this.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeI32Const(5));
  Instrs.push_back(makeOp(OpCode::I32__div_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

// --- Category: INT_MIN / -1 signed division overflow -----------------------
//
// Signed integer division INT_MIN / -1 overflows in two's complement and traps
// in WASM (WebAssembly Core Spec §4.3.2, "idiv_s").  Several compilers have
// propagated INT_MIN / -1 as a constant (yielding INT_MIN or 0) instead of
// respecting the trap semantics.  The WASM spec is unambiguous — we must NOT
// fold this call site.
// (No single canonical tracked bug URL was found for WASM/IPCP specifically;
//  the behaviour is mandated by the WASM specification.)

TEST(ConstFoldTest, WrongCode_I32IntMinDivNegOneNotFolded) {
  // i32.div_s(INT32_MIN, -1) must NOT fold — WASM traps.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(std::numeric_limits<int32_t>::min())));
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I32__div_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs.size(), 3u);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__div_s);
}

TEST(ConstFoldTest, WrongCode_I64IntMinDivNegOneNotFolded) {
  // i64.div_s(INT64_MIN, -1) must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(static_cast<uint64_t>(std::numeric_limits<int64_t>::min())));
  Instrs.push_back(makeI64Const(static_cast<uint64_t>(-1LL)));
  Instrs.push_back(makeOp(OpCode::I64__div_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I64__div_s);
}

// --- Category: INT_MIN rem -1 = 0 ------------------------------------------
//
// Unlike div_s, rem_s does NOT trap when the dividend is INT_MIN and the
// divisor is -1; per WASM Core Spec §4.3.2 ("irem_s") the result is 0.
// A fold that over-applies the INT_MIN/-1 guard (blocking even rem) would
// incorrectly leave this as a Call, and a fold that miscomputes the result
// would produce INT_MIN instead of 0.
// (Behaviour is mandated by the WASM specification; no single canonical
//  upstream bug tracker entry was found for this precise WASM case.)

TEST(ConstFoldTest, WrongCode_I32IntMinRemNegOneFoldsToZero) {
  // i32.rem_s(INT32_MIN, -1) must fold to 0 (WASM spec, no trap for rem).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(std::numeric_limits<int32_t>::min())));
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I32__rem_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::Nop);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::Nop);
}

TEST(ConstFoldTest, WrongCode_I64IntMinRemNegOneFoldsToZero) {
  // i64.rem_s(INT64_MIN, -1) must fold to 0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(static_cast<uint64_t>(std::numeric_limits<int64_t>::min())));
  Instrs.push_back(makeI64Const(static_cast<uint64_t>(-1LL)));
  Instrs.push_back(makeOp(OpCode::I64__rem_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0ull);
}

// --- Category: Normal division / remainder folds correctly -----------------
//
// Ensure the safety guards do not over-eagerly suppress valid folds.

TEST(ConstFoldTest, WrongCode_SafeDivFolds) {
  // i32.div_s(10, 3) = 3.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(10));
  Instrs.push_back(makeI32Const(3));
  Instrs.push_back(makeOp(OpCode::I32__div_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), 3);
}

TEST(ConstFoldTest, WrongCode_SafeUnsignedDivFolds) {
  // i32.div_u(100, 7) = 14.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(100));
  Instrs.push_back(makeI32Const(7));
  Instrs.push_back(makeOp(OpCode::I32__div_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 14u);
}

// --- Category: Signed vs unsigned integer-to-float conversion --------------
//
// LLVM's ConstantRange analysis had off-by-one bugs in UIToFP and SIToFP
// handling that caused wrong range results during constant propagation,
// and InstCombine could misidentify the correct conversion direction for
// negative propagated constants.  In WASM, f64.convert_i32_s(-1) = -1.0 but
// f64.convert_i32_u(-1 as uint32) = 4294967295.0 — these must not be mixed.
//
// Real confirmed bugs:
//   LLVM: "[ConstantRange] Fix off by 1 bugs in UIToFP and SIToFP handling"
//         https://github.com/llvm/llvm-project/commit/feba8727f80566074518c9dbb5e90c8f2371c08d
//   LLVM: "[InstCombine] likely incorrect optimization of sitofp/fptosi roundtrip"
//         https://github.com/llvm/llvm-project/issues/55150

TEST(ConstFoldTest, WrongCode_F64ConvertI32SignedNeg) {
  // f64.convert_i32_s(-1) must produce -1.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::F64__convert_i32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(getF64(Instrs[0]), -1.0);
}

TEST(ConstFoldTest, WrongCode_F64ConvertI32UnsignedNeg) {
  // f64.convert_i32_u(-1 as uint32) must produce 4294967295.0.
  // A wrong fold using signed interpretation would give -1.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::F64__convert_i32_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(getF64(Instrs[0]), 4294967295.0);
}

TEST(ConstFoldTest, WrongCode_F32ConvertI32SignedNeg) {
  // f32.convert_i32_s(-1) must produce -1.0f.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::F32__convert_i32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_FLOAT_EQ(getF32(Instrs[0]), -1.0f);
}

TEST(ConstFoldTest, WrongCode_F32ConvertI32UnsignedNeg) {
  // f32.convert_i32_u(-1 as uint32) must produce 4294967296.0f (rounds up to
  // next representable f32 above 4294967295).  Signed path would give -1.0f.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::F32__convert_i32_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_FLOAT_EQ(getF32(Instrs[0]), 4294967296.0f);
}

// --- Category: i64.extend signed vs unsigned --------------------------------
//
// LLVM's backend and mid-end passes have confused sign extension (sext) with
// zero extension (zext) when folding constants through integer widening
// operations, producing 0xFFFFFFFFFFFFFFFF where 0x00000000FFFFFFFF was
// expected (or vice-versa).  WASM has distinct i64.extend_i32_s (sext) and
// i64.extend_i32_u (zext) instructions that must be folded with the correct
// semantics.
//
// Real confirmed bugs:
//   LLVM: "shift/zext-related miscompile by aarch64 backend"
//         https://github.com/llvm/llvm-project/issues/55833
//   LLVM: "[DAGCombiner] trunc + zext on i64 to i32 folded into constant 0"
//         https://www.mail-archive.com/llvm-bugs@lists.llvm.org/msg89351.html

TEST(ConstFoldTest, WrongCode_I64ExtendI32SignedNeg) {
  // i64.extend_i32_s(-1) = 0xFFFFFFFFFFFFFFFF (sign-extended).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I64__extend_i32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_MAX); // 0xFFFFFFFFFFFFFFFF
}

TEST(ConstFoldTest, WrongCode_I64ExtendI32UnsignedNeg) {
  // i64.extend_i32_u(-1 as uint32) = 0x00000000FFFFFFFF = 4294967295.
  // A wrong fold using signed extension would give 0xFFFFFFFFFFFFFFFF.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(static_cast<uint32_t>(-1)));
  Instrs.push_back(makeOp(OpCode::I64__extend_i32_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0x00000000FFFFFFFFull);
}

TEST(ConstFoldTest, WrongCode_I64ExtendI32SignedPositive) {
  // i64.extend_i32_s(0x7FFFFFFF) = 0x000000007FFFFFFF.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x7FFFFFFFu));
  Instrs.push_back(makeOp(OpCode::I64__extend_i32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0x000000007FFFFFFFull);
}

// --- Category: trunc safety — NaN / Inf / out-of-range ---------------------
//
// LLVM's fptosi / fptoui intrinsics produce poison (undefined) for NaN, ±Inf,
// and out-of-range values; LLVM's constant folder exploited this to fold these
// to 0.  WebAssembly's i32.trunc_f32_s / i64.trunc_f64_u etc. instead *trap*
// on the same inputs (WASM Core Spec §4.3.2, "itrunc_f"), so our fold must
// leave those call sites intact.  The saturating variants (trunc_sat_*) never
// trap and always produce a defined clamped result — those must fold.
//
// Real confirmed bugs:
//   Emscripten: "Correctly implement LLVM's fptoui/fptosi" — describes how
//     LLVM speculatively hoists fptosi/fptoui past guards, causing WASM traps
//     on NaN inputs that the original C code guarded against.
//     https://github.com/emscripten-core/emscripten/issues/5498

TEST(ConstFoldTest, WrongCode_TruncNaNNotFolded) {
  // i32.trunc_f32_s(NaN) must NOT fold — it traps in WASM.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs.size(), 2u);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f32_s);
}

TEST(ConstFoldTest, WrongCode_TruncInfNotFolded) {
  // i32.trunc_f32_s(+Inf) must NOT fold — it traps.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::infinity()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f32_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f32_s);
}

TEST(ConstFoldTest, WrongCode_TruncNegInfNotFolded) {
  // i32.trunc_f64_s(-Inf) must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f64_s);
}

TEST(ConstFoldTest, WrongCode_TruncOutOfRangeNotFolded) {
  // i32.trunc_f64_s(3e10) is out of i32 range — must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(3e10));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f64_s);
}

TEST(ConstFoldTest, WrongCode_TruncValidNegFolds) {
  // i32.trunc_f64_s(-1.7) = -1 (rounds toward zero) — must fold correctly.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-1.7));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])), -1);
}

TEST(ConstFoldTest, WrongCode_TruncI64ValidFolds) {
  // i64.trunc_f64_u(4294967295.0) = 4294967295 — must fold correctly.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(4294967295.0));
  Instrs.push_back(makeOp(OpCode::I64__trunc_f64_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 4294967295ull);
}

TEST(ConstFoldTest, WrongCode_TruncSatNaNFoldsToZero) {
  // i32.trunc_sat_f32_s(NaN) = 0 per WASM sat spec — must fold (does NOT trap).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_sat_f32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, WrongCode_TruncSatInfClampsToMax) {
  // i32.trunc_sat_f32_s(+Inf) = INT32_MAX per WASM sat spec.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::infinity()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_sat_f32_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])),
            std::numeric_limits<int32_t>::max());
}

TEST(ConstFoldTest, WrongCode_TruncSatNegInfClampsToMin) {
  // i32.trunc_sat_f64_s(-Inf) = INT32_MIN per WASM sat spec.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::I32__trunc_sat_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])),
            std::numeric_limits<int32_t>::min());
}

// --- Category: f32 precision ------------------------------------------------
//
// LLVM's ConstantFolding converts all floating-point operands to double
// regardless of the source type, then demotes the result back — producing bit
// patterns that differ from the correct single-precision result.  Our fold
// must use f32 (single-precision) arithmetic throughout.
//
// Real confirmed bugs:
//   LLVM: "Incorrect constant folding behavior of floating-point operations"
//         https://github.com/llvm/llvm-project/issues/62479
//   LLVM Discourse: "ConstantFolding: why are FP32 calls folded in
//         double-precision?"
//         https://discourse.llvm.org/t/constantfolding-why-are-fp32-calls-folded-in-double-precision/89878

TEST(ConstFoldTest, WrongCode_F32AddPrecision) {
  // f32.add(1.0f, 0x1p-24f): result must equal the f32-precision answer.
  // In double precision the sum is exactly 1.000000059604644775390625, which
  // rounds to a different f32 bit pattern than the correct f32 result.
  float A = 1.0f;
  float B = std::numeric_limits<float>::epsilon() / 2.0f; // 0x1p-24
  float Expected = A + B; // computed in f32 by the C compiler

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(A));
  Instrs.push_back(makeF32Const(B));
  Instrs.push_back(makeOp(OpCode::F32__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  // Compare bit-for-bit using memcmp to catch signed-zero / NaN pattern diffs.
  float GotFloat0 = getF32(Instrs[0]);
  uint32_t GotBits, ExpBits;
  std::memcpy(&GotBits, &GotFloat0, 4);
  std::memcpy(&ExpBits, &Expected, 4);
  EXPECT_EQ(GotBits, ExpBits);
}

TEST(ConstFoldTest, WrongCode_F32MulDoesNotPromoteToF64) {
  // f32.mul(3.0f, 1.0f/3.0f): result should be exactly as f32 computes it,
  // not the more accurate f64 result 1.0.
  float A = 3.0f;
  float B = 1.0f / 3.0f;
  float Expected = A * B;

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(A));
  Instrs.push_back(makeF32Const(B));
  Instrs.push_back(makeOp(OpCode::F32__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  float GotFloat1 = getF32(Instrs[0]);
  uint32_t GotBits1, ExpBits1;
  std::memcpy(&GotBits1, &GotFloat1, 4);
  std::memcpy(&ExpBits1, &Expected, 4);
  EXPECT_EQ(GotBits1, ExpBits1);
}

// --- Category: arithmetic vs logical right shift confusion ------------------
//
// LLVM's InstCombine and SCEV passes have confused ashr (arithmetic / signed
// shift right, sign-bit replicated) with lshr (logical / unsigned shift right,
// zero-filled) when folding constants propagated through function arguments,
// producing wrong negative values.  WASM has distinct i32.shr_s (arithmetic)
// and i32.shr_u (logical) instructions.
//
// Real confirmed bugs:
//   LLVM: "InstCombine: wrong folding of a constant comparison involving ashr
//          and negative numbers" — FoldICmpCstShrCst used negation instead of
//          one's complement, giving wrong distance for ashr of negative values.
//         https://github.com/llvm/llvm-project/issues/21596
//   LLVM: "[SCEV] Use ashr to adjust constant multipliers" — SCEV used lshr
//          instead of ashr for negative constant multiplier adjustment.
//         https://github.com/llvm/llvm-project/commit/0dd4235473d4f5a99c46ea631351616d62e9b32e

TEST(ConstFoldTest, WrongCode_I32ArithmeticVsLogicalShr) {
  // i32.shr_s(0x80000001, 1) = 0xC0000000 (arithmetic, sign bit replicated).
  // i32.shr_u(0x80000001, 1) = 0x40000000 (logical, zero-fills).
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0x80000001u));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__shr_s));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
    EXPECT_EQ(getI32(Instrs[0]), 0xC0000000u);
  }
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(0x80000001u));
    Instrs.push_back(makeI32Const(1));
    Instrs.push_back(makeOp(OpCode::I32__shr_u));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
    EXPECT_EQ(getI32(Instrs[0]), 0x40000000u);
  }
}

// --- Category: i32.wrap / i64.extend round-trip -----------------------------
//
// Constants propagated through a truncation (i32.wrap_i64) followed by
// widening (i64.extend_i32_s or i64.extend_i32_u) must preserve the correct
// sign semantics.  If the widening direction is mis-identified the high 32
// bits of the result will be wrong (all-ones vs all-zeros for values whose
// bit 31 is set).  This verifies both extend_s and extend_u paths produce
// distinct correct results for the same wrapped value.
// (Behaviour is intrinsic to the WASM type system; no single canonical
//  upstream bug tracker entry was found for this precise round-trip case.)

TEST(ConstFoldTest, WrongCode_I32WrapRoundTrip) {
  // i64 value 0x1_8000_0000 → wrap to i32 = 0x8000_0000 (negative as signed).
  // Then extend_s back to i64 gives 0xFFFF_FFFF_8000_0000.
  // But extend_u back to i64 gives 0x0000_0000_8000_0000.
  uint64_t Original = 0x180000000ull;
  uint32_t Wrapped = static_cast<uint32_t>(Original & 0xFFFFFFFF);
  ASSERT_EQ(Wrapped, 0x80000000u);

  // extend_s path
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(Wrapped));
    Instrs.push_back(makeOp(OpCode::I64__extend_i32_s));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
    EXPECT_EQ(getI64(Instrs[0]), 0xFFFFFFFF80000000ull);
  }
  // extend_u path
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeI32Const(Wrapped));
    Instrs.push_back(makeOp(OpCode::I64__extend_i32_u));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
    EXPECT_EQ(getI64(Instrs[0]), 0x0000000080000000ull);
  }
}

// --- Category: reinterpret bit-pattern preservation ------------------------
//
// WASM's reinterpret instructions (f32.reinterpret_i32 etc.) are pure
// bit-casts: the bit pattern is preserved verbatim.  A fold that converts to
// a floating-point value and back (doing arithmetic rather than bit-casting)
// will canonicalise NaN payloads or collapse ±0.0, producing the wrong result.
// This verifies that NaN payloads (including sNaN bit patterns) and negative
// zero survive a round-trip through our constant folder unchanged.
// (The WASM reinterpret semantics are specified in WASM Core Spec §4.3.3;
//  no single LLVM/GCC bug tracker entry was identified for this specific
//  WASM constant-folding scenario.)

TEST(ConstFoldTest, WrongCode_ReinterpretI32AsF32NaN) {
  // i32.reinterpret_f32 of the canonical NaN bit pattern 0x7FC00000.
  // The inverse: f32.reinterpret_i32(0x7FC00000) must give that NaN bit
  // pattern, NOT a "normalised" NaN.
  uint32_t NaNBits = 0x7FC00000u;
  float NaNFloat;
  std::memcpy(&NaNFloat, &NaNBits, 4);

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(NaNFloat));
  Instrs.push_back(makeOp(OpCode::I32__reinterpret_f32));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), NaNBits);
}

TEST(ConstFoldTest, WrongCode_ReinterpretF32NaNPayload) {
  // f32.reinterpret_i32 of a signalling NaN with a specific payload.
  // Must not canonicalise the NaN.
  uint32_t SNaNBits = 0x7F800001u; // sNaN with payload 1
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(SNaNBits));
  Instrs.push_back(makeOp(OpCode::F32__reinterpret_i32));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  float GotFloat2 = getF32(Instrs[0]);
  uint32_t GotBits2;
  std::memcpy(&GotBits2, &GotFloat2, 4);
  EXPECT_EQ(GotBits2, SNaNBits);
}

TEST(ConstFoldTest, WrongCode_ReinterpretSignedZero) {
  // i64.reinterpret_f64(-0.0) must produce 0x8000000000000000, not 0.
  double NegZero = -0.0;
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(NegZero));
  Instrs.push_back(makeOp(OpCode::I64__reinterpret_f64));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0x8000000000000000ull);
}

// ===========================================================================
// Tests derived from V8 and JavaScriptCore (WebKit) bytecode optimiser
// test suites.  V8's Ignition interpreter and JSC's LLInt/Baseline perform
// constant folding at the bytecode level — many of those tests exercise
// identical semantic edges that a WASM constant folder must respect.
// ===========================================================================

// --- Category: f64.min / f64.max NaN propagation ----------------------------
//
// JSC's BBQ JIT used C++ std::min/std::max for WASM f64.min/f64.max, which
// filters NaN and returns the non-NaN operand.  WASM mandates NaN propagation:
// if EITHER operand is NaN, the result is canonical NaN (0x7FF8000000000000).
//
// Real confirmed bugs:
//   WebKit: "BBQ JIT: f32.min/f32.max NaN handling incorrect"
//           https://bugs.webkit.org/show_bug.cgi?id=270262
//   GCC:   "Constant folding of _mm_min_ss/_mm_max_ss wrong for NaN and -0.0"
//           https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116738
//
// V8 test: test/mjsunit/wasm/float-constant-folding.js
// JSC test: JSTests/wasm/stress/fp-nan-minmax.js

namespace {
// Helper: extract f64 bits for exact comparison (avoids NaN != NaN issue).
uint64_t f64Bits(double V) {
  uint64_t B;
  std::memcpy(&B, &V, sizeof(B));
  return B;
}
uint64_t f64BitsFromInstr(const AST::Instruction &I) {
  return f64Bits(I.getNum().get<double>());
}
uint32_t f32Bits(float V) {
  uint32_t B;
  std::memcpy(&B, &V, sizeof(B));
  return B;
}
uint32_t f32BitsFromInstr(const AST::Instruction &I) {
  return f32Bits(I.getNum().get<float>());
}
} // namespace

static constexpr uint64_t kF64CanonicalNaN = UINT64_C(0x7FF8000000000000);
static constexpr uint32_t kF32CanonicalNaN = UINT32_C(0x7FC00000);

TEST(ConstFoldTest, JSC_F64MinNaN_LhsNaN) {
  // f64.min(NaN, 42.0) = canonical NaN (not 42.0).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(42.0));
  Instrs.push_back(makeOp(OpCode::F64__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, JSC_F64MinNaN_RhsNaN) {
  // f64.min(42.0, NaN) = canonical NaN.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(42.0));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, JSC_F64MaxNaN_LhsNaN) {
  // f64.max(NaN, -5.0) = canonical NaN (not -5.0).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(-5.0));
  Instrs.push_back(makeOp(OpCode::F64__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, JSC_F64MaxNaN_RhsNaN) {
  // f64.max(-5.0, NaN) = canonical NaN.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-5.0));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, JSC_F32MinNaN) {
  // f32.min(NaN, 1.0f) = canonical NaN.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeF32Const(1.0f));
  Instrs.push_back(makeOp(OpCode::F32__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), kF32CanonicalNaN);
}

TEST(ConstFoldTest, JSC_F32MaxNaN) {
  // f32.max(1.0f, NaN) = canonical NaN.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(1.0f));
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F32__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), kF32CanonicalNaN);
}

// --- Category: f64.min / f64.max with negative zero -------------------------
//
// WASM spec: f64.min(-0.0, +0.0) = -0.0; f64.max(-0.0, +0.0) = +0.0.
// A naive `std::min` (or < comparison) treats -0.0 == +0.0 and may pick the
// wrong one.
//
//   GCC: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116738
//   V8:  test/mjsunit/minus-zero.js

TEST(ConstFoldTest, JSC_F64MinNegZero_AB) {
  // f64.min(-0.0, +0.0) = -0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeOp(OpCode::F64__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, JSC_F64MinNegZero_BA) {
  // f64.min(+0.0, -0.0) = -0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeOp(OpCode::F64__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, JSC_F64MaxNegZero_AB) {
  // f64.max(-0.0, +0.0) = +0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeOp(OpCode::F64__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(+0.0));
}

TEST(ConstFoldTest, JSC_F64MaxNegZero_BA) {
  // f64.max(+0.0, -0.0) = +0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeOp(OpCode::F64__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(+0.0));
}

TEST(ConstFoldTest, JSC_F32MinNegZero) {
  // f32.min(+0.0f, -0.0f) = -0.0f.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(+0.0f));
  Instrs.push_back(makeF32Const(-0.0f));
  Instrs.push_back(makeOp(OpCode::F32__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), f32Bits(-0.0f));
}

TEST(ConstFoldTest, JSC_F32MaxNegZero) {
  // f32.max(-0.0f, +0.0f) = +0.0f.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(-0.0f));
  Instrs.push_back(makeF32Const(+0.0f));
  Instrs.push_back(makeOp(OpCode::F32__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), f32Bits(+0.0f));
}

// --- Category: NaN comparison folding (all 6 relational ops) ----------------
//
// IEEE 754 mandates that all comparisons with NaN return false, except !=
// which returns true.  JSC added explicit regression tests for each after
// implementing comparison folding.
//
// Real confirmed bugs / tests:
//   WebKit: https://bugs.webkit.org/show_bug.cgi?id=292246
//   JSC tests: JSTests/stress/fold-nan-eq.js through fold-nan-gte.js

TEST(ConstFoldTest, JSC_F64EqNaN) {
  // f64.eq(NaN, 1.0) = 0 (false).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__eq));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, JSC_F64NeNaN) {
  // f64.ne(NaN, 1.0) = 1 (true — the ONLY comparison that returns true for NaN).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__ne));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 1u);
}

TEST(ConstFoldTest, JSC_F64LtNaN) {
  // f64.lt(NaN, 1.0) = 0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__lt));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, JSC_F64GtNaN) {
  // f64.gt(1.0, NaN) = 0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__gt));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, JSC_F64LeNaN) {
  // f64.le(NaN, NaN) = 0.  Self-comparison with NaN is also false.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__le));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, JSC_F64GeNaN) {
  // f64.ge(NaN, -Inf) = 0.  NaN is not >= anything.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(-std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::F64__ge));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

// --- Category: NaN-producing f64 arithmetic ---------------------------------
//
// LLVM's constant folder produced inconsistent NaN sign bits for operations
// like 0*Inf and Inf-Inf.  In WASM, the result must be canonical NaN.
//
// Real confirmed bugs:
//   LLVM: "Inconsistent compile-time and runtime NaN production"
//         https://github.com/llvm/llvm-project/issues/61973
//   LLVM: "Unexpected behavior from std::fma when passing optimization flags"
//         https://github.com/llvm/llvm-project/issues/92592
//
// V8 test: test/mjsunit/wasm/float-constant-folding.js (sNaN arithmetic)
// V8 test: test/mjsunit/nans.js (NaN bit-pattern verification)

TEST(ConstFoldTest, V8_F64MulZeroInf) {
  // f64.mul(0.0, +Inf) = NaN (IEEE 754: 0 × ∞ is NaN).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::infinity()));
  Instrs.push_back(makeOp(OpCode::F64__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

TEST(ConstFoldTest, V8_F64SubInfInf) {
  // f64.sub(+Inf, +Inf) = NaN (IEEE 754: ∞ − ∞ is NaN).
  AST::InstrVec Instrs;
  double Inf = std::numeric_limits<double>::infinity();
  Instrs.push_back(makeF64Const(Inf));
  Instrs.push_back(makeF64Const(Inf));
  Instrs.push_back(makeOp(OpCode::F64__sub));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

TEST(ConstFoldTest, V8_F64DivZeroZero) {
  // f64.div(0.0, 0.0) = NaN.  (Note: unlike integer div, float 0/0 = NaN,
  // not a trap.  The constant folder must fold this, not block it.)
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__div));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

TEST(ConstFoldTest, V8_F64DivByZeroProducesInf) {
  // f64.div(1.0, 0.0) = +Inf.  (Float division by zero does NOT trap.)
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__div));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(getF64(Instrs[0]), std::numeric_limits<double>::infinity());
}

TEST(ConstFoldTest, V8_F64DivByNegZeroProducesNegInf) {
  // f64.div(1.0, -0.0) = -Inf.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeOp(OpCode::F64__div));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(getF64(Instrs[0]), -std::numeric_limits<double>::infinity());
}

TEST(ConstFoldTest, V8_F64AddInfNegInf) {
  // f64.add(+Inf, -Inf) = NaN.
  AST::InstrVec Instrs;
  double Inf = std::numeric_limits<double>::infinity();
  Instrs.push_back(makeF64Const(Inf));
  Instrs.push_back(makeF64Const(-Inf));
  Instrs.push_back(makeOp(OpCode::F64__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

// --- sNaN through arithmetic → canonical NaN --------------------------------
//
// V8 test: test/mjsunit/wasm/float-constant-folding.js
//   sNaN - 0 = qNaN, sNaN * 1 = qNaN, sNaN / 1 = qNaN.

TEST(ConstFoldTest, V8_F64SNaNSubZero) {
  // f64.sub(sNaN, 0.0) = canonical NaN.
  // sNaN bit pattern for f64: exponent = all ones, fraction bit 51 clear,
  // at least one other fraction bit set.
  uint64_t SNaN = UINT64_C(0x7FF0000000000001); // sNaN payload=1
  double SNaNVal;
  std::memcpy(&SNaNVal, &SNaN, sizeof(SNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(SNaNVal));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__sub));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

TEST(ConstFoldTest, V8_F64SNaNMulOne) {
  // f64.mul(sNaN, 1.0) = canonical NaN.
  uint64_t SNaN = UINT64_C(0x7FF0000000000001);
  double SNaNVal;
  std::memcpy(&SNaNVal, &SNaN, sizeof(SNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(SNaNVal));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
}

// --- Category: Negative zero arithmetic -------------------------------------
//
// V8 test: test/mjsunit/minus-zero.js
//   -0 + -0 = -0, 0 * -1 = -0, etc.

TEST(ConstFoldTest, V8_F64NegZeroAddNegZero) {
  // f64.add(-0.0, -0.0) = -0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeOp(OpCode::F64__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, V8_F64MulZeroNegOne) {
  // f64.mul(0.0, -1.0) = -0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeF64Const(-1.0));
  Instrs.push_back(makeOp(OpCode::F64__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, V8_F64NegNegZero) {
  // f64.neg(-0.0) = +0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeOp(OpCode::F64__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(+0.0));
}

TEST(ConstFoldTest, V8_F64NegPosZero) {
  // f64.neg(+0.0) = -0.0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeOp(OpCode::F64__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

// --- Category: f64 denormal arithmetic (WASM does NOT flush) ----------------
//
// LLVM's IPSCCP flushed denormals to zero during constant folding on targets
// with FTZ mode, but WASM mandates full IEEE 754 denormal support.
//
// Real confirmed bugs:
//   LLVM: "Constant folding ignoring denormal flush-to-zero mode"
//         https://reviews.llvm.org/D116952
//         https://github.com/llvm/llvm-project/issues/100661

TEST(ConstFoldTest, LLVM_F64DenormalArithmetic) {
  // f64.mul(denormal, 0.5) must produce a smaller denormal, not zero.
  double Denorm = std::numeric_limits<double>::denorm_min(); // ~5e-324
  double Half = 0.5;
  double Expected = Denorm * Half; // rounds to 0.0 (underflow to zero)

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(Denorm));
  Instrs.push_back(makeF64Const(Half));
  Instrs.push_back(makeOp(OpCode::F64__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  // Result is 0.0 (underflow), but the fold must match IEEE 754 arithmetic,
  // not a platform FTZ flush.
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(Expected));
}

TEST(ConstFoldTest, LLVM_F64DenormalAdd) {
  // Adding two denormals should produce a denormal, not zero.
  double D1 = std::numeric_limits<double>::denorm_min();
  double D2 = std::numeric_limits<double>::denorm_min();
  double Expected = D1 + D2;

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(D1));
  Instrs.push_back(makeF64Const(D2));
  Instrs.push_back(makeOp(OpCode::F64__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(Expected));
  EXPECT_NE(Expected, 0.0); // Must not be flushed to zero.
}

TEST(ConstFoldTest, LLVM_F32DenormalAdd) {
  // f32 denormal + denormal should not flush to zero.
  float D1 = std::numeric_limits<float>::denorm_min();
  float D2 = std::numeric_limits<float>::denorm_min();
  float Expected = D1 + D2;

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(D1));
  Instrs.push_back(makeF32Const(D2));
  Instrs.push_back(makeOp(OpCode::F32__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), f32Bits(Expected));
  EXPECT_NE(Expected, 0.0f);
}

// --- Category: Trunc boundary precision -------------------------------------
//
// JSC test: JSTests/wasm/stress/trunc-int-min-minus-one.js
//   Tests f64-to-i32 truncation at exact boundary values where the result is
//   defined (just barely within range) vs where it traps (just outside range).

TEST(ConstFoldTest, JSC_TruncI32SignedBoundaryNeg) {
  // i32.trunc_f64_s(-2147483648.0) = -2147483648 (INT32_MIN, exactly representable).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-2147483648.0));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])),
            std::numeric_limits<int32_t>::min());
}

TEST(ConstFoldTest, JSC_TruncI32SignedBoundaryNegFrac) {
  // i32.trunc_f64_s(-2147483648.9) = -2147483648 (truncates toward zero).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-2147483648.9));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])),
            std::numeric_limits<int32_t>::min());
}

TEST(ConstFoldTest, JSC_TruncI32SignedBoundaryPos) {
  // i32.trunc_f64_s(2147483647.9) = 2147483647 (truncates toward zero).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(2147483647.9));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(static_cast<int32_t>(getI32(Instrs[0])),
            std::numeric_limits<int32_t>::max());
}

TEST(ConstFoldTest, JSC_TruncI32SignedOverflowNotFolded) {
  // i32.trunc_f64_s(2147483648.0) = OUT OF RANGE — must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(2147483648.0));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_s));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f64_s);
}

TEST(ConstFoldTest, JSC_TruncI32UnsignedNegPoint9) {
  // i32.trunc_f64_u(-0.9) = 0 (truncates toward zero).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.9));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, JSC_TruncI32UnsignedNegOneNotFolded) {
  // i32.trunc_f64_u(-1.0) = OUT OF RANGE — must NOT fold.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-1.0));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_u));

  Executor::optimizeConstantExpressions(Instrs);

  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__trunc_f64_u);
}

TEST(ConstFoldTest, JSC_TruncI32UnsignedMaxFrac) {
  // i32.trunc_f64_u(4294967295.9) = 4294967295 (UINT32_MAX).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(4294967295.9));
  Instrs.push_back(makeOp(OpCode::I32__trunc_f64_u));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), UINT32_MAX);
}

// --- Category: SIMD f32x4.min/f32x4.max NaN and negative zero ---------------
//
#if !defined(_MSC_VER) || defined(__clang__)
//
// The same JSC BBQ NaN bug (Bug 270262) applies to vector min/max as well.
// Each lane must independently propagate NaN and respect ±0.0 ordering.
//
// V8 test: test/mjsunit/wasm/float-constant-folding.js (scalar, pattern applies)

TEST(ConstFoldTest, V8_F32x4MinNaN) {
  // f32x4.min([NaN, 1.0, NaN, -2.0], [3.0, NaN, -5.0, NaN])
  // = [NaN, NaN, NaN, NaN] — any lane with NaN propagates.
  auto makeV128 = [](uint128_t V) -> AST::Instruction {
    AST::Instruction I(OpCode::V128__const, 0);
    ValVariant Val;
    Val.emplace<uint128_t>(V);
    I.setNum(Val);
    return I;
  };

  // Pack four f32 lanes into uint128_t.
  auto packF32x4 = [](float a, float b, float c, float d) -> uint128_t {
    uint128_t V{0u};
    uint32_t Lanes[4];
    std::memcpy(&Lanes[0], &a, 4);
    std::memcpy(&Lanes[1], &b, 4);
    std::memcpy(&Lanes[2], &c, 4);
    std::memcpy(&Lanes[3], &d, 4);
    std::memcpy(&V, Lanes, 16);
    return V;
  };

  float NaN = std::numeric_limits<float>::quiet_NaN();
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128(packF32x4(NaN, 1.0f, NaN, -2.0f)));
  Instrs.push_back(makeV128(packF32x4(3.0f, NaN, -5.0f, NaN)));
  Instrs.push_back(makeOp(OpCode::F32x4__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint128_t R = Instrs[0].getNum().get<uint128_t>();
  float Lanes[4];
  std::memcpy(Lanes, &R, 16);
  for (int i = 0; i < 4; ++i)
    EXPECT_TRUE(std::isnan(Lanes[i])) << "Lane " << i << " should be NaN";
}

TEST(ConstFoldTest, V8_F32x4MaxNegZero) {
  // f32x4.max([-0, +0, -0, +0], [+0, -0, +0, -0]) = [+0, +0, +0, +0].
  auto packF32x4 = [](float a, float b, float c, float d) -> uint128_t {
    uint128_t V{0u};
    uint32_t Lanes[4];
    std::memcpy(&Lanes[0], &a, 4);
    std::memcpy(&Lanes[1], &b, 4);
    std::memcpy(&Lanes[2], &c, 4);
    std::memcpy(&Lanes[3], &d, 4);
    std::memcpy(&V, Lanes, 16);
    return V;
  };

  auto makeV128 = [](uint128_t V) -> AST::Instruction {
    AST::Instruction I(OpCode::V128__const, 0);
    ValVariant Val;
    Val.emplace<uint128_t>(V);
    I.setNum(Val);
    return I;
  };

  AST::InstrVec Instrs;
  Instrs.push_back(makeV128(packF32x4(-0.0f, +0.0f, -0.0f, +0.0f)));
  Instrs.push_back(makeV128(packF32x4(+0.0f, -0.0f, +0.0f, -0.0f)));
  Instrs.push_back(makeOp(OpCode::F32x4__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint128_t R = Instrs[0].getNum().get<uint128_t>();
  uint32_t Lanes[4];
  std::memcpy(Lanes, &R, 16);
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(Lanes[i], f32Bits(+0.0f))
        << "Lane " << i << " should be +0.0, not -0.0";
}

#endif // !defined(_MSC_VER) || defined(__clang__)

// --- Category: Algebraic chain miscompilation (LLVM #96366) -----------------
//
// LLVM's DAG combiner incorrectly folded (1-x) + (x*1) + (3-x) to constant 4
// for all inputs.  With x=9 the correct result is (1-9)+(9*1)+(3-9) = -5.
//
// Real confirmed bug:
//   LLVM: "Arithmetic miscompile from constant folding in SelectionDAG"
//         https://github.com/llvm/llvm-project/issues/96366

TEST(ConstFoldTest, LLVM_AlgebraicChainNotCollapsed) {
  // (1-x) + (x*1) + (3-x) with x = 9:
  //   (1-9) + (9*1) + (3-9) = -8 + 9 + -6 = -5.
  // A wrong fold might simplify to (1+3) = 4 by cancelling x terms.
  //
  // Instruction sequence:
  //   i32.const 1, i32.const 9, i32.sub,         → -8
  //   i32.const 9, i32.const 1, i32.mul,          → 9
  //   i32.add,                                      → 1
  //   i32.const 3, i32.const 9, i32.sub,          → -6
  //   i32.add                                       → -5
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeI32Const(9));
  Instrs.push_back(makeOp(OpCode::I32__sub));
  Instrs.push_back(makeI32Const(9));
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeOp(OpCode::I32__mul));
  Instrs.push_back(makeOp(OpCode::I32__add));
  Instrs.push_back(makeI32Const(3));
  Instrs.push_back(makeI32Const(9));
  Instrs.push_back(makeOp(OpCode::I32__sub));
  Instrs.push_back(makeOp(OpCode::I32__add));

  Executor::optimizeConstantExpressions(Instrs);

  // Find the surviving i32.const.
  const AST::Instruction *Result = nullptr;
  for (const auto &I : Instrs) {
    if (I.getOpCode() == OpCode::I32__const) {
      Result = &I;
      break;
    }
  }
  ASSERT_NE(Result, nullptr);
  EXPECT_EQ(static_cast<int32_t>(Result->getNum().get<uint32_t>()), -5);
}

// ===========================================================================
// Tests derived from Meta's Hermes JavaScript engine (bytecode-only, no JIT).
// Hermes performs all optimizations at the bytecode IR level — the closest
// analogy to a WASM interpreter constant-folding pass.  Bugs found in Hermes
// are directly relevant because the same IR-level folding patterns apply.
// ===========================================================================

// --- Category: f64.neg / f64.abs on NaN (Hermes Issue #1639) ----------------
//
// Hermes found that writing -NaN to a Float32Array produced different bit
// patterns at different optimization levels.  WASM spec §4.3.3 requires:
//   fneg: flips the sign bit (even for NaN)
//   fabs: clears the sign bit (even for NaN)
// These are bitwise operations, NOT arithmetic — they must preserve the NaN
// payload and only affect bit 63 (f64) or bit 31 (f32).
//
// Real confirmed bug:
//   Hermes: "NaN canonicalization differs between optimization levels"
//           https://github.com/facebook/hermes/issues/1639
//   Hermes: "NaN canonicalization differs between Intel and ARM"
//           https://github.com/facebook/hermes/issues/1110

TEST(ConstFoldTest, Hermes_F64NegNaN) {
  // f64.neg(+qNaN) must flip the sign bit → -qNaN.
  // Canonical +qNaN = 0x7FF8000000000000 → neg → 0xFFF8000000000000.
  uint64_t PosNaN = UINT64_C(0x7FF8000000000000);
  uint64_t NegNaN = UINT64_C(0xFFF8000000000000);
  double PosNaNVal;
  std::memcpy(&PosNaNVal, &PosNaN, sizeof(PosNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(PosNaNVal));
  Instrs.push_back(makeOp(OpCode::F64__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), NegNaN);
}

TEST(ConstFoldTest, Hermes_F64NegNegNaN) {
  // f64.neg(-qNaN) must flip the sign bit → +qNaN.
  uint64_t NegNaN = UINT64_C(0xFFF8000000000000);
  uint64_t PosNaN = UINT64_C(0x7FF8000000000000);
  double NegNaNVal;
  std::memcpy(&NegNaNVal, &NegNaN, sizeof(NegNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(NegNaNVal));
  Instrs.push_back(makeOp(OpCode::F64__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), PosNaN);
}

TEST(ConstFoldTest, Hermes_F64AbsNegNaN) {
  // f64.abs(-qNaN) must clear the sign bit → +qNaN.
  uint64_t NegNaN = UINT64_C(0xFFF8000000000000);
  uint64_t PosNaN = UINT64_C(0x7FF8000000000000);
  double NegNaNVal;
  std::memcpy(&NegNaNVal, &NegNaN, sizeof(NegNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(NegNaNVal));
  Instrs.push_back(makeOp(OpCode::F64__abs));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), PosNaN);
}

TEST(ConstFoldTest, Hermes_F32NegNaN) {
  // f32.neg(+qNaN) must flip the sign bit.
  uint32_t PosNaN = UINT32_C(0x7FC00000);
  uint32_t NegNaN = UINT32_C(0xFFC00000);
  float PosNaNVal;
  std::memcpy(&PosNaNVal, &PosNaN, sizeof(PosNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(PosNaNVal));
  Instrs.push_back(makeOp(OpCode::F32__neg));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), NegNaN);
}

TEST(ConstFoldTest, Hermes_F32AbsNegNaN) {
  // f32.abs(-qNaN) must clear the sign bit → +qNaN.
  uint32_t NegNaN = UINT32_C(0xFFC00000);
  uint32_t PosNaN = UINT32_C(0x7FC00000);
  float NegNaNVal;
  std::memcpy(&NegNaNVal, &NegNaN, sizeof(NegNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(NegNaNVal));
  Instrs.push_back(makeOp(OpCode::F32__abs));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), PosNaN);
}

// --- Category: NaN self-comparison (Hermes Issue #1200) ---------------------
//
// Hermes InstSimplify incorrectly folded `v0 <= v0` to true when the value
// could be NaN (through an `undefined|string` union type).  The fix:
// self-comparisons on floats must never assume x == x (NaN != NaN).
//
// Real confirmed bug:
//   Hermes: "InstSimplify wrong result -- undefined in relational comparisons"
//           https://github.com/facebook/hermes/issues/1200
//   Fix:    https://github.com/facebook/hermes/pull/1202

TEST(ConstFoldTest, Hermes_F64SelfEqNaN) {
  // f64.eq(NaN, NaN) = 0.  NaN is not equal to itself.
  // A naive optimizer might fold "x == x" to true for same-SSA-value operands.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__eq));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, Hermes_F64SelfNeNaN) {
  // f64.ne(NaN, NaN) = 1.  NaN is unequal to itself.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__ne));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 1u);
}

// --- Category: Negative zero through IEEE 754 identity ops (Hermes #1215) ---
//
// Hermes SimplifySwitchInst used pointer equality for literal comparison,
// treating -0 and +0 as different values.  For us, the concern is that
// constant folding chains must preserve or correctly eliminate -0.0.
//
// IEEE 754 rules:
//   -0.0 + 0.0 = +0.0  (positive zero dominates in addition)
//   -0.0 * 1.0 = -0.0  (sign preserved in multiplication)
//   -0.0 / 1.0 = -0.0  (sign preserved in division)
//
// Real confirmed bug:
//   Hermes: "SimplifySwitchInst ignores negative zero"
//           https://github.com/facebook/hermes/issues/1215
//
// Hermes test: test/Optimizer/simplify.js

TEST(ConstFoldTest, Hermes_NegZeroAddPosZero) {
  // f64.add(-0.0, +0.0) = +0.0 (IEEE 754: positive zero wins).
  // A wrong fold that preserves -0 from the first operand would fail.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(+0.0));
  Instrs.push_back(makeOp(OpCode::F64__add));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(+0.0));
}

TEST(ConstFoldTest, Hermes_NegZeroDivPositive) {
  // f64.div(-0.0, 4.0) = -0.0 (sign preserved in division).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(4.0));
  Instrs.push_back(makeOp(OpCode::F64__div));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, Hermes_NegZeroMulOne) {
  // f64.mul(-0.0, 1.0) = -0.0 (sign preserved in multiplication).
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__mul));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, Hermes_NegZeroChain) {
  // f64.add(f64.mul(-0.0, 1.0), 0.0):
  //   mul(-0.0, 1.0) = -0.0
  //   add(-0.0, +0.0) = +0.0
  // The chain must correctly propagate then eliminate negative zero.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(-0.0));
  Instrs.push_back(makeF64Const(1.0));
  Instrs.push_back(makeOp(OpCode::F64__mul));    // → -0.0
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__add));     // → +0.0

  Executor::optimizeConstantExpressions(Instrs);

  // After folding, should collapse to a single f64.const(+0.0).
  const AST::Instruction *Result = nullptr;
  for (const auto &I : Instrs) {
    if (I.getOpCode() == OpCode::F64__const) {
      Result = &I;
      break;
    }
  }
  ASSERT_NE(Result, nullptr);
  EXPECT_EQ(f64Bits(Result->getNum().get<double>()), f64Bits(+0.0));
}

// --- Category: f64.copysign with NaN (WASM spec §4.3.3) --------------------
//
// f64.copysign produces a value with the magnitude of the first operand and
// the sign of the second.  For NaN inputs, this must work on the raw bits
// (flip/copy bit 63), NOT produce canonical NaN.

TEST(ConstFoldTest, Hermes_F64CopysignNaN) {
  // f64.copysign(+qNaN, -1.0) = -qNaN (copy the sign bit of -1.0).
  uint64_t PosNaN = UINT64_C(0x7FF8000000000000);
  uint64_t NegNaN = UINT64_C(0xFFF8000000000000);
  double PosNaNVal;
  std::memcpy(&PosNaNVal, &PosNaN, sizeof(PosNaNVal));

  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(PosNaNVal));
  Instrs.push_back(makeF64Const(-1.0));
  Instrs.push_back(makeOp(OpCode::F64__copysign));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), NegNaN);
}

// --- Category: i32.shl sign-bit edge case (Hermes commit fb2a848) -----------
//
// Hermes fixed `1 << 31` causing C++ signed integer UB.  In WASM,
// i32.shl(1, 31) = 0x80000000 = INT32_MIN.  Must fold correctly using
// unsigned arithmetic internally.
//
// Real confirmed fix:
//   Hermes: "Fix signed shift error (UB from 1 << 31)"
//           Commit fb2a8481213400528acee7fd16f6069f0205af81

TEST(ConstFoldTest, Hermes_I32ShlSignBit) {
  // i32.shl(1, 31) = 0x80000000.  Must not invoke C++ signed UB.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeI32Const(31));
  Instrs.push_back(makeOp(OpCode::I32__shl));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0x80000000u);
}

// --- Category: Cross-architecture NaN canonicalization (Hermes #1110) -------
//
// Hermes found that Intel produces 0x7FF8000000000000 and ARM produces
// 0xFFF8000000000000 for the same NaN operation.  Our f64.min/f64.max
// explicitly produce the canonical positive quiet NaN; verify the exact bits.
//
// Real confirmed bug:
//   Hermes: "NaN canonicalization differs between Intel and ARM"
//           https://github.com/facebook/hermes/issues/1110

TEST(ConstFoldTest, Hermes_F64MinCanonicalNaNBits) {
  // f64.min(NaN, 0.0) must produce EXACTLY 0x7FF8000000000000.
  // Not 0xFFF8000000000000 (negative NaN), and not some other payload.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeOp(OpCode::F64__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, Hermes_F64MaxCanonicalNaNBits) {
  // f64.max(0.0, NaN) must produce EXACTLY 0x7FF8000000000000.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF64Const(0.0));
  Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
  Instrs.push_back(makeOp(OpCode::F64__max));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), kF64CanonicalNaN);
}

TEST(ConstFoldTest, Hermes_F32MinCanonicalNaNBits) {
  // f32.min(NaN, 0.0f) must produce EXACTLY 0x7FC00000.
  AST::InstrVec Instrs;
  Instrs.push_back(makeF32Const(std::numeric_limits<float>::quiet_NaN()));
  Instrs.push_back(makeF32Const(0.0f));
  Instrs.push_back(makeOp(OpCode::F32__min));

  Executor::optimizeConstantExpressions(Instrs);

  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const);
  EXPECT_EQ(f32BitsFromInstr(Instrs[0]), kF32CanonicalNaN);
}

// ===========================================================================
// Coverage gap: i64 rotate ops (i64.rotl / i64.rotr)
//
// Wasmer/LLVM had a bug where i64.rotr by 0 returned -1 instead of the
// identity value, because the LLVM lowering used (value >> 64) which is UB.
//   https://github.com/wasmerio/wasmer/issues/2215
//   https://github.com/wasmerio/wasmer/issues/2143
// ===========================================================================

TEST(ConstFoldTest, I64RotlBasic) {
  // i64.rotl(0x0123456789ABCDEF, 8) = 0x23456789ABCDEF01
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0x0123456789ABCDEF)));
  Instrs.push_back(makeI64Const(8));
  Instrs.push_back(makeOp(OpCode::I64__rotl));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0x23456789ABCDEF01));
}

TEST(ConstFoldTest, I64RotrBasic) {
  // i64.rotr(0x0123456789ABCDEF, 8) = 0xEF0123456789ABCD
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0x0123456789ABCDEF)));
  Instrs.push_back(makeI64Const(8));
  Instrs.push_back(makeOp(OpCode::I64__rotr));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xEF0123456789ABCD));
}

TEST(ConstFoldTest, I64RotrByZero) {
  // i64.rotr(4, 0) = 4.  Wasmer/LLVM bug: returned -1 due to shift-by-64 UB.
  // https://github.com/wasmerio/wasmer/issues/2215
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(4));
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__rotr));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 4ull);
}

TEST(ConstFoldTest, I64RotlByZero) {
  // i64.rotl(4, 0) = 4 (identity).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(4));
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__rotl));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 4ull);
}

TEST(ConstFoldTest, I64RotrBy64) {
  // i64.rotr(X, 64): WASM masks 64 & 63 = 0, so identity.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0xDEADBEEFCAFEBABE)));
  Instrs.push_back(makeI64Const(64));
  Instrs.push_back(makeOp(OpCode::I64__rotr));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xDEADBEEFCAFEBABE));
}

TEST(ConstFoldTest, I32RotlBasic) {
  // i32.rotl(0x12345678, 4) = 0x23456781
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x12345678u));
  Instrs.push_back(makeI32Const(4));
  Instrs.push_back(makeOp(OpCode::I32__rotl));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0x23456781u);
}

TEST(ConstFoldTest, I32RotrByZero) {
  // i32.rotr(42, 0) = 42 (identity).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(42));
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__rotr));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 42u);
}

// ===========================================================================
// Coverage gap: i64 sign-extension ops (extend8_s / extend16_s / extend32_s)
//
// These test boundary conditions at the sign bit of each narrow width.
// The AArch64 sign-vs-zero-extension CVE in Wasmtime (GHSA-7f6x-jwh5-m9r4)
// demonstrates this class of bug.
//   https://github.com/bytecodealliance/wasmtime/security/advisories/GHSA-7f6x-jwh5-m9r4
// ===========================================================================

TEST(ConstFoldTest, I64Extend8s_Positive) {
  // i64.extend8_s(0x7F) = 0x7F (sign bit clear → no extension).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x7F));
  Instrs.push_back(makeOp(OpCode::I64__extend8_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0x7Full);
}

TEST(ConstFoldTest, I64Extend8s_Negative) {
  // i64.extend8_s(0x80) = 0xFFFFFFFFFFFFFF80 (sign bit set → fill with 1s).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x80));
  Instrs.push_back(makeOp(OpCode::I64__extend8_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xFFFFFFFFFFFFFF80));
}

TEST(ConstFoldTest, I64Extend8s_AllOnes) {
  // i64.extend8_s(0xFF) = 0xFFFFFFFFFFFFFFFF (-1 sign-extended).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0xFF));
  Instrs.push_back(makeOp(OpCode::I64__extend8_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_MAX);
}

TEST(ConstFoldTest, I64Extend16s_Positive) {
  // i64.extend16_s(0x7FFF) = 0x7FFF.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x7FFF));
  Instrs.push_back(makeOp(OpCode::I64__extend16_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 0x7FFFull);
}

TEST(ConstFoldTest, I64Extend16s_Negative) {
  // i64.extend16_s(0x8000) = 0xFFFFFFFFFFFF8000.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x8000));
  Instrs.push_back(makeOp(OpCode::I64__extend16_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xFFFFFFFFFFFF8000));
}

TEST(ConstFoldTest, I64Extend32s_Positive) {
  // i64.extend32_s(0x7FFFFFFF) = 0x000000007FFFFFFF.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x7FFFFFFF));
  Instrs.push_back(makeOp(OpCode::I64__extend32_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0x000000007FFFFFFF));
}

TEST(ConstFoldTest, I64Extend32s_Negative) {
  // i64.extend32_s(0x80000000) = 0xFFFFFFFF80000000.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0x80000000));
  Instrs.push_back(makeOp(OpCode::I64__extend32_s));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), UINT64_C(0xFFFFFFFF80000000));
}

// ===========================================================================
// Coverage gap: f64.nearest (roundTiesToEven) and f32.nearest
//
// IEEE 754 ties-to-even: 0.5→0, 1.5→2, 2.5→2, 3.5→4
// Also covers NaN/±Inf/±0 pass-through and large values.
// ===========================================================================

TEST(ConstFoldTest, F64NearestTiesToEven) {
  // Ties-to-even: round to nearest even integer.
  struct { double In; double Out; } Cases[] = {
    {0.5, 0.0},    // tie → even (0)
    {1.5, 2.0},    // tie → even (2)
    {2.5, 2.0},    // tie → even (2)
    {3.5, 4.0},    // tie → even (4)
    {-0.5, -0.0},  // negative tie → even (0)
    {-1.5, -2.0},  // negative tie → even (-2)
    {-2.5, -2.0},  // negative tie → even (-2)
    {4.3, 4.0},    // normal round down
    {4.7, 5.0},    // normal round up
    {1e17, 1e17},   // large value (already integer)
  };
  for (const auto &C : Cases) {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF64Const(C.In));
    Instrs.push_back(makeOp(OpCode::F64__nearest));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const)
        << "input: " << C.In;
    EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(C.Out))
        << "input: " << C.In;
  }
}

TEST(ConstFoldTest, F64NearestSpecialValues) {
  // NaN → NaN, ±Inf → ±Inf, ±0 → ±0
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF64Const(std::numeric_limits<double>::quiet_NaN()));
    Instrs.push_back(makeOp(OpCode::F64__nearest));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
    EXPECT_TRUE(std::isnan(getF64(Instrs[0])));
  }
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF64Const(std::numeric_limits<double>::infinity()));
    Instrs.push_back(makeOp(OpCode::F64__nearest));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(getF64(Instrs[0]), std::numeric_limits<double>::infinity());
  }
  {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF64Const(-0.0));
    Instrs.push_back(makeOp(OpCode::F64__nearest));
    Executor::optimizeConstantExpressions(Instrs);
    EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
  }
}

TEST(ConstFoldTest, F32NearestTiesToEven) {
  struct { float In; float Out; } Cases[] = {
    {0.5f, 0.0f}, {1.5f, 2.0f}, {2.5f, 2.0f}, {3.5f, 4.0f},
    {-0.5f, -0.0f}, {-1.5f, -2.0f}, {4.3f, 4.0f}, {4.7f, 5.0f},
  };
  for (const auto &C : Cases) {
    AST::InstrVec Instrs;
    Instrs.push_back(makeF32Const(C.In));
    Instrs.push_back(makeOp(OpCode::F32__nearest));
    Executor::optimizeConstantExpressions(Instrs);
    ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F32__const)
        << "input: " << C.In;
    EXPECT_EQ(f32BitsFromInstr(Instrs[0]), f32Bits(C.Out))
        << "input: " << C.In;
  }
}

// ===========================================================================
// Coverage gap: f64.reinterpret_i64
// ===========================================================================

TEST(ConstFoldTest, F64ReinterpretI64) {
  // f64.reinterpret_i64(0x4000000000000000) = 2.0
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0x4000000000000000)));
  Instrs.push_back(makeOp(OpCode::F64__reinterpret_i64));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_DOUBLE_EQ(getF64(Instrs[0]), 2.0);
}

TEST(ConstFoldTest, F64ReinterpretI64_NegZero) {
  // f64.reinterpret_i64(0x8000000000000000) = -0.0
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_C(0x8000000000000000)));
  Instrs.push_back(makeOp(OpCode::F64__reinterpret_i64));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), f64Bits(-0.0));
}

TEST(ConstFoldTest, F64ReinterpretI64_NaN) {
  // f64.reinterpret_i64(0x7FF0000000000001) = sNaN (payload preserved).
  uint64_t SNaN = UINT64_C(0x7FF0000000000001);
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(SNaN));
  Instrs.push_back(makeOp(OpCode::F64__reinterpret_i64));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::F64__const);
  EXPECT_EQ(f64BitsFromInstr(Instrs[0]), SNaN);
}

// ===========================================================================
// Coverage gap: uncommon scalar ops (clz, ctz, popcnt)
//
// clz(0) is UB in C but well-defined in WASM (returns bitwidth).
//   LLVM compiler-rt bug: https://github.com/llvm/llvm-project/issues/167620
//   Wasmtime clz bug: https://github.com/bytecodealliance/wasmtime/issues/1448
// ===========================================================================

TEST(ConstFoldTest, I32ClzZero) {
  // i32.clz(0) = 32.  C __builtin_clz(0) is UB; WASM is well-defined.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__clz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 32u);
}

TEST(ConstFoldTest, I32ClzOne) {
  // i32.clz(1) = 31.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(1));
  Instrs.push_back(makeOp(OpCode::I32__clz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 31u);
}

TEST(ConstFoldTest, I32ClzHighBit) {
  // i32.clz(0x80000000) = 0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x80000000u));
  Instrs.push_back(makeOp(OpCode::I32__clz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, I32CtzZero) {
  // i32.ctz(0) = 32.  C __builtin_ctz(0) is UB; WASM is well-defined.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__ctz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 32u);
}

TEST(ConstFoldTest, I32CtzLowBit) {
  // i32.ctz(0x80000000) = 31.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0x80000000u));
  Instrs.push_back(makeOp(OpCode::I32__ctz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 31u);
}

TEST(ConstFoldTest, I32PopcntPattern) {
  // i32.popcnt(0xDEADBEEF) = 24 (count of set bits).
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0xDEADBEEFu));
  Instrs.push_back(makeOp(OpCode::I32__popcnt));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 24u);
}

TEST(ConstFoldTest, I32PopcntZeroInput) {
  // i32.popcnt(0) = 0.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI32Const(0));
  Instrs.push_back(makeOp(OpCode::I32__popcnt));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(getI32(Instrs[0]), 0u);
}

TEST(ConstFoldTest, I64ClzZero) {
  // i64.clz(0) = 64.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__clz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 64ull);
}

TEST(ConstFoldTest, I64CtzZero) {
  // i64.ctz(0) = 64.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(0));
  Instrs.push_back(makeOp(OpCode::I64__ctz));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 64ull);
}

TEST(ConstFoldTest, I64Popcnt) {
  // i64.popcnt(UINT64_MAX) = 64.
  AST::InstrVec Instrs;
  Instrs.push_back(makeI64Const(UINT64_MAX));
  Instrs.push_back(makeOp(OpCode::I64__popcnt));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::I64__const);
  EXPECT_EQ(getI64(Instrs[0]), 64ull);
}

// ===========================================================================
// Coverage gap: SIMD pmin/pmax NaN semantics
//
// pmin/pmax use C comparison semantics (a < b ? a : b), NOT IEEE min/max.
// For NaN: NaN < x is false, so pmin(NaN, x) returns NaN (first operand).
// This is the OPPOSITE of min/max which returns canonical NaN.
//
// SpiderMonkey had reversed MINPS operand order, causing wrong NaN behavior:
//   https://bugzilla.mozilla.org/show_bug.cgi?id=1669964
// ===========================================================================

#if !defined(_MSC_VER) || defined(__clang__)

TEST(ConstFoldTest, F64x2PMinNaN_Lhs) {
  // f64x2.pmin([NaN, 1.0], [2.0, 3.0]):
  //   lane 0: NaN < 2.0 is false → result = NaN (first operand)
  //   lane 1: 1.0 < 3.0 is true  → result = 1.0 (first operand)
  AST::InstrVec Instrs;
  double NaN = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF64x2(NaN, 1.0)));
  Instrs.push_back(makeV128Const(makeF64x2(2.0, 3.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__pmin));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_TRUE(std::isnan(Out[0]));
  EXPECT_DOUBLE_EQ(Out[1], 1.0);
}

TEST(ConstFoldTest, F64x2PMaxNaN_Rhs) {
  // f64x2.pmax([1.0, 2.0], [NaN, 0.5]):
  //   lane 0: 1.0 < NaN is false → result = 1.0 (first operand)
  //   lane 1: 2.0 < 0.5 is false → result = 2.0 (first operand)
  AST::InstrVec Instrs;
  double NaN = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF64x2(1.0, 2.0)));
  Instrs.push_back(makeV128Const(makeF64x2(NaN, 0.5)));
  Instrs.push_back(makeOp(OpCode::F64x2__pmax));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_DOUBLE_EQ(Out[0], 1.0);
  EXPECT_DOUBLE_EQ(Out[1], 2.0);
}

TEST(ConstFoldTest, F32x4PMinNegZero) {
  // f32x4.pmin(a, b) = b < a ? b : a.
  //   a = [-0.0, 0.0, 1.0, -1.0],  b = [0.0, -0.0, -1.0, 1.0]:
  //   lane 0: 0.0 < -0.0 is false  → a = -0.0
  //   lane 1: -0.0 < 0.0 is false  → a = 0.0
  //   lane 2: -1.0 < 1.0 is true   → b = -1.0
  //   lane 3: 1.0 < -1.0 is false  → a = -1.0
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(-0.0f, 0.0f, 1.0f, -1.0f)));
  Instrs.push_back(makeV128Const(makeF32x4(0.0f, -0.0f, -1.0f, 1.0f)));
  Instrs.push_back(makeOp(OpCode::F32x4__pmin));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_EQ(f32Bits(Out[0]), f32Bits(-0.0f));
  EXPECT_EQ(f32Bits(Out[1]), f32Bits(0.0f));
  EXPECT_FLOAT_EQ(Out[2], -1.0f);
  EXPECT_FLOAT_EQ(Out[3], -1.0f);
}

// ===========================================================================
// Coverage gap: SIMD f64x2 division (including div-by-zero → ±Inf)
//
// Unlike scalar i32.div which traps on div-by-zero, SIMD float division
// produces ±Inf per IEEE 754.  This must fold correctly.
// ===========================================================================

TEST(ConstFoldTest, F64x2DivBasic) {
  // f64x2.div([10.0, -6.0], [2.0, 3.0]) = [5.0, -2.0]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF64x2(10.0, -6.0)));
  Instrs.push_back(makeV128Const(makeF64x2(2.0, 3.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__div));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_DOUBLE_EQ(Out[0], 5.0);
  EXPECT_DOUBLE_EQ(Out[1], -2.0);
}

TEST(ConstFoldTest, F64x2DivByZero) {
  // f64x2.div([1.0, -1.0], [0.0, 0.0]) = [+Inf, -Inf]
  // IEEE 754 float div-by-zero → Inf, NOT a trap.
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF64x2(1.0, -1.0)));
  Instrs.push_back(makeV128Const(makeF64x2(0.0, 0.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__div));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_EQ(Out[0], std::numeric_limits<double>::infinity());
  EXPECT_EQ(Out[1], -std::numeric_limits<double>::infinity());
}

TEST(ConstFoldTest, F64x2DivNaN) {
  // f64x2.div([0.0, NaN], [0.0, 1.0]) = [NaN, NaN]
  // 0/0 = NaN; NaN/1 = NaN.
  AST::InstrVec Instrs;
  double NaN = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF64x2(0.0, NaN)));
  Instrs.push_back(makeV128Const(makeF64x2(0.0, 1.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__div));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_TRUE(std::isnan(Out[0]));
  EXPECT_TRUE(std::isnan(Out[1]));
}

// ===========================================================================
// Coverage gap: SIMD min/max NaN (canonical NaN, not pmin/pmax semantics)
//
// f64x2.min/max must produce canonical NaN when either lane is NaN.
// SpiderMonkey had a bug where the NaN was not canonical:
//   https://bugzilla.mozilla.org/show_bug.cgi?id=1647288
// ===========================================================================

TEST(ConstFoldTest, F64x2MinNaN) {
  // f64x2.min([NaN, 1.0], [2.0, NaN]) = [canonical_NaN, canonical_NaN]
  AST::InstrVec Instrs;
  double NaN = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF64x2(NaN, 1.0)));
  Instrs.push_back(makeV128Const(makeF64x2(2.0, NaN)));
  Instrs.push_back(makeOp(OpCode::F64x2__min));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_TRUE(std::isnan(Out[0]));
  EXPECT_TRUE(std::isnan(Out[1]));
}

TEST(ConstFoldTest, F64x2MaxNaN) {
  // f64x2.max([1.0, NaN], [NaN, 2.0]) = [canonical_NaN, canonical_NaN]
  AST::InstrVec Instrs;
  double NaN = std::numeric_limits<double>::quiet_NaN();
  Instrs.push_back(makeV128Const(makeF64x2(1.0, NaN)));
  Instrs.push_back(makeV128Const(makeF64x2(NaN, 2.0)));
  Instrs.push_back(makeOp(OpCode::F64x2__max));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  double Out[2];
  extractF64x2(getV128(Instrs[0]), Out);
  EXPECT_TRUE(std::isnan(Out[0]));
  EXPECT_TRUE(std::isnan(Out[1]));
}

// ===========================================================================
// Coverage gap: more SIMD binary ops (i8x16, i16x8, i64x2, f32x4)
// ===========================================================================

TEST(ConstFoldTest, I64x2Sub) {
  // i64x2.sub([100, 200], [30, 50]) = [70, 150]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI64x2(100, 200)));
  Instrs.push_back(makeV128Const(makeI64x2(30, 50)));
  Instrs.push_back(makeOp(OpCode::I64x2__sub));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint64_t L[2];
  uint128_t R = getV128(Instrs[0]);
  std::memcpy(L, &R, 16);
  EXPECT_EQ(L[0], 70ull);
  EXPECT_EQ(L[1], 150ull);
}

TEST(ConstFoldTest, I64x2Mul) {
  // i64x2.mul([7, 11], [3, 5]) = [21, 55]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeI64x2(7, 11)));
  Instrs.push_back(makeV128Const(makeI64x2(3, 5)));
  Instrs.push_back(makeOp(OpCode::I64x2__mul));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  uint64_t L[2];
  uint128_t R = getV128(Instrs[0]);
  std::memcpy(L, &R, 16);
  EXPECT_EQ(L[0], 21ull);
  EXPECT_EQ(L[1], 55ull);
}

TEST(ConstFoldTest, F32x4Sub) {
  // f32x4.sub([10, 20, 30, 40], [1, 2, 3, 4]) = [9, 18, 27, 36]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(10, 20, 30, 40)));
  Instrs.push_back(makeV128Const(makeF32x4(1, 2, 3, 4)));
  Instrs.push_back(makeOp(OpCode::F32x4__sub));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 9.0f);
  EXPECT_FLOAT_EQ(Out[1], 18.0f);
  EXPECT_FLOAT_EQ(Out[2], 27.0f);
  EXPECT_FLOAT_EQ(Out[3], 36.0f);
}

TEST(ConstFoldTest, F32x4Div) {
  // f32x4.div([10, 1, -1, 0], [2, 0, 0, 0]) = [5, +Inf, -Inf, NaN]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(10, 1, -1, 0)));
  Instrs.push_back(makeV128Const(makeF32x4(2, 0, 0, 0)));
  Instrs.push_back(makeOp(OpCode::F32x4__div));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 5.0f);
  EXPECT_EQ(Out[1], std::numeric_limits<float>::infinity());
  EXPECT_EQ(Out[2], -std::numeric_limits<float>::infinity());
  EXPECT_TRUE(std::isnan(Out[3])); // 0/0 = NaN
}

TEST(ConstFoldTest, F32x4Mul) {
  // f32x4.mul([2, 3, 4, 5], [10, 20, 30, 40]) = [20, 60, 120, 200]
  AST::InstrVec Instrs;
  Instrs.push_back(makeV128Const(makeF32x4(2, 3, 4, 5)));
  Instrs.push_back(makeV128Const(makeF32x4(10, 20, 30, 40)));
  Instrs.push_back(makeOp(OpCode::F32x4__mul));
  Executor::optimizeConstantExpressions(Instrs);
  ASSERT_EQ(Instrs[0].getOpCode(), OpCode::V128__const);
  float Out[4];
  extractF32x4(getV128(Instrs[0]), Out);
  EXPECT_FLOAT_EQ(Out[0], 20.0f);
  EXPECT_FLOAT_EQ(Out[1], 60.0f);
  EXPECT_FLOAT_EQ(Out[2], 120.0f);
  EXPECT_FLOAT_EQ(Out[3], 200.0f);
}

#endif // !defined(_MSC_VER) || defined(__clang__)
