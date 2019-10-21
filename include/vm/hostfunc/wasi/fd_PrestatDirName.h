#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdPrestatDirName : public Wasi {
public:
  WasiFdPrestatDirName(VM::WasiEnvironment &Env);
  WasiFdPrestatDirName() = delete;
  virtual ~WasiFdPrestatDirName() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM