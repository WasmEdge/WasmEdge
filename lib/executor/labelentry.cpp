#include "executor/labelentry.h"

/// Getter of arity. See "include/executor/labelentry.h".
Executor::ErrCode LabelEntry::getArity(unsigned int &Arity) {
  Arity = LabArity;
  return Executor::ErrCode::Success;
}

/// Getter of instructions. See "include/executor/labelentry.h".
Executor::ErrCode LabelEntry::getInstructions(
    std::vector<std::unique_ptr<AST::Instruction>> *&Body) {
  Body = Instr;
  return Executor::ErrCode::Success;
}