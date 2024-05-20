#pragma once

#include "runtime/callingframe.h"
#include "sb_base.h"

namespace WasmEdge {
namespace Host {
class SBCreateContext : public StableDiffusion::Func<SBCreateContext> {
public:
  SBCreateContext(StableDiffusion::SBEnviornment &HostEnv) : Func(HostEnv) {}
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame) {
    return bodyImpl(Frame).map(castErrNo);
  }

private:
  Expect<StableDiffusion::ErrNo> bodyImpl(const Runtime::CallingFrame &Frame);
};
} // namespace Host
} // namespace WasmEdge