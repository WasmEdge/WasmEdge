// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"
#include "executor/executor.h"

#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {
// The instantiation context runs start functions and canon built-ins as an
// implicit synchronous task, so a blocking operation traps.
struct InstantiateTaskGuard {
  InstantiateTaskGuard(Component::AsyncRuntime &RtIn,
                       const Runtime::Instance::ComponentInstance *Inst)
      : Rt(RtIn) {
    Component::Task *T = Rt.newTask();
    T->Inst = Inst;
    T->CallerTask = Rt.currentTask();
    T->St = Component::Task::State::Started;
    Rt.pushNestedTask(T);
  }
  ~InstantiateTaskGuard() { Rt.popNestedTask(); }
  Component::AsyncRuntime &Rt;
};
} // namespace

// Walk the sections of one component. See
// "include/executor/component/executor.h".
Expect<void>
Component::Executor::instantiate(Component::Instantiator &Ctx,
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

// Instantiate a root component instance. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Component::Executor::instantiate(Runtime::Component::StoreManager &StoreMgr,
                                 const AST::Component::Component &Comp,
                                 std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));
  // The instance cannot be entered until instantiation completes.
  Runtime::Instance::Component::ConcurrencyState::EnterGuard EnterG{
      CompInst->concurrency()};
  InstantiateTaskGuard TaskGuard{AsyncRt, CompInst.get()};

  Component::Instantiator Ctx{StoreMgr, *CompInst};
  EXPECTED_TRY(instantiate(Ctx, Comp));

  if (Name.has_value()) {
    StoreMgr.registerInstance(CompInst.get());
  }
  return CompInst;
}

// Instantiate a nested component instance. See
// "include/executor/component/executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Component::Executor::instantiate(
    Runtime::Instance::Component::ImportManager &ImportMgr,
    const AST::Component::Component &Comp,
    const Runtime::Instance::ComponentInstance *Parent) {
  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");
  // The lexical parent must be wired before sections run: outer aliases
  // resolve through it during instantiation.
  CompInst->setParent(Parent);
  // The instance cannot be entered until instantiation completes.
  Runtime::Instance::Component::ConcurrencyState::EnterGuard EnterG{
      CompInst->concurrency()};
  InstantiateTaskGuard TaskGuard{AsyncRt, CompInst.get()};

  Component::Instantiator Ctx{ImportMgr, *CompInst};
  EXPECTED_TRY(instantiate(Ctx, Comp));

  return CompInst;
}

// Instantiate component section. See
// "include/executor/component/executor.h".
Expect<void> Component::Executor::instantiate(
    Runtime::Instance::ComponentInstance &CompInst,
    const AST::Component::ComponentSection &CompSec) {
  CompInst.addComponent(CompSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
