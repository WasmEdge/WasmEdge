/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2023, eunomia-bpf
 * All rights reserved.
 */
// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2022 Hengqi Chen */

#ifndef __BPF_WASM_API_H
#define __BPF_WASM_API_H

#include "executor/executor.h"
#include "runtime/instance/module.h"
#include "wasmedge/wasmedge.h"
#include <cinttypes>
#include <cstdlib>
#include <memory>
#include <unordered_set>
#include <vector>
extern "C" {
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

#define POLL_TIMEOUT_MS 100
#define DEBUG_LIBBPF_RUNTIME 0

#define PERF_BUFFER_PAGES 64

typedef int (*bpf_buffer_sample_fn)(void *ctx, void *data, size_t size);
/// An absraction of a bpf ring buffer or perf buffer from bcc.
/// https://github.com/iovisor/bcc/blob/master/libbpf-tools/compat.c
struct bpf_buffer {
  struct bpf_map *events;
  void *inner;
  bpf_buffer_sample_fn fn;
  WasmEdge_ExecutorContext *executor;
  const WasmEdge_ModuleInstanceContext *module_instance;
  uint32_t ctx;
  uint32_t wasm_sample_function;
  int type;
  uint32_t wasm_buf_ptr;
};

struct bpf_buffer *bpf_buffer__new(struct bpf_map *events);

int bpf_buffer__open(struct bpf_buffer *buffer, bpf_buffer_sample_fn sample_cb,
                     void *ctx);
int bpf_buffer__poll(struct bpf_buffer *buffer, int timeout_ms);

void bpf_buffer__free(struct bpf_buffer *buffer);

/// @brief init libbpf callbacks
void init_libbpf(void);
struct wasm_bpf_program {
  std::unique_ptr<bpf_object, void (*)(bpf_object *obj)> obj{nullptr,
                                                             bpf_object__close};
  std::unique_ptr<bpf_buffer, void (*)(bpf_buffer *obj)> buffer{
      nullptr, bpf_buffer__free};
  std::unordered_set<std::unique_ptr<bpf_link, int (*)(bpf_link *obj)>> links;
  void *poll_data;
  size_t max_poll_size;
  int bpf_map_fd_by_name(const char *name);
  int load_bpf_object(const void *obj_buf, size_t obj_buf_sz);
  int attach_bpf_program(const char *name, const char *attach_target);
  int bpf_buffer_poll(WasmEdge_ExecutorContext *executor,
                      const WasmEdge_ModuleInstanceContext *module_instance,
                      int fd, int32_t sample_func, uint32_t ctx, void *data,
                      size_t max_size, int timeout_ms, uint32_t wasm_buf_ptr);
};

enum bpf_map_cmd {
  _BPF_MAP_LOOKUP_ELEM = 1,
  _BPF_MAP_UPDATE_ELEM,
  _BPF_MAP_DELETE_ELEM,
  _BPF_MAP_GET_NEXT_KEY,
};

using handle_t = int64_t;

#endif
