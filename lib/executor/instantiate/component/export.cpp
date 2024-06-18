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

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ExportSection &Sec) {
  using namespace WasmEdge::AST::Component;

  for (const auto &Export : Sec.getContent()) {
    auto &SortIndex = Export.getSortIndex();
    const auto &ExportName = Export.getName();

    auto Index = SortIndex.getSortIdx();
    const auto &S = SortIndex.getSort();
    if (std::holds_alternative<CoreSort>(S)) {
      switch (std::get<CoreSort>(S)) {
      case CoreSort::Module: {
        auto const *Mod = CompInst.getModuleInstance(Index);
        CompInst.addExport(ExportName, Mod);
        break;
      }
      default:
        // Any exported sortidx, which disallows core sorts other than core
        // module.
        spdlog::error("export core sort other than core module is invalid."sv);
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_CompExport));
        return Unexpect(ErrCode::Value::InvalidCoreSort);
      }
    } else {
      switch (std::get<SortCase>(S)) {
      case SortCase::Func: {
        auto *Func = CompInst.getFunctionInstance(Index);
        CompInst.addExport(ExportName, Func);
        break;
      }
      default: // TODO: There are four sorts haven't get handled
               // 1. Value
               // 2. Type
               // 3. Component
               // 4. Instance
        spdlog::warn("incomplete sort {}"sv,
                     static_cast<Byte>(std::get<SortCase>(S)));
        break;
      }
    }
  }

  return {};
}

} // namespace Executor
} // namespace WasmEdge
