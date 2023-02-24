// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_BPF_BUFFER_POLL
#define _FUNC_BPF_BUFFER_POLL

#include <cinttypes>
#include <memory>
#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
class BpfBufferPoll : public WasmEdge::Runtime::HostFunction<BpfBufferPoll> {
   public:
    BpfBufferPoll(state_t state) : state(state) {}
    WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame& Frame,
                                   handle_t program,
                                   int fd,
                                   int32_t sample_func,
                                   uint32_t ctx,
                                   uint32_t data,
                                   int max_size,
                                   int timeout_ms);

   private:
    state_t state;
};

#endif