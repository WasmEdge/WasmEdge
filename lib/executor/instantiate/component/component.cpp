// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/spdlog.h"

#include <string_view>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;

namespace {
// The instantiation context runs start functions and canon built-ins as an
// implicit synchronous task of the instance being built: blocking
// operations trap "cannot block a synchronous task before returning".
struct InstantiateTaskGuard {
  InstantiateTaskGuard(AsyncRuntime &RtIn,
                       const Runtime::Instance::ComponentInstance *Inst)
      : Rt(RtIn) {
    ComponentTask *T = Rt.newTask();
    T->Inst = Inst;
    T->CallerTask = Rt.currentTask();
    T->St = ComponentTask::State::Started;
    Rt.pushNestedTask(T);
  }
  ~InstantiateTaskGuard() { Rt.popNestedTask(); }
  AsyncRuntime &Rt;
};
} // namespace

// Instantiate component module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component::Component &Comp,
                      std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));
  // The instance cannot be entered until instantiation completes.
  CompInst->setEntered(true);
  InstantiateTaskGuard TaskGuard{AsyncRt, CompInst.get()};

  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
      } else if constexpr (std::is_same_v<T, AST::Component::ValueSection>) {
        EXPECTED_TRY(instantiate(*CompInst, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      } else if constexpr (std::is_same_v<T, AST::Component::ImportSection>) {
        EXPECTED_TRY(
            instantiate(StoreMgr, *CompInst, Sec).map_error([](auto E) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
              return E;
            }));
      } else {
        EXPECTED_TRY(instantiate(*CompInst, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }

  CompInst->setEntered(false);
  if (Name.has_value()) {
    StoreMgr.registerComponent(CompInst.get());
  }
  return CompInst;
}

// Instantiate component module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::Instance::ComponentImportManager &ImportMgr,
                      const AST::Component::Component &Comp,
                      const Runtime::Instance::ComponentInstance *Parent) {
  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");
  // The lexical parent must be wired before sections run: outer aliases
  // resolve through it during instantiation.
  CompInst->setParent(Parent);
  // The instance cannot be entered until instantiation completes.
  CompInst->setEntered(true);
  InstantiateTaskGuard TaskGuard{AsyncRt, CompInst.get()};

  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
      } else if constexpr (std::is_same_v<T, AST::Component::ValueSection>) {
        EXPECTED_TRY(instantiate(*CompInst, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      } else if constexpr (std::is_same_v<T, AST::Component::ImportSection>) {
        EXPECTED_TRY(
            instantiate(ImportMgr, *CompInst, Sec).map_error([](auto E) {
              spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
              return E;
            }));
      } else {
        EXPECTED_TRY(instantiate(*CompInst, Sec).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
          return E;
        }));
      }
      return {};
    };
    EXPECTED_TRY(std::visit(Func, Section));
  }
  CompInst->setEntered(false);
  return CompInst;
}

// Instantiate component section. See "include/executor/Executor.h".
Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::ComponentSection &CompSec) {
  CompInst.addComponent(CompSec.getContent());
  return {};
}

} // namespace Executor
} // namespace WasmEdge
