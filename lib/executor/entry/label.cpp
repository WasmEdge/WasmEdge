#include "executor/entry/label.h"

namespace SSVM {
namespace Executor {

/// Getter of instructions. See "include/executor/entry/label.h".
ErrCode LabelEntry::getInstructions(
    std::vector<std::unique_ptr<AST::Instruction>> *&Body) {
  Body = Instr;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
