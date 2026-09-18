// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_types.h - Component type system ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component-model type system: the views resolved
/// over the AST and every operation that is a pure function of a type.
/// Context is the counterpart and depends on this file, never the reverse.
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

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
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
  std::string Original;      // the full name as written
  std::string Stripped;      // annotation removed, acronyms lowercased
  std::string StrippedExact; // annotation removed, case preserved
  std::string DottedFirst;   // first label of a dotted annotated name
  bool HasAnnotation = false;
  bool IsConstructor = false;
  bool IsPlainLabel = false;
  bool IsDottedSame = false; // [*]l.l with the same label twice
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
// The type system: owned storage, the resource registry, and type operations.
// ===========================================================================

class TypeSystem {
public:
  // -------------------------------------------------------------------------
  // Resource registry: index is the identity.
  // -------------------------------------------------------------------------

  uint32_t nextNameId() noexcept { return NextNameId++; }

  uint32_t addResource(const AST::Component::ResourceType *RT,
                       const Scope *Origin, bool FromImport) noexcept {
    uint32_t Id = static_cast<uint32_t>(Resources.size());
    Resources.push_back({RT, Origin, FromImport, nextNameId()});
    return Id;
  }

  const ResourceEntry &getResource(uint32_t Id) const noexcept {
    assuming(Id < Resources.size());
    return Resources[Id];
  }

  // -------------------------------------------------------------------------
  // Storage the type system owns. Every view holds raw pointers into it.
  // -------------------------------------------------------------------------

  /// Compose two remaps, Inner first; a leaf table is filled before composed.
  const ResourceMap *composeRemap(const ResourceMap *Outer,
                                  const ResourceMap *Inner) noexcept {
    if (Outer == nullptr) {
      return Inner;
    }
    if (Inner == nullptr) {
      return Outer;
    }
    auto Key = std::make_pair(Outer, Inner);
    auto It = RemapCompose.find(Key);
    if (It != RemapCompose.end()) {
      return It->second;
    }
    auto *Node = &OwnedRemaps.emplace_back();
    // Inner's moves land on their Outer image; Outer-only entries keep theirs.
    for (const auto &[From, To] : Inner->Map) {
      Node->Map.emplace(From, Outer->apply(To));
    }
    for (const auto &[From, To] : Outer->Map) {
      Node->Map.emplace(From, To);
    }
    RemapCompose.emplace(Key, Node);
    return Node;
  }

  /// Resource id denoted by M. If M is absent, Id denotes itself.
  uint32_t applyRemap(const ResourceMap *M, uint32_t Id) const noexcept {
    return M != nullptr ? M->apply(Id) : Id;
  }

  CoreShape *addCoreShape() noexcept { return &OwnedCoreShapes.emplace_back(); }
  Shape *addShape() noexcept { return &OwnedShapes.emplace_back(); }
  ResourceMap *addResourceMap() noexcept { return &OwnedRemaps.emplace_back(); }

  /// An owned core function type for canon lower and resource built-in funcs.
  const AST::SubType *addCoreFuncType(Span<const ValType> Params,
                                      Span<const ValType> Results) noexcept {
    OwnedCoreTypes.push_back(
        std::make_unique<AST::SubType>(AST::FunctionType(Params, Results)));
    return OwnedCoreTypes.back().get();
  }

  // -------------------------------------------------------------------------
  // Resolution: the shared substrate of every walk below.
  // -------------------------------------------------------------------------

  /// Resolve a valtype's type index to its entry in Storage, nullptr if none.
  const TypeEntry *resolveQualType(const QualValType &Q,
                                   TypeEntry &Storage) noexcept;
  /// The primitive a valtype denotes through aliases; nullopt for composites.
  std::optional<PrimValType> resolvePrimValType(const QualValType &Q) noexcept;
  /// Effective resource id behind an own/borrow handle index.
  std::optional<uint32_t> resolveResourceId(const Scope *Home,
                                            const ResourceMap *Remap,
                                            uint32_t Idx) const noexcept;

