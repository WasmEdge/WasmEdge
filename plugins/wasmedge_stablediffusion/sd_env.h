#pragma once

#include "stable-diffusion.h"

#include "plugin/plugin.h"
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

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
  uint32_t addContext(sd_ctx_t *sd_ctx) noexcept;
  sd_ctx_t *getContext(const uint32_t Id) noexcept;

private:
  std::vector<sd_ctx_t *> Contexts;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge