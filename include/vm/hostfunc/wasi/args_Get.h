#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsGet : public Wasi {
public:
  WasiArgsGet(VM::WasiEnvironment &Env);
  WasiArgsGet() = delete;
  virtual ~WasiArgsGet() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
