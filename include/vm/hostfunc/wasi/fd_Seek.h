// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo, int32_t Fd,
               int32_t Offset, uint32_t Whence, uint32_t NewOffsetPtr);
};

} // namespace Executor
} // namespace SSVM
