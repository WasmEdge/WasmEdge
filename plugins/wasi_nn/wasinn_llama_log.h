// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinn_log_gate.h"

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// Gate for llama.cpp's process-global log callback (llama_log_set), shared by
// every llama-based backend (GGML, BitNet).
inline std::atomic<uint32_t> LlamaLogEnabledCount{0};

using LlamaLogToken = LogToken<LlamaLogEnabledCount>;

inline bool llamaLogEnabled() noexcept {
  return logGateEnabled<LlamaLogEnabledCount>();
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
