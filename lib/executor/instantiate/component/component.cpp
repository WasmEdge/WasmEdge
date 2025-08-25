// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

// Instantiate component module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr,
                      const AST::Component::Component &Comp,
                      std::optional<std::string_view> Name) {
  auto CompInst =
      std::make_unique<Runtime::Instance::ComponentInstance>(Name.value_or(""));

  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
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

  if (Name.has_value()) {
    StoreMgr.registerComponent(CompInst.get());
  }
  return CompInst;
}

// Instantiate component module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ComponentInstance>>
Executor::instantiate(Runtime::Instance::ComponentImportManager &ImportMgr,
                      const AST::Component::Component &Comp) {
  auto CompInst = std::make_unique<Runtime::Instance::ComponentInstance>("");

  for (const auto &Section : Comp.getSections()) {
    auto Func = [&](auto &&Sec) -> Expect<void> {
      using T = std::decay_t<decltype(Sec)>;
      if constexpr (std::is_same_v<T, AST::CustomSection>) {
        // Do nothing to custom section.
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
