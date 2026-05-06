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

/// \brief Load a BPF ELF file.
///
/// A binary file should be provided through a Wasm buffer. wasm_bpf handles
/// the remaining process. Calling this function also caches BPF map fds.
///
/// \return a handle to a BPF program, which is stored in a map in the global
/// state. Returns 0 on failure.
class LoadBpfObject : public WasmEdge::Runtime::HostFunction<LoadBpfObject> {
public:
  LoadBpfObject(state_t state) : state(state) {}
  WasmEdge::Expect<handle_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                  uint32_t obj_buf, uint32_t obj_buf_sz);

private:
  state_t state;
};

} // namespace Host
} // namespace WasmEdge
