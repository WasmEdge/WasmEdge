// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "safetensors_fixture.h"
#include "sd_abi.h"
#include "sd_backend_sdcpp.h"

#include "common/spdlog.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

// The opaque handles of the library, as the scripted library below defines
// them.
struct sd_tensor_t {
  sd_tensor_type_t Type;
  std::vector<int64_t> Shape;
  std::vector<uint8_t> Bytes;
};
struct sd_opt {
  int Purpose;
  std::vector<std::pair<std::string, std::string>> Values;
  std::map<std::string, std::vector<int64_t>> Tensors;
};
struct sd_ctx_t {};
struct sd_result {
  std::vector<sd_tensor_t> Images;
};
struct sd_upscaler {};

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;
using namespace Fixture;

struct ScriptedResult {
  int Count;
  sd_tensor_type_t Type;
  std::vector<int64_t> Dims;
};

// Records what the adapter did to the scripted library, and sets how that
// library answers.
struct Script {
  int LiveOptions = 0;
  int LiveTensors = 0;
  int LiveContexts = 0;
  int LiveResults = 0;
  int LiveUpscalers = 0;
  int TotalCalls = 0;
  int GenerateCalls = 0;
  int UpscaleCalls = 0;
  std::map<std::string, sd_status_t> Fail;
  std::map<std::string, std::string> RejectValue;
  sd_status_t CtxCreate = SD_STATUS_OK;
  sd_status_t Generate = SD_STATUS_OK;
  sd_status_t UpscalerCreate = SD_STATUS_OK;
  sd_status_t Upscale = SD_STATUS_OK;
  bool Supports = true;
  bool ThrowInGenerate = false;
  // The scripted function of this name throws when it is called.
  std::string ThrowAt;
  // The scripted option or tensor constructor of this name returns null.
  std::string NullFrom;
  // A failed create, generate, or upscale still returns its object.
  bool HandOutOnFailure = false;
  sd_status_t FileCallbackStatus = SD_STATUS_OK;
  // Extra bytes of a generated image beyond its shape. Negative: fewer bytes.
  int64_t ResultByteDelta = 0;
  ScriptedResult Result{1, SD_TENSOR_U8, {3, 64, 64}};
  int UpscalerScale = 4;
  std::function<void()> InsideGenerate;
  sd_log_cb_t LogCallback = nullptr;
  sd_progress_cb_t ProgressCallback = nullptr;
  sd_dgl_file_access_cb_t FileCallback = nullptr;
  void *FileCallbackData = nullptr;
  std::string LastError = "scripted failure";
  std::vector<int64_t> LastTensorShape;
  // The (key, value) pairs of every option object passed to an operation.
  std::vector<std::pair<std::string, std::string>> Record;
  std::map<std::string, int, std::less<>> RawSets;
  // Every string that the adapter gave to the library.
  std::vector<std::string> Handed;
  // The scripted functions called, in order.
  std::vector<std::string> CallLog;

  bool wasSet(std::string_view Key, std::string_view Value) const {
    return std::any_of(Record.begin(), Record.end(), [&](const auto &Entry) {
      return Entry.first == Key && Entry.second == Value;
    });
  }
  bool wasSetKey(std::string_view Key) const {
    return std::any_of(Record.begin(), Record.end(),
                       [&](const auto &Entry) { return Entry.first == Key; });
  }
  int rawSetCount(std::string_view Key) const {
    const auto It = RawSets.find(Key);
    return It == RawSets.end() ? 0 : It->second;
  }
  bool received(std::string_view Text) const {
    return std::any_of(Handed.begin(), Handed.end(), [&](const auto &String) {
      return String.find(Text) != std::string::npos;
    });
  }
};

Script TheScript;

constexpr int kContext = 1;
constexpr int kImage = 2;
constexpr int kUpscaler = 4;
constexpr int kUpscale = 8;

enum class SetterKind { Bool, Int, Float, String, Tensor, Loras, Embeddings };
struct KeySpec {
  std::string_view Key;
  int Purposes;
  SetterKind Setter;
};
// The keys that the adapter can set, each with its option objects and setter at
// the pinned revision. Every other key is unknown here.
constexpr KeySpec kKeys[] = {
    {"model_path"sv, kContext | kUpscaler, SetterKind::String},
    {"diffusion_model_path"sv, kContext, SetterKind::String},
    {"clip_l_path"sv, kContext, SetterKind::String},
    {"clip_g_path"sv, kContext, SetterKind::String},
    {"t5xxl_path"sv, kContext, SetterKind::String},
    {"vae_path"sv, kContext, SetterKind::String},
    {"taesd_path"sv, kContext, SetterKind::String},
    {"control_net_path"sv, kContext, SetterKind::String},
    {"embeddings"sv, kContext, SetterKind::Embeddings},
    {"n_threads"sv, kContext | kUpscaler, SetterKind::Int},
    {"wtype"sv, kContext, SetterKind::String},
    {"rng_type"sv, kContext, SetterKind::String},
    {"flash_attn_mode"sv, kContext, SetterKind::String},
    {"lora_apply_mode"sv, kContext, SetterKind::String},
    {"backend"sv, kContext | kUpscaler, SetterKind::String},
    {"prompt"sv, kImage, SetterKind::String},
    {"negative_prompt"sv, kImage, SetterKind::String},
    {"width"sv, kImage, SetterKind::Int},
    {"height"sv, kImage, SetterKind::Int},
    {"sample.steps"sv, kImage, SetterKind::Int},
    {"batch_count"sv, kImage, SetterKind::Int},
    {"seed"sv, kImage, SetterKind::Int},
    {"sample.guidance.txt_cfg"sv, kImage, SetterKind::Float},
    {"sample.guidance.distilled_guidance"sv, kImage, SetterKind::Float},
    {"control_strength"sv, kImage, SetterKind::Float},
    {"strength"sv, kImage, SetterKind::Float},
    {"sample.method"sv, kImage, SetterKind::String},
    {"sample.scheduler"sv, kImage, SetterKind::String},
    {"sample.eta"sv, kImage, SetterKind::Float},
    {"clip_skip"sv, kImage, SetterKind::Int},
    {"vae_tiling.enabled"sv, kImage, SetterKind::Bool},
    {"init_image"sv, kImage, SetterKind::Tensor},
    {"mask_image"sv, kImage, SetterKind::Tensor},
    {"control_image"sv, kImage, SetterKind::Tensor},
    {"loras"sv, kImage, SetterKind::Loras},
    {"image"sv, kUpscale, SetterKind::Tensor},
};

void enter(const char *Name) {
  ++TheScript.TotalCalls;
  TheScript.CallLog.emplace_back(Name);
  if (TheScript.ThrowAt == Name) {
    throw std::runtime_error(std::string("scripted throw in ") + Name);
  }
}

std::optional<size_t> byteSize(sd_tensor_type_t Type,
                               const std::vector<int64_t> &Shape) {
  size_t Bytes = 0;
  switch (Type) {
  case SD_TENSOR_U8:
    Bytes = 1;
    break;
  case SD_TENSOR_I32:
  case SD_TENSOR_F32:
    Bytes = 4;
    break;
  case SD_TENSOR_I64:
  case SD_TENSOR_F64:
    Bytes = 8;
    break;
  default:
    return std::nullopt;
  }
  for (const auto Dim : Shape) {
    if (Dim < 0) {
      return std::nullopt;
    }
    Bytes *= static_cast<size_t>(Dim);
  }
  return Bytes;
}

std::string decimal(double Value) {
  std::ostringstream Out;
  Out << Value;
  return Out.str();
}

void publish(const sd_opt_t *Opt) {
  if (Opt != nullptr) {
    TheScript.Record.insert(TheScript.Record.end(), Opt->Values.begin(),
                            Opt->Values.end());
  }
}

sd_result_t *makeResult(int Count, sd_tensor_type_t Type,
                        const std::vector<int64_t> &Dims,
                        int64_t ByteDelta = 0) {
  auto *Result = new sd_result{};
  const auto Bytes = static_cast<size_t>(
      static_cast<int64_t>(byteSize(Type, Dims).value_or(0)) + ByteDelta);
  for (int I = 0; I < Count; ++I) {
    Result->Images.push_back(
        sd_tensor_t{Type, Dims, std::vector<uint8_t>(Bytes, 0x80)});
  }
  ++TheScript.LiveResults;
  return Result;
}

sd_status_t store(const char *Caller, sd_opt_t *Opt, const char *Key,
                  SetterKind Setter, std::string Value) {
  enter(Caller);
  if (Opt == nullptr || Key == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  TheScript.Handed.emplace_back(Key);
  TheScript.Handed.push_back(Value);
  ++TheScript.RawSets[Key];
  const auto *Spec =
      std::find_if(std::begin(kKeys), std::end(kKeys),
                   [&](const KeySpec &Entry) { return Entry.Key == Key; });
  if (Spec == std::end(kKeys)) {
    return SD_STATUS_UNKNOWN_KEY;
  }
  if ((Spec->Purposes & Opt->Purpose) == 0) {
    return SD_STATUS_UNSUPPORTED;
  }
  if (Spec->Setter != Setter) {
    return SD_STATUS_TYPE_MISMATCH;
  }
  if (const auto It = TheScript.Fail.find(Key); It != TheScript.Fail.end()) {
    return It->second;
  }
  if (const auto It = TheScript.RejectValue.find(Key);
      It != TheScript.RejectValue.end() && It->second == Value) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  for (auto &Entry : Opt->Values) {
    if (Entry.first == Key) {
      Entry.second = std::move(Value);
      return SD_STATUS_OK;
    }
  }
  Opt->Values.emplace_back(Key, std::move(Value));
  return SD_STATUS_OK;
}

const char *scriptedGetLastError() {
  enter(__func__);
  return TheScript.LastError.c_str();
}

void scriptedSetLogCallback(sd_log_cb_t Callback, void *) {
  enter(__func__);
  TheScript.LogCallback = Callback;
}

void scriptedSetProgressCallback(sd_progress_cb_t Callback, void *) {
  enter(__func__);
  TheScript.ProgressCallback = Callback;
}

sd_tensor_t *scriptedTensorCreate(sd_tensor_type_t Type, size_t Rank,
                                  const int64_t *Shape, const void *Data,
                                  size_t Size) {
  enter(__func__);
  if (Rank > 8 || (Rank > 0 && Shape == nullptr) ||
      TheScript.NullFrom == __func__) {
    return nullptr;
  }
  std::vector<int64_t> Dims(Shape, Shape + Rank);
  const auto Bytes = byteSize(Type, Dims);
  if (!Bytes || *Bytes != Size || (Size > 0 && Data == nullptr)) {
    return nullptr;
  }
  ++TheScript.LiveTensors;
  TheScript.LastTensorShape = Dims;
  const auto *First = static_cast<const uint8_t *>(Data);
  return new sd_tensor_t{Type, std::move(Dims),
                         std::vector<uint8_t>(First, First + Size)};
}

void scriptedTensorFree(sd_tensor_t *Tensor) {
  enter(__func__);
  if (Tensor != nullptr) {
    --TheScript.LiveTensors;
    delete Tensor;
  }
}

sd_tensor_type_t scriptedTensorType(const sd_tensor_t *Tensor) {
  enter(__func__);
  return Tensor != nullptr ? Tensor->Type : 0;
}

size_t scriptedTensorRank(const sd_tensor_t *Tensor) {
  enter(__func__);
  return Tensor != nullptr ? Tensor->Shape.size() : 0;
}

int64_t scriptedTensorDim(const sd_tensor_t *Tensor, size_t Axis) {
  enter(__func__);
  return Tensor != nullptr && Axis < Tensor->Shape.size() ? Tensor->Shape[Axis]
                                                          : 0;
}

size_t scriptedTensorByteSize(const sd_tensor_t *Tensor) {
  enter(__func__);
  return Tensor != nullptr ? Tensor->Bytes.size() : 0;
}

const void *scriptedTensorData(const sd_tensor_t *Tensor) {
  enter(__func__);
  return Tensor != nullptr && !Tensor->Bytes.empty() ? Tensor->Bytes.data()
                                                     : nullptr;
}

sd_opt_t *createOption(int Purpose, const char *Name) {
  enter(Name);
  if (TheScript.NullFrom == Name) {
    return nullptr;
  }
  ++TheScript.LiveOptions;
  return new sd_opt{Purpose, {}, {}};
}

sd_opt_t *scriptedCtxOptCreate() { return createOption(kContext, __func__); }

sd_opt_t *scriptedImgGenOptCreate() { return createOption(kImage, __func__); }

sd_opt_t *scriptedUpscalerOptCreate() {
  return createOption(kUpscaler, __func__);
}

sd_opt_t *scriptedUpscaleOptCreate() {
  return createOption(kUpscale, __func__);
}

void scriptedOptFree(sd_opt_t *Opt) {
  enter(__func__);
  if (Opt != nullptr) {
    --TheScript.LiveOptions;
    delete Opt;
  }
}

sd_status_t scriptedOptSetBool(sd_opt_t *Opt, const char *Key, bool Value) {
  return store(__func__, Opt, Key, SetterKind::Bool, Value ? "true" : "false");
}

sd_status_t scriptedOptSetInt(sd_opt_t *Opt, const char *Key, int64_t Value) {
  return store(__func__, Opt, Key, SetterKind::Int, std::to_string(Value));
}

sd_status_t scriptedOptSetFloat(sd_opt_t *Opt, const char *Key, double Value) {
  return store(__func__, Opt, Key, SetterKind::Float, decimal(Value));
}

sd_status_t scriptedOptSetString(sd_opt_t *Opt, const char *Key,
                                 const char *Value) {
  if (Value == nullptr) {
    enter(__func__);
    return SD_STATUS_INVALID_ARGUMENT;
  }
  return store(__func__, Opt, Key, SetterKind::String, Value);
}

sd_status_t scriptedOptSetTensor(sd_opt_t *Opt, const char *Key,
                                 const sd_tensor_t *Tensor) {
  if (Tensor == nullptr) {
    enter(__func__);
    return SD_STATUS_INVALID_ARGUMENT;
  }
  std::string Shape;
  for (const auto Dim : Tensor->Shape) {
    Shape += (Shape.empty() ? "" : "x") + std::to_string(Dim);
  }
  const auto Status = store(__func__, Opt, Key, SetterKind::Tensor, Shape);
  if (Status == SD_STATUS_OK) {
    Opt->Tensors[Key] = Tensor->Shape;
  }
  return Status;
}

sd_status_t scriptedOptSetLoras(sd_opt_t *Opt, const char *Key,
                                const sd_lora_t *Values, size_t Count) {
  std::string Text;
  for (size_t I = 0; I < Count; ++I) {
    if (Values == nullptr || Values[I].path == nullptr) {
      enter(__func__);
      return SD_STATUS_INVALID_ARGUMENT;
    }
    Text += (I == 0 ? "" : ",") + std::string(Values[I].path) + "*" +
            decimal(Values[I].multiplier) +
            (Values[I].is_high_noise ? "+high" : "");
  }
  return store(__func__, Opt, Key, SetterKind::Loras, Text);
}

sd_status_t scriptedOptSetEmbeddings(sd_opt_t *Opt, const char *Key,
                                     const sd_embedding_t *Values,
                                     size_t Count) {
  std::string Text;
  for (size_t I = 0; I < Count; ++I) {
    if (Values == nullptr || Values[I].name == nullptr ||
        Values[I].path == nullptr) {
      enter(__func__);
      return SD_STATUS_INVALID_ARGUMENT;
    }
    Text += (I == 0 ? "" : ",") + std::string(Values[I].name) + "=" +
            Values[I].path;
  }
  return store(__func__, Opt, Key, SetterKind::Embeddings, Text);
}

sd_status_t scriptedCtxCreate(const sd_opt_t *Opt, sd_ctx_t **Out) {
  enter(__func__);
  publish(Opt);
  if (Out == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  *Out = nullptr;
  if (Opt == nullptr || Opt->Purpose != kContext) {
    return SD_STATUS_UNSUPPORTED;
  }
  if (TheScript.CtxCreate != SD_STATUS_OK && !TheScript.HandOutOnFailure) {
    return TheScript.CtxCreate;
  }
  ++TheScript.LiveContexts;
  *Out = new sd_ctx_t{};
  return TheScript.CtxCreate;
}

void scriptedCtxFree(sd_ctx_t *Ctx) {
  enter(__func__);
  if (Ctx != nullptr) {
    --TheScript.LiveContexts;
    delete Ctx;
  }
}

sd_status_t scriptedCtxSetFileAccessCallback(sd_ctx_t *Ctx,
                                             sd_dgl_file_access_cb_t Callback,
                                             void *Data) {
  enter(__func__);
  if (Ctx == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  if (TheScript.FileCallbackStatus != SD_STATUS_OK) {
    return TheScript.FileCallbackStatus;
  }
  TheScript.FileCallback = Callback;
  TheScript.FileCallbackData = Data;
  return SD_STATUS_OK;
}

bool scriptedCtxSupportsImageGeneration(const sd_ctx_t *) {
  enter(__func__);
  return TheScript.Supports;
}

sd_status_t scriptedGenerateImage(sd_ctx_t *Ctx, const sd_opt_t *Opt,
                                  sd_result_t **Out) {
  enter(__func__);
  ++TheScript.GenerateCalls;
  publish(Opt);
  if (Out == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  *Out = nullptr;
  if (Ctx == nullptr || Opt == nullptr || Opt->Purpose != kImage) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  if (TheScript.InsideGenerate) {
    TheScript.InsideGenerate();
  }
  if (TheScript.ThrowInGenerate) {
    throw std::runtime_error("scripted exception");
  }
  if (TheScript.Generate != SD_STATUS_OK && !TheScript.HandOutOnFailure) {
    return TheScript.Generate;
  }
  *Out = makeResult(TheScript.Result.Count, TheScript.Result.Type,
                    TheScript.Result.Dims, TheScript.ResultByteDelta);
  return TheScript.Generate;
}

size_t scriptedResultGetImageCount(const sd_result_t *Result) {
  enter(__func__);
  return Result != nullptr ? Result->Images.size() : 0;
}

const sd_tensor_t *scriptedResultGetImage(const sd_result_t *Result,
                                          size_t Index) {
  enter(__func__);
  return Result != nullptr && Index < Result->Images.size()
             ? &Result->Images[Index]
             : nullptr;
}

void scriptedResultFree(sd_result_t *Result) {
  enter(__func__);
  if (Result != nullptr) {
    --TheScript.LiveResults;
    delete Result;
  }
}

sd_status_t scriptedUpscalerCreate(const sd_opt_t *Opt, sd_upscaler_t **Out) {
  enter(__func__);
  publish(Opt);
  if (Out == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  *Out = nullptr;
  if (Opt == nullptr || Opt->Purpose != kUpscaler) {
    return SD_STATUS_UNSUPPORTED;
  }
  if (TheScript.UpscalerCreate != SD_STATUS_OK && !TheScript.HandOutOnFailure) {
    return TheScript.UpscalerCreate;
  }
  ++TheScript.LiveUpscalers;
  *Out = new sd_upscaler{};
  return TheScript.UpscalerCreate;
}

void scriptedUpscalerFree(sd_upscaler_t *Upscaler) {
  enter(__func__);
  if (Upscaler != nullptr) {
    --TheScript.LiveUpscalers;
    delete Upscaler;
  }
}

int scriptedUpscalerGetScale(const sd_upscaler_t *Upscaler) {
  enter(__func__);
  return Upscaler != nullptr ? TheScript.UpscalerScale : 0;
}

sd_status_t scriptedUpscale(sd_upscaler_t *Upscaler, const sd_opt_t *Opt,
                            sd_result_t **Out) {
  enter(__func__);
  ++TheScript.UpscaleCalls;
  publish(Opt);
  if (Out == nullptr) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  *Out = nullptr;
  if (Upscaler == nullptr || Opt == nullptr || Opt->Purpose != kUpscale) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  if (TheScript.Upscale != SD_STATUS_OK && !TheScript.HandOutOnFailure) {
    return TheScript.Upscale;
  }
  const auto Input = Opt->Tensors.find("image");
  if (Input == Opt->Tensors.end() || Input->second.size() != 3) {
    return SD_STATUS_INVALID_ARGUMENT;
  }
  auto Dims = Input->second;
  Dims[1] *= TheScript.UpscalerScale;
  Dims[2] *= TheScript.UpscalerScale;
  *Out = makeResult(1, SD_TENSOR_U8, Dims);
  return TheScript.Upscale;
}

const LibraryApi kScripted = [] {
  LibraryApi Api{};
  Api.GetLastError = scriptedGetLastError;
  Api.SetLogCallback = scriptedSetLogCallback;
  Api.SetProgressCallback = scriptedSetProgressCallback;
  Api.TensorCreate = scriptedTensorCreate;
  Api.TensorFree = scriptedTensorFree;
  Api.TensorType = scriptedTensorType;
  Api.TensorRank = scriptedTensorRank;
  Api.TensorDim = scriptedTensorDim;
  Api.TensorByteSize = scriptedTensorByteSize;
  Api.TensorData = scriptedTensorData;
  Api.CtxOptCreate = scriptedCtxOptCreate;
  Api.ImgGenOptCreate = scriptedImgGenOptCreate;
  Api.UpscalerOptCreate = scriptedUpscalerOptCreate;
  Api.UpscaleOptCreate = scriptedUpscaleOptCreate;
  Api.OptFree = scriptedOptFree;
  Api.OptSetBool = scriptedOptSetBool;
  Api.OptSetInt = scriptedOptSetInt;
  Api.OptSetFloat = scriptedOptSetFloat;
  Api.OptSetString = scriptedOptSetString;
  Api.OptSetTensor = scriptedOptSetTensor;
  Api.OptSetLoras = scriptedOptSetLoras;
  Api.OptSetEmbeddings = scriptedOptSetEmbeddings;
  Api.CtxCreate = scriptedCtxCreate;
  Api.CtxFree = scriptedCtxFree;
  Api.CtxSetFileAccessCallback = scriptedCtxSetFileAccessCallback;
  Api.CtxSupportsImageGeneration = scriptedCtxSupportsImageGeneration;
  Api.GenerateImage = scriptedGenerateImage;
  Api.ResultGetImageCount = scriptedResultGetImageCount;
  Api.ResultGetImage = scriptedResultGetImage;
  Api.ResultFree = scriptedResultFree;
  Api.UpscalerCreate = scriptedUpscalerCreate;
  Api.UpscalerFree = scriptedUpscalerFree;
  Api.UpscalerGetScale = scriptedUpscalerGetScale;
  Api.Upscale = scriptedUpscale;
  return Api;
}();

class SdcppAdapterTest : public ::testing::Test {
protected:
  void SetUp() override {
    namespace fs = std::filesystem;
    S = {};
    LoggerBefore = spdlog::default_logger();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "sd-sdcpp-quiet", std::make_shared<spdlog::sinks::null_sink_mt>()));
    Dir = fs::temp_directory_path() /
          ("wasmedge-sd-sdcpp-" + std::to_string(::getpid()));
    fs::remove_all(Dir);
    fs::create_directories(Dir);
    const auto Plain = "{" + tensor("model.w", "F16", "[4,4]") + "}";
    Good = writeHeaderFile(Dir / "model.safetensors", Plain.size(), Plain, 32);
    Gguf = Dir / "model.gguf";
    {
      std::ofstream Out(Gguf, std::ios::binary);
      Out << "GGUF" << std::string(4096, '\0');
    }
    const auto Quant = "{" + tensor("w", "I8", "[4,4]") + "," +
                       tensor("w.comfy_quant", "U8", "[16]") + "}";
    QuantizedModel =
        writeHeaderFile(Dir / "quantized.safetensors", Quant.size(), Quant, 32);
    const auto GoodPair = lora("[2,320]", "[320,2]");
    GoodLora = writeHeaderFile(Dir / "good-lora.safetensors", GoodPair.size(),
                               GoodPair, 5120);
    const auto BadPair = lora("[2,320]", "[320,3]");
    BadLora = writeHeaderFile(Dir / "bad-lora.safetensors", BadPair.size(),
                              BadPair, 6400);
    const auto Embedding = "{" + tensor("emb", "F32", "[2,768]") + "}";
    GoodEmbedding = writeHeaderFile(Dir / "good-embedding.safetensors",
                                    Embedding.size(), Embedding, 6144);
    BadEmbedding =
        writeHeaderFile(Dir / "bad-embedding.safetensors", Embedding.size(),
                        Embedding, UINT64_C(17) << 20);
  }
  void TearDown() override {
    std::error_code Error;
    std::filesystem::remove_all(Dir, Error);
    spdlog::set_default_logger(LoggerBefore);
    spdlog::drop("sd-sdcpp-quiet");
  }

  SessionFiles model() const {
    SessionFiles Files;
    Files.Model = Good.string();
    return Files;
  }
  static GenerationRequest request() {
    GenerationRequest Req;
    Req.Prompt = "a cat";
    Req.Width = 64;
    Req.Height = 64;
    Req.BatchCount = 1;
    return Req;
  }
  // No object from the scripted library is alive.
  void expectNothingAlive() const {
    EXPECT_EQ(S.LiveOptions, 0);
    EXPECT_EQ(S.LiveTensors, 0);
    EXPECT_EQ(S.LiveContexts, 0);
    EXPECT_EQ(S.LiveResults, 0);
    EXPECT_EQ(S.LiveUpscalers, 0);
  }

  Script &S = TheScript;
  std::filesystem::path Dir, Good, Gguf, QuantizedModel, GoodLora, BadLora,
      GoodEmbedding, BadEmbedding;
  std::shared_ptr<spdlog::logger> LoggerBefore;
};

TEST_F(SdcppAdapterTest, OpenSetsWhatWasConfiguredAndFreesItsOptions) {
  SessionConfig Config;
  Config.Threads = 1;
  Config.WeightType = 4;
  Config.Device = 1;
  Config.Embeddings = {{"e1", "/guest/e1"}};
  SessionFiles Files;
  Files.Model = Good.string();
  Files.Vae = "/host/vae";
  Files.Embeddings = {GoodEmbedding.string()};
  auto Ctx = makeSdcppBackend(kScripted)->open(Config, Files);
  ASSERT_TRUE(Ctx);
  EXPECT_TRUE(S.wasSet("model_path", Good.string()));
  EXPECT_TRUE(S.wasSet("vae_path", "/host/vae"));
  EXPECT_TRUE(S.wasSet("embeddings", "e1=" + GoodEmbedding.string()));
  EXPECT_TRUE(S.wasSet("n_threads", "1"));
  EXPECT_TRUE(S.wasSet("wtype", "q6_K"));
  EXPECT_TRUE(S.wasSet("backend", "cpu"));
  EXPECT_TRUE(S.wasSet("flash_attn_mode", "off"));
  EXPECT_TRUE(S.wasSet("lora_apply_mode", "auto"));
  EXPECT_FALSE(S.wasSetKey("clip_l_path"));
  EXPECT_FALSE(S.received("/guest/"));
  EXPECT_EQ(S.LiveOptions, 0);
  EXPECT_EQ(S.LiveContexts, 1);
  EXPECT_NE(S.FileCallback, nullptr);
  EXPECT_FALSE((*Ctx)->capabilities().ControlNet);
  Ctx->reset();
  EXPECT_EQ(S.LiveContexts, 0);
}

TEST_F(SdcppAdapterTest, TheLibraryGetsAtMostTheMachinesThreads) {
  SessionConfig Config;
  Config.Threads = Limits::MaxThreads;
  SessionFiles Files = model();
  Files.Upscaler = "/host/esrgan";
  ASSERT_TRUE(makeSdcppBackend(kScripted)->open(Config, Files));
  const auto Machine = std::thread::hardware_concurrency();
  const auto Want = std::to_string(Machine != 0 && Machine < Limits::MaxThreads
                                       ? Machine
                                       : Limits::MaxThreads);
  std::vector<std::string> Given;
  for (const auto &[Key, Value] : S.Record) {
    if (Key == "n_threads") {
      Given.push_back(Value);
    }
  }
  EXPECT_EQ(Given, (std::vector<std::string>{Want, Want}));
}

TEST_F(SdcppAdapterTest, PlacementString) {
  SessionConfig Config;
  Config.CpuModules = 0b101;
  ASSERT_TRUE(makeSdcppBackend(kScripted)->open(Config, model()));
  EXPECT_TRUE(S.wasSet("backend", "te=cpu,controlnet=cpu"));
  S = {};
  ASSERT_TRUE(makeSdcppBackend(kScripted)->open({}, model()));
  EXPECT_FALSE(S.wasSetKey("backend"));
}

TEST_F(SdcppAdapterTest, OpenMapsFailuresAndLeavesNothingBehind) {
  const auto Fails = [&](const char *Case, ErrNo Want) {
    SCOPED_TRACE(Case);
    auto R = makeSdcppBackend(kScripted)->open({}, model());
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), Want);
    expectNothingAlive();
    S = {};
  };
  S.CtxCreate = SD_STATUS_LOAD_FAILED;
  Fails("ctx_create LOAD_FAILED", ErrNo::LoadFailed);
  S.CtxCreate = SD_STATUS_LOAD_FAILED;
  S.HandOutOnFailure = true;
  Fails("ctx_create LOAD_FAILED with a context", ErrNo::LoadFailed);
  S.Supports = false;
  Fails("no image generation", ErrNo::LoadFailed);
  S.FileCallbackStatus = SD_STATUS_BUSY;
  Fails("file callback not installed", ErrNo::RuntimeError);
  S.Fail["model_path"] = SD_STATUS_INVALID_ARGUMENT;
  Fails("model_path refused", ErrNo::InvalidArgument);
  S.CtxCreate = SD_STATUS_OUT_OF_MEMORY;
  Fails("ctx_create OUT_OF_MEMORY", ErrNo::RuntimeError);
}

TEST_F(SdcppAdapterTest, ANameTheLibraryRejectsFailsEveryOpen) {
  S.RejectValue["sample.method"] = "tcd";
  const auto Backend = makeSdcppBackend(kScripted);
  auto First = Backend->open({}, model());
  ASSERT_FALSE(First);
  EXPECT_EQ(First.error(), ErrNo::RuntimeError);
  const int Checked = S.rawSetCount("sample.method");
  auto Second = Backend->open({}, model());
  ASSERT_FALSE(Second);
  EXPECT_EQ(Second.error(), ErrNo::RuntimeError);
  EXPECT_EQ(S.rawSetCount("sample.method"), Checked);
  EXPECT_EQ(S.LiveContexts, 0);
}

TEST_F(SdcppAdapterTest, GenerateMapsEveryStatusAndKeepsTheContext) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  const std::pair<sd_status_t, ErrNo> Map[] = {
      {SD_STATUS_INVALID_ARGUMENT, ErrNo::InvalidArgument},
      {SD_STATUS_UNKNOWN_KEY, ErrNo::InvalidArgument},
      {SD_STATUS_TYPE_MISMATCH, ErrNo::InvalidArgument},
      {SD_STATUS_OUT_OF_RANGE, ErrNo::InvalidArgument},
      {SD_STATUS_UNSUPPORTED, ErrNo::InvalidArgument},
      {SD_STATUS_LOAD_FAILED, ErrNo::LoadFailed},
      {SD_STATUS_OUT_OF_MEMORY, ErrNo::RuntimeError},
      {SD_STATUS_GENERATION_FAILED, ErrNo::RuntimeError},
      {SD_STATUS_BUSY, ErrNo::RuntimeError},
      {SD_STATUS_CANCELLED, ErrNo::RuntimeError},
  };
  for (const bool HandOut : {false, true}) {
    S.HandOutOnFailure = HandOut;
    for (const auto &[Status, Want] : Map) {
      S.Generate = Status;
      auto R = Ctx->generate(request(), {}, {});
      ASSERT_FALSE(R);
      EXPECT_EQ(R.error().Code, Want);
      EXPECT_TRUE(R.error().ContextUsable);
      EXPECT_EQ(S.LiveResults, 0);
      EXPECT_EQ(S.LiveOptions, 0);
    }
  }
  S.Generate = SD_STATUS_OK;
  EXPECT_TRUE(Ctx->generate(request(), {}, {}));
}

TEST_F(SdcppAdapterTest, AnExceptionFromTheLibraryIsCaughtAndPoisons) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  S.ThrowInGenerate = true;
  auto R = Ctx->generate(request(), {}, {});
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error().Code, ErrNo::RuntimeError);
  EXPECT_FALSE(R.error().ContextUsable);
  S.ThrowInGenerate = false;
  S.ThrowAt = "scriptedOptSetInt";
  auto Setter = Ctx->generate(request(), {}, {});
  ASSERT_FALSE(Setter);
  EXPECT_EQ(Setter.error().Code, ErrNo::RuntimeError);
  EXPECT_FALSE(Setter.error().ContextUsable);
  EXPECT_EQ(S.LiveOptions, 0);
  S.ThrowAt = "scriptedCtxFree";
  Ctx.reset();
  EXPECT_EQ(S.CallLog.back(), "scriptedCtxFree");
  EXPECT_EQ(S.LiveContexts, 1);
}

TEST_F(SdcppAdapterTest, AFailedGenerationLeavesNothingBehind) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  GenerationInputs In;
  In.Init = std::move(
      *OwnedImage::copyOf(std::vector<uint8_t>(64 * 64 * 3), 64, 64, 3));
  const auto Fails = [&](const char *Case, ErrNo Want) {
    SCOPED_TRACE(Case);
    auto R = Ctx->generate(request(), {}, In);
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error().Code, Want);
    EXPECT_TRUE(R.error().ContextUsable);
    EXPECT_EQ(S.LiveOptions, 0);
    EXPECT_EQ(S.LiveTensors, 0);
    EXPECT_EQ(S.LiveResults, 0);
  };
  S.NullFrom = "scriptedImgGenOptCreate";
  Fails("no option object", ErrNo::RuntimeError);
  S.NullFrom = "scriptedTensorCreate";
  Fails("no tensor", ErrNo::RuntimeError);
  S.NullFrom.clear();
  S.Fail["width"] = SD_STATUS_INVALID_ARGUMENT;
  Fails("width refused", ErrNo::InvalidArgument);
  S.Fail.clear();
  S.Fail["init_image"] = SD_STATUS_INVALID_ARGUMENT;
  Fails("init_image refused", ErrNo::InvalidArgument);
  S.Fail.clear();
  EXPECT_TRUE(Ctx->generate(request(), {}, In));
}

