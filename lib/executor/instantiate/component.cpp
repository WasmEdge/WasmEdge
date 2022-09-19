// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component &Comp,
                      std::optional<std::string_view> Name) {
  // TODO: component validation

  if (Name.has_value()) {
    const auto *FindModInst = StoreMgr.findComponent(Name.value());
    if (FindModInst != nullptr) {
      spdlog::error(ErrCode::ComponentNameConflict);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
      return Unexpect(ErrCode::ComponentNameConflict);
    }
  }

  std::unique_ptr<Runtime::Instance::ComponentInstance> CompInst;
  if (Name.has_value()) {
    CompInst =
        std::make_unique<Runtime::Instance::ComponentInstance>(Name.value());
  } else {
    CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");
  }

  for (auto &CoreInst : Comp.getCoreInstanceSection().getContent()) {
    instantiate(StoreMgr, Comp, CoreInst);
  }

  //  for (auto &CoreAlias : Comp.getCoreAliasSection().getContent()) {
  //    // TODO: instantiate
  //  }

  // Instantiate Core Types in Component Instance.
  for (auto &CoreType : Comp.getCoreTypeSection().getContent()) {
    // Copy param and return lists to module instance.
    CompInst->addCoreType(CoreType);
  }

  //
  //  for (auto &C : Comp.getComponentSection().getContent()) {
  //    auto Tmp = instantiate(StoreMgr, std::move(*C));
  //    // TODO: push Tmp into component
  //  }
  //
  //  for (auto &Inst : Comp.getInstanceSection().getContent()) {
  //    // TODO: instantiate
  //  }
  //
  //  for (auto &Alias : Comp.getAliasSection().getContent()) {
  //    // TODO: instantiate
  //  }

  for (auto &Ty : Comp.getTypeSection().getContent()) {
    CompInst->addType(Ty);
  }

  //  for (auto &Tmp : Comp.getCanonSection().getContent()) {
  //    // TODO: instantiate
  //  }
  //
  //  for (auto &Tmp : Comp.getStartSection().getContent()) {
  //    // TODO: instantiate
  //  }
  //
  //  for (auto &Tmp : Comp.getImportSection().getContent()) {
  //    // TODO: instantiate
  //  }
  //
  //  for (auto &Tmp : Comp.getExportSection().getContent()) {
  //    // TODO: instantiate
  //  }

  // For the named components, register it into the store.
  if (Name.has_value()) {
    StoreMgr.registerComponent(CompInst.get());
  }

  return CompInst;
}

Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiateComponent(Runtime::StoreManager &StoreMgr,
                               const AST::Component &Comp) {
  if (auto Res = instantiate(StoreMgr, Comp)) {
    return Res;
  } else {
    // If Statistics is enabled, then dump it here.
    // When there is an error happened, the following execution will not
    // execute.
    if (Stat) {
      Stat->dumpToLog(Conf);
    }
    return Unexpect(Res);
  }
}

} // namespace Executor
} // namespace WasmEdge
