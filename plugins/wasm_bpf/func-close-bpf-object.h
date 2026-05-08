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

/// \brief Close an opened BPF object and remove map fds from the cache.
/// Returns 0 on success; other values represent error codes.
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
