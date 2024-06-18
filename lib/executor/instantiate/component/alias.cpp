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
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::AliasSection &AliasSec) {
  for (auto &A : AliasSec.getContent()) {
    auto &T = A.getTarget();
    auto &S = A.getSort();
    if (std::holds_alternative<CoreSort>(S)) {
      if (std::holds_alternative<AliasTargetExport>(T)) {
        // This means instance exports a function
        auto &Exp = std::get<AliasTargetExport>(T);
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
          spdlog::warn("incomplete core alias sort: table");
          break;
        case CoreSort::Memory: {
          auto *MemInst = ModInst->getMemoryExports(
              [&](const std::map<std::string,
                                 Runtime::Instance::MemoryInstance *,
                                 std::less<>> &Map) {
                return ModInst->unsafeFindExports(Map, Exp.getName());
              });
          CompInst.addCoreMemoryInstance(MemInst);
          break;
        }
        case CoreSort::Global:
          spdlog::warn("incomplete core alias sort: global"sv);
          break;
        case CoreSort::Type:
          spdlog::warn("incomplete core alias sort: type"sv);
          break;
        case CoreSort::Module:
          spdlog::warn("incomplete core alias sort: module"sv);
          break;
        case CoreSort::Instance:
          spdlog::warn("incomplete core alias sort: instance"sv);
          break;
        }
      } else {
        spdlog::warn("incomplete alias target outer"sv);
      }
    } else if (std::holds_alternative<SortCase>(S)) {
      if (std::holds_alternative<AliasTargetExport>(T)) {
        auto &Exp = std::get<AliasTargetExport>(T);

        switch (std::get<SortCase>(S)) {
        case SortCase::Func: {
          auto *CInst = CompInst.getComponentInstance(Exp.getInstanceIdx());
          auto *FuncInst = CInst->findFuncExports(Exp.getName());
          CompInst.addFunctionInstance(FuncInst);
          break;
        }
        case SortCase::Value: // TODO: need real use cases to analysis how to
                              // implement these cases
          spdlog::warn("incomplete alias sort target export: value"sv);
          break;
        case SortCase::Type:
          spdlog::warn("incomplete alias sort target export: type"sv);
          break;
        case SortCase::Component:
          spdlog::warn("incomplete alias sort target export: component"sv);
          break;
        case SortCase::Instance:
          spdlog::warn("incomplete alias sort target export: instance"sv);
          break;
        }
      } else {
        auto &Out = std::get<AliasTargetOuter>(T);

        switch (std::get<SortCase>(S)) {
        case SortCase::Func: {
          auto *FuncInst = CompInst.getComponentInstance(Out.getComponent())
                               ->getCoreFunctionInstance(Out.getIndex());
          CompInst.addFunctionInstance(FuncInst);
          break;
        }
        case SortCase::Value: // TODO: need real use cases to analysis how to
                              // implement these cases
          spdlog::warn("incomplete alias sort outer: value"sv);
          break;
        case SortCase::Type:
          spdlog::warn("incomplete alias sort outer: type"sv);
          break;
        case SortCase::Component:
          spdlog::warn("incomplete alias sort outer: component"sv);
          break;
        case SortCase::Instance:
          spdlog::warn("incomplete alias sort outer: instance"sv);
          break;
        }
      }
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
