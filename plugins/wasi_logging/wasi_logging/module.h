#pragma once

#include "runtime/instance/module.h"
#include "wasi_logging/env.h"

namespace WasmEdge {
namespace Host {

class WasiLoggingModule : public Runtime::Instance::ModuleInstance {
public:
  WasiLoggingModule();

  WasiLoggingEnvironment &getEnv() { return Env; }

private:
  WasiLoggingEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
