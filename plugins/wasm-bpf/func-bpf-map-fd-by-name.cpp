#include "func-bpf-map-fd-by-name.h"
#include <shared_mutex>
#include "util.h"
using namespace WasmEdge;

Expect<int32_t> BpfMapFdByName::body(const Runtime::CallingFrame& Frame,
                                     handle_t program,
                                     uint32_t name) {
    const char* name_str;
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
    auto& handles = this->state->handles;
    if (!handles.count(program)) {
        return Unexpect(ErrCode::Value::HostFuncError);
    }
    auto val = handles[program];
    return int32_t(val->bpf_map_fd_by_name(name_str));
}