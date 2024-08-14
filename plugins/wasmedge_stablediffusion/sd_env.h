// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "stable-diffusion.h"

#include "plugin/plugin.h"
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

void SBLog(enum sd_log_level_t Level, const char *Log, void *);

enum class ErrNo : uint32_t {
  Success = 0,         // No error occurred.
  InvalidArgument = 1, // Caller module passed an invalid argument.
  InvalidEncoding = 2, // Invalid encoding.
  MissingMemory = 3,   // Caller module is missing a memory export.
  Busy = 4,            // Device or resource busy.
  RuntimeError = 5,    // Runtime Error.
};

class SDEnviornment {
public:
  SDEnviornment() noexcept {
    if (EnableSDLog) {
      sd_set_log_callback(SBLog, nullptr);
    }
  };
  uint32_t addContext(sd_ctx_t *Ctx) noexcept;
  sd_ctx_t *getContext(const uint32_t Id) noexcept;
  size_t getContextSize() noexcept { return Contexts.size(); }

private:
  std::vector<sd_ctx_t *> Contexts;
  bool EnableSDLog = false;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
