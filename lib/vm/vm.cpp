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
  Loader::ErrCode Status = LoaderEngine.setPath(FilePath);
  VMResult.setStage(Result::Stage::Loader);
  if (detail::testAndSetError(Status, VMResult)) {
    return ErrCode::Failed;
  }
  return ErrCode::Success;
}

ErrCode VM::setInput(const std::vector<uint8_t> &InputVec) {
  Input = InputVec;
  return ErrCode::Success;
}

ErrCode VM::execute() {
  /// Load wasm file
  Loader::ErrCode LoaderStatus = Loader::ErrCode::Success;
  VMResult.setStage(Result::Stage::Loader);

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

} // namespace VM
} // namespace SSVM