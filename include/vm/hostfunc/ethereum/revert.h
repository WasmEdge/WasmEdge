#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIRevert : public EEI {
public:
  EEIRevert(VM::EVMEnvironment &Env);
  EEIRevert() = delete;
  virtual ~EEIRevert() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
