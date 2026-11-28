// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_backend_sdcpp.h"

#include "sd_adapter_check.h"
#include "sd_log.h"

#include "common/spdlog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

namespace {

using namespace std::literals;

// The library has process-wide state and documents no thread safety. Thus every
// library call holds this lock. Library callbacks run with the lock held and
// must not take it. The lock is never destroyed, because a context can be freed
// after static destruction starts.
std::mutex &libraryLock() {
  static auto &Lock = *new std::mutex;
  return Lock;
}

// Makes library text safe for one log line: replaces each byte outside 0x20 to
// 0x7e with '?'. Library messages can quote guest text.
std::string printable(std::string_view Text) {
  std::string Safe(Text);
  for (char &C : Safe) {
    const auto Byte = static_cast<unsigned char>(C);
    if (Byte < 0x20 || Byte > 0x7e) {
      C = '?';
    }
  }
  return Safe;
}

// Logs that Step threw an exception. Why is the exception text, if any.
void logThrow(std::string_view Step, const char *Why) noexcept {
  try {
    spdlog::error("[WasmEdge-StableDiffusion] {} threw: {}"sv, Step,
                  Why != nullptr ? printable(Why)
                                 : "an exception of unknown type"s);
  } catch (...) {
  }
}

// Runs Call, a library call, under the lock. If Call throws, this function logs
// the exception and returns false.
template <typename Fn> bool guarded(Fn &&Call) noexcept {
  try {
    const std::lock_guard<std::mutex> Lock(libraryLock());
    Call();
    return true;
  } catch (const std::exception &Error) {
    logThrow("a library call"sv, Error.what());
  } catch (...) {
    logThrow("a library call"sv, nullptr);
  }
  return false;
}

// Frees one library handle with its own free function. If the free throws and
// Threw is set, *Threw becomes true.
template <typename T> struct Release {
  void (*Free)(T *) = nullptr;
  bool *Threw = nullptr;
  void operator()(T *Handle) const noexcept {
    if (!guarded([&] { Free(Handle); }) && Threw != nullptr) {
      *Threw = true;
    }
  }
};
template <typename T> using Owned = std::unique_ptr<T, Release<T>>;

// The library calls of one open or one generation. Threw becomes true when any
// call or free throws. After that, the library state is unknown: run() makes no
// more calls, but frees still run.
struct Calls {
  const LibraryApi &Api;
  bool Threw = false;