TEST_F(SdcppAdapterTest, NoLibraryCallFollowsAThrow) {
  SessionFiles Files = model();
  Files.Upscaler = "/host/esrgan";
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, Files));
  auto Req = request();
  Req.UpscaleRepeats = 1;
  S.ThrowAt = "scriptedResultFree";
  S.CallLog.clear();
  auto R = Ctx->generate(Req, {}, {});
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error().Code, ErrNo::RuntimeError);
  EXPECT_FALSE(R.error().ContextUsable);
  const auto Thrown =
      std::find(S.CallLog.begin(), S.CallLog.end(), "scriptedResultFree");
  ASSERT_NE(Thrown, S.CallLog.end());
  for (auto It = std::next(Thrown); It != S.CallLog.end(); ++It) {
    EXPECT_NE(It->find("Free"), std::string::npos) << *It;
  }
  EXPECT_EQ(S.UpscaleCalls, 0);
}

TEST_F(SdcppAdapterTest, CopiesResultsAndRejectsMalformedOnes) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  S.Result = {2, SD_TENSOR_U8, {3, 128, 64}};
  auto R = Ctx->generate(request(), {}, {});
  ASSERT_TRUE(R);
  ASSERT_EQ(R->size(), 2U);
  EXPECT_EQ((*R)[0].width(), 128U);
  EXPECT_EQ((*R)[0].height(), 64U);
  EXPECT_EQ(S.LiveResults, 0);
  for (const ScriptedResult &Bad :
       {ScriptedResult{0, SD_TENSOR_U8, {3, 64, 64}},
        ScriptedResult{1, SD_TENSOR_F32, {3, 64, 64}},
        ScriptedResult{1, SD_TENSOR_U8, {64, 64}},
        ScriptedResult{1, SD_TENSOR_U8, {4, 64, 64}}}) {
    S.Result = Bad;
    auto E = Ctx->generate(request(), {}, {});
    ASSERT_FALSE(E);
    EXPECT_EQ(E.error().Code, ErrNo::RuntimeError);
    EXPECT_TRUE(E.error().ContextUsable);
    EXPECT_EQ(S.LiveResults, 0);
  }
  S.Result = {1, SD_TENSOR_U8, {3, 64, 64}};
  for (const int64_t Delta : {INT64_C(1), INT64_C(-1)}) {
    S.ResultByteDelta = Delta;
    auto E = Ctx->generate(request(), {}, {});
    ASSERT_FALSE(E) << Delta;
    EXPECT_EQ(E.error().Code, ErrNo::RuntimeError);
    EXPECT_TRUE(E.error().ContextUsable);
    EXPECT_EQ(S.LiveResults, 0);
  }
}

