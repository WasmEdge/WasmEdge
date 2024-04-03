#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace AST::Component;

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreInstanceSection &Sec) {
  for (const CoreInstanceExpr &InstExpr : Sec.getContent()) {
    if (std::holds_alternative<CoreInstantiate>(InstExpr)) {
      auto Instantiate = std::get<CoreInstantiate>(InstExpr);

      for (auto Arg : Instantiate.getArgs()) {
        // instantiate a list of `(with (name $instance))`
        // each $instance get named as `name` as statement tell
        StoreMgr.addNamedModule(Arg.getName(),
                                CompInst.getModuleInstance(Arg.getIndex()));
      }
      const AST::Module &Mod = CompInst.getModule(Instantiate.getModuleIdx());
      auto Res = instantiate(StoreMgr, Mod);
      if (!Res) {
        return Unexpect(Res);
      }
      CompInst.addModuleInstance(std::move(*Res));
    } else {
      auto InstantiateExpr =
          std::get<AST::Component::CoreInlineExports>(InstExpr);

      // create an immediate module instance, which has no name
      //
      // this happened usually at `(with xxx)` statement, where we can have a
      // module instance expression.
      auto M = std::make_unique<Runtime::Instance::ModuleInstance>("");

      for (auto const &S : InstantiateExpr.getExports()) {
        auto SortIdx = S.getSortIdx();
        switch (SortIdx.getSort()) {
        case CoreSort::Func: {
          // The module instance takes functions and export them
          M->exportFunction(
              S.getName(),
              // get stored core function
              CompInst.getCoreFunctionInstance(SortIdx.getSortIdx()));
          break;
        }
        default:
          // TODO: figure out if this is invalid, then we should return an error
          spdlog::warn("incomplete core inline export non-function");
          break;
        }
      }

      // insert this immediate module instance into the component instance
      CompInst.addModuleInstance(std::move(M));
    }
  }
  return {};
}

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::InstanceSection &Sec) {
  for (const auto &InstExpr : Sec.getContent()) {
    if (std::holds_alternative<AST::Component::Instantiate>(InstExpr)) {
      auto Instantiate = std::get<AST::Component::Instantiate>(InstExpr);

      for (auto Arg : Instantiate.getArgs()) {
        auto Idx = Arg.getIndex();
        auto S = Idx.getSort();
        if (std::holds_alternative<CoreSort>(S)) {
          // TODO: insert these into mapping
          switch (std::get<CoreSort>(S)) {
          case CoreSort::Func:
            spdlog::warn("incomplete (with {}) core:func", Arg.getName());
            break;
          case CoreSort::Table:
            spdlog::warn("incomplete (with {}) core:table", Arg.getName());
            break;
          case CoreSort::Memory:
            spdlog::warn("incomplete (with {}) core:memory", Arg.getName());
            break;
          case CoreSort::Global:
            spdlog::warn("incomplete (with {}) core:global", Arg.getName());
            break;
          case CoreSort::Type:
            spdlog::warn("incomplete (with {}) core:type", Arg.getName());
            break;
          case CoreSort::Module:
            spdlog::warn("incomplete (with {}) core:module", Arg.getName());
            break;
          case CoreSort::Instance:
            StoreMgr.addNamedModule(
                Arg.getName(), CompInst.getModuleInstance(Idx.getSortIdx()));
            break;
          }
        } else if (std::holds_alternative<SortCase>(S)) {
          switch (std::get<SortCase>(S)) {
          case SortCase::Func: // TODO:
            spdlog::warn("incomplete (with {}) function", Arg.getName());
            break;
          case SortCase::Value: // TODO:
            spdlog::warn("incomplete (with {}) value", Arg.getName());
            break;
          case SortCase::Type: // TODO:
            spdlog::warn("incomplete (with {}) type", Arg.getName());
            break;
          case SortCase::Component:
            if (auto Res = StoreMgr.registerComponent(
                    Arg.getName(),
                    CompInst.getComponentInstance(Idx.getSortIdx()));
                !Res) {
              spdlog::error("failed to register component instance");
              return Unexpect(Res);
            }
            break;
          case SortCase::Instance: // TODO:
            spdlog::warn("incomplete (with {}) instance", Arg.getName());
            break;
          }
        }
      }
      auto C = CompInst.getComponent(Instantiate.getComponentIdx());
      auto Res = instantiate(StoreMgr, C);
      if (!Res) {
        return Unexpect(Res);
      }
      auto Inst = std::move(*Res);
      CompInst.addComponentInstance(std::move(Inst));
    } else {
      std::get<CompInlineExports>(InstExpr).getExports();
      // TODO: complete inline exports
      spdlog::warn("incomplete component inline exports");
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
