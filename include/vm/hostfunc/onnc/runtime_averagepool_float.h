#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeAveragepoolFloat : public HostFunction {
public:
  ONNCRuntimeAveragepoolFloat();
  virtual ~ONNCRuntimeAveragepoolFloat() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