  template <typename Fn> bool run(Fn &&Call) noexcept {
    if (Threw) {
      return false;
    }
    if (guarded(std::forward<Fn>(Call))) {
      return true;
    }
    Threw = true;
    return false;
  }
  template <typename T> Owned<T> own(T *Handle, void (*Free)(T *)) noexcept {
    return Owned<T>(Handle, Release<T>{Free, &Threw});
  }
};

// Returns the library's last error text for this thread, made printable. The
// text is per thread, so it stays valid after the lock is released. It can hold
// host paths. Send it to the host log only, never to the guest.
std::string lastError(Calls &Lib) {
  const char *Text = nullptr;
  Lib.run([&] { Text = Lib.Api.GetLastError(); });
  return Text != nullptr ? printable(Text) : std::string();
}

void logFailure(Calls &Lib, std::string_view What, sd_status_t Status) {
  spdlog::error("[WasmEdge-StableDiffusion] {} failed with status {}: {}"sv,
                What, static_cast<int>(Status), lastError(Lib));
}

// The context stays usable after every status that the library returns.
ErrNo codeOf(sd_status_t Status) noexcept {
  switch (Status) {
  case SD_STATUS_INVALID_ARGUMENT:
  case SD_STATUS_UNKNOWN_KEY:
  case SD_STATUS_TYPE_MISMATCH:
  case SD_STATUS_OUT_OF_RANGE:
  case SD_STATUS_UNSUPPORTED:
    return ErrNo::InvalidArgument;
  case SD_STATUS_LOAD_FAILED:
    return ErrNo::LoadFailed;
  default:
    return ErrNo::RuntimeError;
  }
}

Unexpected<BackendFailure> usableFailure(ErrNo Code) noexcept {
  return Unexpected<BackendFailure>(BackendFailure{Code, true});
}

Unexpected<BackendFailure> poisoned() noexcept {
  return Unexpected<BackendFailure>(BackendFailure{ErrNo::RuntimeError, false});
}

// Sends the library's error, warning, and info lines to the host log, one host
// line each, without the trailing line break. It drops all lines below the info
// level, because those lines can hold the prompts of a request.
void logBridge(enum sd_log_level_t Level, const char *Text, void *) noexcept {
  if (Text == nullptr ||
      (Level != SD_LOG_ERROR && Level != SD_LOG_WARN && Level != SD_LOG_INFO)) {
    return;
  }
  try {
    std::string_view Trimmed(Text);
    while (!Trimmed.empty() &&
           (Trimmed.back() == '\n' || Trimmed.back() == '\r')) {
      Trimmed.remove_suffix(1);
    }
    const auto Line = printable(Trimmed);
    switch (Level) {
    case SD_LOG_ERROR:
      spdlog::error("[WasmEdge-StableDiffusion] {}"sv, Line);
      break;
    case SD_LOG_WARN:
      spdlog::warn("[WasmEdge-StableDiffusion] {}"sv, Line);
      break;
    default:
      spdlog::debug("[WasmEdge-StableDiffusion] {}"sv, Line);
      break;
    }
  } catch (...) {
  }
}

void ignoreProgress(int, int, float, void *) noexcept {}

// The files that the library can open for one context: the host paths of the
// session and, during a generation, the LoRA files of that request. The library
// asks during its calls, with the lock held. The lists change only under that
// lock.
struct FilePolicy {
  std::vector<std::string> Session;
  std::vector<std::string> Request;
};

bool allowFile(const char *Path, sd_dgl_file_access_t Access,
               void *Data) noexcept {
  if (Path == nullptr || Data == nullptr || Access != SD_DGL_FILE_READ_MODEL) {
    return false;
  }
  const auto &Policy = *static_cast<const FilePolicy *>(Data);
  const std::string_view Asked(Path);
  const auto Listed = [Asked](const std::vector<std::string> &Paths) noexcept {
    return std::any_of(
        Paths.begin(), Paths.end(),
        [Asked](const std::string &Allowed) { return Allowed == Asked; });
  };
  return Listed(Policy.Session) || Listed(Policy.Request);
}

// Puts the LoRA paths of a request in the policy for one library call and
// removes them when the call returns or throws. Use it only under the lock of
// that call.
class LoraWindow {
public:
  LoraWindow(FilePolicy &Target, std::vector<std::string> &Paths) noexcept
      : Policy(Target), Held(Paths) {
    Policy.Request.swap(Held);
  }
  ~LoraWindow() noexcept { Policy.Request.swap(Held); }
  LoraWindow(const LoraWindow &) = delete;
  LoraWindow &operator=(const LoraWindow &) = delete;

private:
  FilePolicy &Policy;
  std::vector<std::string> &Held;
};

// The model files of a session, in the order of kModelKeys.
std::array<const std::string *, 8> modelFiles(const SessionFiles &Files) {
  return {&Files.Model, &Files.DiffusionModel, &Files.ClipL,
          &Files.ClipG, &Files.T5xxl,          &Files.Vae,
          &Files.Taesd, &Files.ControlNet};
}
constexpr std::array<const char *, 8> kModelKeys = {
    "model_path",  "diffusion_model_path", "clip_l_path",
    "clip_g_path", "t5xxl_path",           "vae_path",
    "taesd_path",  "control_net_path"};

// The modules of kCpuModuleNames in the placement syntax of the library.
constexpr std::array<std::string_view, 3> kCpuPlacements = {
    "te=cpu"sv, "vae=cpu"sv, "controlnet=cpu"sv};
static_assert(kCpuPlacements.size() == kCpuModuleNames.size(),
              "one placement per CPU module");

// This sampler is defined only for an eta from 0 to 1. The adapter always sets
// eta to 0 for it, which is the library default on most models. Then no
// per-model default of the library applies.
constexpr std::string_view kEtaBoundSampler = "tcd"sv;

// Returns the device placement string for a session. An empty string lets the
// library choose.
std::string placement(const SessionConfig &Config) {
  if (kDeviceNames[Config.Device] == "cpu"sv) {
    return "cpu";
  }
  std::string Spec;
  for (size_t I = 0; I < kCpuPlacements.size(); ++I) {
    if (((Config.CpuModules >> I) & 1U) != 0) {
      if (!Spec.empty()) {
        Spec += ',';
      }
      Spec += kCpuPlacements[I];
    }
  }
  return Spec;
}

// Returns the thread count for the library: Requested, capped at the hardware
// thread count when the machine reports one.
uint32_t threadsFor(uint32_t Requested) noexcept {
  const unsigned Machine = std::thread::hardware_concurrency();
  return Machine != 0 && Requested > Machine ? Machine : Requested;
}

// Returns the library spelling of a weight type: the table name, with a
// trailing "_k" changed to "_K".
std::string weightTypeName(std::string_view Name) {
  std::string Spelled(Name);
  if (Spelled.size() >= 2 &&
      Spelled.compare(Spelled.size() - 2, 2, "_k"sv) == 0) {
    Spelled.back() = 'K';
  }
  return Spelled;
}

// Returns true when weight_type makes the library quantize the weights at load.
// Such a session takes no LoRA: on quantized weights, a LoRA pair that does not
// fit its layer fails a ggml assertion, and the guest controls weight_type. A
// model that is quantized in its file still takes LoRAs, because the embedder
// chose that model and its LoRA files.
bool asksForQuantizedWeights(const SessionConfig &Config) {
  if (!Config.WeightType) {
    return false;
  }
  const auto Name = kWeightTypeNames[*Config.WeightType];
  return Name != "f32"sv && Name != "f16"sv && Name != "bf16"sv;
}

// Returns true when the weights are known to be plain (not quantized):
// weight_type is not a quantized type, and every model file is a safetensors
// file with no quantized mark and no FP8 tensor. Plain weights get
// lora_apply_mode "immediately": the library merges LoRAs and skips a pair that
// does not fit. All others get "auto", because a forced merge on quantized
// weights fails an assertion even for a LoRA that fits.
bool hasPlainWeights(const SessionConfig &Config, const SessionFiles &Files) {
  if (asksForQuantizedWeights(Config)) {
    return false;
  }
  for (const auto *Path : modelFiles(Files)) {
    if (!Path->empty() && !isUnquantizedSafetensors(*Path)) {
      return false;
    }
  }
  return true;
}

// Returns true when Text holds an <embedding:...> or <image:...> marker,
// escaped or not. The library opens the file that a marker names, and a session
// file read as an embedding can end the process.
bool namesAFile(std::string_view Text) noexcept {
  return Text.find("<embedding:"sv) != std::string_view::npos ||
         Text.find("<image:"sv) != std::string_view::npos;
}

// Returns true when the library treats Byte as part of a word in its search for
// embedding names.
bool isWordByte(unsigned char Byte) noexcept {
  return std::isalnum(Byte) != 0 || Byte == '_' || Byte >= 128;
}

// Returns Text in the form that the library compares with embedding names.
std::string lowered(std::string_view Text) {
  std::string Lower(Text);
  for (char &C : Lower) {
    C = static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
  }
  return Lower;
}

// A configured embedding that a prompt can mention.
struct Mentionable {
  std::string Name; // lowered
  uint64_t Rows;    // maximum tokens of one mention
};

// Returns an upper bound of the embedding rows that Text mentions, counted up
// to Limit + 1. Each whole-word match of a name, in any case, adds the rows of
// that embedding. The library expands an embedding at no other place, so its
// own count is never larger.
uint64_t mentionedRows(const std::vector<Mentionable> &Embeddings,
                       std::string_view Text, uint64_t Limit) {
  const auto Lower = lowered(Text);
  uint64_t Rows = 0;
  for (const auto &Embedding : Embeddings) {
    const auto &Name = Embedding.Name;
    if (Name.empty()) {
      continue;
    }
    for (size_t At = Lower.find(Name); At != std::string::npos && Rows <= Limit;
         At = Lower.find(Name, At + 1)) {
      const auto End = At + Name.size();
      const bool Whole =
          (At == 0 || !isWordByte(static_cast<unsigned char>(Lower[At - 1]))) &&
          (End == Lower.size() ||
           !isWordByte(static_cast<unsigned char>(Lower[End])));
      if (Whole) {
        Rows =
            Embedding.Rows > Limit - Rows ? Limit + 1 : Rows + Embedding.Rows;
      }
    }
  }
  return Rows;
}

// Returns true when Path holds a backslash, '<', or '>'. The library does not
// keep these characters in the path of an embedding file.
bool notKeptByTheLibrary(std::string_view Path) noexcept {
  return Path.find_first_of("\\<>"sv) != std::string_view::npos;
}

// The host paths that the library calls of the context can open.
std::vector<std::string> contextPaths(const SessionFiles &Files) {
  std::vector<std::string> Paths;
  for (const auto *Path : modelFiles(Files)) {
    if (!Path->empty()) {
      Paths.push_back(*Path);
    }
  }
  Paths.insert(Paths.end(), Files.Embeddings.begin(), Files.Embeddings.end());
  return Paths;
}

// Sets the options of one library call. After the first failure, it sets
// nothing more. failure() gives the code to report.
class Setter {
public:
  Setter(Calls &Library, sd_opt_t *Options) noexcept
      : Lib(Library), Opt(Options) {}

