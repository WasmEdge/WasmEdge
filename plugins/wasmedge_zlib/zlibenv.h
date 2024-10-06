// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
  uint32_t NextIn;
  /* number of bytes available at next_in */
  uint32_t AvailIn;
  /* total number of input bytes read so far */
  uint32_t TotalIn;

  /* [Wasm Offset] next output byte will go here */
  uint32_t NextOut;
  /* remaining free space at next_out */
  uint32_t AvailOut;
  /* total number of bytes output so far */
  uint32_t TotalOut;

  /* [Wasm Offset] last error message, NULL if no error */
  uint32_t Msg;
  /* [Wasm Offset] not visible by applications */
  uint32_t State;

  /* used to allocate the internal state */
  uint32_t Zalloc;
  /* used to free the internal state */
  uint32_t Zfree;
  /* [Wasm Offset] private data object passed to zalloc and zfree */
  uint32_t Opaque;

  /* best guess about the data type: binary or text for deflate, or the decoding
     state for inflate */
  int32_t DataType;

  /* Adler-32 or CRC-32 value of the uncompressed data */
  uint32_t Adler;
  /* reserved for future use */
  uint32_t Reserved;
};
static_assert(sizeof(WasmZStream) == 56, "WasmZStream should be 56 bytes");

/*
  gzip header information passed to and from zlib routines. See RFC 1952 for
  more details on the meanings of these fields.
*/
struct WasmGZHeader {
  int32_t Text;      /* true if compressed data believed to be text */
  uint32_t Time;     /* modification time */
  int32_t XFlags;    /* extra flags (not used when writing a gzip file) */
  int32_t OS;        /* operating system */
  uint32_t Extra;    /* pointer to extra field or Z_NULL if none */
  uint32_t ExtraLen; /* extra field length (valid if extra != Z_NULL) */
  uint32_t ExtraMax; /* space at extra (only when reading header) */
  uint32_t Name;     /* pointer to zero-terminated file name or Z_NULL */
  uint32_t NameMax;  /* space at name (only when reading header) */
  uint32_t Comment;  /* pointer to zero-terminated comment or Z_NULL */
  uint32_t CommMax;  /* space at comment (only when reading header) */
  int32_t HCRC;      /* true if there was or will be a header crc */
  int32_t Done;      /* true when done reading gzip header (not used
                        when writing a gzip file) */
};
static_assert(sizeof(WasmGZHeader) == 52, "WasmGZHeader should be 52 bytes");

namespace WasmEdge {
namespace Host {

class WasmEdgeZlibEnvironment {
public:
  using GZFile = std::remove_pointer_t<gzFile>;

  struct GZStore {
    uint32_t WasmGZHeaderOffset;
    std::unique_ptr<gz_header> HostGZHeader;
  };

  std::unordered_map<uint32_t, std::unique_ptr<z_stream>> ZStreamMap;
  std::map<uint32_t, std::unique_ptr<GZFile>, std::greater<uint32_t>> GZFileMap;
  std::unordered_map<uint32_t, GZStore> GZHeaderMap;
};

} // namespace Host
} // namespace WasmEdge