  /// Calls Func on each nested value type; stream and future payloads excluded.
  template <typename F>
  void forEachValType(const AST::Component::DefValType &D, F &&Func) const {
    if (D.isRecordTy()) {
      for (const auto &LT : D.getRecord().LabelTypes) {
        Func(LT.getValType());
      }
    } else if (D.isVariantTy()) {
      for (const auto &[Label, Ty] : D.getVariant().Cases) {
        if (Ty.has_value()) {
          Func(*Ty);
        }
      }
    } else if (D.isListTy()) {
      Func(D.getList().ValTy);
    } else if (D.isTupleTy()) {
      for (const auto &Ty : D.getTuple().Types) {
        Func(Ty);
      }
    } else if (D.isMapTy()) {
      Func(D.getMap().KeyTy);
      Func(D.getMap().ValTy);
    } else if (D.isOptionTy()) {
      Func(D.getOption().ValTy);
    } else if (D.isResultTy()) {
      const auto &R = D.getResult();
      if (R.ValTy.has_value()) {
        Func(*R.ValTy);
      }
      if (R.ErrTy.has_value()) {
        Func(*R.ErrTy);
      }
    }
  }

  // -------------------------------------------------------------------------
  // Resource-id walks.
  // -------------------------------------------------------------------------

  /// Transitive borrow check on value types.
  bool containsBorrow(const QualValType &Q) noexcept;
  /// Collect resource ids reachable from a view (for free-variable rules).
  void collectResources(const ExternInfo &Info,
                        std::unordered_set<uint32_t> &Out) noexcept;
  void collectResources(const QualValType &Q,
                        std::unordered_set<uint32_t> &Out) noexcept;
  /// True iff the id originates in S or one of its descendants.
  bool originatesIn(uint32_t Id, const Scope &S) const noexcept;

  // -------------------------------------------------------------------------
  // Effective type size and nesting depth.
  // -------------------------------------------------------------------------

  static inline constexpr const uint64_t MaxTypeSize = 1000000;
  /// The reference engine bounds value-type nesting at 100.
  static inline constexpr const uint64_t MaxTypeDepth = 100;
  uint64_t getTypeSize(const ExternInfo &Info) noexcept;
  uint64_t getTypeDepth(const ExternInfo &Info) noexcept;
  /// Both limits over one extern, the pairing every call site needs.
  Expect<void> checkTypeLimits(const ExternInfo &Info) noexcept;

  // -------------------------------------------------------------------------
  // Canonical ABI flattening (spec `flatten_functype`).
  // -------------------------------------------------------------------------

  static inline constexpr const uint32_t MaxFlatParams = 16;
  static inline constexpr const uint32_t MaxFlatAsyncParams = 4;
  static inline constexpr const uint32_t MaxFlatResults = 1;
  /// Flattening ceiling; past the largest limit every consumer replaces it.
  static inline constexpr const uint32_t MaxFlatExpand = MaxFlatParams + 1;
  /// Thread-local slots addressable by context.get/context.set.
  static inline constexpr const uint32_t MaxContextSlots = 2;
  /// Elements a fixed-length list may declare, as the reference validator.
  static inline constexpr const uint32_t MaxFixedListElems = 1U << 30;

  /// Appends Q's flat core types to Out, false if invalid; Ptr indexes memory.
  bool flattenValType(const QualValType &Q, std::vector<ValType> &Out,
                      const ValType &Ptr) noexcept;
  /// True iff the type transitively contains a list or string.
  bool needsMemory(const QualValType &Q) noexcept;
  /// Flatten a function type; the flags report which side needs a memory.
  Expect<AST::FunctionType> flattenFuncType(const FuncInfo &FI,
                                            const ValType &Ptr,
                                            bool &ParamsNeedMemory,
                                            bool &ResultsNeedMemory) noexcept;

  // -------------------------------------------------------------------------
  // Instantiation-time substitution; the instantiation itself lives on Context.
  // -------------------------------------------------------------------------

