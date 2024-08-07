// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func-bpf-buffer-poll.h"
#include "wasmedge/wasmedge.h"
#include <shared_mutex>

namespace WasmEdge {
namespace Host {

// Helper functions of context conversions.
inline const auto *toCallFrameCxt(const Runtime::CallingFrame *Cxt) noexcept {
  return reinterpret_cast<const WasmEdge_CallingFrameContext *>(Cxt);
}

Expect<int32_t> BpfBufferPoll::body(const Runtime::CallingFrame &Frame,
                                    handle_t program, int32_t fd,
                                    int32_t sample_func, uint32_t ctx,
                                    uint32_t data, uint32_t max_size,
                                    int32_t timeout_ms) {
  auto c_ctx = toCallFrameCxt(&Frame);
  auto c_module = WasmEdge_CallingFrameGetModuleInstance(c_ctx);
  auto c_executor = WasmEdge_CallingFrameGetExecutor(c_ctx);
  if (unlikely(!c_ctx || !c_module || !c_executor)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto *memory = Frame.getMemoryByIndex(0);
  if (unlikely(!memory)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto module_instance = Frame.getModule();
  if (unlikely(!module_instance)) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::shared_lock lock(state->lock);
  auto program_ptr = state->handles.find(program);
  if (program_ptr == state->handles.end()) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  auto data_buf = memory->getSpan<char>(data, max_size);
  if (data_buf.size() != max_size) {
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return program_ptr->second->bpf_buffer_poll(c_executor, c_module, fd,
                                              sample_func, ctx, data_buf.data(),
                                              max_size, timeout_ms, data);
}

} // namespace Host
} // namespace WasmEdge
