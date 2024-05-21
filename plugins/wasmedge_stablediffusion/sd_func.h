#pragma once

#include "runtime/callingframe.h"
#include "sd_base.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
class SDCreateContext : public StableDiffusion::Func<SDCreateContext> {
public:
  SDCreateContext(StableDiffusion::SDEnviornment &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame) {
    return bodyImpl(Frame).map(castErrNo);
  }

private:
  Expect<StableDiffusion::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame);
};
} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge