#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdFdstatGet : public Wasi {
public:
  WasiFdFdstatGet(VM::WasiEnvironment &Env);
  WasiFdFdstatGet() = delete;
  virtual ~WasiFdFdstatGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM