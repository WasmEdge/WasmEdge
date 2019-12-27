// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "compiler/hostfunc.h"
#include "vm/environment.h"

namespace SSVM {
namespace Compiler {

class EEI : public HostFunction {
public:
  EEI(Library &Lib, VM::EVMEnvironment &HostEnv)
      : HostFunction(Lib), Env(HostEnv) {}

protected:
  VM::EVMEnvironment &Env;
};

} // namespace Compiler
} // namespace SSVM
