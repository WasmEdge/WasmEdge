// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_CLOSE_BPF_OBJECT
#define _FUNC_CLOSE_BPF_OBJECT

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
#include <cinttypes>
#include <memory>
class CloseBpfObject : public WasmEdge::Runtime::HostFunction<CloseBpfObject> {
public:
  CloseBpfObject(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program);

private:
  state_t state;
};

#endif