#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiEnvironSizesGet : public Wasi {
public:
  WasiEnvironSizesGet(VM::WasiEnvironment &Env);
  WasiEnvironSizesGet() = delete;
  virtual ~WasiEnvironSizesGet() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
