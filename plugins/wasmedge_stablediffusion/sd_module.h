#pragma once

#include "runtime/instance/module.h"
#include "sd_env.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
class SDModule : public Runtime::Instance::ModuleInstance {
public:
  SDModule();
  StableDiffusion::SDEnviornment &getEnv() { return Env; }

private:
  StableDiffusion::SDEnviornment Env;
};

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge