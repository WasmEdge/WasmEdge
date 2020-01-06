// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeTransposeFloat : public HostFunction {
public:
  ONNCRuntimeTransposeFloat();

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
               uint32_t OutTransposedOff, uint32_t OutTransposedNDim,
               uint32_t OutTransposedDimsOff, uint32_t PermOff,
               uint32_t PermNum);
};

} // namespace Executor
} // namespace SSVM
