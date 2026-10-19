// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_context.h - Component checker --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the checker of a component under validation: the scope
/// stack with its index spaces, the type views it owns, and every check over
/// them, as FormChecker holds and checks the context of a core module.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/canonical.h"
#include "ast/component/instance.h"
#include "ast/component/sort.h"
#include "ast/component/type.h"
#include "ast/module.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/span.h"
#include "validator/component_name.h"
#include "validator/component_types.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {
namespace Component {

class Context {
public:
  Context() noexcept = default;

  // ==========================================================================
  // The scope stack. Scopes stay owned after they pop; validate() resets it.
  // ==========================================================================

  Scope &pushScope(ScopeKind K) noexcept {
    const Scope *Parent = Stack.empty() ? nullptr : Stack.back();
    OwnedScopes.emplace_back(K, Parent);
    Stack.push_back(&OwnedScopes.back());
    return OwnedScopes.back();
  }

  void popScope() noexcept {
    assuming(!Stack.empty());
    Stack.pop_back();
  }

  Scope &getTop() noexcept {
    assuming(!Stack.empty());
    return *Stack.back();
  }
  const Scope &getTop() const noexcept {
    assuming(!Stack.empty());
    return *Stack.back();
  }

  /// The scope Levels hops up from the current one; nullptr if out of range.
  Scope *getScope(uint32_t Levels) noexcept {
    if (Levels >= Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - 1 - Levels];
  }

  // ==========================================================================
  // Index-space registration and resolution.
  // ==========================================================================

  /// Push the entity described by a resolved view into its index space.
  void addExtern(const ExternInfo &Info) noexcept;
  /// The typed view of a sortidx in the current scope.
  Expect<ExternInfo> getExtern(const AST::Component::SortIndex &SI) noexcept;
  /// A valtype in the current scope: its type index must name a defvaltype.
  Expect<void> validate(const ComponentValType &VT) const noexcept;

  // ==========================================================================
  // Naming checks and the named-types rule; the only writer of NameSide.
  // ==========================================================================

  /// Extern-name grammar checks; an export rejects the import-only name kinds.
  Expect<ExternName> parseExternName(std::string_view Name,
                                     bool IsImport) const noexcept;
  /// Append N to Names, or diagnose the clash (exact or strong-uniqueness).
  Expect<void> addUniqueName(std::vector<NameRecord> &Names,
                             const NameRecord &N, bool IsImport) const noexcept;
  /// Name attributes: a valid versionsuffix, and `implements` only on a
  /// plain, instance-typed name.
  Expect<void> checkNameAttributes(const ExternName &CN,
                                   Span<const std::string> Impls,
                                   Span<const std::string> VSuffixes,
                                   bool IsInstance) const noexcept;
  /// Annotated plainname rules ([constructor]/[method]/[static]).
  Expect<void> checkAnnotatedName(const ExternName &Name,
                                  const ExternInfo &Info,
                                  bool IsImport) noexcept;
  /// Track plain resource labels for later annotated-name checks.
  void addResourceLabel(const ExternName &Name, const ExternInfo &Info,
                        bool IsImport) noexcept;
  /// Named-types rule: an earlier extern must introduce each named type used.
  Expect<void> checkNamedTypesRule(const ExternInfo &Info,
                                   bool IsImport) noexcept;

  // ==========================================================================
  // Declaring an import or an export; name errors precede ascription errors.
  // ==========================================================================

  /// Check an import's name and add the resolved entity it introduces.
  Expect<ExternInfo> addImport(std::string_view Name,
                               const ExternInfo &Resolved,
                               Span<const std::string> Impls = {},
                               Span<const std::string> VSuffixes = {}) noexcept;
  /// Check and record an export's name; addExport then adds the entity.
  Expect<ExternName>
  registerExportName(std::string_view Name, bool IsInstance,
                     Span<const std::string> Impls = {},
                     Span<const std::string> VSuffixes = {}) noexcept;
  /// Apply the optional ascription and add the re-exported index.
  Expect<ExternInfo>
  addExport(const ExternName &CN, const ExternInfo &Inferred,
            const std::optional<ExternInfo> &Ascribed) noexcept;

  // ==========================================================================
  // Canonical options. They resolve core index spaces.
  // ==========================================================================

  /// The index type of the selected canonical memory.
  ValType
  getCanonPtrType(const AST::Component::Canonical &Canon) const noexcept;
  /// canonopt rules: duplicates, indices, core signatures, per-site whitelist.
  Expect<void> checkOptions(const AST::Component::Canonical &Canon,
                            bool IsLift) const noexcept;
  /// The memory and realloc options a definition (What) needs.
  Expect<void> checkRequiredOptions(const AST::Component::Canonical &Canon,
                                    bool NeedMemory, bool NeedRealloc,
                                    std::string_view What) const noexcept;

  // ==========================================================================
  // Instantiation and the substitution applied to the views it creates.
  // ==========================================================================

  /// Match args against CI.Imports and produce the freshened export view.
  Expect<const Shape *> instantiateComponentShape(
      const Shape &CI,
      Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
          Args) noexcept;
  /// Copy an instance view with fresh getTop()-owned ids for its resources.
  const Shape *copyDeclaredResources(const Shape *Inst,
                                     bool FromImport) noexcept;
  /// The export view an instantiation produces: CI's exports copied under Node.
  const Shape *copyInstanceExports(const Shape &CI,
                                   const ResourceMap *Node) noexcept;

  // ==========================================================================
  // Resource registry: index is the identity.
  // ==========================================================================

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

