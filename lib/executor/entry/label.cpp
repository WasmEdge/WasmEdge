#include "executor/entry/label.h"

namespace SSVM {
namespace Executor {
namespace Entry {

/// Getter of instructions. See "include/executor/entry/label.h".
ErrCode LabelEntry::getInstructions(
    std::vector<std::unique_ptr<AST::Instruction>> *&Body) {
  Body = Instr;
  return ErrCode::Success;
}

} // namespace Entry
} // namespace Executor
} // namespace SSVM
