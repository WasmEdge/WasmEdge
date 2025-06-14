// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#pragma once

#include "host/wasi/environ.h"
#include "wasi/api.hpp"
#include <cctype>
#include <cstdint>
#include <ios>
#include <string>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Host {

namespace API {

class WasmEdgeIfstream {
public:
  WasmEdgeIfstream(const Host::WASI::Environ &Env,
                   const std::string_view &FileName) noexcept;
  ~WasmEdgeIfstream();

};

class WasmEdgeOfstream {
public:
  WasmEdgeOfstream(const Host::WASI::Environ &Env,
                   const std::string_view &FileName) noexcept;
  ~WasmEdgeOfstream();

};



} // namespace API
} // namespace Host
} // namespace WasmEdge