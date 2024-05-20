#include "sb_func.h"
#include "common/spdlog.h"
#include "sb_env.h"

namespace WasmEdge {
namespace Host {
namespace {}
Expect<StableDiffusion::ErrNo>
SBCreateContext::bodyImpl(const Runtime::CallingFrame &Frame) {
  return StableDiffusion::ErrNo::RuntimeError;
}

} // namespace Host
} // namespace WasmEdge