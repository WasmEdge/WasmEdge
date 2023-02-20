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
#include "common/enum_errinfo.hpp"
#include "common/errcode.h"
#include "common/span.h"

#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {

typedef std::optional<FullValType> VType;

static inline constexpr VType unreachableVType() { return VType(); }

static inline constexpr bool isNumType(const VType V) {
  return !V || V->isNumType();
}

static inline constexpr bool isRefType(const VType V) {
  return !V || V->isRefType();
}

class LocalType {
public:
  LocalType(FullValType VType, bool Initialized)
      : IsSet(Initialized || VType.isDefaultable()), VType(VType) {}

  const FullValType &getValType() const noexcept { return VType; }
  bool isSet() const noexcept { return IsSet; }
  void set() noexcept { IsSet = true; }

private:
  bool IsSet;
  FullValType VType;
};

class FormChecker {
public:
  FormChecker() = default;
  ~FormChecker() = default;

  void reset(bool CleanGlobal = false);
  Expect<void> validate(AST::InstrView Instrs, Span<const FullValType> RetVals);

  /// Adder of contexts
  void addType(const AST::DefinedType &Type);
  void addFunc(const uint32_t TypeIdx, const bool IsImport = false);
  void addTable(const AST::TableType &Tab);
  void addMemory(const AST::MemoryType &Mem);
  void addGlobal(const AST::GlobalType &Glob, const bool IsImport = false);
  void addElem(const AST::ElementSegment &Elem);
  void addData(const AST::DataSegment &Data);
  void addRef(const uint32_t FuncIdx);
  void addLocal(const FullValType &V, bool Initialized);

  Expect<AST::ArrayType> checkArrayType(const uint32_t TypeIdx) const;
  Expect<AST::StructType> checkStructType(const uint32_t TypeIdx) const;

  bool match_type(const FullValType &LHS,
                  const FullValType &RHS) const noexcept {
    if (LHS == RHS) {
      return true;
    }
    if (LHS.isRefType() && RHS.isRefType()) {
      return match_type(LHS.asRefType(), RHS.asRefType());
    }
    return false;
  }

  bool match_type(const FullRefType &LHS,
                  const FullRefType &RHS) const noexcept {
    if (!match_type(LHS.getHeapType(), RHS.getHeapType())) {
      return false;
    }
    return LHS.getTypeCode() == RefTypeCode::Ref ||
           RHS.getTypeCode() == RefTypeCode::RefNull;
  }

  bool match_type(const HeapType &LHS, const HeapType &RHS) const noexcept {
    if (LHS == RHS) {
      return true;
    }
    const auto TypeCode = LHS.getHTypeCode();
    switch (RHS.getHTypeCode()) {

    // Func
    case HeapTypeCode::Func: {
      if (TypeCode == HeapTypeCode::NoFunc || TypeCode == HeapTypeCode::Func) {
        return true;
      }
      if (TypeCode == HeapTypeCode::Defined) {
        return Types[LHS.getDefinedTypeIdx()].isType<AST::FunctionType>();
      }
      return false;
    }
    case HeapTypeCode::NoFunc: {
      return TypeCode == HeapTypeCode::NoFunc;
    }

    // Extern
    case HeapTypeCode::Extern:
      return TypeCode == HeapTypeCode::Extern ||
             TypeCode == HeapTypeCode::NoExtern;
    case HeapTypeCode::NoExtern:
      return TypeCode == HeapTypeCode::NoExtern;

    // Any
    case HeapTypeCode::Any:
      if (TypeCode == HeapTypeCode::Any || TypeCode == HeapTypeCode::Eq ||
          TypeCode == HeapTypeCode::I31 || TypeCode == HeapTypeCode::Struct ||
          TypeCode == HeapTypeCode::Array || TypeCode == HeapTypeCode::None) {
        return true;
      }
      if (TypeCode == HeapTypeCode::Defined) {
        const auto &Type = Types[LHS.getDefinedTypeIdx()];
        return Type.isType<AST::ArrayType>() || Type.isType<AST::StructType>();
      }
      return false;
    case HeapTypeCode::Eq:
      if (TypeCode == HeapTypeCode::Eq || TypeCode == HeapTypeCode::I31 ||
          TypeCode == HeapTypeCode::Struct || TypeCode == HeapTypeCode::Array ||
          TypeCode == HeapTypeCode::None) {
        return true;
      }
      if (TypeCode == HeapTypeCode::Defined) {
        const auto &Type = Types[LHS.getDefinedTypeIdx()];
        return Type.isType<AST::ArrayType>() || Type.isType<AST::StructType>();
      }
      return false;
    case HeapTypeCode::I31:
      return TypeCode == HeapTypeCode::I31 || TypeCode == HeapTypeCode::None;
    case HeapTypeCode::Struct:
      if (TypeCode == HeapTypeCode::Eq || TypeCode == HeapTypeCode::Struct ||
          TypeCode == HeapTypeCode::None) {
        return true;
      }
      if (TypeCode == HeapTypeCode::Defined) {
        const auto &Type = Types[LHS.getDefinedTypeIdx()];
        return Type.isType<AST::StructType>();
      }
      return false;
    case HeapTypeCode::Array:
      if (TypeCode == HeapTypeCode::Eq || TypeCode == HeapTypeCode::Array ||
          TypeCode == HeapTypeCode::None) {
        return true;
      }
      if (TypeCode == HeapTypeCode::Defined) {
        const auto &Type = Types[LHS.getDefinedTypeIdx()];
        return Type.isType<AST::ArrayType>();
      }
      return false;
    case HeapTypeCode::None:
      return TypeCode == HeapTypeCode::None;
    case HeapTypeCode::Defined:
      if (TypeCode == HeapTypeCode::Defined) {
        return match_type(LHS.getDefinedTypeIdx(), RHS.getDefinedTypeIdx());
      }
      const auto &RHSType = Types[RHS.getDefinedTypeIdx()];
      if (RHSType.isType<AST::FunctionType>()) {
        return TypeCode == HeapTypeCode::NoFunc;
      } else {
        return TypeCode == HeapTypeCode::None;
      }
    }
  }

