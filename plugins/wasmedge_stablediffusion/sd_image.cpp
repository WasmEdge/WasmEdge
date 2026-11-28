// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_image.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace {

// stb_image decodes guest bytes, so it allocates through these wrappers. Each
// decode has a budget, and every stb allocation counts against it. Then data
// that inflates far past its header runs out of budget, not out of memory. The
// wrappers track the blocks that stb holds, because a failed decode does not
// free all of them. The state is per thread, so a decode takes no lock.
constexpr size_t kReaderBlocks = 64;

struct ReaderState {
  uint64_t Budget = 0;
  uint64_t Requested = 0;
  size_t Live = 0;
  std::array<void *, kReaderBlocks> Blocks{};
};

thread_local ReaderState Reader;

size_t findReaderBlock(const void *Ptr) noexcept {
  size_t I = 0;
  while (I < Reader.Live && Reader.Blocks[I] != Ptr) {
    ++I;
  }
  return I;
}

void forgetReaderBlock(const void *Ptr) noexcept {
  const size_t I = findReaderBlock(Ptr);
  if (I < Reader.Live) {
    Reader.Blocks[I] = Reader.Blocks[--Reader.Live];
    Reader.Blocks[Reader.Live] = nullptr;
  }
}

bool chargeReader(size_t Size) noexcept {
  if (Size > Reader.Budget - Reader.Requested) {
    return false;
  }
  Reader.Requested += Size;
  return true;
}

void *readerMalloc(size_t Size) noexcept {
  if (Reader.Live == Reader.Blocks.size() || !chargeReader(Size)) {
    return nullptr;
  }
  void *Ptr = std::malloc(Size);
  if (Ptr != nullptr) {
    Reader.Blocks[Reader.Live++] = Ptr;
  }
  return Ptr;
}

void *readerRealloc(void *Ptr, size_t Size) noexcept {
  if (Ptr == nullptr) {
    return readerMalloc(Size);
  }
  const size_t I = findReaderBlock(Ptr);
  if (I == Reader.Live || !chargeReader(Size)) {
    return nullptr;
  }
  void *Grown = std::realloc(Ptr, Size);
  if (Grown != nullptr) {
    Reader.Blocks[I] = Grown;
  }
  return Grown;
}

void readerFree(void *Ptr) noexcept {
  forgetReaderBlock(Ptr);
  std::free(Ptr);
}

// One decode. It sets the budget and, at the end, frees the blocks that stb
// still holds. take() keeps the block that the caller gets.
class ReaderScope {
public:
  explicit ReaderScope(uint64_t Budget) noexcept : Saved(Reader) {
    Reader = ReaderState{};
    Reader.Budget = Budget;
  }
  ~ReaderScope() noexcept {
    for (size_t I = 0; I < Reader.Live; ++I) {
      std::free(Reader.Blocks[I]);
    }
    Reader = Saved;
  }
  void take(const void *Ptr) noexcept { forgetReaderBlock(Ptr); }
  ReaderScope(const ReaderScope &) = delete;
  ReaderScope &operator=(const ReaderScope &) = delete;

private:
  ReaderState Saved;
};

} // namespace
} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge

// Build only the PNG and JPEG decoders. Each other decoder is one more parser
// of guest bytes in the host. Without failure strings, stb has no state that
// threads share.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#define STBI_MALLOC(Size) WasmEdge::Host::StableDiffusion::readerMalloc(Size)
#define STBI_REALLOC(Ptr, Size)                                                \
  WasmEdge::Host::StableDiffusion::readerRealloc(Ptr, Size)
#define STBI_FREE(Ptr) WasmEdge::Host::StableDiffusion::readerFree(Ptr)
#include "stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
// The PNG writer checks a failed reallocation only with this assertion. If the
// assertion does nothing, the writer writes past its buffer. Thus the assertion
// ends the process.
#define STBI_WRITE_NO_STDIO
#define STBIW_ASSERT(x) ((x) ? static_cast<void>(0) : std::abort())
#include "stb_image_write.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

