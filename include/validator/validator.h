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
#include <vector>

namespace WasmEdge {
namespace Validator {

/// Validator flow control class.
class Validator {
public:
  Validator(const Configure &Conf) noexcept : Conf(Conf) {}
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
                        std::vector<uint32_t> &SubTypeDepthMap,
                        const std::vector<const AST::SubType *> &TypeVec);
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
  // Validate a component body in a fresh scope, into the caller's shape.
  Expect<void> validate(const AST::Component::Component &Comp,
                        Component::Shape &Out) noexcept;
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
  // Imports and exports accumulate into the component's own external type.
  Expect<void> validate(const AST::Component::ImportSection &ImpSec,
                        Component::Shape &Out) noexcept;
  Expect<void> validate(const AST::Component::ExportSection &ExpSec,
                        Component::Shape &Out) noexcept;
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
  // Validate component import/export
  Expect<void> validate(const AST::Component::Import &Im,
                        Component::Shape &Out) noexcept;
  Expect<void> validate(const AST::Component::Export &Ex,
                        Component::Shape &Out) noexcept;
  // Resolve + validate descriptors into typed views. Sub-resource type
  // bounds allocate a fresh abstract resource id (import- or export-side).
  Expect<void> validate(const AST::Component::CoreImportDesc &Desc,
                        Component::CoreExternInfo &Out) noexcept;
  Expect<void> validate(const AST::Component::ExternDesc &Desc, bool IsImport,
                        Component::ExternInfo &Out) noexcept;
  // Validate type declaration bodies into the caller-provided shape.
  Expect<void> validate(Span<const AST::Component::CoreModuleDecl> Decls,
                        Component::CoreShape &Out) noexcept;
  Expect<void> validate(const AST::Component::InstanceType &IT,
                        Component::Shape &Out) noexcept;
  Expect<void> validate(const AST::Component::ComponentType &CT,
                        Component::Shape &Out) noexcept;
  Expect<void> validate(const AST::Component::InstanceDecl &Decl,
                        std::map<std::string, Component::ExternInfo,
                                 std::less<>> &Exports) noexcept;
  Expect<void> validate(const AST::Component::ComponentDecl &Decl,
                        Component::Shape &Out) noexcept;
  // Validate component value types and type definitions
  Expect<void> validate(const ComponentValType &VT) noexcept;
  Expect<void> validate(const AST::Component::DefValType &DVT) noexcept;
  Expect<void> validate(const AST::Component::FuncType &FT) noexcept;
  Expect<void> validate(const AST::Component::ResourceType &RT) noexcept;
  /// @}

  /// Memory page limit for WASM32 and WASM64
  static inline const uint64_t LIMIT_MEMORYTYPE_LIM64 = UINT64_C(1) << 48;
  static inline const uint32_t LIMIT_MEMORYTYPE_LIM32 = UINT32_C(1) << 16;
  /// Proposal configure
  const Configure Conf;
  /// Formal checker
  FormChecker Checker;
  /// Type system for Component validation
  Component::TypeSystem CompTypes;
  /// Per-component state for Component validation
  Component::Context CompCtx{CompTypes};
};

} // namespace Validator
} // namespace WasmEdge
