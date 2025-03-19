// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(AST::InstrView Instrs, Span<const ValType> RetVals);
  Expect<void> validate(const ValType &VT) const noexcept;

  /// Adder of contexts
  void addType(const AST::SubType &Type);
  void addFunc(const uint32_t TypeIdx, const bool IsImport = false);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addElem(const AST::ElementSegment &Elem);
  void addData(const AST::DataSegment &Data);
  void addRef(const uint32_t FuncIdx);
  void addLocal(const ValType &V, bool Initialized);
  void addTag(const uint32_t TypeIdx);

  std::vector<VType> result() { return ValStack; }
  auto &getTypes() { return Types; }
  auto &getFunctions() { return Funcs; }
  auto &getTables() { return Tables; }
  auto &getMemories() { return Mems; }
  auto &getGlobals() { return Globals; }
  auto &getTags() { return Tags; }
  uint32_t getNumImportFuncs() const { return NumImportFuncs; }
  uint32_t getNumImportGlobals() const { return NumImportGlobals; }

  /// Helper function
  ValType VTypeToAST(const VType &V);

  /// Control frame
  struct CtrlFrame {
    CtrlFrame() = default;
    CtrlFrame(struct CtrlFrame &&F)
        : StartTypes(std::move(F.StartTypes)), EndTypes(std::move(F.EndTypes)),
          Jump(F.Jump), Height(F.Height), InitedLocal(F.InitedLocal),
          IsUnreachable(F.IsUnreachable), Code(F.Code) {}
    CtrlFrame(const struct CtrlFrame &F)
        : StartTypes(F.StartTypes), EndTypes(F.EndTypes), Jump(F.Jump),
          Height(F.Height), InitedLocal(F.InitedLocal),
          IsUnreachable(F.IsUnreachable), Code(F.Code) {}
    CtrlFrame(Span<const ValType> In, Span<const ValType> Out,
              const AST::Instruction *J, size_t H, size_t LocalH,
              OpCode Op = OpCode::Unreachable)
        : StartTypes(In.begin(), In.end()), EndTypes(Out.begin(), Out.end()),
          Jump(J), Height(H), InitedLocal(LocalH), IsUnreachable(false),
          Code(Op) {}
    std::vector<ValType> StartTypes;
    std::vector<ValType> EndTypes;
    const AST::Instruction *Jump;
    size_t Height;
    size_t InitedLocal;
    bool IsUnreachable;
    OpCode Code;
  };

  struct LocalType {
    LocalType(ValType VT, bool Initialized = false)
        : IsInit(Initialized), VType(VT) {}
    bool IsInit;
    const ValType VType;
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
  void pushTypes(Span<const ValType> Input);
  Expect<VType> popType();
  Expect<VType> popType(ValType E);
  Expect<void> popTypes(Span<const ValType> Input);
  void pushCtrl(Span<const ValType> In, Span<const ValType> Out,
                const AST::Instruction *Jump,
                OpCode Code = OpCode::Unreachable);
  Expect<CtrlFrame> popCtrl();
  Span<const ValType> getLabelTypes(const CtrlFrame &F);
  Expect<void> unreachable();
  Expect<void> StackTrans(Span<const ValType> Take, Span<const ValType> Put);
  Expect<void> StackPopAny();

  /// Contexts.
  std::vector<const AST::SubType *> Types;
  std::vector<uint32_t> Funcs;
  std::vector<ValType> Tables;
  uint32_t Mems = 0;
  std::vector<std::pair<ValType, ValMut>> Globals;
  std::vector<ValType> Elems;
  std::vector<uint32_t> Datas;
  std::unordered_set<uint32_t> Refs;
  uint32_t NumImportFuncs = 0;
  uint32_t NumImportGlobals = 0;
  std::vector<LocalType> Locals;
  std::vector<uint32_t> LocalInits;
  std::vector<ValType> Returns;
  std::vector<uint32_t> Tags;

  /// Running stack.
  std::vector<CtrlFrame> CtrlStack;
  std::vector<VType> ValStack;
};

} // namespace Validator
} // namespace WasmEdge