namespace {

std::atomic<uint64_t> LiveImages{0};

constexpr uint64_t kMaxSize =
    static_cast<uint64_t>(std::numeric_limits<size_t>::max());
constexpr uint64_t kMaxInt = static_cast<uint64_t>(INT_MAX);
// The PNG writer uses int for buffer sizes and overflows before INT_MAX. The
// largest image that the ABI permits stays far below this limit.
constexpr uint64_t kMaxPngFilteredBytes = UINT64_C(1) << 30;
// The allocation budget of stb to read a header.
constexpr uint64_t kProbeBudget = UINT64_C(1) << 20;

uint64_t roundUpTo32(uint32_t Side) noexcept {
  return (static_cast<uint64_t>(Side) + 31) & ~UINT64_C(31);
}

std::optional<size_t> byteCount(uint32_t W, uint32_t H, uint32_t C) noexcept {
  if (W == 0 || H == 0 || C == 0 || C > 4) {
    return std::nullopt;
  }
  const uint64_t Pixels = static_cast<uint64_t>(W) * H;
  if (Pixels > kMaxSize / C) {
    return std::nullopt;
  }
  return static_cast<size_t>(Pixels * C);
}

} // namespace

void FreeDeleter::operator()(void *P) const noexcept { std::free(P); }

OwnedImage::OwnedImage(uint8_t *Pixels, uint32_t W, uint32_t H,
                       uint32_t C) noexcept
    : Data(Pixels), Width(W), Height(H), Channels(C) {
  LiveImages.fetch_add(1, std::memory_order_relaxed);
}

