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
struct WasmZStream {
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
static_assert(sizeof(WasmZStream) == 56, "WasmZStream should be 56 bytes");

/*
     gzip header information passed to and from zlib routines.  See RFC 1952
  for more details on the meanings of these fields.
*/
struct WasmGZHeader {
  int32_t text;       /* true if compressed data believed to be text */
  uint32_t time;      /* modification time */
  int32_t xflags;     /* extra flags (not used when writing a gzip file) */
  int32_t os;         /* operating system */
  uint32_t extra;     /* pointer to extra field or Z_NULL if none */
  uint32_t extra_len; /* extra field length (valid if extra != Z_NULL) */
  uint32_t extra_max; /* space at extra (only when reading header) */
  uint32_t name;      /* pointer to zero-terminated file name or Z_NULL */
  uint32_t name_max;  /* space at name (only when reading header) */
  uint32_t comment;   /* pointer to zero-terminated comment or Z_NULL */
  uint32_t comm_max;  /* space at comment (only when reading header) */
  int32_t hcrc;       /* true if there was or will be a header crc */
  int32_t done;       /* true when done reading gzip header (not used
                     when writing a gzip file) */
};
static_assert(sizeof(WasmGZHeader) == 52, "WasmGZHeader should be 52 bytes");

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibEnvironment {
public:
  using GZFile_s = std::remove_pointer_t<gzFile>;

  std::unordered_map<uint32_t, std::unique_ptr<z_stream>> ZStreamMap;
  std::map<uint32_t, std::unique_ptr<GZFile_s>, std::greater<uint32_t>>
      GZFileMap;

  /// Initial Configurations
  static Plugin::PluginRegister Register;
};

} // namespace Host
} // namespace WasmEdge
