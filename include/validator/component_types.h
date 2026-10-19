// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_types.h - Component type system ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component-model type views resolved over the AST,
/// the scope holding one component's index spaces, and the subtype relation.
/// Context is the checker that owns them and depends on this file.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/canonical.h"
#include "ast/component/descriptor.h"
#include "ast/component/sort.h"
#include "ast/component/type.h"
#include "ast/type.h"
#include "common/component_valtype.h"
#include "common/errcode.h"
#include "common/span.h"
#include "validator/component_name.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {
namespace Component {

struct Scope;
struct Shape;

// ===========================================================================
// Core-level views.
// ===========================================================================

/// Resolved core extern type: Kind discriminates, a Tag's signature is Func.
struct CoreExternInfo {
  CoreExternInfo() noexcept = default;
  /// A function, or the signature of a tag when K is Tag.
  CoreExternInfo(ExternalType K, const AST::SubType *ST) noexcept
      : Kind(K), Func(ST) {}
  explicit CoreExternInfo(const AST::TableType *TT) noexcept
      : Kind(ExternalType::Table), Table(TT) {}
  explicit CoreExternInfo(const AST::MemoryType *MT) noexcept
      : Kind(ExternalType::Memory), Memory(MT) {}
  explicit CoreExternInfo(const AST::GlobalType *GT) noexcept
      : Kind(ExternalType::Global), Global(GT) {}

  ExternalType Kind = ExternalType::Function;
  const AST::SubType *Func = nullptr;
  const AST::TableType *Table = nullptr;
  const AST::MemoryType *Memory = nullptr;
  const AST::GlobalType *Global = nullptr;
};

/// Core module or instance: ordered imports (none for an instance) and exports.
struct CoreShape {
  std::vector<std::tuple<std::string, std::string, CoreExternInfo>> Imports;
  std::map<std::string, CoreExternInfo, std::less<>> Exports;
};

/// Entry of the core:type index space: a rectype member or a moduletype.
struct CoreTypeEntry {
  const AST::SubType *Func = nullptr;
  const CoreShape *Mod = nullptr;
};

// ===========================================================================
// Resource identity remapping.
// ===========================================================================

/// Remap of a view's resource ids to those meant here; an absent id is itself.
struct ResourceMap {
  std::unordered_map<uint32_t, uint32_t> Map;

  uint32_t apply(uint32_t Id) const noexcept {
    auto It = Map.find(Id);
    return It != Map.end() ? It->second : Id;
  }
};

// ===========================================================================
// Component-level views.
// ===========================================================================

/// A valtype with the scope its indices resolve in and the remap in effect.
struct QualValType {
  ComponentValType VT{};
  const Scope *Home = nullptr;
  const ResourceMap *Remap = nullptr;
};

/// A component-level function type view.
struct FuncInfo {
  const AST::Component::FuncType *FT = nullptr;
  const Scope *Home = nullptr;
  const ResourceMap *Remap = nullptr;
};

/// Type index entry: DT null for resource bounds, ResourceId set iff resource.
struct TypeEntry {
  TypeEntry() noexcept = default;
  TypeEntry(const AST::Component::DefType *Def, const Scope *Owner) noexcept
      : DT(Def), Home(Owner) {}

  const AST::Component::DefType *DT = nullptr;
  const Scope *Home = nullptr;
  const ResourceMap *Remap = nullptr;
  // Instancetype or componenttype shape; which one is set discriminates.
  const Shape *Inst = nullptr;
  const Shape *Comp = nullptr;
  std::optional<uint32_t> ResourceId;
  // Resource naming identity; a re-export mints a new one but keeps ResourceId.
  std::optional<uint32_t> NameId;

