// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeReshapeFloat : public ONNCWasm<ONNCRuntimeReshapeFloat> {
public:
  ONNCRuntimeReshapeFloat() : ONNCWasm("ONNC_RUNTIME_reshape_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
               uint32_t InShapeOff, uint32_t InShapeNDim,
               uint32_t InShapeDimsOff, uint32_t OutReshapedOff,
               uint32_t OutReshapedNDim, uint32_t OutReshapedDimsOff);
};

} // namespace Executor
} // namespace SSVM