TEST_F(SdcppAdapterTest, PassesTheRequestAndFreesItsTensors) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  auto Req = request();
  Req.Seed = -7;
  Req.CfgScale = 7.5F;
  Req.Sampler = 1;
  Req.ClipSkip = 2;
  Req.VaeTiling = true;
  GenerationInputs In;
  In.Init = std::move(
      *OwnedImage::copyOf(std::vector<uint8_t>(64 * 32 * 3), 64, 32, 3));
  ASSERT_TRUE(Ctx->generate(Req, {}, In));
  EXPECT_TRUE(S.wasSet("seed", "-7"));
  EXPECT_TRUE(S.wasSet("sample.method", "euler_a"));
  EXPECT_TRUE(S.wasSet("clip_skip", "2"));
  EXPECT_TRUE(S.wasSet("vae_tiling.enabled", "true"));
  EXPECT_FALSE(S.wasSetKey("sample.scheduler"));
  EXPECT_FALSE(S.wasSetKey("strength"));
  EXPECT_EQ(S.LastTensorShape, (std::vector<int64_t>{3, 64, 32}));
  EXPECT_EQ(S.LiveTensors, 0);
  EXPECT_EQ(S.LiveOptions, 0);
}

TEST_F(SdcppAdapterTest, TheTcdSamplerRunsWithAnEtaOfZero) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  const auto Tcd = indexOf(kSamplerNames, "tcd"sv);
  ASSERT_TRUE(Tcd);
  auto Req = request();
  Req.Sampler = *Tcd;
  ASSERT_TRUE(Ctx->generate(Req, {}, {}));
  EXPECT_TRUE(S.wasSet("sample.method", "tcd"));
  EXPECT_TRUE(S.wasSet("sample.eta", "0"));
  for (size_t I = 0; I < kSamplerNames.size(); ++I) {
    if (I == *Tcd) {
      continue;
    }
    S.Record.clear();
    Req.Sampler = static_cast<uint8_t>(I);
    ASSERT_TRUE(Ctx->generate(Req, {}, {})) << kSamplerNames[I];
    EXPECT_TRUE(S.wasSet("sample.method", kSamplerNames[I]));
    EXPECT_FALSE(S.wasSetKey("sample.eta")) << kSamplerNames[I];
  }
  S.Record.clear();
  ASSERT_TRUE(Ctx->generate(request(), {}, {}));
  EXPECT_FALSE(S.wasSetKey("sample.method"));
  EXPECT_FALSE(S.wasSetKey("sample.eta"));
}

