// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIFinish : public EEI {
public:
  EEIFinish(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIFinish>(); }

  void run(uint32_t DataOffset, uint32_t DataLength);
};

} // namespace Compiler
} // namespace SSVM
