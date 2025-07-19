// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

Expect<void>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ImportSection &ImportSec) {
  for (const auto &Import : ImportSec.getContent()) {
    const auto &Desc = Import.getDesc();
    switch (Desc.getDescType()) {
    case AST::Component::ExternDesc::DescType::CoreType:
    case AST::Component::ExternDesc::DescType::FuncType:
    case AST::Component::ExternDesc::DescType::ValueBound:
    case AST::Component::ExternDesc::DescType::TypeBound:
    case AST::Component::ExternDesc::DescType::ComponentType:
      spdlog::error(ErrCode::Value::ComponentNotImplInstantiate);
      spdlog::error("    incomplete import {} desc types"sv, Import.getName());
      return Unexpect(ErrCode::Value::ComponentNotImplInstantiate);
    case AST::Component::ExternDesc::DescType::InstanceType: {
      auto CompName = Import.getName();
      const auto *ImportedCompInst = StoreMgr.findComponent(CompName);
      if (unlikely(ImportedCompInst == nullptr)) {
        spdlog::error(ErrCode::Value::UnknownImport);
        spdlog::error("    component name: {}"sv, CompName);
        return Unexpect(ErrCode::Value::UnknownImport);
      }
      CompInst.addComponentInstance(ImportedCompInst);
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
