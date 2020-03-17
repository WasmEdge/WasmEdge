// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "experimental/expected.hpp"

namespace SSVM {
namespace Support {

/// Type aliasing of expected class.
template <typename T, typename E>
using Expected = std::experimental::expected<T, E>;

/// Type aliasing of unexpected class.
template <typename E> using Unexpected = std::experimental::unexpected<E>;

} // namespace Support
} // namespace SSVM