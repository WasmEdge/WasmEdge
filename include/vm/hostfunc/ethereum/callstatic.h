#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEICallStatic : public EEI {
public:
  EEICallStatic(VM::EVMEnvironment &Env);
  EEICallStatic() = delete;
  virtual ~EEICallStatic() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM