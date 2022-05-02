// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "host/wasi_nn/wasinnmodule.h"
#include "host/wasi_nn/wasinnfunc.h"

namespace WasmEdge {
namespace Host {

WasiNNModule::WasiNNModule() : ModuleInstance("wasi_ephemeral_nn") {
  Ctx.BackendsMapping.emplace("OpenVINO",
                              static_cast<WASINN::GraphEncoding>(0));

  addHostFunc("load", std::make_unique<WasiNNLoad>(Ctx));
  addHostFunc("init_execution_context",
              std::make_unique<WasiNNInitExecCtx>(Ctx));
  addHostFunc("set_input", std::make_unique<WasiNNSetInput>(Ctx));
  addHostFunc("get_output", std::make_unique<WasiNNGetOuput>(Ctx));
  addHostFunc("compute", std::make_unique<WasiNNCompute>(Ctx));
}

} // namespace Host
} // namespace WasmEdge
