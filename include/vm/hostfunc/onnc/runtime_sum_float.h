// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeSumFloat : public HostFunction {
public:
  ONNCRuntimeSumFloat();

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InDataOffOff, uint32_t InDataNTensor,
               uint32_t InDataNDimOff, uint32_t InDataDimsOffOff,
               uint32_t OutSumOff, uint32_t OutSumNDim, uint32_t OutSumDimsOff);
};

} // namespace Executor
} // namespace SSVM
