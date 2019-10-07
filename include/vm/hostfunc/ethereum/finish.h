#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIFinish : public EEI {
public:
  EEIFinish(VM::Environment &Env);
  EEIFinish() = delete;
  virtual ~EEIFinish() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM