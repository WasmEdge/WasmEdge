// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEICallStatic : public EEI {
public:
  EEICallStatic(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEICallStatic>(); }

  uint32_t run(uint32_t Gas, uint32_t AddressOffset, uint32_t DataOffset,
               uint32_t DataLength);
};

} // namespace Compiler
} // namespace SSVM
