#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiPathOpen : public Wasi {
public:
  WasiPathOpen(VM::WasiEnvironment &Env);
  WasiPathOpen() = delete;
  virtual ~WasiPathOpen() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM