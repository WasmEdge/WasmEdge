#include "executor/worker.h"

namespace SSVM {
namespace Executor {

ErrCode Worker::setArguments(Bytes &Input) {
  Args.assign(Input.begin(), Input.end());
  return ErrCode::Success;
}

ErrCode Worker::setCode(std::vector<std::unique_ptr<AST::Instruction>> &Instrs) {
    for (auto &Instr : Instrs) {
      this->Instrs.push_back(Instr.get());
    }
    return ErrCode::Success;
}

ErrCode Worker::run() {
  for (auto &Inst : Instrs) {

  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
