#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIRevert : public EEI {
public:
  EEIRevert(VM::EVMEnvironment &Env);
  EEIRevert() = delete;
  virtual ~EEIRevert() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM