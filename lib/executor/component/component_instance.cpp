// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate core instance section. See executor.h.
Expect<void> ComponentExecutor::instantiate(
    Component::Instantiator &Ctx,
    const AST::Component::CoreInstanceSection &CoreInstSec) {
  for (const auto &Expr : CoreInstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      // Instantiate-with-arguments: an import manager isolates the imports.
      Runtime::Component::ImportManager ImportMgr;
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        EXPECTED_TRY(const auto *ModInst,
                     Ctx.getCoreModuleInstance(Arg.getIndex()));
        ImportMgr.addCoreModuleInstance(Arg.getName(), ModInst);
      }
      EXPECTED_TRY(const auto *Mod,
                   Ctx.getInstance().getModule(Expr.getModuleIndex()));
      // A core module takes its imports from the arguments, not the store.
      auto NewModInst = std::make_unique<Runtime::Instance::ModuleInstance>("");
      auto Instantiate = [this, &ImportMgr, &NewModInst,
                          Mod]() -> Expect<void> {
        return getCoreExecutor().instantiateModule(
            [&ImportMgr](std::string_view Name)
                -> const Runtime::Instance::ModuleInstance * {
              return ImportMgr.findCoreModuleInstance(Name);
            },
            *NewModInst, *Mod);
      };
      // Only a start function runs guest code: it is an implicit task of the
      // instance under instantiation.
      if (Mod->getStartSection().getContent().has_value()) {
        EXPECTED_TRY(runImplicitTask(&Ctx.getInstance(), Instantiate));
      } else {
        EXPECTED_TRY(Instantiate());
      }
      Ctx.addCoreModuleInstance(std::move(NewModInst));
    } else {
      // Inline exports: create a core module instance with the exports.
      auto Mod = std::make_unique<Runtime::Instance::ModuleInstance>("");
      uint32_t FuncExpIdx = 0;
      uint32_t TableExpIdx = 0;
      uint32_t MemoryExpIdx = 0;
      uint32_t GlobalExpIdx = 0;
      uint32_t TagExpIdx = 0;

      for (const auto &Exp : Expr.getInlineExports()) {
        const auto &SortIdx = Exp.getSortIdx();
        const uint32_t Idx = SortIdx.getIdx();
        switch (SortIdx.getSort().getCoreSortType()) {
        case AST::Component::Sort::CoreSortType::Func: {
          // The inline TypeList must hold the host function's defined type.
          EXPECTED_TRY(auto *Func, Ctx.getCoreFunction(Idx));
          if (Func->isHostFunction()) {
            Mod->importHostFunction(Func);
          } else {
            Mod->importFunction(Func);
          }
          Mod->exportFunction(Exp.getName(), FuncExpIdx++);
          break;
        }
        case AST::Component::Sort::CoreSortType::Table: {
          EXPECTED_TRY(auto *Table, Ctx.getCoreTable(Idx));
          Mod->importTable(Table);
          Mod->exportTable(Exp.getName(), TableExpIdx++);
          break;
        }
        case AST::Component::Sort::CoreSortType::Memory: {
          EXPECTED_TRY(auto *Memory, Ctx.getCoreMemory(Idx));
          Mod->importMemory(Memory);
          Mod->exportMemory(Exp.getName(), MemoryExpIdx++);
          break;
        }
        case AST::Component::Sort::CoreSortType::Global: {
          EXPECTED_TRY(auto *Global, Ctx.getCoreGlobal(Idx));
          Mod->importGlobal(Global);
          Mod->exportGlobal(Exp.getName(), GlobalExpIdx++);
          break;
        }
        case AST::Component::Sort::CoreSortType::Tag: {
          EXPECTED_TRY(auto *Tag, Ctx.getCoreTag(Idx));
          Mod->importTag(Tag);
          Mod->exportTag(Exp.getName(), TagExpIdx++);
          break;
        }
        case AST::Component::Sort::CoreSortType::Type:
        case AST::Component::Sort::CoreSortType::Module:
        case AST::Component::Sort::CoreSortType::Instance:
          spdlog::error(ErrCode::Value::CoreInvalidExport);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_CoreInlineExport));
          return Unexpect(ErrCode::Value::CoreInvalidExport);
        default:
          assumingUnreachable();
        }
      }

      // Add this core module instance to the component instance index space.
      Ctx.addCoreModuleInstance(std::move(Mod));
    }
  }
  return {};
}

