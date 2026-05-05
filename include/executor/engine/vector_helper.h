// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <cmath>
#include <cstring>
#include <type_traits>

namespace WasmEdge {
namespace Executor {
namespace detail {

template <typename T>
[[gnu::always_inline]] inline void canonicalize(T &V) noexcept {
  if constexpr (std::is_floating_point_v<T>) {
    if (std::isnan(V)) {
      if constexpr (sizeof(T) == 4) {
        const uint32_t I32 = 0x7fc00000;
        std::memcpy(&V, &I32, 4);
      } else {
        const uint64_t I64 = 0x7ff8000000000000;
        std::memcpy(&V, &I64, 8);
      }
    }
  }
}

template <typename U, typename V>
[[gnu::always_inline]] constexpr inline std::enable_if_t<
    !std::is_arithmetic_v<V>, V>
vectorSelect(U Cond, V A, V B) noexcept {
#if defined(__clang__)
  return reinterpret_cast<V>((Cond & reinterpret_cast<U>(A)) |
                             (~Cond & reinterpret_cast<U>(B)));
#else
  return Cond ? A : B;
#endif
}

template <typename T>
[[gnu::always_inline]] inline void vectorCanonicalize(T &V) noexcept {
  using ElementType = std::decay_t<decltype(V[0])>;
  if constexpr (std::is_floating_point_v<ElementType>) {
#if defined(_MSC_VER) && !defined(__clang__)
    for (size_t I = 0; I < V.size(); ++I) {
      if (std::isnan(V[I])) {
        if constexpr (sizeof(ElementType) == 4) {
          const uint32_t I32 = 0x7fc00000;
          std::memcpy(&V[I], &I32, 4);
        } else {
          const uint64_t I64 = 0x7ff8000000000000;
          std::memcpy(&V[I], &I64, 8);
        }
      }
    }
#else
    using VT [[gnu::vector_size(16)]] = ElementType;
    using UVT [[gnu::vector_size(16)]] =
        std::conditional_t<sizeof(ElementType) == 4, uint32_t, uint64_t>;
    const UVT IsNan = reinterpret_cast<UVT>(V != V);
    ElementType Nan;
    if constexpr (sizeof(ElementType) == 4) {
      const uint32_t I32 = 0x7fc00000;
      std::memcpy(&Nan, &I32, 4);
    } else {
      const uint64_t I64 = 0x7ff8000000000000;
      std::memcpy(&Nan, &I64, 8);
    }
    V = vectorSelect(IsNan, VT{} + Nan, V);
#endif
  }
}

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
