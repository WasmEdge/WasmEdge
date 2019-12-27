// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Compiler {

class EEIStorageStore : public EEI {
public:
  EEIStorageStore(Library &Lib, VM::EVMEnvironment &Env) : EEI(Lib, Env) {}

  void *getFunction() override { return proxy<EEIStorageStore>(); }

  void run(uint32_t PathOffset, uint32_t ValueOffset);
};

} // namespace Compiler
} // namespace SSVM
