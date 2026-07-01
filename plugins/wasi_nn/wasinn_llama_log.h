// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinn_log_gate.h"

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// llama.cpp exposes one process-global log callback that every llama-based
// backend (GGML, BitNet) registers through llama_log_set, so logging is gated
// on this shared ref-counted counter rather than per callback: each graph that
// wants logs holds a LlamaLogToken owning one unit, so loading or reconfiguring
// one graph never toggles another graph's logging.
inline std::atomic<uint32_t> LlamaLogEnabledCount{0};

using LlamaLogToken = LogToken<LlamaLogEnabledCount>;

inline bool llamaLogEnabled() noexcept {
  return logGateEnabled<LlamaLogEnabledCount>();
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
