// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "experimental/expected.hpp"

namespace WasmEdge {

/// Type aliasing of expected class.
template <typename T, typename E> using Expected = cxx20::expected<T, E>;

/// Type aliasing of unexpected class.
template <typename E> using Unexpected = cxx20::unexpected<E>;

} // namespace WasmEdge
