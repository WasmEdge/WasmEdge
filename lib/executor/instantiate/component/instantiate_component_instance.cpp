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

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &Sec) {
  // In this function, we will create a new module instance and insert it into
  // component module instance index space.
  for (const CoreInstanceExpr &InstExpr : Sec.getContent()) {
    auto Func = [this, &StoreMgr, &CompInst](auto &&Expr) -> Expect<void> {
      using T = std::decay_t<decltype(Expr)>;
      if constexpr (std::is_same_v<T, CoreInstantiate>) {
        for (auto &Arg : Expr.getArgs()) {
          // Expr a list of `(with (name $instance))`
          // each $instance get named as `name` as statement tell
          StoreMgr.addNamedModule(Arg.getName(),
                                  CompInst.getModuleInstance(Arg.getIndex()));
        }
        const AST::Module &Mod = CompInst.getModule(Expr.getModuleIdx());
        EXPECTED_TRY(auto Inst, instantiate(StoreMgr, Mod));
        CompInst.addModuleInstance(std::move(Inst));
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
            M->exportFunction(
                S.getName(),
                // get stored core function
                CompInst.getCoreFunctionInstance(SortIdx.getSortIdx()));
            break;
          case CoreSort::Table:
            M->exportTable(S.getName(),
                           CompInst.getCoreTableInstance(SortIdx.getSortIdx()));
            break;
          case CoreSort::Memory:
            M->exportMemory(S.getName(), CompInst.getCoreMemoryInstance(
                                             SortIdx.getSortIdx()));
            break;
          case CoreSort::Global:
            M->exportGlobal(S.getName(), CompInst.getCoreGlobalInstance(
                                             SortIdx.getSortIdx()));
            break;
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
    auto Func1 = [this, &StoreMgr, &CompInst](auto &&Expr) -> Expect<void> {
      using T = std::decay_t<decltype(Expr)>;
      if constexpr (std::is_same_v<T, AST::Component::Instantiate>) {
        for (auto &Arg : Expr.getArgs()) {
          auto Func2 = [&StoreMgr, &CompInst,
                        &Arg](auto &&Sort) -> Expect<void> {
            const auto &Idx = Arg.getIndex();
            using U = std::decay_t<decltype(Sort)>;
            if constexpr (std::is_same_v<U, CoreSort>) {
              // TODO: insert below into mapping
              switch (Sort) {
              case CoreSort::Func:
                spdlog::warn("incomplete (with {}) core:func"sv, Arg.getName());
                break;
              case CoreSort::Table:
                spdlog::warn("incomplete (with {}) core:table"sv,
                             Arg.getName());
                break;
              case CoreSort::Memory:
                spdlog::warn("incomplete (with {}) core:memory"sv,
                             Arg.getName());
                break;
              case CoreSort::Global:
                spdlog::warn("incomplete (with {}) core:global"sv,
                             Arg.getName());
                break;
              case CoreSort::Type:
                spdlog::warn("incomplete (with {}) core:type"sv, Arg.getName());
                break;
              case CoreSort::Module:
                spdlog::warn("incomplete (with {}) core:module"sv,
                             Arg.getName());
                break;
              case CoreSort::Instance:
                StoreMgr.addNamedModule(
                    Arg.getName(),
                    CompInst.getModuleInstance(Idx.getSortIdx()));
                break;
              }
            } else if constexpr (std::is_same_v<U, SortCase>) {
              switch (Sort) {
              case SortCase::Func: // TODO: figure out how to do this registry
                spdlog::warn("incomplete (with {}) function"sv, Arg.getName());
                break;
              case SortCase::Value: // TODO: figure out how to do this registry
                spdlog::warn("incomplete (with {}) value"sv, Arg.getName());
                break;
              case SortCase::Type: // TODO: figure out how to do this registry
                spdlog::warn("incomplete (with {}) type"sv, Arg.getName());
                break;
              case SortCase::Component:
                EXPECTED_TRY(
                    StoreMgr
                        .registerComponent(
                            Arg.getName(),
                            CompInst.getComponentInstance(Idx.getSortIdx()))
                        .map_error([](auto E) {
                          spdlog::error(
                              "failed to register component instance"sv);
                          return E;
                        }));
                break;
              case SortCase::Instance:
                // TODO: figure out how to do this registry
                spdlog::warn("incomplete (with {}) instance"sv, Arg.getName());
                break;
              }
            }
            return {};
          };
          EXPECTED_TRY(std::visit(Func2, Arg.getIndex().getSort()));
        }
        EXPECTED_TRY(auto Inst,
                     this->instantiate(StoreMgr, CompInst.getComponent(
                                                     Expr.getComponentIdx())));
        CompInst.addComponentInstance(std::move(Inst));
      } else if constexpr (std::is_same_v<T,
                                          AST::Component::CompInlineExports>) {
        Expr.getExports();
        // TODO: complete inline exports
        spdlog::warn("incomplete component inline exports"sv);
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func1, InstExpr));
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
