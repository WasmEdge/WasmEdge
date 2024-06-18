// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "module.h"
#include "func.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiPollModule::WasiPollModule() : ComponentInstance("wasi:poll/poll") {
  addHostFunc("drop-pollable", std::make_unique<Drop>(Env));
}

} // namespace Host
} // namespace WasmEdge
