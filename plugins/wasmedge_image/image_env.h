// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImage {

enum class ErrNo : uint32_t {
  Success = 0, // No error occurred.
  Fail = 1,    // Runtime Error.
};

enum class DataType : uint32_t {
  RGB8 = 0,
  BGR8 = 1,
  RGB32F = 2,
  BGR32F = 3,
};

struct ImgEnv {};

} // namespace WasmEdgeImage
} // namespace Host
} // namespace WasmEdge
