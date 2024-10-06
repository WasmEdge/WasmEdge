// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

// BUILTIN-PLUGIN: Temporary move the wasi-logging plugin sources here until
// the new plugin architecture ready in 0.15.0.

#pragma once

#include "env.h"

#include "common/errcode.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WASILogging {

enum class LogLevel : uint32_t { Trace, Debug, Info, Warn, Error, Critical };

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(LogEnv &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  LogEnv &Env;
};

} // namespace WASILogging
} // namespace Host
} // namespace WasmEdge
