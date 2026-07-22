// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_context.h - Component context --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the context class used by component-model validation.
///
/// The context is organized around two ideas:
///  1. Every index-space entry is a *resolved view* whose leaves are AST
///     pointers. External types are resolved once, at definition time, into
///     per-sort info structs; raw type indices are never stored and never
///     re-resolved in a different scope.
///  2. Scopes (components and type-declaration contexts) live in an arena and
///     stay alive after they pop, so views created in inner scopes (e.g. an
///     instancetype's export table) remain valid wherever they flow.
///
/// Resource identity is a session-global registry; the index is the id.
/// Instantiation freshens defined resources and substitutes imported ones via
/// immutable remap chains attached to the AST-typed leaf views.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/sort.h"
#include "ast/component/type.h"
#include "ast/type.h"
#include "validator/component_name.h"

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace AST {
class Module;
} // namespace AST
namespace Validator {

class ComponentContext {
public:
  struct Scope;
  struct InstanceInfo;
  struct ComponentInfo;

  // ==========================================================================
  // Resolved views. Leaves are pointers into the AST (or into this context's
  // arenas for synthesized core types); all referenced storage outlives
  // validation.
  // ==========================================================================

  /// Resolved core:importdesc / core export type. Kind discriminates; Func
  /// doubles as the tag signature for Kind == Tag.
  struct CoreExternInfo {
    ExternalType Kind = ExternalType::Function;
    const AST::SubType *Func = nullptr;
    const AST::TableType *Table = nullptr;
    const AST::MemoryType *Memory = nullptr;
    const AST::GlobalType *Global = nullptr;
  };

  /// External shape of a core module: ordered imports and named exports.
  struct CoreModuleInfo {
    std::vector<std::tuple<std::string, std::string, CoreExternInfo>> Imports;
    std::map<std::string, CoreExternInfo, std::less<>> Exports;
  };

  /// Export table of a core:instance. Synthetic marks inline-export
  /// instances (diagnostics differ from instantiated ones).
  struct CoreInstanceInfo {
    std::map<std::string, CoreExternInfo, std::less<>> Exports;
    bool Synthetic = false;
  };

  /// Entry of the core:type index space: a rectype member or a moduletype.
  struct CoreTypeEntry {
    const AST::SubType *Func = nullptr;
    const CoreModuleInfo *Mod = nullptr;
  };

  /// Immutable resource-id remap. Either a leaf table or the composition
  /// "apply Inner first, then Outer"; used to view types defined in one scope
  /// through one or more instantiation boundaries.
  struct ResourceMap {
    std::unordered_map<uint32_t, uint32_t> Map;
    const ResourceMap *Inner = nullptr;
    const ResourceMap *Outer = nullptr;

    // NOLINTNEXTLINE(misc-no-recursion) -- depth = instantiation nesting.
    uint32_t apply(uint32_t Id) const noexcept {
      if (Inner != nullptr) {
        Id = Inner->apply(Id);
      }
      if (Outer != nullptr) {
        return Outer->apply(Id);
      }
      auto It = Map.find(Id);
      return It != Map.end() ? It->second : Id;
    }

    static uint32_t apply(const ResourceMap *M, uint32_t Id) noexcept {
      return M != nullptr ? M->apply(Id) : Id;
    }
  };

  /// A valtype together with the scope its type indices resolve in and the
  /// remap chain in effect for resource identities reached through it.
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

  /// Entry of the type index space. DT is null for abstract resources
  /// introduced by `(sub resource)` bounds. ResourceId is set iff the entry
  /// is a resource (already remapped; direct reads need no Remap). Inst/Comp
  /// are the materialized views for instancetype/componenttype entries.
  struct TypeEntry {
    const AST::Component::DefType *DT = nullptr;
    const Scope *Home = nullptr;
    const ResourceMap *Remap = nullptr;
    const InstanceInfo *Inst = nullptr;
    const ComponentInfo *Comp = nullptr;
    std::optional<uint32_t> ResourceId;
    // Naming identity for resources: re-exports mint a fresh NameId while
    // keeping ResourceId, so matching and the named-types rule can differ.
    std::optional<uint32_t> NameId;
  };

  /// Resolved externdesc: the typed identity of an import/export/entity.
  struct ExternInfo {
    enum class Kind : uint8_t {
      CoreModule,
      Func,
      Value,
      Type,
      Instance,
      Component
    };
    Kind K = Kind::Func;
    const CoreModuleInfo *CoreMod = nullptr;
    FuncInfo Func;
    QualValType Value;
    TypeEntry Type;
    const InstanceInfo *Inst = nullptr;
    const ComponentInfo *Comp = nullptr;
  };

  using ExternMap = std::map<std::string, ExternInfo, std::less<>>;

  /// Export table of a component-level instance. DeclScope is the scope whose
  /// resource ids this info binds (its own declarations).
  struct InstanceInfo {
    ExternMap Exports;
    // Export names in declaration order (introduction order matters for the
    // named-types rule).
    std::vector<std::string> Order;
    const Scope *DeclScope = nullptr;
    bool Synthetic = false;
  };

