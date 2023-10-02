// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "executor/executor.h"
#include "host/wasi_preview2/wasibase.h"

namespace WasmEdge {
namespace Host {

class DropPollable : public WasiPreview2<DropPollable> {
public:
  DropPollable(WASIPreview2::Environ &HostEnv) : WasiPreview2(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, Pollable This);
};

class PollOneoff : public WasiPreview2<PollOneoff> {
public:
  PollOneoff(WASIPreview2::Environ &HostEnv) : WasiPreview2(HostEnv) {}
  Expect<ComponentModel::List<bool>> body(const Runtime::CallingFrame &Frame,
                                          uint32_t PollableListAddr,
                                          uint32_t PollableListLen);
};

} // namespace Host
} // namespace WasmEdge
