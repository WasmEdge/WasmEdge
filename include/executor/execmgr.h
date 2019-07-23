#pragma once

#include "common.h"
#include "dbmgr.h"
#include "loader/module.h"
#include "rtdatamgr.h"

namespace Executor {

class ExecMgr {
public:
  ExecMgr() = default;
  ~ExecMgr() = default;
  ErrCode setModule(std::unique_ptr<AST::Module> &NewMod) {
    return ErrCode::Success;
  }
  ErrCode setDBMgr(DBMgr &Mgr) { return ErrCode::Success; }
  ErrCode setArguments() { return ErrCode::Success; }
  ErrCode setRuntimeDataMgr(RTDataMgr &Mgr) { return ErrCode::Success; }
  ErrCode instantiate() { return ErrCode::Success; }
  ErrCode run() { return ErrCode::Success; }

private:
  /// Executor State
  enum class State : unsigned int {
    Inited,
    ASTSet,
    DBSet,
    ArgsSet,
    RuntimeDataMgrSet,
    Instantiated,
    Finished
  };

  std::unique_ptr<AST::Module> Mod;
};

} // namespace Executor