  /// The defvaltype or functype definition this entry holds, else nullptr.
  const AST::Component::DefValType *getDefValType() const noexcept {
    return DT != nullptr && DT->isDefValType() ? &DT->getDefValType() : nullptr;
  }
  const AST::Component::FuncType *getFuncType() const noexcept {
    return DT != nullptr && DT->isFuncType() ? &DT->getFuncType() : nullptr;
  }
  bool isResource() const noexcept { return ResourceId.has_value(); }
};

/// The class of an extern, named as the binary descriptor tag reads it.
using ExternKind = AST::Component::ExternDesc::DescType;

/// Resolved externdesc; exactly one member is live, selected by Kind.
struct ExternInfo {
  ExternKind Kind = ExternKind::FuncType;
  const CoreShape *CoreMod = nullptr;
  FuncInfo Func;
  QualValType Value;
  TypeEntry Type;
  // Instance and component externs share this field; Kind discriminates.
  const Component::Shape *Shape = nullptr;
};

/// Component or instance externs; DeclScope owns the resource ids it binds.
struct Shape {
  std::vector<std::pair<std::string, ExternInfo>> Imports;
  std::map<std::string, ExternInfo, std::less<>> Exports;
  // Export names in declaration order, as the named-types rule needs.
  std::vector<std::string> ExportOrder;
  const Scope *DeclScope = nullptr;
};

/// Entry of the value index space. Linearity requires Consumed once.
struct ValueEntry {
  QualValType Type;
  bool Consumed = false;
};

/// Entry of the resource registry; the vector index is the resource identity.
struct ResourceEntry {
  const AST::Component::ResourceType *RT = nullptr;
  const Scope *Origin = nullptr;
  bool FromImport = false;
  uint32_t NameId = 0;
};

/// Import/export name record for the strong-uniqueness rule.
struct NameRecord {
  /// The record of a parsed name: as written, and canonicalized to compare.
  explicit NameRecord(const ExternName &Name) noexcept;

  std::string Original;  // the full name as written
  std::string Canonical; // the canonicalized name the rule compares
};

// ===========================================================================
// Scope: one per component / componenttype / instancetype / moduletype.
// ===========================================================================

/// The four constructs that open a scope.
enum class ScopeKind : uint8_t {
  Component,
  ComponentType,
  InstanceType,
  ModuleType
};

struct Scope {
  Scope(ScopeKind K, const Scope *P) noexcept : Kind(K), Parent(P) {}

  ScopeKind Kind;
  const Scope *Parent;

  // Core index spaces.
  std::vector<const CoreShape *> CoreModules;
  std::vector<const CoreShape *> CoreInstances;
  std::vector<CoreTypeEntry> CoreTypes;
  std::vector<const AST::SubType *> CoreFuncs;
  std::vector<const AST::TableType *> CoreTables;
  std::vector<const AST::MemoryType *> CoreMemories;
  std::vector<const AST::GlobalType *> CoreGlobals;
  std::vector<const AST::SubType *> CoreTags;

  // Component index spaces.
  std::vector<TypeEntry> Types;
  std::vector<FuncInfo> Funcs;
  std::vector<ValueEntry> Values;
  std::vector<const Shape *> Components;
  std::vector<const Shape *> Instances;

  /// Naming state of one side; imports and exports are checked separately.
  struct NameSide {
    std::vector<NameRecord> Names;
    // Plain resource label -> id, for [constructor]/[method]/[static] checks.
    std::unordered_map<std::string, uint32_t> ResourceLabels;
    std::unordered_map<uint32_t, std::string> ResourceNames;
    // Resource ids introduced by preceding imports/exports (nameability).
    std::unordered_set<uint32_t> NamedResources;
    // Introduced composite types with the scope their inner indices resolve in.
    std::unordered_map<const AST::Component::DefType *, const Scope *>
        NamedTypes;
    // Naming identities of introduced types; local references must name one.
    std::unordered_set<uint32_t> NamedIds;
  };
  NameSide ImportSide;
  NameSide ExportSide;

  // The one type every context.get and context.set of this component share.
  std::optional<ValType> ContextType;

  NameSide &getNameSide(bool IsImport) noexcept {
    return IsImport ? ImportSide : ExportSide;
  }
  const NameSide &getNameSide(bool IsImport) const noexcept {
    return IsImport ? ImportSide : ExportSide;
  }

  const TypeEntry *getType(uint32_t Idx) const noexcept {
    return Idx < Types.size() ? &Types[Idx] : nullptr;
  }
  const FuncInfo *getFunc(uint32_t Idx) const noexcept {
    return Idx < Funcs.size() ? &Funcs[Idx] : nullptr;
  }
  const Shape *getInstance(uint32_t Idx) const noexcept {
    return Idx < Instances.size() ? Instances[Idx] : nullptr;
  }
  const Shape *getComponent(uint32_t Idx) const noexcept {
    return Idx < Components.size() ? Components[Idx] : nullptr;
  }
  const CoreShape *getCoreModule(uint32_t Idx) const noexcept {
    return Idx < CoreModules.size() ? CoreModules[Idx] : nullptr;
  }
  const CoreShape *getCoreInstance(uint32_t Idx) const noexcept {
    return Idx < CoreInstances.size() ? CoreInstances[Idx] : nullptr;
  }
  const CoreTypeEntry *getCoreType(uint32_t Idx) const noexcept {
    return Idx < CoreTypes.size() ? &CoreTypes[Idx] : nullptr;
  }
  const AST::SubType *getCoreFunc(uint32_t Idx) const noexcept {
    return Idx < CoreFuncs.size() ? CoreFuncs[Idx] : nullptr;
  }

