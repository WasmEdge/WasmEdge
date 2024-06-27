// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"

namespace WasmEdge {
namespace Host {

class Drop : public WasiPoll<Drop> {
public:
  Drop(WasiPollEnvironment &HostEnv) : WasiPoll(HostEnv) {}
  Expect<void> body(Pollable This);
};

class PollOneoff : public WasiPoll<PollOneoff> {
public:
  PollOneoff(WasiPollEnvironment &HostEnv) : WasiPoll(HostEnv) {}
  Expect<List<bool>> body(List<Pollable> In);
};

} // namespace Host
} // namespace WasmEdge
