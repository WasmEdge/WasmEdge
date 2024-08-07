// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

using Pollable = uint32_t;

class Drop : public WasiPoll<Drop> {
public:
  Drop(WasiPollEnvironment &HostEnv) : WasiPoll(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, Pollable This);
};

// poll-oneoff: func(in: list<pollable>) -> list<bool>

} // namespace Host
} // namespace WasmEdge
