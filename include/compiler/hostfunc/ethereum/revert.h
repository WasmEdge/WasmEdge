// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIRevert : public EEI {
public:
  EEIRevert(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIRevert>(); }

  void run(uint32_t DataOffset, uint32_t DataLength);
};

} // namespace Compiler
} // namespace SSVM
