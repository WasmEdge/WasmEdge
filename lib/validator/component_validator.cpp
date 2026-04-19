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
      // GAP-I-3: InstanceExport::ST cannot represent a `(core module)` export
      // (its SortType field is component-side only). Skip rather than crash;
      // the entry stays unrecorded until InstanceExport gains a core-sort
      // variant.
      spdlog::debug(
          "    populateInstanceFromType: skipping `(core module)` export "
          "'{}' (GAP-I-3)"sv,
          Exp.getName());
      continue;
    }
    const AST::Component::InstanceType *NestedIT = nullptr;
    if (ED.getDescType() ==
        AST::Component::ExternDesc::DescType::InstanceType) {
      NestedIT = Ctx.getInstanceType(ED.getTypeIndex());
    }
    Ctx.addInstanceExport(InstIdx, Exp.getName(), *ST, NestedIT);
  }
}

// Resolve a type index in `Comp`'s own type index space to an InstanceType.
// Returns nullptr when the index does not refer to an inline InstanceType
// definition — callers treat nullptr as "no required shape" and fall back
// to inferred exports. GAP-B6: TypeBound imports and outer-alias type
// imports currently fall through to nullptr.
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
          // GAP-ED-1: IT==nullptr && no NestedInstIdx; no source to copy.
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
  // TODO: Validation prevents duplicate or conflicting canonopt.
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
  // TODO: Validation requires f have functype with param arity and types
  // matching arg* and result arity r.

  // TODO: Validation appends the result types of f to the value index space
  // (making them available for reference by subsequent definitions).

  // TODO: In addition to the type-compatibility checks mentioned above, the
  // validation rules for value definitions additionally require that each
  // value is consumed exactly once. Thus, during validation, each value has
  // an associated "consumed" boolean flag. When a value is first added to
  // the value index space (via import, instance, alias or start), the flag
  // is clear. When a value is used (via export, instantiate or start), the
  // flag is set. After validating the last definition of a component,
  // validation requires all values' flags are set.
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
    // Get the import descriptors of the module to instantiate.
    const auto *Mod = CompCtx.getCoreModule(ModIdx);
    // TODO(GAP-CI-1): use module type info for imported/aliased modules.
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
    for (const auto &Export : Inst.getInlineExports()) {
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
    // Get the imports of the component to instantiate.
    const auto *Comp = CompCtx.getComponent(CompIdx);
    // TODO(GAP-I-1): use component type info instead of raw AST for
    // imported/aliased components. For now, only check inline components.
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
      // Get the instantiate args.
      auto Args = Inst.getInstantiateArgs();
      // Check the import names are supplied from the instantiate args.
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
        // Basic sort-kind matching (full subtype checking is GAP-I-2).
        const auto &ImportDesc = *It->second;
        const auto &Sort = ArgIt->getIndex().getSort();
        const uint32_t Idx = ArgIt->getIndex().getIdx();
        if (!Sort.isCore()) {
          bool SortMatch = false;
          switch (ImportDesc.getDescType()) {
          case AST::Component::ExternDesc::DescType::FuncType:
            SortMatch =
                Sort.getSortType() == AST::Component::Sort::SortType::Func;
            break;
          case AST::Component::ExternDesc::DescType::ComponentType:
            SortMatch =
                Sort.getSortType() == AST::Component::Sort::SortType::Component;
            break;
          case AST::Component::ExternDesc::DescType::InstanceType:
            SortMatch =
                Sort.getSortType() == AST::Component::Sort::SortType::Instance;
            break;
          case AST::Component::ExternDesc::DescType::ValueBound:
            SortMatch =
                Sort.getSortType() == AST::Component::Sort::SortType::Value;
            break;
          case AST::Component::ExternDesc::DescType::TypeBound:
          case AST::Component::ExternDesc::DescType::CoreType:
            SortMatch =
                Sort.getSortType() == AST::Component::Sort::SortType::Type;
            break;
          default:
            break;
          }
          if (!SortMatch) {
            spdlog::error(ErrCode::Value::ArgTypeMismatch);
            spdlog::error(
                "    Instance: Argument '{}' sort mismatch for import"sv,
                ImportName);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::ArgTypeMismatch);
          }
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
    for (const auto &Export : Inst.getInlineExports()) {
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
      // GAP-I-2: IT not forwarded; NestedInstIdx suffices for alias chains.
      // TODO: reject duplicate inline-export names (strongly-unique).
      CompCtx.addInstanceExport(InstanceIdx, Export.getName(),
                                Sort.getSortType(), nullptr, NestedIdx);
    }
  } else {
    assumingUnreachable();
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
        if (TargetCtx->ResourceTypes.find(Idx) !=
            TargetCtx->ResourceTypes.end()) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "    Alias outer: Cannot outer-alias a resource type at index {}. Resource types are generative."sv,
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
    CompCtx.enterComponent();
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
  // TODO: validation specifies
  switch (Canon.getOpCode()) {
  case AST::Component::Canonical::OpCode::Lift:
    CompCtx.addFunc();
    return {};
  case AST::Component::Canonical::OpCode::Lower:
  case AST::Component::Canonical::OpCode::Resource__new:
  case AST::Component::Canonical::OpCode::Resource__drop:
  case AST::Component::Canonical::OpCode::Resource__rep:
    CompCtx.addCoreFunc();
    return {};
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
}

Expect<void> Validator::validate(const AST::Component::Import &Im) noexcept {
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
  // Check export name uniqueness.
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
    if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
  } else {
    if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    Export: sort index {} out of bounds"sv, Idx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
  }

  // exportname ::= <plainname> | <interfacename>
  EXPECTED_TRY(ComponentName CName,
               ComponentName::parse(Ex.getName()).map_error([](auto E) {
                 spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
                 return E;
               }));
  switch (CName.getKind()) {
  case ComponentNameKind::Label:
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
    break;
  default:
    spdlog::error(ErrCode::Value::ComponentInvalidName);
    spdlog::error("    Export: name kind not valid for exports"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::ComponentInvalidName);
  }

  // Binary.md:405-408 — all exports (of all sorts) introduce a new index.
  if (Sort.isCore()) {
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  } else {
    uint32_t NewIdx = CompCtx.incSortIndexSize(Sort.getSortType());
    // Instance alias slot: propagate the source instance's export table so
    // a later `alias export` on this slot resolves nested exports.
    if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
      const auto &SrcExports = CompCtx.getInstance(Idx);
      for (const auto &[Name, IE] : SrcExports) {
        CompCtx.addInstanceExport(NewIdx, Name, IE.ST, IE.IT,
                                  IE.NestedInstIdx);
      }
    }
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExternDesc &Desc) noexcept {
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType:
    // Binary.md:233 — externdesc CoreType is `(core module (type i))`,
    // which belongs to core:module, not core:type.
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
      uint32_t NewIdx = CompCtx.addType();
      if (CompCtx.isResourceType(RefIdx)) {
        CompCtx.getCurrentContext().ResourceTypes[NewIdx] = nullptr;
      }
    } else {
      // (type (sub resource)) — fresh abstract resource type
      uint32_t NewIdx = CompCtx.addType();
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
    // GAP-ED-1: when IT is nullptr the type index may reference a type in
    // an outer scope (e.g. inside componenttype/instancetype definitions);
    // outer-scope type-index resolution is not yet implemented.
    break;
  }
  default:
    assumingUnreachable();
  }

  // TODO: Validate the type index
  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleDecl &) noexcept {
  // TODO
  return {};
}

