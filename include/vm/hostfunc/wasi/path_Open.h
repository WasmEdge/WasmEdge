// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(VM::WasiEnvironment &HostEnv) : Wasi(HostEnv, "path_open") {}

  ErrCode body(VM::EnvironmentManager &EnvMgr,
               Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
               uint32_t PathLen, uint32_t OFlags, uint64_t FsRightsBase,
               uint64_t FsRightsInheriting, uint32_t FsFlags, uint32_t FdPtr);
};

} // namespace Executor
} // namespace SSVM
