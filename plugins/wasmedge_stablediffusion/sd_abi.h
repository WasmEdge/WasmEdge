// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

using namespace std::literals;

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  InvalidEncoding = 2, // Invalid encoding.
  MissingMemory = 3,   // Caller module is missing a memory export.
  Busy = 4,            // Device or resource busy.
  RuntimeError = 5,    // Runtime Error.
  BufferTooSmall = 6,  // Output buffer smaller than the result.
  NotFound = 7,        // Unknown session or no result at that index.
  LoadFailed = 8,      // A path or file that cannot be loaded.
  NotReadOnly = 9,     // A path names a file the guest could change.
};

inline constexpr uint32_t kAbiVersion = 2;

enum class ResultFormat : uint32_t { Png = 0, Rgb8 = 1 };

struct Limits {
  // Sessions per module instance, each counted until its context is freed. A
  // session keeps its models loaded from its first generation until it closes.
  // This limit bounds that memory.
  static constexpr uint32_t MaxSessions = 4;
  static constexpr uint32_t MaxTextBytes = 65536;
  // Prompt plus negative prompt. Longer prompts worked with the tested models.
  // The text encoders of other model families were not measured.
  static constexpr uint32_t MaxPromptBytes = 1500;
  // Embedding rows that the two prompts of one request mention. Each mention
  // adds the rows of its file. Each row is one more token for the text encoder
  // and for every denoising step.
  static constexpr uint64_t MaxEmbeddingRows = 1024;
  // Limits for every image: generated, upscaled, or input. MaxPixels bounds
  // what an upscaler can return. It does not bound the memory of a generation
  // at that size. The host must bound that memory.
  static constexpr uint32_t MinSide = 64;
  static constexpr uint32_t MaxSide = 8192;
  static constexpr uint32_t SideStep = 64;
  static constexpr uint64_t MaxPixels = UINT64_C(1) << 25;
  static constexpr uint32_t MaxSteps = 1000;
  static constexpr uint32_t MaxBatch = 16;
  // The adapter also caps the value at the hardware thread count.
  static constexpr uint32_t MaxThreads = 1024;
  // The smallest CLIP text encoder of the library has 12 layers. The library
  // does not refuse a larger value: it then runs all the layers, or none.
  static constexpr uint32_t MaxClipSkip = 12;
  static constexpr uint32_t MaxEmbeddings = 64;
  static constexpr uint32_t MaxLoras = 16;
  static constexpr uint32_t MaxBlobs = 8;
  static constexpr uint32_t MaxUpscaleRepeats = 3;
  static constexpr double MaxGuidance = 1000.0;
  static constexpr double MaxControlStrength = 100.0;
  static constexpr double MaxStrength = 1.0;
  static constexpr double MaxLoraMultiplier = 100.0;
  // The library reads an embedding into a fixed-size work area.
  static constexpr uint64_t MaxEmbeddingFileBytes = UINT64_C(16) << 20;
  // The file check reads at most this much of a LoRA, embedding, or model file.
  static constexpr uint64_t MaxSafetensorsHeaderBytes = UINT64_C(8) << 20;
};

inline constexpr std::array<std::string_view, 21> kSamplerNames = {
    "euler"sv,         "euler_a"sv,        "heun"sv,
    "dpm2"sv,          "dpm++2s_a"sv,      "dpm++2m"sv,
    "dpm++2mv2"sv,     "ipndm"sv,          "ipndm_v"sv,
    "lcm"sv,           "ddim_trailing"sv,  "tcd"sv,
    "res_multistep"sv, "res_2s"sv,         "er_sde"sv,
    "euler_cfg_pp"sv,  "euler_a_cfg_pp"sv, "euler_ge"sv,
    "dpm++2m_sde"sv,   "dpm++2m_sde_bt"sv, "lms"sv};
inline constexpr std::array<std::string_view, 17> kSchedulerNames = {
    "discrete"sv,     "karras"sv,      "exponential"sv,  "ays"sv,
    "gits"sv,         "sgm_uniform"sv, "simple"sv,       "smoothstep"sv,
    "kl_optimal"sv,   "lcm"sv,         "bong_tangent"sv, "ltx2"sv,
    "logit_normal"sv, "flux2"sv,       "flux"sv,         "beta"sv,
    "llada_image"sv};
// Minimum steps for each scheduler, in the order of kSchedulerNames. Some
// schedules need two steps or more. The plug-in refuses a smaller request
// before it calls the library.
inline constexpr std::array<uint32_t, kSchedulerNames.size()>
    kSchedulerMinSteps = {1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
inline constexpr std::array<std::string_view, 13> kWeightTypeNames = {
    "f32"sv,  "f16"sv,  "bf16"sv, "q8_0"sv, "q6_k"sv, "q5_k"sv, "q5_1"sv,
    "q5_0"sv, "q4_k"sv, "q4_1"sv, "q4_0"sv, "q3_k"sv, "q2_k"sv};
inline constexpr std::array<std::string_view, 3> kRngNames = {
    "std_default"sv, "cuda"sv, "cpu"sv};
inline constexpr std::array<std::string_view, 2> kDeviceNames = {"auto"sv,
                                                                 "cpu"sv};
inline constexpr std::array<std::string_view, 3> kCpuModuleNames = {
    "text_encoder"sv, "vae"sv, "control_net"sv};

template <size_t N>
constexpr std::optional<uint8_t>
indexOf(const std::array<std::string_view, N> &Table,
        std::string_view Name) noexcept {
  static_assert(N <= 256, "an index must fit in uint8_t");
  for (size_t I = 0; I < N; ++I) {
    if (Table[I] == Name) {
      return static_cast<uint8_t>(I);
    }
  }
  return std::nullopt;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
