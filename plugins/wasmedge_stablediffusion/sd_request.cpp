// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_request.h"

#include "sd_log.h"

#include "common/spdlog.h"

#include "simdjson.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace {

using JsonElement = simdjson::dom::element;
using JsonType = simdjson::dom::element_type;

constexpr std::string_view kConfigPrefix = "session configuration: "sv;
constexpr std::string_view kRequestPrefix = "generation request: "sv;
constexpr int64_t kMaxExactInteger = INT64_C(1) << 53;

struct Place {
  std::string_view Prefix;
  std::string_view Key;
};

bool reject(const Place &At, std::string_view Reason) noexcept {
  spdlog::error("[WasmEdge-StableDiffusion] {}{}: {}"sv, At.Prefix,
                logText(At.Key), Reason);
  return false;
}

template <typename State> struct Field {
  std::string_view Key;
  bool Required;
  bool (*Read)(State &, JsonElement, const Place &) noexcept;
};

// Reads each member of Object with the field for its key. It refuses an unknown
// key and a repeated key. Thus the plug-in ignores nothing from the guest, and
// no value depends on which duplicate the parser keeps.
template <typename State, size_t N>
bool walk(const simdjson::dom::object &Object,
          const std::array<Field<State>, N> &Fields, State &Out,
          std::string_view Prefix) noexcept {
  std::array<bool, N> Seen{};
  for (const auto Member : Object) {
    size_t I = 0;
    while (I < N && Fields[I].Key != Member.key) {
      ++I;
    }
    if (I == N) {
      return reject({Prefix, Member.key}, "is not a known key"sv);
    }
    const Place At{Prefix, Fields[I].Key};
    if (Seen[I]) {
      return reject(At, "is given twice"sv);
    }
    Seen[I] = true;
    if (!Fields[I].Read(Out, Member.value, At)) {
      return false;
    }
  }
  for (size_t I = 0; I < N; ++I) {
    if (Fields[I].Required && !Seen[I]) {
      return reject({Prefix, Fields[I].Key}, "is missing"sv);
    }
  }
  return true;
}

// The library takes C strings, which end at the first NUL. Refuse a string that
// holds a NUL.
bool readString(JsonElement Value, const Place &At,
                std::string_view &Out) noexcept {
  if (Value.get(Out) != simdjson::SUCCESS) {
    return reject(At, "must be a string"sv);
  }
  if (Out.find('\0') != std::string_view::npos) {
    return reject(At, "must not contain NUL"sv);
  }
  return true;
}

bool readText(JsonElement Value, const Place &At, std::string &Out) noexcept {
  std::string_view Text;
  if (!readString(Value, At, Text)) {
    return false;
  }
  Out.assign(Text);
  return true;
}

bool readNonEmpty(JsonElement Value, const Place &At,
                  std::string &Out) noexcept {
  std::string_view Text;
  if (!readString(Value, At, Text)) {
    return false;
  }
  if (Text.empty()) {
    return reject(At, "must not be empty"sv);
  }
  Out.assign(Text);
  return true;
}

template <size_t N, typename OutT>
bool readName(JsonElement Value, const Place &At,
              const std::array<std::string_view, N> &Table,
              OutT &Out) noexcept {
  std::string_view Name;
  if (!readString(Value, At, Name)) {
    return false;
  }
  const auto Index = indexOf(Table, Name);
  if (!Index) {
    return reject(At,
                  fmt::format("\"{}\" is not a known name"sv, logText(Name)));
  }
  Out = *Index;
  return true;
}

bool readBool(JsonElement Value, const Place &At, bool &Out) noexcept {
  if (Value.get(Out) != simdjson::SUCCESS) {
    return reject(At, "must be true or false"sv);
  }
  return true;
}

// Accepts only a value that the guest wrote as an integer. It refuses 3.0 and
// 3e0, so no value is rounded.
template <typename IntT, typename OutT>
bool readInteger(JsonElement Value, const Place &At, IntT Min, IntT Max,
                 OutT &Out) noexcept {
  static_assert(
      std::is_same_v<IntT, int64_t> ||
          (std::is_integral_v<IntT> && sizeof(IntT) < sizeof(int64_t)),
      "a bound must convert to int64_t exactly");
  const auto Type = Value.type();
  if (Type != JsonType::INT64 && Type != JsonType::UINT64) {
    return reject(At,
                  "must be an integer written without fraction or exponent"sv);
  }
  int64_t Integer = 0;
  if (Type != JsonType::INT64 || Value.get(Integer) != simdjson::SUCCESS ||
      Integer < static_cast<int64_t>(Min) ||
      Integer > static_cast<int64_t>(Max)) {
    return reject(At,
                  fmt::format("must be an integer from {} to {}"sv, Min, Max));
  }
  Out = static_cast<IntT>(Integer);
  return true;
}