  void setString(const char *Key, const std::string &Value) {
    set(Key, [&] { return Lib.Api.OptSetString(Opt, Key, Value.c_str()); });
  }
  void setInt(const char *Key, int64_t Value) {
    set(Key, [&] { return Lib.Api.OptSetInt(Opt, Key, Value); });
  }
  void setFloat(const char *Key, double Value) {
    set(Key, [&] { return Lib.Api.OptSetFloat(Opt, Key, Value); });
  }
  void setBool(const char *Key, bool Value) {
    set(Key, [&] { return Lib.Api.OptSetBool(Opt, Key, Value); });
  }
  template <size_t N>
  void setName(const char *Key, const std::array<std::string_view, N> &Table,
               uint8_t Index) {
    if (static_cast<size_t>(Index) >= N) {
      fail(ErrNo::InvalidArgument);
      return;
    }
    setString(Key, std::string(Table[Index]));
  }
  // Copies an image into a tensor with the library's shape for images:
  // [channels, width, height].
  void setImage(const char *Key, const OwnedImage &Image) {
    if (Failed) {
      return;
    }
    const auto Pixels = Image.pixels();
    const std::array<int64_t, 3> Shape = {
        static_cast<int64_t>(Image.channels()),
        static_cast<int64_t>(Image.width()),
        static_cast<int64_t>(Image.height())};
    sd_tensor_t *Raw = nullptr;
    if (!Lib.run([&] {
          Raw = Lib.Api.TensorCreate(SD_TENSOR_U8, Shape.size(), Shape.data(),
                                     Pixels.data(), Pixels.size());
        })) {
      fail(ErrNo::RuntimeError);
      return;
    }
    const auto Tensor = Lib.own(Raw, Lib.Api.TensorFree);
    if (!Tensor) {
      spdlog::error("[WasmEdge-StableDiffusion] the library made no tensor for "
                    "{}: {}"sv,
                    Key, lastError(Lib));
      fail(ErrNo::RuntimeError);
      return;
    }
    set(Key, [&] { return Lib.Api.OptSetTensor(Opt, Key, Tensor.get()); });
  }
  void setLoras(const std::vector<LoraRef> &Refs,
                const std::vector<std::string> &Paths) {
    if (Failed) {
      return;
    }
    std::vector<sd_lora_t> Items;
    Items.reserve(Paths.size());
    for (size_t I = 0; I < Paths.size(); ++I) {
      Items.push_back(sd_lora_t{false, Refs[I].Multiplier, Paths[I].c_str()});
    }
    set("loras", [&] {
      return Lib.Api.OptSetLoras(Opt, "loras", Items.data(), Items.size());
    });
  }
  void setEmbeddings(const std::vector<EmbeddingRef> &Refs,
                     const std::vector<std::string> &Paths) {
    if (Failed) {
      return;
    }
    std::vector<sd_embedding_t> Items;
    Items.reserve(Paths.size());
    for (size_t I = 0; I < Paths.size(); ++I) {
      Items.push_back(sd_embedding_t{Refs[I].Name.c_str(), Paths[I].c_str()});
    }
    set("embeddings", [&] {
      return Lib.Api.OptSetEmbeddings(Opt, "embeddings", Items.data(),
                                      Items.size());
    });
  }
  std::optional<ErrNo> failure() const noexcept { return Failed; }

private:
  void fail(ErrNo Code) noexcept {
    if (!Failed) {
      Failed = Code;
    }
  }
  template <typename Fn> void set(const char *Key, Fn &&Call) {
    if (Failed) {
      return;
    }
    sd_status_t Status = SD_STATUS_OK;
    if (!Lib.run([&] { Status = Call(); })) {
      fail(ErrNo::RuntimeError);
      return;
    }
    if (Status != SD_STATUS_OK) {
      spdlog::error(
          "[WasmEdge-StableDiffusion] setting {} failed with status {}: {}"sv,
          Key, static_cast<int>(Status), lastError(Lib));
      fail(codeOf(Status));
    }
  }

