// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#pragma once

#include "base.h"

namespace WasmEdge {
namespace Host {
namespace WASILogging {

class Log : public Func<Log> {
public:
  Log(LogEnv &HostEnv) : Func(HostEnv) {}
  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Level,
                    uint32_t CxtPtr, uint32_t CxtLen, uint32_t MsgPtr,
                    uint32_t MsgLen);
};

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
