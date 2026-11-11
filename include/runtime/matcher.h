// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/matcher.h - Runtime matcher definition -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the matching of the runtime values and instances against
/// the types. The type rules themselves are in the AST type matcher.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/types.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/composite.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"
#include "runtime/typetable.h"

#include <cstdint>
#include <optional>

namespace WasmEdge {
namespace Runtime {

/// Runtime match helper class.
class Matcher {
public:
  /// Match a reference to a value type of a module instance, if any. The
  /// expected type tells whether the reference is seen as an extern reference.
  static bool matchRef(const Instance::ModuleInstance *ModInst,
                       const ValType &Exp, const RefVariant &Ref) noexcept {
    // The null reference has the bottom type.
    if (Ref.isNull()) {
      return AST::TypeMatcher::matchBottomRef(Exp, true);
    }
    // A non-null reference matches by the own type of the referenced instance.
    auto VT = Ref.getType().getNonNullableRef();
    // An expected type out of any module is abstract and needs no type list.
    Span<const AST::SubType *const> ExpTypeList;
    if (ModInst) {
      ExpTypeList = ModInst->getTypeList();
    }
    Span<const AST::SubType *const> GotTypeList = ExpTypeList;
    const AST::SubType *HostType = nullptr;
    std::optional<uint32_t> GotId;
    if (!VT.isAbsHeapType()) {
      const auto *Inst = Ref.getCompositeInst();
      const auto *GotMod = Inst->getModule() ? Inst->getModule() : ModInst;
      if (GotMod == nullptr ||
          VT.getTypeIndex() >= GotMod->getTypeList().size()) {
        return false;
      }
      GotId = GotMod->getCanonicalTypeId(VT.getTypeIndex());
    } else if (VT.getHeapTypeCode() == TypeCode::FuncRef) {
      const auto *FuncInst = retrieveFuncRef(Ref);
      if (FuncInst->getModule()) {
        GotId =
            FuncInst->getModule()->getCanonicalTypeId(FuncInst->getTypeIndex());
      } else if (const auto Id = FuncInst->getHostCanonicalTypeId();
                 Id.has_value()) {
        GotId = Id;
      } else {
        // A host function type that cannot be registered is a type list of
        // its own.
        HostType = &FuncInst->getHostFunc().getDefinedType();
        GotTypeList = Span<const AST::SubType *const>(&HostType, 1);
        VT = ValType(TypeCode::Ref, 0U);
      }
    } else if (VT.getHeapTypeCode() == TypeCode::ExternRef) {
      // A reference to a host object is an any reference.
      VT = ValType(TypeCode::Ref, TypeCode::AnyRef);
    }
    if (GotId.has_value()) {
      const auto &Table = CanonicalTypeTable::getGlobal();
      if (!Exp.isAbsHeapType()) {
        return ModInst != nullptr &&
               Exp.getTypeIndex() < ModInst->getTypeList().size() &&
               Table.isSubType(ModInst->getCanonicalTypeId(Exp.getTypeIndex()),
                               *GotId);
      }
      // A defined type matches the abstract heap types above its expansion.
      VT = ValType(TypeCode::Ref,
                   Table.getType(*GotId).getCompositeType().expand());
    }
    // An extern reference is the outside view of a reference of the any
    // hierarchy.
    if (Exp.isExternRefType() &&
        AST::TypeMatcher::getTopHeapType(GotTypeList, VT) == TypeCode::AnyRef) {
      VT = ValType(TypeCode::Ref, TypeCode::ExternRef);
    }
    return AST::TypeMatcher::matchType(ExpTypeList, Exp, GotTypeList, VT);
  }

  /// Match a reference passed with a type to a value type of a module
  /// instance, if any. The passed type tells how the reference is seen.
  static bool matchRef(const Instance::ModuleInstance *ModInst,
                       const ValType &Exp, const RefVariant &Ref,
                       const ValType &Got) noexcept {
    // A reference passed as an extern one fits the extern types only, and no
    // other reference fits them. The null reference has no view.
    return (Ref.isNull() || Got.isExternRefType() == Exp.isExternRefType()) &&
           matchRef(ModInst, Exp, Ref);
  }

  /// Match a function instance of a module instance to a defined type.
  static bool
  matchType(const Instance::ModuleInstance &ExpModInst, uint32_t ExpIdx,
            const Instance::ModuleInstance &GotModInst,
            const Instance::FunctionInstance &Got,
            AST::TypeMatcher::RecTypePairs *Equal = nullptr) noexcept {
    return AST::TypeMatcher::matchType(ExpModInst.getTypeList(), ExpIdx,
                                       getTypeList(GotModInst, Got),
                                       Got.getTypeIndex(), Equal);
  }

  /// Match a table instance of a module instance to a table type.
  static bool
  matchType(const Instance::ModuleInstance &ExpModInst,
            const AST::TableType &Exp,
            const Instance::ModuleInstance &GotModInst,
            const Instance::TableInstance &Got,
            AST::TypeMatcher::RecTypePairs *Equal = nullptr) noexcept {
    return AST::TypeMatcher::matchType(ExpModInst.getTypeList(), Exp,
                                       getTypeList(GotModInst, Got),
                                       Got.getTableType(), Equal);
  }

  /// Match a memory instance to a memory type.
  static bool matchType(const AST::MemoryType &Exp,
                        const Instance::MemoryInstance &Got) noexcept {
    return AST::TypeMatcher::matchLimit(Exp.getLimit(),
                                        Got.getMemoryType().getLimit());
  }

  /// Match a tag instance of a module instance to a tag type.
  static bool
  matchType(const Instance::ModuleInstance &ExpModInst, const AST::TagType &Exp,
            const Instance::ModuleInstance &GotModInst,
            const Instance::TagInstance &Got,
            AST::TypeMatcher::RecTypePairs *Equal = nullptr) noexcept {
    return AST::TypeMatcher::matchType(ExpModInst.getTypeList(), Exp,
                                       getTypeList(GotModInst, Got),
                                       Got.getTagType(), Equal);
  }

  /// Match a global instance of a module instance to a global type.
  static bool
  matchType(const Instance::ModuleInstance &ExpModInst,
            const AST::GlobalType &Exp,
            const Instance::ModuleInstance &GotModInst,
            const Instance::GlobalInstance &Got,
            AST::TypeMatcher::RecTypePairs *Equal = nullptr) noexcept {
    return AST::TypeMatcher::matchType(ExpModInst.getTypeList(), Exp,
                                       getTypeList(GotModInst, Got),
                                       Got.getGlobalType(), Equal);
  }

private:
  /// Helper for the type list of an instance: of its defining module, if any.
  template <typename T>
  static Span<const AST::SubType *const>
  getTypeList(const Instance::ModuleInstance &ModInst, const T &Inst) noexcept {
    return Inst.getModule() ? Inst.getModule()->getTypeList()
                            : ModInst.getTypeList();
  }
};

} // namespace Runtime
} // namespace WasmEdge
