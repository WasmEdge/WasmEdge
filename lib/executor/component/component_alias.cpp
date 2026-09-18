// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

namespace {
Expect<void> logNotFound(std::string_view Sort, std::string_view Name,
                         std::string_view Provider) {
  spdlog::error(ErrCode::Value::ComponentImportNotFound);
  spdlog::error(ErrInfo::InfoComponentLinking(Sort, Name, Provider));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Alias));
  return Unexpect(ErrCode::Value::ComponentImportNotFound);
}

Expect<void> logUnexpectedSort() {
  spdlog::error(ErrCode::Value::ComponentUnexpectedSort);
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Alias));
  return Unexpect(ErrCode::Value::ComponentUnexpectedSort);
}
} // namespace

// Instantiate alias section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::AliasSection &AliasSec) {
  for (const auto &Alias : AliasSec.getContent()) {
    const auto &Sort = Alias.getSort();
    switch (Alias.getTargetType()) {
    case AST::Component::Alias::TargetType::Export: {
      // An export of a component instance.
      const auto &[InstIdx, Name] = Alias.getExport();
      EXPECTED_TRY(const auto *Inst, CompInst.getComponentInstance(InstIdx));
      const std::string_view From = Inst->getComponentName();
      if (Sort.isCore()) {
        if (Sort.getCoreSortType() !=
            AST::Component::Sort::CoreSortType::Module) {
          return logUnexpectedSort();
        }
        const auto *Mod = Inst->findCoreModule(Name);
        if (Mod == nullptr) {
          return logNotFound("core module"sv, Name, From);
        }
        CompInst.addModule(*Mod);
        break;
      }
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        auto *Func = Inst->findFunction(Name);
        if (Func == nullptr) {
          return logNotFound("func"sv, Name, From);
        }
        CompInst.addFunction(Func);
        break;
      }
      case AST::Component::Sort::SortType::Value: {
        const auto *Val = Inst->findValue(Name);
        if (Val == nullptr) {
          return logNotFound("value"sv, Name, From);
        }
        CompInst.addValue(*Val);
        break;
      }
      case AST::Component::Sort::SortType::Type: {
        const auto *TypeDef = Inst->findTypeDefinition(Name);
        if (TypeDef == nullptr) {
          return logNotFound("type"sv, Name, From);
        }
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      case AST::Component::Sort::SortType::Component: {
        const auto *Def = Inst->findComponentDefinition(Name);
        if (Def == nullptr) {
          return logNotFound("component"sv, Name, From);
        }
        CompInst.addComponentDefinition(*Def);
        break;
      }
      case AST::Component::Sort::SortType::Instance: {
        const auto *Nested = Inst->findComponentInstance(Name);
        if (Nested == nullptr) {
          return logNotFound("instance"sv, Name, From);
        }
        CompInst.addComponentInstance(Nested);
        break;
      }
      default:
        return logUnexpectedSort();
      }
      break;
    }
    case AST::Component::Alias::TargetType::CoreExport: {
      // An export of a core module instance.
      const auto &[InstIdx, Name] = Alias.getExport();
      EXPECTED_TRY(const auto *ModInst,
                   CompInst.getCoreModuleInstance(InstIdx));
      const std::string_view From = ModInst->getModuleName();
      if (!Sort.isCore()) {
        return logUnexpectedSort();
      }
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func: {
        auto *Func = ModInst->findFuncExports(Name);
        if (Func == nullptr) {
          return logNotFound("core func"sv, Name, From);
        }
        CompInst.addCoreFunction(Func);
        break;
      }
      case AST::Component::Sort::CoreSortType::Table: {
        auto *Tab = ModInst->findTableExports(Name);
        if (Tab == nullptr) {
          return logNotFound("core table"sv, Name, From);
        }
        CompInst.addCoreTable(Tab);
        break;
      }
      case AST::Component::Sort::CoreSortType::Memory: {
        auto *Mem = ModInst->findMemoryExports(Name);
        if (Mem == nullptr) {
          return logNotFound("core memory"sv, Name, From);
        }
        CompInst.addCoreMemory(Mem);
        break;
      }
      case AST::Component::Sort::CoreSortType::Global: {
        auto *Glob = ModInst->findGlobalExports(Name);
        if (Glob == nullptr) {
          return logNotFound("core global"sv, Name, From);
        }
        CompInst.addCoreGlobal(Glob);
        break;
      }
      case AST::Component::Sort::CoreSortType::Tag: {
        auto *Tag = ModInst->findTagExports(Name);
        if (Tag == nullptr) {
          return logNotFound("core tag"sv, Name, From);
        }
        CompInst.addCoreTag(Tag);
        break;
      }
      default:
        return logUnexpectedSort();
      }
      break;
    }
    case AST::Component::Alias::TargetType::Outer: {
      // A definition of an enclosing component, Count levels up.
      const auto &[Count, Idx] = Alias.getOuter();
      Runtime::Instance::ComponentInstance *Outer = &CompInst;
      for (uint32_t I = 0; I < Count && Outer != nullptr; ++I) {
        Outer = Outer->getParent();
      }
      if (Outer == nullptr) {
        spdlog::error(ErrCode::Value::WrongInstanceIndex);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Alias));
        return Unexpect(ErrCode::Value::WrongInstanceIndex);
      }
      if (Sort.isCore()) {
        switch (Sort.getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Module: {
          EXPECTED_TRY(const auto *Mod, Outer->getModule(Idx));
          CompInst.addModule(*Mod);
          break;
        }
        case AST::Component::Sort::CoreSortType::Type: {
          EXPECTED_TRY(const auto *Ty, Outer->getCoreType(Idx));
          CompInst.addCoreType(*Ty);
          break;
        }
        default:
          return logUnexpectedSort();
        }
        break;
      }
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Type: {
        EXPECTED_TRY(const auto *TypeDef, Outer->getTypeDefinition(Idx));
        CompInst.addTypeDefinition(*TypeDef);
        break;
      }
      case AST::Component::Sort::SortType::Component: {
        EXPECTED_TRY(const auto *Def, Outer->getComponentDefinition(Idx));
        CompInst.addComponentDefinition(*Def);
        break;
      }
      default:
        return logUnexpectedSort();
      }
      break;
    }
    default:
      return logUnexpectedSort();
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
