#include "executor/worker/provider.h"
#include "executor/common.h"

namespace SSVM {
namespace Executor {

/// Getter for next instruction. See "include/executor/worker/provider.h".
AST::Instruction *InstrProvider::getNextInstr() {
  /// Instruction sequence vector is empty.
  if (Iters.size() == 0)
    return nullptr;

  /// Instruction sequence executed to end.
  if (Iters.back().Curr == Iters.back().End)
    return nullptr;

  /// Get instruction.
  AST::Instruction *Instr = (*Iters.back().Curr).get();
  (Iters.back().Curr)++;
  return Instr;
}

/// Get sequence type of top scope. See "include/executor/worker/provider.h".
InstrProvider::SeqType InstrProvider::getTopScopeType() {
  if (Iters.size() > 0)
    return Iters.back().Type;
  return SeqType::Expression;
}

/// Push and jump to a new instruction sequence. See
/// "include/executor/worker/provider.h".
ErrCode InstrProvider::pushInstrs(SeqType Type, const InstrVec &Instrs) {
  Iters.emplace_back(Type, Instrs.cbegin(), Instrs.cend());
  return ErrCode::Success;
}

/// Pop last instruction sequence and jump back. See
/// "include/executor/worker/provider.h".
ErrCode InstrProvider::popInstrs() {
  if (Iters.size() == 0)
    return ErrCode::WrongInstructionCounter;
  Iters.pop_back();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM