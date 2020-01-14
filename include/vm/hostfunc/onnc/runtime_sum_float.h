// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeSumFloat : public ONNCWasm<ONNCRuntimeSumFloat> {
public:
  ONNCRuntimeSumFloat() : ONNCWasm("ONNC_RUNTIME_sum_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InDataOffOff, uint32_t InDataNTensor,
               uint32_t InDataNDimOff, uint32_t InDataDimsOffOff,
               uint32_t OutSumOff, uint32_t OutSumNDim, uint32_t OutSumDimsOff);
};

} // namespace Executor
} // namespace SSVM
