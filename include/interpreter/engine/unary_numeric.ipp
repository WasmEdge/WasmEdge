// SPDX-License-Identifier: Apache-2.0
#include "common/roundeven.h"
#include "common/value.h"
#include "interpreter/interpreter.h"
#include <cmath>

namespace SSVM {
namespace Interpreter {

template <typename T> TypeU<T> Interpreter::runClzOp(ValVariant &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of leading zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U << (sizeof(T) * 8 - 1);
    while ((I & Mask) == 0U) {
      Cnt++;
      I <<= 1;
    }
    Val = Cnt;
  } else {
    Val = static_cast<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T> TypeU<T> Interpreter::runCtzOp(ValVariant &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of trailing zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U;
    while ((I & Mask) == 0U) {
      Cnt++;
      I >>= 1;
    }
    Val = Cnt;
  } else {
    Val = static_cast<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T> TypeU<T> Interpreter::runPopcntOp(ValVariant &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of non-zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U;
    while (I != 0U) {
      if (I & Mask) {
        Cnt++;
      }
      I >>= 1;
    }
    Val = Cnt;
  }
  return {};
}

template <typename T> TypeF<T> Interpreter::runAbsOp(ValVariant &Val) const {
  Val = std::fabs(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runNegOp(ValVariant &Val) const {
  Val = -retrieveValue<T>(Val);
  return {};
}

template <typename T> TypeF<T> Interpreter::runCeilOp(ValVariant &Val) const {
  Val = std::ceil(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runFloorOp(ValVariant &Val) const {
  Val = std::floor(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runTruncOp(ValVariant &Val) const {
  Val = std::trunc(retrieveValue<T>(Val));
  return {};
}

template <typename T>
TypeF<T> Interpreter::runNearestOp(ValVariant &Val) const {
  Val = SSVM::roundeven(T(retrieveValue<T>(Val)));
  return {};
}

template <typename T> TypeF<T> Interpreter::runSqrtOp(ValVariant &Val) const {
  Val = std::sqrt(retrieveValue<T>(Val));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runExtractLaneOp(ValVariant &Val,
                                           const uint32_t Index) const {
  using VTIn [[gnu::vector_size(16)]] = TIn;
  VTIn &Result = retrieveValue<VTIn>(Val);
  retrieveValue<TOut>(Val) = Result[Index];
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runSplatOp(ValVariant &Val) const {
  const TOut Part = static_cast<TOut>(retrieveValue<TIn>(Val));
  using VTOut [[gnu::vector_size(16)]] = TOut;
  VTOut &Result = retrieveValue<VTOut>(Val);
  if constexpr (sizeof(TOut) == 1) {
    Result = VTOut{Part, Part, Part, Part, Part, Part, Part, Part,
                   Part, Part, Part, Part, Part, Part, Part, Part};
  } else if constexpr (sizeof(TOut) == 2) {
    Result = VTOut{Part, Part, Part, Part, Part, Part, Part, Part};
  } else if constexpr (sizeof(TOut) == 4) {
    Result = VTOut{Part, Part, Part, Part};
  } else if constexpr (sizeof(TOut) == 8) {
    Result = VTOut{Part, Part};
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorWidenLowOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V = retrieveValue<VTIn>(Val);
  VTOut &Result = retrieveValue<VTOut>(Val);
  if constexpr (sizeof(TIn) == 1) {
    Result = __builtin_convertvector(
        HVTIn{V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7]}, VTOut);
  } else if constexpr (sizeof(TIn) == 2) {
    Result = __builtin_convertvector(HVTIn{V[0], V[1], V[2], V[3]}, VTOut);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorWidenHighOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V = retrieveValue<VTIn>(Val);
  VTOut &Result = retrieveValue<VTOut>(Val);
  if constexpr (sizeof(TIn) == 1) {
    Result = __builtin_convertvector(
        HVTIn{V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]}, VTOut);
  } else if constexpr (sizeof(TIn) == 2) {
    Result = __builtin_convertvector(HVTIn{V[4], V[5], V[6], V[7]}, VTOut);
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAbsOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (std::is_floating_point_v<T>) {
    if constexpr (sizeof(T) == 4) {
      using IVT [[gnu::vector_size(16)]] = uint32_t;
      IVT Mask = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    } else {
      using IVT [[gnu::vector_size(16)]] = uint64_t;
      IVT Mask = {UINT64_C(0x7fffffffffffffff), UINT64_C(0x7fffffffffffffff)};
      Result = reinterpret_cast<VT>(reinterpret_cast<IVT>(Result) & Mask);
    }
  } else {
    Result = Result > 0 ? Result : -Result;
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorNegOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  Result = -Result;
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorSqrtOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1]),
                std::sqrt(Result[2]), std::sqrt(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::sqrt(Result[0]), std::sqrt(Result[1])};
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorTruncSatOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) == sizeof(TOut));
  const TIn FMin = static_cast<TIn>(std::numeric_limits<TOut>::min());
  const TIn FMax = static_cast<TIn>(std::numeric_limits<TOut>::max());
  const TOut IMin = std::numeric_limits<TOut>::min();
  const TOut IMax = std::numeric_limits<TOut>::max();
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  auto &V = retrieveValue<VTIn>(Val);
  auto &Result = retrieveValue<VTOut>(Val);
  if constexpr (sizeof(TIn) == 4) {
    VTIn X = {std::trunc(V[0]), std::trunc(V[1]), std::trunc(V[2]),
              std::trunc(V[3])};
    VTOut Y = __builtin_convertvector(X, VTOut);
    Y = X == X ? Y : 0;
    Y = X <= FMin ? IMin : Y;
    Y = X >= FMax ? IMax : Y;
    Result = Y;
  } else if constexpr (sizeof(TIn) == 8) {
    VTIn X = {std::trunc(V[0]), std::trunc(V[1])};
    VTOut Y = __builtin_convertvector(X, VTOut);
    Y = X <= FMin ? IMin : Y;
    Y = X >= FMax ? IMax : Y;
    Result = Y;
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorConvertOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) == sizeof(TOut));
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  auto &V = retrieveValue<VTIn>(Val);
  auto &Result = retrieveValue<VTOut>(Val);
  Result = __builtin_convertvector(V, VTOut);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAnyTrueOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Vector = retrieveValue<VT>(Val);
  VT Z = Vector != 0;
  uint32_t Result;
  if constexpr (sizeof(T) == 1) {
    Result = Z[0] || Z[1] || Z[2] || Z[3] || Z[4] || Z[5] || Z[6] || Z[7] ||
             Z[8] || Z[9] || Z[10] || Z[11] || Z[12] || Z[13] || Z[14] || Z[15];
  } else if constexpr (sizeof(T) == 2) {
    Result = Z[0] || Z[1] || Z[2] || Z[3] || Z[4] || Z[5] || Z[6] || Z[7];
  } else if constexpr (sizeof(T) == 4) {
    Result = Z[0] || Z[1] || Z[2] || Z[3];
  } else if constexpr (sizeof(T) == 8) {
    Result = Z[0] || Z[1];
  }
  retrieveValue<uint32_t>(Val) = Result;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAllTrueOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Vector = retrieveValue<VT>(Val);
  VT Z = Vector != 0;
  uint32_t Result;
  if constexpr (sizeof(T) == 1) {
    Result = Z[0] && Z[1] && Z[2] && Z[3] && Z[4] && Z[5] && Z[6] && Z[7] &&
             Z[8] && Z[9] && Z[10] && Z[11] && Z[12] && Z[13] && Z[14] && Z[15];
  } else if constexpr (sizeof(T) == 2) {
    Result = Z[0] && Z[1] && Z[2] && Z[3] && Z[4] && Z[5] && Z[6] && Z[7];
  } else if constexpr (sizeof(T) == 4) {
    Result = Z[0] && Z[1] && Z[2] && Z[3];
  } else if constexpr (sizeof(T) == 8) {
    Result = Z[0] && Z[1];
  }
  retrieveValue<uint32_t>(Val) = Result;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorBitMaskOp(ValVariant &Val) const {
  using SVT [[gnu::vector_size(16)]] = std::make_signed_t<T>;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  SVT &Vector = retrieveValue<SVT>(Val);
  const SVT MSB = Vector < 0;
  const UVT Z [[maybe_unused]] = reinterpret_cast<UVT>(MSB);
  if constexpr (sizeof(T) == 1) {
    using int16x16_t [[gnu::vector_size(32)]] = int16_t;
    using uint16x16_t [[gnu::vector_size(32)]] = uint16_t;
    const uint16x16_t Mask = {0x1,    0x2,    0x4,    0x8,   0x10,  0x20,
                              0x40,   0x80,   0x100,  0x200, 0x400, 0x800,
                              0x1000, 0x2000, 0x4000, 0x8000};
    uint16x16_t V =
        reinterpret_cast<uint16x16_t>(__builtin_convertvector(MSB, int16x16_t));
    V &= Mask;
    const uint16_t Result = V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] |
                            V[7] | V[8] | V[9] | V[10] | V[11] | V[12] | V[13] |
                            V[14] | V[15];
    retrieveValue<uint32_t>(Val) = Result;
  } else if constexpr (sizeof(T) == 2) {
    const uint16x8_t Mask = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    using uint8x8_t [[gnu::vector_size(8)]] = uint8_t;
    const uint8x8_t V = __builtin_convertvector(Z & Mask, uint8x8_t);
    const uint8_t Result =
        V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] | V[7];
    retrieveValue<uint32_t>(Val) = Result;
  } else if constexpr (sizeof(T) == 4) {
    const uint32x4_t Mask = {0x1, 0x2, 0x4, 0x8};
    using uint8x4_t [[gnu::vector_size(4)]] = uint8_t;
    const uint8x4_t V = __builtin_convertvector(Z & Mask, uint8x4_t);
    const uint8_t Result = V[0] | V[1] | V[2] | V[3];
    retrieveValue<uint32_t>(Val) = Result;
  } else if constexpr (sizeof(T) == 8) {
    const uint64x2_t Mask = {0x1, 0x2};
    using uint8x2_t [[gnu::vector_size(2)]] = uint8_t;
    const uint8x2_t V = __builtin_convertvector(Z & Mask, uint8x2_t);
    const uint8_t Result = V[0] | V[1];
    retrieveValue<uint32_t>(Val) = Result;
  }

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorCeilOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1]),
                std::ceil(Result[2]), std::ceil(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::ceil(Result[0]), std::ceil(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorFloorOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1]),
                std::floor(Result[2]), std::floor(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::floor(Result[0]), std::floor(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorTruncOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 4) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1]),
                std::trunc(Result[2]), std::trunc(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{std::trunc(Result[0]), std::trunc(Result[1])};
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorNearestOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = retrieveValue<VT>(Val);
  if constexpr (sizeof(T) == 4) {
    Result = VT{SSVM::roundeven(Result[0]), SSVM::roundeven(Result[1]),
                SSVM::roundeven(Result[2]), SSVM::roundeven(Result[3])};
  } else if constexpr (sizeof(T) == 8) {
    Result = VT{SSVM::roundeven(Result[0]), SSVM::roundeven(Result[1])};
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
