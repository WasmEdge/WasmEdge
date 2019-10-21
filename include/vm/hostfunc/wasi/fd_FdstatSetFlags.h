#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdFdstatSetFlags : public Wasi {
public:
  WasiFdFdstatSetFlags(VM::WasiEnvironment &Env);
  WasiFdFdstatSetFlags() = delete;
  virtual ~WasiFdFdstatSetFlags() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM