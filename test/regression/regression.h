#pragma once

#include "executor/execmgr.h"
#include "loader/loadmgr.h"
#include "result.h"

class RegressionMgr {
public:
  RegressionMgr() = default;
  ~RegressionMgr() = default;

  /// Functions to set expect results.
  bool setExpectDBMgr(DBMgr &Mgr);
  bool setExpectResult(Result &Res);

  /// Functions to set inputs.
  bool setFilePath(const std::string &FilePath);
  bool setDBMgr(DBMgr &Mgr);

  /// Functions for running.
  bool runLoader();
  bool runExecutor();
  bool checkResult();

private:
  Loader::LoadMgr LoaderEngine;
  Executor::ExecMgr ExecutorEngine;
  std::unique_ptr<AST::Module> OutModule;
  std::string WasmPath;
  DBMgr InitDB;
  DBMgr ExpectDB;
  Result InitResult;
  Result ExpectResult;
};