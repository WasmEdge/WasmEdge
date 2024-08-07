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

/// Perform a bpf buffer poll. If the map is not opened, it will be opened.
///
/// \param fd the map fd for bpf buffer.
/// \param sample_func callback function. When things are polled, it will be
/// invoked.
/// \param ctx user customized variable.
/// \param data data buffer that will be used to store the polled data.
/// \param max_size How many bytes can be put at data.
/// \param timeout_ms how many milliseconds can be waited.
///
/// \return On success, return 0. On error, return error code.
class BpfBufferPoll : public WasmEdge::Runtime::HostFunction<BpfBufferPoll> {
public:
  BpfBufferPoll(state_t state) : state(state) {}
  WasmEdge::Expect<int32_t> body(const WasmEdge::Runtime::CallingFrame &Frame,
                                 handle_t program, int32_t fd,
                                 int32_t sample_func, uint32_t ctx,
                                 uint32_t data, uint32_t max_size,
                                 int32_t timeout_ms);

private:
  state_t state;
};

} // namespace Host
} // namespace WasmEdge
