#pragma once

#include "wasi.h"

namespace SSVM {
namespace Executor {

class WasiFdSeek : public Wasi {
public:
  WasiFdSeek(VM::WasiEnvironment &Env);
  WasiFdSeek() = delete;
  virtual ~WasiFdSeek() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM