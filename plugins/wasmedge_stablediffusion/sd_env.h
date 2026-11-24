// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "stable-diffusion.h"

#include "plugin/plugin.h"
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

void SBLog(enum sd_log_level_t Level, const char *Log, void *);

struct ContextInfo {
  sd_ctx_t *Context;
  int32_t NThreads;
  uint32_t Wtype;
};

class SDEnviornment {
public:
  SDEnviornment() noexcept {
    if (EnableSDLog) {
      sd_set_log_callback(SBLog, nullptr);
    }
  };
  uint32_t addContext(sd_ctx_t *Ctx, int32_t Nthreads, uint32_t Wtype) noexcept;
  void freeContext(const uint32_t Id) noexcept;
  sd_ctx_t *getContext(const uint32_t Id) noexcept;
  size_t getContextSize() noexcept { return Contexts.size(); }
  int32_t getNThreads(const uint32_t Id) noexcept {
    return Contexts[Id].NThreads;
  }
  uint32_t getWtype(const uint32_t Id) noexcept { return Contexts[Id].Wtype; }

private:
  bool EnableSDLog = false;
  std::vector<ContextInfo> Contexts;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
