#include "vm/vm.h"
#include "vm/result.h"

namespace SSVM {
namespace VM {
ErrCode VM::setPath(const std::string &FilePath) {
  LoaderEngine.setPath(FilePath);
  return ErrCode::Success;
}

ErrCode VM::setInput(const std::vector<uint8_t> &InputVec) {
  Input = InputVec;
  return ErrCode::Success;
}

ErrCode VM::execute(Result &VMResult) {

  return ErrCode::Success;
}
} // namespace VM
} // namespace SSVM
