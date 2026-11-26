// SPDX-License-Identifier: Apache-2.0
// SigmaStar IPU backend for WasmEdge WASI-NN (Phase 1 minimal implementation).
#include "wasinn_sigmastar_ipu.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_SIGMASTAR_IPU

#include "wasinnenv.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <system_error>

namespace {

using namespace WasmEdge;
using namespace WasmEdge::Host;  // makes WASINN:: visible without pulling in bare Graph/Context

constexpr const char *TAG = "[WASI-NN] SigmaStarIPU backend: ";

// ---- minimal tar extractor (no system() call) -------------------------------
// Parses a ustar archive from memory and extracts regular files into DestDir.
// Rejects path traversal (absolute paths, "..") for safety.
bool octalToSize(const char *Field, size_t &Out) noexcept {
  // tar size field: 12 bytes, leading spaces/NUL, octal digits.
  size_t Value = 0;
  bool Any = false;
  for (int I = 0; I < 12; ++I) {
    char C = Field[I];
    if (C == ' ' || C == '\0')
      continue;
    if (C < '0' || C > '7')
      return false;
    Value = Value * 8 + static_cast<size_t>(C - '0');
    Any = true;
  }
  Out = Value;
  return Any || Value == 0;
}

bool safeRelativePath(const std::string &Name) noexcept {
  if (Name.empty() || Name[0] == '/' || Name.find("..") != std::string::npos)
    return false;
  return true;
}

bool extractTar(WasmEdge::Span<const uint8_t> Tar,
                const std::filesystem::path &Dest) noexcept {
  constexpr size_t Block = 512;
  size_t Off = 0;
  const size_t Total = Tar.size();
  while (Off + Block <= Total) {
    const uint8_t *Hdr = Tar.data() + Off;
    // All-zero block => end of archive.
    bool AllZero = true;
    for (size_t I = 0; I < Block; ++I) {
      if (Hdr[I] != 0) { AllZero = false; break; }
    }
    if (AllZero)
      return true;

    char NameBuf[101] = {};
    std::memcpy(NameBuf, Hdr, 100);
    // Strip trailing NUL/spaces.
    std::string Name(NameBuf);
    while (!Name.empty() && (Name.back() == '\0' || Name.back() == ' '))
      Name.pop_back();

    size_t FileSize = 0;
    if (!octalToSize(reinterpret_cast<const char *>(Hdr + 124), FileSize)) {
      spdlog::error("{}bad tar size field for '{}'", TAG, Name);
      return false;
    }
    char TypeFlag = static_cast<char>(Hdr[156]);
    Off += Block; // advance past header

    if (Off + FileSize > Total) {
      spdlog::error("{}tar entry '{}' size {} exceeds archive", TAG, Name, FileSize);
      return false;
    }
    // Only extract regular files ('0' or NUL) with safe relative paths.
    const bool IsFile = (TypeFlag == '0' || TypeFlag == '\0');
    if (IsFile && safeRelativePath(Name)) {
      std::filesystem::path Target = Dest / Name;
      std::error_code EC;
      std::filesystem::create_directories(Target.parent_path(), EC);
      if (EC) {
        spdlog::error("{}mkdir failed for '{}': {}", TAG, Name, EC.message());
        return false;
      }
      std::ofstream Out(Target, std::ios::binary | std::ios::trunc);
      if (!Out) {
        spdlog::error("{}cannot create '{}'", TAG, Name);
        return false;
      }
      Out.write(reinterpret_cast<const char *>(Tar.data() + Off),
                static_cast<std::streamsize>(FileSize));
      if (!Out) {
        spdlog::error("{}write failed for '{}'", TAG, Name);
        return false;
      }
    }
    // Advance past data, rounded up to 512-byte boundary.
    size_t Padded = (FileSize + Block - 1) / Block * Block;
    Off += Padded;
  }
  return false; // no end-of-archive marker
}

// ---- metadata: only max_new_tokens is consumed in Phase 1 -------------------
int parseMaxNewTokens(WasmEdge::Span<const uint8_t> Meta) noexcept {
  std::string S(Meta.begin(), Meta.end());
  const std::string Key = "\"max_new_tokens\"";
  auto Pos = S.find(Key);
  if (Pos == std::string::npos)
    return 32; // default
  Pos = S.find(':', Pos + Key.size());
  if (Pos == std::string::npos)
    return 32;
  ++Pos;
  while (Pos < S.size() && (S[Pos] == ' ' || S[Pos] == '\t')) ++Pos;
  int Value = 0;
  auto [Ptr, Ec] = std::from_chars(S.data() + Pos, S.data() + S.size(), Value);
  if (Ec != std::errc{})
    return 32;
  return Value > 0 ? Value : 32;
}

bool hasRequiredModelFiles(const std::filesystem::path &Dir) noexcept {
  return std::filesystem::exists(Dir / "config.json") &&
         std::filesystem::exists(Dir / "decoder_model_fixed.sim_sgsimg.img");
}

} // namespace

