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

  // Clear the context and reset the sampler.
  clearContext(GraphRef, CxtRef);

  if (GraphRef.Params.embedding) {
    return getEmbedding(GraphRef, CxtRef);
  }

  // Evaluate the input tokens.
  ErrNo ReturnCode = ErrNo::Success;
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

  // Main prediction loop.
  LOG_DEBUG(GraphRef.EnableDebugLog, "compute: enter main prediction loop"sv)
  int64_t NPredict =
      CxtRef.Conf.NPredict < 0 ? INT32_MAX : CxtRef.Conf.NPredict;

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
  // End of main prediction loop.

  // TTS: convert output codes to audio file.
  if (GraphRef.TextToSpeech) {
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: convert output codes to audio file."sv)
    ReturnCode = codesToSpeech(Env, GraphRef, CxtRef);
    if (ReturnCode != ErrNo::Success) {
      RET_ERROR(ReturnCode,
                "compute: failed to convert output codes to audio "sv
                "file."sv)
    }
    LOG_DEBUG(GraphRef.EnableDebugLog,
              "compute: convert output codes to audio file...Done"sv)
  }

  if (GraphRef.EnableLog) {
    common_perf_print(GraphRef.LlamaContext.get(), CxtRef.LlamaSampler);
  }

  LOG_DEBUG(GraphRef.EnableDebugLog, "compute...Done"sv)
  return ReturnCode;
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
