// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeAddFloat : public ONNCWasm<ONNCRuntimeAddFloat> {
public:
  ONNCRuntimeAddFloat() : ONNCWasm("ONNC_RUNTIME_add_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InAOff, uint32_t InANDim, uint32_t InADimsOff,
               uint32_t InBOff, uint32_t InBNDim, uint32_t InBDimsOff,
               uint32_t OutCOff, uint32_t OutCNDim, uint32_t OutCDimsOff);
};

} // namespace Executor
} // namespace SSVM
