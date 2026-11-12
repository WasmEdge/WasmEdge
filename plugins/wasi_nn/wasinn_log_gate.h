// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include <atomic>
#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WASINN {

// llama.cpp and whisper.cpp expose only process-global log callbacks, so
// logging cannot be gated per graph at the callback. Instead it stays enabled
// while at least one loaded graph wants logs: each such graph holds a LogToken
// owning one unit of the subsystem's Counter.
template <std::atomic<uint32_t> &Counter> class LogToken {
public:
  LogToken() noexcept = default;
  LogToken(const LogToken &) = delete;
  LogToken &operator=(const LogToken &) = delete;
  ~LogToken() noexcept { set(false); }

  // Idempotent: repeated calls keep this graph's contribution at one unit.
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
