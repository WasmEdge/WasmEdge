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

class LabelEntry {
public:
  /// Labels are not allowed to create without arguments.
  LabelEntry() = delete;

  /// Constructor of initialization of a label.
  ///
  /// Initialize the label with arity null of instruction sequence.
  ///
  /// \param Arity the return counts of this block.
  ///
  /// \returns None.
  LabelEntry(const unsigned int Arity) : Arity(Arity) {}

  /// Constructor of initialization of a label.
  ///
  /// Initialize the label with arity and pointer to instructions in blocks.
  ///
  /// \param Arity the return counts of this block.
  /// \param Instr the branch target of this label.
  ///
  /// \returns None.
  LabelEntry(const unsigned int Arity, AST::Instruction *Instr)
      : Target(Instr) {}

  ~LabelEntry() = default;

  /// Getter of arity.
  unsigned int getArity() const { return Arity; }

  /// Getter of control instruction for branch target.
  AST::Instruction *getTarget() { return Target; }

private:
  /// \name Data of label entry.
  /// @{
  unsigned int Arity;
  AST::Instruction *Target;
  /// @}
};

} // namespace Executor
} // namespace SSVM
