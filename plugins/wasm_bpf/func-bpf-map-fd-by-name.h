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

/// \brief Lookup a map fd by its name.
///
/// Map fd is returned if succeed, others if failed.
class BpfMapFdByName : public WasmEdge::Runtime::HostFunction<BpfMapFdByName> {
public:
  BpfMapFdByName(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program, uint32_t name);

private:
  state_t state;
};

} // namespace Host
} // namespace WasmEdge
