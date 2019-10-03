#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIGetCaller : public EEI {
public:
  EEIGetCaller(VM::Environment &Env) : EEI(Env) {}
  EEIGetCaller() = delete;
  virtual ~EEIGetCaller() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM