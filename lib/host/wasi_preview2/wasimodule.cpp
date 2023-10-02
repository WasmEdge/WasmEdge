// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#include "host/wasi_preview2/wasimodule.h"
#include "host/wasi_preview2/wasifunc.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiPreview2Module::WasiPreview2Module()
    : ModuleInstance("wasi_snapshot_preview2") {
  addHostFunc("drop-pollable", std::make_unique<DropPollable>(Env));
  // addHostFunc("poll-oneoff", std::make_unique<PollOneoff>(Env));
}

} // namespace Host
} // namespace WasmEdge
