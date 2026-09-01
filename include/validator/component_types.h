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
#include "common/errcode.h"
#include "common/span.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <string>
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

/// Resolved core:importdesc / core export type. Kind discriminates. For
/// Kind == Tag, Func holds the tag signature.
struct CoreExternInfo {
  ExternalType Kind = ExternalType::Function;
  const AST::SubType *Func = nullptr;
  const AST::TableType *Table = nullptr;
  const AST::MemoryType *Memory = nullptr;
  const AST::GlobalType *Global = nullptr;
};

/// Core module or core instance: ordered imports and named exports (an
/// instance has no imports).
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

/// Remap of the resource ids a view was written against to the ids it
/// denotes here. An absent id maps to itself.
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

/// A valtype together with the scope its type indices resolve in and the
/// remap table in effect for resource identities reached through it.
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

/// Entry of the type index space. DT is null for `(sub resource)` bounds;
/// ResourceId is set iff this is a resource, and is already remapped.
struct TypeEntry {
  const AST::Component::DefType *DT = nullptr;
  const Scope *Home = nullptr;
  const ResourceMap *Remap = nullptr;
  // Instancetype / componenttype shape. Two fields, not one: a type entry has
  // no sort tag, so which one is set is the discriminator.
  const Shape *Inst = nullptr;
  const Shape *Comp = nullptr;
  std::optional<uint32_t> ResourceId;
  // Naming identity for resources: re-exports mint a fresh NameId while
  // keeping ResourceId, so matching and the named-types rule can differ.
  std::optional<uint32_t> NameId;
};

/// The class of an extern, named as the binary descriptor tag reads it. A
/// sortidx entity has no descriptor, but the same six classes.
using ExternKind = AST::Component::ExternDesc::DescType;

/// Resolved externdesc: the typed identity of an import/export/entity.
/// Exactly one member is live, selected by K.
struct ExternInfo {
  ExternKind K = ExternKind::FuncType;
  const CoreShape *CoreMod = nullptr;
  FuncInfo Func;
  QualValType Value;
  TypeEntry Type;
  // Instance and component externs share one field: K discriminates. The type
  // is qualified so that the member may carry the same name.
  const Component::Shape *Shape = nullptr;
};

/// Component or instance: ordered imports (an instance has none) and named
/// exports. DeclScope owns the resource ids the shape binds.
struct Shape {
  std::vector<std::pair<std::string, ExternInfo>> Imports;
  std::map<std::string, ExternInfo, std::less<>> Exports;
  // Export names in declaration order (introduction order matters for the
  // named-types rule).
  std::vector<std::string> ExportOrder;
  const Scope *DeclScope = nullptr;
};

/// Entry of the value index space. Linearity requires Consumed once.
struct ValueEntry {
  QualValType Type;
  bool Consumed = false;
};

/// Entry of the session-global resource registry. The vector index is the
/// resource identity.
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

struct Scope {
  enum class Kind : uint8_t {
    Component,
    ComponentType,
    InstanceType,
    ModuleType
  };

  Scope(Kind K, const Scope *P) noexcept : K(K), Parent(P) {}

  Kind K;
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

  /// Naming state of one side. Imports and exports are checked separately, so
  /// every member exists once per side.
  struct NameSide {
    std::vector<NameRecord> Names;
    // Plain resource label -> resource id, for
    // [constructor]/[method]/[static] name checks.
    std::unordered_map<std::string, uint32_t> ResourceLabels;
    std::unordered_map<uint32_t, std::string> ResourceNames;
    // Resource ids introduced by preceding imports/exports (nameability).
    std::unordered_set<uint32_t> NamedResources;
    // Composite defined types introduced by preceding imports/exports, with
    // the scope their inner indices resolve in.
    std::unordered_map<const AST::Component::DefType *, const Scope *>
        NamedTypes;
    // Naming identities of introduced types: local references must name the
    // introduced identity, not merely a structurally identical definition.
    std::unordered_set<uint32_t> NamedIds;
  };
  NameSide ImportSide;
  NameSide ExportSide;

  // All context.get and context.set built-ins of one component share one
  // type, so writes with one type never pair with reads of another.
  std::optional<ValType> ContextType;

  NameSide &nameSide(bool IsImport) noexcept {
    return IsImport ? ImportSide : ExportSide;
  }
  const NameSide &nameSide(bool IsImport) const noexcept {
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
};

/// Normalized value type: a primitive code or a composite defvaltype with its
/// resolution frame.
struct NormalVal {
  ComponentTypeCode Prim = ComponentTypeCode::TypeIndex;
  const AST::Component::DefValType *DVT = nullptr;
  const Scope *Home = nullptr;
  const ResourceMap *Remap = nullptr;
  bool Valid = false;
};

// ===========================================================================
// The type system: arenas, the resource registry, and every operation that
// is a pure function of a type view.
// ===========================================================================

class TypeSystem {
public:
  // -------------------------------------------------------------------------
  // Resource registry: index is the identity.
  // -------------------------------------------------------------------------