  // Index-space registration: one entry per definition, in binary order.
  void addCoreModule(const CoreShape *Mod) noexcept {
    CoreModules.push_back(Mod);
  }
  void addCoreInstance(const CoreShape *Inst) noexcept {
    CoreInstances.push_back(Inst);
  }
  void addCoreType(const CoreTypeEntry &Entry) noexcept {
    CoreTypes.push_back(Entry);
  }
  void addCoreFunc(const AST::SubType *Func) noexcept {
    CoreFuncs.push_back(Func);
  }
  /// A resolved core extern enters the index space of its kind.
  void addCoreExtern(const CoreExternInfo &Ext) noexcept {
    switch (Ext.Kind) {
    case ExternalType::Function:
      CoreFuncs.push_back(Ext.Func);
      break;
    case ExternalType::Table:
      CoreTables.push_back(Ext.Table);
      break;
    case ExternalType::Memory:
      CoreMemories.push_back(Ext.Memory);
      break;
    case ExternalType::Global:
      CoreGlobals.push_back(Ext.Global);
      break;
    case ExternalType::Tag:
      CoreTags.push_back(Ext.Func);
      break;
    }
  }
  void addType(const TypeEntry &Entry) noexcept { Types.push_back(Entry); }
  void addFunc(const FuncInfo &Func) noexcept { Funcs.push_back(Func); }
  void addValue(const QualValType &Type) noexcept {
    Values.push_back({Type, false});
  }
  void addInstance(const Shape *Inst) noexcept { Instances.push_back(Inst); }
  void addComponent(const Shape *Comp) noexcept { Components.push_back(Comp); }
  /// Consume the value at Idx, which the caller has bounds-checked, once.
  Expect<void> consumeValue(uint32_t Idx) noexcept;
};

// ===========================================================================
// The subtype relation, resolving its views through the checker.
// ===========================================================================

class Context;

class Matcher {
public:
  /// MVP subtype relation: structural equality modulo resource identity. The
  /// error is the most specific reason, ArgTypeMismatch when none applies.
  static Expect<void> matchValType(Context &Ctx, const QualValType &Sub,
                                   const QualValType &Sup,
                                   ResourceMap &Subst) noexcept;
  static Expect<void> matchValType(Context &Ctx, const TypeEntry &Sub,
                                   const TypeEntry &Sup,
                                   ResourceMap &Subst) noexcept;
  /// Subst collects the subtype ids the supertype's abstract resources bind to.
  static Expect<void> matchExtern(Context &Ctx, const ExternInfo &Sub,
                                  const ExternInfo &Sup,
                                  ResourceMap &Subst) noexcept;
  /// Core externs carry no resource identity, so no substitution or reason.
  static bool matchCoreExtern(const CoreExternInfo &Sub,
                              const CoreExternInfo &Sup) noexcept;

private:
  static Expect<void> matchFunc(Context &Ctx, const FuncInfo &Sub,
                                const FuncInfo &Sup,
                                ResourceMap &Subst) noexcept;
  static Expect<void> matchTypeEntry(Context &Ctx, const TypeEntry &Sub,
                                     const TypeEntry &Sup,
                                     ResourceMap &Subst) noexcept;
  // Instances match on exports only; components match imports contravariantly.
  static Expect<void> matchInstanceShape(Context &Ctx, const Shape &Sub,
                                         const Shape &Sup,
                                         ResourceMap &Subst) noexcept;
  static Expect<void> matchComponentShape(Context &Ctx, const Shape &Sub,
                                          const Shape &Sup,
                                          ResourceMap &Subst) noexcept;
  static Expect<void> matchPrimValType(PrimValType Sub,
                                       PrimValType Sup) noexcept;
  // A nested failure reports its position; a leaf reason inside a shape drops.
  static ErrCode getNestedFailCode(ErrCode Code) noexcept;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