// Instantiate instance section. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::InstanceSection &InstSec) {
  auto &CompInst = Ctx.getInstance();
  for (const auto &Expr : InstSec.getContent()) {
    if (Expr.isInstantiateModule()) {
      // Create an import manager to implement the isolation of imports.
      Runtime::Component::ImportManager ImportMgr;
      for (const auto &Arg : Expr.getInstantiateArgs()) {
        const auto &SortIdx = Arg.getIndex();
        const auto &Sort = SortIdx.getSort();
        const uint32_t Idx = SortIdx.getIdx();
        if (Sort.isCore()) {
          switch (Sort.getCoreSortType()) {
          case AST::Component::Sort::CoreSortType::Func:
          case AST::Component::Sort::CoreSortType::Table:
          case AST::Component::Sort::CoreSortType::Memory:
          case AST::Component::Sort::CoreSortType::Global:
          case AST::Component::Sort::CoreSortType::Type:
            // A component import is never of these core sorts, and core
            // types carry no runtime state.
            break;
          case AST::Component::Sort::CoreSortType::Instance: {
            EXPECTED_TRY(const auto *ModInst, Ctx.getCoreModuleInstance(Idx));
            ImportMgr.addCoreModuleInstance(Arg.getName(), ModInst);
            break;
          }
          case AST::Component::Sort::CoreSortType::Module: {
            EXPECTED_TRY(const auto *Mod, CompInst.getModule(Idx));
            ImportMgr.addCoreModule(Arg.getName(), Mod);
            break;
          }
          default:
            assumingUnreachable();
          }
        } else {
          switch (Sort.getSortType()) {
          case AST::Component::Sort::SortType::Func: {
            EXPECTED_TRY(auto *Func, Ctx.getFunction(Idx));
            ImportMgr.addFunction(Arg.getName(), Func);
            break;
          }
          case AST::Component::Sort::SortType::Instance: {
            EXPECTED_TRY(const auto *Inst, Ctx.getComponentInstance(Idx));
            ImportMgr.addComponentInstance(Arg.getName(), Inst);
            break;
          }
          case AST::Component::Sort::SortType::Type: {
            EXPECTED_TRY(const auto *TypeDef, CompInst.getTypeDefinition(Idx));
            ImportMgr.addType(Arg.getName(), *TypeDef);
            break;
          }
          case AST::Component::Sort::SortType::Component: {
            EXPECTED_TRY(const auto *CompDef,
                         CompInst.getComponentDefinition(Idx));
            ImportMgr.addComponent(Arg.getName(), *CompDef);
            break;
          }
          case AST::Component::Sort::SortType::Value: {
            EXPECTED_TRY(const auto *Val, Ctx.getValue(Idx));
            ImportMgr.addValue(Arg.getName(), *Val);
            break;
          }
          default:
            assumingUnreachable();
          }
        }
      }
      EXPECTED_TRY(const auto *CompDef,
                   CompInst.getComponentDefinition(Expr.getComponentIndex()));
      // The lexical parent is the value's captured definition environment.
      EXPECTED_TRY(
          auto NewCompInst,
          instantiate(ImportMgr, *CompDef->Ast,
                      CompDef->Env != nullptr ? CompDef->Env : &CompInst));
      Ctx.addComponentInstance(std::move(NewCompInst));
    } else {
      // Inline exports: create a component instance with the exports. It is
      // part of this tree, so what it exports is owned by the root.
      auto Comp =
          std::make_unique<Runtime::Instance::ComponentInstance>("", &CompInst);
      uint32_t CoreModExpIdx = 0;
      uint32_t TypeExpIdx = 0;
      uint32_t CompExpIdx = 0;

      for (const auto &Exp : Expr.getInlineExports()) {
        const auto &SortIdx = Exp.getSortIdx();
        const uint32_t Idx = SortIdx.getIdx();
        const auto &Sort = SortIdx.getSort();

        if (Sort.isCore()) {
          switch (Sort.getCoreSortType()) {
          case AST::Component::Sort::CoreSortType::Module: {
            EXPECTED_TRY(const auto *Mod, CompInst.getModule(Idx));
            Comp->addModule(*Mod);
            Comp->exportCoreModule(Exp.getName(), CoreModExpIdx++);
            break;
          }
          case AST::Component::Sort::CoreSortType::Instance: {
            EXPECTED_TRY(const auto *ModInst, Ctx.getCoreModuleInstance(Idx));
            Comp->exportCoreModuleInstance(Exp.getName(), ModInst);
            break;
          }
          default:
            // A component instance exports no other core sort; validation
            // rejects them.
            break;
          }
        } else {
          switch (Sort.getSortType()) {
          case AST::Component::Sort::SortType::Func: {
            EXPECTED_TRY(auto *Func, Ctx.getFunction(Idx));
            Comp->exportFunction(Exp.getName(), Func);
            break;
          }
          case AST::Component::Sort::SortType::Instance: {
            EXPECTED_TRY(const auto *Inst, Ctx.getComponentInstance(Idx));
            Comp->exportComponentInstance(Exp.getName(), Inst);
            break;
          }
          case AST::Component::Sort::SortType::Type: {
            EXPECTED_TRY(const auto *TypeDef, CompInst.getTypeDefinition(Idx));
            Comp->addTypeDefinition(*TypeDef);
            Comp->exportType(Exp.getName(), TypeExpIdx++);
            break;
          }
          case AST::Component::Sort::SortType::Component: {
            EXPECTED_TRY(const auto *CompDef,
                         CompInst.getComponentDefinition(Idx));
            Comp->addComponentDefinition(*CompDef);
            Comp->exportComponent(Exp.getName(), CompExpIdx++);
            break;
          }
          case AST::Component::Sort::SortType::Value: {
            EXPECTED_TRY(const auto *Val, Ctx.getValue(Idx));
            Comp->exportValue(Exp.getName(), *Val);
            break;
          }
          default:
            assumingUnreachable();
          }
        }
      }

      // Add this component instance to the component instance index space.
      Ctx.addComponentInstance(std::move(Comp));
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
