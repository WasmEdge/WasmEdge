#pragma once

#include "stable-diffusion.h"

#include "plugin/plugin.h"

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

struct Context {
  Context() noexcept {}
  ~Context() noexcept {}
};

class SDEnviornment {};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge