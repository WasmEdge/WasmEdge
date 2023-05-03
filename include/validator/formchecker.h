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
  return !V || V->isNumType();
}

static inline constexpr bool isRefType(const VType V) {
  return !V || V->isRefType();
}

class LocalType {
public:
  LocalType(ValType VType, bool Initialized)
      : IsSet(Initialized || VType.isDefaultable()), VType(VType) {}

  const ValType &getValType() const noexcept { return VType; }
  bool isSet() const noexcept { return IsSet; }
  void set() noexcept { IsSet = true; }

private:
  bool IsSet;
  ValType VType;
};

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(AST::InstrView Instrs, Span<const ValType> RetVals);

  /// Adder of contexts
  void addType(const AST::FunctionType &Func);
  void addFunc(const uint32_t TypeIdx, const bool IsImport = false);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addElem(const AST::ElementSegment &Elem);
  void addData(const AST::DataSegment &Data);
  void addRef(const uint32_t FuncIdx);
  void addLocal(const ValType &V, bool Initialized);

  bool match_type(const ValType &LHS, const ValType &RHS) const noexcept {
    if (LHS == RHS) {
      return true;
    }
    if (LHS.isRefType() && RHS.isRefType()) {
      return match_type(LHS.toRefType(), RHS.toRefType());
    }
    return false;
  }

  bool match_type(const RefType &LHS, const RefType &RHS) const noexcept {
    if (!match_type(LHS.getHeapType(), RHS.getHeapType())) {
      return false;
    }
    return !LHS.isNullableRefType() || RHS.isNullableRefType();
  }

  bool match_type(const HeapType &LHS, const HeapType &RHS) const noexcept {
    if (LHS == RHS) {
      return true;
    }
    if (LHS.getHeapTypeCode() == HeapTypeCode::TypeIndex) {
      if (RHS.getHeapTypeCode() == HeapTypeCode::Func) {
        return true;
      }
      if (RHS.getHeapTypeCode() == HeapTypeCode::TypeIndex &&
          match_type(LHS.getTypeIndex(), RHS.getTypeIndex())) {
        return true;
      }
    }
    return false;
  }

  bool match_type(uint32_t LTypeIdx, uint32_t RTypeIdx) const noexcept {
    assuming(LTypeIdx < Types.size());
    assuming(RTypeIdx < Types.size());
    // Note: In future versions of WebAssembly, subtyping on function types may
    // be relaxed to support co- and contra-variance.
    return Types[LTypeIdx].first == Types[RTypeIdx].first &&
           Types[LTypeIdx].second == Types[RTypeIdx].second;
  }

  bool match_type(Span<const ValType> LHS,
                  Span<const ValType> RHS) const noexcept {
    if (LHS.size() != RHS.size()) {
      return false;
    }
    for (uint32_t I = 0; I < LHS.size(); I++) {
      if (!match_type(LHS[I], RHS[I])) {
        return false;
      }
    }
    return true;
  }

  std::vector<VType> result() { return ValStack; }
  auto &getTypes() { return Types; }
  auto &getFunctions() { return Funcs; }
  auto &getTables() { return Tables; }
  auto &getMemories() { return Mems; }
  auto &getGlobals() { return Globals; }
  uint32_t getNumImportFuncs() const { return NumImportFuncs; }
  uint32_t getNumImportGlobals() const { return NumImportGlobals; }

  /// Helper function
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
    CtrlFrame(Span<const ValType> In, Span<const ValType> Out,
              const AST::Instruction *J, size_t H,
              OpCode Op = OpCode::Unreachable)
        : StartTypes(In.begin(), In.end()), EndTypes(Out.begin(), Out.end()),
          Jump(J), Height(H), IsUnreachable(false), Code(Op) {}
    std::vector<ValType> StartTypes;
    std::vector<ValType> EndTypes;
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
  std::vector<std::pair<std::vector<ValType>, std::vector<ValType>>> Types;
  std::vector<uint32_t> Funcs;
  std::vector<RefType> Tables;
  uint32_t Mems = 0;
  std::vector<std::pair<ValType, ValMut>> Globals;
  std::vector<RefType> Elems;
  std::vector<uint32_t> Datas;
  std::unordered_set<uint32_t> Refs;
  uint32_t NumImportFuncs = 0;
  uint32_t NumImportGlobals = 0;
  std::vector<LocalType> Locals;
  std::vector<ValType> Returns;

  /// Running stack.
  std::vector<CtrlFrame> CtrlStack;
  std::vector<VType> ValStack;
};

} // namespace Validator
} // namespace WasmEdge
