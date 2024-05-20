#pragma once

#include "runtime/instance/module.h"
#include "sb_env.h"

namespace WasmEdge {
namespace Host {

class SBModule : public Runtime::Instance::ModuleInstance {
public:
  SBModule();
  StableDiffusion::SBEnviornment &getEnv() { return Env; }

private:
  StableDiffusion::SBEnviornment Env;
};

} // namespace Host
} // namespace WasmEdge