Expected<OwnedImage, ErrNo> OwnedImage::adopt(uint8_t *Malloced, uint32_t W,
                                              uint32_t H, uint32_t C) noexcept {
  if (Malloced == nullptr || !byteCount(W, H, C)) {
    std::free(Malloced);
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  return OwnedImage(Malloced, W, H, C);
}

Expected<OwnedImage, ErrNo> OwnedImage::copyOf(Span<const uint8_t> Pixels,
                                               uint32_t W, uint32_t H,
                                               uint32_t C) noexcept {
  const auto Bytes = byteCount(W, H, C);
  if (!Bytes || *Bytes != Pixels.size()) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  auto *Copy = static_cast<uint8_t *>(std::malloc(*Bytes));
  if (Copy == nullptr) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  std::memcpy(Copy, Pixels.data(), *Bytes);
  return OwnedImage(Copy, W, H, C);
}

OwnedImage::OwnedImage(OwnedImage &&Other) noexcept
    : Data(std::exchange(Other.Data, nullptr)),
      Width(std::exchange(Other.Width, 0U)),
      Height(std::exchange(Other.Height, 0U)),
      Channels(std::exchange(Other.Channels, 0U)) {}

OwnedImage &OwnedImage::operator=(OwnedImage &&Other) noexcept {
  if (this != &Other) {
    release();
    Data = std::exchange(Other.Data, nullptr);
    Width = std::exchange(Other.Width, 0U);
    Height = std::exchange(Other.Height, 0U);
    Channels = std::exchange(Other.Channels, 0U);
  }
  return *this;
}

OwnedImage::~OwnedImage() noexcept { release(); }

void OwnedImage::release() noexcept {
  if (Data != nullptr) {
    std::free(Data);
    LiveImages.fetch_sub(1, std::memory_order_relaxed);
  }
  Data = nullptr;
  Width = 0;
  Height = 0;
  Channels = 0;
}

uint32_t OwnedImage::width() const noexcept { return Width; }

uint32_t OwnedImage::height() const noexcept { return Height; }

uint32_t OwnedImage::channels() const noexcept { return Channels; }

Span<const uint8_t> OwnedImage::pixels() const noexcept {
  return Span<const uint8_t>(
      Data,
      static_cast<size_t>(static_cast<uint64_t>(Width) * Height * Channels));
}

uint64_t OwnedImage::liveCount() noexcept {
  return LiveImages.load(std::memory_order_relaxed);
}

Expected<ImageHeader, ErrNo> probeImage(Span<const uint8_t> Encoded) noexcept {
  if (Encoded.empty() || Encoded.size() > kMaxInt) {
    return Unexpected<ErrNo>(ErrNo::InvalidEncoding);
  }
  int W = 0;
  int H = 0;
  int C = 0;
  ReaderScope Probe(kProbeBudget);
  if (stbi_info_from_memory(Encoded.data(), static_cast<int>(Encoded.size()),
                            &W, &H, &C) == 0 ||
      W <= 0 || H <= 0 || C < 1 || C > 4) {
    return Unexpected<ErrNo>(ErrNo::InvalidEncoding);
  }
  return ImageHeader{static_cast<uint32_t>(W), static_cast<uint32_t>(H),
                     static_cast<uint32_t>(C)};
}

Expected<OwnedImage, ErrNo> decodeImage(Span<const uint8_t> Encoded,
                                        uint32_t Channels) noexcept {
  if (Channels != 1 && Channels != 3) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  const auto Header = probeImage(Encoded);
  if (!Header) {
    return Unexpected<ErrNo>(Header.error());
  }
  // Apply the output limits to an input before the decode. This also bounds the
  // budget below.
  if (Header->Width > Limits::MaxSide || Header->Height > Limits::MaxSide ||
      static_cast<uint64_t>(Header->Width) * Header->Height >
          Limits::MaxPixels) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  int W = 0;
  int H = 0;
  int FileChannels = 0;
  stbi_uc *Pixels = nullptr;
  {
    // The maximum that a valid image with this header needs: 64 bytes per pixel
    // for the pixels and the stb work copies (sides rounded up as in the JPEG
    // decoder), buffers that grow with the compressed data, and 1 MiB. The
    // compressed data of an image needs at most 8 bytes per pixel and 64 KiB.
    // More input than that, such as padding, adds nothing to the budget.
    const uint64_t Area =
        roundUpTo32(Header->Width) * roundUpTo32(Header->Height);
    const uint64_t Compressed = std::min<uint64_t>(
        Encoded.size(), UINT64_C(8) * Area + (UINT64_C(1) << 16));
    ReaderScope Load(UINT64_C(64) * Area + UINT64_C(8) * Compressed +
                     (UINT64_C(1) << 20));
    Pixels = stbi_load_from_memory(Encoded.data(),
                                   static_cast<int>(Encoded.size()), &W, &H,
                                   &FileChannels, static_cast<int>(Channels));
    if (Pixels != nullptr) {
      Load.take(Pixels);
    }
  }
  if (Pixels == nullptr) {
    return Unexpected<ErrNo>(ErrNo::InvalidEncoding);
  }
  // The guest can change the bytes between the probe and the decode. The
  // decoded size must equal the checked size.
  if (static_cast<uint32_t>(W) != Header->Width ||
      static_cast<uint32_t>(H) != Header->Height) {
    std::free(Pixels);
    return Unexpected<ErrNo>(ErrNo::InvalidEncoding);
  }
  return OwnedImage::adopt(Pixels, Header->Width, Header->Height, Channels);
}

Expected<EncodedPng, ErrNo> encodePng(const OwnedImage &Image) noexcept {
  const auto Pixels = Image.pixels();
  const uint64_t RowBytes =
      static_cast<uint64_t>(Image.width()) * Image.channels() + 1;
  if (Pixels.empty() || RowBytes > kMaxPngFilteredBytes ||
      RowBytes * Image.height() > kMaxPngFilteredBytes) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  int Len = 0;
  PngBuffer Png(
      stbi_write_png_to_mem(Pixels.data(), 0, static_cast<int>(Image.width()),
                            static_cast<int>(Image.height()),
                            static_cast<int>(Image.channels()), &Len, nullptr));
  if (!Png || Len <= 0) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  return EncodedPng{std::move(Png), static_cast<uint32_t>(Len)};
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
