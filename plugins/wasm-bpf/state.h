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
  std::unordered_map<handle_t, wasm_bpf_program *> handles;
  handle_t next_handle = 1;
  std::shared_mutex lock;
  virtual ~WasmBpfState() {
    for (auto p = handles.begin(); p != handles.end(); p++) {
      delete p->second;
    }
  }
  std::unordered_map<int, bpf_map *> map_fd_cache;
};

using state_t = std::shared_ptr<WasmBpfState>;

#endif