#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdWrite : public Wasi {
public:
  WasiFdWrite(VM::WasiEnvironment &Env);
  WasiFdWrite() = delete;
  virtual ~WasiFdWrite() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM