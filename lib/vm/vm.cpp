// SPDX-License-Identifier: Apache-2.0
#include "vm/vm.h"
#include "executor/hostfunc.h"
#include "loader/loader.h"
#include "vm/result.h"

/// EEI Functions
#include "vm/hostfunc/ethereum/ethereum.h"

/// Wasi Functions
#include "vm/hostfunc/wasi/wasi_core.h"

#ifdef ONNC_WASM
/// ONNC Runtime Functions
#include "vm/hostfunc/onnc/onnc.h"
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

void VM::initVMEnv() {
  Rets.clear();
  VMResult.clear();
  prepareVMHost();
}

void VM::cleanup() {
  LoaderEngine.reset();
  ValidatorEngine.reset();
  ExecutorEngine.reset();
  Mod.reset();
  Args.clear();
  InVMStore = nullptr;
  OutVMStore = nullptr;
  OutAlloc = nullptr;
}

ErrCode VM::loadWasm() {
  Loader::ErrCode LoaderStatus = Loader::ErrCode::Success;
  VMResult.setStage(Result::Stage::Loader);

  if (WasmPath == "") {
    LoaderStatus = LoaderEngine.setCode(WasmCode);
  } else {
    LoaderStatus = LoaderEngine.setPath(WasmPath);
  }
  detail::testAndSetError(LoaderStatus, VMResult);
  if (!VMResult.hasError()) {
    LoaderStatus = LoaderEngine.parseModule();
    detail::testAndSetError(LoaderStatus, VMResult);
  }
  if (!VMResult.hasError()) {
    LoaderStatus = LoaderEngine.validateModule();
    detail::testAndSetError(LoaderStatus, VMResult);
  }
  if (!VMResult.hasError()) {
    LoaderStatus = LoaderEngine.getModule(Mod);
    detail::testAndSetError(LoaderStatus, VMResult);
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::validate() {
  Validator::ErrCode ValidatorStatus = Validator::ErrCode::Success;
  VMResult.setStage(Result::Stage::Validator);

  ValidatorStatus = ValidatorEngine.validate(Mod);
  detail::testAndSetError(ValidatorStatus, VMResult);

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::instantiate() {
  Executor::ErrCode ExecutorStatus = Executor::ErrCode::Success;
  VMResult.setStage(Result::Stage::Executor);

  ExecutorStatus = ExecutorEngine.setModule(Mod);
  detail::testAndSetError(ExecutorStatus, VMResult);
  if (!VMResult.hasError()) {
    ExecutorStatus = ExecutorEngine.instantiate();
    detail::testAndSetError(ExecutorStatus, VMResult);
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::runWasm() {
  Executor::ErrCode ExecutorStatus = Executor::ErrCode::Success;

  /// Prepare wasm input arguments.
  ExecutorStatus = ExecutorEngine.setArgs(Args);
  detail::testAndSetError(ExecutorStatus, VMResult);

  /// Load previous snapshot if VMStore is given.
  if (!VMResult.hasError() && InVMStore != nullptr) {
    ExecutorStatus = ExecutorEngine.restore(*InVMStore);
    detail::testAndSetError(ExecutorStatus, VMResult);
  }

  /// Execute wasm function.
  if (!VMResult.hasError()) {
    ExecutorStatus = ExecutorEngine.run();
    detail::testAndSetError(ExecutorStatus, VMResult);
    if (ExecutorStatus == Executor::ErrCode::Revert) {
      VMResult.setState(Result::State::Revert);
    }
  }

  /// Create current snapshot if VMStore is given.
  if (!VMResult.hasError() && OutVMStore != nullptr) {
    ExecutorStatus = ExecutorEngine.snapshot(*OutVMStore, *OutAlloc);
    detail::testAndSetError(ExecutorStatus, VMResult);
  }

  /// Prepare return values.
  if (!VMResult.hasError()) {
    ExecutorStatus = ExecutorEngine.getRets(Rets);
    detail::testAndSetError(ExecutorStatus, VMResult);
  }

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  VMResult.setState(Result::State::Commit);
  return ErrCode::Success;
}

void VM::setEntryFuncName(const std::string &FuncName) {
  /// Set entry function name.
  if (FuncName == "") {
    ExecutorEngine.setStartFuncName(Config.getStartFuncName());
  } else {
    ExecutorEngine.setStartFuncName(FuncName);
  }
}

ErrCode VM::execute(const std::string &FuncName) {
  /// Reset previous return values and VMResult.
  /// Initialize host functions.
  initVMEnv();

  /// Load wasm from file path or given bytes array.
  loadWasm();

  /// Validate wasm module.
  if (!VMResult.hasError()) {
    validate();
  }

  /// Set entry function name here.
  /// XXX: instantiate() will use this information in the instantiation of
  /// export section.
  setEntryFuncName(FuncName);

  /// Create wasm instance.
  if (!VMResult.hasError()) {
    instantiate();
  }

  /// Execute wasm with given input and entry function name.
  if (!VMResult.hasError()) {
    runWasm();
  }

  /// Clear loader and executor engine.
  cleanup();

  if (VMResult.hasError()) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::execute() { return execute(""); }

ErrCode VM::prepareVMHost() {
  ErrCode Status = ErrCode::Success;
  if (Config.hasVMType(Configure::VMType::Ewasm)) {
    /// Ewasm case, insert EEI host functions.
    EVMEnvironment *EVMEnv =
        getEnvironment<EVMEnvironment>(Configure::VMType::Ewasm);

    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEICall>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallCode>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallDataCopy>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallDelegate>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICallStatic>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEICodeCopy>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEICreate>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIExternalCodeCopy>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEIFinish>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetAddress>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetBlockCoinbase>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetBlockDifficulty>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetBlockGasLimit>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetBlockHash>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetBlockNumber>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetBlockTimestamp>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetCallDataSize>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetCallValue>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetCaller>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetCodeSize>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetExternalBalance>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetExternalCodeSize>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetGasLeft>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetReturnDataSize>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(
          std::make_unique<Executor::EEIGetTxGasPrice>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIGetTxOrigin>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEILog>(*EVMEnv));
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
          setHostFunction(std::make_unique<Executor::EEISelfDestruct>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIStorageLoad>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status =
          setHostFunction(std::make_unique<Executor::EEIStorageStore>(*EVMEnv));
    }
    if (Status == ErrCode::Success) {
      Status = setHostFunction(std::make_unique<Executor::EEIUseGas>(*EVMEnv));
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
