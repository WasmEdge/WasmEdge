// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

// Walk the sections of one component. See executor.h.
Expect<void>
ComponentExecutor::instantiate(Component::Instantiator &Ctx,
                               const AST::Component::Component &Comp) {
  auto &CompInst = Ctx.inst();
  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
      } else if constexpr (std::is_same_v<T, AST::Component::ImportSection>) {
        // Only the import section depends on where the imports come from.
        EXPECTED_TRY(instantiate(Ctx, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      } else {
        EXPECTED_TRY(instantiate(CompInst, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }
  return {};
}

// Instantiate a root component instance. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                               const AST::Component::Component &Comp,
                               std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));
  // The instance cannot be entered until instantiation completes.
  Runtime::Instance::Component::ConcurrencyState::EnterGuard EnterG{
      CompInst->concurrency()};

  Component::Instantiator Ctx{StoreMgr, *CompInst};
  EXPECTED_TRY(instantiate(Ctx, Comp));

  if (Name.has_value()) {
    StoreMgr.registerInstance(CompInst.get());
  }
  return CompInst;
}

// Instantiate a nested component instance. See executor.h.
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
ComponentExecutor::instantiate(
    Runtime::Instance::Component::ImportManager &ImportMgr,
    const AST::Component::Component &Comp,
    const Runtime::Instance::ComponentInstance *Parent) {
  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");
  // Outer aliases resolve through the lexical parent, so wire it first.
  CompInst->setParent(Parent);
  // The instance cannot be entered until instantiation completes.
  Runtime::Instance::Component::ConcurrencyState::EnterGuard EnterG{
      CompInst->concurrency()};

  Component::Instantiator Ctx{ImportMgr, *CompInst};
  EXPECTED_TRY(instantiate(Ctx, Comp));

  return CompInst;
}

// Instantiate component section. See executor.h.
Expect<void> ComponentExecutor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::ComponentSection &CompSec) {
  CompInst.addComponent(CompSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
