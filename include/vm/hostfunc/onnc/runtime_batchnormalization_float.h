#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeBatchnormalizationFloat : public HostFunction {
public:
  ONNCRuntimeBatchnormalizationFloat();
  virtual ~ONNCRuntimeBatchnormalizationFloat() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
