// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "func-load-bpf-object.h"
#include <shared_mutex>
using namespace WasmEdge;
Expect<handle_t> LoadBpfObject::body(const Runtime::CallingFrame& Frame,
                                     uint32_t obj_buf,
                                     uint32_t obj_buf_sz) {
    auto* memory = Frame.getMemoryByIndex(0);
    if (memory == nullptr) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    char* const buf = memory->getPointer<char*>(obj_buf, obj_buf_sz);
    if (buf == nullptr) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }

    wasm_bpf_program* program = new wasm_bpf_program();
    int res = program->load_bpf_object(buf, (size_t)obj_buf_sz);
    if (res < 0) {
        delete program;
        return 0;
    }
    auto& state = this->state;

    std::shared_lock guard(state->lock);
    handle_t current_handle = state->next_handle++;
    state->handles[current_handle] = program;
    return current_handle;
}
