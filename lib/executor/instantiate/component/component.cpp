// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component::Component &Comp,
                      std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));

  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::Component::CoreModuleSection>) {
        // TODO: not to copy the module AST
        CompInst->addModule(Sec.getContent());
      } else if constexpr (std::is_same_v<T,
                                          AST::Component::ComponentSection>) {
        // TODO: not to copy the component AST
        CompInst->addComponent(Sec.getContent());
      } else if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
      } else {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }

  if (Name.has_value()) {
    StoreMgr.registerComponent(CompInst.get());
  }
  return CompInst;
}

} // namespace Executor
} // namespace WasmEdge
