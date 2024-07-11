// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <cstdint>

namespace WasmEdge::Host::WASILLM {

enum class ErrNo : uint32_t {
  Success = 0,
  InvalidArgument = 1,
};

} // namespace WasmEdge::Host::WASILLM
