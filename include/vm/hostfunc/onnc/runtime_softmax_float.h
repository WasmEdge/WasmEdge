#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeSoftmaxFloat : public HostFunction {
public:
  ONNCRuntimeSoftmaxFloat();
  virtual ~ONNCRuntimeSoftmaxFloat() = default;

  virtual ErrCode run(std::vector<std::unique_ptr<ValueEntry>> &Args,
                      std::vector<std::unique_ptr<ValueEntry>> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM