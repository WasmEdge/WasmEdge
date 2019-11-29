// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "eei.h"

namespace SSVM {
namespace Executor {

class EEIReturnDataCopy : public EEI {
public:
  EEIReturnDataCopy(VM::EVMEnvironment &Env);
  EEIReturnDataCopy() = delete;
  virtual ~EEIReturnDataCopy() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst);
};

} // namespace Executor
} // namespace SSVM
