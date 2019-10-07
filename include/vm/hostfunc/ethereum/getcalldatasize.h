#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCallDataSize : public EEI {
public:
  EEIGetCallDataSize(VM::Environment &Env);
  EEIGetCallDataSize() = delete;
  virtual ~EEIGetCallDataSize() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM