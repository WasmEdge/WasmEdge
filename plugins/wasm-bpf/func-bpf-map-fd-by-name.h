#ifndef _FUNC_BPF_MAP_FD_BY_NAME
#define _FUNC_BPF_MAP_FD_BY_NAME

#include <cinttypes>
#include <memory>
#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
class BpfMapFdByName : public WasmEdge::Runtime::HostFunction<BpfMapFdByName> {
   public:
    BpfMapFdByName(state_t state) : state(state) {}
    WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame& Frame,
                                   handle_t program,
                                   uint32_t name);

   private:
    state_t state;
};

#endif