#include "regression.h"

bool RegressionMgr::setExpectDBMgr(DBMgr &Mgr) {
  ExpectDB = Mgr;
  return true;
}

bool RegressionMgr::setExpectResult(Result &Res) {
  ExpectResult = Res;
  return true;
}

bool RegressionMgr::setFilePath(const std::string &FilePath) {
  WasmPath = FilePath;
  return true;
}

bool RegressionMgr::setDBMgr(DBMgr &Mgr) {
  InitDB = Mgr;
  return true;
}

bool RegressionMgr::runLoader() {
  LoaderEngine.setPath(WasmPath);
  LoaderEngine.parseModule();
  LoaderEngine.validateModule();
  LoaderEngine.getModule(OutModule);
  return true;
}

bool RegressionMgr::runExecutor() {
  RTDataMgr RTMgr;
  ExecutorEngine.setDBMgr(InitDB);
  ExecutorEngine.setModule(OutModule);
  ExecutorEngine.setArguments();
  ExecutorEngine.setRuntimeDataMgr(RTMgr);
  ExecutorEngine.instantiate();
  ExecutorEngine.run();
  return true;
}

bool RegressionMgr::checkResult() { return true; }