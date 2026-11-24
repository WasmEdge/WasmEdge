// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "sd_image.h"
#include "sd_request.h"

#include "common/expected.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

struct Capabilities {
  bool ControlNet = false;
  bool Upscaler = false;
  uint32_t UpscaleFactor = 0;
  bool Lora = false; // false when the session asked for quantized weights
};
struct SessionFiles { // host paths; empty: not configured
  std::string Model, DiffusionModel, ClipL, ClipG, T5xxl, Vae, Taesd,
      ControlNet, Upscaler;
  std::vector<std::string> Embeddings; // parallel to SessionConfig::Embeddings
};
struct RequestFiles {
  std::vector<std::string> Loras; // parallel to GenerationRequest::Loras
};
struct GenerationInputs {
  std::optional<OwnedImage> Init, Mask, Control;
};
struct BackendFailure {
  ErrNo Code;
  bool ContextUsable; // false makes the session Poisoned
};
class BackendContext {
public:
  virtual ~BackendContext() = default;
  virtual const Capabilities &capabilities() const noexcept = 0;
  // On success the batch is not empty and every image has three channels.
  virtual Expected<ImageBatch, BackendFailure>
  generate(const GenerationRequest &, const RequestFiles &,
           const GenerationInputs &) noexcept = 0;
};
class Backend {
public:
  virtual ~Backend() = default;
  // On success the context is not null.
  virtual Expected<std::unique_ptr<BackendContext>, ErrNo>
  open(const SessionConfig &, const SessionFiles &) noexcept = 0;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
