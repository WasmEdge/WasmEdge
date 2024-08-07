// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func-bpf-map-fd-by-name.h"
#include "util.h"
#include <shared_mutex>

namespace WasmEdge {
namespace Host {

Expect<int32_t> BpfMapFdByName::body(const Runtime::CallingFrame &Frame,
                                     handle_t program, uint32_t name) {
  const char *name_str = nullptr;
  auto *memory = Frame.getMemoryByIndex(0);
  if (unlikely(!memory)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  checkAndSetCstr(memory, name, name_str);
  std::shared_lock guard(this->state->lock);
  auto program_ptr = state->handles.find(program);
  if (program_ptr == state->handles.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return program_ptr->second->bpf_map_fd_by_name(name_str);
}

} // namespace Host
} // namespace WasmEdge
