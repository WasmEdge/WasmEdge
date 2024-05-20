#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "sb_env.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

template <typename T> class Func : public Runtime::HostFunction<T> {
public:
  Func(SBEnviornment &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  static constexpr uint32_t castErrNo(StableDiffusion::ErrNo E) noexcept {
    return static_cast<uint32_t>(E);
  }
  SBEnviornment &Env;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge