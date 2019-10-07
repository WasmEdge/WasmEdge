#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageStore : public EEI {
public:
  EEIStorageStore(VM::Environment &Env);
  EEIStorageStore() = delete;
  virtual ~EEIStorageStore() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM