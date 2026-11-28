// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"

#include "common/expected.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

struct EmbeddingRef {
  std::string Name;
  std::string Path;
};
struct LoraRef {
  std::string Path;
  float Multiplier = 1.0F;
};
struct SessionConfig {
  std::string ModelPath, DiffusionModelPath, ClipLPath, ClipGPath, T5xxlPath,
      VaePath, TaesdPath, ControlNetPath, UpscalerModelPath; // empty: not given
  std::vector<EmbeddingRef> Embeddings;
  uint32_t Threads = 0;              // 0: the number of physical cores
  std::optional<uint8_t> WeightType; // index into kWeightTypeNames
  std::optional<uint8_t> Rng;        // index into kRngNames
  bool FlashAttention = false;
  uint8_t Device = 0;     // index into kDeviceNames
  uint8_t CpuModules = 0; // bit i: kCpuModuleNames[i]
};
struct GenerationRequest {
  std::string Prompt, NegativePrompt;
  uint32_t Width = 512, Height = 512;
  int64_t Seed = -1; // negative: random
  uint32_t Steps = 20;
  std::optional<float> CfgScale, Guidance, ControlStrength, Strength;
  std::optional<uint8_t> Sampler, Scheduler; // indexes into the tables
  std::optional<uint32_t> ClipSkip;
  uint32_t BatchCount = 1;
  std::optional<uint32_t> InitImage, MaskImage, ControlImage; // blob indexes
  std::vector<LoraRef> Loras;
  bool VaeTiling = false;
  uint32_t UpscaleRepeats = 0; // 0: no upscaling
};
Expected<SessionConfig, ErrNo>
parseSessionConfig(std::string_view Text) noexcept;
Expected<GenerationRequest, ErrNo>
parseGenerationRequest(std::string_view Text, uint32_t BlobCount) noexcept;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
