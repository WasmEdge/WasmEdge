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
    auto FuncEEICallDataCopy =
        std::make_unique<Executor::EEICallDataCopy>(*EVMEnv);
    auto FuncEEICallStatic = std::make_unique<Executor::EEICallStatic>(*EVMEnv);
    auto FuncEEIFinish = std::make_unique<Executor::EEIFinish>(*EVMEnv);
    auto FuncEEIGetCallDataSize =
        std::make_unique<Executor::EEIGetCallDataSize>(*EVMEnv);
    auto FuncEEIGetCaller = std::make_unique<Executor::EEIGetCaller>(*EVMEnv);
    auto FuncEEIGetCallValue =
        std::make_unique<Executor::EEIGetCallValue>(*EVMEnv);
    auto FuncEEIGetGasLeft = std::make_unique<Executor::EEIGetGasLeft>(*EVMEnv);
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
      Status = setHostFunction(FuncEEIGetCallValue, "ethereum", "getCallValue");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncEEIGetGasLeft, "ethereum", "getGasLeft");
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
  }
  if (Config.hasVMType(Configure::VMType::Wasi)) {
    /// Wasi case, insert Wasi host functions.
    WasiEnvironment *WasiEnv =
        getEnvironment<WasiEnvironment>(Configure::VMType::Wasi);
    auto FuncWasiArgsGet = std::make_unique<Executor::WasiArgsGet>(*WasiEnv);
    auto FuncWasiArgsSizesGet =
        std::make_unique<Executor::WasiArgsSizesGet>(*WasiEnv);
    auto FuncWasiEnvironGet =
        std::make_unique<Executor::WasiEnvironGet>(*WasiEnv);
    auto FuncWasiEnvironSizesGet =
        std::make_unique<Executor::WasiEnvironSizesGet>(*WasiEnv);
    auto FuncWasiFdClose = std::make_unique<Executor::WasiFdClose>(*WasiEnv);
    auto FuncWasiFdFdstatGet =
        std::make_unique<Executor::WasiFdFdstatGet>(*WasiEnv);
    auto FuncWasiFdFdstatSetFlags =
        std::make_unique<Executor::WasiFdFdstatSetFlags>(*WasiEnv);
    auto FuncWasiFdPrestatDirName =
        std::make_unique<Executor::WasiFdPrestatDirName>(*WasiEnv);
    auto FuncWasiFdPrestatGet =
        std::make_unique<Executor::WasiFdPrestatGet>(*WasiEnv);
    auto FuncWasiFdRead = std::make_unique<Executor::WasiFdRead>(*WasiEnv);
    auto FuncWasiFdSeek = std::make_unique<Executor::WasiFdSeek>(*WasiEnv);
    auto FuncWasiFdWrite = std::make_unique<Executor::WasiFdWrite>(*WasiEnv);
    auto FuncWasiPathOpen = std::make_unique<Executor::WasiPathOpen>(*WasiEnv);
    auto FuncWasiProcExit = std::make_unique<Executor::WasiProcExit>(*WasiEnv);

    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiArgsGet, "wasi_unstable", "args_get");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiArgsSizesGet, "wasi_unstable",
                               "args_sizes_get");
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(FuncWasiEnvironGet, "wasi_unstable", "environ_get");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiEnvironSizesGet, "wasi_unstable",
                               "environ_sizes_get");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdClose, "wasi_unstable", "fd_close");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdFdstatGet, "wasi_unstable",
                               "fd_fdstat_get");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdFdstatSetFlags, "wasi_unstable",
                               "fd_fdstat_set_flags");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdPrestatDirName, "wasi_unstable",
                               "fd_prestat_dir_name");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdPrestatGet, "wasi_unstable",
                               "fd_prestat_get");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdRead, "wasi_unstable", "fd_read");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdSeek, "wasi_unstable", "fd_seek");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiFdWrite, "wasi_unstable", "fd_write");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiPathOpen, "wasi_unstable", "path_open");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncWasiProcExit, "wasi_unstable", "proc_exit");
    }
  }

