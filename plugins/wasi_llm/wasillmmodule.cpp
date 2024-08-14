// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasillmmodule.h"
#include "wasillmfunc.h"

namespace WasmEdge {
namespace Host {

WasiLLMModule::WasiLLMModule() : ModuleInstance("wasi_llm") {
  addHostFunc("model_create", std::make_unique<WasiLLMModelCreate>(Env));
  addHostFunc("dataloader_create",
              std::make_unique<WasiLLMDataLoaderCreate>(Env));
  addHostFunc("tokenizer_create",
              std::make_unique<WasiLLMTokenizerCreate>(Env));
  addHostFunc("model_train", std::make_unique<WasiLLMModelTrain>(Env));
}

} // namespace Host
} // namespace WasmEdge
