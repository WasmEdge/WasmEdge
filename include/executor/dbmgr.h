//===-- ssvm/test/regression/dbmgr.h - database manager class definition --===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the data base manager interface.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include <memory>

namespace SSVM {

class DBMgr {
public:
  DBMgr() = default;
  ~DBMgr() = default;
  // Return a cloned DBMgr
  Executor::ErrCode snapshot(std::unique_ptr<DBMgr> &OutDBMgr) {
    OutDBMgr = std::make_unique<DBMgr>();
    return Executor::ErrCode::Success;
  }
  bool equal(DBMgr &Other) { return true; }
  Executor::ErrCode replaceDBwith(DBMgr &Other) {
    return Executor::ErrCode::Success;
  }
  Executor::ErrCode commit() { return Executor::ErrCode::Success; }
  Executor::ErrCode revert() { return Executor::ErrCode::Success; }
};

} // namespace SSVM