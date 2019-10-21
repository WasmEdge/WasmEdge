#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Executor {

class Wasi : public HostFunction {
public:
  Wasi(VM::WasiEnvironment &HostEnv) : Env(HostEnv) {}
  Wasi() = delete;
  virtual ~Wasi() = default;

protected:
  VM::WasiEnvironment &Env;
};

} // namespace Executor
} // namespace SSVM