#include "vm/vm.h"
#include "loader/loader.h"
#include "vm/result.h"

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

ErrCode VM::execute() {
  ErrCode Status = ErrCode::Success;
  if ((Status = runLoader()) != ErrCode::Success)
    return Status;
  if ((Status = runExecutor()) != ErrCode::Success)
    return Status;
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

} // namespace VM
} // namespace SSVM