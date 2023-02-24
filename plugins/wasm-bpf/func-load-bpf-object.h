// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_LOAD_BPF_OBJECT
#define _FUNC_LOAD_BPF_OBJECT

#include <cinttypes>
#include <memory>
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
#include "bpf-api.h"
class LoadBpfObject : public WasmEdge::Runtime::HostFunction<LoadBpfObject> {
   public:
    LoadBpfObject(state_t state) : state(state) {}
    WasmEdge::Expect<handle_t> body(const WasmEdge::Runtime::CallingFrame& Frame,
                          uint32_t obj_byf,
                          uint32_t obj_buf_sz);

   private:
    state_t state;
};

#endif