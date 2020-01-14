// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeMaxpoolFloat : public ONNCWasm<ONNCRuntimeMaxpoolFloat> {
public:
  ONNCRuntimeMaxpoolFloat() : ONNCWasm("ONNC_RUNTIME_maxpool_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t OutYOff, uint32_t OutYNDim, uint32_t OutYDimsOff,
               uint32_t OutIndicesOff, uint32_t OutIndicesNDim,
               uint32_t OutIndicesDimsOff, uint32_t AutoPadOff,
               uint32_t KernelShapeOff, uint32_t KernelShapeNum,
               uint32_t PadsOff, uint32_t PadsNum, uint32_t StorageOrder,
               uint32_t StridesOff, uint32_t StridesNum);
};

} // namespace Executor
} // namespace SSVM
