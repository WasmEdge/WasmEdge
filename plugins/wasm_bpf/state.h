// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "bpf-api.h"
#include <cinttypes>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace WasmEdge {
namespace Host {

struct WasmBpfState {
  /// manage bpf programs
  std::unordered_map<handle_t, std::unique_ptr<wasm_bpf_program>> handles;
  std::shared_mutex lock;
  ~WasmBpfState() noexcept = default;
};

using state_t = std::shared_ptr<WasmBpfState>;

} // namespace Host
} // namespace WasmEdge
