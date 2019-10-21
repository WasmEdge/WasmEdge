#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiEnvironSizesGet : public Wasi {
public:
  WasiEnvironSizesGet(VM::WasiEnvironment &Env);
  WasiEnvironSizesGet() = delete;
  virtual ~WasiEnvironSizesGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM