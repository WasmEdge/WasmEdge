// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatDirName : public Wasi {
public:
  WasiFdPrestatDirName(VM::WasiEnvironment &Env);
  WasiFdPrestatDirName() = delete;
  virtual ~WasiFdPrestatDirName() = default;

  virtual ErrCode run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                      std::vector<Value> &Res, StoreManager &Store,
                      Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