namespace WasmEdge::Host::WASINN::SigmaStarIPU {

Graph::~Graph() {
  if (!ModelDir.empty()) {
    std::error_code EC;
    std::filesystem::remove_all(ModelDir, EC);
  }
}

Expect<ErrNo> load(WASINN::WasiNNEnvironment &, WASINN::Graph &G,
                   WasmEdge::Span<const WasmEdge::Span<uint8_t>> Builders,
                   WASINN::Device) noexcept {
  if (Builders.empty()) {
    spdlog::error("{}no graph builders", TAG);
    return ErrNo::InvalidArgument;
  }

  std::error_code EC;
  auto &SG = G.get<Graph>();
  std::filesystem::path ModelPath;

  // Phase 3 unload/reload: release resources from a previous load before
  // replacing them. The framework may call load again on the same graph
  // (e.g. after a set_input metadata reload). Without this, the old Model's
  // MI_IPU handles and any owned temp dir would leak.
  if (SG.Model) {
    spdlog::info("{}reload: releasing previous model", TAG);
    SG.Model.reset();  // destructor frees MI_IPU channels
  }
  if (!SG.ModelDir.empty()) {
    std::filesystem::remove_all(SG.ModelDir, EC);
    if (EC) {
      spdlog::warn("{}reload: failed to remove old temp dir {}: {}", TAG,
                   SG.ModelDir.string(), EC.message());
      EC.clear();
    }
    SG.ModelDir.clear();
  }

  // ---- Mode A: path mode (Phase 1 for large models) -------------------------
  // Builders[0] = "file://<absolute model dir>" (UTF-8 bytes).
  // Avoids pushing a ~200MB tar into wasm linear memory. Trusted-app only.
  std::string B0(Builders[0].begin(), Builders[0].end());
  const std::string Prefix = "file://";
  if (B0.rfind(Prefix, 0) == 0) {
    ModelPath = B0.substr(Prefix.size());
    if (!std::filesystem::exists(ModelPath / "config.json")) {
      spdlog::error("{}path-mode model invalid: {}", TAG, ModelPath.string());
      return ErrNo::ModelNotFound;
    }
    // max_new_tokens is not present in model config files (the caller sets
    // it via Model::set_max_new_tokens); keep the Phase 1 default. Sampling
    // params are read internally by Sampler::init(model_dir) and have no
    // public setter. See docs for the Phase 2 metadata-scope limitation.
    SG.MaxNewTokens = 32;
    SG.ModelDir.clear(); // borrowed path: do not delete on destruction
  } else {
    // ---- Mode B: metadata + tar package -------------------------------------
    if (Builders.size() != 2) {
      spdlog::error("{}tar mode needs 2 builders (metadata, tar), got {}",
                    TAG, Builders.size());
      return ErrNo::InvalidArgument;
    }
    SG.MaxNewTokens = parseMaxNewTokens(Builders[0]);
    auto Base = std::filesystem::temp_directory_path(EC);
    if (EC) {
      spdlog::error("{}temp_directory_path failed: {}", TAG, EC.message());
      return ErrNo::RuntimeError;
    }
    ModelPath = Base / ("wasinn_sigs_ipu_" + std::to_string(reinterpret_cast<uintptr_t>(&G)));
    std::filesystem::remove_all(ModelPath, EC);
    std::filesystem::create_directories(ModelPath, EC);
    if (EC) {
      spdlog::error("{}create temp dir failed: {}", TAG, EC.message());
      return ErrNo::RuntimeError;
    }
    if (!extractTar(Builders[1], ModelPath)) {
      spdlog::error("{}tar extraction failed", TAG);
      std::filesystem::remove_all(ModelPath, EC);
      return ErrNo::InvalidArgument;
    }
    SG.ModelDir = ModelPath; // owned temp dir: removed on destruction
  }

  if (!hasRequiredModelFiles(ModelPath)) {
    spdlog::error("{}model package incomplete: {}", TAG, ModelPath.string());
    if (!SG.ModelDir.empty()) {
      std::filesystem::remove_all(SG.ModelDir, EC);
      SG.ModelDir.clear();
    }
    return ErrNo::ModelNotFound;
  }

  // Qwen2 architecture is registered by qwen2.cpp (linked into this plugin).
  // Phase 3 perf: time model loading separately from inference.
  auto LoadStart = std::chrono::steady_clock::now();
  // Phase 3 error handling: from_pretrained() throws (e.g. MI_IPU_CreateCHN
  // failure when MMA heap is exhausted). load() is noexcept, so an uncaught
  // exception would call std::terminate and abort the whole WasmEdge process.
  // Translate it to a WASI-NN error and leave the graph unready.
  try {
    SG.Model = sgsmz::Model::from_pretrained(ModelPath.string());
  } catch (const std::exception &E) {
    spdlog::error("{}from_pretrained threw for {}: {}", TAG,
                  ModelPath.string(), E.what());
    SG.Model.reset();
    G.setInvalid();
    if (!SG.ModelDir.empty()) {
      std::filesystem::remove_all(SG.ModelDir, EC);
      SG.ModelDir.clear();
    }
    return ErrNo::RuntimeError;
  }
  if (!SG.Model) {
    spdlog::error("{}from_pretrained failed: {}", TAG, ModelPath.string());
    G.setInvalid();
    if (!SG.ModelDir.empty()) {
      std::filesystem::remove_all(SG.ModelDir, EC);
      SG.ModelDir.clear();
    }
    return ErrNo::RuntimeError;
  }
  auto LoadMs = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - LoadStart)
                    .count();
  SG.Model->set_max_new_tokens(SG.MaxNewTokens);
  G.setReady();
  spdlog::info("{}model loaded: {} (max_new_tokens={}, load={:.1f}ms)", TAG,
               ModelPath.string(), SG.MaxNewTokens, LoadMs);
  return ErrNo::Success;
}

