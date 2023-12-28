#pragma once

#include "runtime/instance/module.h"
#include "wasi_random/env.h"

namespace WasmEdge {
namespace Host {

class WasiRandomModule : public Runtime::Instance::ModuleInstance {
public:
  WasiRandomModule();
  WasiRandomEnvironment &getEnv() { return Env; }

private:
  WasiRandomEnvironment Env;
};

class WasiRandomInsecureModule : public Runtime::Instance::ModuleInstance {
public:
  WasiRandomInsecureModule();
  WasiRandomEnvironment &getEnv() { return Env; }

private:
  WasiRandomEnvironment Env;
};

class WasiRandomInsecureSeedModule : public Runtime::Instance::ModuleInstance {
public:
  WasiRandomInsecureSeedModule();
  WasiRandomEnvironment &getEnv() { return Env; }

private:
  WasiRandomEnvironment Env;
};

} // namespace Host
} // namespace WasmEdge
