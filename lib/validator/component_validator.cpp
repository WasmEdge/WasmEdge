// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_name.h"
#include "validator/validator.h"

#include <unordered_set>
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

    // Check the module index bound using sort index (covers both inline and
    // imported modules).
    const uint32_t ModIdx = Inst.getModuleIndex();
    const uint32_t ModCount = CompCtx.getCoreSortIndexSize(
        AST::Component::Sort::CoreSortType::Module);
    if (ModIdx >= ModCount) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    CoreInstance: Module index {} exceeds available core modules {}"sv,
          ModIdx, ModCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    uint32_t InstanceIdx = CompCtx.addCoreInstance();

    const auto &ModEntry = CompCtx.getCoreModule(ModIdx);
    if (ModEntry.InlineMod == nullptr) {
      // Imported core module: use type info to check args and register exports.
      uint32_t TypeIdxVal = ModEntry.TypeIdx;
      if (TypeIdxVal != UINT32_MAX &&
          TypeIdxVal < CompCtx.getCoreSortIndexSize(
                           AST::Component::Sort::CoreSortType::Type)) {
        // Check imports from the module type.
        const auto &CoreTypeE = CompCtx.getCoreType(TypeIdxVal);
        {
          auto Args = Inst.getInstantiateArgs();
          // Collect unique module names from imports.
          std::unordered_set<std::string> RequiredModules;
          for (const auto &Imp : CoreTypeE.ModuleImports) {
            RequiredModules.insert(Imp.ModuleName);
          }
          for (const auto &ModName : RequiredModules) {
            auto ArgIt =
                std::find_if(Args.begin(), Args.end(), [&](const auto &Arg) {
                  return Arg.getName() == ModName;
                });
            if (ArgIt == Args.end()) {
              spdlog::error(ErrCode::Value::MissingArgument);
              spdlog::error(
                  "    CoreInstance: Module index {} missing argument for import '{}'"sv,
                  ModIdx, ModName);
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
              return Unexpect(ErrCode::Value::MissingArgument);
            }
            // Check each import from this module against the provided
            // core instance. For CoreInstance, args provide a core instance
            // index directly.
            const uint32_t ArgIdx = ArgIt->getIndex();
            {
              const auto &ArgExports = CompCtx.getCoreInstance(ArgIdx).Exports;
              for (const auto &Imp : CoreTypeE.ModuleImports) {
                if (Imp.ModuleName != ModName) {
                  continue;
                }
                auto ExpIt = ArgExports.find(Imp.FieldName);
                if (ExpIt == ArgExports.end()) {
                  spdlog::error(ErrCode::Value::ArgTypeMismatch);
                  spdlog::error(
                      "    CoreInstance: Import '{}' '{}' not found in provided instance"sv,
                      Imp.ModuleName, Imp.FieldName);
                  spdlog::error(
                      ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
                  return Unexpect(ErrCode::Value::ArgTypeMismatch);
                }
                // Check ExternalType match.
                if (ExpIt->second.ExtType != Imp.Type.ExtType) {
                  spdlog::error(ErrCode::Value::ArgTypeMismatch);
                  spdlog::error(
                      "    CoreInstance: Import '{}' '{}' external type mismatch"sv,
                      Imp.ModuleName, Imp.FieldName);
                  spdlog::error(
                      ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
                  return Unexpect(ErrCode::Value::ArgTypeMismatch);
                }
                // For functions, compare signatures.
                if (Imp.Type.ExtType == ExternalType::Function &&
                    Imp.Type.Sig.has_value() && ExpIt->second.Sig.has_value() &&
                    *Imp.Type.Sig != *ExpIt->second.Sig) {
                  spdlog::error(ErrCode::Value::ArgTypeMismatch);
                  spdlog::error(
                      "    CoreInstance: Import '{}' '{}' function signature mismatch"sv,
                      Imp.ModuleName, Imp.FieldName);
                  spdlog::error(
                      ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
                  return Unexpect(ErrCode::Value::ArgTypeMismatch);
                }
              }
            }
          }
        }
        // Register exports from the module type.
        for (const auto &Exp : CoreTypeE.ModuleExports) {
          CompCtx.addCoreInstanceExport(InstanceIdx, Exp.first, Exp.second);
        }
      }
    } else {
      // Inline-defined core module: use AST directly.
      const auto &Mod = *ModEntry.InlineMod;
      const auto &ImportDescs = Mod.getImportSection().getContent();
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
      const auto &ExportDescs = Mod.getExportSection().getContent();
      for (const auto &ExportDesc : ExportDescs) {
        CompCtx.addCoreInstanceExport(InstanceIdx, ExportDesc.getExternalName(),
                                      ExportDesc.getExternalType());
      }
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.
    uint32_t InlineInstIdx = CompCtx.addCoreInstance();

    // Check the core:sort index bound of the inline exports and register them.
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
      // Register the inline export in the core instance exports.
      ExternalType ET = ExternalType::Function;
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
        break;
      }
      CompCtx.addCoreInstanceExport(InlineInstIdx, Export.getName(), ET);
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

    // Check the component index bound using sort index (covers both inline
    // and imported components).
    const uint32_t CompIdx = Inst.getComponentIndex();
    const uint32_t CompCount =
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Component);
    if (CompIdx >= CompCount) {
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error(
          "    Instance: Component index {} exceeds available components {}"sv,
          CompIdx, CompCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
      return Unexpect(ErrCode::Value::InvalidIndex);
    }

    // For imported components (no AST available), skip detailed import checks.
    const auto &CompEntry = CompCtx.getComponent(CompIdx);
    if (CompEntry.Comp == nullptr) {
      // Imported component — skip detailed import/export validation for now.
      // TODO: Use component type info for full validation.
    } else {
      // Get the imports of the component to instantiate.
      const auto &Comp = *CompEntry.Comp;
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
    } // end of else (inline component) block

    // Register the exports of the instantiated component as instance exports.
    uint32_t NewInstIdx = CompCtx.addInstance();
    if (CompEntry.Comp != nullptr) {
      const auto &Comp = *CompEntry.Comp;
      for (const auto &Sec : Comp.getSections()) {
        if (std::holds_alternative<AST::Component::ExportSection>(Sec)) {
          const auto &ExpSec = std::get<AST::Component::ExportSection>(Sec);
          for (const auto &Exp : ExpSec.getContent()) {
            if (Exp.getDesc().has_value()) {
              CompCtx.addInstanceExport(NewInstIdx, Exp.getName(),
                                        *Exp.getDesc());
            } else {
              // No explicit ExternDesc; synthesize one from the sort.
              AST::Component::ExternDesc SynDesc;
              const auto &ExpSort = Exp.getSortIndex().getSort();
              if (!ExpSort.isCore()) {
                switch (ExpSort.getSortType()) {
                case AST::Component::Sort::SortType::Func:
                  SynDesc.setFuncTypeIdx(0);
                  break;
                case AST::Component::Sort::SortType::Component:
                  SynDesc.setComponentTypeIdx(0);
                  break;
                case AST::Component::Sort::SortType::Instance:
                  SynDesc.setInstanceTypeIdx(0);
                  break;
                case AST::Component::Sort::SortType::Type:
                  SynDesc.setTypeBound();
                  break;
                default:
                  continue;
                }
              } else {
                SynDesc.setCoreTypeIdx(0);
              }
              CompCtx.addSyntheticInstanceExport(NewInstIdx, Exp.getName(),
                                                 std::move(SynDesc));
            }
          }
        }
      }
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case.
    CompCtx.addInstance();

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
  return {};
}

Expect<void> Validator::validate(const AST::Component::Alias &Alias) noexcept {
  const auto &Sort = Alias.getSort();
  switch (Alias.getTargetType()) {
  case AST::Component::Alias::TargetType::Export: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    // Check the component instance index bound.
    if (Idx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance)) {
      spdlog::error(ErrCode::Value::AliasUnknownExportModule);
      spdlog::error(
          "    Alias export: Instance index {} exceeds available instances {}"sv,
          Idx,
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance));
      return Unexpect(ErrCode::Value::AliasUnknownExportModule);
    }

    const auto &CompExports = CompCtx.getInstance(Idx).Exports;

    // Find the export by name.
    auto It = CompExports.find(Name);
    if (It == CompExports.cend()) {
      spdlog::error(ErrCode::Value::AliasUnknownExportName);
      spdlog::error(
          "    Alias export: No matching export '{}' found in component instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::AliasUnknownExportName);
    }

    const auto *ExternDesc = It->second;
    if (Sort.isCore()) {
      // Core sort alias from component instance export.
      // Only core module (CoreType desc) is valid here.
      if (Sort.getCoreSortType() !=
              AST::Component::Sort::CoreSortType::Module ||
          ExternDesc->getDescType() !=
              AST::Component::ExternDesc::DescType::CoreType) {
        spdlog::error(ErrCode::Value::AliasUnknownExportModule);
        spdlog::error(
            "    Alias export: Sort mismatch for export '{}' (core sort from component instance)"sv,
            Name);
        return Unexpect(ErrCode::Value::AliasUnknownExportModule);
      }
      // Valid: aliasing a core module from a component instance export.
      // Record it as an imported core module so CoreInstance can use it.
      // addImportedCoreModule pushes to the CoreModules vector.
      CompCtx.addCoreModule(ExternDesc->getTypeIndex());
    } else {
      // Non-core sort alias from component instance export.
      AST::Component::Sort::SortType ST;
      switch (ExternDesc->getDescType()) {
      case AST::Component::ExternDesc::DescType::FuncType:
        ST = AST::Component::Sort::SortType::Func;
        break;
      case AST::Component::ExternDesc::DescType::ValueBound:
      case AST::Component::ExternDesc::DescType::TypeBound:
        ST = AST::Component::Sort::SortType::Type;
        break;
      case AST::Component::ExternDesc::DescType::ComponentType:
        ST = AST::Component::Sort::SortType::Component;
        break;
      case AST::Component::ExternDesc::DescType::InstanceType:
        ST = AST::Component::Sort::SortType::Instance;
        break;
      case AST::Component::ExternDesc::DescType::CoreType:
      default:
        spdlog::error(ErrCode::Value::AliasUnknownExportModule);
        spdlog::error("    Alias export: Sort mismatch for export '{}'"sv,
                      Name);
        return Unexpect(ErrCode::Value::AliasUnknownExportModule);
      }
      if (ST != Sort.getSortType()) {
        spdlog::error(ErrCode::Value::AliasUnknownExportName);
        spdlog::error(
            "    Alias export: Type mapping mismatch for export '{}'"sv, Name);
        return Unexpect(ErrCode::Value::AliasUnknownExportName);
      }
      // Increment the sort index for the aliased non-core sort.
      CompCtx.incSortIndexSize(Sort.getSortType());
    }
    return {};
  }
  case AST::Component::Alias::TargetType::CoreExport: {
    const auto Idx = Alias.getExport().first;
    const auto &Name = Alias.getExport().second;

    if (!Sort.isCore()) {
      spdlog::error(ErrCode::Value::AliasUnknownExportModule);
      spdlog::error("    Alias core:export: Mapping a export '{}' to sort"sv,
                    Name);
      return Unexpect(ErrCode::Value::AliasUnknownExportModule);
    }

    if (Idx >= CompCtx.getCoreSortIndexSize(
                   AST::Component::Sort::CoreSortType::Instance)) {
      spdlog::error(ErrCode::Value::AliasUnknownExportModule);
      spdlog::error(
          "    Alias core:export: Core instance index {} exceeds available core instances {}"sv,
          Idx,
          CompCtx.getCoreSortIndexSize(
              AST::Component::Sort::CoreSortType::Instance));
      return Unexpect(ErrCode::Value::AliasUnknownExportModule);
    }

    const auto &CoreExports = CompCtx.getCoreInstance(Idx).Exports;
    auto It = CoreExports.find(Name);
    if (It == CoreExports.end()) {
      spdlog::error(ErrCode::Value::AliasUnknownExportName);
      spdlog::error(
          "    Alias core:export: No matching export '{}' found in core instance index {}"sv,
          Name, Idx);
      return Unexpect(ErrCode::Value::AliasUnknownExportName);
    }

    const auto ExternTy = It->second.ExtType;
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
      spdlog::error(ErrCode::Value::AliasUnknownExportName);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrCode::Value::AliasUnknownExportName);
    }
    if (ST != Sort.getCoreSortType()) {
      spdlog::error(ErrCode::Value::AliasUnknownExportName);
      spdlog::error(
          "    Alias core:export: Type mapping mismatch for export '{}'"sv,
          Name);
      return Unexpect(ErrCode::Value::AliasUnknownExportName);
    }
    // Increment the core sort index for the aliased core sort.
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
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
      spdlog::error(ErrCode::Value::InvalidOuterAliasCount);
      spdlog::error(
          "    Alias outer: Component out-link count {} is exceeding the enclosing component count {}"sv,
          Ct, OutLinkCompCnt);
      return Unexpect(ErrCode::Value::InvalidOuterAliasCount);
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
        spdlog::error(ErrCode::Value::IndexOutOfBounds);
        spdlog::error(
            "    Alias outer: core:sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::IndexOutOfBounds);
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
        spdlog::error(ErrCode::Value::IndexOutOfBounds);
        spdlog::error(
            "    Alias outer: sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(ErrCode::Value::IndexOutOfBounds);
      }
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
        if (TargetCtx->Types.size() > Idx &&
            TargetCtx->Types[Idx].ResType != nullptr) {
          spdlog::error(ErrCode::Value::InvalidTypeReference);
          spdlog::error(
              "    Alias outer: Cannot outer-alias a resource type at index {}. Resource types are generative."sv,
              Idx);
          return Unexpect(ErrCode::Value::InvalidTypeReference);
        }
      }
    }
    // Increment the sort index for the outer-aliased sort.
    if (Sort.isCore()) {
      CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
    } else {
      CompCtx.incSortIndexSize(Sort.getSortType());
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
    CompCtx.addCoreType();
  } else if (DType.isModuleType()) {
    // Extract exports, imports, and function types from module type
    // declarations.
    using CEI = ComponentContext::CoreExternInfo;
    using CII = ComponentContext::CoreImportInfo;
    using FSig = ComponentContext::FuncSig;

    std::vector<std::pair<std::string, CEI>> Exports;
    std::vector<CII> Imports;
    // Local function type table built from type declarations in the module
    // type.
    std::vector<FSig> LocalFuncTypes;

    auto DescToEntry = [&](const AST::Component::CoreImportDesc &Desc) -> CEI {
      CEI Entry;
      if (Desc.isFunc()) {
        Entry.ExtType = ExternalType::Function;
        uint32_t TIdx = Desc.getTypeIndex();
        if (TIdx < LocalFuncTypes.size()) {
          Entry.Sig = LocalFuncTypes[TIdx];
        }
      } else if (Desc.isTable()) {
        Entry.ExtType = ExternalType::Table;
      } else if (Desc.isMemory()) {
        Entry.ExtType = ExternalType::Memory;
      } else if (Desc.isGlobal()) {
        Entry.ExtType = ExternalType::Global;
      } else if (Desc.isTag()) {
        Entry.ExtType = ExternalType::Tag;
      }
      return Entry;
    };

    for (const auto &Decl : DType.getModuleType()) {
      EXPECTED_TRY(validate(Decl).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return E;
      }));
      if (Decl.isType()) {
        // Extract function types from type declarations.
        const auto *CDT = Decl.getType();
        if (CDT && CDT->isRecType()) {
          for (const auto &ST : CDT->getSubTypes()) {
            if (ST.getCompositeType().isFunc()) {
              const auto &FT = ST.getCompositeType().getFuncType();
              FSig Sig;
              for (auto P : FT.getParamTypes()) {
                Sig.Params.push_back(P);
              }
              for (auto R : FT.getReturnTypes()) {
                Sig.Returns.push_back(R);
              }
              LocalFuncTypes.push_back(std::move(Sig));
            } else {
              // Non-function types get a placeholder in the type table.
              LocalFuncTypes.emplace_back();
            }
          }
        }
      } else if (Decl.isExport()) {
        const auto &Exp = Decl.getExport();
        Exports.emplace_back(std::string(Exp.getName()),
                             DescToEntry(Exp.getImportDesc()));
      } else if (Decl.isImport()) {
        const auto &Imp = Decl.getImport();
        CII Entry;
        Entry.ModuleName = std::string(Imp.getModuleName());
        Entry.FieldName = std::string(Imp.getName());
        Entry.Type = DescToEntry(Imp.getImportDesc());
        Imports.push_back(std::move(Entry));
      }
    }
    CompCtx.addCoreType(std::move(Imports), std::move(Exports));
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
        EXPECTED_TRY(validate(Decl.getImportDecl()).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
      } else if (Decl.isInstanceDecl()) {
        EXPECTED_TRY(validate(Decl.getInstanceDecl()).map_error([](auto E) {
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
    CompCtx.addType(DType.getInstanceType());
  } else if (DType.isResourceType()) {
    CompCtx.addType(DType.getResourceType());
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
  case AST::Component::ExternDesc::DescType::CoreType: {
    // CoreType (0x00 0x11) represents a core module import.
    // Record the imported module with its type index for later use.
    CompCtx.addCoreModule(Desc.getTypeIndex());
    break;
  }
  case AST::Component::ExternDesc::DescType::FuncType:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Func);
    break;
  case AST::Component::ExternDesc::DescType::ValueBound:
  case AST::Component::ExternDesc::DescType::TypeBound:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    break;
  case AST::Component::ExternDesc::DescType::ComponentType:
    CompCtx.addComponent();
    break;
  case AST::Component::ExternDesc::DescType::InstanceType: {
    uint32_t InstIdx = CompCtx.addInstance();
    uint32_t TypeIdx = Desc.getTypeIndex();
    if (TypeIdx <
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      const auto *IT = CompCtx.getType(TypeIdx).InstType;
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
            const auto &Exp = InstDecl.getExportDecl();
            CompCtx.addInstanceExport(InstIdx, Exp.getName(),
                                      Exp.getExternDesc());
          } else {
            assumingUnreachable();
          }
        }
      }
    } else {
      // TODO: check getCoreSortIndexSize(), because it may miss type, not out
      // of bound
      spdlog::error(ErrCode::Value::InvalidIndex);
      spdlog::error("    ExternDesc: Instance index {} out of bound"sv,
                    TypeIdx);
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