TEST_F(SdcppAdapterTest, ARefusedAdapterFileNeverReachesTheLibrary) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  auto Req = request();
  Req.Loras = {{"/guest/bad", 1.0F}};
  const int Calls = S.GenerateCalls;
  const auto Total = S.TotalCalls;
  auto R = Ctx->generate(Req, {{BadLora.string()}}, {});
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error().Code, ErrNo::LoadFailed);
  EXPECT_TRUE(R.error().ContextUsable);
  EXPECT_EQ(S.GenerateCalls, Calls);
  EXPECT_EQ(S.TotalCalls, Total);
  EXPECT_EQ(S.rawSetCount("loras"), 0);
  EXPECT_FALSE(S.received("/guest/"));
  SessionConfig Config;
  Config.Embeddings = {{"e", "/guest/e"}};
  SessionFiles Files = model();
  Files.Embeddings = {BadEmbedding.string()};
  S = {};
  EXPECT_EQ(makeSdcppBackend(kScripted)->open(Config, Files).error(),
            ErrNo::LoadFailed);
  EXPECT_EQ(S.rawSetCount("embeddings"), 0);
  EXPECT_EQ(S.LiveContexts, 0);
  EXPECT_FALSE(S.received("/guest/"));
}

TEST_F(SdcppAdapterTest, AnEmbeddingPathTheLibraryDoesNotKeepIsRefused) {
  const auto Copy = [&](const std::string &Name) {
    const auto Path = Dir / Name;
    std::filesystem::copy_file(GoodEmbedding, Path);
    return Path.string();
  };
  const auto Refused = [&](const SessionConfig &Config,
                           const SessionFiles &Files) {
    const auto Backend = makeSdcppBackend(kScripted);
    const auto Total = S.TotalCalls;
    auto R = Backend->open(Config, Files);
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), ErrNo::LoadFailed);
    EXPECT_EQ(S.TotalCalls, Total);
    expectNothingAlive();
  };
  const auto Plain = Copy("plain.safetensors");
  SessionConfig One;
  One.Embeddings = {{"e1", ""}};
  for (const auto &Path :
       {Copy("back\\slash.safetensors"), Copy("angle>bracket.safetensors"),
        Copy("angle<bracket.safetensors")}) {
    SCOPED_TRACE(Path);
    SessionFiles Files = model();
    Files.Embeddings = {Path};
    Refused(One, Files);
  }
  SessionConfig Two;
  Two.Embeddings = {{"e1", ""}, {"bad\nname", ""}};
  SessionFiles Files = model();
  Files.Embeddings = {Plain, Copy("second<one.safetensors")};
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-embedding-path",
      std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  Refused(Two, Files);
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-embedding-path");
  EXPECT_EQ(Out.str(),
            "error|[WasmEdge-StableDiffusion] embeddings[1] \"bad?name\": the "
            "host path holds a backslash, '<', or '>', which the library does "
            "not keep when it refers to an embedding's file\n");
  EXPECT_EQ(Out.str().find(Dir.string()), std::string::npos);
  Files.Embeddings = {Copy("first<one.safetensors"), Plain};
  Refused(Two, Files);
  Files.Embeddings = {Plain};
  EXPECT_TRUE(makeSdcppBackend(kScripted)->open(One, Files));
}

