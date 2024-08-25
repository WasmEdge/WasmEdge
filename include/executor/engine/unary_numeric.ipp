// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/roundeven.h"
#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T> TypeU<T> Executor::runClzOp(ValVariant &Val) const {
  T I = Val.get<T>();
  // Return the count of leading zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U) << (sizeof(T) * 8 - 1);
    while ((I & Mask) == 0U) {
      Cnt++;
      I <<= 1;
    }
    Val.get<T>() = Cnt;
  } else {
    Val.get<T>() = static_cast<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T> TypeU<T> Executor::runCtzOp(ValVariant &Val) const {
  T I = Val.get<T>();
  // Return the count of trailing zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U);
    while ((I & Mask) == 0U) {
      Cnt++;
      I >>= 1;
    }
    Val.get<T>() = Cnt;
  } else {
    Val.get<T>() = static_cast<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T> TypeU<T> Executor::runPopcntOp(ValVariant &Val) const {
  T I = Val.get<T>();
  // Return the count of non-zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U);
    while (I != 0U) {
      if (I & Mask) {
        Cnt++;
      }
      I >>= 1;
    }
    Val.get<T>() = Cnt;
  }
  return {};
}

template <typename T> TypeF<T> Executor::runAbsOp(ValVariant &Val) const {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  // In MSVC, std::fabs cannot be used. If input is NAN, std::fabs will set the
  // highest bit of fraction.
  T &Fp = Val.get<T>();
  static_assert(std::is_floating_point_v<T>);
  if constexpr (sizeof(T) == 4) {
    uint32_t Tmp = reinterpret_cast<uint32_t &>(Fp) & UINT32_C(0x7fffffff);
    Val.get<T>() = reinterpret_cast<T &>(Tmp);
  } else {
    uint64_t Tmp =
        reinterpret_cast<uint64_t &>(Fp) & UINT64_C(0x7fffffffffffffff);
    Val.get<T>() = reinterpret_cast<T &>(Tmp);
  }
#else
  Val.get<T>() = std::fabs(Val.get<T>());
#endif
  return {};
}

template <typename T> TypeF<T> Executor::runNegOp(ValVariant &Val) const {
  Val.get<T>() = -Val.get<T>();
  return {};
}

template <typename T> TypeF<T> Executor::runCeilOp(ValVariant &Val) const {
  Val.get<T>() = std::ceil(Val.get<T>());
  return {};
}

template <typename T> TypeF<T> Executor::runFloorOp(ValVariant &Val) const {
  Val.get<T>() = std::floor(Val.get<T>());
  return {};
}

template <typename T> TypeF<T> Executor::runTruncOp(ValVariant &Val) const {
  Val.get<T>() = std::trunc(Val.get<T>());
  return {};
}

template <typename T> TypeF<T> Executor::runNearestOp(ValVariant &Val) const {
  Val.get<T>() = WasmEdge::roundeven(Val.get<T>());
  return {};
}

template <typename T> TypeF<T> Executor::runSqrtOp(ValVariant &Val) const {
  Val.get<T>() = std::sqrt(Val.get<T>());
  return {};
}

} // namespace Executor
} // namespace WasmEdge

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#include "executor/engine/unary_numeric_vector_msvc.ipp"
#else
#include "executor/engine/unary_numeric_vector.ipp"
#endif // MSVC
