#include "sd_func.h"
#include "common/spdlog.h"
#include "sd_env.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace {}
Expect<ErrNo>
SDCreateContext::bodyImpl(const Runtime::CallingFrame &) {
  return ErrNo::RuntimeError;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge