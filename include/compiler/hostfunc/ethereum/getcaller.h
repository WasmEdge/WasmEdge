// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIGetCaller : public EEI {
public:
  EEIGetCaller(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIGetCaller>(); }

  void run(uint32_t ResultOffset);
};

} // namespace Compiler
} // namespace SSVM
