// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llmc_module.h"
#include "llmc_func.h"

namespace WasmEdge::Host {

LLMCModule::LLMCModule() : ModuleInstance("wasmedge_llmc") {
  addHostFunc("model_create", std::make_unique<LLMCModelCreate>(Env));
  addHostFunc("dataloader_create", std::make_unique<LLMCDataLoaderCreate>(Env));
  addHostFunc("tokenizer_create", std::make_unique<LLMCTokenizerCreate>(Env));
  addHostFunc("model_train", std::make_unique<LLMCModelTrain>(Env));
}

} // namespace WasmEdge::Host
