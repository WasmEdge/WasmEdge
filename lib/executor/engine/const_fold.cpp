// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/engine/const_fold.h"

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/enum_ast.hpp"
#include "common/enum_types.hpp"
#include "common/roundeven.h"
#include "common/types.h"

// SIMD constant-folding helpers use the [[gnu::vector_size(16)]] attribute for
// GCC-style lane-wise vector operations and uint128_t (via __int128).  Pure
// MSVC (cl.exe) supports neither — it falls back to std::array<T, lanes> in
// the runtime dispatch path (see unary_numeric_vector_msvc.ipp), but those
// array-based types are not compatible with the free-function templates in
// simd_ops.h.  Clang-CL (MSVC ABI with Clang codegen) DOES support both, so
// we include simd_ops.h whenever Clang is available.  On pure MSVC builds,
// SIMD operations are still executed correctly at runtime — they simply cannot
// be constant-folded at load time.  This matches the existing WasmEdge
// convention in binary_numeric_vector.ipp and unary_numeric_vector.ipp.
#if !defined(_MSC_VER) || defined(__clang__)
#include "executor/engine/simd_ops.h"
#endif

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace WasmEdge {
namespace Executor {

namespace {

// ---------------------------------------------------------------------------
// Classification helpers
// ---------------------------------------------------------------------------

bool isConstOp(OpCode Code) noexcept {
  switch (Code) {
  case OpCode::I32__const:
  case OpCode::I64__const:
  case OpCode::F32__const:
  case OpCode::F64__const:
  case OpCode::V128__const:
    return true;
  default:
    return false;
  }
}

/// Return the const opcode for the result type of a binary/relational op.
/// Relational ops always produce i32. Returns OpCode::End as sentinel if
/// the opcode is not a recognized foldable binary or relational op.
OpCode resultConstOp(OpCode Code) noexcept {
  switch (Code) {
  // I32 binary arithmetic
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
    return OpCode::I32__const;

  // I64 binary arithmetic
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
  case OpCode::I64__and:
  case OpCode::I64__or:
  case OpCode::I64__xor:
  case OpCode::I64__shl:
  case OpCode::I64__shr_s:
  case OpCode::I64__shr_u:
  case OpCode::I64__rotl:
  case OpCode::I64__rotr:
    return OpCode::I64__const;

  // F32 binary arithmetic
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
    return OpCode::F32__const;

  // F64 binary arithmetic
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return OpCode::F64__const;

  // All relational ops produce i32 (0 or 1).
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I32__lt_s:
  case OpCode::I32__lt_u:
  case OpCode::I32__gt_s:
  case OpCode::I32__gt_u:
  case OpCode::I32__le_s:
  case OpCode::I32__le_u:
  case OpCode::I32__ge_s:
  case OpCode::I32__ge_u:
  case OpCode::I64__eq:
  case OpCode::I64__ne:
  case OpCode::I64__lt_s:
  case OpCode::I64__lt_u:
  case OpCode::I64__gt_s:
  case OpCode::I64__gt_u:
  case OpCode::I64__le_s:
  case OpCode::I64__le_u:
  case OpCode::I64__ge_s:
  case OpCode::I64__ge_u:
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:
    return OpCode::I32__const;

  // V128 bitwise operations (both operands are V128__const)
  case OpCode::V128__and:
  case OpCode::V128__andnot:
  case OpCode::V128__or:
  case OpCode::V128__xor:
  // I8x16 integer arithmetic (wrap-around, no traps)
  case OpCode::I8x16__add:
  case OpCode::I8x16__sub:
  // I16x8 integer arithmetic
  case OpCode::I16x8__add:
  case OpCode::I16x8__sub:
  case OpCode::I16x8__mul:
  // I32x4 integer arithmetic
  case OpCode::I32x4__add:
  case OpCode::I32x4__sub:
  case OpCode::I32x4__mul:
  case OpCode::I32x4__min_s:
  case OpCode::I32x4__min_u:
  case OpCode::I32x4__max_s:
  case OpCode::I32x4__max_u:
  // I64x2 integer arithmetic
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:
  // F32x4 float arithmetic
  case OpCode::F32x4__add:
  case OpCode::F32x4__sub:
  case OpCode::F32x4__mul:
  case OpCode::F32x4__div:
  case OpCode::F32x4__min:
  case OpCode::F32x4__max:
  case OpCode::F32x4__pmin:
  case OpCode::F32x4__pmax:
  // F64x2 float arithmetic
  case OpCode::F64x2__add:
  case OpCode::F64x2__sub:
  case OpCode::F64x2__mul:
  case OpCode::F64x2__div:
  case OpCode::F64x2__min:
  case OpCode::F64x2__max:
  case OpCode::F64x2__pmin:
  case OpCode::F64x2__pmax:
    return OpCode::V128__const;

  default:
    return OpCode::End; // sentinel: not a foldable binary op
  }
}

/// Return the const opcode for the result type of a unary/cast op,
/// given the opcode of the const operand. Returns OpCode::End as sentinel.
OpCode unaryResultConstOp(OpCode ConstOp, OpCode UnaryOp) noexcept {
  switch (UnaryOp) {
  // Unary ops that preserve their operand type
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
    return OpCode::I32__const;
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
    return OpCode::I64__const;
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
    return OpCode::F32__const;
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
    return OpCode::F64__const;

  // eqz produces i32
  case OpCode::I32__eqz:
  case OpCode::I64__eqz:
    return OpCode::I32__const;

  // Sign-extension in-place
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
    return OpCode::I32__const;
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
    return OpCode::I64__const;

  // Type conversions
  case OpCode::I32__wrap_i64:
    return OpCode::I32__const;
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
    return OpCode::I64__const;

  // Float-to-int truncation (trapping -- only fold when safe)
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
    return OpCode::I32__const;
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    return OpCode::I64__const;

  // Saturating truncation (always safe)
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
    return OpCode::I32__const;
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:
    return OpCode::I64__const;

  // Int-to-float conversion (always safe)
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
  case OpCode::F32__demote_f64:
    return OpCode::F32__const;
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
  case OpCode::F64__promote_f32:
    return OpCode::F64__const;

  // Reinterpret (always safe, just bit reinterpretation)
  case OpCode::I32__reinterpret_f32:
    return OpCode::I32__const;
  case OpCode::I64__reinterpret_f64:
    return OpCode::I64__const;
  case OpCode::F32__reinterpret_i32:
    return OpCode::F32__const;
  case OpCode::F64__reinterpret_i64:
    return OpCode::F64__const;

  // V128 unary: bitwise NOT, integer abs/neg, float unary math
  case OpCode::V128__not:
  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I8x16__popcnt:
  case OpCode::I16x8__abs:
  case OpCode::I16x8__neg:
  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I64x2__abs:
  case OpCode::I64x2__neg:
  case OpCode::F32x4__abs:
  case OpCode::F32x4__neg:
  case OpCode::F32x4__sqrt:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__abs:
  case OpCode::F64x2__neg:
  case OpCode::F64x2__sqrt:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
    return OpCode::V128__const;

  // Splat ops: scalar const → V128. Check input type matches.
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
    return ConstOp == OpCode::I32__const ? OpCode::V128__const : OpCode::End;
  case OpCode::I64x2__splat:
    return ConstOp == OpCode::I64__const ? OpCode::V128__const : OpCode::End;
  case OpCode::F32x4__splat:
    return ConstOp == OpCode::F32__const ? OpCode::V128__const : OpCode::End;
  case OpCode::F64x2__splat:
    return ConstOp == OpCode::F64__const ? OpCode::V128__const : OpCode::End;

  default:
    (void)ConstOp;
    return OpCode::End;
  }
}

// ---------------------------------------------------------------------------
// Value extraction helpers -- work on raw uint128_t from Instruction::getNum()
// ---------------------------------------------------------------------------

uint32_t getI32(const AST::Instruction &I) noexcept {
  return I.getNum().get<uint32_t>();
}
uint64_t getI64(const AST::Instruction &I) noexcept {
  return I.getNum().get<uint64_t>();
}
float getF32(const AST::Instruction &I) noexcept {
  return I.getNum().get<float>();
}
double getF64(const AST::Instruction &I) noexcept {
  return I.getNum().get<double>();
}

/// Construct a ValVariant holding a uint32_t.
ValVariant makeI32(uint32_t V) noexcept {
  ValVariant R;
  R.emplace<uint32_t>(V);
  return R;
}
ValVariant makeI64(uint64_t V) noexcept {
  ValVariant R;
  R.emplace<uint64_t>(V);
  return R;
}
ValVariant makeF32(float V) noexcept {
  ValVariant R;
  R.emplace<float>(V);
  return R;
}
ValVariant makeF64(double V) noexcept {
  ValVariant R;
  R.emplace<double>(V);
  return R;
}
uint128_t getV128(const AST::Instruction &I) noexcept {
  return I.getNum().get<uint128_t>();
}
ValVariant makeV128(uint128_t V) noexcept {
  ValVariant R;
  R.emplace<uint128_t>(V);
  return R;
}

#if !defined(_MSC_VER) || defined(__clang__)
/// Fold a binary V128 op by calling a simdOps:: free function in-place.
/// Uses std::memcpy to transfer bits in/out of ValVariant storage so that
/// GCC's strict aliasing (TBAA) cannot fold away the store through the
/// simdOps vector-type view before the subsequent uint128_t read.
template <typename SIMDFn>
ValVariant foldV128Bin(SIMDFn Fn, const AST::Instruction &Lhs,
                       const AST::Instruction &Rhs) noexcept {
  ValVariant V1, V2;
  V1.emplace<uint128_t>(getV128(Lhs));
  V2.emplace<uint128_t>(getV128(Rhs));
  Fn(V1, V2);
  uint128_t Out;
  std::memcpy(&Out, static_cast<const void *>(&V1), sizeof(uint128_t));
  return makeV128(Out);
}

/// Fold a unary V128 op by calling a simdOps:: free function in-place.
template <typename SIMDFn>
ValVariant foldV128Un(SIMDFn Fn, const AST::Instruction &Operand) noexcept {
  ValVariant V;
  V.emplace<uint128_t>(getV128(Operand));
  Fn(V);
  uint128_t Out;
  std::memcpy(&Out, static_cast<const void *>(&V), sizeof(uint128_t));
  return makeV128(Out);
}

/// Fold a splat op via simdOps::splatOp<TIn, TOut>.
/// TIn is the scalar input type stored in the const operand (e.g. uint32_t
/// for i32x4.splat).  TOut is the SIMD lane element type (e.g. uint8_t for
/// i8x16.splat).  memcpy out at the end defeats TBAA so the vector-typed
/// store through the Variant is not reordered relative to the final read.
template <typename TIn, typename TOut>
ValVariant foldSplatOp(const AST::Instruction &Operand) noexcept {
  ValVariant V;
  V.emplace<TIn>(Operand.getNum().get<TIn>());
  simdOps::splatOp<TIn, TOut>(V);
  uint128_t Out;
  std::memcpy(&Out, static_cast<const void *>(&V), sizeof(uint128_t));
  return makeV128(Out);
}
#endif // !_MSC_VER || __clang__

// ---------------------------------------------------------------------------
// Safety checks for trapping operations
// ---------------------------------------------------------------------------

/// Check if folding integer div/rem is safe for these operands.
/// Returns false when the runtime must trap (div-by-zero or signed overflow).
bool isSafeDivRem(OpCode Code, const AST::Instruction &Lhs,
                  const AST::Instruction &Rhs) noexcept {
  switch (Code) {
  case OpCode::I32__div_s: {
    uint32_t V2 = getI32(Rhs);
    if (V2 == 0)
      return false;
    int32_t S1 = static_cast<int32_t>(getI32(Lhs));
    int32_t S2 = static_cast<int32_t>(V2);
    return !(S1 == std::numeric_limits<int32_t>::min() && S2 == -1);
  }
  case OpCode::I32__div_u:
    return getI32(Rhs) != 0;
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
    return getI32(Rhs) != 0;
  case OpCode::I64__div_s: {
    uint64_t V2 = getI64(Rhs);
    if (V2 == 0)
      return false;
    int64_t S1 = static_cast<int64_t>(getI64(Lhs));
    int64_t S2 = static_cast<int64_t>(V2);
    return !(S1 == std::numeric_limits<int64_t>::min() && S2 == -1);
  }
  case OpCode::I64__div_u:
    return getI64(Rhs) != 0;
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
    return getI64(Rhs) != 0;
  default:
    return true;
  }
}

/// Check if folding a trapping truncation is safe.
/// Returns false for NaN, Inf, or out-of-range values.
template <typename TIn, typename TOut> bool isSafeTrunc(TIn Z) noexcept {
  if (std::isnan(Z) || std::isinf(Z))
    return false;
  Z = std::trunc(Z);
  TIn ValMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if constexpr (sizeof(TIn) > sizeof(TOut)) {
    return Z >= ValMin && Z <= ValMax;
  } else {
    return Z >= ValMin && Z < ValMax;
  }
}

bool isSafeTruncOp(OpCode Code, const AST::Instruction &Operand) noexcept {
  switch (Code) {
  case OpCode::I32__trunc_f32_s:
    return isSafeTrunc<float, int32_t>(getF32(Operand));
  case OpCode::I32__trunc_f32_u:
    return isSafeTrunc<float, uint32_t>(getF32(Operand));
  case OpCode::I32__trunc_f64_s:
    return isSafeTrunc<double, int32_t>(getF64(Operand));
  case OpCode::I32__trunc_f64_u:
    return isSafeTrunc<double, uint32_t>(getF64(Operand));
  case OpCode::I64__trunc_f32_s:
    return isSafeTrunc<float, int64_t>(getF32(Operand));
  case OpCode::I64__trunc_f32_u:
    return isSafeTrunc<float, uint64_t>(getF32(Operand));
  case OpCode::I64__trunc_f64_s:
    return isSafeTrunc<double, int64_t>(getF64(Operand));
  case OpCode::I64__trunc_f64_u:
    return isSafeTrunc<double, uint64_t>(getF64(Operand));
  default:
    return true;
  }
}

// ---------------------------------------------------------------------------
// Binary fold computation
// ---------------------------------------------------------------------------

/// Compute the result of a binary operation on two constants.
/// Caller guarantees safety (no div-by-zero, no overflow trap).
/// Returns the result as a ValVariant.
std::optional<ValVariant> foldBinary(OpCode Code, const AST::Instruction &Lhs,
                                     const AST::Instruction &Rhs) noexcept {
  switch (Code) {
  // --- I32 arithmetic ---
  case OpCode::I32__add:
    return makeI32(getI32(Lhs) + getI32(Rhs));
  case OpCode::I32__sub:
    return makeI32(getI32(Lhs) - getI32(Rhs));
  case OpCode::I32__mul:
    return makeI32(getI32(Lhs) * getI32(Rhs));
  case OpCode::I32__div_s:
    return makeI32(static_cast<uint32_t>(static_cast<int32_t>(getI32(Lhs)) /
                                         static_cast<int32_t>(getI32(Rhs))));
  case OpCode::I32__div_u:
    return makeI32(getI32(Lhs) / getI32(Rhs));
  case OpCode::I32__rem_s: {
    int32_t S1 = static_cast<int32_t>(getI32(Lhs));
    int32_t S2 = static_cast<int32_t>(getI32(Rhs));
    // Wasm spec: if s1 == INT_MIN and s2 == -1, result is 0.
    if (S1 == std::numeric_limits<int32_t>::min() && S2 == -1)
      return makeI32(0);
    return makeI32(static_cast<uint32_t>(S1 % S2));
  }
  case OpCode::I32__rem_u:
    return makeI32(getI32(Lhs) % getI32(Rhs));
  case OpCode::I32__and:
    return makeI32(getI32(Lhs) & getI32(Rhs));
  case OpCode::I32__or:
    return makeI32(getI32(Lhs) | getI32(Rhs));
  case OpCode::I32__xor:
    return makeI32(getI32(Lhs) ^ getI32(Rhs));
  case OpCode::I32__shl:
    return makeI32(getI32(Lhs) << (getI32(Rhs) & 31));
  case OpCode::I32__shr_s:
    return makeI32(static_cast<uint32_t>(static_cast<int32_t>(getI32(Lhs)) >>
                                         (getI32(Rhs) & 31)));
  case OpCode::I32__shr_u:
    return makeI32(getI32(Lhs) >> (getI32(Rhs) & 31));
  case OpCode::I32__rotl: {
    uint32_t V = getI32(Lhs);
    uint32_t K = getI32(Rhs) & 31;
    return makeI32((V << K) | (V >> ((32 - K) & 31)));
  }
  case OpCode::I32__rotr: {
    uint32_t V = getI32(Lhs);
    uint32_t K = getI32(Rhs) & 31;
    return makeI32((V >> K) | (V << ((32 - K) & 31)));
  }

  // --- I64 arithmetic ---
  case OpCode::I64__add:
    return makeI64(getI64(Lhs) + getI64(Rhs));
  case OpCode::I64__sub:
    return makeI64(getI64(Lhs) - getI64(Rhs));
  case OpCode::I64__mul:
    return makeI64(getI64(Lhs) * getI64(Rhs));
  case OpCode::I64__div_s:
    return makeI64(static_cast<uint64_t>(static_cast<int64_t>(getI64(Lhs)) /
                                         static_cast<int64_t>(getI64(Rhs))));
  case OpCode::I64__div_u:
    return makeI64(getI64(Lhs) / getI64(Rhs));
  case OpCode::I64__rem_s: {
    int64_t S1 = static_cast<int64_t>(getI64(Lhs));
    int64_t S2 = static_cast<int64_t>(getI64(Rhs));
    if (S1 == std::numeric_limits<int64_t>::min() && S2 == -1)
      return makeI64(0);
    return makeI64(static_cast<uint64_t>(S1 % S2));
  }
  case OpCode::I64__rem_u:
    return makeI64(getI64(Lhs) % getI64(Rhs));
  case OpCode::I64__and:
    return makeI64(getI64(Lhs) & getI64(Rhs));
  case OpCode::I64__or:
    return makeI64(getI64(Lhs) | getI64(Rhs));
  case OpCode::I64__xor:
    return makeI64(getI64(Lhs) ^ getI64(Rhs));
  case OpCode::I64__shl:
    return makeI64(getI64(Lhs) << (getI64(Rhs) & 63));
  case OpCode::I64__shr_s:
    return makeI64(static_cast<uint64_t>(static_cast<int64_t>(getI64(Lhs)) >>
                                         (getI64(Rhs) & 63)));
  case OpCode::I64__shr_u:
    return makeI64(getI64(Lhs) >> (getI64(Rhs) & 63));
  case OpCode::I64__rotl: {
    uint64_t V = getI64(Lhs);
    uint64_t K = getI64(Rhs) & 63;
    return makeI64((V << K) | (V >> ((64 - K) & 63)));
  }
  case OpCode::I64__rotr: {
    uint64_t V = getI64(Lhs);
    uint64_t K = getI64(Rhs) & 63;
    return makeI64((V >> K) | (V << ((64 - K) & 63)));
  }

  // --- F32 arithmetic ---
  // IEEE 754 arithmetic is deterministic for basic ops. NaN propagation and
  // signed zero behavior are well-specified by the Wasm standard.
  case OpCode::F32__add:
    return makeF32(getF32(Lhs) + getF32(Rhs));
  case OpCode::F32__sub:
    return makeF32(getF32(Lhs) - getF32(Rhs));
  case OpCode::F32__mul:
    return makeF32(getF32(Lhs) * getF32(Rhs));
  case OpCode::F32__div:
    return makeF32(getF32(Lhs) / getF32(Rhs));
  case OpCode::F32__min: {
    float A = getF32(Lhs), B = getF32(Rhs);
    if (std::isnan(A) || std::isnan(B)) {
      // Wasm NaN propagation: return a canonical NaN.
      uint32_t Bits = UINT32_C(0x7FC00000);
      float R;
      std::memcpy(&R, &Bits, sizeof(R));
      return makeF32(R);
    }
    if (A == 0.0f && B == 0.0f) {
      // min(+0, -0) = min(-0, +0) = -0
      return makeF32(std::signbit(A) ? A : B);
    }
    return makeF32(A < B ? A : B);
  }
  case OpCode::F32__max: {
    float A = getF32(Lhs), B = getF32(Rhs);
    if (std::isnan(A) || std::isnan(B)) {
      uint32_t Bits = UINT32_C(0x7FC00000);
      float R;
      std::memcpy(&R, &Bits, sizeof(R));
      return makeF32(R);
    }
    if (A == 0.0f && B == 0.0f) {
      // max(+0, -0) = max(-0, +0) = +0
      return makeF32(std::signbit(A) ? B : A);
    }
    return makeF32(A > B ? A : B);
  }
  case OpCode::F32__copysign:
    return makeF32(std::copysign(getF32(Lhs), getF32(Rhs)));

  // --- F64 arithmetic ---
  case OpCode::F64__add:
    return makeF64(getF64(Lhs) + getF64(Rhs));
  case OpCode::F64__sub:
    return makeF64(getF64(Lhs) - getF64(Rhs));
  case OpCode::F64__mul:
    return makeF64(getF64(Lhs) * getF64(Rhs));
  case OpCode::F64__div:
    return makeF64(getF64(Lhs) / getF64(Rhs));
  case OpCode::F64__min: {
    double A = getF64(Lhs), B = getF64(Rhs);
    if (std::isnan(A) || std::isnan(B)) {
      uint64_t Bits = UINT64_C(0x7FF8000000000000);
      double R;
      std::memcpy(&R, &Bits, sizeof(R));
      return makeF64(R);
    }
    if (A == 0.0 && B == 0.0) {
      return makeF64(std::signbit(A) ? A : B);
    }
    return makeF64(A < B ? A : B);
  }
  case OpCode::F64__max: {
    double A = getF64(Lhs), B = getF64(Rhs);
    if (std::isnan(A) || std::isnan(B)) {
      uint64_t Bits = UINT64_C(0x7FF8000000000000);
      double R;
      std::memcpy(&R, &Bits, sizeof(R));
      return makeF64(R);
    }
    if (A == 0.0 && B == 0.0) {
      return makeF64(std::signbit(A) ? B : A);
    }
    return makeF64(A > B ? A : B);
  }
  case OpCode::F64__copysign:
    return makeF64(std::copysign(getF64(Lhs), getF64(Rhs)));

  // --- Relational ops (all produce i32) ---
  case OpCode::I32__eq:
    return makeI32(getI32(Lhs) == getI32(Rhs) ? 1U : 0U);
  case OpCode::I32__ne:
    return makeI32(getI32(Lhs) != getI32(Rhs) ? 1U : 0U);
  case OpCode::I32__lt_s:
    return makeI32(static_cast<int32_t>(getI32(Lhs)) <
                           static_cast<int32_t>(getI32(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I32__lt_u:
    return makeI32(getI32(Lhs) < getI32(Rhs) ? 1U : 0U);
  case OpCode::I32__gt_s:
    return makeI32(static_cast<int32_t>(getI32(Lhs)) >
                           static_cast<int32_t>(getI32(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I32__gt_u:
    return makeI32(getI32(Lhs) > getI32(Rhs) ? 1U : 0U);
  case OpCode::I32__le_s:
    return makeI32(static_cast<int32_t>(getI32(Lhs)) <=
                           static_cast<int32_t>(getI32(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I32__le_u:
    return makeI32(getI32(Lhs) <= getI32(Rhs) ? 1U : 0U);
  case OpCode::I32__ge_s:
    return makeI32(static_cast<int32_t>(getI32(Lhs)) >=
                           static_cast<int32_t>(getI32(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I32__ge_u:
    return makeI32(getI32(Lhs) >= getI32(Rhs) ? 1U : 0U);

  case OpCode::I64__eq:
    return makeI32(getI64(Lhs) == getI64(Rhs) ? 1U : 0U);
  case OpCode::I64__ne:
    return makeI32(getI64(Lhs) != getI64(Rhs) ? 1U : 0U);
  case OpCode::I64__lt_s:
    return makeI32(static_cast<int64_t>(getI64(Lhs)) <
                           static_cast<int64_t>(getI64(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I64__lt_u:
    return makeI32(getI64(Lhs) < getI64(Rhs) ? 1U : 0U);
  case OpCode::I64__gt_s:
    return makeI32(static_cast<int64_t>(getI64(Lhs)) >
                           static_cast<int64_t>(getI64(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I64__gt_u:
    return makeI32(getI64(Lhs) > getI64(Rhs) ? 1U : 0U);
  case OpCode::I64__le_s:
    return makeI32(static_cast<int64_t>(getI64(Lhs)) <=
                           static_cast<int64_t>(getI64(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I64__le_u:
    return makeI32(getI64(Lhs) <= getI64(Rhs) ? 1U : 0U);
  case OpCode::I64__ge_s:
    return makeI32(static_cast<int64_t>(getI64(Lhs)) >=
                           static_cast<int64_t>(getI64(Rhs))
                       ? 1U
                       : 0U);
  case OpCode::I64__ge_u:
    return makeI32(getI64(Lhs) >= getI64(Rhs) ? 1U : 0U);

  case OpCode::F32__eq:
    return makeI32(getF32(Lhs) == getF32(Rhs) ? 1U : 0U);
  case OpCode::F32__ne:
    return makeI32(getF32(Lhs) != getF32(Rhs) ? 1U : 0U);
  case OpCode::F32__lt:
    return makeI32(getF32(Lhs) < getF32(Rhs) ? 1U : 0U);
  case OpCode::F32__gt:
    return makeI32(getF32(Lhs) > getF32(Rhs) ? 1U : 0U);
  case OpCode::F32__le:
    return makeI32(getF32(Lhs) <= getF32(Rhs) ? 1U : 0U);
  case OpCode::F32__ge:
    return makeI32(getF32(Lhs) >= getF32(Rhs) ? 1U : 0U);

  case OpCode::F64__eq:
    return makeI32(getF64(Lhs) == getF64(Rhs) ? 1U : 0U);
  case OpCode::F64__ne:
    return makeI32(getF64(Lhs) != getF64(Rhs) ? 1U : 0U);
  case OpCode::F64__lt:
    return makeI32(getF64(Lhs) < getF64(Rhs) ? 1U : 0U);
  case OpCode::F64__gt:
    return makeI32(getF64(Lhs) > getF64(Rhs) ? 1U : 0U);
  case OpCode::F64__le:
    return makeI32(getF64(Lhs) <= getF64(Rhs) ? 1U : 0U);
  case OpCode::F64__ge:
    return makeI32(getF64(Lhs) >= getF64(Rhs) ? 1U : 0U);

#if !defined(_MSC_VER) || defined(__clang__)
  // --- V128 bitwise ---
  case OpCode::V128__and:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::v128And(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::V128__andnot:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::v128Andnot(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::V128__or:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::v128Or(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::V128__xor:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::v128Xor(V1, V2);
        },
        Lhs, Rhs);

  // --- I8x16 arithmetic ---
  case OpCode::I8x16__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<uint8_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I8x16__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<uint8_t>(V1, V2);
        },
        Lhs, Rhs);

  // --- I16x8 arithmetic ---
  case OpCode::I16x8__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<uint16_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I16x8__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<uint16_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I16x8__mul:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMul<uint16_t>(V1, V2);
        },
        Lhs, Rhs);

  // --- I32x4 arithmetic ---
  case OpCode::I32x4__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<uint32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<uint32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__mul:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMul<uint32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__min_s:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMin<int32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__min_u:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMin<uint32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__max_s:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMax<int32_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I32x4__max_u:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMax<uint32_t>(V1, V2);
        },
        Lhs, Rhs);

  // --- I64x2 arithmetic ---
  case OpCode::I64x2__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<uint64_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I64x2__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<uint64_t>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::I64x2__mul:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMul<uint64_t>(V1, V2);
        },
        Lhs, Rhs);

  // --- F32x4 arithmetic ---
  case OpCode::F32x4__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__mul:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMul<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__div:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorDiv<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__min:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorFMin<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__max:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorFMax<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__pmin: // pseudo-min: no NaN handling, B if B<A else A
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMin<float>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F32x4__pmax:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMax<float>(V1, V2);
        },
        Lhs, Rhs);

  // --- F64x2 arithmetic ---
  case OpCode::F64x2__add:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorAdd<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__sub:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorSub<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__mul:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMul<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__div:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorDiv<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__min:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorFMin<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__max:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorFMax<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__pmin:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMin<double>(V1, V2);
        },
        Lhs, Rhs);
  case OpCode::F64x2__pmax:
    return foldV128Bin(
        [](ValVariant &V1, const ValVariant &V2) noexcept {
          simdOps::vectorMax<double>(V1, V2);
        },
        Lhs, Rhs);
#endif // !_MSC_VER || __clang__

  default:
    return std::nullopt;
  }
}

// ---------------------------------------------------------------------------
// Unary/cast fold computation
// ---------------------------------------------------------------------------

/// Helper: clz for a T value.
template <typename T> T computeClz(T I) noexcept {
  if (I == 0)
    return static_cast<T>(sizeof(T) * 8);
  T Cnt = 0;
  T Mask = static_cast<T>(1) << (sizeof(T) * 8 - 1);
  while ((I & Mask) == 0) {
    Cnt++;
    I <<= 1;
  }
  return Cnt;
}

template <typename T> T computeCtz(T I) noexcept {
  if (I == 0)
    return static_cast<T>(sizeof(T) * 8);
  T Cnt = 0;
  while ((I & 1) == 0) {
    Cnt++;
    I >>= 1;
  }
  return Cnt;
}

template <typename T> T computePopcnt(T I) noexcept {
  T Cnt = 0;
  while (I != 0) {
    if (I & 1)
      Cnt++;
    I >>= 1;
  }
  return Cnt;
}

/// Saturating truncation helper.
template <typename TIn, typename TOut> TOut truncSat(TIn Z) noexcept {
  if (std::isnan(Z))
    return static_cast<TOut>(0);
  if (std::isinf(Z)) {
    if (Z < static_cast<TIn>(0))
      return std::numeric_limits<TOut>::min();
    return std::numeric_limits<TOut>::max();
  }
  Z = std::trunc(Z);
  TIn ValMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  TIn ValMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  if (Z <= ValMin)
    return std::numeric_limits<TOut>::min();
  if (Z >= ValMax)
    return std::numeric_limits<TOut>::max();
  return static_cast<TOut>(Z);
}

std::optional<ValVariant> foldUnary(OpCode Code,
                                    const AST::Instruction &Operand) noexcept {
  switch (Code) {
  // --- I32 unary ---
  case OpCode::I32__eqz:
    return makeI32(getI32(Operand) == 0 ? 1U : 0U);
  case OpCode::I32__clz:
    return makeI32(computeClz(getI32(Operand)));
  case OpCode::I32__ctz:
    return makeI32(computeCtz(getI32(Operand)));
  case OpCode::I32__popcnt:
    return makeI32(computePopcnt(getI32(Operand)));
  case OpCode::I32__extend8_s:
    return makeI32(static_cast<uint32_t>(static_cast<int32_t>(
        static_cast<int8_t>(static_cast<uint8_t>(getI32(Operand))))));
  case OpCode::I32__extend16_s:
    return makeI32(static_cast<uint32_t>(static_cast<int32_t>(
        static_cast<int16_t>(static_cast<uint16_t>(getI32(Operand))))));

  // --- I64 unary ---
  case OpCode::I64__eqz:
    return makeI32(getI64(Operand) == 0 ? 1U : 0U);
  case OpCode::I64__clz:
    return makeI64(computeClz(getI64(Operand)));
  case OpCode::I64__ctz:
    return makeI64(computeCtz(getI64(Operand)));
  case OpCode::I64__popcnt:
    return makeI64(computePopcnt(getI64(Operand)));
  case OpCode::I64__extend8_s:
    return makeI64(static_cast<uint64_t>(static_cast<int64_t>(
        static_cast<int8_t>(static_cast<uint8_t>(getI64(Operand))))));
  case OpCode::I64__extend16_s:
    return makeI64(static_cast<uint64_t>(static_cast<int64_t>(
        static_cast<int16_t>(static_cast<uint16_t>(getI64(Operand))))));
  case OpCode::I64__extend32_s:
    return makeI64(static_cast<uint64_t>(static_cast<int64_t>(
        static_cast<int32_t>(static_cast<uint32_t>(getI64(Operand))))));

  // --- F32 unary ---
  case OpCode::F32__abs:
    return makeF32(std::fabs(getF32(Operand)));
  case OpCode::F32__neg:
    return makeF32(-getF32(Operand));
  case OpCode::F32__ceil:
    return makeF32(std::ceil(getF32(Operand)));
  case OpCode::F32__floor:
    return makeF32(std::floor(getF32(Operand)));
  case OpCode::F32__trunc:
    return makeF32(std::trunc(getF32(Operand)));
  case OpCode::F32__nearest:
    return makeF32(WasmEdge::roundeven(getF32(Operand)));
  case OpCode::F32__sqrt:
    return makeF32(std::sqrt(getF32(Operand)));

  // --- F64 unary ---
  case OpCode::F64__abs:
    return makeF64(std::fabs(getF64(Operand)));
  case OpCode::F64__neg:
    return makeF64(-getF64(Operand));
  case OpCode::F64__ceil:
    return makeF64(std::ceil(getF64(Operand)));
  case OpCode::F64__floor:
    return makeF64(std::floor(getF64(Operand)));
  case OpCode::F64__trunc:
    return makeF64(std::trunc(getF64(Operand)));
  case OpCode::F64__nearest:
    return makeF64(WasmEdge::roundeven(getF64(Operand)));
  case OpCode::F64__sqrt:
    return makeF64(std::sqrt(getF64(Operand)));

  // --- Type conversions ---
  case OpCode::I32__wrap_i64:
    return makeI32(static_cast<uint32_t>(getI64(Operand)));
  case OpCode::I64__extend_i32_s:
    return makeI64(static_cast<uint64_t>(
        static_cast<int64_t>(static_cast<int32_t>(getI32(Operand)))));
  case OpCode::I64__extend_i32_u:
    return makeI64(static_cast<uint64_t>(getI32(Operand)));

  // Trapping truncation (caller must verify safety via isSafeTruncOp first)
  case OpCode::I32__trunc_f32_s:
    return makeI32(static_cast<uint32_t>(
        static_cast<int32_t>(std::trunc(getF32(Operand)))));
  case OpCode::I32__trunc_f32_u:
    return makeI32(static_cast<uint32_t>(std::trunc(getF32(Operand))));
  case OpCode::I32__trunc_f64_s:
    return makeI32(static_cast<uint32_t>(
        static_cast<int32_t>(std::trunc(getF64(Operand)))));
  case OpCode::I32__trunc_f64_u:
    return makeI32(static_cast<uint32_t>(std::trunc(getF64(Operand))));
  case OpCode::I64__trunc_f32_s:
    return makeI64(static_cast<uint64_t>(
        static_cast<int64_t>(std::trunc(getF32(Operand)))));
  case OpCode::I64__trunc_f32_u:
    return makeI64(static_cast<uint64_t>(std::trunc(getF32(Operand))));
  case OpCode::I64__trunc_f64_s:
    return makeI64(static_cast<uint64_t>(
        static_cast<int64_t>(std::trunc(getF64(Operand)))));
  case OpCode::I64__trunc_f64_u:
    return makeI64(static_cast<uint64_t>(std::trunc(getF64(Operand))));

  // Saturating truncation (always safe)
  case OpCode::I32__trunc_sat_f32_s:
    return makeI32(
        static_cast<uint32_t>(truncSat<float, int32_t>(getF32(Operand))));
  case OpCode::I32__trunc_sat_f32_u:
    return makeI32(truncSat<float, uint32_t>(getF32(Operand)));
  case OpCode::I32__trunc_sat_f64_s:
    return makeI32(
        static_cast<uint32_t>(truncSat<double, int32_t>(getF64(Operand))));
  case OpCode::I32__trunc_sat_f64_u:
    return makeI32(truncSat<double, uint32_t>(getF64(Operand)));
  case OpCode::I64__trunc_sat_f32_s:
    return makeI64(
        static_cast<uint64_t>(truncSat<float, int64_t>(getF32(Operand))));
  case OpCode::I64__trunc_sat_f32_u:
    return makeI64(truncSat<float, uint64_t>(getF32(Operand)));
  case OpCode::I64__trunc_sat_f64_s:
    return makeI64(
        static_cast<uint64_t>(truncSat<double, int64_t>(getF64(Operand))));
  case OpCode::I64__trunc_sat_f64_u:
    return makeI64(truncSat<double, uint64_t>(getF64(Operand)));

  // Int-to-float conversion
  case OpCode::F32__convert_i32_s:
    return makeF32(static_cast<float>(static_cast<int32_t>(getI32(Operand))));
  case OpCode::F32__convert_i32_u:
    return makeF32(static_cast<float>(getI32(Operand)));
  case OpCode::F32__convert_i64_s:
    return makeF32(static_cast<float>(static_cast<int64_t>(getI64(Operand))));
  case OpCode::F32__convert_i64_u:
    return makeF32(static_cast<float>(getI64(Operand)));
  case OpCode::F64__convert_i32_s:
    return makeF64(static_cast<double>(static_cast<int32_t>(getI32(Operand))));
  case OpCode::F64__convert_i32_u:
    return makeF64(static_cast<double>(getI32(Operand)));
  case OpCode::F64__convert_i64_s:
    return makeF64(static_cast<double>(static_cast<int64_t>(getI64(Operand))));
  case OpCode::F64__convert_i64_u:
    return makeF64(static_cast<double>(getI64(Operand)));

  // Float-to-float conversion
  case OpCode::F32__demote_f64:
    return makeF32(static_cast<float>(getF64(Operand)));
  case OpCode::F64__promote_f32:
    return makeF64(static_cast<double>(getF32(Operand)));

  // Reinterpret (bit cast)
  case OpCode::I32__reinterpret_f32: {
    float F = getF32(Operand);
    uint32_t Bits;
    std::memcpy(&Bits, &F, sizeof(Bits));
    return makeI32(Bits);
  }
  case OpCode::I64__reinterpret_f64: {
    double D = getF64(Operand);
    uint64_t Bits;
    std::memcpy(&Bits, &D, sizeof(Bits));
    return makeI64(Bits);
  }
  case OpCode::F32__reinterpret_i32: {
    uint32_t Bits = getI32(Operand);
    float F;
    std::memcpy(&F, &Bits, sizeof(F));
    return makeF32(F);
  }
  case OpCode::F64__reinterpret_i64: {
    uint64_t Bits = getI64(Operand);
    double D;
    std::memcpy(&D, &Bits, sizeof(D));
    return makeF64(D);
  }

#if !defined(_MSC_VER) || defined(__clang__)
  // --- V128 bitwise NOT ---
  case OpCode::V128__not:
    return foldV128Un([](ValVariant &V) noexcept { simdOps::v128Not(V); },
                      Operand);

  // --- I8x16 abs/neg/popcnt ---
  case OpCode::I8x16__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<int8_t>(V); }, Operand);
  case OpCode::I8x16__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<int8_t>(V); }, Operand);
  case OpCode::I8x16__popcnt:
    return foldV128Un([](ValVariant &V) noexcept { simdOps::vectorPopcnt(V); },
                      Operand);

  // --- I16x8 abs/neg ---
  case OpCode::I16x8__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<int16_t>(V); },
        Operand);
  case OpCode::I16x8__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<int16_t>(V); },
        Operand);

  // --- I32x4 abs/neg ---
  case OpCode::I32x4__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<int32_t>(V); },
        Operand);
  case OpCode::I32x4__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<int32_t>(V); },
        Operand);

  // --- I64x2 abs/neg ---
  case OpCode::I64x2__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<int64_t>(V); },
        Operand);
  case OpCode::I64x2__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<int64_t>(V); },
        Operand);

  // --- F32x4 unary ---
  case OpCode::F32x4__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<float>(V); }, Operand);
  case OpCode::F32x4__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<float>(V); }, Operand);
  case OpCode::F32x4__sqrt:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorSqrt<float>(V); }, Operand);
  case OpCode::F32x4__ceil:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorCeil<float>(V); }, Operand);
  case OpCode::F32x4__floor:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorFloor<float>(V); },
        Operand);
  case OpCode::F32x4__trunc:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorTrunc<float>(V); },
        Operand);
  case OpCode::F32x4__nearest:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNearest<float>(V); },
        Operand);

  // --- F64x2 unary ---
  case OpCode::F64x2__abs:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorAbs<double>(V); }, Operand);
  case OpCode::F64x2__neg:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNeg<double>(V); }, Operand);
  case OpCode::F64x2__sqrt:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorSqrt<double>(V); },
        Operand);
  case OpCode::F64x2__ceil:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorCeil<double>(V); },
        Operand);
  case OpCode::F64x2__floor:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorFloor<double>(V); },
        Operand);
  case OpCode::F64x2__trunc:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorTrunc<double>(V); },
        Operand);
  case OpCode::F64x2__nearest:
    return foldV128Un(
        [](ValVariant &V) noexcept { simdOps::vectorNearest<double>(V); },
        Operand);

  // --- Splat operations (scalar → V128) ---
  // splatOp<TIn, TOut>: reads scalar TIn from Val, writes VTOut splatted vec.
  // Reading back as uint128_t via unsafe-union aliasing extracts the raw bytes.
  case OpCode::I8x16__splat:
    return foldSplatOp<uint32_t, uint8_t>(Operand);
  case OpCode::I16x8__splat:
    return foldSplatOp<uint32_t, uint16_t>(Operand);
  case OpCode::I32x4__splat:
    return foldSplatOp<uint32_t, uint32_t>(Operand);
  case OpCode::I64x2__splat:
    return foldSplatOp<uint64_t, uint64_t>(Operand);
  case OpCode::F32x4__splat:
    return foldSplatOp<float, float>(Operand);
  case OpCode::F64x2__splat:
    return foldSplatOp<double, double>(Operand);
