//===-- ssvm/executor/entry/label.h - Label Entry class definition --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Label Entry class in stack manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "executor/common.h"

namespace SSVM {
namespace Executor {

class Label {
public:
  Label() = default;
  ~Label() = default;

  /// Initializer of label entry.
  ///
  /// Initialize the label with arity and pointer to instructions in blocks.
  ///
  /// \param LabelArity the return counts of this block.
  /// \param Instr the branch target of this label.
  ///
  /// \returns ErrCode.
  Label(const unsigned int LabelArity,
        AST::BlockControlInstruction *Instr = nullptr)
      : Arity(LabelArity), Target(Instr) {}

  /// Getter of arity.
  unsigned int getArity() const { return Arity; }

  /// Getter of control instruction for branch target.
  AST::BlockControlInstruction *getTarget() { return Target; }

private:
  /// \name Data of label entry.
  /// @{
  unsigned int Arity;
  AST::BlockControlInstruction *Target = nullptr;
  /// @}
};

} // namespace Executor
} // namespace SSVM
