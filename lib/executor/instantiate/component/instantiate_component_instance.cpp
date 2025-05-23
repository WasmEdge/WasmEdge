// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;

// In this function, we will create a new module instance and insert it into
// component module instance index space.
Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &Sec) {
  for (const CoreInstanceExpr &InstExpr : Sec.getContent()) {
    auto Func = [this, &StoreMgr, &CompInst](auto &&Expr) -> Expect<void> {
      using T = std::decay_t<decltype(Expr)>;
      if constexpr (std::is_same_v<T, CoreInstantiate>) {
        for (auto &Arg : Expr.getArgs()) {
          // Expr a list of `(with (name $instance))`
          // each $instance get named as `name` as statement tell
          EXPECTED_TRY(auto Inst, CompInst.getModuleInstance(Arg.getIndex()));
          StoreMgr.addNamedModule(Arg.getName(), Inst);
        }
        const AST::Module &Mod = CompInst.getModule(Expr.getModuleIdx());
        if (auto Res = instantiate(StoreMgr, Mod)) {
          CompInst.addModuleInstance(std::move(*Res));
        } else {
          spdlog::error("instantiating core instance {}", Expr.getModuleIdx());
          return Unexpect(Res);
        }
      } else if constexpr (std::is_same_v<T,
                                          AST::Component::CoreInlineExports>) {
        // create an immediate module instance, which has no name
        //
        // this happened usually at `(with xxx)` statement, where we can have a
        // module instance expression.
        auto M = std::make_unique<Runtime::Instance::ModuleInstance>("");

        for (auto &S : Expr.getExports()) {
          const auto &SortIdx = S.getSortIdx();
          switch (SortIdx.getSort()) {
          case CoreSort::Func:
            // The module instance takes functions and export them
            {
              EXPECTED_TRY(auto CoreFunc, CompInst.getCoreFunctionInstance(
                                              SortIdx.getSortIdx()));
              M->exportFunction(S.getName(),
                                // get stored core function
                                CoreFunc);
              break;
            }
          case CoreSort::Table: {
            EXPECTED_TRY(auto TableInst,
                         CompInst.getCoreTableInstance(SortIdx.getSortIdx()));
            M->exportTable(S.getName(), TableInst);
            break;
          }
          case CoreSort::Memory: {
            EXPECTED_TRY(auto MemInst,
                         CompInst.getCoreMemoryInstance(SortIdx.getSortIdx()));
            M->exportMemory(S.getName(), MemInst);
            break;
          }
          case CoreSort::Global: {
            EXPECTED_TRY(auto GlobInst,
                         CompInst.getCoreGlobalInstance(SortIdx.getSortIdx()));
            M->exportGlobal(S.getName(), GlobInst);
            break;
          }
          case CoreSort::Type:
          case CoreSort::Module:
          case CoreSort::Instance:
            spdlog::error("A module instance cannot exports types, modules, or "
                          "instances"sv);
            return Unexpect(ErrCode::Value::CoreInvalidExport);
          }
        }

        // insert this immediate module instance into the component instance
        CompInst.addModuleInstance(std::move(M));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, InstExpr));
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::InstanceSection &Sec) {
  for (auto &InstExpr : Sec.getContent()) {
    if (std::holds_alternative<AST::Component::Instantiate>(InstExpr)) {
      auto &Instantiate = std::get<AST::Component::Instantiate>(InstExpr);

      auto &TemplateComponent =
          CompInst.getComponent(Instantiate.getComponentIdx());
      auto Res = instantiate(StoreMgr, TemplateComponent);
      if (!Res) {
        return Unexpect(Res);
      }
      auto Inst = std::move(*Res);

      for (auto &Arg : Instantiate.getArgs()) {
        const auto &Idx = Arg.getIndex();
        const auto &S = Idx.getSort();
        if (std::holds_alternative<CoreSort>(S)) {
          // TODO: insert below into mapping
          switch (std::get<CoreSort>(S)) {
          case CoreSort::Func:
            spdlog::warn("incomplete (with {}) core:func"sv, Arg.getName());
            break;
          case CoreSort::Table:
            spdlog::warn("incomplete (with {}) core:table"sv, Arg.getName());
            break;
          case CoreSort::Memory:
            spdlog::warn("incomplete (with {}) core:memory"sv, Arg.getName());
            break;
          case CoreSort::Global:
            spdlog::warn("incomplete (with {}) core:global"sv, Arg.getName());
            break;
          case CoreSort::Type:
            spdlog::warn("incomplete (with {}) core:type"sv, Arg.getName());
            break;
          case CoreSort::Module:
            spdlog::warn("incomplete (with {}) core:module"sv, Arg.getName());
            break;
          case CoreSort::Instance: {
            auto Res = CompInst.getModuleInstance(Idx.getSortIdx());
            if (!Res) {
              return Unexpect(Res);
            }
            StoreMgr.addNamedModule(Arg.getName(), *Res);
            break;
          }
          }
        } else if (std::holds_alternative<SortCase>(S)) {
          switch (std::get<SortCase>(S)) {
          case SortCase::Func: {
            EXPECTED_TRY(auto *FuncInst,
                         CompInst.getFunctionInstance(Idx.getSortIdx()));
            Inst->addExport(Arg.getName(), FuncInst);
            break;
          }
          case SortCase::Value: {
            // TODO: where to register value argument?
            spdlog::warn("incomplete (with {}) value"sv, Arg.getName());
            break;
          }
          case SortCase::Type: {
            EXPECTED_TRY(auto const GotCompInst,
                         CompInst.getComponentInstance(Idx.getSortIdx()));
            if (auto Res =
                    StoreMgr.registerComponent(Arg.getName(), GotCompInst);
                !Res) {
              spdlog::error("failed to register component instance"sv);
              return Unexpect(Res);
            }
            break;
          }
          case SortCase::Component: {
            // with a component instance
            EXPECTED_TRY(auto GotCompInst,
                         CompInst.getComponentInstance(Idx.getSortIdx()));
            if (auto Res =
                    StoreMgr.registerComponent(Arg.getName(), GotCompInst);
                !Res) {
              spdlog::error("failed to register component instance"sv);
              return Unexpect(Res);
            }
            break;
          }
          case SortCase::Instance:
            // TODO: figure out how to do this registry
            spdlog::warn("incomplete (with {}) instance"sv, Arg.getName());
            break;
          }
        }
      }

      EXPECTED_TRY(Inst->instantiate());
      CompInst.addComponentInstance(std::move(Inst));
    } else {
      std::get<InlineExports>(InstExpr).getExports();
      // TODO: complete inline exports
      spdlog::warn("incomplete component inline exports"sv);
    }
  }

  return CompInst.instantiate();
}

} // namespace Executor
} // namespace WasmEdge
