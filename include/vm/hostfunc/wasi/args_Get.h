#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsGet : public Wasi {
public:
  WasiArgsGet(VM::WasiEnvironment &Env);
  WasiArgsGet() = delete;
  virtual ~WasiArgsGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM