#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiPathOpen : public Wasi {
public:
  WasiPathOpen(VM::WasiEnvironment &Env);
  WasiPathOpen() = delete;
  virtual ~WasiPathOpen() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
