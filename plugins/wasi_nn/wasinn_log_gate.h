// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// Several inference libraries (llama.cpp, whisper.cpp) expose a single
// process-global log callback that can only be toggled process-wide, so logging
// cannot be gated per graph at the callback. Instead the callback is enabled
// whenever at least one currently-loaded graph requested logging: each such
// graph holds a LogToken that owns one unit of the subsystem's Counter while it
// wants logs. Loading or reconfiguring one graph therefore never toggles
// another graph's logging. A graph owns its unit in place: non-copyable and
// (via the user destructor) non-movable.
template <std::atomic<uint32_t> &Counter> class LogToken {
public:
  LogToken() noexcept = default;
  LogToken(const LogToken &) = delete;
  LogToken &operator=(const LogToken &) = delete;
  ~LogToken() noexcept { set(false); }

  // Reconcile this graph's single contribution to match its current EnableLog;
  // idempotent across load() and every set_input() reload.
  void set(bool Enable) noexcept {
    if (Enable == Counted) {
      return;
    }
    if (Enable) {
      Counter.fetch_add(1, std::memory_order_relaxed);
    } else {
      Counter.fetch_sub(1, std::memory_order_relaxed);
    }
    Counted = Enable;
  }

private:
  bool Counted = false;
};

template <std::atomic<uint32_t> &Counter>
inline bool logGateEnabled() noexcept {
  return Counter.load(std::memory_order_relaxed) != 0;
}

} // namespace WASINN
} // namespace Host
} // namespace WasmEdge