  bool match_type(uint32_t LTypeIdx, uint32_t RTypeIdx) const noexcept {
    assuming(LTypeIdx < Types.size());
    assuming(RTypeIdx < Types.size());

    if (LTypeIdx == RTypeIdx) {
      return true;
    }

    const auto &ParentTypeIdx = Types[LTypeIdx].getParentTypeIdx();

    if (ParentTypeIdx.empty()) {
      return false;
    }

    assuming(ParentTypeIdx.size() == 1);
    return match_type(ParentTypeIdx[0], RTypeIdx);
  }

  bool match_type(const AST::DefinedType &LHS,
                  const AST::DefinedType &RHS) const noexcept {
    if (LHS.isType<AST::FunctionType>() && RHS.isType<AST::FunctionType>()) {
      return match_type(LHS.asFunctionType(), RHS.asFunctionType());
    } else if (LHS.isType<AST::ArrayType>() && RHS.isType<AST::ArrayType>()) {
      return match_type(LHS.asArrayType().getFieldType(),
                        RHS.asArrayType().getFieldType());
    } else if (LHS.isType<AST::StructType>() && RHS.isType<AST::StructType>()) {
      return match_type(LHS.asStructType(), RHS.asStructType());
    }
    return false;
  }

  bool match_type(const AST::StructType &LHS,
                  const AST::StructType &RHS) const noexcept {
    if (LHS.getContent().size() < RHS.getContent().size()) {
      spdlog::error("sub type has fewer fields than parent type");
      return false;
    }
    for (uint32_t I = 0; I < RHS.getContent().size(); I++) {
      if (!match_type(LHS.getContent()[I], RHS.getContent()[I])) {
        return false;
      }
    }
    return true;
  }

  bool match_type(const AST::FieldType &LHS,
                  const AST::FieldType &RHS) const noexcept {
    if (LHS.getMutability() == ValMut::Const &&
        RHS.getMutability() == ValMut::Var) {
      spdlog::error("child type is constant while the parent type is mutable");
      return false;
    }
    const auto &LStorageType = LHS.getStorageType();
    const auto &RStorageType = RHS.getStorageType();
    if (LStorageType.isValType() && RStorageType.isValType()) {
      return match_type(LStorageType.asValType(), RStorageType.asValType());
    }
    if (LStorageType.isPackedType() && RStorageType.isPackedType()) {
      return LStorageType.asPackedType() == RStorageType.asPackedType();
    }
    return false;
  }

  bool match_type(const AST::FunctionType &LHS,
                  const AST::FunctionType &RHS) const noexcept {
    return match_type(RHS.getParamTypes(), LHS.getParamTypes()) &&
           match_type(LHS.getReturnTypes(), RHS.getReturnTypes());
  }

  bool match_type(Span<const FullValType> LHS,
                  Span<const FullValType> RHS) const noexcept {
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
  FullValType VTypeToAST(const VType &V);

  struct CtrlFrame {
    CtrlFrame() = default;
    CtrlFrame(struct CtrlFrame &&F)
        : StartTypes(std::move(F.StartTypes)), EndTypes(std::move(F.EndTypes)),
          Jump(F.Jump), Height(F.Height), IsUnreachable(F.IsUnreachable),
          Code(F.Code) {}
    CtrlFrame(const struct CtrlFrame &F)
        : StartTypes(F.StartTypes), EndTypes(F.EndTypes), Jump(F.Jump),
          Height(F.Height), IsUnreachable(F.IsUnreachable), Code(F.Code) {}
    CtrlFrame(Span<const FullValType> In, Span<const FullValType> Out,
              const AST::Instruction *J, size_t H,
              OpCode Op = OpCode::Unreachable)
        : StartTypes(In.begin(), In.end()), EndTypes(Out.begin(), Out.end()),
          Jump(J), Height(H), IsUnreachable(false), Code(Op) {}
    std::vector<FullValType> StartTypes;
    std::vector<FullValType> EndTypes;
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
  void pushTypes(Span<const FullValType> Input);
  Expect<VType> popType();
  Expect<VType> popType(FullValType E);
  Expect<void> popTypes(Span<const FullValType> Input);
  void pushCtrl(Span<const FullValType> In, Span<const FullValType> Out,
                const AST::Instruction *Jump,
                OpCode Code = OpCode::Unreachable);
  Expect<CtrlFrame> popCtrl();
  Span<const FullValType> getLabelTypes(const CtrlFrame &F);
  Expect<void> unreachable();
  Expect<void> StackTrans(Span<const FullValType> Take,
                          Span<const FullValType> Put);
  Expect<void> StackPopAny();

  /// Contexts.
  std::vector<AST::DefinedType> Types;
  std::vector<uint32_t> Funcs;
  std::vector<FullRefType> Tables;
  uint32_t Mems = 0;
  std::vector<std::pair<FullValType, ValMut>> Globals;
  std::vector<FullRefType> Elems;
  std::vector<uint32_t> Datas;
  std::unordered_set<uint32_t> Refs;
  uint32_t NumImportFuncs = 0;
  uint32_t NumImportGlobals = 0;
  std::vector<LocalType> Locals;
  std::vector<FullValType> Returns;

  /// Running stack.
  std::vector<CtrlFrame> CtrlStack;
  std::vector<VType> ValStack;
};

} // namespace Validator
} // namespace WasmEdge
