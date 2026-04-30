// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "wasinntypes.h"

#include <atomic>
#include <cstdint>
#include <future>

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

/// Thread-safe state machine for tracking async GGML inference across WASM
/// execution boundaries. Non-copyable to prevent dangling future references.
/// Move-safe for std::vector<Context> reallocation.
struct AsyncContext {
  /// Inference lifecycle phases.
  /// Transitions: Idle → PromptEval → Generating → Complete | Error.
  /// Reset to Idle via reset().
  enum class Phase : uint8_t {
    Idle,       // No active computation.
    PromptEval, // llama_decode loop over input batch chunks.
    Generating, // Autoregressive token sampling loop.
    Complete,   // Output ready in Context::LlamaOutputs.
    Error       // Computation failed; inspect LastError.
  };

  /// Current execution phase.
  /// Written by worker thread (release), read by WASM poller (acquire).
  std::atomic<Phase> CurrentPhase{Phase::Idle};

  /// Resolves to the final ErrNo when compute completes or fails.
  /// Shared to allow concurrent waiters (WASM poll + host timeout).
  std::shared_future<ErrNo> ComputeFuture;

  /// Cooperative cancellation flag. Set by WASM guest or Executor::stop();
  /// checked in the sampleOutput loop between llama_decode calls.
  std::atomic<bool> CancelRequested{false};

  /// High-granularity progress counters for streaming support.
  /// Worker thread writes with relaxed ordering; Phase transitions provide
  /// the release-acquire fence that makes counter values visible to readers.
  struct Progress {
    std::atomic<uint64_t> TokensGenerated{0};
    std::atomic<uint64_t> TokensTotal{0};
    std::atomic<uint64_t> PromptTokensEvaluated{0};
    std::atomic<uint64_t> PromptTokensTotal{0};
  } Stats;

  /// Error code from worker thread. Written under release after Phase
  /// transition to Error; read under acquire by the poller.
  std::atomic<ErrNo> LastError{ErrNo::Success};

  // --- Lifecycle ---

  AsyncContext() noexcept = default;
  ~AsyncContext() noexcept = default;

  /// Copy prohibited: would alias the shared_future's backing promise.
  AsyncContext(const AsyncContext &) = delete;
  AsyncContext &operator=(const AsyncContext &) = delete;

  /// Move: transfers future ownership cleanly. Source left in Idle state.
  /// Required for std::vector<Context> reallocation.
  AsyncContext(AsyncContext &&Other) noexcept
      : CurrentPhase(Other.CurrentPhase.load(std::memory_order_acquire)),
        ComputeFuture(std::move(Other.ComputeFuture)),
        CancelRequested(
            Other.CancelRequested.load(std::memory_order_acquire)),
        LastError(Other.LastError.load(std::memory_order_acquire)) {
    Stats.TokensGenerated.store(
        Other.Stats.TokensGenerated.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    Stats.TokensTotal.store(
        Other.Stats.TokensTotal.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    Stats.PromptTokensEvaluated.store(
        Other.Stats.PromptTokensEvaluated.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    Stats.PromptTokensTotal.store(
        Other.Stats.PromptTokensTotal.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    Other.CurrentPhase.store(Phase::Idle, std::memory_order_release);
    Other.CancelRequested.store(false, std::memory_order_release);
    Other.LastError.store(ErrNo::Success, std::memory_order_release);
  }

  AsyncContext &operator=(AsyncContext &&Other) noexcept {
    if (this != &Other) {
      CurrentPhase.store(
          Other.CurrentPhase.load(std::memory_order_acquire),
          std::memory_order_release);
      ComputeFuture = std::move(Other.ComputeFuture);
      CancelRequested.store(
          Other.CancelRequested.load(std::memory_order_acquire),
          std::memory_order_release);
      LastError.store(Other.LastError.load(std::memory_order_acquire),
                      std::memory_order_release);
      Stats.TokensGenerated.store(
          Other.Stats.TokensGenerated.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      Stats.TokensTotal.store(
          Other.Stats.TokensTotal.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      Stats.PromptTokensEvaluated.store(
          Other.Stats.PromptTokensEvaluated.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      Stats.PromptTokensTotal.store(
          Other.Stats.PromptTokensTotal.load(std::memory_order_relaxed),
          std::memory_order_relaxed);
      Other.CurrentPhase.store(Phase::Idle, std::memory_order_release);
      Other.CancelRequested.store(false, std::memory_order_release);
      Other.LastError.store(ErrNo::Success, std::memory_order_release);
    }
    return *this;
  }

  // --- Query ---

  /// Non-blocking phase read for WASM polling loops.
  Phase poll() const noexcept {
    return CurrentPhase.load(std::memory_order_acquire);
  }

  /// Returns true if computation has reached a terminal phase.
  bool isTerminal() const noexcept {
    auto P = poll();
    return P == Phase::Complete || P == Phase::Error;
  }

  /// Block with timeout. Returns true if computation completed within
  /// the given duration. No-op if no future is active.
  template <typename Rep, typename Period>
  bool waitFor(const std::chrono::duration<Rep, Period> &Timeout) const {
    if (!ComputeFuture.valid()) {
      return true;
    }
    return ComputeFuture.wait_for(Timeout) == std::future_status::ready;
  }

  // --- Control ---

  /// Request cooperative cancellation. The worker thread checks this flag
  /// in the sampleOutput loop between llama_decode calls.
  void cancel() noexcept {
    CancelRequested.store(true, std::memory_order_release);
  }

  /// Reset all state for context reuse. Must only be called when no
  /// worker thread is active (Phase ∈ {Idle, Complete, Error}).
  void reset() noexcept {
    CurrentPhase.store(Phase::Idle, std::memory_order_release);
    CancelRequested.store(false, std::memory_order_release);
    LastError.store(ErrNo::Success, std::memory_order_release);
    Stats.TokensGenerated.store(0, std::memory_order_relaxed);
    Stats.TokensTotal.store(0, std::memory_order_relaxed);
    Stats.PromptTokensEvaluated.store(0, std::memory_order_relaxed);
    Stats.PromptTokensTotal.store(0, std::memory_order_relaxed);
    ComputeFuture = {};
  }
};

#else
/// Stub when GGML backend is not compiled.
struct AsyncContext {};
#endif
} // namespace WasmEdge::Host::WASINN::GGML
