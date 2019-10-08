#include "vm/vm.h"
#include "executor/hostfunc.h"
#include "loader/loader.h"
#include "vm/hostfunc/ethereum/calldatacopy.h"
#include "vm/hostfunc/ethereum/callstatic.h"
#include "vm/hostfunc/ethereum/finish.h"
#include "vm/hostfunc/ethereum/getcalldatasize.h"
#include "vm/hostfunc/ethereum/getcaller.h"
#include "vm/hostfunc/ethereum/returndatacopy.h"
#include "vm/hostfunc/ethereum/revert.h"
#include "vm/hostfunc/ethereum/storageload.h"
#include "vm/hostfunc/ethereum/storagestore.h"
#include "vm/result.h"
#include <stdio.h>

namespace SSVM {
namespace VM {

namespace detail {

template <typename T> bool testAndSetError(T Status, Result &VMResult) {
  if (Status != T::Success) {
    VMResult.setErrCode(static_cast<unsigned int>(Status));
    return true;
  }
  return false;
}

} // namespace detail

ErrCode VM::setPath(const std::string &FilePath) {
  WasmPath = FilePath;
  return ErrCode::Success;
}

ErrCode VM::setCallData(std::vector<unsigned char> &Data) {
  std::vector<unsigned char> &CallData = Env.getCallData();
  CallData = Data;
  return ErrCode::Success;
}

ErrCode VM::execute() {
  /// Insert EEI functions.
  insertEEI();

  /// Run code.
  ErrCode Status = runLoader();
  if (Status == ErrCode::Success) {
    Status = runExecutor();
  }

  /// Clear loader and executor engine.
  LoaderEngine.reset();
  ExecutorEngine.reset();
  Mod.reset();
  Args.clear();
  return Status;
}

ErrCode VM::runLoader() {
  Loader::ErrCode LoaderStatus = Loader::ErrCode::Success;
  VMResult.setStage(Result::Stage::Loader);

  LoaderStatus = LoaderEngine.setPath(WasmPath);
  if (detail::testAndSetError(LoaderStatus, VMResult)) {
    return ErrCode::Failed;
  }
  LoaderStatus = LoaderEngine.parseModule();
  if (detail::testAndSetError(LoaderStatus, VMResult)) {
    return ErrCode::Failed;
  }
  LoaderStatus = LoaderEngine.validateModule();
  if (detail::testAndSetError(LoaderStatus, VMResult)) {
    return ErrCode::Failed;
  }
  LoaderStatus = LoaderEngine.getModule(Mod);
  if (detail::testAndSetError(LoaderStatus, VMResult)) {
    return ErrCode::Failed;
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::runExecutor() {
  Executor::ErrCode ExecutorStatus = Executor::ErrCode::Success;
  VMResult.setStage(Result::Stage::Executor);

  ExecutorStatus = ExecutorEngine.setModule(Mod);
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.instantiate();
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.setArgs(Args);
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.run();
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    return ErrCode::Failed;
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::insertEEI() {
  auto FuncEEICallDataCopy =
      castHostFunc(std::make_unique<Executor::EEICallDataCopy>(Env));
  auto FuncEEICallStatic =
      castHostFunc(std::make_unique<Executor::EEICallStatic>(Env));
  auto FuncEEIFinish = castHostFunc(std::make_unique<Executor::EEIFinish>(Env));
  auto FuncEEIGetCallDataSize =
      castHostFunc(std::make_unique<Executor::EEIGetCallDataSize>(Env));
  auto FuncEEIGetCaller =
      castHostFunc(std::make_unique<Executor::EEIGetCaller>(Env));
  auto FuncEEIReturnDataCopy =
      castHostFunc(std::make_unique<Executor::EEIReturnDataCopy>(Env));
  auto FuncEEIRevert = castHostFunc(std::make_unique<Executor::EEIRevert>(Env));
  auto FuncEEIStorageLoad =
      castHostFunc(std::make_unique<Executor::EEIStorageLoad>(Env));
  auto FuncEEIStorageStore =
      castHostFunc(std::make_unique<Executor::EEIStorageStore>(Env));

  ErrCode Status = ErrCode::Success;
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEICallDataCopy, "ethereum", "callDataCopy");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEICallStatic, "ethereum", "callStatic");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEIFinish, "ethereum", "finish");
  }
  if (Status == ErrCode::Success) {
    Status =
        setHostFunction(FuncEEIGetCallDataSize, "ethereum", "getCallDataSize");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEIGetCaller, "ethereum", "getCaller");
  }
  if (Status == ErrCode::Success) {
    Status =
        setHostFunction(FuncEEIReturnDataCopy, "ethereum", "returnDataCopy");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEIRevert, "ethereum", "revert");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEIStorageLoad, "ethereum", "storageLoad");
  }
  if (Status == ErrCode::Success) {
    Status = setHostFunction(FuncEEIStorageStore, "ethereum", "storageStore");
  }
  return Status;
}

} // namespace VM
} // namespace SSVM