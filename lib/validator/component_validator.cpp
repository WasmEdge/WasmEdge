// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_name.h"
#include "validator/validator.h"

#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

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

  CompCtx.enterComponent(Comp);
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
  CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Module);
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
  CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Component);
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
      CompCtx.incSortIndexSize(Sort.getSortType());
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
    if (ModIdx >= CompCtx.getCoreModuleCount()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    CoreInstance: Module index {} exceeds available core modules {}"sv,
          ModIdx, CompCtx.getCoreModuleCount());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Get the import descriptors of the module to instantiate.
    const auto &Mod = CompCtx.getCoreModule(ModIdx);
    const auto &ImportDescs = Mod.getImportSection().getContent();
    // Get the instantiate args.
    auto Args = Inst.getInstantiateArgs();
    // Check the import module names are supplied from the instantiate args.
    for (const auto &Import : ImportDescs) {
      auto ArgIt = std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
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
    // Add the module exports and bind to the core:instance index.
    uint32_t InstanceIdx = CompCtx.getCoreSortIndexSize(
        AST::Component::Sort::CoreSortType::Instance);
    const auto &ExportDescs = Mod.getExportSection().getContent();
    for (const auto &ExportDesc : ExportDescs) {
      CompCtx.addCoreInstanceExport(InstanceIdx, ExportDesc.getExternalName(),
                                    ExportDesc.getExternalType());
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.

    // Check the core:sort index bound of the inline exports.
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
    }
  } else {
    assumingUnreachable();
  }
  // Increase the sort index of the core:instance.
  CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Instance);
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Instance &Inst) noexcept {
  if (Inst.isInstantiateModule()) {
    // Instantiate module case.

    // Check the component index bound first.
    const uint32_t CompIdx = Inst.getComponentIndex();
    if (CompIdx >= CompCtx.getComponentCount()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Instance: Component index {} exceeds available components {}"sv,
          CompIdx, CompCtx.getComponentCount());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
    // Get the imports of the component to instantiate.
    const auto &Comp = CompCtx.getComponent(CompIdx);
    // Prepare the import descriptor map.
    // TODO: move this to the context.
    std::unordered_map<std::string, const AST::Component::ExternDesc *>
        ImportMap;
    for (const auto &Sec : Comp.getSections()) {
      if (std::holds_alternative<AST::Component::ImportSection>(Sec)) {
        const auto &ImportSec = std::get<AST::Component::ImportSection>(Sec);
        for (const auto &Import : ImportSec.getContent()) {
          // TODO: strongly-unique problem of the import name.
          ImportMap[std::string(Import.getName())] = &Import.getDesc();
        }
      }
    }
    // Get the instantiate args.
    auto Args = Inst.getInstantiateArgs();
    // Check the import names are supplied from the instantiate args.
    for (auto It = ImportMap.begin(); It != ImportMap.end(); ++It) {
      const auto &ImportName = It->first;
      const auto &ImportDesc = *It->second;
      auto ArgIt = std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
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

      const auto &Sort = ArgIt->getIndex().getSort();
      const uint32_t Idx = ArgIt->getIndex().getIdx();

      if (Sort.isCore()) {
        // Check the import descriptor type matches the core:sort type.
        bool IsMatched = false;
        switch (ImportDesc.getDescType()) {
        case AST::Component::ExternDesc::DescType::CoreType:
          IsMatched = Sort.getCoreSortType() ==
                      AST::Component::Sort::CoreSortType::Type;
          break;
        case AST::Component::ExternDesc::DescType::FuncType:
          IsMatched = Sort.getCoreSortType() ==
                      AST::Component::Sort::CoreSortType::Func;
          break;
        case AST::Component::ExternDesc::DescType::ValueBound:
          IsMatched = Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Func ||
                      Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Table ||
                      Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Memory ||
                      Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Global;
          break;
        case AST::Component::ExternDesc::DescType::TypeBound:
          IsMatched = Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Module ||
                      Sort.getCoreSortType() ==
                          AST::Component::Sort::CoreSortType::Instance;
          break;
        case AST::Component::ExternDesc::DescType::ComponentType:
          IsMatched = Sort.getCoreSortType() ==
                      AST::Component::Sort::CoreSortType::Module;
          break;
        case AST::Component::ExternDesc::DescType::InstanceType:
          IsMatched = Sort.getCoreSortType() ==
                      AST::Component::Sort::CoreSortType::Instance;
          break;
        default:
          break;
        }
        if (!IsMatched) {
          // TODO: print types to string
          spdlog::error(ErrCode::Value::ArgTypeMismatch);
          spdlog::error(
              "    Instance: Component index {} Argument '{}' type mismatch"sv,
              Inst.getComponentIndex(), ImportName);
          return Unexpect(ErrCode::Value::ArgTypeMismatch);
        }
        // Check the core:sort index bound of the args.
        if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
          spdlog::error(ErrCode::Value::InvalidIndex);
          spdlog::error(
              "    Instance: Argument '{}' refers to invalid index {}"sv,
              ImportName, Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::InvalidIndex);
        }
      } else {
        // Check the import descriptor type matches the sort type.
        bool IsMatched = false;
        switch (ImportDesc.getDescType()) {
        case AST::Component::ExternDesc::DescType::CoreType:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Type;
          break;
        case AST::Component::ExternDesc::DescType::FuncType:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Func;
          break;
        case AST::Component::ExternDesc::DescType::ValueBound:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Value;
          break;
        case AST::Component::ExternDesc::DescType::TypeBound:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Type;
          break;
        case AST::Component::ExternDesc::DescType::ComponentType:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Component;
          break;
        case AST::Component::ExternDesc::DescType::InstanceType:
          IsMatched =
              Sort.getSortType() == AST::Component::Sort::SortType::Instance;
          break;
        default:
          break;
        }
        if (!IsMatched) {
          // TODO: print types to string
          spdlog::error(ErrCode::Value::ArgTypeMismatch);
          spdlog::error(
              "    Instance: Component index {} Argument '{}' type mismatch"sv,
              Inst.getComponentIndex(), ImportName);
          return Unexpect(ErrCode::Value::ArgTypeMismatch);
        }
        // Check the sort index bound of the args.
        if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
          spdlog::error(ErrCode::Value::InvalidIndex);
          spdlog::error(
              "    Instance: Argument '{}' refers to invalid index {}"sv,
              ImportName, Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::InvalidIndex);
        }
        if (Sort.getSortType() == AST::Component::Sort::SortType::Type &&
            ImportDesc.getDescType() ==
                AST::Component::ExternDesc::DescType::TypeBound) {
          CompCtx.substituteTypeImport(ImportName, Idx);
        }
      }
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.

    // Check the sort index bound of the inline exports.
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
      } else {
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
      }
    }
  } else {
    assumingUnreachable();
  }
  // Increase the sort index of the instance.
  CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Instance);
  return {};
}

