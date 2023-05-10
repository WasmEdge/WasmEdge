// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/mock/wasi_threads_func.h"
#include "runtime/instance/module.h"

namespace WasmEdge {
namespace Host {

class WasiThreadsModuleMock : public Runtime::Instance::ModuleInstance {
public:
  WasiThreadsModuleMock() : Runtime::Instance::ModuleInstance("wasi") {
    addHostFunc("wasi_thread_spawn",
                std::make_unique<WasiThreadsMock::ThreadSpawn>());
  }
};

} // namespace Host
} // namespace WasmEdge
