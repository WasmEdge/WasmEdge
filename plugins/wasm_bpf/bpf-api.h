// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "executor/executor.h"
#include "runtime/instance/module.h"
#include "wasmedge/wasmedge.h"

extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

#define POLL_TIMEOUT_MS 100
#define PERF_BUFFER_PAGES 64
#define DEBUG_LIBBPF_RUNTIME 0
#define DEBUG_PRINT_BUFFER_SIZE 1024

/// @brief init libbpf callbacks
void init_libbpf(void);

typedef int (*bpf_buffer_sample_fn)(void *ctx, void *data, size_t size);

/// An absraction of a bpf ring buffer or perf buffer
/// see https://github.com/iovisor/bcc/blob/master/libbpf-tools/compat.c
class bpf_buffer {
protected:
  bpf_buffer_sample_fn fn;
  WasmEdge_ExecutorContext *wasm_executor;
  const WasmEdge_ModuleInstanceContext *wasm_module_instance;
  uint32_t wasm_ctx;
  uint32_t wasm_sample_function;
  void *poll_data;
  size_t max_poll_size;
  uint32_t wasm_buf_ptr;

public:
  /// sample callback which calls the wasm handler indirectly
  int bpf_buffer_sample(void *data, size_t size);
  /// set the wasm callback parameters
  void
  set_callback_params(WasmEdge_ExecutorContext *executor,
                      const WasmEdge_ModuleInstanceContext *module_instance,
                      uint32_t sample_func, void *data, size_t max_size,
                      uint32_t ctx, uint32_t buf_ptr);
  /// polling the bpf buffer
  virtual int bpf_buffer__poll(int timeout_ms) = 0;
  /// open the bpf buffer map
  virtual int bpf_buffer__open(int fd, bpf_buffer_sample_fn sample_cb,
                               void *ctx) = 0;
  virtual ~bpf_buffer() = default;
};

/// bpf program instance
class wasm_bpf_program {
  std::unique_ptr<bpf_object, void (*)(bpf_object *obj)> obj{nullptr,
                                                             bpf_object__close};
  std::unique_ptr<bpf_buffer> buffer;
  std::unordered_set<std::unique_ptr<bpf_link, int (*)(bpf_link *obj)>> links;

public:
  int bpf_map_fd_by_name(const char *name);
  int load_bpf_object(const void *obj_buf, size_t obj_buf_sz);
  int attach_bpf_program(const char *name, const char *attach_target);
  int bpf_buffer_poll(WasmEdge_ExecutorContext *executor,
                      const WasmEdge_ModuleInstanceContext *module_instance,
                      int fd, int32_t sample_func, uint32_t ctx,
                      void *buffer_data, size_t max_size, int timeout_ms,
                      uint32_t wasm_buf_ptr);
  bpf_map *map_ptr_by_fd(int fd);
};

enum bpf_map_cmd {
  _BPF_MAP_LOOKUP_ELEM = 1,
  _BPF_MAP_UPDATE_ELEM,
  _BPF_MAP_DELETE_ELEM,
  _BPF_MAP_GET_NEXT_KEY,
};

/// Operate on a bpf map.
int bpf_map_operate(int fd, int cmd, void *key, void *value, void *next_key,
                    uint64_t flags);
using handle_t = int64_t;
