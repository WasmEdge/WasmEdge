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

/// \brief Initialize libbpf callbacks.
void init_libbpf(void);

typedef int32_t (*bpf_buffer_sample_fn)(void *ctx, void *data, size_t size);

/// An abstraction of a BPF ring buffer or perf buffer.
/// See https://github.com/iovisor/bcc/blob/master/libbpf-tools/compat.c.
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
  /// Sample callback that calls the Wasm handler indirectly.
  int32_t bpf_buffer_sample(void *data, size_t size);
  /// Check whether the BPF buffer is valid.
  ///
  /// A valid module instance should have only one table and a sample function.
  bool is_valid() const;
  /// Set the Wasm callback parameters.
  void
  set_callback_params(WasmEdge_ExecutorContext *executor,
                      const WasmEdge_ModuleInstanceContext *module_instance,
                      uint32_t sample_func, void *data, size_t max_size,
                      uint32_t ctx, uint32_t buf_ptr);
  /// Poll the BPF buffer.
  virtual int32_t bpf_buffer__poll(int32_t timeout_ms) = 0;
  /// Open the BPF buffer map.
  virtual int32_t bpf_buffer__open(int32_t fd, bpf_buffer_sample_fn sample_cb,
                                   void *ctx) = 0;
  virtual ~bpf_buffer() noexcept = default;
};

/// BPF program instance.
class wasm_bpf_program {
  std::unique_ptr<bpf_object, void (*)(bpf_object *obj)> obj{nullptr,
                                                             bpf_object__close};
  std::unique_ptr<bpf_buffer> buffer;
  std::unordered_set<std::unique_ptr<bpf_link, int32_t (*)(bpf_link *obj)>>
      links;

public:
  /// Find a BPF map fd by name.
  int32_t bpf_map_fd_by_name(const char *name);
  /// Load a BPF object from a buffer into the kernel.
  int32_t load_bpf_object(const void *obj_buf, size_t obj_buf_sz);
  /// Attach a BPF program to a target (e.g. a kernel function on a kprobe).
  int32_t attach_bpf_program(const char *name, const char *attach_target);
  /// Poll the BPF buffer to get data from the kernel.
  int32_t bpf_buffer_poll(WasmEdge_ExecutorContext *executor,
                          const WasmEdge_ModuleInstanceContext *module_instance,
                          int32_t fd, int32_t sample_func, uint32_t ctx,
                          void *buffer_data, size_t max_size,
                          int32_t timeout_ms, uint32_t wasm_buf_ptr);
  /// Get the BPF map pointer by fd.
  bpf_map *map_ptr_by_fd(int32_t fd);
};

enum bpf_map_cmd {
  _BPF_MAP_LOOKUP_ELEM = 1,
  _BPF_MAP_UPDATE_ELEM,
  _BPF_MAP_DELETE_ELEM,
  _BPF_MAP_GET_NEXT_KEY,
};

/// Operate on a BPF map.
int32_t bpf_map_operate(int32_t fd, int32_t cmd, void *key, void *value,
                        void *next_key, uint64_t flags);
using handle_t = int64_t;

} // namespace Host
} // namespace WasmEdge
