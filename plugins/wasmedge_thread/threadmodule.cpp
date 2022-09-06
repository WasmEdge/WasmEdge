// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "threadmodule.h"
#include "threadfunc.h"

namespace WasmEdge {
namespace Host {

WasmEdgeThreadModule::WasmEdgeThreadModule() : ModuleInstance("wasmedge_thread") {
  addHostFunc("pthread_create", std::make_unique<WasmEdgeThreadCreate>(Env));
  addHostFunc("pthread_join", std::make_unique<WasmEdgeThreadJoin>(Env));
}

} // namespace Host
} // namespace WasmEdge
