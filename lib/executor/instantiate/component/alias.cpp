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
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::AliasSection &AliasSec) {
  for (auto A : AliasSec.getContent()) {
    auto T = A.getTarget();
    auto S = A.getSort();
    if (std::holds_alternative<CoreSort>(S)) {
      if (std::holds_alternative<AliasTargetExport>(T)) {
        // This means instance exports a function
        auto Exp = std::get<AliasTargetExport>(T);
        const auto *ModInst = CompInst.getModuleInstance(Exp.getInstanceIdx());

        switch (std::get<CoreSort>(S)) {
        case CoreSort::Func: {
          auto *FuncInst = ModInst->getFuncExports(
              [&](const std::map<std::string,
                                 Runtime::Instance::FunctionInstance *,
                                 std::less<>> &Map) {
                return ModInst->unsafeFindExports(Map, Exp.getName());
              });
          CompInst.addCoreFunctionInstance(FuncInst);
          break;
        }
        case CoreSort::Table:
        case CoreSort::Memory: {
          auto *MemInst = ModInst->getMemoryExports(
              [&](const std::map<std::string,
                                 Runtime::Instance::MemoryInstance *,
                                 std::less<>> &Map) {
                return ModInst->unsafeFindExports(Map, Exp.getName());
              });
          spdlog::info("memory instance before insert: {}", MemInst != nullptr);
          CompInst.addCoreMemoryInstance(MemInst);
          break;
        }
        case CoreSort::Global:
        case CoreSort::Type:
        case CoreSort::Module:
        case CoreSort::Instance:
          spdlog::warn("incomplete core alias sort");
          break;
        }
      } else {
        spdlog::warn("incomplete alias target outer");
      }
    } else if (std::holds_alternative<SortCase>(S)) {
      if (std::holds_alternative<AliasTargetExport>(T)) {
        auto Exp = std::get<AliasTargetExport>(T);

        switch (std::get<SortCase>(S)) {
        case SortCase::Func: {
          auto *CInst = CompInst.getComponentInstance(Exp.getInstanceIdx());
          auto *FuncInst = CInst->findFuncExports(Exp.getName());
          CompInst.addFunctionInstance(FuncInst);
          break;
        }
        case SortCase::Value: // TODO
        case SortCase::Type:
        case SortCase::Component:
        case SortCase::Instance:
          spdlog::warn("incomplete alias sort target export");
          break;
        }
      } else {
        auto Out = std::get<AliasTargetOuter>(T);

        switch (std::get<SortCase>(S)) {
        case SortCase::Func: {
          auto *FuncInst = CompInst.getComponentInstance(Out.getComponent())
                               ->getCoreFunctionInstance(Out.getIndex());
          CompInst.addFunctionInstance(FuncInst);
          break;
        }
        case SortCase::Value: // TODO
        case SortCase::Type:
        case SortCase::Component:
        case SortCase::Instance:
          spdlog::warn("incomplete alias sort outer");
          break;
        }
      }
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
