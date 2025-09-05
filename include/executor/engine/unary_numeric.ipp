// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/roundeven.h"
#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeU<T> Executor::runClzOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.peekTop<T>();
  // Return the count of leading zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U) << (sizeof(T) * 8 - 1);
    while ((I & Mask) == 0U) {
      Cnt++;
      I <<= 1;
    }
    StackMgr.emplaceTop(Cnt);
  } else {
    StackMgr.emplaceTop<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runCtzOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.peekTop<T>();
  // Return the count of trailing zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = static_cast<T>(0x1U);
    while ((I & Mask) == 0U) {
      Cnt++;
      I >>= 1;
    }
    StackMgr.emplaceTop(Cnt);
  } else {
    StackMgr.emplaceTop<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runPopcntOp(Runtime::StackManager &StackMgr) const noexcept {
  T I = StackMgr.peekTop<T>();
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
    StackMgr.emplaceTop(Cnt);
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runAbsOp(Runtime::StackManager &StackMgr) const noexcept {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
  // In MSVC, std::fabs cannot be used. If input is NAN, std::fabs will set the
  // highest bit of fraction.
  T Fp = StackMgr.peekTop<T>();
  static_assert(std::is_floating_point_v<T>);
  if constexpr (sizeof(T) == 4) {
    uint32_t Tmp = reinterpret_cast<uint32_t &>(Fp) & UINT32_C(0x7fffffff);
    StackMgr.emplaceTop<T>(reinterpret_cast<T &&>(Tmp));
  } else {
    uint64_t Tmp =
        reinterpret_cast<uint64_t &>(Fp) & UINT64_C(0x7fffffffffffffff);
    StackMgr.emplaceTop<T>(reinterpret_cast<T &&>(Tmp));
  }
#else
  StackMgr.emplaceTop<T>(std::fabs(StackMgr.peekTop<T>()));
#endif
  return {};
}

template <typename T>
TypeF<T> Executor::runNegOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(-StackMgr.peekTop<T>());
  return {};
}

template <typename T>
TypeF<T> Executor::runCeilOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(std::ceil(StackMgr.peekTop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runFloorOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(std::floor(StackMgr.peekTop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runTruncOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(std::trunc(StackMgr.peekTop<T>()));
  return {};
}

template <typename T>
TypeF<T>
Executor::runNearestOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(WasmEdge::roundeven(StackMgr.peekTop<T>()));
  return {};
}

template <typename T>
TypeF<T> Executor::runSqrtOp(Runtime::StackManager &StackMgr) const noexcept {
  StackMgr.emplaceTop<T>(std::sqrt(StackMgr.peekTop<T>()));
  return {};
}

} // namespace Executor
} // namespace WasmEdge

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#include "executor/engine/unary_numeric_vector_msvc.ipp"
#else
#include "executor/engine/unary_numeric_vector.ipp"
#endif // MSVC
