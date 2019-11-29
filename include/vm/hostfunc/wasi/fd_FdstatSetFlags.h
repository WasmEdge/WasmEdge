// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdFdstatSetFlags : public Wasi {
public:
  WasiFdFdstatSetFlags(VM::WasiEnvironment &Env);
  WasiFdFdstatSetFlags() = delete;
  virtual ~WasiFdFdstatSetFlags() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
