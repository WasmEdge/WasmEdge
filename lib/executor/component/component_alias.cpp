// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
// An alias of an export the providing instance does not have.
Expect<void> logExportNotFound(std::string_view Sort, std::string_view Name,
                               std::string_view Provider) noexcept {
  spdlog::error(ErrCode::Value::ComponentImportNotFound);
  spdlog::error(ErrInfo::InfoComponentLinking(Sort, Name, Provider));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Alias));
  return Unexpect(ErrCode::Value::ComponentImportNotFound);
}

// A sort validation admits nowhere on this alias target.
Expect<void> logUnexpectedSort() noexcept {
  spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Alias));
  return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
}
} // namespace

Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::AliasSection &AliasSec) {
  auto &CompInst = Ctx.getInstance();
  for (const auto &Alias : AliasSec.getContent()) {
    const auto &Sort = Alias.getSort();
    switch (Alias.getTargetType()) {
    case AST::Component::Alias::TargetType::Export: {
      const std::string_view Name = Alias.getExport().second;
      EXPECTED_TRY(const auto *Provider,
                   Ctx.getComponentInstance(Alias.getExport().first));
      if (Sort.isCore()) {
        // Core-module exports of component instances.
        if (Sort.getCoreSortType() !=
            AST::Component::Sort::CoreSortType::Module) {
          return logUnexpectedSort();
        }
        const auto *Mod = Provider->findCoreModule(Name);
        if (Mod == nullptr) {
          return logExportNotFound("core module"sv, Name,
                                   Provider->getComponentName());
        }
        CompInst.addModule(*Mod);
        break;
      }
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        auto *Func = Provider->findFunction(Name);
        if (Func == nullptr) {
          return logExportNotFound("function"sv, Name,
                                   Provider->getComponentName());
        }
        Ctx.importFunction(Func);
        break;
      }
      case AST::Component::Sort::SortType::Type: {
        const auto *TypeDef = Provider->findTypeDefinition(Name);
        if (TypeDef == nullptr) {
          return logExportNotFound("type"sv, Name,
                                   Provider->getComponentName());
        }
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      case AST::Component::Sort::SortType::Instance: {
        const auto *Nested = Provider->findComponentInstance(Name);
        if (Nested == nullptr) {
          return logExportNotFound("instance"sv, Name,
                                   Provider->getComponentName());
        }
        Ctx.importComponentInstance(Nested);
        break;
      }
      case AST::Component::Sort::SortType::Component: {
        const auto *CompDef = Provider->findComponentDefinition(Name);
        if (CompDef == nullptr) {
          return logExportNotFound("component"sv, Name,
                                   Provider->getComponentName());
        }
        CompInst.addComponentDefinition(*CompDef);
        break;
      }
      case AST::Component::Sort::SortType::Value: {
        const auto *Val = Provider->findValue(Name);
        if (Val == nullptr) {
          return logExportNotFound("value"sv, Name,
                                   Provider->getComponentName());
        }
        Ctx.addValue(*Val);
        break;
      }
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::CoreExport: {
      assuming(Sort.isCore());
      const std::string_view Name = Alias.getExport().second;
      EXPECTED_TRY(const auto *ModInst,
                   Ctx.getCoreModuleInstance(Alias.getExport().first));
      auto FindExports = [&](const auto &Map) {
        return ModInst->unsafeFindExports(Map, Name);
      };
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func:
        Ctx.importCoreFunction(ModInst->getFuncExports(FindExports));
        break;
      case AST::Component::Sort::CoreSortType::Table:
        Ctx.importCoreTable(ModInst->getTableExports(FindExports));
        break;
      case AST::Component::Sort::CoreSortType::Memory:
        Ctx.importCoreMemory(ModInst->getMemoryExports(FindExports));
        break;
      case AST::Component::Sort::CoreSortType::Global:
        Ctx.importCoreGlobal(ModInst->getGlobalExports(FindExports));
        break;
      case AST::Component::Sort::CoreSortType::Tag:
        Ctx.importCoreTag(ModInst->getTagExports(FindExports));
        break;
      case AST::Component::Sort::CoreSortType::Type:
      case AST::Component::Sort::CoreSortType::Module:
      case AST::Component::Sort::CoreSortType::Instance:
        // Validation rejects these sorts on a core instance.
        return logUnexpectedSort();
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::Outer: {
      const uint32_t Idx = Alias.getOuter().second;
      // Walk the lexical parent chain; validation checked the depth.
      const auto *Target = &CompInst;
      for (uint32_t I = 0; I < Alias.getOuter().first; ++I) {
        Target = Target->getParent();
        assuming(Target != nullptr);
      }
      if (Sort.isCore()) {
        switch (Sort.getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Module: {
          EXPECTED_TRY(const auto *Mod, Target->getModule(Idx));
          CompInst.addModule(*Mod);
          break;
        }
        case AST::Component::Sort::CoreSortType::Type: {
          EXPECTED_TRY(const auto *Ty, Target->getCoreType(Idx));
          CompInst.addCoreType(*Ty);
          break;
        }
        default:
          // Validation only admits module / type outer aliases.
          return logUnexpectedSort();
        }
      } else {
        switch (Sort.getSortType()) {
        case AST::Component::Sort::SortType::Type: {
          EXPECTED_TRY(const auto *TypeDef, Target->getTypeDefinition(Idx));
          CompInst.addTypeDefinition(*TypeDef);
          break;
        }
        case AST::Component::Sort::SortType::Component: {
          EXPECTED_TRY(const auto *CompDef,
                       Target->getComponentDefinition(Idx));
          CompInst.addComponentDefinition(*CompDef);
          break;
        }
        default:
          // Validation only admits type / component outer aliases.
          return logUnexpectedSort();
        }
      }
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