  // ==========================================================================
  // Storage the checker owns. Every view holds raw pointers into it.
  // ==========================================================================

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
    auto It = ComposedRemaps.find(Key);
    if (It != ComposedRemaps.end()) {
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
    ComposedRemaps.emplace(Key, Node);
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

  // ==========================================================================
  // Resolution: the shared substrate of every walk below.
  // ==========================================================================

  /// The entry a valtype's index names, written to Storage; nullptr if none.
  const TypeEntry *getTypeEntry(const QualValType &Q,
                                TypeEntry &Storage) noexcept;
  /// The primitive a valtype denotes through aliases; nullopt for composites.
  std::optional<AST::Component::PrimValType>
  getPrimValType(const QualValType &Q) noexcept;
  /// Effective resource id behind an own/borrow handle index.
  std::optional<uint32_t> getResourceId(const Scope *Home,
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

  // ==========================================================================
  // Resource-id walks.
  // ==========================================================================

  /// Transitive borrow check on value types.
  bool hasBorrow(const QualValType &Q) noexcept;
  /// Collect resource ids reachable from a view (for free-variable rules).
  void collectResources(const ExternInfo &Info,
                        std::unordered_set<uint32_t> &Out) noexcept;
  void collectResources(const QualValType &Q,
                        std::unordered_set<uint32_t> &Out) noexcept;
  /// True iff the id is declared in S or one of its descendants.
  bool isDeclaredIn(uint32_t Id, const Scope &S) const noexcept;

  // ==========================================================================
  // Effective type size and nesting depth.
  // ==========================================================================

  static inline constexpr const uint64_t MaxTypeSize = 1000000;
  /// The reference engine bounds value-type nesting at 100.
  static inline constexpr const uint64_t MaxTypeDepth = 100;
  uint64_t getTypeSize(const ExternInfo &Info) noexcept;
  uint64_t getTypeDepth(const ExternInfo &Info) noexcept;
  /// Both limits over one extern, the pairing every call site needs.
  Expect<void> checkTypeLimits(const ExternInfo &Info) noexcept;

  // ==========================================================================
  // Canonical ABI layout (spec `elem_size` and `alignment`, 64-bit pointers).
  // ==========================================================================

  /// Every defvaltype's element size must stay below this bound.
  static inline constexpr const uint64_t MaxElemSize = UINT64_C(1) << 28;
  /// The element size and alignment of a value type.
  std::pair<uint64_t, uint64_t> getElemLayout(const QualValType &Q) noexcept;
  std::pair<uint64_t, uint64_t>
  getElemLayout(const AST::Component::DefValType &D, const Scope *Home,
                const ResourceMap *Remap) noexcept;

  // ==========================================================================
  // Canonical ABI flattening (spec `flatten_functype`).
  // ==========================================================================

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

  void reset() noexcept {
    Stack.clear();
    OwnedScopes.clear();
    NextNameId = 0;
    Resources.clear();
    OwnedCoreShapes.clear();
    OwnedShapes.clear();
    OwnedRemaps.clear();
    ComposedRemaps.clear();
    OwnedCoreTypes.clear();
    TypeSizes.clear();
    TypeDepths.clear();
    ElemLayouts.clear();
  }

private:
  // ==========================================================================
  // The named-types rule: introduceExternTypes checks and records an extern.
  // ==========================================================================

  bool introduceExternTypes(const ExternInfo &Info, bool IsImport) noexcept;
  bool isIntroduced(const QualValType &Q, bool IsImport) noexcept;
  bool isIntroduced(const TypeEntry &E, bool IsImport) noexcept;
  // The immediate components of the type. The extern names the type itself.
  bool areInnerTypesIntroduced(const TypeEntry &E, bool IsImport) noexcept;

  // ==========================================================================
  // The leaves of the walks above.
  // ==========================================================================

  void collectResources(const TypeEntry &E,
                        std::unordered_set<uint32_t> &Out) noexcept;
  uint64_t getTypeSize(const QualValType &Q) noexcept;
  uint64_t getTypeSize(const TypeEntry &E) noexcept;
  uint64_t getTypeDepth(const QualValType &Q) noexcept;
  uint64_t getTypeDepth(const TypeEntry &E) noexcept;

  // ==========================================================================
  // Copies of views under a remap; Copies maps each shape met to its copy.
  // ==========================================================================

  ExternInfo
  copyExtern(const ExternInfo &E, const ResourceMap *Node,
             std::unordered_map<const Shape *, const Shape *> &Copies) noexcept;
  TypeEntry copyTypeEntry(
      const TypeEntry &E, const ResourceMap *Node,
      std::unordered_map<const Shape *, const Shape *> &Copies) noexcept;
  const Shape *
  copyShape(const Shape *S, const ResourceMap *Node,
            std::unordered_map<const Shape *, const Shape *> &Copies) noexcept;

  std::vector<Scope *> Stack;
  std::deque<Scope> OwnedScopes;
  uint32_t NextNameId = 0;
  std::vector<ResourceEntry> Resources;
  std::deque<CoreShape> OwnedCoreShapes;
  std::deque<Shape> OwnedShapes;
  std::deque<ResourceMap> OwnedRemaps;
  std::map<std::pair<const ResourceMap *, const ResourceMap *>,
           const ResourceMap *>
      ComposedRemaps;
  std::vector<std::unique_ptr<AST::SubType>> OwnedCoreTypes;
  std::unordered_map<const void *, uint64_t> TypeSizes;
  std::unordered_map<const void *, uint64_t> TypeDepths;
  std::unordered_map<const void *, std::pair<uint64_t, uint64_t>> ElemLayouts;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
