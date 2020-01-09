// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeSoftmaxFloat : public HostFunction<ONNCRuntimeSoftmaxFloat> {
public:
  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t RuntimeContextOff,
               uint32_t InOff, uint32_t InNDim, uint32_t InDimsOff,
               uint32_t OutOff, uint32_t OutNDim, uint32_t OutDimsOff,
               uint32_t Axis);
};

} // namespace Executor
} // namespace SSVM
