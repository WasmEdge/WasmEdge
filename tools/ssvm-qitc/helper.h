// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/common.h"
#include "executor/hostfunc.h"
#include "executor/worker/util.h"

#include <iostream>
#include <string>

namespace SSVM {
namespace Executor {

class QITCTimer : public HostFunction {
public:
  QITCTimer() = default;
  virtual ~QITCTimer() = default;

  virtual ErrCode run(std::vector<Value> &Args, std::vector<Value> &Res,
                      StoreManager &Store, Instance::ModuleInstance *ModInst) {
    return ErrCode::Success;
  }
};

} // namespace Executor
} // namespace SSVM