#endif // !_MSC_VER || __clang__

  default:
    return std::nullopt;
  }
}

// ---------------------------------------------------------------------------
// Identity pattern detection
// ---------------------------------------------------------------------------

/// Check if a const value followed by a binary op is an identity operation
/// (i.e., the result is always the other operand on the stack).
///
/// NOTE: In the Wasm stack machine, for `... X, const V, binop`, the const V
/// is the RHS operand. So we check for right-identity patterns:
///   X + 0 = X, X - 0 = X, X * 1 = X, X | 0 = X, X ^ 0 = X, X & ~0 = X
///   X << 0 = X, X >> 0 = X, X rotl 0 = X, X rotr 0 = X
bool isRightIdentity(OpCode ConstOp, const AST::Instruction &ConstInstr,
                     OpCode BinOp) noexcept {
  if (ConstOp == OpCode::I32__const) {
    uint32_t V = getI32(ConstInstr);
    switch (BinOp) {
    case OpCode::I32__add:
    case OpCode::I32__sub:
    case OpCode::I32__or:
    case OpCode::I32__xor:
    case OpCode::I32__shl:
    case OpCode::I32__shr_s:
    case OpCode::I32__shr_u:
    case OpCode::I32__rotl:
    case OpCode::I32__rotr:
      return V == 0;
    case OpCode::I32__mul:
      return V == 1;
    case OpCode::I32__and:
      return V == UINT32_MAX;
    default:
      return false;
    }
  }
  if (ConstOp == OpCode::I64__const) {
    uint64_t V = getI64(ConstInstr);
    switch (BinOp) {
    case OpCode::I64__add:
    case OpCode::I64__sub:
    case OpCode::I64__or:
    case OpCode::I64__xor:
    case OpCode::I64__shl:
    case OpCode::I64__shr_s:
    case OpCode::I64__shr_u:
    case OpCode::I64__rotl:
    case OpCode::I64__rotr:
      return V == 0;
    case OpCode::I64__mul:
      return V == 1;
    case OpCode::I64__and:
      return V == UINT64_MAX;
    default:
      return false;
    }
  }
  return false;
}

