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
#include "support/span.h"

#include <deque>
#include <vector>

namespace SSVM {
namespace Validator {

enum class VType : uint8_t { Unknown, I32, I64, F32, F64, FuncRef, ExternRef };

static inline constexpr bool isNumType(const VType V) {
  return V == VType::I32 || V == VType::I64 || V == VType::F32 ||
         V == VType::F64 || V == VType::Unknown;
}

static inline constexpr bool isRefType(const VType V) {
  return V == VType::FuncRef || V == VType::ExternRef || V == VType::Unknown;
}

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(const AST::InstrVec &Instrs,
                        Span<const ValType> RetVals);
  Expect<void> validate(const AST::InstrVec &Instrs, Span<const VType> RetVals);

  /// Adder of contexts
  void addType(const AST::FunctionType &Func);
  void addFunc(const uint32_t TypeIdx, const bool IsImport = false);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addLocal(const ValType &V);
  void addLocal(const VType &V);

  std::vector<VType> result() { return ValStack; };
  auto &getTypes() { return Types; }
  auto &getFunctions() { return Funcs; }
  auto &getTables() { return Tables; }
  auto &getMemories() { return Mems; }
  auto &getGlobals() { return Globals; }
  uint32_t getNumImportFuncs() const { return NumImportFuncs; }
  uint32_t getNumImportGlobals() const { return NumImportGlobals; }

  /// Helper function
  VType ASTToVType(const ValType &V);
  VType ASTToVType(const NumType &V);
  VType ASTToVType(const RefType &V);
  ValType VTypeToAST(const VType &V);

  struct CtrlFrame {
    CtrlFrame() = default;
    CtrlFrame(struct CtrlFrame &&F)
        : StartTypes(std::move(F.StartTypes)), EndTypes(std::move(F.EndTypes)),
          Height(F.Height), IsUnreachable(F.IsUnreachable), IsLoop(F.IsLoop) {}
    CtrlFrame(const struct CtrlFrame &F)
        : StartTypes(F.StartTypes), EndTypes(F.EndTypes), Height(F.Height),
          IsUnreachable(F.IsUnreachable), IsLoop(F.IsLoop) {}
    CtrlFrame(Span<const VType> In, Span<const VType> Out, size_t H,
              bool IsLoopOp = false)
        : StartTypes(In.begin(), In.end()), EndTypes(Out.begin(), Out.end()),
          Height(H), IsUnreachable(false), IsLoop(IsLoopOp) {}
    std::vector<VType> StartTypes;
    std::vector<VType> EndTypes;
    size_t Height;
    bool IsUnreachable;
    bool IsLoop;
  };

private:
  /// Checking expression
  Expect<void> checkExpr(const AST::InstrVec &Instrs);

  /// Checking instruction list
  Expect<void> checkInstrs(const AST::InstrVec &Instrs);

  /// Instruction iteration
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

  /// Stack operations
  void pushType(VType);
  void pushTypes(Span<const VType> Input);
  Expect<VType> popType();
  Expect<VType> popType(VType E);
  Expect<void> popTypes(Span<const VType> Input);
  void pushCtrl(Span<const VType> Label, Span<const VType> Out,
                bool IsLoopOp = false);
  Expect<CtrlFrame> popCtrl();
  Span<const VType> getLabelTypes(const CtrlFrame &F);
  Expect<void> unreachable();
  Expect<void> StackTrans(Span<const VType> Take, Span<const VType> Put);

  /// Helper functions
  Expect<std::pair<Span<const VType>, Span<const VType>>>
  resolveBlockType(std::vector<VType> &Buffer, BlockType Type);

  /// Contexts.
  std::vector<std::pair<std::vector<VType>, std::vector<VType>>> Types;
  std::vector<uint32_t> Funcs;
  std::vector<RefType> Tables;
  std::vector<uint32_t> Mems;
  std::vector<std::pair<VType, ValMut>> Globals;
  uint32_t NumImportFuncs = 0;
  uint32_t NumImportGlobals = 0;
  std::vector<VType> Locals;
  std::vector<VType> Returns;

  /// Running stack.
  std::vector<CtrlFrame> CtrlStack;
  std::vector<VType> ValStack;
};

} // namespace Validator
} // namespace SSVM
