// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeLrnFloat : public ONNCWasm<ONNCRuntimeLrnFloat> {
public:
  ONNCRuntimeLrnFloat() : ONNCWasm("ONNC_RUNTIME_lrn_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t OutYOff, uint32_t OutYNDim, uint32_t OutYDimsOff,
               float Alpha, float Beta, float Bias, uint32_t Size);
};

} // namespace Executor
} // namespace SSVM