#ifdef ONNC_WASM
  if (Config.hasVMType(Configure::VMType::ONNC)) {
    /// Found ONNC library, insert ONNC host functions.
    auto FuncONNCRuntimeAddFloat =
        std::make_unique<Executor::ONNCRuntimeAddFloat>();
    auto FuncONNCRuntimeAveragepoolFloat =
        std::make_unique<Executor::ONNCRuntimeAveragepoolFloat>();
    auto FuncONNCRuntimeBatchnormalizationFloat =
        std::make_unique<Executor::ONNCRuntimeBatchnormalizationFloat>();
    auto FuncONNCRuntimeConcatFloat =
        std::make_unique<Executor::ONNCRuntimeConcatFloat>();
    auto FuncONNCRuntimeConvFloat =
        std::make_unique<Executor::ONNCRuntimeConvFloat>();
    auto FuncONNCRuntimeGemmFloat =
        std::make_unique<Executor::ONNCRuntimeGemmFloat>();
    auto FuncONNCRuntimeGlobalaveragepoolFloat =
        std::make_unique<Executor::ONNCRuntimeGlobalaveragepoolFloat>();
    auto FuncONNCRuntimeLrnFloat =
        std::make_unique<Executor::ONNCRuntimeLrnFloat>();
    auto FuncONNCRuntimeMaxpoolFloat =
        std::make_unique<Executor::ONNCRuntimeMaxpoolFloat>();
    auto FuncONNCRuntimeMulFloat =
        std::make_unique<Executor::ONNCRuntimeMulFloat>();
    auto FuncONNCRuntimeReluFloat =
        std::make_unique<Executor::ONNCRuntimeReluFloat>();
    auto FuncONNCRuntimeReshapeFloat =
        std::make_unique<Executor::ONNCRuntimeReshapeFloat>();
    auto FuncONNCRuntimeSoftmaxFloat =
        std::make_unique<Executor::ONNCRuntimeSoftmaxFloat>();
    auto FuncONNCRuntimeSumFloat =
        std::make_unique<Executor::ONNCRuntimeSumFloat>();
    auto FuncONNCRuntimeTransposeFloat =
        std::make_unique<Executor::ONNCRuntimeTransposeFloat>();
    auto FuncONNCRuntimeUnsqueezeFloat =
        std::make_unique<Executor::ONNCRuntimeUnsqueezeFloat>();
    auto FuncONNCRuntimeAddInt8 =
        std::make_unique<Executor::ONNCRuntimeAddInt8>();
    auto FuncONNCRuntimeBatchnormalizationInt8 =
        std::make_unique<Executor::ONNCRuntimeBatchnormalizationInt8>();
    auto FuncONNCRuntimeConvInt8 =
        std::make_unique<Executor::ONNCRuntimeConvInt8>();
    auto FuncONNCRuntimeMaxpoolInt8 =
        std::make_unique<Executor::ONNCRuntimeMaxpoolInt8>();
    auto FuncONNCRuntimeMulInt8 =
        std::make_unique<Executor::ONNCRuntimeMulInt8>();
    auto FuncONNCRuntimeReluInt8 =
        std::make_unique<Executor::ONNCRuntimeReluInt8>();
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeAddFloat, "onnc_wasm",
                               "ONNC_RUNTIME_add_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeAveragepoolFloat, "onnc_wasm",
                               "ONNC_RUNTIME_averagepool_float");
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(FuncONNCRuntimeBatchnormalizationFloat, "onnc_wasm",
                          "ONNC_RUNTIME_batchnormalization_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeConcatFloat, "onnc_wasm",
                               "ONNC_RUNTIME_concat_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeConvFloat, "onnc_wasm",
                               "ONNC_RUNTIME_conv_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeGemmFloat, "onnc_wasm",
                               "ONNC_RUNTIME_gemm_float");
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(FuncONNCRuntimeGlobalaveragepoolFloat, "onnc_wasm",
                          "ONNC_RUNTIME_globalaveragepool_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeLrnFloat, "onnc_wasm",
                               "ONNC_RUNTIME_lrn_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeMaxpoolFloat, "onnc_wasm",
                               "ONNC_RUNTIME_maxpool_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeMulFloat, "onnc_wasm",
                               "ONNC_RUNTIME_mul_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeReluFloat, "onnc_wasm",
                               "ONNC_RUNTIME_relu_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeReshapeFloat, "onnc_wasm",
                               "ONNC_RUNTIME_reshape_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeSoftmaxFloat, "onnc_wasm",
                               "ONNC_RUNTIME_softmax_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeSumFloat, "onnc_wasm",
                               "ONNC_RUNTIME_sum_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeTransposeFloat, "onnc_wasm",
                               "ONNC_RUNTIME_transpose_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeUnsqueezeFloat, "onnc_wasm",
                               "ONNC_RUNTIME_unsqueeze_float");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeAddInt8, "onnc_wasm",
                               "ONNC_RUNTIME_add_int8");
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(FuncONNCRuntimeBatchnormalizationInt8, "onnc_wasm",
                          "ONNC_RUNTIME_batchnormalization_int8");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeConvInt8, "onnc_wasm",
                               "ONNC_RUNTIME_conv_int8");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeMaxpoolInt8, "onnc_wasm",
                               "ONNC_RUNTIME_maxpool_int8");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeMulInt8, "onnc_wasm",
                               "ONNC_RUNTIME_mul_int8");
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(FuncONNCRuntimeReluInt8, "onnc_wasm",
                               "ONNC_RUNTIME_relu_int8");
    }
  }
#endif

  return Status;
}

} // namespace VM
} // namespace SSVM
