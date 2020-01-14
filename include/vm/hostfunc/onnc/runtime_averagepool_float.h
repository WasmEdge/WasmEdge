// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeAveragepoolFloat
    : public ONNCWasm<ONNCRuntimeAveragepoolFloat> {
public:
  ONNCRuntimeAveragepoolFloat() : ONNCWasm("ONNC_RUNTIME_averagepool_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t OutYOff, uint32_t OutYNDim, uint32_t OutYDimsOff,
               uint32_t AutoPadOff, uint32_t IncludePadCnt,
               uint32_t KernelShapeOff, uint32_t KernelShapeNum,
               uint32_t PadsOff, uint32_t PadsNum, uint32_t StridesOff,
               uint32_t StridesNum);
};

} // namespace Executor
} // namespace SSVM
