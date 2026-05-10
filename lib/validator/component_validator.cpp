// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_name.h"
#include "validator/validator.h"

#include <algorithm>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

namespace {
std::string toLowerStr(std::string_view SV) {
  std::string Result(SV);
  std::transform(
      Result.begin(), Result.end(), Result.begin(),
      [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
  return Result;
}

// Maps a component-side ExternDesc::DescType to its Sort::SortType.
// Returns nullopt for `CoreType` (= `(core module (type i))`), which has no
// representation in Sort::SortType; callers must handle that sort separately
// via Sort::CoreSortType::Module.
std::optional<AST::Component::Sort::SortType>
descTypeToSortType(AST::Component::ExternDesc::DescType DT) noexcept {
  switch (DT) {
  case AST::Component::ExternDesc::DescType::CoreType:
    return std::nullopt;
  case AST::Component::ExternDesc::DescType::FuncType:
    return AST::Component::Sort::SortType::Func;
  case AST::Component::ExternDesc::DescType::ValueBound:
    return AST::Component::Sort::SortType::Value;
  case AST::Component::ExternDesc::DescType::TypeBound:
    return AST::Component::Sort::SortType::Type;
  case AST::Component::ExternDesc::DescType::ComponentType:
    return AST::Component::Sort::SortType::Component;
  case AST::Component::ExternDesc::DescType::InstanceType:
    return AST::Component::Sort::SortType::Instance;
  default:
    assumingUnreachable();
  }
}

// Shallow sort-kind match between a Sort and an ExternDesc. The spec's
// instantiation / export-ascription rules require the supplied sortidx to be
// a subtype of the externdesc. Here we only enforce that the kind agrees;
// deep structural subtyping (record fields, func signatures, etc.) is not
// yet implemented and is the main remaining correctness gap at these sites.
bool sortMatchesDescType(const AST::Component::Sort &S,
                         AST::Component::ExternDesc::DescType DT) noexcept {
  auto Mapped = descTypeToSortType(DT);
  if (S.isCore()) {
    return !Mapped.has_value() &&
           S.getCoreSortType() == AST::Component::Sort::CoreSortType::Module;
  }
  return Mapped.has_value() && S.getSortType() == *Mapped;
}

// Shallow populate: copy an InstanceType's exportdecls into the instance at
// InstIdx. InstanceType exportdecls get `IT` resolved in the current scope
// so alias-export can chase shapes through un-ascribed chains.
//
// Precondition: `IT` must be owned by the current ComponentContext scope —
// nested TypeIndex lookups resolve against that scope.
void populateInstanceFromType(ComponentContext &Ctx, uint32_t InstIdx,
                              const AST::Component::InstanceType &IT) {
  for (const auto &Decl : IT.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &Exp = Decl.getExport();
    const auto &ED = Exp.getExternDesc();
    auto ST = descTypeToSortType(ED.getDescType());
    if (!ST.has_value()) {
      // The InstanceExport::ST field is component-side only and has no
      // variant for `(core module)`. Skip the export rather than crash;
      // follow-up: extend InstanceExport to carry a core-sort alternative
      // so nested alias lookups through a core-module export can resolve.
      spdlog::debug(
          "    populateInstanceFromType: skipping `(core module)` export "
          "'{}'"sv,
          Exp.getName());
      continue;
    }
    const AST::Component::InstanceType *NestedIT = nullptr;
    if (ED.getDescType() ==
        AST::Component::ExternDesc::DescType::InstanceType) {
      NestedIT = Ctx.getInstanceType(ED.getTypeIndex());
      // Fallback: when IT is being processed inside a
      // componenttype/instancetype scope, nested inline InstanceTypes live
      // in IT's own local type-index space rather than the current scope's
      // typestate. Walk IT's type-declaring decls to locate the N-th one.
      if (NestedIT == nullptr) {
        uint32_t TargetIdx = ED.getTypeIndex();
        uint32_t LocalIdx = 0;
        for (const auto &LocalDecl : IT.getDecl()) {
          if (!LocalDecl.isType()) {
            continue;
          }
          if (LocalIdx == TargetIdx) {
            const auto *LocalDT = LocalDecl.getType();
            if (LocalDT != nullptr && LocalDT->isInstanceType()) {
              NestedIT = &LocalDT->getInstanceType();
            }
            break;
          }
          LocalIdx++;
        }
      }
    }
    Ctx.addInstanceExport(InstIdx, Exp.getName(), *ST, NestedIT);
  }
}

// Returns the first export in `RequiredIT` missing from the instance at
// `ProvidedInstIdx`, or whose stored SortType doesn't match the required
// sort kind. nullopt ⇒ Provided is a sort-kind subtype of RequiredIT.
// Only the sort kind is compared; deep structural subtype between the
// required externdesc and the provided export's inferred type is not yet
// implemented.
std::optional<std::string>
findMissingRequiredExport(const ComponentContext &Ctx, uint32_t ProvidedInstIdx,
                          const AST::Component::InstanceType &RequiredIT) {
  const auto &Exports = Ctx.getInstance(ProvidedInstIdx);
  for (const auto &Decl : RequiredIT.getDecl()) {
    if (!Decl.isExportDecl()) {
      continue;
    }
    const auto &Exp = Decl.getExport();
    auto It = Exports.find(std::string(Exp.getName()));
    if (It == Exports.end()) {
      return std::string(Exp.getName());
    }
    auto RequiredST = descTypeToSortType(Exp.getExternDesc().getDescType());
    if (!RequiredST.has_value()) {
      // `(core module)` required-exports can't be compared against
      // InstanceExport::ST (component-side only). Skip symmetrically with
      // populateInstanceFromType; will be revisited when InstanceExport
      // gains a core-sort variant.
      spdlog::debug("    findMissingRequiredExport: skipping `(core module)` "
                    "required-export '{}'"sv,
                    Exp.getName());
      continue;
    }
    if (It->second.ST != *RequiredST) {
      return std::string(Exp.getName());
    }
  }
  return std::nullopt;
}

// Resolve a type index in `Comp`'s own type index space to an InstanceType.
// Returns nullptr when the index does not refer to an inline InstanceType
// definition — callers treat nullptr as "no required shape" and fall back
// to inferred exports. TypeBound imports and outer-alias type imports
// currently fall through to nullptr; a more complete resolver would walk
// the alias chain to recover the underlying InstanceType.
const AST::Component::InstanceType *
resolveChildInstanceType(const AST::Component::Component &Comp,
                         uint32_t TypeIdx) {
  uint32_t CurrentIdx = 0;
  for (const auto &Sec : Comp.getSections()) {
    if (std::holds_alternative<AST::Component::TypeSection>(Sec)) {
      const auto &TSec = std::get<AST::Component::TypeSection>(Sec);
      for (const auto &DT : TSec.getContent()) {
        if (CurrentIdx == TypeIdx) {
          if (DT.isInstanceType()) {
            return &DT.getInstanceType();
          }
          return nullptr;
        }
        CurrentIdx++;
      }
    } else if (std::holds_alternative<AST::Component::ImportSection>(Sec)) {
      const auto &ISec = std::get<AST::Component::ImportSection>(Sec);
      for (const auto &Import : ISec.getContent()) {
        if (Import.getDesc().getDescType() ==
            AST::Component::ExternDesc::DescType::TypeBound) {
          if (CurrentIdx == TypeIdx) {
            return nullptr;
          }
          CurrentIdx++;
        }
      }
    } else if (std::holds_alternative<AST::Component::AliasSection>(Sec)) {
      const auto &ASec = std::get<AST::Component::AliasSection>(Sec);
      for (const auto &Alias : ASec.getContent()) {
        if (!Alias.getSort().isCore() &&
            Alias.getSort().getSortType() ==
                AST::Component::Sort::SortType::Type) {
          if (CurrentIdx == TypeIdx) {
            return nullptr;
          }
          CurrentIdx++;
        }
      }
    }
  }
  return nullptr;
}

// Validate that a name may appear at an export position: reject the
// `relative-url=` prefix (not part of the extern-name grammar) and any
// plainname/interfacename kind that isn't allowed on an export.
Expect<void> validateExportName(std::string_view Name) noexcept {
  if (Name.rfind("relative-url="sv, 0) == 0) {
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Export name '{}' is not a valid extern name"sv, Name);
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  EXPECTED_TRY(ComponentName CName, ComponentName::parse(Name));
  switch (CName.getKind()) {
  case ComponentNameKind::Label:
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
    return {};
  default:
    spdlog::error(ErrCode::Value::InvalidExportName);
    spdlog::error("    Export name '{}' kind is not valid for exports"sv, Name);
    return Unexpect(ErrCode::Value::InvalidExportName);
  }
}
} // namespace

Expect<void>
Validator::validate(const AST::Component::Component &Comp) noexcept {
  spdlog::warn("Component Model Validation is in active development."sv);
  CompCtx.reset();
  return validateComponent(Comp).and_then([&]() {
    const_cast<AST::Component::Component &>(Comp).setIsValidated();
    return Expect<void>{};
  });
}

Expect<void>
Validator::validateComponent(const AST::Component::Component &Comp) noexcept {
  // Validation enters a fresh component scope and walks sections in their
  // binary order. The per-sort index spaces are built incrementally as
  // definitions are validated, so sortidx references in later sections
  // only resolve against entries already introduced. Custom sections are
  // ignored. Nested components recurse through this same function.
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
    return E;
  };

  CompCtx.enterComponent(&Comp);
  for (const auto &Sec : Comp.getSections()) {
    auto Func = [&](auto &&S) -> Expect<void> {
      using T = std::decay_t<decltype(S)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Always pass validation.
      } else {
        EXPECTED_TRY(validate(S).map_error(ReportError));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Sec));
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleSection &ModSec) noexcept {
  EXPECTED_TRY(validate(ModSec.getContent()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreMod));
    return E;
  }));
  const_cast<AST::Module &>(ModSec.getContent()).setIsValidated();
  CompCtx.addCoreModule(ModSec.getContent());
  return {};
}

Expect<void> Validator::validate(
    const AST::Component::CoreInstanceSection &InstSec) noexcept {
  for (const auto &Inst : InstSec.getContent()) {
    EXPECTED_TRY(validate(Inst).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreInstance));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreTypeSection &TypeSec) noexcept {
  for (const auto &Type : TypeSec.getContent()) {
    EXPECTED_TRY(validate(Type).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_CoreType));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentSection &CompSec) noexcept {
  EXPECTED_TRY(validateComponent(CompSec.getContent()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Component));
    return E;
  }));
  CompCtx.addComponent(CompSec.getContent());
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceSection &InstSec) noexcept {
  for (const auto &Inst : InstSec.getContent()) {
    EXPECTED_TRY(validate(Inst).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Instance));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::AliasSection &AliasSec) noexcept {
  for (const auto &Alias : AliasSec.getContent()) {
    EXPECTED_TRY(validate(Alias).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Alias));
      return E;
    }));
    const auto &Sort = Alias.getSort();
    if (Sort.isCore()) {
      CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
    } else {
      uint32_t NewIdx = CompCtx.incSortIndexSize(Sort.getSortType());
      // If the alias creates a new instance entry out of an `alias export`
      // on another instance, propagate the source instance's export table
      // into the new slot so a subsequent `alias export` on this slot can
      // resolve nested exports.
      if (Sort.getSortType() == AST::Component::Sort::SortType::Instance &&
          Alias.getTargetType() == AST::Component::Alias::TargetType::Export) {
        const auto SrcInstIdx = Alias.getExport().first;
        const auto &SrcName = Alias.getExport().second;
        const auto &SrcExports = CompCtx.getInstance(SrcInstIdx);
        auto It = SrcExports.find(std::string(SrcName));
        if (It != SrcExports.end()) {
          if (It->second.IT != nullptr) {
            populateInstanceFromType(CompCtx, NewIdx, *It->second.IT);
          } else if (It->second.NestedInstIdx.has_value()) {
            const auto &NestedExports =
                CompCtx.getInstance(*It->second.NestedInstIdx);
            for (const auto &[Name, IE] : NestedExports) {
              CompCtx.addInstanceExport(NewIdx, Name, IE.ST, IE.IT,
                                        IE.NestedInstIdx);
            }
          }
          // Source instance has neither a resolved InstanceType nor a
          // NestedInstIdx (e.g. ascription by a cross-scope type index we
          // cannot resolve yet). Leave the new slot's export table empty.
        }
      }
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::TypeSection &TypeSec) noexcept {
  for (const auto &Type : TypeSec.getContent()) {
    EXPECTED_TRY(validate(Type).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Type));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CanonSection &CanonSec) noexcept {
  for (const auto &C : CanonSec.getContent()) {
    EXPECTED_TRY(validate(C).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Canon));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::StartSection &) noexcept {
  // The start section is stubbed. A conformant implementation performs:
  //
  //   1. `f` is in bounds of the component func index space.
  //   2. `f`'s functype matches the start's arguments: param arity equals
  //      |arg*|, each arg's value-type is a subtype of the corresponding
  //      param, and the function's result arity equals `r`.
  //   3. Each argument consumes a value from the value index space — its
  //      "consumed" flag must be currently clear and is set after
  //      consumption. Values are linear: each must be consumed exactly
  //      once over the lifetime of a component.
  //   4. The function's result types are appended to the value index
  //      space as fresh values with consumed=false, so later definitions
  //      can reference them.
  //   5. After the component's final definition, every value's consumed
  //      flag must be set.
  //
  // Value-linearity tracking (the "consumed" flag) is not yet wired into
  // the validator at all, so start validation is deferred until that
  // machinery is added.
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ImportSection &ImpSec) noexcept {
  for (const auto &Imp : ImpSec.getContent()) {
    EXPECTED_TRY(validate(Imp).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Import));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExportSection &ExpSec) noexcept {
  for (const auto &Exp : ExpSec.getContent()) {
    EXPECTED_TRY(validate(Exp).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Export));
      return E;
    }));
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreInstance &Inst) noexcept {
  if (Inst.isInstantiateModule()) {
    // Instantiate module case.

    // Check the module index bound first.
    const uint32_t ModIdx = Inst.getModuleIndex();
    if (ModIdx >= CompCtx.getCoreSortIndexSize(
                      AST::Component::Sort::CoreSortType::Module)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    CoreInstance: Module index {} exceeds available core modules {}"sv,
          ModIdx,
          CompCtx.getCoreSortIndexSize(
              AST::Component::Sort::CoreSortType::Module));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Reject duplicate argument names on an instantiate expression. The
    // spec requires argument names to be strongly-unique per instantiation.
    {
      std::unordered_set<std::string_view> SeenArgs;
      for (const auto &Arg : Inst.getInstantiateArgs()) {
        if (!SeenArgs.insert(Arg.getName()).second) {
          spdlog::error(ErrCode::Value::ComponentDuplicateName);
          spdlog::error("    CoreInstance: Duplicate argument name '{}'"sv,
                        Arg.getName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::ComponentDuplicateName);
        }
      }
    }

    // Get the import descriptors of the module to instantiate.
    const auto *Mod = CompCtx.getCoreModule(ModIdx);
    // TODO: use core:moduletype info (not the raw AST) so arg checking also
    // applies to imported/aliased modules, not just modules defined inline.
    // This also unlocks structural subtype checks of each supplied argument
    // against the corresponding core import's type.
    if (Mod != nullptr) {
      const auto &ImportDescs = Mod->getImportSection().getContent();
      auto Args = Inst.getInstantiateArgs();
      for (const auto &Import : ImportDescs) {
        auto ArgIt =
            std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
              return Arg.getName() == Import.getModuleName();
            });
        if (ArgIt == Args.end()) {
          spdlog::error(ErrCode::Value::MissingArgument);
          spdlog::error(
              "    CoreInstance: Module index {} missing argument for import '{}'"sv,
              Inst.getModuleIndex(), Import.getModuleName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::MissingArgument);
        }
      }
      // Allocate a core:instance and bind module exports to it.
      uint32_t InstanceIdx = CompCtx.addCoreInstance();
      const auto &ExportDescs = Mod->getExportSection().getContent();
      for (const auto &ExportDesc : ExportDescs) {
        CompCtx.addCoreInstanceExport(InstanceIdx, ExportDesc.getExternalName(),
                                      ExportDesc.getExternalType());
      }
    } else {
      CompCtx.addCoreInstance();
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.
    // Allocate the core instance first, then register each inline export.
    uint32_t InstanceIdx = CompCtx.addCoreInstance();

    // Check the core:sort index bound and register the inline exports.
    // Inline-export names on a core instance must be strongly-unique.
    std::unordered_set<std::string_view> SeenExports;
    for (const auto &Export : Inst.getInlineExports()) {
      if (!SeenExports.insert(Export.getName()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    CoreInstance: Duplicate inline-export name '{}'"sv,
                      Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      assuming(Sort.isCore());
      if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    CoreInstance: Inline export '{}' refers to invalid index {}"sv,
            Export.getName(), Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // Map CoreSortType to ExternalType for the instance export map.
      ExternalType ET;
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func:
        ET = ExternalType::Function;
        break;
      case AST::Component::Sort::CoreSortType::Table:
        ET = ExternalType::Table;
        break;
      case AST::Component::Sort::CoreSortType::Memory:
        ET = ExternalType::Memory;
        break;
      case AST::Component::Sort::CoreSortType::Global:
        ET = ExternalType::Global;
        break;
      default:
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    CoreInstance: Inline export '{}' has unsupported core sort"sv,
            Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      CompCtx.addCoreInstanceExport(InstanceIdx, Export.getName(), ET);
    }
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Instance &Inst) noexcept {
  if (Inst.isInstantiateModule()) {
    // Instantiate module case.

    // Check the component index bound first.
    const uint32_t CompIdx = Inst.getComponentIndex();
    if (CompIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Component)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Instance: Component index {} exceeds available components {}"sv,
          CompIdx,
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Component));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Reject duplicate argument names on an instantiate expression. The
    // spec requires argument names to be strongly-unique per instantiation.
    {
      std::unordered_set<std::string_view> SeenArgs;
      for (const auto &Arg : Inst.getInstantiateArgs()) {
        if (!SeenArgs.insert(Arg.getName()).second) {
          spdlog::error(ErrCode::Value::ComponentDuplicateName);
          spdlog::error("    Instance: Duplicate argument name '{}'"sv,
                        Arg.getName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::ComponentDuplicateName);
        }
      }
    }

    // Get the imports of the component to instantiate.
    const auto *Comp = CompCtx.getComponent(CompIdx);
    // TODO: use componenttype info (not the raw AST) so arg checking also
    // applies to imported/aliased components. For now, arg checking only
    // runs when the component being instantiated is defined inline.
    if (Comp != nullptr) {
      // Prepare the import descriptor map.
      std::unordered_map<std::string, const AST::Component::ExternDesc *>
          ImportMap;
      for (const auto &Sec : Comp->getSections()) {
        if (std::holds_alternative<AST::Component::ImportSection>(Sec)) {
          const auto &ImportSec = std::get<AST::Component::ImportSection>(Sec);
          for (const auto &Import : ImportSec.getContent()) {
            ImportMap[std::string(Import.getName())] = &Import.getDesc();
          }
        }
      }
      // Get the instantiation arguments.
      auto Args = Inst.getInstantiateArgs();
      // Check that the import names are supplied by the instantiation
      // arguments.
      for (auto It = ImportMap.begin(); It != ImportMap.end(); ++It) {
        const auto &ImportName = It->first;
        auto ArgIt =
            std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
              return Arg.getName() == ImportName;
            });
        if (ArgIt == Args.end()) {
          spdlog::error(ErrCode::Value::MissingArgument);
          spdlog::error(
              "    Instance: Component index {} missing argument for import '{}'"sv,
              Inst.getComponentIndex(), ImportName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::MissingArgument);
        }
        const auto &ImportDesc = *It->second;
        const auto &Sort = ArgIt->getIndex().getSort();
        const uint32_t Idx = ArgIt->getIndex().getIdx();
        // The externdesc grammar only admits `core module` on the core side
        // (other core sorts don't appear as importable externdescs), so any
        // other core sort can't satisfy a component import.
        if (Sort.isCore() && Sort.getCoreSortType() !=
                                 AST::Component::Sort::CoreSortType::Module) {
          spdlog::error(ErrCode::Value::ArgTypeMismatch);
          spdlog::error(
              "    Instance: Argument '{}' uses a core sort other than "
              "`core module`, which no import externdesc can accept"sv,
              ImportName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::ArgTypeMismatch);
        }
        if (!sortMatchesDescType(Sort, ImportDesc.getDescType())) {
          spdlog::error(ErrCode::Value::ArgTypeMismatch);
          spdlog::error(
              "    Instance: Argument '{}' sort mismatch for import"sv,
              ImportName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::ArgTypeMismatch);
        }
        if (Sort.isCore()) {
          if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
            spdlog::error(ErrCode::Value::InvalidIndex);
            spdlog::error(
                "    Instance: Argument '{}' refers to invalid index {}"sv,
                ImportName, Idx);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::InvalidIndex);
          }
        } else {
          if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
            spdlog::error(ErrCode::Value::InvalidIndex);
            spdlog::error(
                "    Instance: Argument '{}' refers to invalid index {}"sv,
                ImportName, Idx);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::InvalidIndex);
          }
          // Partial step toward structural subtype: when the import asks
          // for an InstanceType, check that every export required by the
          // import's declared InstanceType is present on the provided
          // instance. Field-level subtype of each matching export is still
          // pending.
          if (ImportDesc.getDescType() ==
                  AST::Component::ExternDesc::DescType::InstanceType &&
              Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
            const auto *RequiredIT =
                resolveChildInstanceType(*Comp, ImportDesc.getTypeIndex());
            if (RequiredIT != nullptr) {
              if (auto Missing =
                      findMissingRequiredExport(CompCtx, Idx, *RequiredIT)) {
                spdlog::error(ErrCode::Value::InstanceMissingExpectedExport);
                spdlog::error(
                    "    Instance: Argument '{}' missing required export "
                    "'{}' for import"sv,
                    ImportName, *Missing);
                spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
                return Unexpect(ErrCode::Value::InstanceMissingExpectedExport);
              }
            }
          }
        }
      }
    }
    // Allocate the instance in the index space and populate its export table
    // from the instantiated component's ExportSection(s) so alias-export can
    // resolve subsequent lookups.
    uint32_t InstanceIdx = CompCtx.addInstance();
    if (Comp != nullptr) {
      for (const auto &Sec : Comp->getSections()) {
        if (!std::holds_alternative<AST::Component::ExportSection>(Sec)) {
          continue;
        }
        const auto &ExpSec = std::get<AST::Component::ExportSection>(Sec);
        for (const auto &Exp : ExpSec.getContent()) {
          const auto &ExpSort = Exp.getSortIndex().getSort();
          if (ExpSort.isCore()) {
            continue;
          }
          const AST::Component::InstanceType *IT = nullptr;
          if (ExpSort.getSortType() ==
                  AST::Component::Sort::SortType::Instance &&
              Exp.getDesc().has_value() &&
              Exp.getDesc()->getDescType() ==
                  AST::Component::ExternDesc::DescType::InstanceType) {
            IT = resolveChildInstanceType(*Comp, Exp.getDesc()->getTypeIndex());
          }
          CompCtx.addInstanceExport(InstanceIdx, Exp.getName(),
                                    ExpSort.getSortType(), IT);
        }
      }
    }
  } else if (Inst.isInlineExport()) {
    // Allocate the instance first so exports can be registered on it.
    uint32_t InstanceIdx = CompCtx.addInstance();

    // Check the sort index bound of the inline exports, then record each
    // export on the new instance with a NestedInstIdx fallback so alias
    // chains through inline-export instances can follow the source.
    // Inline-export names on a component instance must be strongly-unique.
    std::unordered_set<std::string_view> SeenExports;
    for (const auto &Export : Inst.getInlineExports()) {
      if (!SeenExports.insert(Export.getName()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    Instance: Duplicate inline-export name '{}'"sv,
                      Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      if (Sort.isCore()) {
        if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
          spdlog::error(ErrCode::Value::InvalidIndex);
          spdlog::error(
              "    Instance: Inline export '{}' refers to invalid index {}"sv,
              Export.getName(), Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::InvalidIndex);
        }
        continue;
      }
      if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Instance: Inline export '{}' refers to invalid index {}"sv,
            Export.getName(), Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        auto SubstitutedIdx =
            CompCtx.getSubstitutedType(std::string(Export.getName()));
        if (SubstitutedIdx.has_value() && Idx != SubstitutedIdx.value()) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "    Instance: Inline export '{}' type index {} does not match substituted type index {}"sv,
              Export.getName(), Idx, *SubstitutedIdx);
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
      }
      std::optional<uint32_t> NestedIdx;
      if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
        NestedIdx = Idx;
      }
      // No resolved InstanceType is forwarded here; NestedInstIdx is
      // enough for subsequent alias-export lookups to chase into the
      // source instance.
      CompCtx.addInstanceExport(InstanceIdx, Export.getName(),
                                Sort.getSortType(), nullptr, NestedIdx);
    }
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::CoreAlias &A) noexcept {
  // CoreAlias is always an outer alias.
  uint32_t Ct = A.getComponentJump();
  uint32_t Idx = A.getIndex();

  uint32_t OutLinkCompCnt = 0;
  const auto *TargetCtx = &CompCtx.getCurrentContext();
  while (Ct > OutLinkCompCnt && TargetCtx != nullptr) {
    TargetCtx = TargetCtx->Parent;
    OutLinkCompCnt++;
  }
  if (TargetCtx == nullptr) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    CoreAlias: outer count {} exceeds enclosing component count {}"sv,
        Ct, OutLinkCompCnt);
    return Unexpect(ErrCode::Value::InvalidIndex);
  }

  const auto &Sort = A.getSort();
  if (Sort.isCore()) {
    if (Idx >= TargetCtx->getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    CoreAlias: outer index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::Alias &Alias) noexcept {
  const auto &Sort = Alias.getSort();
  switch (Alias.getTargetType()) {
  case AST::Component::Alias::TargetType::Export: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    if (Sort.isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias export: Mapping an export '{}' to core:sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }

    if (Idx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Alias export: Export index {} exceeds available component instance index {}"sv,
          Idx,
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    const auto &InstExports = CompCtx.getInstance(Idx);
    auto It = InstExports.find(std::string(Name));
    if (It == InstExports.cend()) {
      spdlog::error(ErrCode::Value::ExportNotFound);
      spdlog::error(
          "    Alias export: No matching export '{}' found in component instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::ExportNotFound);
    }

    if (It->second.ST != Sort.getSortType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias export: Type mapping mismatch for export '{}'"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::CoreExport: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    if (!Sort.isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias core:export: Mapping a export '{}' to sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }

    if (Idx >= CompCtx.getCoreSortIndexSize(
                   AST::Component::Sort::CoreSortType::Instance)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Alias core:export: Export index {} exceeds available core instance index {}"sv,
          Idx,
          CompCtx.getCoreSortIndexSize(
              AST::Component::Sort::CoreSortType::Instance) -
              1);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    const auto &CoreExports = CompCtx.getCoreInstance(Idx);
    auto It = CoreExports.find(std::string(Name));
    if (It == CoreExports.end()) {
      spdlog::error(ErrCode::Value::ExportNotFound);
      spdlog::error(
          "    Alias core:export: No matching export '{}' found in core instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::ExportNotFound);
    }

    const auto ExternTy = It->second;
    AST::Component::Sort::CoreSortType ST;
    switch (ExternTy) {
    case ExternalType::Function:
      ST = AST::Component::Sort::CoreSortType::Func;
      break;
    case ExternalType::Table:
      ST = AST::Component::Sort::CoreSortType::Table;
      break;
    case ExternalType::Memory:
      ST = AST::Component::Sort::CoreSortType::Memory;
      break;
    case ExternalType::Global:
      ST = AST::Component::Sort::CoreSortType::Global;
      break;
    case ExternalType::Tag:
    default:
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (ST != Sort.getCoreSortType()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::Outer: {
    const auto Ct = Alias.getOuter().first;
    const auto Idx = Alias.getOuter().second;

    uint32_t OutLinkCompCnt = 0;
    const auto *TargetCtx = &CompCtx.getCurrentContext();
    while (Ct > OutLinkCompCnt && TargetCtx != nullptr) {
      TargetCtx = TargetCtx->Parent;
      OutLinkCompCnt++;
    }
    if (TargetCtx == nullptr) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Alias outer: Component out-link count {} is exceeding the enclosing component count {}"sv,
          Ct, OutLinkCompCnt);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    if (Sort.isCore()) {
      if (Sort.getCoreSortType() !=
              AST::Component::Sort::CoreSortType::Module &&
          Sort.getCoreSortType() != AST::Component::Sort::CoreSortType::Type) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error(
            "    Alias outer: Invalid core:sort for outer alias. Only type, module, or component are allowed."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      if (Idx >= TargetCtx->getCoreSortIndexSize(Sort.getCoreSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Alias outer: core:sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
    } else {
      if (Sort.getSortType() != AST::Component::Sort::SortType::Type &&
          Sort.getSortType() != AST::Component::Sort::SortType::Component) {
        spdlog::error(ErrCode::Value::InvalidTypeReference);
        spdlog::error(
            "    Alias outer: Invalid sort for outer alias. Only type, module, or component are allowed."sv);
        return Unexpect(ErrCode::Value::InvalidTypeReference);
      }
      if (Idx >= TargetCtx->getSortIndexSize(Sort.getSortType())) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Alias outer: sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        // Narrow spec rule: forbid outer-aliasing a resource type directly.
        // Resource types are generative, so aliasing them across a component
        // boundary would leak a specific instance's resource into another
        // component's type index space.
        //
        // TODO: broaden to wasm-tools' full free-variable rule — forbid
        // outer-aliasing *any* type whose transitive free variables include
        // resources defined on the far side of a crossed component boundary.
        // Implementing that needs a free-variable walker over type bodies,
        // which is tracked alongside the Phase 3/4 type-handle work
        // (GAP-TH-1, GAP-EX-5).
        if (TargetCtx->ResourceTypes.find(Idx) !=
            TargetCtx->ResourceTypes.end()) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "    Alias outer: Cannot outer-alias resource type at index {}"sv,
              Idx);
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
      }
    }
    return {};
  }
  default:
    assumingUnreachable();
  }
}

Expect<void>
Validator::validate(const AST::Component::CoreDefType &DType) noexcept {
  if (DType.isRecType()) {
    // Each sub-type in the rec group gets its own entry in core:type.
    for (const auto &ST : DType.getSubTypes()) {
      CompCtx.addCoreType(&ST);
    }
  } else if (DType.isModuleType()) {
    // Module types are validated with an initially-empty type index space.
    CompCtx.enterTypeDefinition();
    for (const auto &Decl : DType.getModuleType()) {
      EXPECTED_TRY(validate(Decl).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return E;
      }));
    }
    CompCtx.exitComponent();
    // Module type also gets an entry in core:type (nullptr — not a SubType).
    CompCtx.addCoreType();
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefType &DType) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
    return E;
  };

  if (DType.isDefValType()) {
    EXPECTED_TRY(validate(DType.getDefValType()).map_error(ReportError));
  } else if (DType.isFuncType()) {
    EXPECTED_TRY(validate(DType.getFuncType()).map_error(ReportError));
  } else if (DType.isComponentType()) {
    EXPECTED_TRY(validate(DType.getComponentType()).map_error(ReportError));
  } else if (DType.isInstanceType()) {
    EXPECTED_TRY(validate(DType.getInstanceType()).map_error(ReportError));
  } else if (DType.isResourceType()) {
    EXPECTED_TRY(validate(DType.getResourceType()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  CompCtx.addType(&DType);
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Canonical &Canon) noexcept {
  switch (Canon.getOpCode()) {
  case AST::Component::Canonical::OpCode::Lift:
    return validateCanonLift(Canon);
  case AST::Component::Canonical::OpCode::Lower:
    return validateCanonLower(Canon);
  case AST::Component::Canonical::OpCode::Resource__new:
    return validateCanonResourceNew(Canon);
  case AST::Component::Canonical::OpCode::Resource__rep:
    return validateCanonResourceRep(Canon);
  case AST::Component::Canonical::OpCode::Resource__drop:
  case AST::Component::Canonical::OpCode::Resource__drop_async:
    return validateCanonResourceDrop(Canon);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
}

Expect<void> Validator::validateCanonOptions(
    AST::Component::Canonical::OpCode Code,
    Span<const AST::Component::CanonOpt> Opts) noexcept {
  using OptCode = AST::Component::CanonOpt::OptCode;
  using CanonOp = AST::Component::Canonical::OpCode;

  // Only canon lift/lower accept canonical options. Any other built-in
  // (resource.new/rep/drop, drop_async, ...) must be invoked without options.
  if (Code != CanonOp::Lift && Code != CanonOp::Lower && !Opts.empty()) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical options are not allowed for this canon built-in"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  bool HasEncoding = false;
  bool HasMemory = false;
  bool HasRealloc = false;
  bool HasPostReturn = false;
  bool HasAsync = false;
  bool HasCallback = false;
  bool HasAlwaysTaskReturn = false;
  uint32_t ReallocIdx = 0;
  uint32_t CallbackIdx = 0;
  uint32_t PostReturnIdx = 0;
  uint32_t MemoryIdx = 0;

  auto RejectDup = [&](const char *Name) -> Expect<void> {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    canonical option '{}' appears more than once"sv, Name);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  };
  auto RejectSite = [&](const char *Name) -> Expect<void> {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option '{}' is not allowed in this canon built-in"sv,
        Name);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  };

  for (const auto &Opt : Opts) {
    switch (Opt.getCode()) {
    case OptCode::Encode_UTF8:
    case OptCode::Encode_UTF16:
    case OptCode::Encode_Latin1:
      if (HasEncoding) {
        return RejectDup("string-encoding");
      }
      HasEncoding = true;
      break;
    case OptCode::Memory:
      if (HasMemory) {
        return RejectDup("memory");
      }
      HasMemory = true;
      MemoryIdx = Opt.getIndex();
      break;
    case OptCode::Realloc:
      if (HasRealloc) {
        return RejectDup("realloc");
      }
      HasRealloc = true;
      ReallocIdx = Opt.getIndex();
      break;
    case OptCode::PostReturn:
      if (Code != CanonOp::Lift) {
        return RejectSite("post-return");
      }
      if (HasPostReturn) {
        return RejectDup("post-return");
      }
      HasPostReturn = true;
      PostReturnIdx = Opt.getIndex();
      break;
    case OptCode::Async:
      if (HasAsync) {
        return RejectDup("async");
      }
      HasAsync = true;
      break;
    case OptCode::Callback:
      if (Code != CanonOp::Lift) {
        return RejectSite("callback");
      }
      if (HasCallback) {
        return RejectDup("callback");
      }
      HasCallback = true;
      CallbackIdx = Opt.getIndex();
      break;
    case OptCode::AlwaysTaskReturn:
      if (Code != CanonOp::Lift) {
        return RejectSite("always-task-return");
      }
      if (HasAlwaysTaskReturn) {
        return RejectDup("always-task-return");
      }
      HasAlwaysTaskReturn = true;
      break;
    default:
      spdlog::error(ErrCode::Value::UnknownCanonicalOption);
      spdlog::error("    unknown canonical option code 0x{:02x}"sv,
                    static_cast<unsigned>(Opt.getCode()));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::UnknownCanonicalOption);
    }
  }

  // Structural rules.
  if (HasPostReturn && HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical options 'post-return' and 'async' are mutually exclusive"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasCallback && !HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error("    canonical option 'callback' requires 'async'"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasAlwaysTaskReturn && !HasAsync) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option 'always-task-return' requires 'async'"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }
  if (HasRealloc && !HasMemory) {
    spdlog::error(ErrCode::Value::InvalidCanonOption);
    spdlog::error(
        "    canonical option 'realloc' requires 'memory' to also be specified"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidCanonOption);
  }

  // Index bounds checks. Core func signature body checks (realloc/callback)
  // deferred as GAP-C-5b once getCoreFunc signatures are populated.
  if (HasMemory &&
      MemoryIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Memory)) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'memory': core memory index {} out of bounds"sv,
        MemoryIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  const uint32_t CoreFuncSpaceSize =
      CompCtx.getCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
  if (HasRealloc && ReallocIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'realloc': core func index {} out of bounds"sv,
        ReallocIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  if (HasCallback && CallbackIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'callback': core func index {} out of bounds"sv,
        CallbackIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  if (HasPostReturn && PostReturnIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canonical option 'post-return': core func index {} out of bounds"sv,
        PostReturnIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  return {};
}

Expect<void>
Validator::validateCanonLift(const AST::Component::Canonical &Canon) noexcept {
  const uint32_t CoreFuncIdx = Canon.getIndex();
  const uint32_t CoreFuncSpaceSize =
      CompCtx.getCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
  // 1. Core func index bounds.
  if (CoreFuncIdx >= CoreFuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lift: core func index {} exceeds core func index space size {}"sv,
        CoreFuncIdx, CoreFuncSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  const uint32_t TypeIdx = Canon.getTargetIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 2. Target type index bounds.
  if (TypeIdx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lift: type index {} exceeds type index space size {}"sv,
        TypeIdx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 3. Target type must be a component FuncType.
  const auto *DT = CompCtx.getDefType(TypeIdx);
  if (DT == nullptr) {
    // Unresolved slot: the index was registered by an import or outer alias
    // but the concrete definition has not been filled in yet.
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon lift: target type index {} is an unresolved type slot"sv,
        TypeIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (!DT->isFuncType()) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon lift: target type index {} does not reference a component func type"sv,
        TypeIdx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options (Lift site allows all).
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate component func, binding the resolved FuncType. Full ABI
  // signature match (flat_lifted) deferred as GAP-C-1b.
  CompCtx.addFunc(&DT->getFuncType());
  return {};
}

Expect<void>
Validator::validateCanonLower(const AST::Component::Canonical &Canon) noexcept {
  const uint32_t FuncIdx = Canon.getIndex();
  const uint32_t FuncSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Func);
  // 1. Component func index bounds.
  if (FuncIdx >= FuncSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon lower: component func index {} exceeds func index space size {}"sv,
        FuncIdx, FuncSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Validate canonical options (per-site rules for Lower).
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 3. Allocate the resulting core func. Full ABI signature synthesis
  // (flatten_functype for lower) deferred as GAP-C-2b.
  CompCtx.addCoreFunc();
  return {};
}

Expect<void> Validator::validateCanonResourceNew(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.new: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a resource type.
  if (!CompCtx.isResourceType(Idx)) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.new: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 3. Resource must be locally defined.
  if (!CompCtx.isLocalResource(Idx)) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.new: type index {} is not locally defined (imported or outer-aliased resources are not allowed)"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func. Full signature tracking deferred.
  CompCtx.addCoreFunc();
  return {};
}

Expect<void> Validator::validateCanonResourceRep(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.rep: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a resource type.
  if (!CompCtx.isResourceType(Idx)) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.rep: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 3. Resource must be locally defined.
  if (!CompCtx.isLocalResource(Idx)) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.rep: type index {} is not locally defined (imported or outer-aliased resources are not allowed)"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func. Full signature tracking deferred.
  CompCtx.addCoreFunc();
  return {};
}

Expect<void> Validator::validateCanonResourceDrop(
    const AST::Component::Canonical &Canon) noexcept {
  const uint32_t Idx = Canon.getIndex();
  const uint32_t TypeSpaceSize =
      CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);
  // 1. Type index bounds.
  if (Idx >= TypeSpaceSize) {
    spdlog::error(ErrCode::Value::InvalidIndex);
    spdlog::error(
        "    canon resource.drop: type index {} exceeds type index space size {}"sv,
        Idx, TypeSpaceSize);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidIndex);
  }
  // 2. Type must be a resource type.
  if (!CompCtx.isResourceType(Idx)) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error(
        "    canon resource.drop: type index {} does not reference a resource"sv,
        Idx);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  // 3. resource.drop accepts both local and imported resources — no locality
  // check.
  // 4. Validate canonical options.
  EXPECTED_TRY(validateCanonOptions(Canon.getOpCode(), Canon.getOptions()));
  // 5. Allocate the resulting core func. Full signature tracking deferred.
  CompCtx.addCoreFunc();
  return {};
}

Expect<void> Validator::validate(const AST::Component::Import &Im) noexcept {
  // Validation steps:
  //   1. Validate the externdesc and introduce the imported entity into
  //      its sort's index space.
  //   2. Parse the import name per the structured import-name grammar.
  //   3. Enforce the annotated-name constraints ([constructor]/[method]/
  //      [static] only on func imports).
  //   4. Reject duplicate import names (strongly-unique across imports).
  //
  // The per-annotated-name structural checks (constructor result type,
  // method `self` param, static resource name in scope) are not yet
  // implemented and tracked as follow-ups below.
  EXPECTED_TRY(validate(Im.getDesc()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return E;
  }));

  EXPECTED_TRY(ComponentName CName,
               ComponentName::parse(Im.getName()).map_error([](auto E) {
                 spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
                 return E;
               }));

  // Annotated plainnames ([constructor], [method], [static]) can only appear
  // on func imports.
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
    if (Im.getDesc().getDescType() !=
        AST::Component::ExternDesc::DescType::FuncType) {
      spdlog::error(ErrCode::Value::ComponentInvalidName);
      spdlog::error("    Import: annotated name requires func type"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    break;
  default:
    break;
  }

  // TODO: Validation of [constructor] names requires that the func returns
  // a (result (own $R)), where $R is the resource labeled r.

  // TODO: Validation of [method] names requires the first parameter of the
  // function to be (param "self" (borrow $R)), where $R is the resource
  // labeled r.

  // TODO: Validation of [static] names requires the resource name to be
  // known in the current context.
  if (!CompCtx.addImportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    Import: Duplicate import name"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }

  return {};
}

Expect<void> Validator::validate(const AST::Component::Export &Ex) noexcept {
  // Validation steps:
  //   1. Export name is strongly-unique across exports.
  //   2. `sortidx` is in-bounds and (for core sorts) must be `core module`.
  //   3. If an `externdesc` ascription is present, it must be a supertype
  //      of the inferred externdesc of the `sortidx` (kind-only check for
  //      now; structural subtype is a follow-up).
  //   4. The export name parses under the export-name grammar.
  //   5. The export introduces a new index aliasing the definition in the
  //      component's own index space for its sort.
  //
  // Not yet enforced: transitive resource-avoidance in exported types;
  // flipping the "consumed" flag for value exports.
  if (!CompCtx.addExportedName(Ex.getName())) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    Export: Duplicate export name '{}'"sv, Ex.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }

  // Validate the sortidx bounds.
  const auto &Sort = Ex.getSortIndex().getSort();
  uint32_t Idx = Ex.getSortIndex().getIdx();
  if (Sort.isCore()) {
    // The externdesc grammar permits only `core module` as a component-level
    // core export — no other core sort is exportable from a component.
    if (Sort.getCoreSortType() != AST::Component::Sort::CoreSortType::Module) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Export: core sort other than `core module` is not allowed"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
  } else {
    if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
  }

  // If an externdesc ascription is present, the spec requires it to be a
  // supertype of the inferred externdesc of the sortidx. For now we only
  // enforce that the ascription's kind matches the sortidx's sort; full
  // structural subtype between ascription and inferred type is not yet
  // implemented.
  if (Ex.getDesc().has_value() &&
      !sortMatchesDescType(Sort, Ex.getDesc()->getDescType())) {
    spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
    spdlog::error(
        "    Export: ascribed externdesc kind does not match sortidx sort"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
  }

  // exportname ::= <plainname> | <interfacename>
  EXPECTED_TRY(validateExportName(Ex.getName()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return E;
  }));

  // Every export introduces a new index that aliases the exported
  // definition in the component's own index space for that sort. For
  // instance-sort exports we also copy/build the export table so that a
  // later alias-export against this slot can resolve its sub-exports.
  if (Sort.isCore()) {
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  } else {
    const AST::Component::InstanceType *IT = nullptr;
    const bool IsInst =
        Sort.getSortType() == AST::Component::Sort::SortType::Instance;
    const bool HasInstAscription =
        IsInst && Ex.getDesc().has_value() &&
        Ex.getDesc()->getDescType() ==
            AST::Component::ExternDesc::DescType::InstanceType;
    if (HasInstAscription) {
      IT = CompCtx.getInstanceType(Ex.getDesc()->getTypeIndex());
      if (IT != nullptr) {
        if (auto Missing = findMissingRequiredExport(CompCtx, Idx, *IT)) {
          spdlog::error(ErrCode::Value::ExportAscriptionIncompatible);
          spdlog::error(
              "    Export: ascribed instance type requires export '{}' "
              "not present in inferred type"sv,
              *Missing);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
          return Unexpect(ErrCode::Value::ExportAscriptionIncompatible);
        }
      }
    }
    uint32_t NewIdx = CompCtx.incSortIndexSize(Sort.getSortType());
    if (IsInst) {
      if (IT != nullptr) {
        populateInstanceFromType(CompCtx, NewIdx, *IT);
      } else {
        // Either no ascription, or the ascription's type index didn't
        // resolve to an inline InstanceType here (cross-scope type-index
        // resolution is not yet implemented). Copy the source instance's
        // inferred exports so a later alias-export on this slot can still
        // find them.
        const auto &SrcExports = CompCtx.getInstance(Idx);
        for (const auto &[Name, IE] : SrcExports) {
          CompCtx.addInstanceExport(NewIdx, Name, IE.ST, IE.IT,
                                    IE.NestedInstIdx);
        }
      }
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExternDesc &Desc) noexcept {
  // Validating an externdesc introduces a new entry into the index space
  // matching the descriptor's sort:
  //   * core module (CoreType) → core:module
  //   * func                   → func
  //   * value                  → value (consumed=false; not yet tracked)
  //   * type (eq i)            → type aliased to i (resource property
  //                              propagated when i refers to a resource)
  //   * type (sub resource)    → fresh abstract resource type
  //   * component / instance   → component / instance
  //
  // TODO: validate referenced typeidx kinds against the declared sort
  // (functype / componenttype / instancetype / core:moduletype) and,
  // transitively, the contents of any component/instance type bodies.
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType:
    // The CoreType externdesc is `(core module (type i))`, so it introduces
    // a new core module index, not a core type.
    CompCtx.addCoreModule();
    break;
  case AST::Component::ExternDesc::DescType::FuncType:
    CompCtx.addFunc();
    break;
  case AST::Component::ExternDesc::DescType::ValueBound:
    CompCtx.addValue();
    break;
  case AST::Component::ExternDesc::DescType::TypeBound:
    if (Desc.isEqType()) {
      // (type (eq i)) — alias type i
      uint32_t RefIdx = Desc.getTypeIndex();
      if (RefIdx >=
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error("    ExternDesc: eq type bound index {} out of bounds"sv,
                      RefIdx);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      // Create alias: propagate resource property if referenced type is
      // resource
      uint32_t NewIdx = CompCtx.addTypeImported(nullptr);
      if (CompCtx.isResourceType(RefIdx)) {
        CompCtx.getCurrentContext().ResourceTypes[NewIdx] = nullptr;
      }
    } else {
      // (type (sub resource)) — fresh abstract resource type
      uint32_t NewIdx = CompCtx.addTypeImported(nullptr);
      CompCtx.getCurrentContext().ResourceTypes[NewIdx] = nullptr;
    }
    break;
  case AST::Component::ExternDesc::DescType::ComponentType:
    CompCtx.addComponent();
    break;
  case AST::Component::ExternDesc::DescType::InstanceType: {
    uint32_t InstIdx = CompCtx.addInstance();
    const auto *IT = CompCtx.getInstanceType(Desc.getTypeIndex());
    if (IT != nullptr) {
      populateInstanceFromType(CompCtx, InstIdx, *IT);
    }
    // If the type index didn't resolve to an inline InstanceType here, it
    // may reference a type in an outer scope (e.g. inside
    // componenttype/instancetype bodies). Outer-scope type-index
    // resolution is not yet implemented, so the instance is left with an
    // empty export table.
    break;
  }
  default:
    assumingUnreachable();
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreImportDesc &Desc) noexcept {
  if (Desc.isFunc()) {
    uint32_t TypeIdx = Desc.getTypeIndex();
    if (TypeIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Type)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    CoreImportDesc: func type index {} out of bounds"sv,
                    TypeIdx);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    CompCtx.addCoreFunc();
  } else if (Desc.isTable()) {
    CompCtx.addCoreTable();
  } else if (Desc.isMemory()) {
    CompCtx.addCoreMemory();
  } else if (Desc.isGlobal()) {
    CompCtx.addCoreGlobal();
  } else if (Desc.isTag()) {
    CompCtx.addCoreTag();
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreImportDecl &Decl) noexcept {
  return validate(Decl.getImportDesc());
}

Expect<void>
Validator::validate(const AST::Component::CoreExportDecl &Decl) noexcept {
  return validate(Decl.getImportDesc());
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_CoreModule));
    return E;
  };

  if (Decl.isImport()) {
    EXPECTED_TRY(validate(Decl.getImport()).map_error(ReportError));
  } else if (Decl.isType()) {
    EXPECTED_TRY(validate(*Decl.getType()).map_error(ReportError));
  } else if (Decl.isAlias()) {
    EXPECTED_TRY(validate(Decl.getAlias()).map_error(ReportError));
  } else if (Decl.isExport()) {
    EXPECTED_TRY(validate(Decl.getExport()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ImportDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
    return E;
  };

  // Validate the extern descriptor (also increments sort index spaces).
  EXPECTED_TRY(validate(Decl.getExternDesc()).map_error(ReportError));

  // Parse and validate the import name.
  EXPECTED_TRY(ComponentName CName,
               ComponentName::parse(Decl.getName()).map_error(ReportError));

  // Annotated plainnames can only appear on func imports.
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
    if (Decl.getExternDesc().getDescType() !=
        AST::Component::ExternDesc::DescType::FuncType) {
      spdlog::error(ErrCode::Value::ComponentInvalidName);
      spdlog::error("    ImportDecl: annotated name requires func type"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    break;
  default:
    break;
  }

  // Check import name uniqueness.
  if (!CompCtx.addImportedName(CName)) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    ImportDecl: Duplicate import name"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Import));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExportDecl &Decl) noexcept {
  // Check export name uniqueness before mutating the index spaces so a
  // duplicate or malformed name doesn't widen the scope's sort counts.
  if (!CompCtx.addExportedName(Decl.getName())) {
    spdlog::error(ErrCode::Value::ComponentDuplicateName);
    spdlog::error("    ExportDecl: Duplicate export name '{}'"sv,
                  Decl.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
    return Unexpect(ErrCode::Value::ComponentDuplicateName);
  }
  EXPECTED_TRY(validateExportName(Decl.getName()).map_error([](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Export));
    return E;
  }));

  // Increment sort index spaces based on the extern descriptor type.
  // Full ExternDesc validation (type index bounds) is deferred because
  // ExportDecls inside nested InstanceTypes reference type indices from
  // the defining scope, not the InstanceType's own scope.
  const auto &Desc = Decl.getExternDesc();
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType:
    // The CoreType externdesc is `(core module (type i))`, so it introduces
    // a new core module index, not a core type.
    CompCtx.addCoreModule();
    break;
  case AST::Component::ExternDesc::DescType::FuncType:
    CompCtx.addFunc();
    break;
  case AST::Component::ExternDesc::DescType::ValueBound:
    CompCtx.addValue();
    break;
  case AST::Component::ExternDesc::DescType::TypeBound: {
    uint32_t NewIdx = CompCtx.addTypeImported(nullptr);
    if (!Desc.isEqType()) {
      // (type (sub resource)) — fresh abstract resource type
      CompCtx.getCurrentContext().ResourceTypes[NewIdx] = nullptr;
    } else {
      // (type (eq i)) — propagate resource property if in bounds
      uint32_t RefIdx = Desc.getTypeIndex();
      if (RefIdx <
              CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type) &&
          CompCtx.isResourceType(RefIdx)) {
        CompCtx.getCurrentContext().ResourceTypes[NewIdx] = nullptr;
      }
    }
    break;
  }
  case AST::Component::ExternDesc::DescType::ComponentType:
    CompCtx.addComponent();
    break;
  case AST::Component::ExternDesc::DescType::InstanceType: {
    // Populate the new instance slot with the referenced InstanceType's
    // exports so a subsequent `alias export` against this slot can resolve
    // its sub-exports (mirrors the ExternDesc-for-imports handling).
    uint32_t InstIdx = CompCtx.addInstance();
    const auto *IT = CompCtx.getInstanceType(Desc.getTypeIndex());
    if (IT != nullptr) {
      populateInstanceFromType(CompCtx, InstIdx, *IT);
    }
    break;
  }
  default:
    assumingUnreachable();
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Instance));
    return E;
  };

  if (Decl.isCoreType()) {
    EXPECTED_TRY(validate(*Decl.getCoreType()).map_error(ReportError));
  } else if (Decl.isType()) {
    EXPECTED_TRY(validate(*Decl.getType()).map_error(ReportError));
  } else if (Decl.isAlias()) {
    EXPECTED_TRY(validate(Decl.getAlias()).map_error(ReportError));
    const auto &A = Decl.getAlias();
    const auto &Sort = A.getSort();
    if (Sort.isCore()) {
      CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
    } else {
      uint32_t NewInstIdx = CompCtx.incSortIndexSize(Sort.getSortType());
      // When aliasing an instance export that is itself an instance,
      // propagate the source instance's export table into the new slot so
      // subsequent `alias export` on this slot can resolve nested exports
      // (mirrors the AliasSection handling).
      if (Sort.getSortType() == AST::Component::Sort::SortType::Instance &&
          A.getTargetType() == AST::Component::Alias::TargetType::Export) {
        const auto SrcInstIdx = A.getExport().first;
        const auto &SrcName = A.getExport().second;
        const auto &SrcExports = CompCtx.getInstance(SrcInstIdx);
        auto It = SrcExports.find(std::string(SrcName));
        if (It != SrcExports.end()) {
          if (It->second.IT != nullptr) {
            populateInstanceFromType(CompCtx, NewInstIdx, *It->second.IT);
          } else if (It->second.NestedInstIdx.has_value()) {
            const auto &NestedExports =
                CompCtx.getInstance(*It->second.NestedInstIdx);
            for (const auto &[Name, IE] : NestedExports) {
              CompCtx.addInstanceExport(NewInstIdx, Name, IE.ST, IE.IT,
                                        IE.NestedInstIdx);
            }
          }
        }
      }
    }
  } else if (Decl.isExportDecl()) {
    EXPECTED_TRY(validate(Decl.getExport()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentDecl &Decl) noexcept {
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Decl_Component));
    return E;
  };

  if (Decl.isImportDecl()) {
    EXPECTED_TRY(validate(Decl.getImport()).map_error(ReportError));
  } else if (Decl.isInstanceDecl()) {
    EXPECTED_TRY(validate(Decl.getInstance()).map_error(ReportError));
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void> Validator::validate(const ComponentValType &VT) noexcept {
  if (VT.getCode() == ComponentTypeCode::TypeIndex) {
    uint32_t Idx = VT.getTypeIndex();
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    ComponentValType: type index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    const auto *DT = CompCtx.getDefType(Idx);
    if (DT != nullptr && !DT->isDefValType() && !DT->isResourceType()) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    ComponentValType: type index {} is not a defined value type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefValType &DVT) noexcept {
  if (DVT.isOwnTy()) {
    uint32_t Idx = DVT.getOwn().Idx;
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    DefValType: own type index {} out of bounds"sv, Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (!CompCtx.isResourceType(Idx)) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    DefValType: own type index {} does not refer to a resource type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  } else if (DVT.isBorrowTy()) {
    uint32_t Idx = DVT.getBorrow().Idx;
    if (Idx >= CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::DefTypeIndexOutOfBounds);
      spdlog::error("    DefValType: borrow type index {} out of bounds"sv,
                    Idx);
      return Unexpect(ErrCode::Value::DefTypeIndexOutOfBounds);
    }
    if (!CompCtx.isResourceType(Idx)) {
      spdlog::error(ErrCode::Value::NotADefinedType);
      spdlog::error(
          "    DefValType: borrow type index {} does not refer to a resource type"sv,
          Idx);
      return Unexpect(ErrCode::Value::NotADefinedType);
    }
  } else if (DVT.isRecordTy()) {
    const auto &Rec = DVT.getRecord();
    if (Rec.LabelTypes.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: record must have at least one field"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &LT : Rec.LabelTypes) {
      if (LT.getLabel().empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(LT.getLabel())) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    DefValType: record field '{}' is not valid kebab-case"sv,
            LT.getLabel());
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      if (!Seen.insert(toLowerStr(LT.getLabel())).second) {
        spdlog::error(ErrCode::Value::RecordFieldNameConflicts);
        spdlog::error("    DefValType: duplicate record field '{}'"sv,
                      LT.getLabel());
        return Unexpect(ErrCode::Value::RecordFieldNameConflicts);
      }
      EXPECTED_TRY(validate(LT.getValType()));
    }
  } else if (DVT.isVariantTy()) {
    const auto &Var = DVT.getVariant();
    if (Var.Cases.empty()) {
      spdlog::error(ErrCode::Value::VariantMustHaveCase);
      return Unexpect(ErrCode::Value::VariantMustHaveCase);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &C : Var.Cases) {
      if (C.first.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(C.first)) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    DefValType: variant case '{}' is not valid kebab-case"sv,
            C.first);
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      if (!Seen.insert(toLowerStr(C.first)).second) {
        spdlog::error(ErrCode::Value::VariantCaseNameConflicts);
        spdlog::error("    DefValType: duplicate variant case '{}'"sv, C.first);
        return Unexpect(ErrCode::Value::VariantCaseNameConflicts);
      }
      if (C.second.has_value()) {
        EXPECTED_TRY(validate(*C.second));
      }
    }
  } else if (DVT.isTupleTy()) {
    if (DVT.getTuple().Types.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: tuple must have at least one element"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    for (const auto &T : DVT.getTuple().Types) {
      EXPECTED_TRY(validate(T));
    }
  } else if (DVT.isListTy()) {
    EXPECTED_TRY(validate(DVT.getList().ValTy));
  } else if (DVT.isOptionTy()) {
    EXPECTED_TRY(validate(DVT.getOption().ValTy));
  } else if (DVT.isResultTy()) {
    const auto &R = DVT.getResult();
    if (R.ValTy.has_value()) {
      EXPECTED_TRY(validate(*R.ValTy));
    }
    if (R.ErrTy.has_value()) {
      EXPECTED_TRY(validate(*R.ErrTy));
    }
  } else if (DVT.isFlagsTy()) {
    const auto &Flags = DVT.getFlags();
    if (Flags.Labels.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: flags must have at least one label"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (Flags.Labels.size() > 32) {
      spdlog::error(ErrCode::Value::CannotHaveMoreThan32Flags);
      return Unexpect(ErrCode::Value::CannotHaveMoreThan32Flags);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &L : Flags.Labels) {
      if (L.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(L)) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    DefValType: flags label '{}' is not valid kebab-case"sv, L);
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      if (!Seen.insert(toLowerStr(L)).second) {
        spdlog::error(ErrCode::Value::FlagNameConflicts);
        spdlog::error("    DefValType: duplicate flags label '{}'"sv, L);
        return Unexpect(ErrCode::Value::FlagNameConflicts);
      }
    }
  } else if (DVT.isEnumTy()) {
    const auto &Enm = DVT.getEnum();
    if (Enm.Labels.empty()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    DefValType: enum must have at least one label"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    std::unordered_set<std::string> Seen;
    for (const auto &L : Enm.Labels) {
      if (L.empty()) {
        spdlog::error(ErrCode::Value::NameCannotBeEmpty);
        return Unexpect(ErrCode::Value::NameCannotBeEmpty);
      }
      if (!isKebabString(L)) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    DefValType: enum label '{}' is not valid kebab-case"sv, L);
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      if (!Seen.insert(toLowerStr(L)).second) {
        spdlog::error(ErrCode::Value::EnumTagNameConflicts);
        spdlog::error("    DefValType: duplicate enum label '{}'"sv, L);
        return Unexpect(ErrCode::Value::EnumTagNameConflicts);
      }
    }
  } else if (DVT.isStreamTy()) {
    if (DVT.getStream().ValTy.has_value()) {
      EXPECTED_TRY(validate(*DVT.getStream().ValTy));
    }
  } else if (DVT.isFutureTy()) {
    if (DVT.getFuture().ValTy.has_value()) {
      EXPECTED_TRY(validate(*DVT.getFuture().ValTy));
    }
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::FuncType &FT) noexcept {
  // Validate param names: kebab-case + unique
  std::unordered_set<std::string_view> ParamNames;
  for (const auto &P : FT.getParamList()) {
    if (!P.getLabel().empty()) {
      if (!isKebabString(P.getLabel())) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    FuncType: parameter name '{}' is not valid kebab-case"sv,
            P.getLabel());
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      if (!ParamNames.insert(P.getLabel()).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error("    FuncType: duplicate parameter name '{}'"sv,
                      P.getLabel());
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
    }
    EXPECTED_TRY(validate(P.getValType()));
  }
  // Reject transitive use of borrow in results
  for (const auto &R : FT.getResultList()) {
    EXPECTED_TRY(validate(R.getValType()));
    if (containsBorrow(R.getValType())) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    FuncType: borrow type not allowed in function results"sv);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceType &IT) noexcept {
  // Instance types are validated with an initially-empty index space.
  CompCtx.enterTypeDefinition();
  for (const auto &Decl : IT.getDecl()) {
    EXPECTED_TRY(validate(Decl).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return E;
    }));
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentType &CT) noexcept {
  // Component types are validated with an initially-empty index space.
  CompCtx.enterTypeDefinition();
  for (const auto &Decl : CT.getDecl()) {
    EXPECTED_TRY(validate(Decl).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return E;
    }));
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ResourceType &RT) noexcept {
  // Resource types are not allowed inside componenttype/instancetype scopes.
  if (CompCtx.isTypeDefinitionScope()) {
    spdlog::error(ErrCode::Value::InvalidTypeReference);
    spdlog::error("    ResourceType: resource types cannot be defined inside "
                  "componenttype or instancetype"sv);
    return Unexpect(ErrCode::Value::InvalidTypeReference);
  }
  if (RT.getDestructor().has_value()) {
    uint32_t DtorIdx = *RT.getDestructor();
    if (DtorIdx >= CompCtx.getCoreSortIndexSize(
                       AST::Component::Sort::CoreSortType::Func)) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    ResourceType: destructor core func index {} out of bounds"sv,
          DtorIdx);
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // TODO: validate that the destructor's core func type is `[i32] -> []`
    // (or `[i64] -> []` when the resource rep is i64). This requires
    // tracking the signature of each core func, which the validator does
    // not yet do.
  }
  return {};
}

bool Validator::containsBorrow(const ComponentValType &VT) const noexcept {
  if (VT.getCode() == ComponentTypeCode::Borrow) {
    return true;
  }
  if (VT.getCode() != ComponentTypeCode::TypeIndex) {
    return false;
  }
  uint32_t Idx = VT.getTypeIndex();
  const auto *DT = CompCtx.getDefType(Idx);
  if (DT == nullptr || !DT->isDefValType()) {
    return false;
  }
  return containsBorrow(DT->getDefValType());
}

bool Validator::containsBorrow(
    const AST::Component::DefValType &DVT) const noexcept {
  if (DVT.isBorrowTy()) {
    return true;
  }
  if (DVT.isRecordTy()) {
    for (const auto &F : DVT.getRecord().LabelTypes) {
      if (containsBorrow(F.getValType())) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isVariantTy()) {
    for (const auto &C : DVT.getVariant().Cases) {
      if (C.second.has_value() && containsBorrow(*C.second)) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isListTy()) {
    return containsBorrow(DVT.getList().ValTy);
  }
  if (DVT.isTupleTy()) {
    for (const auto &T : DVT.getTuple().Types) {
      if (containsBorrow(T)) {
        return true;
      }
    }
    return false;
  }
  if (DVT.isOptionTy()) {
    return containsBorrow(DVT.getOption().ValTy);
  }
  if (DVT.isResultTy()) {
    const auto &R = DVT.getResult();
    return (R.ValTy.has_value() && containsBorrow(*R.ValTy)) ||
           (R.ErrTy.has_value() && containsBorrow(*R.ErrTy));
  }
  if (DVT.isStreamTy()) {
    return DVT.getStream().ValTy.has_value() &&
           containsBorrow(*DVT.getStream().ValTy);
  }
  if (DVT.isFutureTy()) {
    return DVT.getFuture().ValTy.has_value() &&
           containsBorrow(*DVT.getFuture().ValTy);
  }
  return false; // PrimValType, OwnTy, FlagsTy, EnumTy
}

} // namespace Validator
} // namespace WasmEdge
