// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-load-bpf-object.h"

namespace WasmEdge {
namespace Host {

Expect<handle_t> LoadBpfObject::body(const Runtime::CallingFrame &Frame,
                                     uint32_t obj_buf, uint32_t obj_buf_sz) {
  auto *memory = Frame.getMemoryByIndex(0);
  if (unlikely(!memory)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  char *const object_buffer = memory->getPointer<char *>(obj_buf, obj_buf_sz);
  if (!object_buffer) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto program = std::make_unique<wasm_bpf_program>();
  int32_t res =
      program->load_bpf_object(object_buffer, static_cast<size_t>(obj_buf_sz));
  if (res < 0)
    return 0;
  auto key = reinterpret_cast<uint64_t>(program.get());

  std::shared_lock guard(state->lock);
  state->handles.emplace(key, std::move(program));
  return key;
}

} // namespace Host
} // namespace WasmEdge
