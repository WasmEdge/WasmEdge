// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "common/defines.h"
#include "common/errcode.h"
#include "common/span.h"

namespace WasmEdge {
namespace Host {

namespace ComponentModel {

using String = std::tuple</* address */ uint32_t, /* length */ uint32_t>;

// template <typename T>
using List = std::tuple</* address */ uint32_t, /* length */ uint32_t>;

} // namespace ComponentModel

using Pollable = uint32_t;

namespace WASIPreview2 {

class Environ {
public:
  void getPollable(Pollable /*Id*/) {}

  ~Environ() noexcept;
};

} // namespace WASIPreview2
} // namespace Host
} // namespace WasmEdge
