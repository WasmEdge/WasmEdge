// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llmc_module.h"
#include "llmc_func.h"

namespace WasmEdge {
namespace Host {

WasmEdgeLLMCModule::WasmEdgeLLMCModule() : ModuleInstance("wasmedge_llmc") {
  addHostFunc("model_create", std::make_unique<WasmEdgeLLMC::ModelCreate>(Env));
  addHostFunc("dataloader_create",
              std::make_unique<WasmEdgeLLMC::DataLoaderCreate>(Env));
  addHostFunc("tokenizer_create",
              std::make_unique<WasmEdgeLLMC::TokenizerCreate>(Env));
  addHostFunc("model_train", std::make_unique<WasmEdgeLLMC::ModelTrain>(Env));
}

} // namespace Host
} // namespace WasmEdge