  uint32_t newNameId() noexcept { return NextNameId++; }

  uint32_t addResource(const AST::Component::ResourceType *RT,
                       const Scope *Origin, bool FromImport) noexcept {
    uint32_t Id = static_cast<uint32_t>(Resources.size());
    Resources.push_back({RT, Origin, FromImport, newNameId()});
    return Id;
  }

  const ResourceEntry &getResource(uint32_t Id) const noexcept {
    assuming(Id < Resources.size());
    return Resources[Id];
  }

  // -------------------------------------------------------------------------
  // Arenas for synthesized views.
  // -------------------------------------------------------------------------

  /// Compose two remaps, Inner first, into one memoized table. This is safe
  /// only because a leaf table is filled before anything composes it.
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
    auto *Node = &RemapArena.emplace_back();
    // What Inner moves lands on its Outer image. What only Outer knows keeps
    // that image. Everything else is already identity in both.
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

  CoreShape *newCoreShape() noexcept { return &CoreShapeArena.emplace_back(); }
  Shape *newShape() noexcept { return &ShapeArena.emplace_back(); }
  ResourceMap *newResourceMap() noexcept { return &RemapArena.emplace_back(); }

  /// Synthesize a core function type owned by the universe (canon lower and
  /// the resource built-ins produce core funcs with no AST-backed type).
  const AST::SubType *makeCoreFuncType(Span<const ValType> Params,
                                       Span<const ValType> Results) noexcept {
    SynthCoreTypes.push_back(
        std::make_unique<AST::SubType>(AST::FunctionType(Params, Results)));
    return SynthCoreTypes.back().get();
  }

  // -------------------------------------------------------------------------
  // Normalization: the shared substrate of every walk below.
  // -------------------------------------------------------------------------

  /// Resolve through type-index and prim-alias indirections.
  NormalVal normalizeValType(const QualValType &Q) noexcept;
  NormalVal normalizeEntry(const TypeEntry &E) const noexcept;
  /// Resolve a valtype's type index to its entry (nullptr for primitives or
  /// out-of-bounds). Composes the view's remap into Storage.
  const TypeEntry *resolveQualType(const QualValType &Q,
                                   TypeEntry &Storage) noexcept;
  /// Effective resource id behind an own/borrow handle index.
  std::optional<uint32_t> resolveResourceId(const Scope *Home,
                                            const ResourceMap *Remap,
                                            uint32_t Idx) const noexcept;

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
  uint64_t sizeOfExtern(const ExternInfo &Info) noexcept;
  uint64_t depthOfExtern(const ExternInfo &Info) noexcept;
  Expect<void> checkTypeSize(uint64_t Size) const noexcept;
  Expect<void> checkTypeDepth(uint64_t Depth) const noexcept;
  /// Both limits over one extern, the pairing every call site needs.
  Expect<void> checkTypeLimits(const ExternInfo &Info) noexcept;

  // -------------------------------------------------------------------------
  // Canonical ABI flattening (spec `flatten_functype`).
  // -------------------------------------------------------------------------

  static inline constexpr const uint32_t MaxFlatParams = 16;
  static inline constexpr const uint32_t MaxFlatAsyncParams = 4;
  static inline constexpr const uint32_t MaxFlatResults = 1;
  /// Ceiling kept while flattening: past the largest limit above, every
  /// consumer replaces the list, so more entries cannot change an outcome.
  static inline constexpr const uint32_t MaxFlatExpand = MaxFlatParams + 1;
  /// Thread-local slots addressable by context.get/context.set.
  static inline constexpr const uint32_t MaxContextSlots = 2;
  /// Elements a fixed-length list may declare, as the reference validator.
  static inline constexpr const uint32_t MaxFixedListElems = 1U << 30;

  /// Appends the flat core types of Q to Out, false on an invalid type. Ptr
  /// is the index type of the selected canonical memory.
  bool flattenValType(const QualValType &Q, std::vector<ValType> &Out,
                      const ValType &Ptr) noexcept;
  /// True iff the type transitively contains a list or string.
  bool needsMemory(const QualValType &Q) noexcept;
  /// Flatten a function type into its core signature. The flags report which
  /// side transitively needs a memory.
  Expect<AST::FunctionType> flattenFuncType(const FuncInfo &FI,
                                            const ValType &Ptr,
                                            bool &ParamsNeedMemory,
                                            bool &ResultsNeedMemory) noexcept;