  Calls &Lib;
  sd_opt_t *Opt;
  std::optional<ErrNo> Failed;
};

// Returns a new option object from Create. If the library returns none or
// throws, this function logs it and returns null. Lib.Threw tells which.
Owned<sd_opt_t> createOption(Calls &Lib, decltype(&::sd_ctx_opt_create) Create,
                             std::string_view Purpose) {
  sd_opt_t *Raw = nullptr;
  if (!Lib.run([&] { Raw = Create(); })) {
    return Owned<sd_opt_t>();
  }
  auto Opt = Lib.own(Raw, Lib.Api.OptFree);
  if (!Opt) {
    spdlog::error(
        "[WasmEdge-StableDiffusion] the library made no {} options: {}"sv,
        Purpose, lastError(Lib));
  }
  return Opt;
}

// Copies a result tensor into an owned image. The tensor must be 8-bit and rank
// 3, with three channels, sides that fit in 32 bits, and exactly C * W * H
// bytes of data.
Expected<OwnedImage, BackendFailure> copyImage(Calls &Lib,
                                               const sd_tensor_t *Tensor) {
  sd_tensor_type_t Type = 0;
  size_t Rank = 0;
  std::array<int64_t, 3> Dims{};
  size_t Bytes = 0;
  const void *Data = nullptr;
  if (!Lib.run([&] {
        Type = Lib.Api.TensorType(Tensor);
        Rank = Lib.Api.TensorRank(Tensor);
        if (Type == SD_TENSOR_U8 && Rank == Dims.size()) {
          for (size_t Axis = 0; Axis < Dims.size(); ++Axis) {
            Dims[Axis] = Lib.Api.TensorDim(Tensor, Axis);
          }
          Bytes = Lib.Api.TensorByteSize(Tensor);
          Data = Lib.Api.TensorData(Tensor);
        }
      })) {
    return poisoned();
  }
  constexpr int64_t kMaxSide = std::numeric_limits<uint32_t>::max();
  const bool Shaped = Type == SD_TENSOR_U8 && Rank == Dims.size() &&
                      Dims[0] == 3 && Dims[1] > 0 && Dims[1] <= kMaxSide &&
                      Dims[2] > 0 && Dims[2] <= kMaxSide;
  const auto W = static_cast<uint64_t>(Shaped ? Dims[1] : 0);
  const auto H = static_cast<uint64_t>(Shaped ? Dims[2] : 0);
  if (!Shaped || W * H > std::numeric_limits<size_t>::max() / 3 ||
      Bytes != W * H * 3 || Data == nullptr) {
    spdlog::error("[WasmEdge-StableDiffusion] the library returned an image "
                  "of type {}, rank {}, shape [{}, {}, {}], and {} bytes, not "
                  "three channels of 8-bit pixels"sv,
                  Type, Rank, Dims[0], Dims[1], Dims[2], Bytes);
    return usableFailure(ErrNo::RuntimeError);
  }
  auto Image = OwnedImage::copyOf(
      Span<const uint8_t>(static_cast<const uint8_t *>(Data), Bytes),
      static_cast<uint32_t>(W), static_cast<uint32_t>(H), 3);
  if (!Image) {
    spdlog::error("[WasmEdge-StableDiffusion] an image of {}x{} pixels could "
                  "not be copied"sv,
                  W, H);
    return usableFailure(ErrNo::RuntimeError);
  }
  return std::move(*Image);
}

// Returns the single image of an upscale result, or the image at Index of a
// generation result.
Expected<OwnedImage, BackendFailure>
imageOf(Calls &Lib, const sd_result_t *Result, size_t Index) {
  const sd_tensor_t *Tensor = nullptr;
  if (!Lib.run([&] { Tensor = Lib.Api.ResultGetImage(Result, Index); })) {
    return poisoned();
  }
  if (Tensor == nullptr) {
    spdlog::error("[WasmEdge-StableDiffusion] the library returned no image "
                  "at index {}: {}"sv,
                  Index, lastError(Lib));
    return usableFailure(ErrNo::RuntimeError);
  }
  return copyImage(Lib, Tensor);
}

class SdcppContext : public BackendContext {
public:
  explicit SdcppContext(const LibraryApi &Api) noexcept : Library(&Api) {}

  // EmbeddingRows holds, for each embedding of Config, the row count of one
  // mention from the file check.
  static Expected<std::unique_ptr<BackendContext>, ErrNo>
  create(Calls &Lib, const SessionConfig &Config, const SessionFiles &Files,
         const std::vector<uint64_t> &EmbeddingRows);

