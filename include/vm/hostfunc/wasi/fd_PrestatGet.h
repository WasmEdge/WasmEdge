// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatGet : public Wasi {
public:
  WasiFdPrestatGet(VM::WasiEnvironment &Env);
  WasiFdPrestatGet() = delete;
  virtual ~WasiFdPrestatGet() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
