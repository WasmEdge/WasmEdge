#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiProcExit : public Wasi {
public:
  WasiProcExit(VM::WasiEnvironment &Env);
  WasiProcExit() = delete;
  virtual ~WasiProcExit() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