  /// External shape of a component. Imports stay ordered because argument
  /// matching accumulates resource substitutions left to right.
  struct ComponentInfo {
    std::vector<std::pair<std::string, ExternInfo>> Imports;
    ExternMap Exports;
    const Scope *DeclScope = nullptr;
    // True when built from a componenttype declaration (imported shapes);
    // inline bodies keep plain diagnostics on their instances.
    bool FromDecl = false;
  };

  /// Entry of the value index space; linearity requires Consumed once.
  struct ValueEntry {
    QualValType Type;
    bool Consumed = false;
  };

  // ==========================================================================
  // Resource registry: index is the identity.
  // ==========================================================================

  struct ResourceEntry {
    const AST::Component::ResourceType *RT = nullptr;
    const Scope *Origin = nullptr;
    bool FromImport = false;
    uint32_t NameId = 0;
  };

  // ==========================================================================
  // Import/export name record for the strong-uniqueness rule.
  // ==========================================================================

  struct NameRecord {
    std::string Original;      // the full name as written
    std::string Stripped;      // annotation removed, acronyms lowercased
    std::string StrippedExact; // annotation removed, case preserved
    std::string DottedFirst;   // first label of a dotted annotated name
    bool HasAnnotation = false;
    bool IsConstructor = false;
    bool IsPlainLabel = false;
    bool IsDottedSame = false; // [*]l.l with the same label twice
    bool IsPlainish = false;   // label or annotated label (not interface/dep)
  };

  /// Result of adding a name: exact duplicate vs strong-uniqueness conflict.
  enum class NameClash : uint8_t { None, Duplicate, Conflict };

  // ==========================================================================
  // Scope: one per component / componenttype / instancetype / moduletype.
  // ==========================================================================

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
    // Set for component bodies: the external view being materialized while
    // this scope's import/export sections validate.
    ComponentInfo *SelfInfo = nullptr;

    // Core index spaces.
    std::vector<const CoreModuleInfo *> CoreModules;
    std::vector<const CoreInstanceInfo *> CoreInstances;
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
    std::vector<const ComponentInfo *> Components;
    std::vector<const InstanceInfo *> Instances;

    // Naming state.
    std::vector<NameRecord> ImportNames;
    std::vector<NameRecord> ExportNames;
    // Plain resource label -> resource id, per side, for
    // [constructor]/[method]/[static] name checks.
    std::unordered_map<std::string, uint32_t> ImportResourceLabels;
    std::unordered_map<std::string, uint32_t> ExportResourceLabels;
    std::unordered_map<uint32_t, std::string> ImportResourceNames;
    std::unordered_map<uint32_t, std::string> ExportResourceNames;
    // Resource ids introduced by preceding imports/exports (nameability).
    std::unordered_set<uint32_t> ImportNamedResources;
    std::unordered_set<uint32_t> ExportNamedResources;
    // Composite defined types introduced by preceding imports/exports,
    // with the scope their inner indices resolve in.
    std::unordered_map<const AST::Component::DefType *, const Scope *>
        ImportNamedTypes;
    std::unordered_map<const AST::Component::DefType *, const Scope *>
        ExportNamedTypes;
    // Naming identities of introduced types: local references must name the
    // introduced identity, not merely a structurally identical definition.
    std::unordered_set<uint32_t> ImportNamedIds;
    std::unordered_set<uint32_t> ExportNamedIds;

    uint32_t getSortSize(AST::Component::Sort::SortType ST) const noexcept;
    uint32_t
    getCoreSortSize(AST::Component::Sort::CoreSortType ST) const noexcept;

    const TypeEntry *getType(uint32_t Idx) const noexcept {
      return Idx < Types.size() ? &Types[Idx] : nullptr;
    }
    const FuncInfo *getFunc(uint32_t Idx) const noexcept {
      return Idx < Funcs.size() ? &Funcs[Idx] : nullptr;
    }
    const InstanceInfo *getInstance(uint32_t Idx) const noexcept {
      return Idx < Instances.size() ? Instances[Idx] : nullptr;
    }
    const ComponentInfo *getComponent(uint32_t Idx) const noexcept {
      return Idx < Components.size() ? Components[Idx] : nullptr;
    }
    const CoreModuleInfo *getCoreModule(uint32_t Idx) const noexcept {
      return Idx < CoreModules.size() ? CoreModules[Idx] : nullptr;
    }
    const CoreInstanceInfo *getCoreInstance(uint32_t Idx) const noexcept {
      return Idx < CoreInstances.size() ? CoreInstances[Idx] : nullptr;
    }
    const CoreTypeEntry *getCoreType(uint32_t Idx) const noexcept {
      return Idx < CoreTypes.size() ? &CoreTypes[Idx] : nullptr;
    }
    const AST::SubType *getCoreFunc(uint32_t Idx) const noexcept {
      return Idx < CoreFuncs.size() ? CoreFuncs[Idx] : nullptr;
    }
  };

  // ==========================================================================
  // Scope stack management. Popped scopes stay alive in the arena.
  // ==========================================================================

  Scope &enterScope(Scope::Kind K) noexcept {
    const Scope *Parent = Stack.empty() ? nullptr : Stack.back();
    ScopeArena.emplace_back(K, Parent);
    Stack.push_back(&ScopeArena.back());
    return ScopeArena.back();
  }

  void exitScope() noexcept {
    assuming(!Stack.empty());
    Stack.pop_back();
  }

  Scope &top() noexcept {
    assuming(!Stack.empty());
    return *Stack.back();
  }
  const Scope &top() const noexcept {
    assuming(!Stack.empty());
    return *Stack.back();
  }

  uint32_t depth() const noexcept {
    return static_cast<uint32_t>(Stack.size());
  }

  /// The scope Ct hops up from the current one; nullptr if out of range.
  Scope *scopeUp(uint32_t Ct) noexcept {
    if (Ct >= Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - 1 - Ct];
  }

  /// The scope one level inside the outer-alias target (Ct > 0): if it is a
  /// real component, the alias crosses a component boundary.
  const Scope *scopeInsideTarget(uint32_t Ct) const noexcept {
    if (Ct == 0 || Ct > Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - Ct];
  }

  // ==========================================================================
  // Resource registry.
  // ==========================================================================

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

  // ==========================================================================
  // Arenas for synthesized views.
  // ==========================================================================

  /// Composition of two remaps (Inner first, then Outer); memoized so views
  /// resolved repeatedly share nodes. Either side may be null.
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
    Node->Inner = Inner;
    Node->Outer = Outer;
    RemapCompose.emplace(Key, Node);
    return Node;
  }

  CoreModuleInfo *newCoreModuleInfo() noexcept {
    return &CoreModuleArena.emplace_back();
  }
  CoreInstanceInfo *newCoreInstanceInfo() noexcept {
    return &CoreInstanceArena.emplace_back();
  }
  InstanceInfo *newInstanceInfo() noexcept {
    return &InstanceArena.emplace_back();
  }
  ComponentInfo *newComponentInfo() noexcept {
    return &ComponentArena.emplace_back();
  }
  ResourceMap *newResourceMap() noexcept { return &RemapArena.emplace_back(); }

  /// Synthesize a core function type owned by the context (canon lower and
  /// the resource built-ins produce core funcs with no AST-backed type).
  const AST::SubType *makeCoreFuncType(Span<const ValType> Params,
                                       Span<const ValType> Results) noexcept {
    SynthCoreTypes.push_back(
        std::make_unique<AST::SubType>(AST::FunctionType(Params, Results)));
    return SynthCoreTypes.back().get();
  }

  // ==========================================================================
  // Strong-uniqueness of import/export names.
  // ==========================================================================

  /// Builds the comparison record for a parsed name.
  static NameRecord makeNameRecord(const ComponentName &Name) noexcept;

  /// Appends N to Names, reporting how it clashes with an earlier name.
  static NameClash addUniqueName(std::vector<NameRecord> &Names,
                                 const NameRecord &N) noexcept;

  /// RAII scope push/pop for validation bodies with early returns.
  class ScopedScope {
  public:
    ScopedScope(ComponentContext &C, Scope::Kind K) noexcept
        : Ctx(C), S(C.enterScope(K)) {}
    ~ScopedScope() noexcept { Ctx.exitScope(); }
    ScopedScope(const ScopedScope &) = delete;
    ScopedScope &operator=(const ScopedScope &) = delete;
    Scope &get() noexcept { return S; }

  private:
    ComponentContext &Ctx;
    Scope &S;
  };

  /// Core modules whose function bodies still need validation (deferred to
  /// the end of the root component).
  std::vector<const AST::Module *> DeferredModules;

  void reset() noexcept {
    NextNameId = 0;
    DeferredModules.clear();
    Stack.clear();
    ScopeArena.clear();
    Resources.clear();
    CoreModuleArena.clear();
    CoreInstanceArena.clear();
    InstanceArena.clear();
    ComponentArena.clear();
    RemapArena.clear();
    RemapCompose.clear();
    SynthCoreTypes.clear();
  }

private:
  struct PtrPairHash {
    size_t operator()(const std::pair<const ResourceMap *, const ResourceMap *>
                          &P) const noexcept {
      return std::hash<const void *>{}(P.first) ^
             (std::hash<const void *>{}(P.second) << 1);
    }
  };

  uint32_t NextNameId = 0;
  std::vector<Scope *> Stack;
  std::deque<Scope> ScopeArena;
  std::vector<ResourceEntry> Resources;
  std::deque<CoreModuleInfo> CoreModuleArena;
  std::deque<CoreInstanceInfo> CoreInstanceArena;
  std::deque<InstanceInfo> InstanceArena;
  std::deque<ComponentInfo> ComponentArena;
  std::deque<ResourceMap> RemapArena;
  std::unordered_map<std::pair<const ResourceMap *, const ResourceMap *>,
                     const ResourceMap *, PtrPairHash>
      RemapCompose;
  std::vector<std::unique_ptr<AST::SubType>> SynthCoreTypes;
};

} // namespace Validator
} // namespace WasmEdge
