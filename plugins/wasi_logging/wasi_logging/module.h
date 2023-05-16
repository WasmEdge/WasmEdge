#pragma once

#include "wasi_logging/env.h"
#include "runtime/instance/module.h"

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
