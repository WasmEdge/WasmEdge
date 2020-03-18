// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/validator/formchecker.h - Form checking class definition -----===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the FormChecker class, which helps
/// validator to check types in stack with instructions.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/instruction.h"
#include "common/ast/module.h"
#include "common/errcode.h"
#include "common/types.h"
#include "common/value.h"

#include <deque>
#include <vector>

namespace SSVM {
namespace Validator {

enum class VType : uint32_t { Unknown, I32, I64, F32, F64 };
using OpCode = AST::Instruction::OpCode;

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(const AST::InstrVec &Instrs,
                        const std::vector<ValType> &RetVals);
  Expect<void> validate(const AST::InstrVec &Instrs,
                        const std::vector<VType> &RetVals);

  /// Adder of contexts
  void addType(const AST::FunctionType &Func);
  void addFunc(const uint32_t &TypeIdx);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addLocal(const ValType &V);
  void addLocal(const VType &V);

  std::deque<VType> result() { return ValStack; };
  auto &getTypes() { return Types; }
  auto &getFunctions() { return Funcs; }
  auto &getTables() { return Tables; }
  auto &getMemories() { return Mems; }
  auto &getGlobals() { return Globals; }
  uint32_t getNumImportGlobals() const { return NumImportGlobals; }

private:
  struct CtrlFrame {
    std::vector<VType> LabelTypes;
    std::vector<VType> EndTypes;
    size_t Height;
    bool IsUnreachable;
  };

  /// Instruction iteration
  Expect<void> checkInstrs(const AST::InstrVec &Instrs);
  Expect<void> checkInstr(const AST::ControlInstruction &Instr);
  Expect<void> checkInstr(const AST::BlockControlInstruction &Instr);
  Expect<void> checkInstr(const AST::IfElseControlInstruction &Instr);
  Expect<void> checkInstr(const AST::BrControlInstruction &Instr);
  Expect<void> checkInstr(const AST::BrTableControlInstruction &Instr);
  Expect<void> checkInstr(const AST::CallControlInstruction &Instr);
  Expect<void> checkInstr(const AST::ParametricInstruction &Instr);
  Expect<void> checkInstr(const AST::VariableInstruction &Instr);
  Expect<void> checkInstr(const AST::MemoryInstruction &Instr);
  Expect<void> checkInstr(const AST::ConstInstruction &Instr);
  Expect<void> checkInstr(const AST::UnaryNumericInstruction &Instr);
  Expect<void> checkInstr(const AST::BinaryNumericInstruction &Instr);

  /// Helper function
  VType ASTToVType(const ValType &V);

  /// Stack operations
  void pushType(VType);
  void pushTypes(const std::vector<VType> &Input);
  Expect<VType> popType();
  Expect<VType> popType(VType E);
  Expect<void> popTypes(const std::vector<VType> &Input);
  void pushCtrl(const std::vector<VType> &Label, const std::vector<VType> &Out);
  Expect<std::vector<VType>> popCtrl();
  Expect<void> unreachable();
  Expect<void> StackTrans(const std::vector<VType> &Take,
                          const std::vector<VType> &Put);

  /// Contexts.
  std::vector<std::pair<std::vector<VType>, std::vector<VType>>> Types;
  std::vector<uint32_t> Funcs;
  std::vector<ElemType> Tables;
  std::vector<uint32_t> Mems;
  std::vector<std::pair<VType, ValMut>> Globals;
  uint32_t NumImportGlobals = 0;
  std::vector<VType> Locals;
  std::vector<VType> Returns;

  /// Running stack.
  std::deque<CtrlFrame> CtrlStack;
  std::deque<VType> ValStack;
};

} // namespace Validator
} // namespace SSVM