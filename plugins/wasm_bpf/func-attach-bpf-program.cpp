// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "func-attach-bpf-program.h"
#include "util.h"

namespace WasmEdge {
namespace Host {

Expect<int32_t> AttachBpfProgram::body(const Runtime::CallingFrame &Frame,
                                       handle_t program, uint32_t name,
                                       uint32_t attach_target) {
  auto *memory = Frame.getMemoryByIndex(0);
  if (unlikely(!memory)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto program_ref = state->find(program);
  if (!program_ref) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const char *name_str = nullptr;
  const char *attach_target_str = nullptr;
  checkAndSetCstr(memory, name, name_str);
  checkAndSetCstr(memory, attach_target, attach_target_str);
  return program_ref->attach_bpf_program(name_str, attach_target_str);
}

} // namespace Host
} // namespace WasmEdge
