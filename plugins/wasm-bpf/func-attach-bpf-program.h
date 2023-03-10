// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_ATTACH_BPF_OBJECT
#define _FUNC_ATTACH_BPF_OBJECT

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
#include <cinttypes>
#include <memory>
/// @brief Attach a bpf program to the specified target
class AttachBpfProgram
    : public WasmEdge::Runtime::HostFunction<AttachBpfProgram> {
public:
  AttachBpfProgram(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program, uint32_t name,
                                 uint32_t attach_target);

private:
  state_t state;
};

#endif