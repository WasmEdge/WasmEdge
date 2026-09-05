// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/component_context.h - Component state ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the state of one component under validation: the scope
/// stack, its index spaces, and everything that reads them. An operation that
/// reads an index space lives here. Every other type operation lives in
/// TypeSystem (component_types.h).
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/canonical.h"
#include "ast/component/instance.h"
#include "ast/component/sort.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "common/span.h"
#include "validator/component_name.h"
#include "validator/component_types.h"
#include "validator/formchecker.h"

#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Validator {
namespace Component {

class Context {
public:
  explicit Context(TypeSystem &Types) noexcept : Types(Types) {}

  // ==========================================================================
  // The scope stack. Scopes stay owned after they pop; validate() resets it.
  // ==========================================================================

  Scope &enterScope(ScopeKind K) noexcept {
    const Scope *Parent = Stack.empty() ? nullptr : Stack.back();
    OwnedScopes.emplace_back(K, Parent);
    Stack.push_back(&OwnedScopes.back());
    return OwnedScopes.back();
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

  /// The scope Levels hops up from the current one; nullptr if out of range.
  Scope *scopeUp(uint32_t Levels) noexcept {
    if (Levels >= Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - 1 - Levels];
  }

  /// The scope inside the outer-alias target; a component there is a crossing.
  const Scope *scopeInsideTarget(uint32_t Levels) const noexcept {
    if (Levels == 0 || Levels > Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - Levels];
  }

  // ==========================================================================
  // Index-space registration and resolution.
  // ==========================================================================

  /// Push the entity described by a resolved view into its index space.
  void defineExtern(const ExternInfo &Info) noexcept;
  /// Resolve a sortidx to its typed view in the current scope.
  Expect<ExternInfo>
  resolveSortIndex(const AST::Component::SortIndex &SI) noexcept;
  /// Build the external view of an inline core module.
  Expect<const CoreShape *> buildCoreShape(const AST::Module &Mod) noexcept;

  // ==========================================================================
  // Naming checks and the named-types rule; the only writer of NameSide.
  // ==========================================================================

  /// Extern-name grammar checks; an export rejects the import-only name kinds.
  Expect<ExternName> parseExternName(std::string_view Name,
                                     bool IsImport) const noexcept;
  /// Build the comparison record for a parsed name.
  NameRecord makeNameRecord(const ExternName &Name) const noexcept;
  /// Append N to Names, or diagnose the clash (exact or strong-uniqueness).
  Expect<void> addUniqueName(std::vector<NameRecord> &Names,
                             const NameRecord &N, bool IsImport) const noexcept;
  /// Name attributes: each kind at most once, on a plain, instance-typed name.
  Expect<void> checkNameAttributes(const ExternName &CN,
                                   Span<const std::string> Impls,
                                   Span<const std::string> ExtIds,
                                   Span<const std::string> VSuffixes,
                                   bool IsInstance) const noexcept;
  /// Annotated plainname rules ([constructor]/[method]/[static]).
  Expect<void> checkAnnotatedName(const ExternName &Name,
                                  const ExternInfo &Info,
                                  bool IsImport) noexcept;
  /// Track plain resource labels for later annotated-name checks.
  void recordResourceLabel(const ExternName &Name, const ExternInfo &Info,
                           bool IsImport) noexcept;
  /// Named-types rule: an earlier extern must introduce each named type used.
  Expect<void> checkNamedTypesRule(const ExternInfo &Info,
                                   bool IsImport) noexcept;

  // ==========================================================================
  // Declaring an import or an export; name errors precede ascription errors.
  // ==========================================================================

  /// Check an import's name and define the resolved entity it introduces.
  Expect<ExternInfo>
  defineImport(std::string_view Name, const ExternInfo &Resolved,
               Span<const std::string> Impls = {},
               Span<const std::string> ExtIds = {},
               Span<const std::string> VSuffixes = {}) noexcept;
  /// Check and record an export's name; defineExport then defines the entity.
  Expect<ExternName>
  registerExportName(std::string_view Name, bool IsInstance,
                     Span<const std::string> Impls = {},
                     Span<const std::string> ExtIds = {},
                     Span<const std::string> VSuffixes = {}) noexcept;
  /// Apply the optional ascription and define the re-exported index.
  Expect<ExternInfo>
  defineExport(const ExternName &CN, const ExternInfo &Inferred,
               const std::optional<ExternInfo> &Ascribed) noexcept;

  /// Mint fresh identities for the resources Inst declares; top() owns them.
  const Shape *freshenDeclaredResources(const Shape *Inst,
                                        bool FromImport) noexcept {
    return Types.freshenDeclaredResources(Inst, top(), FromImport);
  }

  // ==========================================================================
  // Canonical options. They resolve core index spaces, so they live here.
  // ==========================================================================

  /// The index type of the selected canonical memory.
  ValType
  getCanonPtrType(const AST::Component::Canonical &Canon) const noexcept;
  /// canonopt rules: duplicates, indices, core signatures, per-site whitelist.
  Expect<void> checkOptions(const AST::Component::Canonical &Canon,
                            bool IsLift) const noexcept;
  /// The memory and realloc options a definition (What) needs.
  Expect<void> requireOptions(const AST::Component::Canonical &Canon,
                              bool NeedMemory, bool NeedRealloc,
                              std::string_view What) const noexcept;

  // ==========================================================================
  // Instantiation.
  // ==========================================================================

  /// Match args against CI.Imports and produce the freshened export view.
  Expect<const Shape *> instantiateComponentShape(
      const Shape &CI,
      Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
          Args) noexcept;

  /// Core-module code sections deferred to the end of the root component.
  std::vector<std::pair<const AST::Module *, FormChecker>> DeferredModules;

  void reset() noexcept {
    DeferredModules.clear();
    Stack.clear();
    OwnedScopes.clear();
    Types.reset();
  }

private:
  // Named-types rule: introduceExternTypes checks (false if unmet) and records.
  bool introduceExternTypes(const ExternInfo &Info, bool IsImport) noexcept;
  bool isIntroduced(const QualValType &Q, bool IsImport) noexcept;
  bool isIntroduced(const TypeEntry &E, bool IsImport) noexcept;
  // The immediate components of the type. The extern names the type itself.
  bool areInnerTypesIntroduced(const TypeEntry &E, bool IsImport) noexcept;

  TypeSystem &Types;
  std::vector<Scope *> Stack;
  std::deque<Scope> OwnedScopes;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
