#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsSizesGet : public Wasi {
public:
  WasiArgsSizesGet(VM::WasiEnvironment &Env);
  WasiArgsSizesGet() = delete;
  virtual ~WasiArgsSizesGet() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
