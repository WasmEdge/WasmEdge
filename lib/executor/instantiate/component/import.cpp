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
                      const ImportSection &Sec) {
  for (auto &ImportStatement : Sec.getContent()) {
    auto &Desc = ImportStatement.getDesc();
    if (std::holds_alternative<DescTypeIndex>(Desc)) {
      auto &TypeIndex = std::get<DescTypeIndex>(Desc);

      // TODO: get type via index `TypeIndex.getIndex()`, then use the type to
      // check the imported thing

      switch (TypeIndex.getKind()) {
      case IndexKind::CoreType: // TODO
        spdlog::warn("incomplete import core type"sv);
        break;
      case IndexKind::FuncType: // TODO
        spdlog::warn("incomplete import function"sv);
        break;
      case IndexKind::ComponentType: // TODO
        spdlog::warn("incomplete import component"sv);
        break;
      case IndexKind::InstanceType:
        auto CompName = ImportStatement.getName();
        const auto *ImportedCompInst = StoreMgr.findComponent(CompName);
        if (unlikely(ImportedCompInst == nullptr)) {
          spdlog::error(ErrCode::Value::UnknownImport);
          spdlog::error("component name: {}"sv, CompName);
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_CompImport));
          return Unexpect(ErrCode::Value::UnknownImport);
        }
        CompInst.addComponentInstance(ImportedCompInst);
        break;
      }
    } else if (std::holds_alternative<TypeBound>(Desc)) {
      // TODO: import a type or resource
      spdlog::warn("incomplete import type bound"sv);
    } else if (std::holds_alternative<ValueType>(Desc)) {
      // TODO: import a value and check its type, this is a new concept, so a
      // plugin of component should allow this
      spdlog::warn("incomplete import value type"sv);
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