/// Replace an instruction with a Nop, preserving its source offset.
void replaceWithNop(AST::Instruction &Instr) noexcept {
  uint32_t Off = Instr.getOffset();
  Instr = AST::Instruction(OpCode::Nop, Off);
}

/// Replace an instruction with a const holding the given value.
void replaceWithConst(AST::Instruction &Instr, OpCode ConstOp,
                      ValVariant Val) noexcept {
  uint32_t Off = Instr.getOffset();
  Instr = AST::Instruction(ConstOp, Off);
  Instr.setNum(Val);
}

/// Check whether an opcode is a trapping truncation that needs safety checks.
bool isTrappingTrunc(OpCode Code) noexcept {
  switch (Code) {
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
    return true;
  default:
    return false;
  }
}

/// Check whether an opcode is a div or rem that needs safety checks.
bool isDivRem(OpCode Code) noexcept {
  switch (Code) {
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I64__div_s:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
    return true;
  default:
    return false;
  }
}

/// Find the next instruction index after Start that is not a Nop.
/// Returns -1 if no such instruction exists before End.
int64_t nextNonNop(const AST::InstrVec &Instrs, int64_t Start,
                   int64_t End) noexcept {
  for (int64_t J = Start; J < End; ++J) {
    if (Instrs[static_cast<size_t>(J)].getOpCode() != OpCode::Nop) {
      return J;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
// IPCP: purity analysis, mini Wasm evaluator, and call-site folding
// ---------------------------------------------------------------------------

/// Returns true if opcode Op has no observable side effects.
/// Side effects include: memory loads/stores, global access, table access,
/// indirect/ref calls, exception ops, bulk memory/table ops, SIMD memory ops,
/// and atomic ops.  Pure arithmetic, const, control flow, and local ops return
/// true.
bool isPureOpcode(OpCode Op) noexcept {
  switch (Op) {
  // ── Memory loads ──────────────────────────────────────────────────────
  case OpCode::I32__load:
  case OpCode::I64__load:
  case OpCode::F32__load:
  case OpCode::F64__load:
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  // ── Memory stores ─────────────────────────────────────────────────────
  case OpCode::I32__store:
  case OpCode::I64__store:
  case OpCode::F32__store:
  case OpCode::F64__store:
  case OpCode::I32__store8:
  case OpCode::I32__store16:
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
  // ── Memory control ────────────────────────────────────────────────────
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
  // ── Global access ─────────────────────────────────────────────────────
  case OpCode::Global__get:
  case OpCode::Global__set:
  // ── Table access (part 1) ─────────────────────────────────────────────
  case OpCode::Table__get:
  case OpCode::Table__set:
  // ── Indirect / ref calls ──────────────────────────────────────────────
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect:
  case OpCode::Call_ref:
  case OpCode::Return_call_ref:
  // ── Exception operations ──────────────────────────────────────────────
  case OpCode::Throw:
  case OpCode::Throw_ref:
  case OpCode::Try_table:
  // ── Bulk memory (0xFC prefix) ─────────────────────────────────────────
  case OpCode::Memory__init:
  case OpCode::Data__drop:
  case OpCode::Memory__copy:
  case OpCode::Memory__fill:
  // ── Bulk table (0xFC prefix) ──────────────────────────────────────────
  case OpCode::Table__init:
  case OpCode::Elem__drop:
  case OpCode::Table__copy:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  // ── SIMD memory (0xFD prefix) ─────────────────────────────────────────
  case OpCode::V128__load:
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
  case OpCode::V128__load8_splat:
  case OpCode::V128__load16_splat:
  case OpCode::V128__load32_splat:
  case OpCode::V128__load64_splat:
  case OpCode::V128__store:
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
  case OpCode::V128__load32_zero:
  case OpCode::V128__load64_zero:
  // ── Thread / atomic ops (0xFE prefix) ─────────────────────────────────
  case OpCode::Memory__atomic__notify:
  case OpCode::Memory__atomic__wait32:
  case OpCode::Memory__atomic__wait64:
    return false;
  default:
    return true; // arithmetic, control flow, const, locals — all pure
  }
}

// ─── Module-level function information ──────────────────────────────────────

/// Per-function metadata used during purity analysis and call-site folding.
struct FuncEntry {
  const AST::FunctionType *FType =
      nullptr;              ///< Param/return types; null if unavailable.
  bool IsPure = false;      ///< No observable side effects.
  size_t SegIdx = SIZE_MAX; ///< CodeSection index; SIZE_MAX = imported.
};

/// Build the per-function entry table for Mod.
///
/// Imported functions are always marked impure (conservative — no body to
/// inspect without the runtime store).  Defined functions are first scanned
/// for directly-impure opcodes, then purity is propagated through the call
/// graph via a fixpoint loop (a callee being impure makes the caller impure).
std::vector<FuncEntry> buildFuncTable(AST::Module &Mod) {
  const auto &TypeSec = Mod.getTypeSection().getContent();
  const auto &ImportSec = Mod.getImportSection().getContent();
  const auto &FuncSec = Mod.getFunctionSection().getContent();
  const auto &CodeSec = Mod.getCodeSection().getContent();

  // Count imported functions.
  size_t ImportedFuncCount = 0;
  for (const auto &Imp : ImportSec)
    if (Imp.getExternalType() == ExternalType::Function)
      ++ImportedFuncCount;

  const size_t TotalFuncs = ImportedFuncCount + FuncSec.size();
  std::vector<FuncEntry> Table(TotalFuncs);

  // Helper: look up FunctionType from a type section index.
  auto getType = [&](uint32_t TIdx) -> const AST::FunctionType * {
    if (TIdx >= TypeSec.size())
      return nullptr;
    const auto &CT = TypeSec[TIdx].getCompositeType();
    return CT.isFunc() ? &CT.getFuncType() : nullptr;
  };

  // Fill imported function entries (always impure).
  size_t IFI = 0;
  for (const auto &Imp : ImportSec) {
    if (Imp.getExternalType() != ExternalType::Function)
      continue;
    Table[IFI].FType = getType(Imp.getExternalFuncTypeIdx());
    Table[IFI].IsPure = false;
    Table[IFI].SegIdx = SIZE_MAX;
    ++IFI;
  }

  // Fill defined function entries; initial purity from direct opcode scan.
  for (size_t SI = 0; SI < FuncSec.size(); ++SI) {
    const size_t AbsIdx = ImportedFuncCount + SI;
    Table[AbsIdx].FType = getType(FuncSec[SI]);
    Table[AbsIdx].SegIdx = SI;
    // Scan for any directly-impure opcode.
    Table[AbsIdx].IsPure = true;
    for (const auto &I : CodeSec[SI].getExpr().getInstrs()) {
      if (!isPureOpcode(I.getOpCode())) {
        Table[AbsIdx].IsPure = false;
        break;
      }
    }
  }

  // Fixpoint: a function that calls an impure callee is itself impure.
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (size_t SI = 0; SI < FuncSec.size(); ++SI) {
      const size_t AbsIdx = ImportedFuncCount + SI;
      if (!Table[AbsIdx].IsPure)
        continue; // already impure
      for (const auto &I : CodeSec[SI].getExpr().getInstrs()) {
        if (I.getOpCode() != OpCode::Call &&
            I.getOpCode() != OpCode::Return_call)
          continue;
        const uint32_t Callee = I.getTargetIndex();
        if (Callee >= TotalFuncs || !Table[Callee].IsPure) {
          Table[AbsIdx].IsPure = false;
          Changed = true;
          break;
        }
      }
    }
  }

  return Table;
}

// ─── Mini Wasm evaluator ────────────────────────────────────────────────────

/// Map a scalar ValType to the const opcode used to push it.
/// Returns OpCode::End for non-scalar types (references, etc.) which cannot
/// be stored in a constant instruction.
OpCode typeToConstOp(const ValType &VT) noexcept {
  switch (VT.getCode()) {
  case TypeCode::I32:
    return OpCode::I32__const;
  case TypeCode::I64:
    return OpCode::I64__const;
  case TypeCode::F32:
    return OpCode::F32__const;
  case TypeCode::F64:
    return OpCode::F64__const;
  case TypeCode::V128:
    return OpCode::V128__const;
  default:
    return OpCode::End;
  }
}

/// Build the initial local-variable vector for a function invocation.
/// Args contains the (already-evaluated) parameter values; additional
/// locals are zero-initialised according to their declared types.
std::vector<ValVariant>
buildLocalVec(std::vector<ValVariant> Args,
              Span<const std::pair<uint32_t, ValType>> LocalDefs) {
  for (const auto &[Count, VType] : LocalDefs)
    for (uint32_t K = 0; K < Count; ++K)
      Args.push_back(ValueFromType(VType));
  return Args;
}

/// Pop the top N values from Stack and return them in push order (oldest
/// first).
std::vector<ValVariant> popN(std::vector<ValVariant> &Stack, size_t N) {
  std::vector<ValVariant> Vals(Stack.end() - static_cast<ptrdiff_t>(N),
                               Stack.end());
  Stack.erase(Stack.end() - static_cast<ptrdiff_t>(N), Stack.end());
  return Vals;
}

/// Create a temporary const instruction carrying value V.
AST::Instruction mkConst(OpCode ConstOp, ValVariant V) noexcept {
  AST::Instruction I(ConstOp);
  I.setNum(V);
  return I;
}

// Forward declaration — evalFunc and evalInstrs are mutually recursive.
std::optional<std::vector<ValVariant>>
evalFunc(size_t AbsFuncIdx, std::vector<ValVariant> Args,
         const std::vector<FuncEntry> &FuncTable, AST::Module &Mod,
         size_t ImportedFuncCount, uint32_t &StepsLeft,
         uint32_t DepthLeft) noexcept;

/// Execute the instruction sequence Instrs with the given Locals and return
/// NumReturns values from the top of the value stack on completion.
///
/// Returns std::nullopt on: step-limit exceeded, trap
/// (unreachable/div-by-zero), or any unsupported instruction (conservative
/// bail-out).
std::optional<std::vector<ValVariant>>
evalInstrs(AST::InstrView Instrs, std::vector<ValVariant> Locals,
           uint32_t NumReturns, const std::vector<FuncEntry> &FuncTable,
           AST::Module &Mod, size_t ImportedFuncCount, uint32_t &StepsLeft,
           uint32_t DepthLeft) noexcept {

  std::vector<ValVariant> Stack;
  Stack.reserve(16);
  int64_t PC = 0;
  const int64_t Size = static_cast<int64_t>(Instrs.size());

  // Typed pop helpers.
  auto popI32 = [&]() noexcept -> uint32_t {
    uint32_t V = Stack.back().get<uint32_t>();
    Stack.pop_back();
    return V;
  };

  // Helper: binary op via existing foldBinary (reuses all arithmetic + safety).
  auto doBin = [&](OpCode BinOp, OpCode ConstOp) noexcept -> bool {
    if (Stack.size() < 2)
      return false;
    ValVariant RhsV = Stack.back();
    Stack.pop_back();
    ValVariant LhsV = Stack.back();
    Stack.pop_back();
    AST::Instruction RhsI = mkConst(ConstOp, RhsV);
    AST::Instruction LhsI = mkConst(ConstOp, LhsV);
    if (isDivRem(BinOp) && !isSafeDivRem(BinOp, LhsI, RhsI))
      return false;
    auto R = foldBinary(BinOp, LhsI, RhsI);
    if (!R)
      return false;
    Stack.push_back(*R);
    return true;
  };

  // Helper: unary op via existing foldUnary.
  auto doUn = [&](OpCode UnOp, OpCode ConstOp) noexcept -> bool {
    if (Stack.empty())
      return false;
    ValVariant V = Stack.back();
    Stack.pop_back();
    AST::Instruction I = mkConst(ConstOp, V);
    if (isTrappingTrunc(UnOp) && !isSafeTruncOp(UnOp, I))
      return false;
    auto R = foldUnary(UnOp, I);
    if (!R)
      return false;
    Stack.push_back(*R);
    return true;
  };

  while (PC >= 0 && PC < Size) {
    if (StepsLeft == 0)
      return std::nullopt;
    --StepsLeft;

    const auto &Instr = Instrs[static_cast<size_t>(PC)];
    ++PC; // advance past current instruction
    const OpCode Op = Instr.getOpCode();

    switch (Op) {
    // ── Trivial ─────────────────────────────────────────────────────────
    case OpCode::Nop:
      break;
    case OpCode::Unreachable:
      return std::nullopt; // trap

    // ── Constants ───────────────────────────────────────────────────────
    case OpCode::I32__const:
      Stack.push_back(makeI32(Instr.getNum().get<uint32_t>()));
      break;
    case OpCode::I64__const:
      Stack.push_back(makeI64(Instr.getNum().get<uint64_t>()));
      break;
    case OpCode::F32__const:
      Stack.push_back(makeF32(Instr.getNum().get<float>()));
      break;
    case OpCode::F64__const:
      Stack.push_back(makeF64(Instr.getNum().get<double>()));
      break;
    case OpCode::V128__const:
      Stack.push_back(makeV128(Instr.getNum().get<uint128_t>()));
      break;

    // ── Locals ──────────────────────────────────────────────────────────
    case OpCode::Local__get: {
      uint32_t Idx = Instr.getTargetIndex();
      if (Idx >= Locals.size())
        return std::nullopt;
      Stack.push_back(Locals[Idx]);
      break;
    }
    case OpCode::Local__set: {
      uint32_t Idx = Instr.getTargetIndex();
      if (Idx >= Locals.size() || Stack.empty())
        return std::nullopt;
      Locals[Idx] = Stack.back();
      Stack.pop_back();
      break;
    }
    case OpCode::Local__tee: {
      uint32_t Idx = Instr.getTargetIndex();
      if (Idx >= Locals.size() || Stack.empty())
        return std::nullopt;
      Locals[Idx] = Stack.back(); // leave value on stack
      break;
    }

    // ── Stack ops ───────────────────────────────────────────────────────
    case OpCode::Drop:
      if (Stack.empty())
        return std::nullopt;
      Stack.pop_back();
      break;

    case OpCode::Select:
    case OpCode::Select_t: {
      if (Stack.size() < 3)
        return std::nullopt;
      uint32_t Cond = popI32(); // condition (always i32)
      ValVariant Val2 = Stack.back();
      Stack.pop_back(); // second value
      if (Cond == 0)
        Stack.back() = Val2; // pick second if false
      // else keep first value (already on top)
      break;
    }

    // ── Control flow: block and loop entry ──────────────────────────────
    // Nothing to do at entry; pre-computed PCOffset/JumpEnd handle jumps.
    // Bail out for multi-value (TypeIdx) blocks to stay conservative.
    case OpCode::Block:
    case OpCode::Loop:
      if (!Instr.getBlockType().isEmpty() && !Instr.getBlockType().isValType())
        return std::nullopt; // multi-value block: unsupported
      break;

    case OpCode::If: {
      if (!Instr.getBlockType().isEmpty() && !Instr.getBlockType().isValType())
        return std::nullopt;
      if (Stack.empty())
        return std::nullopt;
      uint32_t Cond = popI32();
      if (Cond == 0) {
        // Jump to the else-body start (past the Else instruction), or to
        // the instruction after End if there is no Else clause.
        // JumpElse = Else_idx - If_idx  (or = JumpEnd when no Else).
        // After ++PC we are at If_idx+1; adding JumpElse → Else_idx+1. ✓
        PC += static_cast<int64_t>(Instr.getJumpElse());
      }
      break;
    }

    case OpCode::Else:
      // We hit Else while executing the true branch — jump past End.
      // JumpEnd = End_idx - Else_idx.  After ++PC → Else_idx+1.
      // Adding JumpEnd → End_idx+1 (after End). ✓
      PC += static_cast<int64_t>(Instr.getJumpEnd());
      break;

    case OpCode::End:
      if (Instr.isExprLast()) {
        // Function-level end: collect and return NumReturns values.
        if (Stack.size() < NumReturns)
          return std::nullopt;
        return std::vector<ValVariant>(Stack.end() - NumReturns, Stack.end());
      }
      // Block/loop/if end: execution falls through; results stay on stack. ✓
      break;

    case OpCode::Return:
      if (Stack.size() < NumReturns)
        return std::nullopt;
      return std::vector<ValVariant>(Stack.end() - NumReturns, Stack.end());

    // ── Branches ────────────────────────────────────────────────────────
    case OpCode::Br: {
      const auto &J = Instr.getJump();
      if (J.StackEraseBegin > Stack.size())
        return std::nullopt;
      // Erase intermediate values; keep result values at the top.
      Stack.erase(Stack.end() - J.StackEraseBegin,
                  Stack.end() - J.StackEraseEnd);
      // Apply pre-computed PCOffset: net jump = PCOffset from the Br site.
      // After ++PC, PC = Br_idx+1; PC += PCOffset-1 → Br_idx+PCOffset. ✓
      PC += static_cast<int64_t>(J.PCOffset) - 1;
      break;
    }
    case OpCode::Br_if: {
      if (Stack.empty())
        return std::nullopt;
      uint32_t Cond = popI32();
      if (Cond != 0) {
        const auto &J = Instr.getJump();
        if (J.StackEraseBegin > Stack.size())
          return std::nullopt;
        Stack.erase(Stack.end() - J.StackEraseBegin,
                    Stack.end() - J.StackEraseEnd);
        PC += static_cast<int64_t>(J.PCOffset) - 1;
      }
      break;
    }

    // ── Call ────────────────────────────────────────────────────────────
    case OpCode::Call: {
      if (DepthLeft == 0 || StepsLeft == 0)
        return std::nullopt;
      uint32_t FuncIdx = Instr.getTargetIndex();
      if (FuncIdx >= FuncTable.size() || !FuncTable[FuncIdx].IsPure)
        return std::nullopt;
      const AST::FunctionType *FType = FuncTable[FuncIdx].FType;
      if (!FType)
        return std::nullopt;
      uint32_t NumP = static_cast<uint32_t>(FType->getParamTypes().size());
      if (Stack.size() < NumP)
        return std::nullopt;
      std::vector<ValVariant> CallArgs = popN(Stack, NumP);
      auto CR = evalFunc(FuncIdx, std::move(CallArgs), FuncTable, Mod,
                         ImportedFuncCount, StepsLeft, DepthLeft - 1);
      if (!CR)
        return std::nullopt;
      for (auto &V : *CR)
        Stack.push_back(V);
      break;
    }

    // ── I32 arithmetic ──────────────────────────────────────────────────
    case OpCode::I32__add:
    case OpCode::I32__sub:
    case OpCode::I32__mul:
    case OpCode::I32__div_s:
    case OpCode::I32__div_u:
    case OpCode::I32__rem_s:
    case OpCode::I32__rem_u:
    case OpCode::I32__and:
    case OpCode::I32__or:
    case OpCode::I32__xor:
    case OpCode::I32__shl:
    case OpCode::I32__shr_s:
    case OpCode::I32__shr_u:
    case OpCode::I32__rotl:
    case OpCode::I32__rotr:
    // I32 relational (produce i32)
    case OpCode::I32__eq:
    case OpCode::I32__ne:
    case OpCode::I32__lt_s:
    case OpCode::I32__lt_u:
    case OpCode::I32__gt_s:
    case OpCode::I32__gt_u:
    case OpCode::I32__le_s:
    case OpCode::I32__le_u:
    case OpCode::I32__ge_s:
    case OpCode::I32__ge_u:
      if (!doBin(Op, OpCode::I32__const))
        return std::nullopt;
      break;

    // ── I32 unary ───────────────────────────────────────────────────────
    case OpCode::I32__clz:
    case OpCode::I32__ctz:
    case OpCode::I32__popcnt:
    case OpCode::I32__eqz:
      if (!doUn(Op, OpCode::I32__const))
        return std::nullopt;
      break;

    // ── I64 arithmetic ──────────────────────────────────────────────────
    case OpCode::I64__add:
    case OpCode::I64__sub:
    case OpCode::I64__mul:
    case OpCode::I64__div_s:
    case OpCode::I64__div_u:
    case OpCode::I64__rem_s:
    case OpCode::I64__rem_u:
    case OpCode::I64__and:
    case OpCode::I64__or:
    case OpCode::I64__xor:
    case OpCode::I64__shl:
    case OpCode::I64__shr_s:
    case OpCode::I64__shr_u:
    case OpCode::I64__rotl:
    case OpCode::I64__rotr:
    // I64 relational (produce i32)
    case OpCode::I64__eq:
    case OpCode::I64__ne:
    case OpCode::I64__lt_s:
    case OpCode::I64__lt_u:
    case OpCode::I64__gt_s:
    case OpCode::I64__gt_u:
    case OpCode::I64__le_s:
    case OpCode::I64__le_u:
    case OpCode::I64__ge_s:
    case OpCode::I64__ge_u:
      if (!doBin(Op, OpCode::I64__const))
        return std::nullopt;
      break;

    // ── I64 unary ───────────────────────────────────────────────────────
    case OpCode::I64__clz:
    case OpCode::I64__ctz:
    case OpCode::I64__popcnt:
    case OpCode::I64__eqz:
      if (!doUn(Op, OpCode::I64__const))
        return std::nullopt;
      break;

    // ── F32 arithmetic + relational ─────────────────────────────────────
    case OpCode::F32__add:
    case OpCode::F32__sub:
    case OpCode::F32__mul:
    case OpCode::F32__div:
    case OpCode::F32__min:
    case OpCode::F32__max:
    case OpCode::F32__copysign:
    case OpCode::F32__eq:
    case OpCode::F32__ne:
    case OpCode::F32__lt:
    case OpCode::F32__gt:
    case OpCode::F32__le:
    case OpCode::F32__ge:
      if (!doBin(Op, OpCode::F32__const))
        return std::nullopt;
      break;

    // ── F32 unary ───────────────────────────────────────────────────────
    case OpCode::F32__abs:
    case OpCode::F32__neg:
    case OpCode::F32__ceil:
    case OpCode::F32__floor:
    case OpCode::F32__trunc:
    case OpCode::F32__nearest:
    case OpCode::F32__sqrt:
      if (!doUn(Op, OpCode::F32__const))
        return std::nullopt;
      break;

    // ── F64 arithmetic + relational ─────────────────────────────────────
    case OpCode::F64__add:
    case OpCode::F64__sub:
    case OpCode::F64__mul:
    case OpCode::F64__div:
    case OpCode::F64__min:
    case OpCode::F64__max:
    case OpCode::F64__copysign:
    case OpCode::F64__eq:
    case OpCode::F64__ne:
    case OpCode::F64__lt:
    case OpCode::F64__gt:
    case OpCode::F64__le:
    case OpCode::F64__ge:
      if (!doBin(Op, OpCode::F64__const))
        return std::nullopt;
      break;

    // ── F64 unary ───────────────────────────────────────────────────────
    case OpCode::F64__abs:
    case OpCode::F64__neg:
    case OpCode::F64__ceil:
    case OpCode::F64__floor:
    case OpCode::F64__trunc:
    case OpCode::F64__nearest:
    case OpCode::F64__sqrt:
      if (!doUn(Op, OpCode::F64__const))
        return std::nullopt;
      break;

    // ── Cast / conversion (input type determines ConstOp) ───────────────
    // i64/f32/f64 → i32
    case OpCode::I32__wrap_i64:
      if (!doUn(Op, OpCode::I64__const))
        return std::nullopt;
      break;
    case OpCode::I32__trunc_f32_s:
    case OpCode::I32__trunc_f32_u:
    case OpCode::I32__trunc_sat_f32_s:
    case OpCode::I32__trunc_sat_f32_u:
      if (!doUn(Op, OpCode::F32__const))
        return std::nullopt;
      break;
    case OpCode::I32__trunc_f64_s:
    case OpCode::I32__trunc_f64_u:
    case OpCode::I32__trunc_sat_f64_s:
    case OpCode::I32__trunc_sat_f64_u:
      if (!doUn(Op, OpCode::F64__const))
        return std::nullopt;
      break;
    // i32/f32/f64 → i64
    case OpCode::I64__extend_i32_s:
    case OpCode::I64__extend_i32_u:
      if (!doUn(Op, OpCode::I32__const))
        return std::nullopt;
      break;
    case OpCode::I64__trunc_f32_s:
    case OpCode::I64__trunc_f32_u:
    case OpCode::I64__trunc_sat_f32_s:
    case OpCode::I64__trunc_sat_f32_u:
      if (!doUn(Op, OpCode::F32__const))
        return std::nullopt;
      break;
    case OpCode::I64__trunc_f64_s:
    case OpCode::I64__trunc_f64_u:
    case OpCode::I64__trunc_sat_f64_s:
    case OpCode::I64__trunc_sat_f64_u:
      if (!doUn(Op, OpCode::F64__const))
        return std::nullopt;
      break;
    // i32/i64 → f32
    case OpCode::F32__convert_i32_s:
    case OpCode::F32__convert_i32_u:
      if (!doUn(Op, OpCode::I32__const))
        return std::nullopt;
      break;
    case OpCode::F32__convert_i64_s:
    case OpCode::F32__convert_i64_u:
      if (!doUn(Op, OpCode::I64__const))
        return std::nullopt;
      break;
    // i32/i64 → f64
    case OpCode::F64__convert_i32_s:
    case OpCode::F64__convert_i32_u:
      if (!doUn(Op, OpCode::I32__const))
        return std::nullopt;
      break;
    case OpCode::F64__convert_i64_s:
    case OpCode::F64__convert_i64_u:
      if (!doUn(Op, OpCode::I64__const))
        return std::nullopt;
      break;
    // float ↔ float
    case OpCode::F32__demote_f64:
      if (!doUn(Op, OpCode::F64__const))
        return std::nullopt;
      break;
    case OpCode::F64__promote_f32:
      if (!doUn(Op, OpCode::F32__const))
        return std::nullopt;
      break;
    // reinterpret
    case OpCode::I32__reinterpret_f32:
      if (!doUn(Op, OpCode::F32__const))
        return std::nullopt;
      break;
    case OpCode::I64__reinterpret_f64:
      if (!doUn(Op, OpCode::F64__const))
        return std::nullopt;
      break;
    case OpCode::F32__reinterpret_i32:
      if (!doUn(Op, OpCode::I32__const))
        return std::nullopt;
      break;
    case OpCode::F64__reinterpret_i64:
      if (!doUn(Op, OpCode::I64__const))
        return std::nullopt;
      break;

    // ── Any other instruction: conservatively bail out ───────────────────
    default:
      return std::nullopt;
    }
  }

  // Fell off the end without an explicit Return or function-End.
  // Collect results from the top of the stack (well-formed Wasm only).
  if (Stack.size() < NumReturns)
    return std::nullopt;
  return std::vector<ValVariant>(Stack.end() - NumReturns, Stack.end());
}

/// Entry point for the mini evaluator: call function AbsFuncIdx with Args.
std::optional<std::vector<ValVariant>>
evalFunc(size_t AbsFuncIdx, std::vector<ValVariant> Args,
         const std::vector<FuncEntry> &FuncTable, AST::Module &Mod,
         size_t ImportedFuncCount, uint32_t &StepsLeft,
         uint32_t DepthLeft) noexcept {
  if (DepthLeft == 0 || StepsLeft == 0)
    return std::nullopt;
  if (AbsFuncIdx >= FuncTable.size())
    return std::nullopt;

  const FuncEntry &Entry = FuncTable[AbsFuncIdx];
  if (!Entry.IsPure || !Entry.FType || Entry.SegIdx == SIZE_MAX)
    return std::nullopt; // imported or impure

  const auto &CodeSeg = Mod.getCodeSection().getContent()[Entry.SegIdx];
  const uint32_t NumReturns =
      static_cast<uint32_t>(Entry.FType->getReturnTypes().size());

  std::vector<ValVariant> Locals =
      buildLocalVec(std::move(Args), CodeSeg.getLocals());

  return evalInstrs(CodeSeg.getExpr().getInstrs(), std::move(Locals),
                    NumReturns, FuncTable, Mod, ImportedFuncCount, StepsLeft,
                    DepthLeft - 1);
}

// ─── Call-site folding pass ──────────────────────────────────────────────────

/// Scan Instrs for Call instructions whose arguments are all known constants
/// (immediately preceding const instructions, possibly interleaved with Nops),
/// evaluate the pure callee, and replace the const+call sequence in-place.
///
/// Returns true if at least one call site was folded.
bool foldCallSites(AST::InstrVec &Instrs,
                   const std::vector<FuncEntry> &FuncTable, AST::Module &Mod,
                   size_t ImportedFuncCount, uint32_t &StepsLeft,
                   uint32_t DepthBudget) noexcept {
  bool Changed = false;
  const int64_t Size = static_cast<int64_t>(Instrs.size());

  for (int64_t I = 0; I < Size; ++I) {
    if (Instrs[static_cast<size_t>(I)].getOpCode() != OpCode::Call)
      continue;

    const uint32_t FuncIdx = Instrs[static_cast<size_t>(I)].getTargetIndex();
    if (FuncIdx >= FuncTable.size())
      continue;

    const FuncEntry &Entry = FuncTable[FuncIdx];
    if (!Entry.IsPure || !Entry.FType)
      continue;

    const uint32_t NumParams =
        static_cast<uint32_t>(Entry.FType->getParamTypes().size());
    const uint32_t NumReturns =
        static_cast<uint32_t>(Entry.FType->getReturnTypes().size());

    // We have (NumParams param-const slots) + (1 call slot) = NumParams+1
    // total. All result consts must fit inside these slots.
    if (NumReturns > NumParams + 1)
      continue;

    // Verify that all return types are representable as const instructions.
    bool AllScalar = true;
    for (const auto &RT : Entry.FType->getReturnTypes())
      if (typeToConstOp(RT) == OpCode::End) {
        AllScalar = false;
        break;
      }
    if (!AllScalar)
      continue;

    // Scan backward from (I-1) for exactly NumParams const instructions,
    // skipping Nops.  Stop at the first non-const, non-Nop instruction.
    std::vector<int64_t> ParamSlots;
    ParamSlots.reserve(NumParams);
    for (int64_t J = I - 1;
         J >= 0 && static_cast<uint32_t>(ParamSlots.size()) < NumParams; --J) {
      const OpCode Jop = Instrs[static_cast<size_t>(J)].getOpCode();
      if (Jop == OpCode::Nop)
        continue;
      if (isConstOp(Jop)) {
        ParamSlots.push_back(J);
        continue;
      }
      break; // any other instruction: can't establish all params are const
    }
    if (static_cast<uint32_t>(ParamSlots.size()) < NumParams)
      continue;

    // ParamSlots[0] = closest to call → reverse for push order (p0..pN-1).
    std::reverse(ParamSlots.begin(), ParamSlots.end());

    // Gather arguments from the const instruction payloads.
    std::vector<ValVariant> Args;
    Args.reserve(NumParams);
    for (int64_t Slot : ParamSlots)
      Args.push_back(Instrs[static_cast<size_t>(Slot)].getNum());

    // Evaluate the pure callee.
    uint32_t DepthLeft = DepthBudget;
    auto Result = evalFunc(FuncIdx, std::move(Args), FuncTable, Mod,
                           ImportedFuncCount, StepsLeft, DepthLeft);
    if (!Result || Result->size() != NumReturns)
      continue;

    // Patch in-place.  Available slots = ParamSlots + call slot (I).
    // Results occupy the first NumReturns slots; remainder become Nops.
    std::vector<int64_t> AllSlots = ParamSlots;
    AllSlots.push_back(I);

    for (uint32_t K = 0; K < NumReturns; ++K) {
      OpCode COP = typeToConstOp(Entry.FType->getReturnTypes()[K]);
      replaceWithConst(Instrs[static_cast<size_t>(AllSlots[K])], COP,
                       (*Result)[K]);
    }
    for (size_t K = NumReturns; K < AllSlots.size(); ++K)
      replaceWithNop(Instrs[static_cast<size_t>(AllSlots[K])]);

    Changed = true;
  }
  return Changed;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Main optimization pass
// ---------------------------------------------------------------------------

void optimizeConstantExpressions(AST::InstrVec &Instrs) {
  if (Instrs.size() < 2) {
    return;
  }

  const auto Size = static_cast<int64_t>(Instrs.size());
  bool Changed = true;

  // Iterate until fixpoint. Each pass can enable new folds when a folded
  // const becomes adjacent to another const or operator after nop-skipping.
  while (Changed) {
    Changed = false;

    for (int64_t I = 0; I < Size; ++I) {
      OpCode Op0 = Instrs[static_cast<size_t>(I)].getOpCode();

      if (!isConstOp(Op0)) {
        continue;
      }

      // Find the next non-Nop instruction after this const.
      int64_t J1 = nextNonNop(Instrs, I + 1, Size);
      if (J1 < 0)
        continue;

      OpCode Op1 = Instrs[static_cast<size_t>(J1)].getOpCode();

      // --- Pattern 1: const + const + binary/relational ---
      // Stack effect: [const A, const B, binop] -> [const result]
      // Cost savings: eliminates 2 dispatches + 2 pushes + 1 pop
      if (isConstOp(Op1)) {
        int64_t J2 = nextNonNop(Instrs, J1 + 1, Size);
        if (J2 >= 0) {
          OpCode Op2 = Instrs[static_cast<size_t>(J2)].getOpCode();
          OpCode ResOp = resultConstOp(Op2);
          if (ResOp != OpCode::End) {
            // Safety check for div/rem
            if (isDivRem(Op2) &&
                !isSafeDivRem(Op2, Instrs[static_cast<size_t>(I)],
                              Instrs[static_cast<size_t>(J1)])) {
              continue;
            }

            auto Result = foldBinary(Op2, Instrs[static_cast<size_t>(I)],
                                     Instrs[static_cast<size_t>(J1)]);
            if (Result) {
              replaceWithConst(Instrs[static_cast<size_t>(I)], ResOp, *Result);
              replaceWithNop(Instrs[static_cast<size_t>(J1)]);
              replaceWithNop(Instrs[static_cast<size_t>(J2)]);
              Changed = true;
              // Back up to re-examine this position (chained folding).
              I = (I > 0) ? I - 1 : -1;
              continue;
            }
          }
        }
      }

      // --- Pattern 2: const + unary/cast ---
      // Stack effect: [const A, unaryop] -> [const result]
      // Cost savings: eliminates 1 dispatch + saves the computation
      {
        OpCode ResOp = unaryResultConstOp(Op0, Op1);
        if (ResOp != OpCode::End) {
          // Safety check for trapping truncation
          if (isTrappingTrunc(Op1) &&
              !isSafeTruncOp(Op1, Instrs[static_cast<size_t>(I)])) {
            continue;
          }

          auto Result = foldUnary(Op1, Instrs[static_cast<size_t>(I)]);
          if (Result) {
            replaceWithConst(Instrs[static_cast<size_t>(I)], ResOp, *Result);
            replaceWithNop(Instrs[static_cast<size_t>(J1)]);
            Changed = true;
            I = (I > 0) ? I - 1 : -1;
            continue;
          }
        }
      }

      // --- Pattern 3: const(identity) + binop -> nop + nop ---
      // Detects right-identity patterns where the const is the RHS of a
      // commutative-safe operation. E.g., [X, i32.const 0, i32.add] -> [X].
      // Cost savings: eliminates 1 push + 1 dispatch(const) + 1 pop + 1
      //   dispatch(binop) = net 5 cost units removed, 2 nops added (cost 2).
      if (isRightIdentity(Op0, Instrs[static_cast<size_t>(I)], Op1)) {
        replaceWithNop(Instrs[static_cast<size_t>(I)]);
        replaceWithNop(Instrs[static_cast<size_t>(J1)]);
        Changed = true;
        continue;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Module-level IPCP pass
// ---------------------------------------------------------------------------

void optimizeModuleConstantExpressions(AST::Module &Mod, uint32_t StepBudget,
                                       uint32_t DepthBudget) noexcept {
  // Build per-function purity table and collect imported function count.
  auto FuncTable = buildFuncTable(Mod);

  size_t ImportedFuncCount = 0;
  for (const auto &Imp : Mod.getImportSection().getContent())
    if (Imp.getExternalType() == ExternalType::Function)
      ++ImportedFuncCount;

  // Step budget shared across all call-site evaluations in this module.
  //
  // Cross-project reference for comparable limits at each optimization level:
  //
  //   O0 (off):     Clang -O0, Luau -O0, V8 --no-opt: no folding at all.
  //   O1 (basic):   Clang -O1 runs InstCombine (1 iteration, LLVM 18+).
  //                 Luau -O1: LuauCompileInlineThreshold = 25.
  //                 GCC -O1: ipa-cp-eval-threshold = 500.
  //                 Budget: 100K steps — enough for simple constant
  //                 propagation without deep recursion evaluation.
  //   O2 (default): Clang -O2 runs IPSCCP + InstCombine.
  //                 Luau -O2: same thresholds, but unrolling enabled.
  //                 GCC -O2: same IPA-CP thresholds as -O1.
  //                 Budget: 1M steps — evaluates moderate recursion.
  //   O3 (full):    Clang -O3 adds FuncSpec (funcspec-max-iters = 10).
  //                 GCC -O3: ipa-cp-max-recursive-depth = 8.
  //                 Cranelift: max 5 chained rewrites (hardcoded).
  //                 Hermes: no explicit budget (fixed pass pipeline).
  //                 Budget: 4M steps — evaluates fib(30), fac(20), etc.
  //   Os/Oz:        Same as O2 (favour code size; IPCP with nop-fill does
  //                 not increase code size, so no reason to restrict).
  //
  // The budget is a safety valve against pathological recursion or loop
  // iteration counts, not a tuning knob for optimization quality.
  uint32_t StepsLeft = StepBudget;

  auto &Segments = Mod.getCodeSection().getContent();

  // Arithmetic folding runs unconditionally (even at StepBudget=0 the
  // per-function optimizeConstantExpressions pass is worthwhile).
  // IPCP call-site folding only runs when StepBudget > 0.
  for (auto &Seg : Segments) {
    AST::InstrVec &Instrs = Seg.getExpr().getInstrs();

    // Always run at least one arithmetic fold pass.
    optimizeConstantExpressions(Instrs);

    // IPCP: alternate arithmetic folding with call-site folding until
    // neither pass makes progress.  This enables patterns like:
    //   i32.const 10   -> foldCallSites folds fac(10)  -> i32.const 3628800
    if (StepsLeft > 0) {
      bool Changed = true;
      while (Changed) {
        Changed = foldCallSites(Instrs, FuncTable, Mod, ImportedFuncCount,
                                StepsLeft, DepthBudget);
        if (Changed)
          optimizeConstantExpressions(Instrs);
        if (StepsLeft == 0)
          break;
      }
    }
  }
}

} // namespace Executor
} // namespace WasmEdge
