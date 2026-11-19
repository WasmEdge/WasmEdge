// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "bpf-api.h"
#include <cinttypes>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace WasmEdge {
namespace Host {

struct WasmBpfState {
  /// Manage BPF programs.
  std::unordered_map<handle_t, std::shared_ptr<wasm_bpf_program>> handles;
  std::shared_mutex lock;
  /// Get a program by handle. Returns nullptr if the handle is unknown.
  std::shared_ptr<wasm_bpf_program> find(handle_t handle) {
    std::shared_lock guard(lock);
    auto iter = handles.find(handle);
    if (iter == handles.end()) {
      return nullptr;
    }
    return iter->second;
  }
  ~WasmBpfState() noexcept = default;
};

using state_t = std::shared_ptr<WasmBpfState>;

} // namespace Host
} // namespace WasmEdge