bool readSide(JsonElement Value, const Place &At, uint32_t &Out) noexcept {
  if (!readInteger(Value, At, Limits::MinSide, Limits::MaxSide, Out)) {
    return false;
  }
  if (Out % Limits::SideStep != 0) {
    return reject(At,
                  fmt::format("must be a multiple of {}"sv, Limits::SideStep));
  }
  return true;
}

// Returns a number as a double if the conversion is exact. It refuses an
// integer beyond 2^53.
std::optional<double> exactNumber(JsonElement Value) noexcept {
  double Number = 0.0;
  int64_t Integer = 0;
  switch (Value.type()) {
  case JsonType::DOUBLE:
    if (Value.get(Number) == simdjson::SUCCESS) {
      return Number;
    }
    break;
  case JsonType::INT64:
    if (Value.get(Integer) == simdjson::SUCCESS &&
        Integer >= -kMaxExactInteger && Integer <= kMaxExactInteger) {
      return static_cast<double>(Integer);
    }
    break;
  default:
    break;
  }
  return std::nullopt;
}

template <typename OutT>
bool readNumber(JsonElement Value, const Place &At, double Min, double Max,
                OutT &Out) noexcept {
  const auto Type = Value.type();
  if (Type != JsonType::DOUBLE && Type != JsonType::INT64 &&
      Type != JsonType::UINT64) {
    return reject(At, "must be a number"sv);
  }
  const auto Number = exactNumber(Value);
  if (!Number || !(*Number >= Min && *Number <= Max)) {
    return reject(At,
                  fmt::format("must be a number from {} to {}"sv, Min, Max));
  }
  Out = static_cast<float>(*Number);
  return true;
}

template <typename EntryT, size_t N>
bool readEntries(JsonElement Value, const Place &At, size_t MaxEntries,
                 const std::array<Field<EntryT>, N> &Fields,
                 std::vector<EntryT> &Out) noexcept {
  simdjson::dom::array Entries;
  if (Value.get(Entries) != simdjson::SUCCESS) {
    return reject(At, "must be an array of objects"sv);
  }
  if (Entries.size() > MaxEntries) {
    return reject(At,
                  fmt::format("must have at most {} entries"sv, MaxEntries));
  }
  size_t Index = 0;
  for (const auto Entry : Entries) {
    simdjson::dom::object Object;
    if (Entry.get(Object) != simdjson::SUCCESS) {
      return reject(At, "must be an array of objects"sv);
    }
    const auto Prefix = fmt::format("{}{}[{}]."sv, At.Prefix, At.Key, Index);
    EntryT Parsed;
    if (!walk(Object, Fields, Parsed, Prefix)) {
      return false;
    }
    Out.push_back(std::move(Parsed));
    ++Index;
  }
  return true;
}

bool readCpuModules(JsonElement Value, const Place &At, uint8_t &Out) noexcept {
  static_assert(kCpuModuleNames.size() <= 8, "a module needs a bit");
  simdjson::dom::array Names;
  if (Value.get(Names) != simdjson::SUCCESS) {
    return reject(At, "must be an array of strings"sv);
  }
  uint8_t Modules = 0;
  for (const auto Name : Names) {
    if (Name.type() != JsonType::STRING) {
      return reject(At, "must be an array of strings"sv);
    }
    uint8_t Index = 0;
    if (!readName(Name, At, kCpuModuleNames, Index)) {
      return false;
    }
    const auto Bit = static_cast<uint8_t>(1U << Index);
    if ((Modules & Bit) != 0) {
      return reject(
          At, fmt::format("\"{}\" is given twice"sv, kCpuModuleNames[Index]));
    }
    Modules = static_cast<uint8_t>(Modules | Bit);
  }
  Out = Modules;
  return true;
}

struct ImageRef {
  uint32_t BlobCount = 0;
  std::optional<uint32_t> Blob;
};

constexpr std::array<Field<ImageRef>, 1> kImageFields{{
    {"blob"sv, true,
     [](ImageRef &Image, JsonElement Value, const Place &At) noexcept {
       if (Image.BlobCount == 0) {
         return reject(At, "names a blob, but the call passed none"sv);
       }
       return readInteger(Value, At, uint32_t{0}, Image.BlobCount - 1,
                          Image.Blob);
     }},
}};

