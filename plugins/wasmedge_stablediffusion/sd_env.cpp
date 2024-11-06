// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "sd_env.h"
#include "sd_module.h"

namespace WasmEdge {
namespace Host {
namespace {

Runtime::Instance::ModuleInstance *
create(const Plugin::PluginModule::ModuleDescriptor *) noexcept {
  return new SDModule;
}

Plugin::Plugin::PluginDescriptor Descriptor{
    .Name = "wasmedge_stablediffusion",
    .Description = "Stable Diffusion plug-in for WasmEdge.",
    .APIVersion = Plugin::Plugin::CurrentAPIVersion,
    .Version = {0, 1, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions =
        (Plugin::PluginModule::ModuleDescriptor[]){
            {
                .Name = "wasmedge_stablediffusion",
                .Description =
                    "This module contains Stable Diffusion host functions.",
                .Create = create,
            },
        },
    .AddOptions = nullptr,
};

EXPORT_GET_DESCRIPTOR(Descriptor)

} // namespace

namespace StableDiffusion {

uint32_t SDEnviornment::addContext(sd_ctx_t *Ctx, int32_t Nthreads,
                                   uint32_t Wtype) noexcept {
  Contexts.push_back({Ctx, Nthreads, Wtype});
  return Contexts.size() - 1;
}

void SDEnviornment::freeContext(const uint32_t Id) noexcept {
  sd_ctx_t *SDCtx = Contexts[Id].Context;
  free_sd_ctx(SDCtx);
  Contexts.erase(Contexts.begin() + Id - 1);
}

sd_ctx_t *SDEnviornment::getContext(const uint32_t Id) noexcept {
  if (Id >= Contexts.size()) {
    return nullptr;
  }
  return Contexts[Id].Context;
}

void SBLog(enum sd_log_level_t Level, const char *Log, void *) {
  if (!Log) {
    return;
  }
  std::string LevelStr;
  switch (Level) {
  case SD_LOG_DEBUG:
    LevelStr = "DEBUG";
    break;
  case SD_LOG_INFO:
    LevelStr = "INFO";
    break;
  case SD_LOG_WARN:
    LevelStr = "WARN";
    break;
  case SD_LOG_ERROR:
    LevelStr = "ERROR";
    break;
  default:
    LevelStr = "?????";
    break;
  }

  spdlog::info("[WasmEdge-StableDiffusion] SD-log: [{}] {}", LevelStr, Log);
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
