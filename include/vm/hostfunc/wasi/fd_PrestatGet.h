#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatGet : public Wasi {
public:
  WasiFdPrestatGet(VM::WasiEnvironment &Env);
  WasiFdPrestatGet() = delete;
  virtual ~WasiFdPrestatGet() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM