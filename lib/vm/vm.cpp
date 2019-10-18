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

VM::VM(Configure &InputConfig) : Config(InputConfig) {
  Configure::VMType Type = Config.getVMType();
  switch (Type) {
  case Configure::VMType::EWasm:
    Env = std::make_unique<EVMEnvironment>();
    break;
  case Configure::VMType::Wasi:
    Env = std::make_unique<WasiEnvironment>();
    break;
  default:
    Env.reset();
    break;
  }
}

ErrCode VM::setPath(const std::string &FilePath) {
  WasmPath = FilePath;
  return ErrCode::Success;
}

ErrCode VM::execute() {
  /// Prepare VM according to VM type.
  prepareVMHost();

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

ErrCode VM::getEnvironment(EVMEnvironment *&OutEnv) {
  if (Config.getVMType() == Configure::VMType::EWasm) {
    OutEnv = dynamic_cast<EVMEnvironment *>(Env.get());
    return ErrCode::Success;
  }
  return ErrCode::Failed;
}

ErrCode VM::getEnvironment(WasiEnvironment *&OutEnv) {
  if (Config.getVMType() == Configure::VMType::Wasi) {
    OutEnv = dynamic_cast<WasiEnvironment *>(Env.get());
    return ErrCode::Success;
  }
  return ErrCode::Failed;
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

ErrCode VM::prepareVMHost() {
  ErrCode Status = ErrCode::Success;
  Configure::VMType Type = Config.getVMType();
  if (Type == Configure::VMType::EWasm) {
    /// EWasm case, insert EEI host functions.
    EVMEnvironment *EVMEnv = dynamic_cast<EVMEnvironment *>(Env.get());
    auto FuncEEICallDataCopy =
        std::make_unique<Executor::EEICallDataCopy>(*EVMEnv);
    auto FuncEEICallStatic = std::make_unique<Executor::EEICallStatic>(*EVMEnv);
    auto FuncEEIFinish = std::make_unique<Executor::EEIFinish>(*EVMEnv);
    auto FuncEEIGetCallDataSize =
        std::make_unique<Executor::EEIGetCallDataSize>(*EVMEnv);
    auto FuncEEIGetCaller = std::make_unique<Executor::EEIGetCaller>(*EVMEnv);
    auto FuncEEIReturnDataCopy =
        std::make_unique<Executor::EEIReturnDataCopy>(*EVMEnv);
    auto FuncEEIRevert = std::make_unique<Executor::EEIRevert>(*EVMEnv);
    auto FuncEEIStorageLoad =
        std::make_unique<Executor::EEIStorageLoad>(*EVMEnv);
    auto FuncEEIStorageStore =
        std::make_unique<Executor::EEIStorageStore>(*EVMEnv);

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
      Status = setHostFunction(FuncEEIGetCallDataSize, "ethereum",
                               "getCallDataSize");
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
  } else if (Type == Configure::VMType::Wasi) {
    /// Wasi case, insert Wasi host functions.
  }
  return Status;
}

} // namespace VM
} // namespace SSVM