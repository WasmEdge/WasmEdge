// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"

namespace WasmEdge {
namespace Host {

/// Perform bpf map operations on a specified bpf map through map fd.
///
/// Return zero if succeed, others if error
class BpfMapOperate : public WasmEdge::Runtime::HostFunction<BpfMapOperate> {
public:
  BpfMapOperate(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 int32_t fd, int32_t cmd, uint32_t key,
                                 uint32_t value, uint32_t next_key,
                                 uint64_t flags);

private:
  state_t state;
};

} // namespace Host
} // namespace WasmEdge