Expect<void> Validator::validate(const AST::Component::Alias &Alias) noexcept {
  const auto &Sort = Alias.getSort();
  switch (Alias.getTargetType()) {
  case AST::Component::Alias::TargetType::Export: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;
    const auto &CompExports = CompCtx.getComponentInstanceExports(Idx);

    if (Sort.isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias export: Mapping an export '{}' to core:sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }

    if (Idx >= CompCtx.getComponentInstanceExportsSize()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      auto Size = CompCtx.getComponentInstanceExportsSize();
      if (Size == 0) {
        spdlog::error(
            "    Alias export: Instance index {} invalid, no component instances available"sv,
            Idx);
      } else {
        spdlog::error(
            "    Alias export: Instance index {} exceeds available instances (max index: {})"sv,
            Idx, Size - 1);
      }
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    auto It = CompExports.find(Name);
    if (It == CompExports.cend()) {
      spdlog::error(ErrCode::Value::ExportNotFound);
      spdlog::error(
          "    Alias export: instance {} has no export named `{}`"sv,
          Idx, Name);
      return Unexpect(ErrCode::Value::ExportNotFound);
    }

    const auto *ExternDesc = It->second;
    AST::Component::Sort::SortType ST;
    std::string_view ActualType;
    switch (ExternDesc->getDescType()) {
    case AST::Component::ExternDesc::DescType::FuncType:
      ST = AST::Component::Sort::SortType::Func;
      ActualType = "func"sv;
      break;
    case AST::Component::ExternDesc::DescType::ValueBound:
    case AST::Component::ExternDesc::DescType::TypeBound:
      ST = AST::Component::Sort::SortType::Type;
      ActualType = "type"sv;
      break;
    case AST::Component::ExternDesc::DescType::ComponentType:
      ST = AST::Component::Sort::SortType::Component;
      ActualType = "component"sv;
      break;
    case AST::Component::ExternDesc::DescType::InstanceType:
      ST = AST::Component::Sort::SortType::Instance;
      ActualType = "instance"sv;
      break;
    case AST::Component::ExternDesc::DescType::CoreType:
    default:
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias export: Unknown mapping mismatch for export '{}'"sv, Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    if (ST != Sort.getSortType()) {
      std::string_view ExpectedType;
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func:
        ExpectedType = "func"sv;
        break;
      case AST::Component::Sort::SortType::Value:
        ExpectedType = "value"sv;
        break;
      case AST::Component::Sort::SortType::Type:
        ExpectedType = "type"sv;
        break;
      case AST::Component::Sort::SortType::Component:
        ExpectedType = "component"sv;
        break;
      case AST::Component::Sort::SortType::Instance:
        ExpectedType = "instance"sv;
        break;
      default:
        ExpectedType = "unknown"sv;
        break;
      }
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias export: export `{}` for instance {} is not a {} (actual: {})"sv,
          Name, Idx, ExpectedType, ActualType);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }
    return {};
  }
  case AST::Component::Alias::TargetType::CoreExport: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;
    const auto &CoreExports = CompCtx.getCoreInstanceExports(Idx);

    if (!Sort.isCore()) {
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error("    Alias core:export: Mapping a export '{}' to sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::InvalidTypeReference);
    }

    if (Idx >= CompCtx.getCoreInstanceExportsSize()) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      auto Size = CompCtx.getCoreInstanceExportsSize();
      if (Size == 0) {
        spdlog::error(
            "    Alias core:export: Core instance index {} invalid, no core instances available"sv,
            Idx);
      } else {
        spdlog::error(
            "    Alias core:export: Core instance index {} exceeds available core instances (max index: {})"sv,
            Idx, Size - 1);
      }
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    auto It = CoreExports.find(Name);
    if (It == CoreExports.end()) {
      spdlog::error(ErrCode::Value::ExportNotFound);
      spdlog::error(
          "    Alias core:export: core instance {} has no export named `{}`"sv,
          Idx, Name);
      return Unexpect(ErrCode::Value::ExportNotFound);
    }

    const auto ExternTy = It->second;
    AST::Component::Sort::CoreSortType ST;
    std::string_view ActualType;
    switch (ExternTy) {
    case ExternalType::Function:
      ST = AST::Component::Sort::CoreSortType::Func;
      ActualType = "func"sv;
      break;
    case ExternalType::Table:
      ST = AST::Component::Sort::CoreSortType::Table;
      ActualType = "table"sv;
      break;
    case ExternalType::Memory:
      ST = AST::Component::Sort::CoreSortType::Memory;
      ActualType = "memory"sv;
      break;
    case ExternalType::Global:
      ST = AST::Component::Sort::CoreSortType::Global;
      ActualType = "global"sv;
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
      std::string_view ExpectedType;
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func:
        ExpectedType = "func"sv;
        break;
      case AST::Component::Sort::CoreSortType::Table:
        ExpectedType = "table"sv;
        break;
      case AST::Component::Sort::CoreSortType::Memory:
        ExpectedType = "memory"sv;
        break;
      case AST::Component::Sort::CoreSortType::Global:
        ExpectedType = "global"sv;
        break;
      case AST::Component::Sort::CoreSortType::Type:
        ExpectedType = "type"sv;
        break;
      case AST::Component::Sort::CoreSortType::Module:
        ExpectedType = "module"sv;
        break;
      case AST::Component::Sort::CoreSortType::Instance:
        ExpectedType = "instance"sv;
        break;
      default:
        ExpectedType = "unknown"sv;
        break;
      }
      spdlog::error(ErrCode::Value::InvalidTypeReference);
      spdlog::error(
          "    Alias core:export: export `{}` for core instance {} is not a {} (actual: {})"sv,
          Name, Idx, ExpectedType, ActualType);
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
          "    Alias outer: invalid outer alias count of {}"sv,
          Ct);
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
        spdlog::error("    Alias outer: index out of bounds"sv);
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
        spdlog::error("    Alias outer: index out of bounds"sv);
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        if (TargetCtx->ComponentResourceTypes.find(Idx) !=
            TargetCtx->ComponentResourceTypes.end()) {
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
    CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Type);
  } else if (DType.isModuleType()) {
    for (const auto &Decl : DType.getModuleType()) {
      EXPECTED_TRY(validate(Decl).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return E;
      }));
    }
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::DefType &DType) noexcept {
  if (DType.isDefValType()) {
    // TODO: Validation of valtype requires the typeidx to refer to a
    // defvaltype.
    // TODO: Validation of own and borrow requires the typeidx to refer to a
    // resource type.
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
  } else if (DType.isFuncType()) {
    // TODO: Validation of functype rejects any transitive use of borrow in
    // a result type. Similarly, validation of components and component
    // types rejects any transitive use of borrow in an exported value type.
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
  } else if (DType.isComponentType()) {
    for (const auto &Decl : DType.getComponentType().getDecl()) {
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
      CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    }
    // TODO: Validation rejects resourcetype type definitions inside
    // componenttype and instancettype. Thus, handle types inside a
    // componenttype can only refer to resource types that are imported or
    // exported.

    // TODO: As described in the explainer, each component and instance type
    // is validated with an initially-empty type index space. Outer aliases
    // can be used to pull in type definitions from containing components.
  } else if (DType.isInstanceType()) {
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    CompCtx.addComponentInstanceType(
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type) - 1,
        DType.getInstanceType());
  } else if (DType.isResourceType()) {
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    CompCtx.addComponentResourceType(
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type) - 1,
        DType.getResourceType());
  } else {
    assumingUnreachable();
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::Canonical &Canon) noexcept {
  // TODO: validation specifies
  switch (Canon.getOpCode()) {
  case AST::Component::Canonical::OpCode::Lift:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Func);
    return {};
  case AST::Component::Canonical::OpCode::Lower:
  case AST::Component::Canonical::OpCode::Resource__new:
  case AST::Component::Canonical::OpCode::Resource__drop:
  case AST::Component::Canonical::OpCode::Resource__rep:
    CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
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

  ComponentName CName(Im.getName());
  switch (CName.getKind()) {
  case ComponentNameKind::InterfaceType:
  case ComponentNameKind::Label:
    break;
  case ComponentNameKind::Invalid:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    Import: Invalid import name"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    Import: Import name kind not supported yet"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }
  // TODO: Validation requires that annotated plainnames only occur on func
  // imports or exports and that the first label of a [constructor],
  // [method] or [static] matches the plainname of a preceding resource
  // import or export, respectively, in the same scope (component, component
  // type or instance type).

  // TODO: Validation of [constructor] names requires that the func returns
  // a (result (own $R)), where $R is the resource labeled r.

  // TODO: Validation of [method] names requires the first parameter of the
  // function to be (param "self" (borrow $R)), where $R is the resource
  // labeled r.

  // TODO: Validation of [method] and [static] names ensures that all field
  // names are disjoint.
  switch (CName.getKind()) {
  case ComponentNameKind::Constructor:
  case ComponentNameKind::Method:
  case ComponentNameKind::Static:
  case ComponentNameKind::InterfaceType:
  case ComponentNameKind::Label:
    if (!CompCtx.AddImportedName(CName)) {
      spdlog::error(ErrCode::Value::ComponentDuplicateName);
      spdlog::error("    Import: Duplicate import name"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::ComponentDuplicateName);
    }
    break;
  default:
    spdlog::error(ErrCode::Value::ComponentNotImplValidator);
    spdlog::error("    Import: Name is not resolved"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
    return Unexpect(ErrCode::Value::ComponentNotImplValidator);
  }

  return {};
}

Expect<void> Validator::validate(const AST::Component::Export &Ex) noexcept {
  if (Ex.getDesc().has_value()) {
    EXPECTED_TRY(validate(*Ex.getDesc()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return E;
    }));
  }
  const auto &Sort = Ex.getSortIndex().getSort();
  if (!Sort.isCore()) {
    CompCtx.incSortIndexSize(Sort.getSortType());
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExternDesc &Desc) noexcept {
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType:
    CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Type);
    break;
  case AST::Component::ExternDesc::DescType::FuncType:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Func);
    break;
  case AST::Component::ExternDesc::DescType::ValueBound:
  case AST::Component::ExternDesc::DescType::TypeBound:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    break;
  case AST::Component::ExternDesc::DescType::ComponentType:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Component);
    break;
  case AST::Component::ExternDesc::DescType::InstanceType: {
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Instance);
    const auto *IT = CompCtx.getComponentInstanceType(Desc.getTypeIndex());
    if (IT != nullptr) {
      auto InstDecls = IT->getDecl();
      for (auto &InstDecl : InstDecls) {
        if (InstDecl.isCoreType()) {
          spdlog::debug("CoreDefType found"sv);
          // TODO
        } else if (InstDecl.isType()) {
          spdlog::debug("DefType found"sv);
          // TODO
        } else if (InstDecl.isAlias()) {
          spdlog::debug("Alias found"sv);
          // TODO
        } else if (InstDecl.isExportDecl()) {
          spdlog::debug("Export Decl found"sv);
          const auto &Exp = InstDecl.getExport();
          uint32_t InstIdx = CompCtx.getSortIndexSize(
                                 AST::Component::Sort::SortType::Instance) -
                             1;
          CompCtx.addComponentInstanceExport(InstIdx, Exp.getName(),
                                             Exp.getExternDesc());
        } else {
          assumingUnreachable();
        }
      }
    } else {
      // TODO: check getCoreSortIndexSize(), because it may miss type, not out
      // of bound
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    ExternDesc: Instance index {} out of bound"sv,
                    Desc.getTypeIndex());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }
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

} // namespace Validator
} // namespace WasmEdge
