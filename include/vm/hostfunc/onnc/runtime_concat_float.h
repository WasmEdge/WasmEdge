// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeConcatFloat : public ONNCWasm<ONNCRuntimeConcatFloat> {
public:
  ONNCRuntimeConcatFloat() : ONNCWasm("ONNC_RUNTIME_concat_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InInputsOffOff, uint32_t InInputsNTensor,
               uint32_t InInputsNDimOff, uint32_t InInputsDimsOffOff,
               uint32_t OutConcatResultOff, uint32_t OutConcatResultNDim,
               uint32_t OutConcatResultDimsOff, uint32_t Axis);
};

} // namespace Executor
} // namespace SSVM
