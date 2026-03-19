// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "validator/component_name.h"
#include "validator/validator.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

static constexpr uint64_t MAX_TYPE_SIZE = 1000000;

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
  // Capture the component's effective type size before exiting.
  uint64_t CompSize = CompCtx.getCurrentContext().ComponentTypeSize;
  CompCtx.exitComponent();
  // Store the component's type size in the parent context.
  if (CompCtx.getDepth() > 0) {
    CompCtx.getCurrentContext().ComponentTypeSizes.push_back(CompSize);
  }
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
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error(
          "    CoreInstance: Module index {} exceeds available core modules {}"sv,
          ModIdx, ModCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }

    // Collect and validate instantiation arguments.
    // Check for duplicate argument names (wasmtime: insert_arg).
    auto Args = Inst.getInstantiateArgs();
    std::unordered_map<std::string, uint32_t> ArgMap;
    for (const auto &Arg : Args) {
      if (!ArgMap.emplace(std::string(Arg.getName()), Arg.getIndex()).second) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    CoreInstance: Duplicate instantiation argument named '{}'"sv,
            Arg.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
    }

    uint32_t InstanceIdx = CompCtx.addCoreInstance();

    const auto &ModEntry = CompCtx.getCoreModule(ModIdx);
    if (ModEntry.InlineMod == nullptr) {
      // Imported core module: use type info to check args and register exports.
      uint32_t TypeIdxVal = ModEntry.TypeIdx;
      if (TypeIdxVal != UINT32_MAX &&
          TypeIdxVal < CompCtx.getCoreSortIndexSize(
                           AST::Component::Sort::CoreSortType::Type)) {
        const auto &CoreTypeE = CompCtx.getCoreType(TypeIdxVal);
        // Validate each import: check arg presence, field presence, and type.
        for (const auto &Imp : CoreTypeE.ModuleImports) {
          auto ArgIt = ArgMap.find(Imp.ModuleName);
          if (ArgIt == ArgMap.end()) {
            spdlog::error(ErrCode::Value::MissingArgument);
            spdlog::error(
                "    CoreInstance: Missing argument for import module '{}'"sv,
                Imp.ModuleName);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
            return Unexpect(ErrCode::Value::MissingArgument);
          }
          const auto &ArgExports = CompCtx.getCoreInstance(ArgIt->second);
          auto ExpIt = ArgExports.find(Imp.FieldName);
          if (ExpIt == ArgExports.end()) {
            spdlog::error(ErrCode::Value::ArgTypeMismatch);
            spdlog::error(
                "    CoreInstance: Argument '{}' does not export '{}'"sv,
                Imp.ModuleName, Imp.FieldName);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
            return Unexpect(ErrCode::Value::ArgTypeMismatch);
          }
          // Check ExternalType match.
          if (ExpIt->second.ExtType != Imp.Type.ExtType) {
            spdlog::error(ErrCode::Value::ArgTypeMismatch);
            spdlog::error(
                "    CoreInstance: Type mismatch for export '{}' of argument '{}'"sv,
                Imp.FieldName, Imp.ModuleName);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
            return Unexpect(ErrCode::Value::ArgTypeMismatch);
          }
          // For functions, compare signatures.
          if (Imp.Type.ExtType == ExternalType::Function &&
              Imp.Type.Sig.has_value() && ExpIt->second.Sig.has_value() &&
              *Imp.Type.Sig != *ExpIt->second.Sig) {
            spdlog::error(ErrCode::Value::ArgTypeMismatch);
            spdlog::error(
                "    CoreInstance: Type mismatch for export '{}' of argument '{}'"sv,
                Imp.FieldName, Imp.ModuleName);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
            return Unexpect(ErrCode::Value::ArgTypeMismatch);
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
      // Validate each import: check arg presence and field presence.
      for (const auto &Import : ImportDescs) {
        auto ArgIt = ArgMap.find(std::string(Import.getModuleName()));
        if (ArgIt == ArgMap.end()) {
          spdlog::error(ErrCode::Value::MissingArgument);
          spdlog::error(
              "    CoreInstance: Missing argument for import module '{}'"sv,
              Import.getModuleName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::MissingArgument);
        }
        // Check that the supplied instance exports the required field.
        const auto &ArgExports = CompCtx.getCoreInstance(ArgIt->second);
        if (ArgExports.find(std::string(Import.getExternalName())) ==
            ArgExports.end()) {
          spdlog::error(ErrCode::Value::ArgTypeMismatch);
          spdlog::error(
              "    CoreInstance: Argument '{}' does not export '{}'"sv,
              Import.getModuleName(), Import.getExternalName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
          return Unexpect(ErrCode::Value::ArgTypeMismatch);
        }
      }
      const auto &ExportDescs = Mod.getExportSection().getContent();
      for (const auto &ExportDesc : ExportDescs) {
        CompCtx.addCoreInstanceExport(InstanceIdx, ExportDesc.getExternalName(),
                                      ExportDesc.getExternalType());
      }
    }
  } else if (Inst.isInlineExport()) {
    // Inline export case (wasmtime: instantiate_core_exports).
    uint32_t InlineInstIdx = CompCtx.addCoreInstance();
    std::unordered_set<std::string> SeenNames;

    // Check the core:sort index bound of the inline exports and register them.
    for (const auto &Export : Inst.getInlineExports()) {
      // Check for duplicate export names.
      if (!SeenNames.emplace(std::string(Export.getName())).second) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    CoreInstance: Duplicate export name '{}'"sv,
            Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      assuming(Sort.isCore());
      if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
        spdlog::error(ErrCode::Value::IndexOutOfBounds);
        spdlog::error(
            "    CoreInstance: Inline export '{}' refers to invalid index {}"sv,
            Export.getName(), Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInstance));
        return Unexpect(ErrCode::Value::IndexOutOfBounds);
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
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error(
          "    Instance: Component index {} exceeds available components {}"sv,
          CompIdx, CompCount);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }

    // Collect and validate instantiation arguments.
    // Check for duplicate argument names (wasmtime: instantiate_component).
    auto Args = Inst.getInstantiateArgs();
    std::unordered_set<std::string> ArgNames;
    for (const auto &Arg : Args) {
      if (!ArgNames.emplace(std::string(Arg.getName())).second) {
        spdlog::error(ErrCode::Value::InvalidIndex);
        spdlog::error(
            "    Instance: Duplicate instantiation argument named '{}'"sv,
            Arg.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::InvalidIndex);
      }
    }

    // For imported components (no AST available), skip detailed import checks.
    const auto *CompPtr = CompCtx.getComponent(CompIdx);
    if (CompPtr == nullptr) {
      // Imported component — skip detailed import/export validation for now.
      // TODO: Use component type info for full validation.
    } else {
      // Get the imports of the component to instantiate.
      const auto &Comp = *CompPtr;
      // Prepare the import descriptor map.
      std::unordered_map<std::string, const AST::Component::ExternDesc *>
          ImportMap;
      for (const auto &Sec : Comp.getSections()) {
        if (std::holds_alternative<AST::Component::ImportSection>(Sec)) {
          const auto &ImportSec = std::get<AST::Component::ImportSection>(Sec);
          for (const auto &Import : ImportSec.getContent()) {
            ImportMap[std::string(Import.getName())] = &Import.getDesc();
          }
        }
      }
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
          // A core sort argument can only satisfy a CoreType (core module)
          // import. All other import desc types require component-level args.
          bool IsMatched = ImportDesc.getDescType() ==
                               AST::Component::ExternDesc::DescType::CoreType &&
                           Sort.getCoreSortType() ==
                               AST::Component::Sort::CoreSortType::Module;
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
            spdlog::error(ErrCode::Value::IndexOutOfBounds);
            spdlog::error(
                "    Instance: Argument '{}' refers to invalid index {}"sv,
                ImportName, Idx);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::IndexOutOfBounds);
          }
        } else {
          // Check the import descriptor type matches the sort type.
          bool IsMatched = false;
          switch (ImportDesc.getDescType()) {
          case AST::Component::ExternDesc::DescType::CoreType:
            // CoreType (core module) import requires a core sort arg, not a
            // component sort. This branch handles non-core args, so never
            // match.
            IsMatched = false;
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
            spdlog::error(ErrCode::Value::IndexOutOfBounds);
            spdlog::error(
                "    Instance: Argument '{}' refers to invalid index {}"sv,
                ImportName, Idx);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
            return Unexpect(ErrCode::Value::IndexOutOfBounds);
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
    // Set the instance's type size to the instantiated component's type size.
    if (CompIdx < CompCtx.getCurrentContext().ComponentTypeSizes.size()) {
      CompCtx.getCurrentContext().Instances[NewInstIdx].TypeSize =
          CompCtx.getCurrentContext().ComponentTypeSizes[CompIdx];
    }
    if (CompPtr != nullptr) {
      const auto &Comp = *CompPtr;
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

    // Check for duplicate export names and sort index bounds.
    std::unordered_set<std::string> SeenNames;
    for (const auto &Export : Inst.getInlineExports()) {
      // Check for duplicate export names.
      if (!SeenNames.emplace(std::string(Export.getName())).second) {
        spdlog::error(ErrCode::Value::ComponentDuplicateName);
        spdlog::error(
            "    Instance: Duplicate inline export name '{}'"sv,
            Export.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
        return Unexpect(ErrCode::Value::ComponentDuplicateName);
      }
      const auto &Sort = Export.getSortIdx().getSort();
      uint32_t Idx = Export.getSortIdx().getIdx();
      if (Sort.isCore()) {
        if (Idx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
          spdlog::error(ErrCode::Value::IndexOutOfBounds);
          spdlog::error(
              "    Instance: Inline export '{}' refers to invalid index {}"sv,
              Export.getName(), Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::IndexOutOfBounds);
        }
      } else {
        if (Idx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
          spdlog::error(ErrCode::Value::IndexOutOfBounds);
          spdlog::error(
              "    Instance: Inline export '{}' refers to invalid index {}"sv,
              Export.getName(), Idx);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Instance));
          return Unexpect(ErrCode::Value::IndexOutOfBounds);
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
      // Map the export's ExternDesc to the sort it provides, following
      // wasmtime's ComponentEntityType → ComponentExternalKind matching.
      AST::Component::Sort::SortType ST;
      switch (ExternDesc->getDescType()) {
      case AST::Component::ExternDesc::DescType::FuncType:
        ST = AST::Component::Sort::SortType::Func;
        break;
      case AST::Component::ExternDesc::DescType::ValueBound:
        ST = AST::Component::Sort::SortType::Value;
        break;
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
            "    Alias export: Export '{}' for instance {} is not a {}"sv, Name,
            Idx, static_cast<int>(Sort.getSortType()));
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

    const auto &CoreExports = CompCtx.getCoreInstance(Idx);
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
        // Use "type index out of bounds" when inside a type scope
        // (Component == nullptr) and aliasing a type sort.
        auto EC =
            (Sort.getSortType() == AST::Component::Sort::SortType::Type &&
             CompCtx.getCurrentContext().Component == nullptr)
                ? ErrCode::Value::CoreTypeIndexOOB
                : ErrCode::Value::IndexOutOfBounds;
        spdlog::error(EC);
        spdlog::error(
            "    Alias outer: sort index {} invalid in component context"sv,
            Idx);
        return Unexpect(EC);
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
      if (Sort.getSortType() == AST::Component::Sort::SortType::Type &&
          TargetCtx->Types.size() > Idx) {
        // Copy the TypeEntry (including TypeSize) from the parent scope.
        auto &V = CompCtx.getCurrentContext().Types;
        auto TE = TargetCtx->Types[Idx];
        V.push_back(TE);
      } else if (Sort.getSortType() ==
                     AST::Component::Sort::SortType::Component &&
                 TargetCtx->ComponentTypeSizes.size() > Idx) {
        // Copy the component's type size from the parent scope.
        CompCtx.addComponent();
        CompCtx.getCurrentContext().ComponentTypeSizes.push_back(
            TargetCtx->ComponentTypeSizes[Idx]);
      } else {
        CompCtx.incSortIndexSize(Sort.getSortType());
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
    // Validate ref type indices within the rec type.
    uint32_t CoreTypeCount = CompCtx.getCoreSortIndexSize(
        AST::Component::Sort::CoreSortType::Type);
    for (const auto &ST : DType.getSubTypes()) {
      if (ST.getCompositeType().isFunc()) {
        const auto &FT = ST.getCompositeType().getFuncType();
        // Check all parameter and return types for ref type indices.
        auto CheckRefTypes = [&](Span<const ValType> Types) -> Expect<void> {
          for (const auto &VT : Types) {
            if (VT.isRefType()) {
              uint32_t TIdx = VT.getTypeIndex();
              if (TIdx >= CoreTypeCount) {
                spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
                spdlog::error(
                    "    CoreDefType: type index {} out of bounds (have {})"sv,
                    TIdx, CoreTypeCount);
                return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
              }
              // Check that the referenced core type is not a module type.
              if (CompCtx.getCoreType(TIdx).IsModuleType) {
                spdlog::error(ErrCode::Value::UnknownModuleType);
                spdlog::error(
                    "    CoreDefType: type index {} refers to a module type"sv,
                    TIdx);
                return Unexpect(ErrCode::Value::UnknownModuleType);
              }
            }
          }
          return {};
        };
        EXPECTED_TRY(CheckRefTypes(FT.getParamTypes()));
        EXPECTED_TRY(CheckRefTypes(FT.getReturnTypes()));
      }
    }
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

    auto DescToEntry =
        [&](const AST::Component::CoreImportDesc &Desc) -> Expect<CEI> {
      CEI Entry;
      if (Desc.isFunc()) {
        Entry.ExtType = ExternalType::Function;
        uint32_t TIdx = Desc.getTypeIndex();
        if (TIdx >= LocalFuncTypes.size()) {
          spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
          spdlog::error(
              "    CoreDefType: type index {} out of bounds (have {})"sv,
              TIdx, LocalFuncTypes.size());
          return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
        }
        Entry.Sig = LocalFuncTypes[TIdx];
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

    std::unordered_set<std::string> SeenExportNames;
    std::unordered_set<std::string> SeenImportPairs;

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
        // Check for duplicate export names.
        std::string ExpName(Exp.getName());
        if (!SeenExportNames.emplace(ExpName).second) {
          spdlog::error(ErrCode::Value::DuplicateExportName);
          spdlog::error(
              "    CoreDefType: duplicate export name '{}'"sv, ExpName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
          return Unexpect(ErrCode::Value::DuplicateExportName);
        }
        auto ExpEntryRes = DescToEntry(Exp.getImportDesc());
        if (!ExpEntryRes) {
          return Unexpect(ExpEntryRes);
        }
        Exports.emplace_back(std::string(Exp.getName()),
                             std::move(*ExpEntryRes));
      } else if (Decl.isImport()) {
        const auto &Imp = Decl.getImport();
        // Check for duplicate import names (module+field pair).
        std::string ImpKey =
            std::string(Imp.getModuleName()) + "\0" + std::string(Imp.getName());
        if (!SeenImportPairs.emplace(ImpKey).second) {
          spdlog::error(ErrCode::Value::DuplicateImportName);
          spdlog::error("    CoreDefType: duplicate import name"sv);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
          return Unexpect(ErrCode::Value::DuplicateImportName);
        }
        // Check memory size limits.
        if (Imp.getImportDesc().isMemory()) {
          const auto &MT = Imp.getImportDesc().getMemoryType();
          const auto &Lim = MT.getLimit();
          // Core WASM memory limit is 65536 pages (4 GiB).
          if (Lim.getMin() > 65536 ||
              (Lim.hasMax() && Lim.getMax() > 65536)) {
            spdlog::error(ErrCode::Value::MemorySizeLimit);
            spdlog::error(
                "    CoreDefType: memory size must be at most 65536 pages"sv);
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
            return Unexpect(ErrCode::Value::MemorySizeLimit);
          }
        }
        auto ImpEntryRes = DescToEntry(Imp.getImportDesc());
        if (!ImpEntryRes) {
          return Unexpect(ImpEntryRes);
        }
        CII Entry;
        Entry.ModuleName = std::string(Imp.getModuleName());
        Entry.FieldName = std::string(Imp.getName());
        Entry.Type = std::move(*ImpEntryRes);
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
    const auto &DVT = DType.getDefValType();
    const uint32_t TypeCount =
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);

    // Helper to convert string to lowercase for case-insensitive comparison.
    auto toLower = [](std::string_view SV) -> std::string {
      std::string Result(SV);
      std::transform(Result.begin(), Result.end(), Result.begin(),
                     [](unsigned char C) {
                       return static_cast<char>(std::tolower(C));
                     });
      return Result;
    };

    // Helper to get the effective type size of a ComponentValType.
    auto GetValTypeSize = [&](const ComponentValType &VT) -> uint64_t {
      if (VT.isPrimValType()) {
        return 1;
      }
      uint32_t TIdx = VT.getTypeIndex();
      if (TIdx < TypeCount) {
        return CompCtx.getType(TIdx).TypeSize;
      }
      return 1;
    };

    // Helper to check a ComponentValType index is in bounds and refers to a
    // defined type.
    auto CheckValTypeIdx = [&](const ComponentValType &VT) -> Expect<void> {
      if (!VT.isPrimValType()) {
        uint32_t TIdx = VT.getTypeIndex();
        if (TIdx >= TypeCount) {
          spdlog::error(ErrCode::Value::IndexOutOfBounds);
          spdlog::error(
              "    DefValType: type index {} out of bounds (have {})"sv,
              TIdx, TypeCount);
          return Unexpect(ErrCode::Value::IndexOutOfBounds);
        }
        // Check that the referenced type is a defined value type.
        const auto &TE = CompCtx.getType(TIdx);
        if (TE.InstType != nullptr || TE.ResType != nullptr ||
            TE.IsFuncType || TE.IsComponentType) {
          spdlog::error(ErrCode::Value::NotADefinedType);
          spdlog::error(
              "    DefValType: type index {} is not a defined type"sv, TIdx);
          return Unexpect(ErrCode::Value::NotADefinedType);
        }
      }
      return {};
    };

    // Flags: check count limit and names.
    if (DVT.isFlags()) {
      const auto &F = DVT.getFlags();
      if (F.Labels.empty()) {
        spdlog::error(ErrCode::Value::EmptyFlags);
        spdlog::error("    DefValType: flags must have at least one entry"sv);
        return Unexpect(ErrCode::Value::EmptyFlags);
      }
      if (F.Labels.size() > 32) {
        spdlog::error(ErrCode::Value::TooManyFlags);
        spdlog::error("    DefValType: cannot have more than 32 flags"sv);
        return Unexpect(ErrCode::Value::TooManyFlags);
      }
      std::unordered_set<std::string> Seen;
      for (const auto &L : F.Labels) {
        if (L.empty()) {
          spdlog::error(ErrCode::Value::DefinedTypeEmptyName);
          spdlog::error("    DefValType: flag name cannot be empty"sv);
          return Unexpect(ErrCode::Value::DefinedTypeEmptyName);
        }
        if (!Seen.emplace(toLower(L)).second) {
          spdlog::error(ErrCode::Value::DuplicateFlagName);
          spdlog::error(
              "    DefValType: flag name conflicts with previous flag name"sv);
          return Unexpect(ErrCode::Value::DuplicateFlagName);
        }
      }
    }
    // Enum: check non-empty, duplicate labels, empty names.
    else if (DVT.isEnum()) {
      const auto &E = DVT.getEnum();
      if (E.Labels.empty()) {
        spdlog::error(ErrCode::Value::EmptyEnum);
        spdlog::error(
            "    DefValType: enum type must have at least one variant"sv);
        return Unexpect(ErrCode::Value::EmptyEnum);
      }
      std::unordered_set<std::string> Seen;
      for (const auto &L : E.Labels) {
        if (L.empty()) {
          spdlog::error(ErrCode::Value::DefinedTypeEmptyName);
          spdlog::error("    DefValType: enum tag name cannot be empty"sv);
          return Unexpect(ErrCode::Value::DefinedTypeEmptyName);
        }
        if (!Seen.emplace(toLower(L)).second) {
          spdlog::error(ErrCode::Value::DuplicateEnumTag);
          spdlog::error(
              "    DefValType: enum tag name conflicts with previous tag name"sv);
          return Unexpect(ErrCode::Value::DuplicateEnumTag);
        }
      }
    }
    // Record: check non-empty, duplicate field names, empty names, type refs.
    else if (DVT.isRecord()) {
      const auto &R = DVT.getRecord();
      if (R.LabelTypes.empty()) {
        spdlog::error(ErrCode::Value::EmptyRecord);
        spdlog::error(
            "    DefValType: record type must have at least one field"sv);
        return Unexpect(ErrCode::Value::EmptyRecord);
      }
      std::unordered_set<std::string> Seen;
      for (const auto &LT : R.LabelTypes) {
        if (LT.getLabel().empty()) {
          spdlog::error(ErrCode::Value::DefinedTypeEmptyName);
          spdlog::error("    DefValType: record field name cannot be empty"sv);
          return Unexpect(ErrCode::Value::DefinedTypeEmptyName);
        }
        if (!Seen.emplace(toLower(LT.getLabel())).second) {
          spdlog::error(ErrCode::Value::DuplicateRecordField);
          spdlog::error(
              "    DefValType: record field name conflicts with previous field name"sv);
          return Unexpect(ErrCode::Value::DuplicateRecordField);
        }
        EXPECTED_TRY(CheckValTypeIdx(LT.getValType()));
      }
    }
    // Variant: check non-empty, duplicate case names, empty names, type refs.
    else if (DVT.isVariant()) {
      const auto &V = DVT.getVariant();
      if (V.Cases.empty()) {
        spdlog::error(ErrCode::Value::EmptyVariant);
        spdlog::error(
            "    DefValType: variant type must have at least one case"sv);
        return Unexpect(ErrCode::Value::EmptyVariant);
      }
      std::unordered_set<std::string> Seen;
      for (const auto &C : V.Cases) {
        if (C.first.empty()) {
          spdlog::error(ErrCode::Value::DefinedTypeEmptyName);
          spdlog::error("    DefValType: variant case name cannot be empty"sv);
          return Unexpect(ErrCode::Value::DefinedTypeEmptyName);
        }
        if (!Seen.emplace(toLower(C.first)).second) {
          spdlog::error(ErrCode::Value::DuplicateVariantCase);
          spdlog::error(
              "    DefValType: variant case name conflicts with previous case name"sv);
          return Unexpect(ErrCode::Value::DuplicateVariantCase);
        }
        if (C.second.has_value()) {
          EXPECTED_TRY(CheckValTypeIdx(*C.second));
        }
      }
    }
    // Tuple: check non-empty, type refs.
    else if (DVT.isTuple()) {
      const auto &T = DVT.getTuple();
      if (T.Types.empty()) {
        spdlog::error(ErrCode::Value::EmptyTuple);
        spdlog::error(
            "    DefValType: tuple type must have at least one type"sv);
        return Unexpect(ErrCode::Value::EmptyTuple);
      }
      for (const auto &VT : T.Types) {
        EXPECTED_TRY(CheckValTypeIdx(VT));
      }
    }
    // List: check type ref.
    else if (DVT.isList()) {
      EXPECTED_TRY(CheckValTypeIdx(DVT.getList().ValTy));
    }
    // Option: check type ref.
    else if (DVT.isOption()) {
      EXPECTED_TRY(CheckValTypeIdx(DVT.getOption().ValTy));
    }
    // Result: check type refs.
    else if (DVT.isResult()) {
      const auto &R = DVT.getResult();
      if (R.ValTy.has_value()) {
        EXPECTED_TRY(CheckValTypeIdx(*R.ValTy));
      }
      if (R.ErrTy.has_value()) {
        EXPECTED_TRY(CheckValTypeIdx(*R.ErrTy));
      }
    }
    // Own/Borrow: check type ref.
    else if (DVT.isOwn()) {
      ComponentValType VT(DVT.getOwn().Idx);
      EXPECTED_TRY(CheckValTypeIdx(VT));
    }
    else if (DVT.isBorrow()) {
      ComponentValType VT(DVT.getBorrow().Idx);
      EXPECTED_TRY(CheckValTypeIdx(VT));
    }
    // Stream: check type ref.
    else if (DVT.isStream()) {
      if (DVT.getStream().ValTy.has_value()) {
        EXPECTED_TRY(CheckValTypeIdx(*DVT.getStream().ValTy));
      }
    }
    // Future: check type ref.
    else if (DVT.isFuture()) {
      if (DVT.getFuture().ValTy.has_value()) {
        EXPECTED_TRY(CheckValTypeIdx(*DVT.getFuture().ValTy));
      }
    }

    // Compute effective type size.
    uint64_t TSize = 1;
    if (DVT.isRecord()) {
      for (const auto &F : DVT.getRecord().LabelTypes) {
        TSize += GetValTypeSize(F.getValType());
      }
    } else if (DVT.isTuple()) {
      for (const auto &T : DVT.getTuple().Types) {
        TSize += GetValTypeSize(T);
      }
    } else if (DVT.isVariant()) {
      for (const auto &C : DVT.getVariant().Cases) {
        if (C.second.has_value()) {
          TSize += GetValTypeSize(*C.second);
        } else {
          TSize += 1;
        }
      }
    } else if (DVT.isResult()) {
      if (DVT.getResult().ValTy.has_value()) {
        TSize += GetValTypeSize(*DVT.getResult().ValTy);
      }
      if (DVT.getResult().ErrTy.has_value()) {
        TSize += GetValTypeSize(*DVT.getResult().ErrTy);
      }
    } else if (DVT.isOption()) {
      TSize += GetValTypeSize(DVT.getOption().ValTy);
    } else if (DVT.isList()) {
      TSize += GetValTypeSize(DVT.getList().ValTy);
    }
    if (TSize > MAX_TYPE_SIZE) {
      spdlog::error(ErrCode::Value::TypeSizeExceeded);
      spdlog::error("    DefValType: effective type size exceeds the limit"sv);
      return Unexpect(ErrCode::Value::TypeSizeExceeded);
    }
    CompCtx.addDefValType(TSize);
  } else if (DType.isFuncType()) {
    const auto &FT = DType.getFuncType();
    const uint32_t FuncTypeCount =
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type);

    // Helper to check a ComponentValType index within FuncType params/results.
    auto CheckFuncValTypeIdx =
        [&](const ComponentValType &VT) -> Expect<void> {
      if (!VT.isPrimValType()) {
        uint32_t TIdx = VT.getTypeIndex();
        if (TIdx >= FuncTypeCount) {
          spdlog::error(ErrCode::Value::IndexOutOfBounds);
          spdlog::error(
              "    FuncType: type index {} out of bounds (have {})"sv,
              TIdx, FuncTypeCount);
          return Unexpect(ErrCode::Value::IndexOutOfBounds);
        }
        const auto &TE = CompCtx.getType(TIdx);
        if (TE.InstType != nullptr || TE.ResType != nullptr ||
            TE.IsFuncType || TE.IsComponentType) {
          spdlog::error(ErrCode::Value::NotADefinedType);
          spdlog::error(
              "    FuncType: type index {} is not a defined type"sv, TIdx);
          return Unexpect(ErrCode::Value::NotADefinedType);
        }
      }
      return {};
    };

    // Check function parameter names are non-empty and types are valid.
    for (const auto &P : FT.getParamList()) {
      if (P.getLabel().empty()) {
        spdlog::error(ErrCode::Value::EmptyParamName);
        spdlog::error(
            "    FuncType: function parameter name cannot be empty"sv);
        return Unexpect(ErrCode::Value::EmptyParamName);
      }
      EXPECTED_TRY(CheckFuncValTypeIdx(P.getValType()));
    }
    // Check result types are valid.
    for (const auto &R : FT.getResultList()) {
      EXPECTED_TRY(CheckFuncValTypeIdx(R.getValType()));
    }
    // TODO: Validation of functype rejects any transitive use of borrow in
    // a result type. Similarly, validation of components and component
    // types rejects any transitive use of borrow in an exported value type.

    // Compute effective type size for the func type.
    auto GetFuncValTypeSize = [&](const ComponentValType &VT) -> uint64_t {
      if (VT.isPrimValType()) {
        return 1;
      }
      uint32_t TIdx = VT.getTypeIndex();
      if (TIdx < FuncTypeCount) {
        return CompCtx.getType(TIdx).TypeSize;
      }
      return 1;
    };
    uint64_t FTSize = 1;
    for (const auto &P : FT.getParamList()) {
      FTSize += GetFuncValTypeSize(P.getValType());
    }
    for (const auto &R : FT.getResultList()) {
      FTSize += GetFuncValTypeSize(R.getValType());
    }
    if (FTSize > MAX_TYPE_SIZE) {
      spdlog::error(ErrCode::Value::TypeSizeExceeded);
      spdlog::error("    FuncType: effective type size exceeds the limit"sv);
      return Unexpect(ErrCode::Value::TypeSizeExceeded);
    }
    CompCtx.addFuncType(FTSize);
  } else if (DType.isComponentType()) {
    // Component types are validated in a fresh sub-scope with initially-empty
    // index spaces. Outer aliases can pull in types from parent scopes.
    CompCtx.enterTypeScope();
    auto SizeRes =
        validateComponentTypeDecls(DType.getComponentType().getDecl());
    if (!SizeRes) {
      CompCtx.exitTypeScope();
      return Unexpect(SizeRes);
    }
    CompCtx.exitTypeScope();
    // A componenttype definition adds exactly one entry to the type index
    // space, not one per sub-declaration.
    CompCtx.addComponentType(*SizeRes);
  } else if (DType.isInstanceType()) {
    // Instance types are also validated in a fresh sub-scope.
    CompCtx.enterTypeScope();
    auto SizeRes =
        validateInstanceTypeDecls(DType.getInstanceType().getDecl());
    if (!SizeRes) {
      CompCtx.exitTypeScope();
      return Unexpect(SizeRes);
    }
    CompCtx.exitTypeScope();
    CompCtx.addType(DType.getInstanceType(), *SizeRes);
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
  case AST::Component::Canonical::OpCode::Lift: {
    // Lift: getIndex() is core:funcidx, getTargetIndex() is typeidx.
    uint32_t CoreFuncIdx = Canon.getIndex();
    uint32_t TypeIdx = Canon.getTargetIndex();
    if (CoreFuncIdx >= CompCtx.getCoreSortIndexSize(
                           AST::Component::Sort::CoreSortType::Func)) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error(
          "    Canonical Lift: Core function index {} out of bounds"sv,
          CoreFuncIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
    if (TypeIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error("    Canonical Lift: Type index {} out of bounds"sv,
                    TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Func);
    return {};
  }
  case AST::Component::Canonical::OpCode::Lower: {
    // Lower: getIndex() is funcidx (component function index).
    uint32_t FuncIdx = Canon.getIndex();
    if (FuncIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Func)) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error(
          "    Canonical Lower: Function index {} out of bounds"sv, FuncIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
    CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
    return {};
  }
  case AST::Component::Canonical::OpCode::Resource__new:
  case AST::Component::Canonical::OpCode::Resource__drop:
  case AST::Component::Canonical::OpCode::Resource__rep: {
    // Resource operations: getIndex() is typeidx.
    uint32_t TypeIdx = Canon.getIndex();
    if (TypeIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error("    Canonical Resource: Type index {} out of bounds"sv,
                    TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Canonical));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
    CompCtx.incCoreSortIndexSize(AST::Component::Sort::CoreSortType::Func);
    return {};
  }
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
      spdlog::error(ErrCode::Value::NameConflict);
      spdlog::error("    Import: Duplicate import name"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Import));
      return Unexpect(ErrCode::Value::NameConflict);
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
  // TODO: If the export has an ExternDesc ascription, validate that the
  // sort index's actual type is a subtype of the ascribed type. For now
  // we skip ascription type-checking.

  // Check that the referenced sort index is in bounds.
  const auto &Sort = Ex.getSortIndex().getSort();
  uint32_t SortIdx = Ex.getSortIndex().getIdx();
  if (Sort.isCore()) {
    if (SortIdx >= CompCtx.getCoreSortIndexSize(Sort.getCoreSortType())) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error("    Export: Sort index {} out of bounds"sv, SortIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
  } else {
    if (SortIdx >= CompCtx.getSortIndexSize(Sort.getSortType())) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error("    Export: Sort index {} out of bounds"sv, SortIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
  }

  // Validate the export name.
  ComponentName CName(Ex.getName());
  if (CName.getKind() == ComponentNameKind::Invalid) {
    // Not even a valid extern name.
    spdlog::error(ErrCode::Value::InvalidExternName);
    spdlog::error("    Export: '{}' is not a valid extern name"sv,
                  Ex.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::InvalidExternName);
  }
  // Only labels, interface types, constructors, methods, and statics are
  // valid export names. Hash, URL, locked-dep, and unlocked-dep are valid
  // extern names but not valid export names.
  if (CName.getKind() == ComponentNameKind::Hash ||
      CName.getKind() == ComponentNameKind::URL ||
      CName.getKind() == ComponentNameKind::LockedDep ||
      CName.getKind() == ComponentNameKind::UnlockedDep) {
    spdlog::error(ErrCode::Value::InvalidExportName);
    spdlog::error("    Export: '{}' is not a valid export name"sv,
                  Ex.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::InvalidExportName);
  }

  // Check for duplicate export names.
  std::string ExportName(Ex.getName());
  if (!CompCtx.getCurrentContext().ExportedNames.emplace(ExportName).second) {
    spdlog::error(ErrCode::Value::NameConflict);
    spdlog::error("    Export: Duplicate export name '{}'"sv, Ex.getName());
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::NameConflict);
  }

  // Accumulate effective type size for this component's exports.
  uint64_t ExportTypeSize = 1;
  if (!Sort.isCore()) {
    // For component/instance exports, look up the size.
    if (Sort.getSortType() == AST::Component::Sort::SortType::Component) {
      auto &Sizes = CompCtx.getCurrentContext().ComponentTypeSizes;
      if (SortIdx < Sizes.size()) {
        ExportTypeSize = Sizes[SortIdx];
      }
    } else if (Sort.getSortType() == AST::Component::Sort::SortType::Instance) {
      // Instance sizes are derived from the component that was instantiated.
      if (SortIdx <
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Instance)) {
        ExportTypeSize = CompCtx.getInstance(SortIdx).TypeSize;
      }
    } else if (Sort.getSortType() == AST::Component::Sort::SortType::Type) {
      if (SortIdx <
          CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
        ExportTypeSize = CompCtx.getType(SortIdx).TypeSize;
      }
    } else if (Sort.getSortType() == AST::Component::Sort::SortType::Func) {
      // Func types have sizes tracked in the type index space.
      ExportTypeSize = 1;
    }
  }
  CompCtx.getCurrentContext().ComponentTypeSize += ExportTypeSize;
  if (CompCtx.getCurrentContext().ComponentTypeSize > MAX_TYPE_SIZE) {
    spdlog::error(ErrCode::Value::TypeSizeExceeded);
    spdlog::error(
        "    Export: effective type size exceeds the limit"sv);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Export));
    return Unexpect(ErrCode::Value::TypeSizeExceeded);
  }

  // An export adds exactly one entry to its sort's index space.
  if (Sort.isCore()) {
    CompCtx.incCoreSortIndexSize(Sort.getCoreSortType());
  } else {
    CompCtx.incSortIndexSize(Sort.getSortType());
  }
  return {};
}

Expect<void>
Validator::validate(const AST::Component::ExternDesc &Desc) noexcept {
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType: {
    // CoreType (0x00 0x11) represents a core module import.
    uint32_t CoreTypeIdx = Desc.getTypeIndex();
    // Check bounds.
    if (CoreTypeIdx >= CompCtx.getCoreSortIndexSize(
                           AST::Component::Sort::CoreSortType::Type)) {
      spdlog::error(ErrCode::Value::IndexOutOfBounds);
      spdlog::error("    ExternDesc: Core type index {} out of bounds"sv,
                    CoreTypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::IndexOutOfBounds);
    }
    // Check it's a module type.
    if (!CompCtx.getCoreType(CoreTypeIdx).IsModuleType) {
      spdlog::error(ErrCode::Value::UnknownModuleType);
      spdlog::error("    ExternDesc: Core type index {} is not a module type"sv,
                    CoreTypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::UnknownModuleType);
    }
    CompCtx.addCoreModule(CoreTypeIdx);
    break;
  }
  case AST::Component::ExternDesc::DescType::FuncType: {
    uint32_t TypeIdx = Desc.getTypeIndex();
    // Check bounds.
    if (TypeIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error("    ExternDesc: Type index {} out of bounds"sv, TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }
    // Check it's a function type.
    if (!CompCtx.getType(TypeIdx).IsFuncType) {
      spdlog::error(ErrCode::Value::UnknownFuncType);
      spdlog::error("    ExternDesc: Type index {} is not a function type"sv,
                    TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::UnknownFuncType);
    }
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Func);
    break;
  }
  case AST::Component::ExternDesc::DescType::ValueBound:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Value);
    break;
  case AST::Component::ExternDesc::DescType::TypeBound:
    CompCtx.incSortIndexSize(AST::Component::Sort::SortType::Type);
    break;
  case AST::Component::ExternDesc::DescType::ComponentType: {
    uint32_t TypeIdx = Desc.getTypeIndex();
    // Check bounds.
    if (TypeIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error("    ExternDesc: Type index {} out of bounds"sv, TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }
    // Check it's a component type.
    if (!CompCtx.getType(TypeIdx).IsComponentType) {
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error(
          "    ExternDesc: Type index {} is not a component type"sv, TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }
    CompCtx.addComponent();
    break;
  }
  case AST::Component::ExternDesc::DescType::InstanceType: {
    uint32_t TypeIdx = Desc.getTypeIndex();
    if (TypeIdx >=
        CompCtx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error("    ExternDesc: Type index {} out of bounds"sv, TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }
    const auto &TE = CompCtx.getType(TypeIdx);
    if (TE.InstType == nullptr) {
      spdlog::error(ErrCode::Value::UnknownInstanceType);
      spdlog::error(
          "    ExternDesc: Type index {} is not an instance type"sv, TypeIdx);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Desc_Extern));
      return Unexpect(ErrCode::Value::UnknownInstanceType);
    }
    uint32_t InstIdx = CompCtx.addInstance();
    const auto *IT = TE.InstType;
    // Register instance exports from the instance type declarations.
    // Only ExportDecl declarations contribute exports; CoreType, Type,
    // and Alias declarations define internal types within the instance
    // type scope and do not directly produce instance exports.
    for (const auto &InstDecl : IT->getDecl()) {
      if (InstDecl.isExportDecl()) {
        const auto &Exp = InstDecl.getExportDecl();
        CompCtx.addInstanceExport(InstIdx, Exp.getName(),
                                  Exp.getExternDesc());
      }
    }
    break;
  }
  default:
    assumingUnreachable();
  }

  return {};
}

Expect<void>
Validator::validate(const AST::Component::CoreModuleDecl &Decl) noexcept {
  if (Decl.isAlias()) {
    const auto &Alias = Decl.getAlias();
    // Core aliases within module types are always outer aliases.
    // CompJump indicates how many enclosing scopes to traverse.
    // For a module type within a component, CompJump=1 reaches the component's
    // core type index space.
    uint32_t Ct = Alias.getComponentJump();
    uint32_t Idx = Alias.getIndex();
    const auto &Sort = Alias.getSort();

    // Module types don't have their own context on the stack, so the "outer"
    // count of 1 reaches the current component context (which is already
    // on the stack). We treat module type as being 1 level deep relative to
    // the current component context.
    if (Ct < 1) {
      // outer 0 would mean the module type scope itself, which has no types.
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error(
          "    CoreModuleDecl: outer alias count 0 in module type"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }

    // Navigate to the target context. We start from the current context
    // and go up (Ct - 1) more levels (since the module type itself counts
    // as 1 level).
    const auto *TargetCtx = &CompCtx.getCurrentContext();
    for (uint32_t I = 1; I < Ct; ++I) {
      if (TargetCtx->Parent == nullptr) {
        spdlog::error(ErrCode::Value::InvalidOuterAliasCount);
        spdlog::error(
            "    CoreModuleDecl: outer alias count {} exceeds nesting"sv, Ct);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return Unexpect(ErrCode::Value::InvalidOuterAliasCount);
      }
      TargetCtx = TargetCtx->Parent;
    }

    // Check the index in the target context.
    if (Sort.isCore()) {
      if (Idx >= TargetCtx->getCoreSortIndexSize(Sort.getCoreSortType())) {
        spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
        spdlog::error(
            "    CoreModuleDecl: core sort index {} out of bounds"sv, Idx);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
        return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
      }
    } else {
      spdlog::error(ErrCode::Value::CoreTypeIndexOOB);
      spdlog::error(
          "    CoreModuleDecl: non-core sort in core module alias"sv);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreDefType));
      return Unexpect(ErrCode::Value::CoreTypeIndexOOB);
    }
  }
  return {};
}

Expect<void> Validator::validate(const AST::Component::ImportDecl &) noexcept {
  // TODO
  return {};
}

Expect<void>
Validator::validate(const AST::Component::InstanceDecl &Decl) noexcept {
  if (Decl.isExportDecl()) {
    const auto &Exp = Decl.getExportDecl();
    // Validate the export name.
    ComponentName CName(Exp.getName());
    if (CName.getKind() == ComponentNameKind::Invalid) {
      spdlog::error(ErrCode::Value::InvalidExternName);
      spdlog::error("    ExportDecl: '{}' is not a valid extern name"sv,
                    Exp.getName());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return Unexpect(ErrCode::Value::InvalidExternName);
    }
    if (CName.getKind() == ComponentNameKind::Hash ||
        CName.getKind() == ComponentNameKind::URL ||
        CName.getKind() == ComponentNameKind::LockedDep ||
        CName.getKind() == ComponentNameKind::UnlockedDep) {
      spdlog::error(ErrCode::Value::InvalidExportName);
      spdlog::error("    ExportDecl: '{}' is not a valid export name"sv,
                    Exp.getName());
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
      return Unexpect(ErrCode::Value::InvalidExportName);
    }
  }
  return {};
}

// Helper to compute the type size contribution of an ExternDesc.
// Returns the effective type size that this extern desc adds.
static uint64_t
getExternDescTypeSize(const AST::Component::ExternDesc &Desc,
                      const ComponentContext &Ctx) noexcept {
  switch (Desc.getDescType()) {
  case AST::Component::ExternDesc::DescType::CoreType:
    return 1;
  case AST::Component::ExternDesc::DescType::FuncType: {
    uint32_t TIdx = Desc.getTypeIndex();
    if (TIdx < Ctx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      return Ctx.getType(TIdx).TypeSize;
    }
    return 1;
  }
  case AST::Component::ExternDesc::DescType::ValueBound:
    return 1;
  case AST::Component::ExternDesc::DescType::TypeBound:
    return 1;
  case AST::Component::ExternDesc::DescType::ComponentType: {
    uint32_t TIdx = Desc.getTypeIndex();
    if (TIdx < Ctx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      return Ctx.getType(TIdx).TypeSize;
    }
    return 1;
  }
  case AST::Component::ExternDesc::DescType::InstanceType: {
    uint32_t TIdx = Desc.getTypeIndex();
    if (TIdx < Ctx.getSortIndexSize(AST::Component::Sort::SortType::Type)) {
      return Ctx.getType(TIdx).TypeSize;
    }
    return 1;
  }
  default:
    return 1;
  }
}

Expect<uint64_t> Validator::validateComponentTypeDecls(
    Span<const AST::Component::ComponentDecl> Decls) noexcept {
  // Helper for case-insensitive name comparison.
  auto toLower = [](std::string_view SV) -> std::string {
    std::string Result(SV);
    std::transform(Result.begin(), Result.end(), Result.begin(),
                   [](unsigned char C) {
                     return static_cast<char>(std::tolower(C));
                   });
    return Result;
  };

  std::unordered_set<std::string> ExportNames;
  std::unordered_set<std::string> ImportNames;
  uint64_t TypeSize = 1; // Start with 1 for the type itself.

  for (const auto &Decl : Decls) {
    if (Decl.isImportDecl()) {
      const auto &Imp = Decl.getImportDecl();
      // Check for duplicate import names (case-insensitive).
      std::string LowerName = toLower(Imp.getName());
      if (!ImportNames.emplace(LowerName).second) {
        spdlog::error(ErrCode::Value::ImportNameConflict);
        spdlog::error(
            "    ComponentType: import name '{}' conflicts with previous"sv,
            Imp.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return Unexpect(ErrCode::Value::ImportNameConflict);
      }
      // Validate the import's ExternDesc (checks type index bounds/kinds).
      EXPECTED_TRY(validate(Imp.getExternDesc()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
      // Accumulate type size from imports.
      TypeSize += getExternDescTypeSize(Imp.getExternDesc(), CompCtx);
      if (TypeSize > MAX_TYPE_SIZE) {
        spdlog::error(ErrCode::Value::TypeSizeExceeded);
        spdlog::error(
            "    ComponentType: effective type size exceeds the limit"sv);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return Unexpect(ErrCode::Value::TypeSizeExceeded);
      }
    } else if (Decl.isInstanceDecl()) {
      const auto &InstDecl = Decl.getInstanceDecl();
      if (InstDecl.isExportDecl()) {
        const auto &Exp = InstDecl.getExportDecl();
        // Validate the export's ExternDesc first (type index bounds, etc).
        EXPECTED_TRY(validate(Exp.getExternDesc()).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
        // Validate export name validity.
        EXPECTED_TRY(validate(InstDecl).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
        // Check for duplicate export names (case-insensitive).
        std::string LowerName = toLower(Exp.getName());
        if (!ExportNames.emplace(LowerName).second) {
          spdlog::error(ErrCode::Value::ExportNameConflict);
          spdlog::error(
              "    ComponentType: export name '{}' conflicts with previous"sv,
              Exp.getName());
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return Unexpect(ErrCode::Value::ExportNameConflict);
        }
        // Accumulate type size from exports.
        TypeSize += getExternDescTypeSize(Exp.getExternDesc(), CompCtx);
        if (TypeSize > MAX_TYPE_SIZE) {
          spdlog::error(ErrCode::Value::TypeSizeExceeded);
          spdlog::error(
              "    ComponentType: effective type size exceeds the limit"sv);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return Unexpect(ErrCode::Value::TypeSizeExceeded);
        }
      } else if (InstDecl.isType()) {
        // Type declaration within component type: adds to sub-scope.
        const auto *DT = InstDecl.getType();
        if (DT) {
          EXPECTED_TRY(validate(*DT).map_error([](auto E) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
            return E;
          }));
        }
      } else if (InstDecl.isCoreType()) {
        // Core type declaration within component type.
        const auto *CDT = InstDecl.getCoreType();
        if (CDT) {
          EXPECTED_TRY(validate(*CDT).map_error([](auto E) {
            spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
            return E;
          }));
        }
      } else if (InstDecl.isAlias()) {
        // Alias within component type.
        EXPECTED_TRY(validate(InstDecl.getAlias()).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
      }
    }
  }
  return TypeSize;
}

Expect<uint64_t> Validator::validateInstanceTypeDecls(
    Span<const AST::Component::InstanceDecl> Decls) noexcept {
  // Helper for case-insensitive name comparison.
  auto toLower = [](std::string_view SV) -> std::string {
    std::string Result(SV);
    std::transform(Result.begin(), Result.end(), Result.begin(),
                   [](unsigned char C) {
                     return static_cast<char>(std::tolower(C));
                   });
    return Result;
  };

  std::unordered_set<std::string> ExportNames;
  uint64_t TypeSize = 1; // Start with 1 for the type itself.

  for (const auto &Decl : Decls) {
    if (Decl.isExportDecl()) {
      const auto &Exp = Decl.getExportDecl();
      // Validate the export's ExternDesc first (type index bounds, etc).
      EXPECTED_TRY(validate(Exp.getExternDesc()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
      // Validate export name validity.
      EXPECTED_TRY(validate(Decl).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
      // Check for duplicate export names (case-insensitive).
      std::string LowerName = toLower(Exp.getName());
      if (!ExportNames.emplace(LowerName).second) {
        spdlog::error(ErrCode::Value::ExportNameConflict);
        spdlog::error(
            "    InstanceType: export name '{}' conflicts with previous"sv,
            Exp.getName());
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return Unexpect(ErrCode::Value::ExportNameConflict);
      }
      // Accumulate type size from exports.
      TypeSize += getExternDescTypeSize(Exp.getExternDesc(), CompCtx);
      if (TypeSize > MAX_TYPE_SIZE) {
        spdlog::error(ErrCode::Value::TypeSizeExceeded);
        spdlog::error(
            "    InstanceType: effective type size exceeds the limit"sv);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return Unexpect(ErrCode::Value::TypeSizeExceeded);
      }
    } else if (Decl.isType()) {
      // Type declaration within instance type: adds to sub-scope.
      const auto *DT = Decl.getType();
      if (DT) {
        EXPECTED_TRY(validate(*DT).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
      }
    } else if (Decl.isCoreType()) {
      // Core type declaration within instance type.
      const auto *CDT = Decl.getCoreType();
      if (CDT) {
        EXPECTED_TRY(validate(*CDT).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
          return E;
        }));
      }
    } else if (Decl.isAlias()) {
      // Alias within instance type.
      EXPECTED_TRY(validate(Decl.getAlias()).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_DefType));
        return E;
      }));
    }
  }
  return TypeSize;
}

} // namespace Validator
} // namespace WasmEdge