TEST_F(SdcppAdapterTest, APromptThatNamesAFileNeverReachesTheLibrary) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-file-marker", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  const auto Marker = "<embedding:" + Good.string() + ">";
  for (const std::string &Text :
       {Marker, "a photo of " + Marker + " at dusk", "\\" + Marker,
        std::string("<image:/etc/hosts>")}) {
    for (const bool Negative : {false, true}) {
      auto Req = request();
      (Negative ? Req.NegativePrompt : Req.Prompt) = Text;
      const auto Total = S.TotalCalls;
      const int Calls = S.GenerateCalls;
      auto R = Ctx->generate(Req, {}, {});
      ASSERT_FALSE(R) << Text;
      EXPECT_EQ(R.error().Code, ErrNo::InvalidArgument);
      EXPECT_TRUE(R.error().ContextUsable);
      EXPECT_EQ(S.TotalCalls, Total);
      EXPECT_EQ(S.GenerateCalls, Calls);
      EXPECT_EQ(S.LiveOptions, 0);
    }
  }
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-file-marker");
  const auto Logged = Out.str();
  EXPECT_EQ(std::count(Logged.begin(), Logged.end(), '\n'), 8);
  EXPECT_EQ(Logged.find("embedding:"), std::string::npos);
  EXPECT_EQ(Logged.find("image:"), std::string::npos);
  EXPECT_EQ(Logged.find(Good.string()), std::string::npos);
  EXPECT_EQ(Logged.find("/etc/hosts"), std::string::npos);
  EXPECT_FALSE(S.received("<embedding:"));
  EXPECT_FALSE(S.received("<image:"));
  EXPECT_TRUE(Ctx->generate(request(), {}, {}));
}

TEST_F(SdcppAdapterTest, OnlyTheTwoFileMarkersAreRefused) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  for (const char *Text : {"<lora:x:1>", "<EMBEDDING:/x>", "< embedding:/x>",
                           "<embeddings:/x>", "embedding:/x", "<image>"}) {
    auto Req = request();
    Req.Prompt = Text;
    auto R = Ctx->generate(Req, {}, {});
    ASSERT_TRUE(R) << Text;
    EXPECT_EQ(R->size(), 1U);
    EXPECT_TRUE(S.wasSet("prompt", Text)) << Text;
  }
}

TEST_F(SdcppAdapterTest, LorasAreMergedOnlyIntoWeightsKnownToBePlain) {
  // Whether a session on Config and Files takes LoRAs, and the apply mode that
  // the library gets.
  struct Answer {
    bool Lora;
    std::string Mode;
    bool operator==(const Answer &Other) const {
      return Lora == Other.Lora && Mode == Other.Mode;
    }
  };
  const auto Ask = [&](const SessionConfig &Config,
                       const SessionFiles &Files) -> Answer {
    S.Record.clear();
    auto Ctx = makeSdcppBackend(kScripted)->open(Config, Files);
    EXPECT_TRUE(Ctx);
    for (const auto Name : {"immediately"sv, "auto"sv}) {
      if (S.wasSet("lora_apply_mode", Name)) {
        return {Ctx && (*Ctx)->capabilities().Lora, std::string(Name)};
      }
    }
    return {false, {}};
  };
  const Answer Merged{true, "immediately"};
  const Answer LibrarysChoice{true, "auto"};
  const Answer None{false, "auto"};
  EXPECT_TRUE(Ask({}, model()) == Merged);
  SessionConfig Half;
  Half.WeightType = 1;
  EXPECT_TRUE(Ask(Half, model()) == Merged);
  SessionFiles WithGguf = model();
  WithGguf.Vae = Gguf.string();
  EXPECT_TRUE(Ask({}, WithGguf) == LibrarysChoice);
  EXPECT_TRUE(Ask(Half, WithGguf) == LibrarysChoice);
  SessionFiles WithQuant = model();
  WithQuant.ClipL = QuantizedModel.string();
  EXPECT_TRUE(Ask({}, WithQuant) == LibrarysChoice);
  SessionFiles Missing = model();
  Missing.T5xxl = "/host/missing";
  EXPECT_TRUE(Ask({}, Missing) == LibrarysChoice);
  const auto Fp8 = "{" + tensor("w", "F8_E4M3", "[4,4]") + "}";
  SessionFiles WithFp8 = model();
  WithFp8.Model =
      writeHeaderFile(Dir / "fp8.safetensors", Fp8.size(), Fp8, 16).string();
  EXPECT_TRUE(Ask({}, WithFp8) == LibrarysChoice);
  for (uint8_t Type = 3; Type < kWeightTypeNames.size(); ++Type) {
    SessionConfig Quantized;
    Quantized.WeightType = Type;
    EXPECT_TRUE(Ask(Quantized, model()) == None) << kWeightTypeNames[Type];
    EXPECT_TRUE(Ask(Quantized, WithGguf) == None) << kWeightTypeNames[Type];
  }

  auto Req = request();
  Req.Loras = {{"/guest/good", 0.5F}};
  auto OnAFile = std::move(*makeSdcppBackend(kScripted)->open({}, WithGguf));
  const int Calls = S.GenerateCalls;
  EXPECT_TRUE(OnAFile->generate(Req, {{GoodLora.string()}}, {}));
  EXPECT_EQ(S.GenerateCalls, Calls + 1);
  EXPECT_EQ(S.rawSetCount("loras"), 1);
  EXPECT_TRUE(S.received(GoodLora.string()));

  SessionConfig Quantized;
  Quantized.WeightType = 3;
  auto Asked =
      std::move(*makeSdcppBackend(kScripted)->open(Quantized, model()));
  const auto Total = S.TotalCalls;
  auto R = Asked->generate(Req, {{GoodLora.string()}}, {});
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error().Code, ErrNo::InvalidArgument);
  EXPECT_TRUE(R.error().ContextUsable);
  EXPECT_EQ(S.TotalCalls, Total);
  EXPECT_TRUE(Asked->generate(request(), {}, {}));
  EXPECT_FALSE(S.received("/guest/"));
}

TEST_F(SdcppAdapterTest, TheRowsThatMentionedEmbeddingsStandForAreLimited) {
  const auto Wide = "{" + tensor("clip_l", "F32", "[600,768]") + "," +
                    tensor("clip_g", "F32", "[400,1280]") + "}";
  const auto WideFile = writeHeaderFile(Dir / "wide.safetensors", Wide.size(),
                                        Wide, 600 * 768 * 4 + 400 * 1280 * 4);
  SessionConfig Config;
  Config.Embeddings = {{"Wide", "/guest/wide"}, {"two", "/guest/two"}};
  SessionFiles Files = model();
  Files.Embeddings = {WideFile.string(), GoodEmbedding.string()};
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open(Config, Files));
  const auto Run = [&](std::string_view Prompt, std::string_view Negative) {
    auto Req = request();
    Req.Prompt = Prompt;
    Req.NegativePrompt = Negative;
    const auto Total = S.TotalCalls;
    auto R = Ctx->generate(Req, {}, {});
    if (!R) {
      EXPECT_EQ(R.error().Code, ErrNo::InvalidArgument) << Prompt;
      EXPECT_TRUE(R.error().ContextUsable) << Prompt;
      EXPECT_EQ(S.TotalCalls, Total) << Prompt;
    }
    return static_cast<bool>(R);
  };
  // 600 rows per mention, and 2 rows per mention.
  EXPECT_TRUE(Run("a wide cat", ""));
  EXPECT_TRUE(Run("wide, two two two", "two"));
  EXPECT_FALSE(Run("wide wide", ""));
  EXPECT_FALSE(Run("WIDE", "a Wide one"));
  // This counts, although the library reads nothing between '<' and '>'.
  EXPECT_FALSE(Run("(wide:1.2)<wide>", ""));
  // Not a mention: the name inside a longer word.
  EXPECT_TRUE(Run("wider wide_ awide wide2 \xc3\xa9wide", "widewide"));
  // 212 mentions of 2 rows plus one of 600 give 1,024 rows. One more is
  // refused.
  std::string Many = "wide";
  for (int I = 0; I < 212; ++I) {
    Many += " two";
  }
  EXPECT_TRUE(Run(Many, ""));
  EXPECT_FALSE(Run(Many, "two"));
  EXPECT_TRUE(Run("a cat", ""));
}

