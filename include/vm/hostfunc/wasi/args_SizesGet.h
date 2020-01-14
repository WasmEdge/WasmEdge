// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(VM::WasiEnvironment &HostEnv)
      : Wasi(HostEnv, "args_sizes_get") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

} // namespace Executor
} // namespace SSVM
