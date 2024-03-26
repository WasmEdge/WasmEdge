// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "whisper.h"
#include "wasinnenv.h"

#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER
#include "simdjson.h"
#include <algorithm>
#include <common.h>
#include <cstdlib>
#include <sstream>
#endif

namespace WasmEdge::Host::WASINN::WHISPER {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_WHISPER

Expect<ErrNo> load(WasiNNEnvironment &Env, Span<const Span<uint8_t>> Builders,
                   [[maybe_unused]] Device Device, uint32_t &GraphId) noexcept {
  Env.NNGraph.emplace_back(Backend::WHISPER);
  auto &GraphRef = Env.NNGraph.back().get<Graph>();
  struct whisper_context_params ContextDefault =
      whisper_context_default_params(); // from whisper.cpp
  GraphRef.EnableLog = false;
  GraphRef.EnableDebugLog = false;
  GraphRef.StreamStdout = false;

  auto Weight = Builders[0];
  const std::string BinModel(reinterpret_cast<char *>(Weight.data()),
                             Weight.size());

  // Handle the model path.
  std::string ModelFilePath;
  if (BinModel.substr(0, 8) == "preload:") {
    ModelFilePath = BinModel.substr(8);
  } else {
    // Write whisper model to file.
    ModelFilePath = "models/ggml-base.en.bin"sv;
    std::ofstream TempFile(ModelFilePath);
    if (!TempFile) {
      spdlog::error(
          "[WASI-NN] Whisper backend: Failed to create the temporary file.");
      Env.NNGraph.pop_back();
      return ErrNo::InvalidArgument;
    }
    TempFile << BinModel;
    TempFile.close();
  }

  // auto==struct whisper_context definition available in whisper.cpp which is
  // not included
  auto wctx = whisper_init_from_file_with_params(GraphRef.ModelFilePath.c_str(),
                                                 ContextDefault);
  GraphRef.WhisperModel = wctx.model;

  if (GraphRef.WhisperModel == nullptr) {
    spdlog::error("[WASI-NN] Whisper backend: Error: unable to init model."sv);
    Env.NNGraph.pop_back();
    return ErrNo::InvalidArgument;
  }
  if (GraphRef.EnableDebugLog) {
    spdlog::info(
        "[WASI-NN][Debug] Whisper backend: Initialize whisper model with given parameters...Done"sv);
  }
  // Store the loaded graph.
  GraphId = Env.NNGraph.size() - 1;
  return ErrNo::Success;
}

#endif
} // namespace WasmEdge::Host::WASINN::WHISPER