Expect<ErrNo> initExecCtx(WASINN::WasiNNEnvironment &, WASINN::Graph &G,
                            WASINN::Context &C) noexcept {
  if (!G.isReady()) {
    spdlog::error("{}graph not ready", TAG);
    return ErrNo::InvalidArgument;
  }
  auto &SC = C.get<Context>();
  SC.InputIds.clear();
  SC.OutputIds.clear();
  SC.HasInput = false;
  SC.HasOutput = false;
  // Phase 3: reset single-token streaming state on context re-init, otherwise
  // a reused context can emit stale tokens from a prior stream.
  SC.ComputeSingleStarted = false;
  SC.NextTokenIndex = 0;
  return ErrNo::Success;
}

Expect<ErrNo> setInput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                       WASINN::Context &C, uint32_t Index,
                       const WASINN::TensorData &Tensor) noexcept {
  if (Index != 0) {
    spdlog::error("{}only index 0 supported, got {}", TAG, Index);
    return ErrNo::InvalidArgument;
  }
  if (Tensor.RType != TensorType::I32) {
    spdlog::error("{}tensor must be I32 token ids", TAG);
    return ErrNo::InvalidArgument;
  }
  if (Tensor.Dimension.size() != 1) {
    spdlog::error("{}expect 1-D token id tensor", TAG);
    return ErrNo::InvalidArgument;
  }
  if (Tensor.Tensor.size() % sizeof(int32_t) != 0) {
    spdlog::error("{}tensor byte size not multiple of 4", TAG);
    return ErrNo::InvalidArgument;
  }
  auto &SC = C.get<Context>();
  SC.InputIds.clear();
  const int32_t *P = reinterpret_cast<const int32_t *>(Tensor.Tensor.data());
  size_t Count = Tensor.Tensor.size() / sizeof(int32_t);
  SC.InputIds.assign(P, P + Count);
  SC.HasInput = !SC.InputIds.empty();
  SC.HasOutput = false;
  // Phase 3: also reset streaming state on new input, so a context switching
  // from single-token mode to normal mode starts clean.
  SC.ComputeSingleStarted = false;
  SC.NextTokenIndex = 0;
  if (!SC.HasInput) {
    spdlog::error("{}set_input: empty token tensor", TAG);
    return ErrNo::InvalidArgument;
  }
  return ErrNo::Success;
}

