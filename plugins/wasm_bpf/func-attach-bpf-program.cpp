// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-attach-bpf-program.h"
#include "util.h"

namespace WasmEdge {
namespace Host {

Expect<int32_t> AttachBpfProgram::body(const Runtime::CallingFrame &Frame,
                                       handle_t program, uint32_t name,
                                       uint32_t attach_target) {
  std::shared_lock lock(state->lock);
  auto program_ptr = state->handles.find(program);
  if (program_ptr == state->handles.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto *memory = Frame.getMemoryByIndex(0);
  if (!memory) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *name_str;
  const char *attach_target_str;
  {
    auto v1 = read_c_str(memory, name);
    if (v1.has_value()) {
      name_str = v1.value();
    } else {
      return Unexpect(v1.error());
    }
  }
  {
    auto v1 = read_c_str(memory, attach_target);
    if (v1.has_value()) {
      attach_target_str = v1.value();
    } else {
      return Unexpect(v1.error());
    }
  }
  return program_ptr->second->attach_bpf_program(name_str, attach_target_str);
}

} // namespace Host
} // namespace WasmEdge
