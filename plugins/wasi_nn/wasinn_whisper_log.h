// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "wasinn_log_gate.h"

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// Gate for whisper.cpp's process-global log callback (whisper_log_set), on a
// separate counter so whisper and llama logging toggle independently.
inline std::atomic<uint32_t> WhisperLogEnabledCount{0};

using WhisperLogToken = LogToken<WhisperLogEnabledCount>;

inline bool whisperLogEnabled() noexcept {
  return logGateEnabled<WhisperLogEnabledCount>();
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
