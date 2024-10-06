// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "executor/executor.h"
#include "runtime/instance/module.h"
#include "wasmedge/wasmedge.h"

#pragma GCC diagnostic push
#ifdef __clang__
// Allow compilation using clang
#pragma GCC diagnostic warning "-Wextern-c-compat"

#endif
extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

#pragma GCC diagnostic pop

#define POLL_TIMEOUT_MS 100
#define PERF_BUFFER_PAGES 64
#define DEBUG_LIBBPF_RUNTIME 0
#define DEBUG_PRINT_BUFFER_SIZE 1024

namespace WasmEdge {
namespace Host {

/// \brief init libbpf callbacks
void init_libbpf(void);

typedef int32_t (*bpf_buffer_sample_fn)(void *ctx, void *data, size_t size);

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
  int32_t bpf_buffer_sample(void *data, size_t size);
  /// Check if the bpf buffer is valid
  ///
  /// a valid module instance should have only one table and a sample function
  bool is_valid() const;
  /// set the wasm callback parameters
  void
  set_callback_params(WasmEdge_ExecutorContext *executor,
                      const WasmEdge_ModuleInstanceContext *module_instance,
                      uint32_t sample_func, void *data, size_t max_size,
                      uint32_t ctx, uint32_t buf_ptr);
  /// polling the bpf buffer
  virtual int32_t bpf_buffer__poll(int32_t timeout_ms) = 0;
  /// open the bpf buffer map
  virtual int32_t bpf_buffer__open(int32_t fd, bpf_buffer_sample_fn sample_cb,
                                   void *ctx) = 0;
  virtual ~bpf_buffer() noexcept = default;
};

/// bpf program instance
class wasm_bpf_program {
  std::unique_ptr<bpf_object, void (*)(bpf_object *obj)> obj{nullptr,
                                                             bpf_object__close};
  std::unique_ptr<bpf_buffer> buffer;
  std::unordered_set<std::unique_ptr<bpf_link, int32_t (*)(bpf_link *obj)>>
      links;

public:
  /// Find a bpf map fd by name
  int32_t bpf_map_fd_by_name(const char *name);
  /// Load a bpf object from a buffer into the kernel
  int32_t load_bpf_object(const void *obj_buf, size_t obj_buf_sz);
  /// Attach a bpf program to a target (e.g. a kernel function on a kprobe)
  int32_t attach_bpf_program(const char *name, const char *attach_target);
  /// Poll the bpf buffer to get data from the kernel
  int32_t bpf_buffer_poll(WasmEdge_ExecutorContext *executor,
                          const WasmEdge_ModuleInstanceContext *module_instance,
                          int32_t fd, int32_t sample_func, uint32_t ctx,
                          void *buffer_data, size_t max_size,
                          int32_t timeout_ms, uint32_t wasm_buf_ptr);
  /// Get the bpf map pointer by fd
  bpf_map *map_ptr_by_fd(int32_t fd);
};

enum bpf_map_cmd {
  _BPF_MAP_LOOKUP_ELEM = 1,
  _BPF_MAP_UPDATE_ELEM,
  _BPF_MAP_DELETE_ELEM,
  _BPF_MAP_GET_NEXT_KEY,
};

/// Operate on a bpf map.
int32_t bpf_map_operate(int32_t fd, int32_t cmd, void *key, void *value,
                        void *next_key, uint64_t flags);
using handle_t = int64_t;

} // namespace Host
} // namespace WasmEdge