Expect<ErrNo> compute(WASINN::WasiNNEnvironment &, WASINN::Graph &G,
                          WASINN::Context &C) noexcept {
  auto &SC = C.get<Context>();
  if (!SC.HasInput) {
    spdlog::error("{}no input set", TAG);
    return ErrNo::InvalidArgument;
  }
  auto &SG = G.get<Graph>();
  if (!SG.Model) {
    spdlog::error("{}model not loaded", TAG);
    return ErrNo::RuntimeError;
  }
  try {
    // Phase 3 perf: use the streamer to capture TTFT and total decode time.
    TokenCollector Collector;
    Collector.start();
    SG.Model->generate(SC.InputIds, &Collector);
    SC.OutputIds = Collector.generated();
    SC.HasOutput = true;
    const size_t Count = SC.OutputIds.size();
    const double Ttft = Collector.ttftMs();
    const double Total = Collector.totalMs();
    const double DecodeMs = Total - Ttft;
    const double Tps = DecodeMs > 0.0 && Count > 1
                           ? (static_cast<double>(Count - 1) / DecodeMs) * 1000.0
                           : 0.0;
    spdlog::info("{}perf: tokens={} ttft={:.1f}ms total={:.1f}ms decode={:.2f} tok/s",
                 TAG, Count, Ttft, Total, Tps);
    // Emit generated token ids as text for T7 parity checking against native demo.
    {
      std::string Ids;
      for (size_t I = 0; I < Count && I < 32; ++I) {
        if (I) Ids += ",";
        Ids += std::to_string(SC.OutputIds[I]);
      }
      spdlog::info("{}output_ids=[{}] (count={})", TAG, Ids, Count);
    }
  } catch (const std::exception &E) {
    // Phase 3 error handling: invalidate output state on failure so a later
    // getOutput cannot return stale tokens from a previous successful run.
    SC.OutputIds.clear();
    SC.HasOutput = false;
    SC.ComputeSingleStarted = false;
    SC.NextTokenIndex = 0;
    spdlog::error("{}generate() threw: {}", TAG, E.what());
    return ErrNo::RuntimeError;
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutput(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                        WASINN::Context &C, uint32_t Index,
                        WasmEdge::Span<uint8_t> OutBuffer,
                        uint32_t &BytesWritten) noexcept {
  if (Index != 0) {
    spdlog::error("{}get_output: only index 0 supported, got {}", TAG, Index);
    return ErrNo::InvalidArgument;
  }
  auto &SC = C.get<Context>();
  if (!SC.HasOutput) {
    spdlog::error("{}no output available", TAG);
    return ErrNo::InvalidArgument;
  }
  size_t Need = SC.OutputIds.size() * sizeof(int32_t);
  if (OutBuffer.size() < Need) {
    spdlog::error("{}output buffer too small: {} < {}", TAG, OutBuffer.size(), Need);
    return ErrNo::TooLarge;
  }
  std::memcpy(OutBuffer.data(), SC.OutputIds.data(), Need);
  BytesWritten = static_cast<uint32_t>(Need);
  return ErrNo::Success;
}

// ---- Phase 2: ABI-compatible single-token streaming ------------------------
// sgs Model::generate() has no public single-step API (run_prefill_step /
// run_decode_step are protected), so on the first compute_single we run the
// full generation with a TokenCollector and stage the tokens. Subsequent
// compute_single calls are no-ops; get_output_single emits one I32 token each.
Expect<ErrNo> computeSingle(WASINN::WasiNNEnvironment &, WASINN::Graph &G,
                            WASINN::Context &C) noexcept {
  auto &SC = C.get<Context>();
  if (!SC.HasInput) {
    spdlog::error("{}compute_single: no input set", TAG);
    return ErrNo::InvalidArgument;
  }
  if (!SC.ComputeSingleStarted) {
    auto &SG = G.get<Graph>();
    if (!SG.Model) {
      spdlog::error("{}compute_single: model not loaded", TAG);
      return ErrNo::RuntimeError;
    }
    TokenCollector Collector;
    try {
      // Non-streaming compute path already verified; here we pass the
      // collector so tokens are staged for get_output_single.
      SG.Model->generate(SC.InputIds, &Collector);
    } catch (const std::exception &E) {
      SC.OutputIds.clear();
      SC.HasOutput = false;
      SC.ComputeSingleStarted = false;
      SC.NextTokenIndex = 0;
      spdlog::error("{}compute_single: generate() threw: {}", TAG, E.what());
      return ErrNo::RuntimeError;
    }
    SC.OutputIds = Collector.generated();
    SC.NextTokenIndex = 0;
    SC.ComputeSingleStarted = true;
    SC.HasOutput = true;
    spdlog::info("{}compute_single: staged {} tokens", TAG, SC.OutputIds.size());
  }
  return ErrNo::Success;
}

Expect<ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                              WASINN::Context &C, uint32_t Index,
                              WasmEdge::Span<uint8_t> OutBuffer,
                              uint32_t &BytesWritten) noexcept {
  if (Index != 0) {
    spdlog::error("{}get_output_single: only index 0 supported, got {}", TAG, Index);
    return ErrNo::InvalidArgument;
  }
  auto &SC = C.get<Context>();
  if (!SC.ComputeSingleStarted) {
    spdlog::error("{}get_output_single: compute_single not called", TAG);
    return ErrNo::InvalidArgument;
  }
  if (SC.NextTokenIndex >= SC.OutputIds.size()) {
    BytesWritten = 0; // no more tokens; guest can call fini_single
    return ErrNo::Success;
  }
  constexpr size_t Need = sizeof(int32_t);
  if (OutBuffer.size() < Need) {
    return ErrNo::TooLarge;
  }
  int32_t Token = static_cast<int32_t>(SC.OutputIds[SC.NextTokenIndex++]);
  std::memcpy(OutBuffer.data(), &Token, Need);
  BytesWritten = static_cast<uint32_t>(Need);
  return ErrNo::Success;
}

Expect<ErrNo> finiSingle(WASINN::WasiNNEnvironment &, WASINN::Graph &,
                         WASINN::Context &C) noexcept {
  auto &SC = C.get<Context>();
  SC.ComputeSingleStarted = false;
  SC.NextTokenIndex = 0;
  SC.OutputIds.clear();
  SC.HasOutput = false;
  return ErrNo::Success;
}

} // namespace WasmEdge::Host::WASINN::SigmaStarIPU

#endif // WASMEDGE_PLUGIN_WASI_NN_BACKEND_SIGMASTAR_IPU
