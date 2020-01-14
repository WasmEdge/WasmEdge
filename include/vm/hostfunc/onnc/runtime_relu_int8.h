// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeReluInt8 : public ONNCWasm<ONNCRuntimeReluInt8> {
public:
  ONNCRuntimeReluInt8() : ONNCWasm("ONNC_RUNTIME_relu_int8") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t OutYOff, uint32_t OutYNDim, uint32_t OutYDimsOff);
};

} // namespace Executor
} // namespace SSVM
