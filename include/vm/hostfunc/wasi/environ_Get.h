#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiEnvironGet : public Wasi {
public:
  WasiEnvironGet(VM::WasiEnvironment &Env);
  WasiEnvironGet() = delete;
  virtual ~WasiEnvironGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM