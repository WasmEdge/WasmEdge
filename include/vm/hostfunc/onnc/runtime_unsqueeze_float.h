#pragma once

#include "onncwasm.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeUnsqueezeFloat : public ONNCWasm<ONNCRuntimeUnsqueezeFloat> {
public:
  ONNCRuntimeUnsqueezeFloat() : ONNCWasm("ONNC_RUNTIME_unsqueeze_float") {}
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InDataOff, uint32_t InDataNDim, uint32_t InDataDimsOff,
               uint32_t OutExpandedOff, uint32_t OutExpandedNDim,
               uint32_t OutExpandedDimsOff, uint32_t AxesOff, uint32_t AxesNum);
};

} // namespace Executor
} // namespace SSVM