bool readImage(JsonElement Value, const Place &At, uint32_t BlobCount,
               std::optional<uint32_t> &Out) noexcept {
  simdjson::dom::object Object;
  if (Value.get(Object) != simdjson::SUCCESS) {
    return reject(At, "must be an object"sv);
  }
  const auto Prefix = fmt::format("{}{}."sv, At.Prefix, At.Key);
  ImageRef Image{BlobCount, std::nullopt};
  if (!walk(Object, kImageFields, Image, Prefix)) {
    return false;
  }
  Out = Image.Blob;
  return true;
}

constexpr std::array<Field<EmbeddingRef>, 2> kEmbeddingFields{{
    {"name"sv, true,
     [](EmbeddingRef &Embedding, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Embedding.Name);
     }},
    {"path"sv, true,
     [](EmbeddingRef &Embedding, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Embedding.Path);
     }},
}};

constexpr std::array<Field<LoraRef>, 2> kLoraFields{{
    {"path"sv, true,
     [](LoraRef &Lora, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Lora.Path);
     }},
    {"multiplier"sv, false,
     [](LoraRef &Lora, JsonElement Value, const Place &At) noexcept {
       return readNumber(Value, At, -Limits::MaxLoraMultiplier,
                         Limits::MaxLoraMultiplier, Lora.Multiplier);
     }},
}};

constexpr std::array<Field<SessionConfig>, 16> kSessionFields{{
    {"model_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.ModelPath);
     }},
    {"diffusion_model_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.DiffusionModelPath);
     }},
    {"clip_l_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.ClipLPath);
     }},
    {"clip_g_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.ClipGPath);
     }},
    {"t5xxl_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.T5xxlPath);
     }},
    {"vae_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.VaePath);
     }},
    {"taesd_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.TaesdPath);
     }},
    {"control_net_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.ControlNetPath);
     }},
    {"upscaler_model_path"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readNonEmpty(Value, At, Config.UpscalerModelPath);
     }},
    {"embeddings"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readEntries(Value, At, Limits::MaxEmbeddings, kEmbeddingFields,
                          Config.Embeddings);
     }},
    {"n_threads"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, uint32_t{1}, Limits::MaxThreads,
                          Config.Threads);
     }},
    {"weight_type"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readName(Value, At, kWeightTypeNames, Config.WeightType);
     }},
    {"rng"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readName(Value, At, kRngNames, Config.Rng);
     }},
    {"flash_attention"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readBool(Value, At, Config.FlashAttention);
     }},
    {"device"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readName(Value, At, kDeviceNames, Config.Device);
     }},
    {"cpu_modules"sv, false,
     [](SessionConfig &Config, JsonElement Value, const Place &At) noexcept {
       return readCpuModules(Value, At, Config.CpuModules);
     }},
}};

struct RequestState {
  GenerationRequest Out;
  uint32_t BlobCount = 0;
};

constexpr std::array<Field<RequestState>, 20> kRequestFields{{
    {"prompt"sv, true,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readText(Value, At, Request.Out.Prompt);
     }},
    {"negative_prompt"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readText(Value, At, Request.Out.NegativePrompt);
     }},
    {"width"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readSide(Value, At, Request.Out.Width);
     }},
    {"height"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readSide(Value, At, Request.Out.Height);
     }},
    {"seed"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, std::numeric_limits<int64_t>::min(),
                          std::numeric_limits<int64_t>::max(),
                          Request.Out.Seed);
     }},
    {"steps"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, uint32_t{1}, Limits::MaxSteps,
                          Request.Out.Steps);
     }},
    {"cfg_scale"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readNumber(Value, At, 0.0, Limits::MaxGuidance,
                         Request.Out.CfgScale);
     }},
    {"guidance"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readNumber(Value, At, 0.0, Limits::MaxGuidance,
                         Request.Out.Guidance);
     }},
    {"control_strength"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readNumber(Value, At, 0.0, Limits::MaxControlStrength,
                         Request.Out.ControlStrength);
     }},
    {"strength"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readNumber(Value, At, 0.0, Limits::MaxStrength,
                         Request.Out.Strength);
     }},
    {"sampler"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readName(Value, At, kSamplerNames, Request.Out.Sampler);
     }},
    {"scheduler"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readName(Value, At, kSchedulerNames, Request.Out.Scheduler);
     }},
    {"clip_skip"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, uint32_t{1}, Limits::MaxClipSkip,
                          Request.Out.ClipSkip);
     }},
    {"batch_count"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, uint32_t{1}, Limits::MaxBatch,
                          Request.Out.BatchCount);
     }},
    {"init_image"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readImage(Value, At, Request.BlobCount, Request.Out.InitImage);
     }},
    {"mask_image"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readImage(Value, At, Request.BlobCount, Request.Out.MaskImage);
     }},
    {"control_image"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readImage(Value, At, Request.BlobCount, Request.Out.ControlImage);
     }},
    {"loras"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readEntries(Value, At, Limits::MaxLoras, kLoraFields,
                          Request.Out.Loras);
     }},
    {"vae_tiling"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readBool(Value, At, Request.Out.VaeTiling);
     }},
    {"upscale_repeats"sv, false,
     [](RequestState &Request, JsonElement Value, const Place &At) noexcept {
       return readInteger(Value, At, uint32_t{1}, Limits::MaxUpscaleRepeats,
                          Request.Out.UpscaleRepeats);
     }},
}};

