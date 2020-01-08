// SPDX-License-Identifier: Apache-2.0
#include "vm/vm.h"
#include "executor/hostfunc.h"
#include "loader/loader.h"
#include "vm/result.h"

/// EEI Functions
#include "vm/hostfunc/ethereum/callDataCopy.h"
#include "vm/hostfunc/ethereum/callStatic.h"
#include "vm/hostfunc/ethereum/finish.h"
#include "vm/hostfunc/ethereum/getCallDataSize.h"
#include "vm/hostfunc/ethereum/getCallValue.h"
#include "vm/hostfunc/ethereum/getCaller.h"
#include "vm/hostfunc/ethereum/getGasLeft.h"
#include "vm/hostfunc/ethereum/returnDataCopy.h"
#include "vm/hostfunc/ethereum/revert.h"
#include "vm/hostfunc/ethereum/storageLoad.h"
#include "vm/hostfunc/ethereum/storageStore.h"

/// Wasi Functions
#include "vm/hostfunc/wasi/args_Get.h"
#include "vm/hostfunc/wasi/args_SizesGet.h"
#include "vm/hostfunc/wasi/environ_Get.h"
#include "vm/hostfunc/wasi/environ_SizesGet.h"
#include "vm/hostfunc/wasi/fd_Close.h"
#include "vm/hostfunc/wasi/fd_FdstatGet.h"
#include "vm/hostfunc/wasi/fd_FdstatSetFlags.h"
#include "vm/hostfunc/wasi/fd_PrestatDirName.h"
#include "vm/hostfunc/wasi/fd_PrestatGet.h"
#include "vm/hostfunc/wasi/fd_Read.h"
#include "vm/hostfunc/wasi/fd_Seek.h"
#include "vm/hostfunc/wasi/fd_Write.h"
#include "vm/hostfunc/wasi/path_Open.h"
#include "vm/hostfunc/wasi/proc_Exit.h"

#ifdef ONNC_WASM
/// ONNC Runtime Functions
#include "vm/hostfunc/onnc/runtime_add_float.h"
#include "vm/hostfunc/onnc/runtime_add_int8.h"
#include "vm/hostfunc/onnc/runtime_averagepool_float.h"
#include "vm/hostfunc/onnc/runtime_batchnormalization_float.h"
#include "vm/hostfunc/onnc/runtime_batchnormalization_int8.h"
#include "vm/hostfunc/onnc/runtime_concat_float.h"
#include "vm/hostfunc/onnc/runtime_conv_float.h"
#include "vm/hostfunc/onnc/runtime_conv_int8.h"
#include "vm/hostfunc/onnc/runtime_gemm_float.h"
#include "vm/hostfunc/onnc/runtime_globalaveragepool_float.h"
#include "vm/hostfunc/onnc/runtime_lrn_float.h"
#include "vm/hostfunc/onnc/runtime_maxpool_float.h"
#include "vm/hostfunc/onnc/runtime_maxpool_int8.h"
#include "vm/hostfunc/onnc/runtime_mul_float.h"
#include "vm/hostfunc/onnc/runtime_mul_int8.h"
#include "vm/hostfunc/onnc/runtime_relu_float.h"
#include "vm/hostfunc/onnc/runtime_relu_int8.h"
#include "vm/hostfunc/onnc/runtime_reshape_float.h"
#include "vm/hostfunc/onnc/runtime_softmax_float.h"
#include "vm/hostfunc/onnc/runtime_sum_float.h"
#include "vm/hostfunc/onnc/runtime_transpose_float.h"
#include "vm/hostfunc/onnc/runtime_unsqueeze_float.h"
#endif

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
  WasmCode.clear();
  return ErrCode::Success;
}

ErrCode VM::setCode(const std::vector<uint8_t> &Code) {
  WasmPath = "";
  WasmCode = Code;
  return ErrCode::Success;
}

