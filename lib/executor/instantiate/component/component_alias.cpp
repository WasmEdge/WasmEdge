// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::AliasSection &AliasSec) {
  for (const auto &Alias : AliasSec.getContent()) {
    const auto &Sort = Alias.getSort();
    switch (Alias.getTargetType()) {
    case AST::Component::Alias::TargetType::Export: {
      const auto &Export = Alias.getExport();
      if (Sort.isCore()) {
        // Core-module exports of component instances.
        if (Sort.getCoreSortType() ==
            AST::Component::Sort::CoreSortType::Module) {
          const auto *CInst = CompInst.getComponentInstance(Export.first);
          const auto *Mod =
              CInst != nullptr ? CInst->findCoreModule(Export.second) : nullptr;
          if (Mod == nullptr) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    alias export core module '{}' not found"sv,
                          Export.second);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          CompInst.addModule(*Mod);
          break;
        }
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete alias export core sort"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      switch (Sort.getSortType()) {
      case AST::Component::Sort::SortType::Func: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        auto *FuncInst = CInst->findFunction(Export.second);
        CompInst.addFunction(FuncInst);
        break;
      }
      case AST::Component::Sort::SortType::Type: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        if (CInst == nullptr) {
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    alias export type: unknown instance"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
        CompInst.addTypeWithResource(CInst->findType(Export.second),
                                     CInst->findTypeResource(Export.second));
        break;
      }
      case AST::Component::Sort::SortType::Instance: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        const auto *Nested = CInst->findComponentInstance(Export.second);
        if (Nested == nullptr) {
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    alias export instance '{}' not found"sv,
                        Export.second);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
        CompInst.addComponentInstance(Nested);
        break;
      }
      case AST::Component::Sort::SortType::Component: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        const auto *Entry = CInst != nullptr
                                ? CInst->findComponentEntry(Export.second)
                                : nullptr;
        if (Entry == nullptr) {
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    alias export component '{}' not found"sv,
                        Export.second);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
        CompInst.addComponentEntry(Entry->Ast, Entry->Env);
        break;
      }
      case AST::Component::Sort::SortType::Value: {
        const auto *CInst = CompInst.getComponentInstance(Export.first);
        const auto *V =
            CInst != nullptr ? CInst->findValueExport(Export.second) : nullptr;
        if (V == nullptr) {
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    alias export value '{}' not found"sv,
                        Export.second);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
        CompInst.addValue(*V);
        break;
      }
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::CoreExport: {
      assuming(Sort.isCore());
      const auto &Export = Alias.getExport();
      const auto *ModInst = CompInst.getCoreModuleInstance(Export.first);
      auto FindExports = [&](const auto &Map) {
        return ModInst->unsafeFindExports(Map, Export.second);
      };
      switch (Sort.getCoreSortType()) {
      case AST::Component::Sort::CoreSortType::Func: {
        auto *FuncInst = ModInst->getFuncExports(FindExports);
        CompInst.addCoreFunction(FuncInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Table: {
        auto *TableInst = ModInst->getTableExports(FindExports);
        CompInst.addCoreTable(TableInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Memory: {
        auto *MemInst = ModInst->getMemoryExports(FindExports);
        CompInst.addCoreMemory(MemInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Global: {
        auto *GlobInst = ModInst->getGlobalExports(FindExports);
        CompInst.addCoreGlobal(GlobInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Tag: {
        auto *TagInst = ModInst->getTagExports(FindExports);
        CompInst.addCoreTag(TagInst);
        break;
      }
      case AST::Component::Sort::CoreSortType::Type:
      case AST::Component::Sort::CoreSortType::Module:
      case AST::Component::Sort::CoreSortType::Instance:
        // TODO: COMPONENT - complete the alias instantiation.
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    incomplete alias core:export"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      default:
        assumingUnreachable();
      }
      break;
    }
    case AST::Component::Alias::TargetType::Outer: {
      const auto &Outer = Alias.getOuter();
      // Walk the lexical parent chain; validation checked the depth.
      const auto *Target = &CompInst;
      for (uint32_t I = 0; I < Outer.first && Target != nullptr; ++I) {
        Target = Target->getParent();
      }
      if (Target == nullptr) {
        spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
        spdlog::error("    outer alias exceeds the instantiation depth"sv);
        return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
      }
      if (Sort.isCore()) {
        switch (Sort.getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Module: {
          const auto *Mod = Target->getModule(Outer.second);
          if (Mod == nullptr) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    outer core module {} not found"sv, Outer.second);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          CompInst.addModule(*Mod);
          break;
        }
        case AST::Component::Sort::CoreSortType::Type: {
          const auto *Ty = Target->getCoreType(Outer.second);
          if (Ty == nullptr) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    outer core type {} not found"sv, Outer.second);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          CompInst.addCoreType(*Ty);
          break;
        }
        default:
          // Validation only admits module / type outer aliases.
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete alias target outer: core:sort"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
        }
      } else {
        switch (Sort.getSortType()) {
        case AST::Component::Sort::SortType::Type:
          CompInst.addTypeWithResource(Target->getType(Outer.second),
                                       Target->getTypeResource(Outer.second));
          break;
        case AST::Component::Sort::SortType::Component:
          if (Target->getComponent(Outer.second) == nullptr) {
            spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
            spdlog::error("    outer component {} not found"sv, Outer.second);
            return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
          }
          CompInst.addComponentEntry(Target->getComponent(Outer.second),
                                     Target->getComponentEnv(Outer.second));
          break;
        default:
          // Validation only admits type / component outer aliases.
          spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
          spdlog::error("    incomplete alias target outer: sort"sv);
          return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
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
