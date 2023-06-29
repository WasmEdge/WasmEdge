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
  /* [Wasm Offset] next input byte */
  uint32_t next_in;
  /* number of bytes available at next_in */
  uint32_t avail_in;
  /* total number of input bytes read so far */
  uint32_t total_in;

  /* [Wasm Offset] next output byte will go here */
  uint32_t next_out;
  /* remaining free space at next_out */
  uint32_t avail_out;
  /* total number of bytes output so far */
  uint32_t total_out;

  /* [Wasm Offset] last error message, NULL if no error */
  uint32_t msg;
  /* [Wasm Offset] not visible by applications */
  uint32_t state;

  /* used to allocate the internal state */
  uint32_t zalloc;
  /* used to free the internal state */
  uint32_t zfree;
  /* [Wasm Offset] private data object passed to zalloc and zfree */
  uint32_t opaque;

  /* best guess about the data type: binary or text
                                           for deflate, or the decoding state
     for inflate */
  int32_t data_type;

  /* Adler-32 or CRC-32 value of the uncompressed data */
  uint32_t adler;
  /* reserved for future use */
  uint32_t reserved;
};

static_assert(sizeof(Wasm_z_stream) == 56, "Wasm_z_stream should be 56 bytes");

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
