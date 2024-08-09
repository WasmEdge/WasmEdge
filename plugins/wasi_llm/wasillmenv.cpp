// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "wasillmenv.h"
#include "llmc_fwd.h"
#include "wasillmmodule.h"

namespace WasmEdge {
namespace Host {

namespace WASILLM {

uint32_t WASILLMEnv::addModel(GPT2 *M) noexcept {
  Models.push_back(M);
  return Models.size() - 1;
}

GPT2 *WASILLMEnv::getModel(uint32_t Id) noexcept { return Models[Id]; }

uint32_t WASILLMEnv::addTokenizer(Tokenizer *T) noexcept {
  Tokenizers.push_back(T);
  return Tokenizers.size() - 1;
}

Tokenizer *WASILLMEnv::getTokenizer(uint32_t Id) noexcept {
  return Tokenizers[Id];
}

uint32_t WASILLMEnv::addDataLoader(DataLoader *D) noexcept {
  DataLoaders.push_back(D);
  return DataLoaders.size() - 1;
}

DataLoader *WASILLMEnv::getDataLoader(uint32_t Id) noexcept {
  return DataLoaders[Id];
}

WASILLMEnv::~WASILLMEnv() {
  for (GPT2 *M : Models) {
    gpt2_free(M);
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
  return new WasiLLMModule;
}

static Plugin::PluginModule::ModuleDescriptor MD[] = {
    {
        /* Name */ "wasi_llm",
        /* Description */ "",
        /* Create */ create,
    },
};

Plugin::Plugin::PluginDescriptor Descriptor{
    /* Name */ "wasi_llm",
    /* Description */ "",
    /* APIVersion */ Plugin::Plugin::CurrentAPIVersion,
    /* Version */ {0, 10, 1, 0},
    /* ModuleCount */ 1,
    /* ModuleDescriptions */ MD,
    /* ComponentCount */ 0,
    /* ComponentDescriptions */ nullptr,
    /*AddOptions=*/nullptr,
};
} // namespace

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace WASILLM

} // namespace Host
} // namespace WasmEdge
