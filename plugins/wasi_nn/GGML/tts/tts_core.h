// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "GGML/core/ggml_core.h"

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
std::vector<llama_token> processTTSPrompt(WasiNNEnvironment &Env,
                                          Graph &GraphRef,
                                          std::string &Prompt) noexcept;
ErrNo codesToSpeech(WasiNNEnvironment &Env, Graph &GraphRef,
                    Context &CxtRef) noexcept;
#endif
} // namespace WasmEdge::Host::WASINN::GGML
