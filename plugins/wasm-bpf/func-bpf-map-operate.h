// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_BPF_MAP_OPERATE
#define _FUNC_BPF_MAP_OPERATE

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
#include <cinttypes>
#include <memory>
#include <mutex>
#include <unordered_map>
/// @brief Perform bpf map operations on a specified bpf map through map fd.
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

#endif