TEST_F(SdcppAdapterTest, TheFilePolicyAllowsOnlyWhatWasHandedOver) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  const auto Ask = [&](const std::string &Path, sd_dgl_file_access_t Kind) {
    return S.FileCallback(Path.c_str(), Kind, S.FileCallbackData);
  };
  EXPECT_TRUE(Ask(Good.string(), SD_DGL_FILE_READ_MODEL));
  EXPECT_FALSE(Ask(Good.string(), SD_DGL_FILE_READ));
  EXPECT_FALSE(Ask(Good.string(), SD_DGL_FILE_WRITE));
  EXPECT_FALSE(Ask("/etc/hosts", SD_DGL_FILE_READ_MODEL));
  EXPECT_FALSE(Ask(GoodLora.string(), SD_DGL_FILE_READ_MODEL));
  bool DuringCall = false;
  S.InsideGenerate = [&] {
    DuringCall = Ask(GoodLora.string(), SD_DGL_FILE_READ_MODEL);
  };
  auto Req = request();
  Req.Loras = {{"/guest/good", 0.5F}};
  ASSERT_TRUE(Ctx->generate(Req, {{GoodLora.string()}}, {}));
  EXPECT_TRUE(DuringCall);
  EXPECT_FALSE(Ask(GoodLora.string(), SD_DGL_FILE_READ_MODEL));
  EXPECT_TRUE(S.received(GoodLora.string()));
  EXPECT_FALSE(S.received("/guest/"));
  DuringCall = false;
  S.ThrowInGenerate = true;
  ASSERT_FALSE(Ctx->generate(Req, {{GoodLora.string()}}, {}));
  EXPECT_TRUE(DuringCall);
  EXPECT_FALSE(Ask(GoodLora.string(), SD_DGL_FILE_READ_MODEL));
}

TEST_F(SdcppAdapterTest, TheUpscalerIsCreatedAtOpenAndUsedPerRepeat) {
  SessionFiles Files = model();
  Files.Upscaler = "/host/esrgan";
  S.UpscalerScale = 4;
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, Files));
  EXPECT_TRUE(Ctx->capabilities().Upscaler);
  EXPECT_EQ(Ctx->capabilities().UpscaleFactor, 4U);
  auto Req = request();
  Req.BatchCount = 2;
  Req.UpscaleRepeats = 2;
  S.Result = {2, SD_TENSOR_U8, {3, 64, 64}};
  ASSERT_TRUE(Ctx->generate(Req, {}, {}));
  EXPECT_EQ(S.UpscaleCalls, 4);
  EXPECT_EQ(S.LiveResults, 0);
  S.Upscale = SD_STATUS_LOAD_FAILED;
  for (const bool HandOut : {false, true}) {
    S.HandOutOnFailure = HandOut;
    auto Failed = Ctx->generate(Req, {}, {});
    ASSERT_FALSE(Failed) << HandOut;
    EXPECT_EQ(Failed.error().Code, ErrNo::LoadFailed);
    EXPECT_TRUE(Failed.error().ContextUsable);
    EXPECT_EQ(S.LiveOptions, 0);
    EXPECT_EQ(S.LiveTensors, 0);
    EXPECT_EQ(S.LiveResults, 0);
  }
  Ctx.reset();
  EXPECT_EQ(S.LiveUpscalers, 0);
  for (const bool HandOut : {false, true}) {
    SCOPED_TRACE(HandOut);
    S = {};
    S.UpscalerCreate = SD_STATUS_LOAD_FAILED;
    S.HandOutOnFailure = HandOut;
    auto R = makeSdcppBackend(kScripted)->open({}, Files);
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), ErrNo::LoadFailed);
    expectNothingAlive();
  }
}

TEST_F(SdcppAdapterTest, CreatingABackendDoesNotWaitForALibraryCall) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  std::promise<void> Entered;
  std::promise<void> Leave;
  auto WhenEntered = Entered.get_future();
  const auto WhenLeave = Leave.get_future().share();
  S.InsideGenerate = [&Entered, WhenLeave] {
    Entered.set_value();
    WhenLeave.wait();
  };
  auto Generating = std::async(
      std::launch::async, [&] { return Ctx->generate(request(), {}, {}); });
  const bool Inside = WhenEntered.wait_for(std::chrono::seconds(30)) ==
                      std::future_status::ready;
  std::future<std::shared_ptr<Backend>> Created;
  bool Returned = false;
  if (Inside) {
    Created = std::async(std::launch::async,
                         [] { return makeSdcppBackend(kScripted); });
    Returned =
        Created.wait_for(std::chrono::seconds(30)) == std::future_status::ready;
  }
  Leave.set_value();
  EXPECT_TRUE(Inside);
  EXPECT_TRUE(Returned);
  EXPECT_TRUE(Generating.get());
  if (Created.valid()) {
    EXPECT_NE(Created.get(), nullptr);
  }
}

TEST_F(SdcppAdapterTest, TheLogBridgeDropsLinesBelowInfoAndIgnoresNullText) {
  ASSERT_TRUE(makeSdcppBackend(kScripted)->open({}, model()));
  ASSERT_NE(S.LogCallback, nullptr);
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-log-bridge", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  Logger->set_level(spdlog::level::trace);
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  for (const auto Level :
       {SD_LOG_DEBUG, SD_LOG_VERBOSE, SD_LOG_INFO, SD_LOG_WARN, SD_LOG_ERROR}) {
    S.LogCallback(Level, "text\n", nullptr);
    S.LogCallback(Level, nullptr, nullptr);
  }
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-log-bridge");
  EXPECT_EQ(Out.str(), "debug|[WasmEdge-StableDiffusion] text\n"
                       "warning|[WasmEdge-StableDiffusion] text\n"
                       "error|[WasmEdge-StableDiffusion] text\n");
}

TEST_F(SdcppAdapterTest, ALibraryLineIsOneHostLine) {
  auto Ctx = std::move(*makeSdcppBackend(kScripted)->open({}, model()));
  ASSERT_NE(S.LogCallback, nullptr);
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-library-line", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  Logger->set_level(spdlog::level::trace);
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  S.LogCallback(SD_LOG_WARN, "embedding \"a\nb\x1b[3\xc3\xa9\" failed\n",
                nullptr);
  S.LastError = "bad\nthing\x1b[0m";
  S.Generate = SD_STATUS_LOAD_FAILED;
  const auto R = Ctx->generate(request(), {}, {});
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-library-line");
  ASSERT_FALSE(R);
  EXPECT_EQ(Out.str(),
            "warning|[WasmEdge-StableDiffusion] embedding \"a?b?[3??\" "
            "failed\n"
            "error|[WasmEdge-StableDiffusion] generation failed with status 7: "
            "bad?thing?[0m\n");
}

} // namespace
