// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-bpf-map-fd-by-name.h"
#include "util.h"
#include <shared_mutex>

namespace WasmEdge {
namespace Host {

Expect<int32_t> BpfMapFdByName::body(const Runtime::CallingFrame &Frame,
                                     handle_t program, uint32_t name) {
  const char *name_str;
  auto memory = Frame.getMemoryByIndex(0);
  if (memory == nullptr) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  {
    auto v = read_c_str(memory, name);
    if (v.has_value()) {
      name_str = v.value();
    } else {
      return Unexpect(v.error());
    }
  }
  std::shared_lock guard(this->state->lock);
  auto program_ptr = state->handles.find(program);
  if (program_ptr == state->handles.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return program_ptr->second->bpf_map_fd_by_name(name_str);
}

} // namespace Host
} // namespace WasmEdge
