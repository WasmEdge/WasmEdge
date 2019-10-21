#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiArgsSizesGet : public Wasi {
public:
  WasiArgsSizesGet(VM::WasiEnvironment &Env);
  WasiArgsSizesGet() = delete;
  virtual ~WasiArgsSizesGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM