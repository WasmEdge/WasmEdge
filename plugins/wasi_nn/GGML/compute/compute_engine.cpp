// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "GGML/core/ggml_core.h"
#include "GGML/tts/tts_core.h"
#include "inference_manager.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
#include <mtmd-helper.h>
#include <mtmd.h>
#endif

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML

Expect<ErrNo> compute(WasiNNEnvironment &Env, uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute")

  // Reject if a previous async compute is still running.
  auto Phase = CxtRef.Async.poll();
  if (Phase == AsyncContext::Phase::PromptEval ||
      Phase == AsyncContext::Phase::Generating) {
    RET_ERROR(ErrNo::Busy,
              "compute: an async computation is already in progress."sv)
  }

  // Reset the async state for this new inference run.
  CxtRef.Async.reset();

  // Clear the context and reset the sampler.
  clearContext(GraphRef, CxtRef);

  if (GraphRef.Params.embedding) {
    return getEmbedding(GraphRef, CxtRef);
  }

  // Publish the prediction target so pollers can calculate progress.
  int64_t NPredict =
      CxtRef.Conf.NPredict < 0 ? INT32_MAX : CxtRef.Conf.NPredict;
  CxtRef.Async.Stats.TokensTotal.store(static_cast<uint64_t>(NPredict),
                                       std::memory_order_relaxed);

  // Mark prompt evaluation phase before launching the worker.
  CxtRef.Async.CurrentPhase.store(AsyncContext::Phase::PromptEval,
                                  std::memory_order_release);

  // Package the entire inference pipeline into a task for the worker thread.
  // GraphRef and CxtRef are captured by reference: their lifetimes are
  // managed by WasiNNEnvironment, which outlives any individual compute.
  auto InferenceTask = [&Env, &GraphRef, &CxtRef, NPredict]() mutable -> ErrNo {
    // --- Prompt evaluation ---
    ErrNo ReturnCode = ErrNo::Success;
    if (GraphRef.VisionContext == nullptr) {
      // Text-only prompt.
      ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
      if (ReturnCode != ErrNo::Success) {
        return ReturnCode;
      }
    } else {
      // Multimodal prompt.
      llama_pos NewNPos;
      int32_t Res = mtmd_helper_eval_chunks(
          GraphRef.VisionContext.get(), GraphRef.LlamaContext.get(),
          GraphRef.VisionInputChunks.get(), CxtRef.NPos,
          /* seq_id */ 0, static_cast<int32_t>(CxtRef.CurrentBatchSize),
          /* logits_last */ true, &NewNPos);
      CxtRef.NPos = NewNPos;
      if (Res != 0) {
        spdlog::error(
            "[WASI-NN] GGML backend: compute: unable to eval the mtmd "
            "prompt."sv);
        return ErrNo::InvalidArgument;
      }
    }

    // Transition to generation phase.
    CxtRef.Async.CurrentPhase.store(AsyncContext::Phase::Generating,
                                    std::memory_order_release);

    // --- Autoregressive token generation ---
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: enter main prediction loop"sv)
    while (NPredict-- > 0) {
      ReturnCode = sampleOutput(GraphRef, CxtRef);
      if (ReturnCode != ErrNo::Success) {
        break;
      }
    }
    if (ReturnCode == ErrNo::EndOfSequence) {
      ReturnCode = ErrNo::Success;
    }
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: enter main prediction loop...Done"sv)

    // --- Post-processing (runs on the worker thread) ---

    // TTS: convert output codes to audio file.
    if (GraphRef.TextToSpeech && ReturnCode == ErrNo::Success) {
      LOG_DEBUG(GraphRef.EnableDebugLog,
                "compute: convert output codes to audio file."sv)
      ReturnCode = codesToSpeech(Env, GraphRef, CxtRef);
      if (ReturnCode != ErrNo::Success) {
        spdlog::error(
            "[WASI-NN] GGML backend: compute: failed to convert output "
            "codes to audio file."sv);
      }
      LOG_DEBUG(GraphRef.EnableDebugLog,
                "compute: convert output codes to audio file...Done"sv)
    }

    if (GraphRef.EnableLog) {
      common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler);
    }

    LOG_DEBUG(GraphRef.EnableDebugLog, "compute...Done"sv)
    return ReturnCode;
  };

  // Launch on a detached worker thread. AsyncContext::launch() handles
  // Phase transitions to Complete/Error and fulfills the shared_future.
  CxtRef.Async.launch(std::move(InferenceTask));

  // Return immediately—the WASM guest can poll via Async.poll() or block
  // via Async.waitFor() to retrieve the result.
  return ErrNo::Success;
}

Expect<ErrNo> computeSingle(WasiNNEnvironment &Env,
                            uint32_t ContextId) noexcept {
  auto &CxtRef = Env.NNContext[ContextId].get<Context>();
  auto &GraphRef = Env.NNGraph[CxtRef.GraphId].get<Graph>();
  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle"sv)

  // New compute single token context.
  auto ReturnCode = ErrNo::Success;
  if (!CxtRef.ComputeSingleStarted) {
    CxtRef.ComputeSingleStarted = true;

    // Clear the context and reset the sampler.
    clearContext(GraphRef, CxtRef);

    // Evaluate the input tokens.
    if (GraphRef.VisionContext == nullptr) {
      // Text only prompt.
      ReturnCode = evaluateInput(GraphRef, CxtRef, "compute"sv);
      if (ReturnCode != ErrNo::Success) {
        return ReturnCode;
      }
    } else {
      // Multimodal prompt.
      llama_pos NewNPos;
      int32_t Res = mtmd_helper_eval_chunks(
          GraphRef.VisionContext.get(), GraphRef.LlamaContext.get(),
          GraphRef.VisionInputChunks.get(), CxtRef.NPos,
          /* seq_id */ 0, static_cast<int32_t>(CxtRef.CurrentBatchSize),
          /* logits_last */ true, &NewNPos);
      CxtRef.NPos = NewNPos;
      if (Res != 0) {
        RET_ERROR(ErrNo::InvalidArgument,
                  "compute: unable to eval the mtmd prompt."sv)
      }
    }
  }

  // Main prediction process.
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "computeSingle: enter main prediction process"sv)
  ReturnCode = sampleOutput(GraphRef, CxtRef, true);
  if (ReturnCode != ErrNo::Success) {
    CxtRef.ComputeSingleStarted = false;
  }
  LOG_DEBUG(GraphRef.EnableDebugLog,
            "computeSingle: enter main prediction process...Done"sv)
  // End of main predict process.

  LOG_DEBUG(GraphRef.EnableDebugLog, "computeSingle...Done"sv)
  return ReturnCode;
}

#endif
} // namespace WasmEdge::Host::WASINN::GGML
