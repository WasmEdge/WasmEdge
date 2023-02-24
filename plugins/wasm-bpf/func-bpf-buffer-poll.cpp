// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-bpf-buffer-poll.h"
#include <shared_mutex>
#include "wasmedge/wasmedge.h"
using namespace WasmEdge;

// Helper functions of context conversions.
#define CONVTO(SIMP, INST, NAME, QUANT)                                \
    inline QUANT auto* to##SIMP##Cxt(QUANT INST* Cxt) noexcept {       \
        return reinterpret_cast<QUANT WasmEdge_##NAME##Context*>(Cxt); \
    }
CONVTO(CallFrame, Runtime::CallingFrame, CallingFrame, const)
Expect<int32_t> BpfBufferPoll::body(const Runtime::CallingFrame& Frame,
                                    handle_t program,
                                    int fd,
                                    int32_t sample_func,
                                    uint32_t ctx,
                                    uint32_t data,
                                    int max_size,
                                    int timeout_ms) {
    std::shared_lock lock(state->lock);
    if (!state->handles.count(program)) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto* program_ptr = state->handles[program];
    auto module_instance = Frame.getModule();
    if (module_instance == nullptr) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto memory = Frame.getMemoryByIndex(0);
    if (memory == nullptr) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto data_buf = memory->getPointer<char*>(data, max_size);
    if (data_buf == nullptr) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto c_ctx = toCallFrameCxt(&Frame);

    auto c_module = WasmEdge_CallingFrameGetModuleInstance(c_ctx);
    auto c_executor = WasmEdge_CallingFrameGetExecutor(c_ctx);
    return program_ptr->bpf_buffer_poll(c_executor, c_module, fd, sample_func,
                                        ctx, data_buf, max_size, timeout_ms,
                                        data);
}
