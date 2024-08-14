// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <type_traits>

namespace WasmEdge {
namespace Executor {
namespace detail {

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

} // namespace detail
} // namespace Executor
} // namespace WasmEdge
