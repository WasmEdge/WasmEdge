// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeBatchnormalizationInt8 : public HostFunction {
public:
  ONNCRuntimeBatchnormalizationInt8();

  ErrCode run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
              Instance::MemoryInstance &MemInst) override;

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t InScaleOff, uint32_t InScaleNDim,
               uint32_t InScaleDimsOff, uint32_t InBOff, uint32_t InBNDim,
               uint32_t InBDimsOff, uint32_t InMeanOff, uint32_t InMeanNDim,
               uint32_t InMeanDimsOff, uint32_t InVarOff, uint32_t InVarNDim,
               uint32_t InVarDimsOff, uint32_t OutYOff, uint32_t OutYNDim,
               uint32_t OutYDimsOff, uint32_t OutMeanOff, uint32_t OutMeanNDim,
               uint32_t OutMeanDimsOff, uint32_t OutVarOff, uint32_t OutVarNDim,
               uint32_t OutVarDimsOff, uint32_t OutSavedMeanOff,
               uint32_t OutSavedMeanNDim, uint32_t OutSavedMeanDimsOff,
               uint32_t OutSavedVarOff, uint32_t OutSavedVarNDim,
               uint32_t OutSavedVarDimsOff, int32_t Epsilon, int32_t Momentum,
               uint32_t Spatial);
};

} // namespace Executor
} // namespace SSVM
