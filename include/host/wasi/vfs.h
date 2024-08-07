// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/filesystem.h"
#include "host/wasi/error.h"
#include "wasi/api.hpp"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASI {

class VFS {
public:
  /// Flags for open path
  enum Flags : uint8_t {
    Read = 1,       ///< Open for read.
    Write = 2,      ///< Open for write.
    AllowEmpty = 4, ///< Allow empty path for self reference.
  };
};
DEFINE_ENUM_OPERATORS(VFS::Flags)

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