  // -------------------------------------------------------------------------
  // Instantiation-time substitution. The instantiation itself lives on
  // Context, which reads the index spaces to match the arguments.
  // -------------------------------------------------------------------------

  /// Rebuild an instance view with fresh ids for the resources its own
  /// declarations introduced. Origin owns them.
  const Shape *freshenDeclaredResources(const Shape *Inst, const Scope &Origin,
                                        bool FromImport) noexcept;
  /// The export view an instantiation produces: CI's exports rebuilt against
  /// the combined substitution + freshening table.
  const Shape *rebuildInstanceExports(const Shape &CI,
                                      const ResourceMap *Node) noexcept;

  void reset() noexcept {
    NextNameId = 0;
    Resources.clear();
    CoreShapeArena.clear();
    ShapeArena.clear();
    RemapArena.clear();
    RemapCompose.clear();
    SynthCoreTypes.clear();
    TypeSizeMemo.clear();
    TypeDepthMemo.clear();
  }

private:
  void collectNormalValResources(const NormalVal &N,
                                 std::unordered_set<uint32_t> &Out) noexcept;
  uint64_t sizeOfValType(const QualValType &Q) noexcept;
  uint64_t sizeOfNormalVal(const NormalVal &N) noexcept;
  uint64_t depthOfValType(const QualValType &Q) noexcept;
  uint64_t depthOfNormalVal(const NormalVal &N) noexcept;
  // Memo that keeps shape identity within one rebuild. This is per-rebuild
  // state, so the code threads it rather than stores it.
  using ShapeMemo = std::unordered_map<const Shape *, const Shape *>;
  ExternInfo rebuildExtern(const ExternInfo &E, const ResourceMap *Node,
                           ShapeMemo &Memo) noexcept;
  TypeEntry rebuildTypeEntry(const TypeEntry &E, const ResourceMap *Node,
                             ShapeMemo &Memo) noexcept;
  const Shape *rebuildShape(const Shape *S, const ResourceMap *Node,
                            ShapeMemo &Memo) noexcept;

  uint32_t NextNameId = 0;
  std::vector<ResourceEntry> Resources;
  std::deque<CoreShape> CoreShapeArena;
  std::deque<Shape> ShapeArena;
  std::deque<ResourceMap> RemapArena;
  std::map<std::pair<const ResourceMap *, const ResourceMap *>,
           const ResourceMap *>
      RemapCompose;
  std::vector<std::unique_ptr<AST::SubType>> SynthCoreTypes;
  std::unordered_map<const void *, uint64_t> TypeSizeMemo;
  std::unordered_map<const void *, uint64_t> TypeDepthMemo;
};

// ===========================================================================
// The subtype relation. One matcher serves one match, and the caller then
// reads its resource substitution and its failure reason.
// ===========================================================================

class Matcher {
public:
  explicit Matcher(TypeSystem &Types) noexcept : Types(Types) {}

  /// MVP subtype relation: structural equality modulo resource identity.
  bool matchValType(const QualValType &Sub, const QualValType &Sup) noexcept;
  bool matchNormalVal(const NormalVal &NSub, const NormalVal &NSup) noexcept;
  bool matchExtern(const ExternInfo &Sub, const ExternInfo &Sup) noexcept;
  /// Core externs carry no resource identity, so this needs neither the
  /// substitution nor the failure reason.
  bool matchCoreExtern(const CoreExternInfo &Sub,
                       const CoreExternInfo &Sup) const noexcept;

  /// Most specific reason the innermost failing matcher recorded. A site
  /// falls back to its own generic mismatch code.
  ErrCode::Value getFailCode() const noexcept { return FailCode; }
  /// Supertype-side abstract resource ids bound to subtype ids by the match.
  const ResourceMap &getSubst() const noexcept { return Subst; }

private:
  bool matchFunc(const FuncInfo &Sub, const FuncInfo &Sup) noexcept;
  bool matchTypeEntry(const TypeEntry &Sub, const TypeEntry &Sup) noexcept;
  // Instances match on exports only. Components also match imports
  // contravariantly, so the two relations stay separate.
  bool matchInstanceShape(const Shape &Sub, const Shape &Sup) noexcept;
  bool matchComponentShape(const Shape &Sub, const Shape &Sup) noexcept;
  bool matchCoreFuncType(const AST::SubType *Sub,
                         const AST::SubType *Sup) const noexcept;
  // Drop a leaf reason. A nested failure reports its position instead.
  void clearLeafFailCode() noexcept;

  TypeSystem &Types;
  ResourceMap Subst;
  ErrCode::Value FailCode = ErrCode::Value::Success;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
