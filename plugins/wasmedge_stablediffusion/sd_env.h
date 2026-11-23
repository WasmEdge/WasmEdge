// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "stable-diffusion.h"

#include "plugin/plugin.h"
#include <memory>
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

struct ContextDeleter {
  void operator()(sd_ctx_t *Ctx) const noexcept;
};

struct ContextInfo {
  std::unique_ptr<sd_ctx_t, ContextDeleter> Context;
  int32_t NThreads;
};

class SDEnviornment {
public:
  SDEnviornment() noexcept {
    if (EnableSDLog) {
      sd_set_log_callback(SBLog, nullptr);
    }
  };
  ~SDEnviornment() noexcept;
  SDEnviornment(const SDEnviornment &) = delete;
  SDEnviornment &operator=(const SDEnviornment &) = delete;
  uint32_t addContext(sd_ctx_t *Ctx, int32_t Nthreads) noexcept;
  void freeContext(const uint32_t Id) noexcept;
  sd_ctx_t *getContext(const uint32_t Id) noexcept;
  size_t getContextSize() noexcept { return Contexts.size(); }
  int32_t getNThreads(const uint32_t Id) noexcept {
    if (Id >= Contexts.size() || Contexts[Id].Context == nullptr) {
      return 0;
    }
    return Contexts[Id].NThreads;
  }

private:
  bool EnableSDLog = false;
  std::vector<ContextInfo> Contexts;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
