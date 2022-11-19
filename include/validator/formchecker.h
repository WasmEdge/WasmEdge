// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/validator/formchecker.h - Form checking class definition -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the FormChecker class, which helps
/// validator to check types in stack with instructions.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "common/span.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {

typedef std::optional<ValType> VType;

static inline constexpr VType unreachableVType() { return VType(); }

static inline constexpr bool isNumType(const VType V) {

  return !V || *V == ValType::I32 || *V == ValType::I64 || *V == ValType::F32 ||
         *V == ValType::F64 || *V == ValType::V128;
}

static inline constexpr bool isRefType(const VType V) {
  return !V || *V == ValType::FuncRef || *V == ValType::ExternRef;
}

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(AST::InstrView Instrs, Span<const ValType> RetVals);
  Expect<void> validate(AST::InstrView Instrs, Span<const VType> RetVals);

  /// Adder of contexts
  void addType(const AST::FunctionType &Func);
  void addFunc(const uint32_t TypeIdx, const bool IsImport = false);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addElem(const AST::ElementSegment &Elem);
  void addData(const AST::DataSegment &Data);
  void addRef(const uint32_t FuncIdx);
  void addLocal(const ValType &V);
  void addLocal(const VType &V);

  std::vector<VType> result() { return ValStack; }
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
          Jump(F.Jump), Height(F.Height), IsUnreachable(F.IsUnreachable),
          Code(F.Code) {}
    CtrlFrame(const struct CtrlFrame &F)
        : StartTypes(F.StartTypes), EndTypes(F.EndTypes), Jump(F.Jump),
          Height(F.Height), IsUnreachable(F.IsUnreachable), Code(F.Code) {}
    CtrlFrame(Span<const VType> In, Span<const VType> Out,
              const AST::Instruction *J, size_t H,
              OpCode Op = OpCode::Unreachable)
        : StartTypes(In.begin(), In.end()), EndTypes(Out.begin(), Out.end()),
          Jump(J), Height(H), IsUnreachable(false), Code(Op) {}
    std::vector<VType> StartTypes;
    std::vector<VType> EndTypes;
    const AST::Instruction *Jump;
    size_t Height;
    bool IsUnreachable;
    OpCode Code;
  };

private:
  /// Checking expression
  Expect<void> checkExpr(AST::InstrView Instrs);

  /// Checking instruction list
  Expect<void> checkInstrs(AST::InstrView Instrs);

  /// Instruction iteration
  Expect<void> checkInstr(const AST::Instruction &Instr);

  /// Stack operations
  void pushType(VType);
  void pushTypes(Span<const VType> Input);
  Expect<VType> popType();
  Expect<VType> popType(VType E);
  Expect<void> popTypes(Span<const VType> Input);
  void pushCtrl(Span<const VType> In, Span<const VType> Out,
                const AST::Instruction *Jump,
                OpCode Code = OpCode::Unreachable);
  Expect<CtrlFrame> popCtrl();
  Span<const VType> getLabelTypes(const CtrlFrame &F);
  Expect<void> unreachable();
  Expect<void> StackTrans(Span<const VType> Take, Span<const VType> Put);

  /// Contexts.
  std::vector<std::pair<std::vector<VType>, std::vector<VType>>> Types;
  std::vector<uint32_t> Funcs;
  std::vector<RefType> Tables;
  uint32_t Mems = 0;
  std::vector<std::pair<VType, ValMut>> Globals;
  std::vector<RefType> Elems;
  std::vector<uint32_t> Datas;
  std::unordered_set<uint32_t> Refs;
  uint32_t NumImportFuncs = 0;
  uint32_t NumImportGlobals = 0;
  std::vector<VType> Locals;
  std::vector<VType> Returns;

  /// Running stack.
  std::vector<CtrlFrame> CtrlStack;
  std::vector<VType> ValStack;
};

} // namespace Validator
} // namespace WasmEdge
