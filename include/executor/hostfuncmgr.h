//===-- ssvm/executor/hostfuncmgr.h - host function manager class ---------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of host function manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "hostfunc.h"

namespace SSVM {
namespace Executor {

class HostFunctionManager {
public:
  HostFunctionManager() = default;
  ~HostFunctionManager() = default;

  ErrCode insertHostFunction(std::unique_ptr<HostFunction> &Func,
                             unsigned int &NewId) {
    NewId = HostFuncs.size();
    HostFuncs.push_back(std::move(Func));
    return ErrCode::Success;
  }

  ErrCode getHostFunction(unsigned int Addr, HostFunction *&Func) {
    if (Addr >= HostFuncs.size()) {
      return ErrCode::WrongInstanceAddress;
    }
    Func = HostFuncs[Addr].get();
    return ErrCode::Success;
  }

private:
  std::vector<std::unique_ptr<HostFunction>> HostFuncs;
};

} // namespace Executor
} // namespace SSVM