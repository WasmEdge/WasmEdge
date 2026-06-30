// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once
#include "GGML/core/ggml_core.h"

namespace WasmEdge::Host::WASINN::GGML {
#ifdef WASMEDGE_PLUGIN_WASI_NN_BACKEND_GGML
void clearContext(Graph &GraphRef, Context &CxtRef) noexcept;
Expect<ErrNo> getEmbedding(Graph &GraphRef, Context &CxtRef) noexcept;
ErrNo evaluateInput(Graph &GraphRef, Context &CxtRef,
                    std::string_view LogPrefix) noexcept;
ErrNo sampleOutput(Graph &GraphRef, Context &CxtRef,
                   bool IsSingleTokenMode = false) noexcept;
#endif
} // namespace WasmEdge::Host::WASINN::GGML
