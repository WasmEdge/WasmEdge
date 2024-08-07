// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func-bpf-map-operate.h"
#include "bpf-api.h"

extern "C" {
#include <bpf/libbpf.h>
}

namespace WasmEdge {
namespace Host {

#define ensure_memory_size(var, offset, expected_size)                         \
  const auto var##_span = memory->getSpan<char>(offset, expected_size);        \
  if (var##_span.size() != expected_size)                                      \
    return Unexpect(ErrCode::Value::HostFuncError);                            \
  const auto var = var##_span.data();

Expect<int32_t>
BpfMapOperate::body(const WasmEdge::Runtime::CallingFrame &Frame, int32_t fd,
                    int32_t cmd, uint32_t key, uint32_t value,
                    uint32_t next_key, uint64_t flags) {

  auto *memory = Frame.getMemoryByIndex(0);
  if (unlikely(!memory)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::shared_lock guard(this->state->lock);
  bpf_map_info map_info;
  memset(&map_info, 0, sizeof(map_info));
  uint32_t info_len = sizeof(map_info);
  int32_t err;
  if ((err = bpf_map_get_info_by_fd(fd, &map_info, &info_len)) != 0) {
    spdlog::debug("[WasmEdge Wasm_bpf] Invalid map fd found: fd={},err={}", fd,
                  err);
    // Invalid map fd
    return err;
  }
  auto key_size = map_info.key_size;
  auto value_size = map_info.value_size;

  switch ((bpf_cmd)cmd) {
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
    spdlog::debug("[WasmEdge Wasm_bpf] Invalid map operation", cmd);
    return -EINVAL;
  }
}

} // namespace Host
} // namespace WasmEdge
