#pragma once

#include "executor/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Executor {

class EEI : public HostFunction {
public:
  EEI(VM::Environment &HostEnv) : Env(HostEnv) {}
  EEI() = delete;
  virtual ~EEI() = default;

protected:
  VM::Environment &Env;
};

} // namespace Executor
} // namespace SSVM