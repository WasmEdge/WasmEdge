// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "func-bpf-map-fd-by-name.h"
#include "util.h"

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
  auto program_ref = state->find(program);
  if (!program_ref) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return program_ref->bpf_map_fd_by_name(name_str);
}

} // namespace Host
} // namespace WasmEdge
