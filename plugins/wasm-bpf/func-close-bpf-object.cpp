#include "func-close-bpf-object.h"
#include <shared_mutex>

using namespace WasmEdge;

Expect<int32_t> CloseBpfObject::body(const WasmEdge::Runtime::CallingFrame&,
                                  handle_t program) {
    std::shared_lock guard(this->state->lock);
    auto& handles = this->state->handles;
    if (!handles.count(program)) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto val = handles[program];
    handles.erase(program);
    delete val;
    return 0;
}