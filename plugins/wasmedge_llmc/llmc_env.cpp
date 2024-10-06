// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llmc_env.h"
#include "llmc_fwd.h"
#include "llmc_module.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeLLMC {

uint32_t LLMCEnv::addModel(GPT2 *M) noexcept {
  Models.push_back(M);
  return Models.size() - 1;
}

GPT2 *LLMCEnv::getModel(uint32_t Id) noexcept {
  assert(Id < Models.size() && "Out of bounds");
  return Models[Id];
}

uint32_t LLMCEnv::addTokenizer(Tokenizer *T) noexcept {
  Tokenizers.push_back(T);
  return Tokenizers.size() - 1;
}

Tokenizer *LLMCEnv::getTokenizer(uint32_t Id) noexcept {
  assert(Id < Tokenizers.size() && "Out of bounds");
  return Tokenizers[Id];
}

uint32_t LLMCEnv::addDataLoader(DataLoader *D) noexcept {
  DataLoaders.push_back(D);
  return DataLoaders.size() - 1;
}

DataLoader *LLMCEnv::getDataLoader(uint32_t Id) noexcept {
  assert(Id < DataLoaders.size() && "Out of bounds");
  return DataLoaders[Id];
}

LLMCEnv::~LLMCEnv() {
  for (GPT2 *M : Models) {
    gpt2_destroy(M);
  }
  for (DataLoader *DL : DataLoaders) {
    dataloader_destroy(DL);
  }
  for (Tokenizer *T : Tokenizers) {
    tokenizer_destroy(T);
  }
}

namespace {
Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new WasmEdgeLLMCModule;
}

static Plugin::PluginModule::ModuleDescriptor MD[] = {
    {
        /* Name */ "wasmedge_llmc",
        /* Description */ "",
        /* Create */ create,
    },
};

Plugin::Plugin::PluginDescriptor Descriptor{
    /* Name */ "wasmedge_llmc",
    /* Description */ "",
    /* APIVersion */ Plugin::Plugin::CurrentAPIVersion,
    /* Version */ {0, 1, 0, 0},
    /* ModuleCount */ 1,
    /* ModuleDescriptions */ MD,
    /* ComponentCount */ 0,
    /* ComponentDescriptions */ nullptr,
    /*AddOptions*/ nullptr,
};
} // namespace

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace WasmEdgeLLMC
} // namespace Host
} // namespace WasmEdge
