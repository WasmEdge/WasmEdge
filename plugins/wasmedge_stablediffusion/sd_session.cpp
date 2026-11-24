// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_session.h"

#include "common/expected.h"

#include <atomic>
#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

namespace {

constexpr uint64_t kMaxReported = std::numeric_limits<uint32_t>::max();

// Returns true for a batch that the session can hold as results: one image or
// more, a count that fits in uint32_t, and three channels in every image.
bool isServable(const ImageBatch &Batch) noexcept {
  if (Batch.empty() || Batch.size() > kMaxReported) {
    return false;
  }
  for (const auto &Image : Batch) {
    if (Image.channels() != 3) {
      return false;
    }
  }
  return true;
}

// Call it with the operation lock held. Returns the bytes of result Index in
// Format. It encodes a PNG on first use and keeps it. It does not keep a failed
// encoding.
Expected<Span<const uint8_t>, ErrNo>
resultBytes(const ImageBatch &Results, std::vector<EncodedPng> &Encoded,
            bool Poisoned, uint32_t Index, ResultFormat Format) noexcept {
  if (Format != ResultFormat::Png && Format != ResultFormat::Rgb8) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  if (Poisoned || Index >= Results.size()) {
    return Unexpected<ErrNo>(ErrNo::NotFound);
  }
  if (Format == ResultFormat::Rgb8) {
    const auto Pixels = Results[Index].pixels();
    if (Pixels.size() > kMaxReported) {
      return Unexpected<ErrNo>(ErrNo::RuntimeError);
    }
    return Pixels;
  }
  auto &Png = Encoded[Index];
  if (!Png.Data) {
    auto Fresh = encodePng(Results[Index]);
    if (!Fresh) {
      return Unexpected<ErrNo>(ErrNo::RuntimeError);
    }
    Png = std::move(*Fresh);
  }
  return Span<const uint8_t>(Png.Data.get(), Png.Size);
}

} // namespace

ErrNo checkAgainst(const Capabilities &Caps,
                   const GenerationRequest &Req) noexcept {
  if (!Req.Loras.empty() && !Caps.Lora) {
    return ErrNo::InvalidArgument;
  }
  if (Req.ControlImage && !Caps.ControlNet) {
    return ErrNo::InvalidArgument;
  }
  if (Req.UpscaleRepeats == 0) {
    return ErrNo::Success;
  }
  if (!Caps.Upscaler || Caps.UpscaleFactor == 0) {
    return ErrNo::InvalidArgument;
  }
  uint64_t W = Req.Width;
  uint64_t H = Req.Height;
  for (uint32_t I = 0;
       I < Req.UpscaleRepeats && W <= Limits::MaxSide && H <= Limits::MaxSide;
       ++I) {
    W *= Caps.UpscaleFactor;
    H *= Caps.UpscaleFactor;
  }
  if (W > Limits::MaxSide || H > Limits::MaxSide || W * H > Limits::MaxPixels) {
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

void ReleaseSessionSlot::operator()(
    std::atomic<uint32_t> *Taken) const noexcept {
  Taken->fetch_sub(1);
}

SessionSlot takeSlot(std::atomic<uint32_t> &Taken, uint32_t Limit) noexcept {
  auto Count = Taken.load();
  while (Count < Limit) {
    if (Taken.compare_exchange_weak(Count, Count + 1)) {
      return SessionSlot(&Taken);
    }
  }
  return nullptr;
}

Session::Session(std::unique_ptr<BackendContext> Ctx, SessionSlot Slot) noexcept
    : Slot(std::move(Slot)), Ctx(std::move(Ctx)),
      Caps(this->Ctx->capabilities()) {}

const Capabilities &Session::capabilities() const noexcept { return Caps; }

ErrNo Session::generate(
    const GenerationRequest &Req, const RequestFiles &Files,
    const std::function<Expected<GenerationInputs, ErrNo>()> &Decode,
    uint32_t &Count) noexcept {
  std::lock_guard<std::mutex> Lock(Op);
  const auto Inputs =
      Decode ? Decode() : Expected<GenerationInputs, ErrNo>(GenerationInputs());
  if (!Inputs) {
    return Inputs.error();
  }
  if (Poisoned) {
    return ErrNo::RuntimeError;
  }
  Encoded.clear();
  Results.clear();
  auto Batch = Ctx->generate(Req, Files, *Inputs);
  if (!Batch) {
    const auto &Failure = Batch.error();
    if (!Failure.ContextUsable) {
      Poisoned = true;
      return ErrNo::RuntimeError;
    }
    if (Failure.Code == ErrNo::InvalidArgument ||
        Failure.Code == ErrNo::LoadFailed) {
      return Failure.Code;
    }
    return ErrNo::RuntimeError;
  }
  if (!isServable(*Batch)) {
    return ErrNo::RuntimeError;
  }
  Results = std::move(*Batch);
  Encoded.resize(Results.size());
  Count = static_cast<uint32_t>(Results.size());
  return ErrNo::Success;
}

ErrNo Session::resultInfo(uint32_t Index, ResultFormat Format,
                          ResultInfo &Info) noexcept {
  std::lock_guard<std::mutex> Lock(Op);
  const auto Bytes = resultBytes(Results, Encoded, Poisoned, Index, Format);
  if (!Bytes) {
    return Bytes.error();
  }
  const auto &Image = Results[Index];
  Info = ResultInfo{Image.width(), Image.height(), Image.channels(),
                    static_cast<uint32_t>(Bytes->size())};
  return ErrNo::Success;
}

ErrNo Session::resultRead(
    uint32_t Index, ResultFormat Format, uint32_t BufLen, uint32_t &Size,
    const std::function<ErrNo(Span<const uint8_t>)> &Copy) noexcept {
  std::lock_guard<std::mutex> Lock(Op);
  const auto Bytes = resultBytes(Results, Encoded, Poisoned, Index, Format);
  if (!Bytes) {
    return Bytes.error();
  }
  Size = static_cast<uint32_t>(Bytes->size());
  if (BufLen < Size) {
    return ErrNo::BufferTooSmall;
  }
  return Copy(*Bytes);
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
