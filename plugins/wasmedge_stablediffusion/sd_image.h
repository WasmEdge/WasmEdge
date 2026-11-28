// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"

#include "common/expected.h"
#include "common/span.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

struct FreeDeleter {
  void operator()(void *P) const noexcept;
};
using PngBuffer = std::unique_ptr<unsigned char, FreeDeleter>;
struct EncodedPng {
  PngBuffer Data;
  uint32_t Size = 0;
};

class OwnedImage {
public:
  // Always takes ownership of Malloced, also when it returns an error.
  static Expected<OwnedImage, ErrNo> adopt(uint8_t *Malloced, uint32_t W,
                                           uint32_t H, uint32_t C) noexcept;
  static Expected<OwnedImage, ErrNo> copyOf(Span<const uint8_t> Pixels,
                                            uint32_t W, uint32_t H,
                                            uint32_t C) noexcept;
  OwnedImage(OwnedImage &&) noexcept;
  OwnedImage &operator=(OwnedImage &&) noexcept;
  OwnedImage(const OwnedImage &) = delete;
  OwnedImage &operator=(const OwnedImage &) = delete;
  ~OwnedImage() noexcept;
  uint32_t width() const noexcept;
  uint32_t height() const noexcept;
  uint32_t channels() const noexcept;
  Span<const uint8_t> pixels() const noexcept; // exactly W * H * C bytes
  static uint64_t liveCount() noexcept;        // images alive in this process

private:
  OwnedImage(uint8_t *Pixels, uint32_t W, uint32_t H, uint32_t C) noexcept;
  void release() noexcept;

  uint8_t *Data = nullptr;
  uint32_t Width = 0;
  uint32_t Height = 0;
  uint32_t Channels = 0;
};
using ImageBatch = std::vector<OwnedImage>;

struct ImageHeader {
  uint32_t Width;
  uint32_t Height;
  uint32_t Channels;
};
Expected<ImageHeader, ErrNo> probeImage(Span<const uint8_t> Encoded) noexcept;
Expected<OwnedImage, ErrNo> decodeImage(Span<const uint8_t> Encoded,
                                        uint32_t Channels) noexcept;
Expected<EncodedPng, ErrNo> encodePng(const OwnedImage &Image) noexcept;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
