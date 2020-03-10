// SPDX-License-Identifier: Apache-2.0
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

#include "common/ast/instruction.h"
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
  /// \param StackSize the current label position in stack.
  /// \param LabelArity the return counts of this block.
  /// \param Instr the branch target of this label.
  ///
  /// \returns ErrCode.
  Label(unsigned StackSize, unsigned int Coarity,
        AST::BlockControlInstruction *Instr)
      : StackSize(StackSize), Coarity(Coarity), Target(Instr) {}

  /// Getter of stack size.
  unsigned int getStackSize() const { return StackSize; }

  /// Getter of coarity.
  unsigned int getCoarity() const { return Coarity; }

  /// Getter of control instruction for branch target.
  AST::BlockControlInstruction *getTarget() { return Target; }

private:
  /// \name Data of label entry.
  /// @{
  unsigned int StackSize;
  unsigned int Coarity;
  AST::BlockControlInstruction *Target = nullptr;
  /// @}
};

} // namespace Executor
} // namespace SSVM
