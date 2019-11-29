// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

class ONNCRuntimeConcatFloat : public HostFunction {
public:
  ONNCRuntimeConcatFloat();
  virtual ~ONNCRuntimeConcatFloat() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
