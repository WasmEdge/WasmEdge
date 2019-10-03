#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIReturnDataCopy : public EEI {
public:
  EEIReturnDataCopy(VM::Environment &Env) : EEI(Env) {}
  EEIReturnDataCopy() = delete;
  virtual ~EEIReturnDataCopy() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM