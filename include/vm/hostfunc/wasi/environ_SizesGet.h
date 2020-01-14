// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "environ_sizes_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

} // namespace Executor
} // namespace SSVM