ErrCode VM::execute(const std::string &FuncName) {
  /// Prepare VM according to VM type.
  Rets.clear();
  prepareVMHost();

  if (FuncName == "") {
    ExecutorEngine.setStartFuncName(Config.getStartFuncName());
  } else {
    ExecutorEngine.setStartFuncName(FuncName);
  }

  /// Run code.
  ErrCode Status = runLoader();
  if (Status == ErrCode::Success) {
    Status = runValidator();
  }
  if (Status == ErrCode::Success) {
    Status = runExecutor();
    ExecutorEngine.statistics();
  }
  VMResult.setErrCode(static_cast<unsigned int>(Status));

  /// Clear loader and executor engine.
  LoaderEngine.reset();
  ValidatorEngine.reset();
  ExecutorEngine.reset();
  Mod.reset();
  Args.clear();
  InVMStore = nullptr;
  OutVMStore = nullptr;
  OutAlloc = nullptr;
  return Status;
}

ErrCode VM::execute() { return execute(""); }

ErrCode VM::runLoader() {
  Loader::ErrCode LoaderStatus = Loader::ErrCode::Success;
  VMResult.setStage(Result::Stage::Loader);

  if (WasmPath == "") {
    LoaderStatus = LoaderEngine.setCode(WasmCode);
  } else {
    LoaderStatus = LoaderEngine.setPath(WasmPath);
  }
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

ErrCode VM::runValidator() {
  Validator::ErrCode ValidatorStatus = Validator::ErrCode::Success;
  VMResult.setStage(Result::Stage::Validator);

  ValidatorStatus = ValidatorEngine.validate(Mod);

  if (ValidatorStatus != Validator::ErrCode::Success) {
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

  if (InVMStore != nullptr) {
    ExecutorStatus = ExecutorEngine.restore(*InVMStore);
    if (detail::testAndSetError(ExecutorStatus, VMResult)) {
      return ErrCode::Failed;
    }
  }

  ExecutorStatus = ExecutorEngine.run();
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    if (ExecutorStatus == Executor::ErrCode::Revert) {
      VMResult.setState(Result::State::Revert);
    }
    return ErrCode::Failed;
  }

  if (OutVMStore != nullptr) {
    ExecutorStatus = ExecutorEngine.snapshot(*OutVMStore, *OutAlloc);
    if (detail::testAndSetError(ExecutorStatus, VMResult)) {
      return ErrCode::Failed;
    }
  }

  ExecutorStatus = ExecutorEngine.getRets(Rets);
  if (detail::testAndSetError(ExecutorStatus, VMResult)) {
    return ErrCode::Failed;
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  VMResult.setState(Result::State::Commit);
  return ErrCode::Success;
}

ErrCode VM::prepareVMHost() {
  ErrCode Status = ErrCode::Success;
  if (Config.hasVMType(Configure::VMType::Ewasm)) {
    /// Ewasm case, insert EEI host functions.
    EVMEnvironment *EVMEnv =
        getEnvironment<EVMEnvironment>(Configure::VMType::Ewasm);

    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallDataCopy>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallStatic>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEIFinish>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetCallDataSize>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetCaller>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetCallValue>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetGasLeft>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIReturnDataCopy>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEIRevert>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIStorageLoad>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIStorageStore>(*EVMEnv));
    }
  }
  if (Config.hasVMType(Configure::VMType::Wasi)) {
    /// Wasi case, insert Wasi host functions.
    WasiEnvironment *WasiEnv =
        getEnvironment<WasiEnvironment>(Configure::VMType::Wasi);

    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiArgsGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiArgsSizesGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiEnvironGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiEnvironSizesGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiFdClose>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiFdFdstatGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiFdFdstatSetFlags>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiFdPrestatDirName>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::WasiFdPrestatGet>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiFdRead>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiFdSeek>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiFdWrite>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiPathOpen>(*WasiEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::WasiProcExit>(*WasiEnv));
    }
  }

#ifdef ONNC_WASM
  if (Config.hasVMType(Configure::VMType::ONNC)) {
    /// Found ONNC library, insert ONNC host functions.
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeAddFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeAveragepoolFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeBatchnormalizationFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeConcatFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeConvFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeGemmFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeGlobalaveragepoolFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeLrnFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeMaxpoolFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeMulFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeReluFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeReshapeFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeSoftmaxFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeSumFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeTransposeFloat>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeUnsqueezeFloat>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeAddInt8>());
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::ONNCRuntimeBatchnormalizationInt8>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeConvInt8>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeMaxpoolInt8>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeMulInt8>());
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::ONNCRuntimeReluInt8>());
    }
  }
#endif

  return Status;
}

} // namespace VM
} // namespace SSVM