  const Capabilities &capabilities() const noexcept override { return Caps; }

  Expected<ImageBatch, BackendFailure>
  generate(const GenerationRequest &Req, const RequestFiles &Files,
           const GenerationInputs &Inputs) noexcept override {
    Calls Lib{*Library, false};
    try {
      auto Batch = generateWith(Lib, Req, Files, Inputs);
      if (Lib.Threw) {
        return poisoned();
      }
      return Batch;
    } catch (const std::exception &Error) {
      logThrow("the generation"sv, Error.what());
    } catch (...) {
      logThrow("the generation"sv, nullptr);
    }
    return Unexpected<BackendFailure>(
        BackendFailure{ErrNo::RuntimeError, !Lib.Threw});
  }

private:
  ErrNo createUpscaler(Calls &Lib, const SessionConfig &Config,
                       const SessionFiles &Files, const std::string &Placement);
  Expected<ImageBatch, BackendFailure>
  generateWith(Calls &Lib, const GenerationRequest &Req,
               const RequestFiles &Files, const GenerationInputs &Inputs);
  Expected<OwnedImage, BackendFailure> upscale(Calls &Lib,
                                               const OwnedImage &Image);

  const LibraryApi *Library;
  // The library holds a pointer to Policy while Ctx lives. Thus Policy is
  // declared first and destroyed last.
  FilePolicy Policy;
  Owned<sd_ctx_t> Ctx;
  Owned<sd_upscaler_t> Upscaler;
  Capabilities Caps;
  std::vector<Mentionable> Embeddings;
};

Expected<std::unique_ptr<BackendContext>, ErrNo>
SdcppContext::create(Calls &Lib, const SessionConfig &Config,
                     const SessionFiles &Files,
                     const std::vector<uint64_t> &EmbeddingRows) {
  if (static_cast<size_t>(Config.Device) >= kDeviceNames.size() ||
      static_cast<size_t>(Config.CpuModules) >=
          (size_t{1} << kCpuModuleNames.size()) ||
      (Config.WeightType &&
       static_cast<size_t>(*Config.WeightType) >= kWeightTypeNames.size()) ||
      (Config.Rng && static_cast<size_t>(*Config.Rng) >= kRngNames.size())) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  if (EmbeddingRows.size() != Config.Embeddings.size()) {
    return Unexpected<ErrNo>(ErrNo::InvalidArgument);
  }
  const auto Placement = placement(Config);
  auto Context = std::make_unique<SdcppContext>(Lib.Api);
  Context->Policy.Session = contextPaths(Files);
  for (size_t I = 0; I < EmbeddingRows.size(); ++I) {
    Context->Embeddings.push_back(
        Mentionable{lowered(Config.Embeddings[I].Name), EmbeddingRows[I]});
  }

  auto Opt = createOption(Lib, Lib.Api.CtxOptCreate, "context"sv);
  if (!Opt) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  Setter Set(Lib, Opt.get());
  const auto Paths = modelFiles(Files);
  for (size_t I = 0; I < Paths.size(); ++I) {
    if (!Paths[I]->empty()) {
      Set.setString(kModelKeys[I], *Paths[I]);
    }
  }
  if (!Files.Embeddings.empty()) {
    Set.setEmbeddings(Config.Embeddings, Files.Embeddings);
  }
  if (Config.Threads != 0) {
    Set.setInt("n_threads", threadsFor(Config.Threads));
  }
  if (Config.WeightType) {
    Set.setString("wtype",
                  weightTypeName(kWeightTypeNames[*Config.WeightType]));
  }
  if (Config.Rng) {
    Set.setName("rng_type", kRngNames, *Config.Rng);
  }
  // Always set this option. If it is not set, the library runs a timed probe at
  // start.
  Set.setString("flash_attn_mode", Config.FlashAttention ? "on" : "off");
  Set.setString("lora_apply_mode",
                hasPlainWeights(Config, Files) ? "immediately" : "auto");
  if (!Placement.empty()) {
    Set.setString("backend", Placement);
  }
  if (const auto Failed = Set.failure()) {
    return Unexpected<ErrNo>(*Failed);
  }

  // The library only records the options here. It reads the weights at the
  // first generation. So this call checks only what needs no weights.
  sd_ctx_t *Raw = nullptr;
  sd_status_t Status = SD_STATUS_OK;
  const bool Ran =
      Lib.run([&] { Status = Lib.Api.CtxCreate(Opt.get(), &Raw); });
  Context->Ctx =
      Owned<sd_ctx_t>(Raw, Release<sd_ctx_t>{Lib.Api.CtxFree, nullptr});
  if (!Ran) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  if (Status != SD_STATUS_OK) {
    logFailure(Lib, "creating the context"sv, Status);
    return Unexpected<ErrNo>(codeOf(Status));
  }
  Opt.reset();
  if (!Context->Ctx) {
    spdlog::error("[WasmEdge-StableDiffusion] the library created no "
                  "context"sv);
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  if (!Lib.run([&] {
        Status = Lib.Api.CtxSetFileAccessCallback(Context->Ctx.get(), allowFile,
                                                  &Context->Policy);
      })) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  if (Status != SD_STATUS_OK) {
    logFailure(Lib, "installing the file policy"sv, Status);
    return Unexpected<ErrNo>(codeOf(Status));
  }
  bool Supports = false;
  if (!Lib.run([&] {
        Supports = Lib.Api.CtxSupportsImageGeneration(Context->Ctx.get());
      })) {
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }
  if (!Supports) {
    spdlog::error("[WasmEdge-StableDiffusion] the model cannot generate "
                  "images: {}"sv,
                  lastError(Lib));
    return Unexpected<ErrNo>(ErrNo::LoadFailed);
  }
  if (!Files.Upscaler.empty()) {
    if (const auto Err = Context->createUpscaler(Lib, Config, Files, Placement);
        Err != ErrNo::Success) {
      return Unexpected<ErrNo>(Err);
    }
  }
  Context->Caps.ControlNet = !Files.ControlNet.empty();
  Context->Caps.Lora = !asksForQuantizedWeights(Config);
  return std::unique_ptr<BackendContext>(std::move(Context));
}

ErrNo SdcppContext::createUpscaler(Calls &Lib, const SessionConfig &Config,
                                   const SessionFiles &Files,
                                   const std::string &Placement) {
  auto Opt = createOption(Lib, Lib.Api.UpscalerOptCreate, "upscaler"sv);
  if (!Opt) {
    return ErrNo::RuntimeError;
  }
  Setter Set(Lib, Opt.get());
  Set.setString("model_path", Files.Upscaler);
  if (Config.Threads != 0) {
    Set.setInt("n_threads", threadsFor(Config.Threads));
  }
  if (!Placement.empty()) {
    Set.setString("backend", Placement);
  }
  if (const auto Failed = Set.failure()) {
    return *Failed;
  }
  sd_upscaler_t *Raw = nullptr;
  sd_status_t Status = SD_STATUS_OK;
  const bool Ran =
      Lib.run([&] { Status = Lib.Api.UpscalerCreate(Opt.get(), &Raw); });
  Upscaler = Owned<sd_upscaler_t>(
      Raw, Release<sd_upscaler_t>{Lib.Api.UpscalerFree, nullptr});
  if (!Ran) {
    return ErrNo::RuntimeError;
  }
  if (Status != SD_STATUS_OK) {
    logFailure(Lib, "loading the upscaler"sv, Status);
    return codeOf(Status);
  }
  if (!Upscaler) {
    spdlog::error("[WasmEdge-StableDiffusion] the library created no "
                  "upscaler"sv);
    return ErrNo::RuntimeError;
  }
  int Scale = 0;
  if (!Lib.run([&] { Scale = Lib.Api.UpscalerGetScale(Upscaler.get()); })) {
    return ErrNo::RuntimeError;
  }
  if (Scale <= 0) {
    spdlog::error("[WasmEdge-StableDiffusion] the upscaler reports a scale of "
                  "{}"sv,
                  Scale);
    return ErrNo::LoadFailed;
  }
  Caps.Upscaler = true;
  Caps.UpscaleFactor = static_cast<uint32_t>(Scale);
  return ErrNo::Success;
}

Expected<ImageBatch, BackendFailure>
SdcppContext::generateWith(Calls &Lib, const GenerationRequest &Req,
                           const RequestFiles &Files,
                           const GenerationInputs &Inputs) {
  if (const bool InPrompt = namesAFile(Req.Prompt);
      InPrompt || namesAFile(Req.NegativePrompt)) {
    spdlog::error("[WasmEdge-StableDiffusion] the {} names a file in an "
                  "embedding or image marker"sv,
                  InPrompt ? "prompt"sv : "negative prompt"sv);
    return usableFailure(ErrNo::InvalidArgument);
  }
  if (const auto Rows =
          mentionedRows(Embeddings, Req.Prompt, Limits::MaxEmbeddingRows);
      Rows > Limits::MaxEmbeddingRows ||
      mentionedRows(Embeddings, Req.NegativePrompt,
                    Limits::MaxEmbeddingRows - Rows) >
          Limits::MaxEmbeddingRows - Rows) {
    spdlog::error("[WasmEdge-StableDiffusion] the prompts mention embeddings "
                  "of more than {} rows together"sv,
                  Limits::MaxEmbeddingRows);
    return usableFailure(ErrNo::InvalidArgument);
  }
  if (!Req.Loras.empty() && !Caps.Lora) {
    spdlog::error("[WasmEdge-StableDiffusion] this session takes no LoRA: "
                  "its weight_type is a quantized one"sv);
    return usableFailure(ErrNo::InvalidArgument);
  }
  if (Files.Loras.size() != Req.Loras.size()) {
    spdlog::error("[WasmEdge-StableDiffusion] the request names {} LoRAs and "
                  "comes with {} files"sv,
                  Req.Loras.size(), Files.Loras.size());
    return usableFailure(ErrNo::InvalidArgument);
  }
  if (Req.UpscaleRepeats > 0 && !Upscaler) {
    spdlog::error("[WasmEdge-StableDiffusion] this session has no "
                  "upscaler"sv);
    return usableFailure(ErrNo::InvalidArgument);
  }
  // Check each LoRA file before its path reaches a setter. The library reads
  // the same file next.
  for (const auto &Path : Files.Loras) {
    if (checkAdapterFile(AdapterKind::Lora, Path) != ErrNo::Success) {
      return usableFailure(ErrNo::LoadFailed);
    }
  }

  const auto Opt = createOption(Lib, Lib.Api.ImgGenOptCreate, "generation"sv);
  if (!Opt) {
    return Lib.Threw ? poisoned() : usableFailure(ErrNo::RuntimeError);
  }
  Setter Set(Lib, Opt.get());
  Set.setString("prompt", Req.Prompt);
  Set.setString("negative_prompt", Req.NegativePrompt);
  Set.setInt("width", Req.Width);
  Set.setInt("height", Req.Height);
  Set.setInt("sample.steps", Req.Steps);
  Set.setInt("batch_count", Req.BatchCount);
  Set.setInt("seed", Req.Seed);
  if (Req.CfgScale) {
    Set.setFloat("sample.guidance.txt_cfg", *Req.CfgScale);
  }
  if (Req.Guidance) {
    Set.setFloat("sample.guidance.distilled_guidance", *Req.Guidance);
  }
  if (Req.ControlStrength) {
    Set.setFloat("control_strength", *Req.ControlStrength);
  }
  if (Req.Strength) {
    Set.setFloat("strength", *Req.Strength);
  }
  if (Req.Sampler) {
    Set.setName("sample.method", kSamplerNames, *Req.Sampler);
    if (static_cast<size_t>(*Req.Sampler) < kSamplerNames.size() &&
        kSamplerNames[*Req.Sampler] == kEtaBoundSampler) {
      Set.setFloat("sample.eta", 0.0);
    }
  }
  if (Req.Scheduler) {
    Set.setName("sample.scheduler", kSchedulerNames, *Req.Scheduler);
  }
  if (Req.ClipSkip) {
    Set.setInt("clip_skip", *Req.ClipSkip);
  }
  if (Req.VaeTiling) {
    Set.setBool("vae_tiling.enabled", true);
  }
  if (Inputs.Init) {
    Set.setImage("init_image", *Inputs.Init);
  }
  if (Inputs.Mask) {
    Set.setImage("mask_image", *Inputs.Mask);
  }
  if (Inputs.Control) {
    Set.setImage("control_image", *Inputs.Control);
  }
  if (!Files.Loras.empty()) {
    Set.setLoras(Req.Loras, Files.Loras);
  }
  if (const auto Failed = Set.failure()) {
    return Lib.Threw ? poisoned() : usableFailure(*Failed);
  }

  std::vector<std::string> LoraPaths = Files.Loras;
  sd_result_t *Raw = nullptr;
  sd_status_t Status = SD_STATUS_OK;
  const bool Ran = Lib.run([&] {
    const LoraWindow Window(Policy, LoraPaths);
    Status = Lib.Api.GenerateImage(Ctx.get(), Opt.get(), &Raw);
  });
  auto Result = Lib.own(Raw, Lib.Api.ResultFree);
  if (!Ran) {
    return poisoned();
  }
  if (Status != SD_STATUS_OK) {
    logFailure(Lib, "generation"sv, Status);
    return usableFailure(codeOf(Status));
  }
  if (!Result) {
    spdlog::error("[WasmEdge-StableDiffusion] the generation returned no "
                  "result"sv);
    return usableFailure(ErrNo::RuntimeError);
  }
  size_t Count = 0;
  if (!Lib.run([&] { Count = Lib.Api.ResultGetImageCount(Result.get()); })) {
    return poisoned();
  }
  if (Count == 0) {
    spdlog::error("[WasmEdge-StableDiffusion] the generation returned no "
                  "image"sv);
    return usableFailure(ErrNo::RuntimeError);
  }
  ImageBatch Batch;
  for (size_t I = 0; I < Count; ++I) {
    auto Image = imageOf(Lib, Result.get(), I);
    if (!Image) {
      return Unexpected<BackendFailure>(Image.error());
    }
    Batch.push_back(std::move(*Image));
  }
  Result.reset();
  for (auto &Image : Batch) {
    for (uint32_t Repeat = 0; Repeat < Req.UpscaleRepeats; ++Repeat) {
      auto Larger = upscale(Lib, Image);
      if (!Larger) {
        return Unexpected<BackendFailure>(Larger.error());
      }
      Image = std::move(*Larger);
    }
  }
  return Batch;
}

Expected<OwnedImage, BackendFailure>
SdcppContext::upscale(Calls &Lib, const OwnedImage &Image) {
  const auto Opt = createOption(Lib, Lib.Api.UpscaleOptCreate, "upscale"sv);
  if (!Opt) {
    return Lib.Threw ? poisoned() : usableFailure(ErrNo::RuntimeError);
  }
  Setter Set(Lib, Opt.get());
  Set.setImage("image", Image);
  if (const auto Failed = Set.failure()) {
    return Lib.Threw ? poisoned() : usableFailure(*Failed);
  }
  sd_result_t *Raw = nullptr;
  sd_status_t Status = SD_STATUS_OK;
  const bool Ran = Lib.run(
      [&] { Status = Lib.Api.Upscale(Upscaler.get(), Opt.get(), &Raw); });
  const auto Result = Lib.own(Raw, Lib.Api.ResultFree);
  if (!Ran) {
    return poisoned();
  }
  if (Status != SD_STATUS_OK) {
    logFailure(Lib, "upscaling"sv, Status);
    return usableFailure(codeOf(Status));
  }
  size_t Count = 0;
  if (Result &&
      !Lib.run([&] { Count = Lib.Api.ResultGetImageCount(Result.get()); })) {
    return poisoned();
  }
  if (Count != 1) {
    spdlog::error("[WasmEdge-StableDiffusion] the upscaling returned {} "
                  "images, not one"sv,
                  Count);
    return usableFailure(ErrNo::RuntimeError);
  }
  return imageOf(Lib, Result.get(), 0);
}

// Sets each name of Names under Key on Opt. Returns false when a call throws.
// Sets Refused when the library rejects a name.
bool tryNames(Calls &Lib, sd_opt_t *Opt, const char *Key,
              const std::vector<std::string> &Names, bool &Refused) {
  for (const auto &Name : Names) {
    sd_status_t Status = SD_STATUS_OK;
    if (!Lib.run(
            [&] { Status = Lib.Api.OptSetString(Opt, Key, Name.c_str()); })) {
      return false;
    }
    if (Status != SD_STATUS_OK) {
      spdlog::error("[WasmEdge-StableDiffusion] the library does not accept "
                    "{} \"{}\": {}"sv,
                    Key, Name, lastError(Lib));
      Refused = true;
    }
  }
  return true;
}

template <size_t N>
std::vector<std::string>
namesOf(const std::array<std::string_view, N> &Table,
        std::string (*Spell)(std::string_view) = nullptr) {
  std::vector<std::string> Names;
  Names.reserve(N);
  for (const auto Name : Table) {
    Names.push_back(Spell != nullptr ? Spell(Name) : std::string(Name));
  }
  return Names;
}

class SdcppBackend : public Backend {
public:
  explicit SdcppBackend(const LibraryApi &Library) noexcept : Api(Library) {}

  Expected<std::unique_ptr<BackendContext>, ErrNo>
  open(const SessionConfig &Config,
       const SessionFiles &Files) noexcept override {
    Calls Lib{Api, false};
    try {
      auto Context = openWith(Lib, Config, Files);
      if (Lib.Threw) {
        return Unexpected<ErrNo>(ErrNo::RuntimeError);
      }
      return Context;
    } catch (const std::exception &Error) {
      logThrow("opening the session"sv, Error.what());
    } catch (...) {
      logThrow("opening the session"sv, nullptr);
    }
    return Unexpected<ErrNo>(ErrNo::RuntimeError);
  }

private:
  enum class NameCheck { Pending, Accepted, Refused };

  Expected<std::unique_ptr<BackendContext>, ErrNo>
  openWith(Calls &Lib, const SessionConfig &Config, const SessionFiles &Files) {
    if (Files.Embeddings.size() != Config.Embeddings.size()) {
      return Unexpected<ErrNo>(ErrNo::InvalidArgument);
    }
    std::vector<uint64_t> EmbeddingRows(Files.Embeddings.size(), 1);
    for (size_t I = 0; I < Files.Embeddings.size(); ++I) {
      if (notKeptByTheLibrary(Files.Embeddings[I])) {
        spdlog::error("[WasmEdge-StableDiffusion] embeddings[{}] \"{}\": the "
                      "host path holds a backslash, '<', or '>', which the "
                      "library does not keep when it refers to an embedding's "
                      "file"sv,
                      I, logText(Config.Embeddings[I].Name));
        return Unexpected<ErrNo>(ErrNo::LoadFailed);
      }
      if (checkAdapterFile(AdapterKind::Embedding, Files.Embeddings[I],
                           &EmbeddingRows[I]) != ErrNo::Success) {
        return Unexpected<ErrNo>(ErrNo::LoadFailed);
      }
    }
    if (const auto Err = checkNames(Lib); Err != ErrNo::Success) {
      return Unexpected<ErrNo>(Err);
    }
    return SdcppContext::create(Lib, Config, Files, EmbeddingRows);
  }

  // At the first open, tries every sampler, scheduler, weight type, and RNG
  // name on temporary option objects. A name that the library rejects fails
  // every open of this backend. A throw or a missing option object fails only
  // this open.
  ErrNo checkNames(Calls &Lib) {
    const std::lock_guard<std::mutex> Lock(NamesLock);
    if (Names != NameCheck::Pending) {
      return Names == NameCheck::Accepted ? ErrNo::Success
                                          : ErrNo::RuntimeError;
    }
    bool Refused = false;
    {
      const auto Opt = createOption(Lib, Api.ImgGenOptCreate, "generation"sv);
      if (!Opt ||
          !tryNames(Lib, Opt.get(), "sample.method", namesOf(kSamplerNames),
                    Refused) ||
          !tryNames(Lib, Opt.get(), "sample.scheduler",
                    namesOf(kSchedulerNames), Refused)) {
        return ErrNo::RuntimeError;
      }
    }
    {
      const auto Opt = createOption(Lib, Api.CtxOptCreate, "context"sv);
      if (!Opt ||
          !tryNames(Lib, Opt.get(), "wtype",
                    namesOf(kWeightTypeNames, weightTypeName), Refused) ||
          !tryNames(Lib, Opt.get(), "rng_type", namesOf(kRngNames), Refused)) {
        return ErrNo::RuntimeError;
      }
    }
    Names = Refused ? NameCheck::Refused : NameCheck::Accepted;
    return Refused ? ErrNo::RuntimeError : ErrNo::Success;
  }

  const LibraryApi &Api;
  std::mutex NamesLock;
  NameCheck Names = NameCheck::Pending;
};

} // namespace

std::shared_ptr<Backend> makeSdcppBackend(const LibraryApi &Api) noexcept {
  // The callbacks are process-wide. Register them only if the library lock is
  // free, so module creation never waits for a library call. A busy lock means
  // that the callbacks are set or are being set.
  try {
    const std::unique_lock<std::mutex> Lock(libraryLock(), std::try_to_lock);
    if (Lock.owns_lock()) {
      Api.SetLogCallback(logBridge, nullptr);
      Api.SetProgressCallback(ignoreProgress, nullptr);
    }
  } catch (const std::exception &Error) {
    logThrow("registering the callbacks"sv, Error.what());
  } catch (...) {
    logThrow("registering the callbacks"sv, nullptr);
  }
  try {
    return std::make_shared<SdcppBackend>(Api);
  } catch (...) {
    return nullptr;
  }
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
