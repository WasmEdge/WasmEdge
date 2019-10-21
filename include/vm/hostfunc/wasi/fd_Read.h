#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdRead : public Wasi {
public:
  WasiFdRead(VM::WasiEnvironment &Env);
  WasiFdRead() = delete;
  virtual ~WasiFdRead() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM