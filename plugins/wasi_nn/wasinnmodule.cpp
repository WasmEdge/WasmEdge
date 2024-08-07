// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasinnmodule.h"
#include "wasinnfunc.h"

namespace WasmEdge {
namespace Host {

WasiNNModule::WasiNNModule() : ModuleInstance("wasi_ephemeral_nn") {
  addHostFunc("load", std::make_unique<WasiNNLoad>(Env));
  addHostFunc("load_by_name", std::make_unique<WasiNNLoadByName>(Env));
  addHostFunc("load_by_name_with_config",
              std::make_unique<WasiNNLoadByNameWithConfig>(Env));
  addHostFunc("init_execution_context",
              std::make_unique<WasiNNInitExecCtx>(Env));
  addHostFunc("set_input", std::make_unique<WasiNNSetInput>(Env));
  addHostFunc("get_output", std::make_unique<WasiNNGetOutput>(Env));
  addHostFunc("get_output_single",
              std::make_unique<WasiNNGetOutputSingle>(Env));
  addHostFunc("compute", std::make_unique<WasiNNCompute>(Env));
  addHostFunc("compute_single", std::make_unique<WasiNNComputeSingle>(Env));
  addHostFunc("fini_single", std::make_unique<WasiNNFiniSingle>(Env));
  addHostFunc("unload", std::make_unique<WasiNNUnload>(Env));
}

} // namespace Host
} // namespace WasmEdge
