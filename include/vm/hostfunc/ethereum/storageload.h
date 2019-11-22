#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIStorageLoad : public EEI {
public:
  EEIStorageLoad(VM::EVMEnvironment &Env);
  EEIStorageLoad() = delete;
  virtual ~EEIStorageLoad() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
