// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#ifndef _FUNC_BPF_MAP_FD_BY_NAME
#define _FUNC_BPF_MAP_FD_BY_NAME

#include "bpf-api.h"
#include "plugin/plugin.h"
#include "po/helper.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"
#include "state.h"
#include <cinttypes>
#include <memory>
/// @brief Lookup a map fd by its name. Map fd is returned if succeed, others if
/// failed
class BpfMapFdByName : public WasmEdge::Runtime::HostFunction<BpfMapFdByName> {
public:
  BpfMapFdByName(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program, uint32_t name);

private:
  state_t state;
};

#endif