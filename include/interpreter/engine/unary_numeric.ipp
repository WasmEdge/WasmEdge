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
                                           const uint8_t Index) const {
  using VTIn [[gnu::vector_size(16)]] = TIn;
  VTIn &Result = reinterpret_cast<VTIn &>(retrieveValue<uint64x2_t>(Val));
  retrieveValue<TOut>(Val) = Result[Index];
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runSplatOp(ValVariant &Val) const {
  const TOut Part = static_cast<TOut>(retrieveValue<TIn>(Val));
  using VTOut [[gnu::vector_size(16)]] = TOut;
  VTOut &Result = reinterpret_cast<VTOut &>(retrieveValue<uint128_t>(Val));
  if constexpr (sizeof(TOut) == 1) {
    const VTOut V = {Part, Part, Part, Part, Part, Part, Part, Part,
                     Part, Part, Part, Part, Part, Part, Part, Part};
    Result = V;
  } else if constexpr (sizeof(TOut) == 2) {
    const VTOut V = {Part, Part, Part, Part, Part, Part, Part, Part};
    Result = V;
  } else if constexpr (sizeof(TOut) == 4) {
    const VTOut V = {Part, Part, Part, Part};
    Result = V;
  } else if constexpr (sizeof(TOut) == 8) {
    const VTOut V = {Part, Part};
    Result = V;
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorWidenLowOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V =
      reinterpret_cast<const VTIn &>(retrieveValue<uint64x2_t>(Val));
  VTOut &Result = reinterpret_cast<VTOut &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(TIn) == 1) {
    HVTIn Half = {V[0], V[1], V[2], V[3], V[4], V[5], V[6], V[7]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 2) {
    HVTIn Half = {V[0], V[1], V[2], V[3]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 4) {
    HVTIn Half = {V[0], V[1]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 8) {
    HVTIn Half = {V[0]};
    Result = __builtin_convertvector(Half, VTOut);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorWidenHighOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V =
      reinterpret_cast<const VTIn &>(retrieveValue<uint64x2_t>(Val));
  VTOut &Result = reinterpret_cast<VTOut &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(TIn) == 1) {
    HVTIn Half = {V[8], V[9], V[10], V[11], V[12], V[13], V[14], V[15]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 2) {
    HVTIn Half = {V[4], V[5], V[6], V[7]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 4) {
    HVTIn Half = {V[2], V[3]};
    Result = __builtin_convertvector(Half, VTOut);
  } else if constexpr (sizeof(TIn) == 8) {
    HVTIn Half = {V[1]};
    Result = __builtin_convertvector(Half, VTOut);
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAbsOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
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
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  Result = -Result;
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorSqrtOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(T) == 4) {
    const VT V = {std::sqrt(Result[0]), std::sqrt(Result[1]),
                  std::sqrt(Result[2]), std::sqrt(Result[3])};
    Result = V;
  } else if constexpr (sizeof(T) == 8) {
    const VT V = {std::sqrt(Result[0]), std::sqrt(Result[1])};
    Result = V;
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorTruncSatOp(ValVariant &Val) const {
  static_assert(sizeof(TIn) == sizeof(TOut));
  const TIn FMin = std::numeric_limits<TOut>::min();
  const TIn FMax = std::numeric_limits<TOut>::max();
  const TOut IMin = std::numeric_limits<TOut>::min();
  const TOut IMax = std::numeric_limits<TOut>::max();
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  auto &V = reinterpret_cast<const VTIn &>(retrieveValue<uint64x2_t>(Val));
  auto &Result = reinterpret_cast<VTOut &>(retrieveValue<uint64x2_t>(Val));
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
  auto &V = reinterpret_cast<const VTIn &>(retrieveValue<uint64x2_t>(Val));
  auto &Result = reinterpret_cast<VTOut &>(retrieveValue<uint64x2_t>(Val));
  Result = __builtin_convertvector(V, VTOut);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAnyTrueOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Vector = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
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
  VT &Vector = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
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
  SVT &Vector = reinterpret_cast<SVT &>(retrieveValue<uint64x2_t>(Val));
  const SVT MSB = Vector < 0;
  const UVT Z = reinterpret_cast<UVT>(MSB);
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
    using uint16x8_t [[gnu::vector_size(16)]] = uint16_t;
    const uint16x8_t Mask = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    uint16x8_t X = {Z[0], Z[1], Z[2], Z[3], Z[4], Z[5], Z[6], Z[7]};
    X &= Mask;
    using uint8x8_t [[gnu::vector_size(8)]] = uint8_t;
    const uint8x8_t V = __builtin_convertvector(X, uint8x8_t);
    const uint8_t Result =
        V[0] | V[1] | V[2] | V[3] | V[4] | V[5] | V[6] | V[7];
    retrieveValue<uint32_t>(Val) = Result;
  } else if constexpr (sizeof(T) == 4) {
    using uint32x4_t [[gnu::vector_size(16)]] = uint32_t;
    const uint32x4_t Mask = {0x1, 0x2, 0x4, 0x8};
    uint32x4_t X = {Z[0], Z[1], Z[2], Z[3]};
    X &= Mask;
    using uint8x4_t [[gnu::vector_size(4)]] = uint8_t;
    const uint8x4_t V = __builtin_convertvector(X, uint8x4_t);
    const uint8_t Result = V[0] | V[1] | V[2] | V[3];
    retrieveValue<uint32_t>(Val) = Result;
  } else if constexpr (sizeof(T) == 8) {
    using uint64x2_t [[gnu::vector_size(16)]] = uint64_t;
    const uint64x2_t Mask = {0x1, 0x2};
    uint64x2_t X = {Z[0], Z[1]};
    X &= Mask;
    using uint8x2_t [[gnu::vector_size(2)]] = uint8_t;
    const uint8x2_t V = __builtin_convertvector(X, uint8x2_t);
    const uint8_t Result = V[0] | V[1];
    retrieveValue<uint32_t>(Val) = Result;
  }

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorCeilOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(T) == 4) {
    const VT V = {std::ceil(Result[0]), std::ceil(Result[1]),
                  std::ceil(Result[2]), std::ceil(Result[3])};
    Result = V;
  } else if constexpr (sizeof(T) == 8) {
    const VT V = {std::ceil(Result[0]), std::ceil(Result[1])};
    Result = V;
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorFloorOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(T) == 4) {
    const VT V = {std::floor(Result[0]), std::floor(Result[1]),
                  std::floor(Result[2]), std::floor(Result[3])};
    Result = V;
  } else if constexpr (sizeof(T) == 8) {
    const VT V = {std::floor(Result[0]), std::floor(Result[1])};
    Result = V;
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorTruncOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(T) == 4) {
    const VT V = {std::trunc(Result[0]), std::trunc(Result[1]),
                  std::trunc(Result[2]), std::trunc(Result[3])};
    Result = V;
  } else if constexpr (sizeof(T) == 8) {
    const VT V = {std::trunc(Result[0]), std::trunc(Result[1])};
    Result = V;
  }
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorNearestOp(ValVariant &Val) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &Result = reinterpret_cast<VT &>(retrieveValue<uint64x2_t>(Val));
  if constexpr (sizeof(T) == 4) {
    const VT V = {SSVM::roundeven(Result[0]), SSVM::roundeven(Result[1]),
                  SSVM::roundeven(Result[2]), SSVM::roundeven(Result[3])};
    Result = V;
  } else if constexpr (sizeof(T) == 8) {
    const VT V = {SSVM::roundeven(Result[0]), SSVM::roundeven(Result[1])};
    Result = V;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
