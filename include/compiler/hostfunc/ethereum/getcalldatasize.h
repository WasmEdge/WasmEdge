// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIGetCallDataSize : public EEI {
public:
  EEIGetCallDataSize(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIGetCallDataSize>(); }

  uint32_t run();
};

} // namespace Compiler
} // namespace SSVM
