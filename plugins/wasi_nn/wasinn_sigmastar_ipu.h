// SPDX-License-Identifier: Apache-2.0
// SigmaStar IPU backend for WasmEdge WASI-NN (Phase 1 minimal).
// Adapts WASI-NN ABI to sgsmz::Model (sgs_transformers) on D3 (mhera) MI_IPU.
#pragma once

#include "plugin/plugin.h"
#include "wasinntypes.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_SIGMASTAR_IPU
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

// sgs_transformers C++ API
#include "model.h"
#include "streamer.h"
#endif

namespace WasmEdge::Host::WASINN {
struct WasiNNEnvironment;
class Graph;
class Context;
}

namespace WasmEdge::Host::WASINN::SigmaStarIPU {

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_SIGMASTAR_IPU

// Model-level state, filled into the dispatch-layer WASINN::Graph variant.
struct Graph {
  std::filesystem::path ModelDir;         // unpacked temp model directory
  std::unique_ptr<sgsmz::Model> Model;    // from_pretrained() result
  int MaxNewTokens = 32;                  // Phase 1: from metadata JSON
  ~Graph();                               // removes ModelDir on destruction
};

// BaseStreamer that collects generated tokens into a vector.
// sgs Model::generate() calls put(input_ids,false) once for the prompt, then
// put({token},false) per generated token, then end(). We skip the prompt batch
// (first put) so the collector holds only generated tokens.
class TokenCollector final : public sgsmz::BaseStreamer {
public:
  using Clock = std::chrono::steady_clock;

  // Phase 3 perf: start timing just before generate() is called.
  void start() noexcept {
    Start_ = Clock::now();
    FirstToken_ = Clock::time_point{};
    End_ = Clock::time_point{};
    Generated_.clear();
    SkippedPrompt_ = false;
  }

  void put(const std::vector<int> &Ids, bool IsEnd) override {
    (void)IsEnd;
    if (SkippedPrompt_) {
      if (FirstToken_ == Clock::time_point{})
        FirstToken_ = Clock::now();  // first generated token -> TTFT
      Generated_.insert(Generated_.end(), Ids.begin(), Ids.end());
    } else {
      SkippedPrompt_ = true; // first put is the prompt; ignore it
    }
  }
  void end() override { End_ = Clock::now(); }
  const std::vector<int> &generated() const noexcept { return Generated_; }
  void clear() noexcept { Generated_.clear(); SkippedPrompt_ = false; }

  // Milliseconds from start() to first generated token / end().
  double ttftMs() const noexcept {
    return FirstToken_ == Clock::time_point{}
        ? 0.0
        : std::chrono::duration<double, std::milli>(FirstToken_ - Start_).count();
  }
  double totalMs() const noexcept {
    return End_ == Clock::time_point{}
        ? 0.0
        : std::chrono::duration<double, std::milli>(End_ - Start_).count();
  }

private:
  bool SkippedPrompt_ = false;
  std::vector<int> Generated_;
  Clock::time_point Start_{};
  Clock::time_point FirstToken_{};
  Clock::time_point End_{};
};

// Per-inference state, filled into the dispatch-layer WASINN::Context variant.
struct Context {
  Context(uint32_t, Graph &) noexcept {}  // dispatch-layer emplace signature
  std::vector<int> InputIds;              // setInput: I32 token ids
  std::vector<int> OutputIds;             // compute: generated token ids
  bool HasInput = false;
  bool HasOutput = false;

  // Phase 2 single-token streaming state.
  bool ComputeSingleStarted = false;      // compute_single has begun
  size_t NextTokenIndex = 0;              // next token to emit via get_output_single
};

#else
struct Graph {};
struct Context {
  Context(uint32_t, Graph &) noexcept {}
};
#endif

struct Environ {};

Expect<WASINN::ErrNo> load(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                           Span<const Span<uint8_t>> Builders,
                           WASINN::Device Device) noexcept;
Expect<WASINN::ErrNo> initExecCtx(WASINN::WasiNNEnvironment &Env,
                                  WASINN::Graph &G, WASINN::Context &C) noexcept;
Expect<WASINN::ErrNo> setInput(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                               WASINN::Context &C, uint32_t Index,
                               const TensorData &Tensor) noexcept;
Expect<WASINN::ErrNo> getOutput(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                                WASINN::Context &C, uint32_t Index,
                                Span<uint8_t> OutBuffer,
                                uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> compute(WASINN::WasiNNEnvironment &Env, WASINN::Graph &G,
                              WASINN::Context &C) noexcept;

// Phase 2 streaming (ABI-compatible). Note: sgs Model::generate() is a
// one-shot blocking call; these functions stage all tokens on first
// compute_single and emit them one by one via get_output_single.
Expect<WASINN::ErrNo> computeSingle(WASINN::WasiNNEnvironment &Env,
                                    WASINN::Graph &G,
                                    WASINN::Context &C) noexcept;
Expect<WASINN::ErrNo> getOutputSingle(WASINN::WasiNNEnvironment &Env,
                                      WASINN::Graph &G, WASINN::Context &C,
                                      uint32_t Index, WasmEdge::Span<uint8_t> OutBuffer,
                                      uint32_t &BytesWritten) noexcept;
Expect<WASINN::ErrNo> finiSingle(WASINN::WasiNNEnvironment &Env,
                                 WASINN::Graph &G,
                                 WASINN::Context &C) noexcept;

} // namespace WasmEdge::Host::WASINN::SigmaStarIPU
