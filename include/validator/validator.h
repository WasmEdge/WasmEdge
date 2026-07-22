// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/validator/validator.h - validator class definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the validator class, which controls
/// the flow of WASM validation.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/configure.h"
#include "validator/component_context.h"
#include "validator/formchecker.h"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace WasmEdge {
namespace Validator {

/// Validator flow control class.
class Validator {
public:
  Validator(const Configure &Conf) noexcept;
  ~Validator() noexcept = default;

  /// Validate AST::Module.
  Expect<void> validate(const AST::Module &Mod);
  /// Validate AST::Component.
  Expect<void> validate(const AST::Component::Component &Comp) noexcept;

private:
  /// \name Validate WASM AST nodes
  /// @{
  // Validate AST::Types
  Expect<void> validate(const AST::SubType &Type, uint32_t OwnTypeIdx,
                        std::vector<uint32_t> &SubTypeDepthMap);
  Expect<void> validate(const AST::Limit &Lim);
  Expect<void> validate(const AST::TableType &Tab);
  Expect<void> validate(const AST::MemoryType &Mem);
  Expect<void> validate(const AST::GlobalType &Glob);
  // Validate AST::Segments
  Expect<void> validate(const AST::TableSegment &TabSeg);
  Expect<void> validate(const AST::GlobalSegment &GlobSeg);
  Expect<void> validate(const AST::ElementSegment &ElemSeg);
  Expect<void> validate(const AST::CodeSegment &CodeSeg,
                        const uint32_t TypeIdx);
  Expect<void> validate(const AST::DataSegment &DataSeg);
  // Validate AST::Desc
  Expect<void> validate(const AST::ImportDesc &ImpDesc);
  Expect<void> validate(const AST::ExportDesc &ExpDesc);
  // Validate AST::Sections
  Expect<void> validate(const AST::TypeSection &TypeSec);
  Expect<void> validate(const AST::ImportSection &ImportSec);
  Expect<void> validate(const AST::FunctionSection &FuncSec);
  Expect<void> validate(const AST::TableSection &TabSec);
  Expect<void> validate(const AST::MemorySection &MemSec);
  Expect<void> validate(const AST::GlobalSection &GlobSec);
  Expect<void> validate(const AST::ElementSection &ElemSec);
  Expect<void> validate(const AST::CodeSection &CodeSec);
  Expect<void> validate(const AST::DataSection &DataSec);
  Expect<void> validate(const AST::StartSection &StartSec);
  Expect<void> validate(const AST::ExportSection &ExportSec);
  Expect<void> validate(const AST::TagSection &TagSec);
  // Validate const expression
  Expect<void> validateConstExpr(AST::InstrView Instrs,
                                 Span<const ValType> Returns);
  /// @}

  /// \name Validate Component Model AST nodes
  /// @{
  // Shorthands for context view types used in signatures below.
  using CtxView = ComponentContext;

  // Validate a component body in a fresh scope (nested components recurse)
  // and materialize its external view.
  Expect<const ComponentContext::ComponentInfo *>
  validateComponent(const AST::Component::Component &Comp) noexcept;
  // Validate component sections
  Expect<void>
  validate(const AST::Component::CoreModuleSection &ModSec) noexcept;
  Expect<void>
  validate(const AST::Component::CoreInstanceSection &InstSec) noexcept;
  Expect<void>
  validate(const AST::Component::CoreTypeSection &TypeSec) noexcept;
  Expect<void>
  validate(const AST::Component::ComponentSection &CompSec) noexcept;
  Expect<void>
  validate(const AST::Component::InstanceSection &InstSec) noexcept;
  Expect<void> validate(const AST::Component::AliasSection &AliasSec) noexcept;
  Expect<void> validate(const AST::Component::TypeSection &TypeSec) noexcept;
  Expect<void> validate(const AST::Component::CanonSection &CanonSec) noexcept;
  Expect<void> validate(const AST::Component::StartSection &StartSec) noexcept;
  Expect<void> validate(const AST::Component::ImportSection &ImpSec) noexcept;
  Expect<void> validate(const AST::Component::ExportSection &ExpSec) noexcept;
  Expect<void> validate(const AST::Component::ValueSection &ValSec) noexcept;
  // Validate component core:instance and instance
  Expect<void> validate(const AST::Component::CoreInstance &Inst) noexcept;
  Expect<void> validate(const AST::Component::Instance &Inst) noexcept;
  // Validate component core:alias and alias
  Expect<void> validate(const AST::Component::CoreAlias &Alias) noexcept;
  Expect<void> validate(const AST::Component::Alias &Alias) noexcept;
  // Validate component core:deftype and deftype
  Expect<void> validate(const AST::Component::CoreDefType &DType) noexcept;
  Expect<void> validate(const AST::Component::DefType &DType) noexcept;
  // Validate component canonical
  Expect<void> validate(const AST::Component::Canonical &Canon) noexcept;
  Expect<void> validateCanonOptions(const AST::Component::Canonical &Canon,
                                    bool IsLift) noexcept;
  Expect<void>
  validateCanonLift(const AST::Component::Canonical &Canon) noexcept;
  Expect<void>
  validateCanonLower(const AST::Component::Canonical &Canon) noexcept;
  Expect<void>
  validateCanonResourceNew(const AST::Component::Canonical &Canon) noexcept;
  Expect<void>
  validateCanonResourceRep(const AST::Component::Canonical &Canon) noexcept;
  Expect<void>
  validateCanonResourceDrop(const AST::Component::Canonical &Canon) noexcept;
  Expect<void>
  validateCanonAsyncBuiltin(const AST::Component::Canonical &Canon) noexcept;
  // Validate component import/export
  Expect<void> validate(const AST::Component::Import &Im) noexcept;
  Expect<void> validate(const AST::Component::Export &Ex) noexcept;
  // Resolve + validate descriptors into typed views. Sub-resource type
  // bounds allocate a fresh abstract resource id (import- or export-side).
  Expect<CtxView::CoreExternInfo>
  validate(const AST::Component::CoreImportDesc &Desc) noexcept;
  Expect<CtxView::ExternInfo> validate(const AST::Component::ExternDesc &Desc,
                                       bool ImportSide) noexcept;
  // Push the entity described by a resolved view into its index space.
  void defineExtern(const CtxView::ExternInfo &Info) noexcept;
  // Validate an import's name and define the entity it introduces.
  Expect<CtxView::ExternInfo>
  defineImport(std::string_view Name, const AST::Component::ExternDesc &Desc,
               Span<const std::string> Impls = {}) noexcept;
  // Validate an export's name/uniqueness/annotations, apply the optional
  // ascription, and define the re-exported index.
  Expect<CtxView::ExternInfo>
  defineExport(std::string_view Name, const CtxView::ExternInfo &Inferred,
               const std::optional<AST::Component::ExternDesc> &Ascribed,
               Span<const std::string> Impls = {}) noexcept;
  // Import/export name grammar + position checks.
  Expect<ComponentName> parseImportName(std::string_view Name) noexcept;
  // The `implements` annotation: values must be interface names, the
  // annotated name must be plain, and the extern must be instance-typed.
  Expect<void> checkImplements(const ComponentName &CN,
                               Span<const std::string> Impls,
                               bool IsInstance) noexcept;
  Expect<ComponentName> parseExportName(std::string_view Name) noexcept;
  // Type declaration bodies -> materialized views.
  Expect<const CtxView::CoreModuleInfo *>
  validateModuleType(Span<const AST::Component::CoreModuleDecl> Decls) noexcept;
  Expect<const CtxView::InstanceInfo *>
  validateInstanceType(const AST::Component::InstanceType &IT) noexcept;
  Expect<const CtxView::ComponentInfo *>
  validateComponentType(const AST::Component::ComponentType &CT) noexcept;
  Expect<void> validate(const AST::Component::InstanceDecl &Decl,
                        CtxView::ExternMap &Exports) noexcept;
  Expect<void> validate(const AST::Component::ComponentDecl &Decl,
                        CtxView::ComponentInfo &Info) noexcept;
  // Validate component value types and type definitions
  Expect<void> validate(const ComponentValType &VT) noexcept;
  Expect<void> validate(const AST::Component::DefValType &DVT) noexcept;
  Expect<void> validate(const AST::Component::FuncType &FT) noexcept;
  Expect<void> validate(const AST::Component::ResourceType &RT) noexcept;

  // Build the external view of an inline core module.
  Expect<const CtxView::CoreModuleInfo *>
  buildCoreModuleInfo(const AST::Module &Mod) noexcept;

  // Annotated plainname rules ([constructor]/[method]/[static]).
  Expect<void> checkAnnotatedName(const ComponentName &Name,
                                  const CtxView::ExternInfo &Info,
                                  bool IsImport) noexcept;
  // Track plain resource labels for later annotated-name checks.
  void recordResourceLabel(const ComponentName &Name,
                           const CtxView::ExternInfo &Info,
                           bool IsImport) noexcept;
  // Named-types rule: flags/enums/records/variants/resources referenced by
  // an extern must have been introduced by a preceding import/export.
  Expect<void> checkResourceNameability(const CtxView::ExternInfo &Info,
                                        bool IsImport) noexcept;
  bool namedExtern(const CtxView::ExternInfo &Info, bool IsImport) noexcept;
  bool namedValType(const CtxView::QualValType &Q, bool IsImport) noexcept;
  bool namedTypeEntry(const CtxView::TypeEntry &E, bool IsImport) noexcept;
  bool allValTypesNamed(const CtxView::TypeEntry &E, bool IsImport) noexcept;
  const CtxView::InstanceInfo *
  freshenDeclaredResources(const CtxView::InstanceInfo *Inst,
                           bool FromImport) noexcept;
  // Composite defined types referenced by an extern's type.
  void collectNamedTypes(
      const CtxView::QualValType &Q, bool IncludeTop,
      const CtxView::Scope *Binder,
      std::unordered_set<const AST::Component::DefType *> &Out) noexcept;
  void collectNamedTypes(
      const CtxView::ExternInfo &Info, bool IncludeTop,
      const CtxView::Scope *Binder,
      std::unordered_set<const AST::Component::DefType *> &Out) noexcept;
  // Effective type-size limit (prevents exponential type blowup).
  static inline constexpr const uint64_t MaxTypeSize = 1000000;
  std::unordered_map<const void *, uint64_t> TypeSizeMemo;
  std::unordered_map<const void *, uint64_t> TypeDepthMemo;
  uint64_t sizeOfValType(const CtxView::QualValType &Q) noexcept;
  uint64_t depthOfValType(const CtxView::QualValType &Q) noexcept;
  uint64_t depthOfExtern(const CtxView::ExternInfo &Info) noexcept;
  uint64_t sizeOfExtern(const CtxView::ExternInfo &Info) noexcept;
  Expect<void> checkTypeSize(uint64_t Size) noexcept;
  Expect<void> checkTypeDepth(uint64_t Depth) noexcept;

  /// \name Structural matching (MVP: equality modulo resource identity).
  /// Substitution maps supertype-side abstract resource ids to subtype ids.
  /// @{
  using ResourceSubst = std::unordered_map<uint32_t, uint32_t>;
  // Most-specific reason recorded by the innermost failing matcher; sites
  // report it when set, falling back to their generic mismatch code.
  ErrCode::Value MatchWhy = ErrCode::Value::Success;
  void resetNestedMatchWhy() noexcept;
  bool matchValType(const CtxView::QualValType &Sub,
                    const CtxView::QualValType &Sup,
                    ResourceSubst &Subst) noexcept;
  bool matchFunc(const CtxView::FuncInfo &Sub, const CtxView::FuncInfo &Sup,
                 ResourceSubst &Subst) noexcept;
  bool matchTypeEntry(const CtxView::TypeEntry &Sub,
                      const CtxView::TypeEntry &Sup,
                      ResourceSubst &Subst) noexcept;
  bool matchInstanceInfo(const CtxView::InstanceInfo &Sub,
                         const CtxView::InstanceInfo &Sup,
                         ResourceSubst &Subst) noexcept;
  bool matchComponentInfo(const CtxView::ComponentInfo &Sub,
                          const CtxView::ComponentInfo &Sup,
                          ResourceSubst &Subst) noexcept;
  bool matchExtern(const CtxView::ExternInfo &Sub,
                   const CtxView::ExternInfo &Sup,
                   ResourceSubst &Subst) noexcept;
  bool matchCoreExtern(const CtxView::CoreExternInfo &Sub,
                       const CtxView::CoreExternInfo &Sup) const noexcept;
  bool matchCoreFuncType(const AST::SubType *Sub,
                         const AST::SubType *Sup) const noexcept;
  /// @}

  // Resolve a valtype's type index to its entry (nullptr for primitives or
  // out-of-bounds). Composes the view's remap into Storage.
  const CtxView::TypeEntry *
  resolveQualType(const CtxView::QualValType &Q,
                  CtxView::TypeEntry &Storage) noexcept;

  // Normalized value type: a primitive code or a composite defvaltype with
  // its resolution frame. Shared by the matchers and walkers.
  struct NormalVal {
    ComponentTypeCode Prim = ComponentTypeCode::TypeIndex;
    const AST::Component::DefValType *DVT = nullptr;
    const CtxView::Scope *Home = nullptr;
    const CtxView::ResourceMap *Remap = nullptr;
    bool Valid = false;
  };
  NormalVal normalizeValType(const CtxView::QualValType &Q) noexcept;
  NormalVal normalizeEntry(const CtxView::TypeEntry &E) const noexcept;
  bool matchNormalVal(const NormalVal &A, const NormalVal &B,
                      ResourceSubst &Subst) noexcept;
  void collectResourcesNormal(const NormalVal &N,
                              std::unordered_set<uint32_t> &Out) noexcept;

  // Transitive borrow check on value types.
  bool containsBorrow(const CtxView::QualValType &Q) noexcept;
  // Collect resource ids reachable from a view (for free-variable rules).
  void collectResources(const CtxView::ExternInfo &Info,
                        std::unordered_set<uint32_t> &Out) noexcept;
  void collectResources(const CtxView::QualValType &Q,
                        std::unordered_set<uint32_t> &Out) noexcept;
  // True iff the id originates in Scope or one of its descendants.
  bool originatesIn(uint32_t Id, const CtxView::Scope &Scope) const noexcept;

  // Instantiation: match args against CI.Imports, then produce the
  // instance's export view with substituted + freshened resource ids.
  Expect<const CtxView::InstanceInfo *> instantiateComponentInfo(
      const CtxView::ComponentInfo &CI,
      Span<const AST::Component::InstantiateArg<AST::Component::SortIndex>>
          Args) noexcept;
  // Resolve a sortidx to its typed view in the current scope.
  Expect<CtxView::ExternInfo>
  resolveSortIndex(const AST::Component::SortIndex &SI) noexcept;

  /// \name Canonical ABI flattening (validator-side, spec `flatten_functype`).
  /// @{
  static inline constexpr const uint32_t MaxFlatParams = 16;
  static inline constexpr const uint32_t MaxFlatAsyncParams = 4;
  static inline constexpr const uint32_t MaxFlatResults = 1;
  // Appends the flat core types of Q to Out; false on invalid type. Ptr is
  // the index type of the selected canonical memory (pointer-bearing slots
  // widen to i64 under a 64-bit memory).
  bool flattenValType(const CtxView::QualValType &Q, std::vector<ValType> &Out,
                      const ValType &Ptr) noexcept;
  // True iff the type transitively contains a list or string.
  bool needsMemory(const CtxView::QualValType &Q) noexcept;
  // True iff the canonical `memory` option selects a 64-bit memory.
  bool canonMemoryIs64(const AST::Component::Canonical &Canon) noexcept;
  /// @}
  /// @}

  /// Memory page limit for WASM32 and WASM64
  static inline const uint64_t LIMIT_MEMORYTYPE_LIM64 = UINT64_C(1) << 48;
  static inline const uint32_t LIMIT_MEMORYTYPE_LIM32 = UINT32_C(1) << 16;
  /// Proposal configure
  const Configure Conf;
  /// Formal checker
  FormChecker Checker;
  /// Skip function-body checking (component validation defers bodies to the
  /// end, matching the reference validator's streaming order).
  bool SkipFuncBodies = false;
  /// Context for Component validation
  ComponentContext CompCtx;
};

} // namespace Validator
} // namespace WasmEdge
