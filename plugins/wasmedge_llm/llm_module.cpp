// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llm_module.h"
#include "llm_func.h"

namespace WasmEdge {
namespace Host {

WasmEdgeLLMModule::WasmEdgeLLMModule() : ModuleInstance("wasmedge_llm") {
  addHostFunc("model_create", std::make_unique<WasmEdgeLLM::ModelCreate>(Env));
  addHostFunc("dataloader_create",
              std::make_unique<WasmEdgeLLM::DataLoaderCreate>(Env));
  addHostFunc("tokenizer_create",
              std::make_unique<WasmEdgeLLM::TokenizerCreate>(Env));
  addHostFunc("model_train", std::make_unique<WasmEdgeLLM::ModelTrain>(Env));
}

} // namespace Host
} // namespace WasmEdge
