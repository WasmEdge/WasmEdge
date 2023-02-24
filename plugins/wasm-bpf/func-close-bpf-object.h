#ifndef _FUNC_CLOSE_BPF_OBJECT
#define _FUNC_CLOSE_BPF_OBJECT

#include <cinttypes>
#include <memory>
#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
class CloseBpfObject : public WasmEdge::Runtime::HostFunction<CloseBpfObject> {
   public:
    CloseBpfObject(state_t state) : state(state) {}
    WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame& Frame,
                                   handle_t program);

   private:
    state_t state;
};

#endif