// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdRead : public Wasi {
public:
  WasiFdRead(VM::WasiEnvironment &Env);
  WasiFdRead() = delete;
  virtual ~WasiFdRead() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