Expect<void> Validator::validate(const AST::Component::ImportDecl &) noexcept {
  // TODO
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceDecl &) noexcept {
  // TODO
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
  CompCtx.enterComponent();
  for (const auto &Decl : IT.getDecl()) {
    if (Decl.isCoreType()) {
      EXPECTED_TRY(validate(*Decl.getCoreType()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
    } else if (Decl.isType()) {
      EXPECTED_TRY(validate(*Decl.getType()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
    } else if (Decl.isAlias()) {
      EXPECTED_TRY(validate(Decl.getAlias()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
      const auto &Sort = Decl.getAlias().getSort();
      if (Sort.isCore()) {
        CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
      } else {
        CompCtx.incSortIndexSize(Sort.getSortType());
      }
    } else if (Decl.isExportDecl()) {
      // TODO: validate export declarations within instance types.
    } else {
      assumingUnreachable();
    }
  }
  CompCtx.exitComponent();
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ComponentType &CT) noexcept {
  // Component types are validated with an initially-empty index space.
  CompCtx.enterComponent();
  for (const auto &Decl : CT.getDecl()) {
    if (Decl.isImportDecl()) {
      EXPECTED_TRY(validate(Decl.getImport()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
    } else if (Decl.isInstanceDecl()) {
      EXPECTED_TRY(validate(Decl.getInstance()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
    } else {
      assumingUnreachable();
    }
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
    // TODO(GAP-C-1): validate destructor has core type [i32] -> []
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
