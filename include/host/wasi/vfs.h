// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/filesystem.h"
#include "host/wasi/error.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASI {

class VINode;
class VFS {
public:
  VFS(const VFS &) = delete;
  VFS &operator=(const VFS &) = delete;
  VFS(VFS &&) = default;
  VFS &operator=(VFS &&) = default;

  VFS() = default;

  /// Flags for open path
  enum VFSFlags : uint8_t {
    Read = 1,       ///< Open for read.
    Write = 2,      ///< Open for write.
    AllowEmpty = 4, ///< Allow empty path for self reference.
  };
};

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
