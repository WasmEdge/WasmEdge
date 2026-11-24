// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "sd_backend.h"
#include "sd_image.h"
#include "sd_request.h"

#include "common/expected.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// The error side of a scripted generation.
inline Unexpected<BackendFailure> Unexpect(BackendFailure Failure) noexcept {
  return Unexpected<BackendFailure>(Failure);
}

// Count solid RGB images of W x H.
inline ImageBatch solidImages(uint32_t Count, uint32_t W, uint32_t H) {
  const std::vector<uint8_t> Pixels(static_cast<size_t>(W) * H * 3, 0x80);
  ImageBatch Batch;
  for (uint32_t I = 0; I < Count; ++I) {
    auto Image = OwnedImage::copyOf(Pixels, W, H, 3);
    if (!Image) {
      return {};
    }
    Batch.push_back(std::move(*Image));
  }
  return Batch;
}

class FakeBackend : public Backend {
public:
  Capabilities Caps;
  std::optional<ErrNo> OpenError; // open() fails with it
  bool OpenNull = false;          // open() succeeds with a null context
  // Default: solidImages(Req.BatchCount, Req.Width, Req.Height).
  std::function<Expected<ImageBatch, BackendFailure>(const GenerationRequest &,
                                                     const RequestFiles &,
                                                     const GenerationInputs &)>
      OnGenerate;
  std::atomic<int> LiveContexts{0}, Opens{0}, Generates{0};
  SessionConfig LastSessionConfig;
  SessionFiles LastSessionFiles;
  GenerationRequest LastGenerationRequest;
  RequestFiles LastRequestFiles;
  Expected<std::unique_ptr<BackendContext>, ErrNo>
  open(const SessionConfig &, const SessionFiles &) noexcept override;
};

// Counts calls in its FakeBackend, which must outlive it. Reads OnGenerate of
// that backend at each call.
class FakeContext : public BackendContext {
public:
  explicit FakeContext(FakeBackend &Parent) noexcept
      : Owner(Parent), Caps(Parent.Caps) {
    ++Owner.LiveContexts;
  }
  ~FakeContext() noexcept override { --Owner.LiveContexts; }
  const Capabilities &capabilities() const noexcept override { return Caps; }
  Expected<ImageBatch, BackendFailure>
  generate(const GenerationRequest &Req, const RequestFiles &Files,
           const GenerationInputs &Inputs) noexcept override {
    ++Owner.Generates;
    Owner.LastGenerationRequest = Req;
    Owner.LastRequestFiles = Files;
    if (Owner.OnGenerate) {
      return Owner.OnGenerate(Req, Files, Inputs);
    }
    return solidImages(Req.BatchCount, Req.Width, Req.Height);
  }

private:
  FakeBackend &Owner;
  const Capabilities Caps;
};

inline Expected<std::unique_ptr<BackendContext>, ErrNo>
FakeBackend::open(const SessionConfig &Config,
                  const SessionFiles &Files) noexcept {
  ++Opens;
  LastSessionConfig = Config;
  LastSessionFiles = Files;
  if (OpenError) {
    return Unexpected<ErrNo>(*OpenError);
  }
  if (OpenNull) {
    return std::unique_ptr<BackendContext>();
  }
  return std::unique_ptr<BackendContext>(std::make_unique<FakeContext>(*this));
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
