// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace Host {

class WasiHttpEnvironment {
public:
  WasiHttpEnvironment() noexcept;

  std::string_view loadURI(uint64_t URIIndex) { return URIs[URIIndex]; }
  std::vector<std::string> Bodies;

private:
  std::vector<std::string> URIs;
};

} // namespace Host
} // namespace WasmEdge
