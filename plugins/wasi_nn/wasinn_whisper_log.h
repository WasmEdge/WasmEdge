// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinn_log_gate.h"

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// whisper.cpp exposes one process-global log callback (whisper_log_set) shared
// by every whisper graph. Its enable gate is ref-counted exactly like the llama
// gate, but over a separate counter so a whisper graph requesting logs never
// enables llama logging and vice versa.
inline std::atomic<uint32_t> WhisperLogEnabledCount{0};

using WhisperLogToken = LogToken<WhisperLogEnabledCount>;

inline bool whisperLogEnabled() noexcept {
  return logGateEnabled<WhisperLogEnabledCount>();
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