  /// Rebuild an instance view with fresh Origin-owned ids for its resources.
  const Shape *freshenDeclaredResources(const Shape *Inst, const Scope &Origin,
                                        bool FromImport) noexcept;
  /// The export view an instantiation produces: CI's exports rebuilt via Node.
  const Shape *rebuildInstanceExports(const Shape &CI,
                                      const ResourceMap *Node) noexcept;

  void reset() noexcept {
    NextNameId = 0;
    Resources.clear();
    OwnedCoreShapes.clear();
    OwnedShapes.clear();
    OwnedRemaps.clear();
    RemapCompose.clear();
    OwnedCoreTypes.clear();
    TypeSizeMemo.clear();
    TypeDepthMemo.clear();
  }

private:
  void collectResources(const TypeEntry &E,
                        std::unordered_set<uint32_t> &Out) noexcept;
  uint64_t getTypeSize(const QualValType &Q) noexcept;
  uint64_t getTypeSize(const TypeEntry &E) noexcept;
  uint64_t getTypeDepth(const QualValType &Q) noexcept;
  uint64_t getTypeDepth(const TypeEntry &E) noexcept;
  // Memo keeping shape identity within one rebuild; threaded, never stored.
  using ShapeMemo = std::unordered_map<const Shape *, const Shape *>;
  ExternInfo rebuildExtern(const ExternInfo &E, const ResourceMap *Node,
                           ShapeMemo &Memo) noexcept;
  TypeEntry rebuildTypeEntry(const TypeEntry &E, const ResourceMap *Node,
                             ShapeMemo &Memo) noexcept;
  const Shape *rebuildShape(const Shape *S, const ResourceMap *Node,
                            ShapeMemo &Memo) noexcept;

  uint32_t NextNameId = 0;
  std::vector<ResourceEntry> Resources;
  std::deque<CoreShape> OwnedCoreShapes;
  std::deque<Shape> OwnedShapes;
  std::deque<ResourceMap> OwnedRemaps;
  std::map<std::pair<const ResourceMap *, const ResourceMap *>,
           const ResourceMap *>
      RemapCompose;
  std::vector<std::unique_ptr<AST::SubType>> OwnedCoreTypes;
  std::unordered_map<const void *, uint64_t> TypeSizeMemo;
  std::unordered_map<const void *, uint64_t> TypeDepthMemo;
};

// ===========================================================================
// The subtype relation; one matcher per match, then read substitution/reason.
// ===========================================================================

class Matcher {
public:
  explicit Matcher(TypeSystem &Types) noexcept : Types(Types) {}

  /// MVP subtype relation: structural equality modulo resource identity.
  bool matchValType(const QualValType &Sub, const QualValType &Sup) noexcept;
  bool matchValType(const TypeEntry &Sub, const TypeEntry &Sup) noexcept;
  bool matchExtern(const ExternInfo &Sub, const ExternInfo &Sup) noexcept;
  /// Core externs carry no resource identity, so no substitution or reason.
  bool matchCoreExtern(const CoreExternInfo &Sub,
                       const CoreExternInfo &Sup) const noexcept;

  /// Most specific reason recorded; a site falls back to its own mismatch code.
  ErrCode::Value getFailCode() const noexcept { return FailCode; }
  /// Supertype-side abstract resource ids bound to subtype ids by the match.
  const ResourceMap &getSubst() const noexcept { return Subst; }

private:
  bool matchFunc(const FuncInfo &Sub, const FuncInfo &Sup) noexcept;
  bool matchTypeEntry(const TypeEntry &Sub, const TypeEntry &Sup) noexcept;
  // Instances match on exports only; components match imports contravariantly.
  bool matchInstanceShape(const Shape &Sub, const Shape &Sup) noexcept;
  bool matchComponentShape(const Shape &Sub, const Shape &Sup) noexcept;
  bool matchPrimValType(PrimValType Sub, PrimValType Sup) noexcept;
  // Drop a leaf reason. A nested failure reports its position instead.
  void clearLeafFailCode() noexcept;

  TypeSystem &Types;
  ResourceMap Subst;
  ErrCode::Value FailCode = ErrCode::Value::Success;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
