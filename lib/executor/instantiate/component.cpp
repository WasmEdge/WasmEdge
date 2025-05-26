// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Instantiate module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component::Component &Comp,
                      std::optional<std::string_view> Name) {
  using namespace AST::Component;

  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>(
      Name.value_or(""sv));

  for (auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
      } else if constexpr (std::is_same_v<T, CoreModuleSection>) {
        CompInst->addModule(Sec.getContent());
      } else if constexpr (std::is_same_v<T, ComponentSection>) {
        CompInst->addComponent(Sec.getContent());
      } else if constexpr (std::is_same_v<T, CoreInstanceSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, InstanceSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, ImportSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, CoreTypeSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, TypeSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, StartSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, CanonSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, AliasSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      } else if constexpr (std::is_same_v<T, ExportSection>) {
        EXPECTED_TRY(instantiate(StoreMgr, *CompInst, Sec));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }

  StoreMgr.registerComponent(CompInst.get());

  return CompInst;
}

} // namespace Executor
} // namespace WasmEdge
