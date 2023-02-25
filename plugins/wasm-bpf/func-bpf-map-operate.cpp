// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-bpf-map-operate.h"
#include "bpf-api.h"
#include <mutex>
#include <shared_mutex>
extern "C" {
#include <bpf/libbpf.h>
}
using namespace WasmEdge;

Expect<int32_t>
BpfMapOperate::body(const WasmEdge::Runtime::CallingFrame &Frame, int32_t fd,
                    int32_t cmd, uint32_t key, uint32_t value,
                    uint32_t next_key, uint64_t flags) {

  auto memory = Frame.getMemoryByIndex(0);
  if (memory == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::shared_lock guard(this->state->lock);
  if (!state->map_fd_cache.count(fd)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  bpf_map *map = state->map_fd_cache[fd];
  auto key_size = bpf_map__key_size(map);
  auto value_size = bpf_map__value_size(map);
#define ensure_memory_size(var, offset, size)                                  \
  void *var = memory->getPointer<char *>(offset, size);                        \
  if (var == nullptr)                                                          \
    return Unexpect(ErrCode::Value::HostFuncError);
  switch ((bpf_map_cmd)cmd) {
  case BPF_MAP_GET_NEXT_KEY: {
    ensure_memory_size(key_ptr, key, key_size);
    ensure_memory_size(next_key_ptr, next_key, key_size);
    return bpf_map_get_next_key(fd, key_ptr, next_key_ptr);
  }
  case BPF_MAP_LOOKUP_ELEM: {
    ensure_memory_size(key_ptr, key, key_size);
    ensure_memory_size(value_ptr, value, value_size);
    return bpf_map_lookup_elem_flags(fd, key_ptr, value_ptr, flags);
  }
  case BPF_MAP_UPDATE_ELEM: {
    ensure_memory_size(key_ptr, key, key_size);
    ensure_memory_size(value_ptr, value, value_size);
    return bpf_map_update_elem(fd, key_ptr, value_ptr, flags);
  }
  case BPF_MAP_DELETE_ELEM: {
    ensure_memory_size(key_ptr, key, key_size);
    return bpf_map_delete_elem_flags(fd, key_ptr, flags);
  }
  default: // More syscall commands can be allowed here
    return -EINVAL;
  }
#undef ensure_memory_size
}
