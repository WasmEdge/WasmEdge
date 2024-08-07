// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"

namespace WasmEdge {
namespace Host {

/// \brief Close an opened bpf object. Will remove mapfds from the cache.
/// Return 0 if success. Others represent error codes.
class CloseBpfObject : public WasmEdge::Runtime::HostFunction<CloseBpfObject> {
public:
  CloseBpfObject(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program);

private:
  state_t state;
};

} // namespace Host
} // namespace WasmEdge
