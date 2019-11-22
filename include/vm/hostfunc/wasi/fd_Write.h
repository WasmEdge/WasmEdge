#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdWrite : public Wasi {
public:
  WasiFdWrite(VM::WasiEnvironment &Env);
  WasiFdWrite() = delete;
  virtual ~WasiFdWrite() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
