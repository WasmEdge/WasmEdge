// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "plugin/plugin.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <zlib.h>

/**
 * @brief A struct which maps perfectly to a wasm 32bit z_stream object
 *
 */
struct Wasm_z_stream {
  uint32_t next_in;
  uint32_t avail_in;
  uint32_t total_in;

  uint32_t next_out;
  uint32_t avail_out;
  uint32_t total_out;

  uint32_t msg;
  uint32_t state;

  uint32_t zalloc;
  uint32_t zfree;
  uint32_t opaque;

  int32_t data_type; // +ve & 2's complement in memory so int ~ uint

  uint32_t adler;
  uint32_t reserved;
}; // 56 bytes

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibEnvironment {
public:
  std::unordered_map<uint32_t, std::unique_ptr<z_stream>> ZStreamMap;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
