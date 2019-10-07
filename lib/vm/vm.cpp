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

VM::VM(Environment &InputEnv) : Env(InputEnv) {
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
  ExecutorEngine.setHostFunction(FuncEEICallDataCopy, "ethereum",
                                 "callDataCopy");
  ExecutorEngine.setHostFunction(FuncEEICallStatic, "ethereum", "callStatic");
  ExecutorEngine.setHostFunction(FuncEEIFinish, "ethereum", "finish");
  ExecutorEngine.setHostFunction(FuncEEIGetCallDataSize, "ethereum",
                                 "getCallDataSize");
  ExecutorEngine.setHostFunction(FuncEEIGetCaller, "ethereum", "getCaller");
  ExecutorEngine.setHostFunction(FuncEEIReturnDataCopy, "ethereum",
                                 "returnDataCopy");
  ExecutorEngine.setHostFunction(FuncEEIRevert, "ethereum", "revert");
  ExecutorEngine.setHostFunction(FuncEEIStorageLoad, "ethereum", "storageLoad");
  ExecutorEngine.setHostFunction(FuncEEIStorageStore, "ethereum",
                                 "storageStore");
}

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
  ErrCode Status = ErrCode::Success;
  if ((Status = runLoader()) != ErrCode::Success) {
    printf(" !!! load error \n");
    return Status;
  }
  if ((Status = runExecutor()) != ErrCode::Success) {

    printf(" !!! exec error \n");
    return Status;
  }
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
    printf(" !!! setModule error \n");
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.instantiate();
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    printf(" !!! instantiate error \n");
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.setArgs(Args);
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    printf(" !!! setArgs error \n");
    return ErrCode::Failed;
  }

  ExecutorStatus = ExecutorEngine.run();
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    printf(" !!! run error \n");
    return ErrCode::Failed;
  }

  if (VMResult.hasError()) {
    printf(" !!! result hasError error \n");
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

} // namespace VM
} // namespace SSVM