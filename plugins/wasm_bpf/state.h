// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _WASM_BPF_STATE
#define _WASM_BPF_STATE

#include "bpf-api.h"
#include <cinttypes>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
struct WasmBpfState {
  std::unordered_map<handle_t, std::unique_ptr<wasm_bpf_program>> handles;
  std::shared_mutex lock;
};

using state_t = std::shared_ptr<WasmBpfState>;

#endif