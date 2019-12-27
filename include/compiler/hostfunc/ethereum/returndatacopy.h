// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIReturnDataCopy : public EEI {
public:
  EEIReturnDataCopy(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIReturnDataCopy>(); }

  void run(uint32_t ResultOffset, uint32_t DataOffset, uint32_t Length);
};

} // namespace Compiler
} // namespace SSVM
