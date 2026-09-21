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

  /// The type system these index spaces resolve against.
  TypeSystem &types() noexcept { return Types; }

  // ==========================================================================
  // The scope stack. Scopes stay in the arena after they pop; an unbalanced
  // exit on an error path is harmless because validate() resets the stack.
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

  /// The scope Levels hops up from the current one. Returns nullptr if Levels
  /// is out of range.
  Scope *scopeUp(uint32_t Levels) noexcept {
    if (Levels >= Stack.size()) {
      return nullptr;
    }
    return Stack[Stack.size() - 1 - Levels];
  }

  /// The scope one level inside the outer-alias target (Levels > 0): if it is
  /// a real component, the alias crosses a component boundary.
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
  // Naming: grammar, strong uniqueness, `implements`, annotated plainnames,
  // and the named-types rule. This is the only writer of NameSide.
  // ==========================================================================

  /// Extern-name grammar and position checks. An export rejects the
  /// import-only dep, url, and hash names.
  Expect<ExternName> parseExternName(std::string_view Name,
                                     bool IsImport) const noexcept;
  /// Build the comparison record for a parsed name.
  NameRecord makeNameRecord(const ExternName &Name) const noexcept;
  /// Append N to Names, or diagnose the clash. An exact duplicate is always
  /// a name conflict, a strong-uniqueness one names the declarator side.
  Expect<void> addUniqueName(std::vector<NameRecord> &Names,
                             const NameRecord &N, bool IsImport) const noexcept;
  /// The name attributes. Each kind may appear at most once. An
  /// `implements` value must be an interface name, and the annotated name
  /// must be plain and instance-typed.
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
  /// Named-types rule. A preceding import or export must introduce every
  /// flags, enum, record, variant, or resource that an extern refers to.
  Expect<void> checkNamedTypesRule(const ExternInfo &Info,
                                   bool IsImport) noexcept;

  // ==========================================================================
  // Declaring an import or an export. The export path is split so name
  // errors precede ascription errors, so the checks above stay callable.
  // ==========================================================================

  /// Check an import's name and define the (already resolved) entity it
  /// introduces.
  Expect<ExternInfo>
  defineImport(std::string_view Name, const ExternInfo &Resolved,
               Span<const std::string> Impls = {},
               Span<const std::string> ExtIds = {},
               Span<const std::string> VSuffixes = {}) noexcept;
  /// Check and record the name of an export. defineExport then defines the
  /// entity, with its optional ascription.
  Expect<ExternName>
  registerExportName(std::string_view Name, bool IsInstance,
                     Span<const std::string> Impls = {},
                     Span<const std::string> ExtIds = {},
                     Span<const std::string> VSuffixes = {}) noexcept;
  /// Apply the optional ascription and define the re-exported index.
  Expect<ExternInfo>
  defineExport(const ExternName &CN, const ExternInfo &Inferred,
               const std::optional<ExternInfo> &Ascribed) noexcept;

  /// Mint fresh identities for the resources that the own declarations of an
  /// instance shape introduced. The current scope owns them.
  const Shape *freshenDeclaredResources(const Shape *Inst,
                                        bool FromImport) noexcept {
    return Types.freshenDeclaredResources(Inst, top(), FromImport);
  }

  // ==========================================================================
  // Canonical options. The flattening they parameterize lives on
  // TypeSystem; these resolve core index spaces, so they live here.
  // ==========================================================================

  /// True iff the canonical definition carries the given option.
  static bool hasOption(const AST::Component::Canonical &Canon,
                        ComponentCanonOptCode Code) noexcept;
  /// True iff the canonical `memory` option selects a 64-bit memory.
  bool canonMemoryIs64(const AST::Component::Canonical &Canon) const noexcept;
  /// The index type of the selected canonical memory.
  ValType canonPtrType(const AST::Component::Canonical &Canon) const noexcept {
    return ValType(canonMemoryIs64(Canon) ? TypeCode::I64 : TypeCode::I32);
  }
  /// canonopt structural rules: duplicates, index validity, the referenced
  /// core function signatures, and the per-site option whitelist.
  Expect<void> checkOptions(const AST::Component::Canonical &Canon,
                            bool IsLift) const noexcept;

  // ==========================================================================
  // Instantiation.
  // ==========================================================================

  /// Match args against CI.Imports, then produce the instance's export view
  /// with substituted and freshened resource ids.
  Expect<const Shape *> instantiateComponentShape(
      const Shape &CI,
      Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
          Args) noexcept;

  /// Core-module code sections deferred to the end of the root component,
  /// each with the checker state captured at its definition.
  std::vector<std::pair<const AST::Module *, FormChecker>> DeferredModules;

  void reset() noexcept {
    DeferredModules.clear();
    Stack.clear();
    ScopeArena.clear();
    Types.reset();
  }

private:
  // Named-types rule. introduceExternTypes both checks (false if a referenced
  // type was not introduced first) and records what it introduces.
  bool introduceExternTypes(const ExternInfo &Info, bool IsImport) noexcept;
  bool isValTypeIntroduced(const QualValType &Q, bool IsImport) noexcept;
  bool isTypeEntryIntroduced(const TypeEntry &E, bool IsImport) noexcept;
  // The immediate components of the type. The extern names the type itself.
  bool areInnerTypesIntroduced(const TypeEntry &E, bool IsImport) noexcept;

  TypeSystem &Types;
  std::vector<Scope *> Stack;
  std::deque<Scope> ScopeArena;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
