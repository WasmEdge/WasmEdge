#include "regression.h"

bool RegressionTester::setExpectDBMgr(DBMgr &Mgr) {
  ExpectDB = Mgr;
  return true;
}

bool RegressionTester::setExpectResult(SSVM::Result &Res) {
  ExpectResult = Res;
  return true;
}

bool RegressionTester::setFilePath(const std::string &FilePath) {
  WasmPath = FilePath;
  return true;
}

bool RegressionTester::setDBMgr(DBMgr &Mgr) {
  InitDB = Mgr;
  return true;
}

bool RegressionTester::runLoader() {
  LoaderEngine.setPath(WasmPath);
  LoaderEngine.parseModule();
  LoaderEngine.validateModule();
  LoaderEngine.getModule(OutModule);
  return true;
}

bool RegressionTester::runExecutor() {
  RTDataMgr RTMgr;
  ExecutorEngine.setRuntimeDataMgr(RTMgr);
  ExecutorEngine.setDBMgr(InitDB);
  ExecutorEngine.setArguments();
  ExecutorEngine.instantiate();
  ExecutorEngine.run();
  return true;
}

bool RegressionTester::checkResult() { return true; }
