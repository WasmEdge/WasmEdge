#include "executor/labelentry.h"

namespace SSVM {

/// Getter of instructions. See "include/executor/labelentry.h".
Executor::ErrCode LabelEntry::getInstructions(
    std::vector<std::unique_ptr<AST::Instruction>> *&Body) {
  Body = Instr;
  return Executor::ErrCode::Success;
}

} // namespace SSVM