// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeConvInt8 : public ONNCWasm<ONNCRuntimeConvInt8> {
public:
  ONNCRuntimeConvInt8() : ONNCWasm("ONNC_RUNTIME_conv_int8") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InXOff, uint32_t InXNDim, uint32_t InXDimsOff,
               uint32_t InWOff, uint32_t InWNDim, uint32_t InWDimsOff,
               uint32_t InBOff, uint32_t InBNDim, uint32_t InBDimsOff,
               uint32_t OutYOff, uint32_t OutYNDim, uint32_t OutYDimsOff,
               uint32_t AutoPadOff, uint32_t DelationsOff, uint32_t DelationNum,
               uint32_t Group, uint32_t KernelShapeOff, uint32_t KernelShapeNum,
               uint32_t PadsOff, uint32_t PadsNum, uint32_t StridesOff,
               uint32_t StridesNum);
};

} // namespace Executor
} // namespace SSVM
