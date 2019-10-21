#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdClose : public Wasi {
public:
  WasiFdClose(VM::WasiEnvironment &Env);
  WasiFdClose() = delete;
  virtual ~WasiFdClose() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM