// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasillmmodule.h"
#include "wasillmfunc.h"

namespace WasmEdge {
namespace Host {

WasiLLMModule::WasiLLMModule() : ModuleInstance("wasi_llm") {
  addHostFunc("model_create", std::make_unique<WasiLLMModelCreate>());
  addHostFunc("model_free", std::make_unique<WasiLLMModelFree>());
  addHostFunc("dataloader_create", std::make_unique<WasiLLMDataLoaderCreate>());
  addHostFunc("dataloader_free", std::make_unique<WasiLLMDataLoaderFree>());
  addHostFunc("tokenizer_create", std::make_unique<WasiLLMTokenizerCreate>());
  addHostFunc("tokenizer_free", std::make_unique<WasiLLMTokenizerFree>());
  addHostFunc("model_train", std::make_unique<WasiLLMModelTrain>());
}

} // namespace Host
} // namespace WasmEdge
