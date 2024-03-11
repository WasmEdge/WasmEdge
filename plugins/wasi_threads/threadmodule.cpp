// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadmodule.h"
#include "threadfunc.h"

namespace WasmEdge {
namespace Host {

WasiThreadsModule::WasiThreadsModule() : ModuleInstance("wasi") {
  addHostFunc("thread-spawn", std::make_unique<WasiThreadSpawn>(Env));
}

} // namespace Host
} // namespace WasmEdge