// The rules that need more than one key. These rules and the ranges above are
// equal to or narrower than the limits of the pinned library. Thus the library
// does not reject the numbers of a request after work is done.
bool checkRequest(const GenerationRequest &Request) noexcept {
  if (static_cast<uint64_t>(Request.Width) * Request.Height >
      Limits::MaxPixels) {
    return reject(
        {kRequestPrefix, "width * height"sv},
        fmt::format("must be at most {} pixels"sv, Limits::MaxPixels));
  }
  if (Request.Prompt.size() + Request.NegativePrompt.size() >
      Limits::MaxPromptBytes) {
    return reject(
        {kRequestPrefix, "prompt + negative_prompt"sv},
        fmt::format("must be at most {} bytes"sv, Limits::MaxPromptBytes));
  }
  if (Request.MaskImage && !Request.InitImage) {
    return reject({kRequestPrefix, "mask_image"sv}, "needs init_image"sv);
  }
  // If a request names no scheduler, the library chooses one for the model and
  // the sampler. That choice is not known here. On a model with EDM-V
  // prediction, the choice needs two steps, and a request for one step fails in
  // the generation with RuntimeError.
  if (Request.Scheduler &&
      Request.Steps < kSchedulerMinSteps[*Request.Scheduler]) {
    return reject({kRequestPrefix, "steps"sv},
                  fmt::format("must be at least {} with the scheduler \"{}\""sv,
                              kSchedulerMinSteps[*Request.Scheduler],
                              kSchedulerNames[*Request.Scheduler]));
  }
  return true;
}

ErrNo parseObject(std::string_view Text, std::string_view Prefix,
                  simdjson::dom::parser &Parser,
                  simdjson::dom::object &Root) noexcept {
  if (Text.size() > Limits::MaxTextBytes) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] {}the text is longer than {} bytes"sv,
        Prefix, Limits::MaxTextBytes);
    return ErrNo::InvalidArgument;
  }
  const simdjson::padded_string Padded(Text.data(), Text.size());
  JsonElement Document;
  if (const auto Err = Parser.parse(Padded).get(Document);
      Err != simdjson::SUCCESS) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] {}the JSON parser refused the text: {}"sv,
        Prefix, simdjson::error_message(Err));
    return Err == simdjson::BIGINT_ERROR ? ErrNo::InvalidArgument
                                         : ErrNo::InvalidEncoding;
  }
  if (Document.get(Root) != simdjson::SUCCESS) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] {}the text is not a JSON object"sv, Prefix);
    return ErrNo::InvalidEncoding;
  }
  return ErrNo::Success;
}

} // namespace

Expected<SessionConfig, ErrNo>
parseSessionConfig(std::string_view Text) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::object Root;
  if (const auto Err = parseObject(Text, kConfigPrefix, Parser, Root);
      Err != ErrNo::Success) {
    return Unexpected<ErrNo>(Err);
  }
  SessionConfig Config;
  if (!walk(Root, kSessionFields, Config, kConfigPrefix)) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  if (Config.ModelPath.empty() && Config.DiffusionModelPath.empty()) {
    reject({kConfigPrefix, "model_path"sv},
           "is required unless diffusion_model_path is given"sv);
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  return Config;
}

Expected<GenerationRequest, ErrNo>
parseGenerationRequest(std::string_view Text, uint32_t BlobCount) noexcept {
  simdjson::dom::parser Parser;
  simdjson::dom::object Root;
  if (const auto Err = parseObject(Text, kRequestPrefix, Parser, Root);
      Err != ErrNo::Success) {
    return Unexpected<ErrNo>(Err);
  }
  RequestState Request{GenerationRequest{}, BlobCount};
  if (!walk(Root, kRequestFields, Request, kRequestPrefix) ||
      !checkRequest(Request.Out)) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  return std::move(Request.Out);
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
