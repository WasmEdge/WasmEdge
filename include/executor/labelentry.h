//===-- ssvm/executor/labelentry.h - Label Entry class definition ---------===//
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
#include "common.h"
#include <memory>
#include <vector>

class LabelEntry {
public:
  /// Constructor of initialization of a label.
  ///
  /// Initialize the label with arity and pointer to instructions in blocks.
  ///
  /// \param Arity the return counts of this function type.
  /// \param Body the pointer to vector of instructions in blocks.
  ///
  /// \returns None.
  LabelEntry(unsigned int Arity,
             std::vector<std::unique_ptr<AST::Instruction>> *Body)
      : LabArity(Arity), Instr(Body){};

  ~LabelEntry() = default;

  /// Getter of arity.
  Executor::ErrCode getArity(unsigned int &Arity);

  /// Getter of instructions.
  Executor::ErrCode
  getInstructions(std::vector<std::unique_ptr<AST::Instruction>> *&Body);

private:
  /// \name Data of frame entry.
  /// @{
  unsigned int LabArity;
  std::vector<std::unique_ptr<AST::Instruction>> *Instr;
  /// @}
};