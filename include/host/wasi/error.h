// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/expected.h"
#include "wasi/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASI {

/// Type aliasing for Expected<T, __wasi_errno_t>.
template <typename T> using WasiExpect = Expected<T, __wasi_errno_t>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto WasiUnexpect(__wasi_errno_t Val) {
  return Unexpected<__wasi_errno_t>(Val);
}
template <typename T> constexpr auto WasiUnexpect(const WasiExpect<T> &Val) {
  return Unexpected<__wasi_errno_t>(Val.error());
}

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
