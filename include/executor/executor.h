#pragma once

#include "common.h"
#include "dbmgr.h"
#include "rtdatamgr.h"

namespace Executor {

class Executor {
public:
  Executor() = default;
  ~Executor() = default;
  ErrCode setRuntimeDataMgr(RTDataMgr &Mgr) { return ErrCode::Success; }
  ErrCode setArguments() { return ErrCode::Success; }
  ErrCode instantiate() { return ErrCode::Success; }
  ErrCode run() { return ErrCode::Success; }

private:
  /// Executor State
  enum class State : unsigned int {
    Inited,
    ASTSet,
    ArgsSet,
    RuntimeDataMgrSet,
    Instantiated,
    Finished
  };
};

} // namespace Executor
