// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <optional>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

// Instantiate start section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::StartSection &StartSec) {
  const auto &Start = StartSec.getContent();
  auto ReportError = [](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Start));
    return E;
  };
  EXPECTED_TRY(
      auto *Func,
      CompInst.getFunction(Start.getFunctionIndex()).map_error(ReportError));
  std::vector<ComponentValVariant> Args;
  Args.reserve(Start.getArguments().size());
  for (const uint32_t Idx : Start.getArguments()) {
    EXPECTED_TRY(const auto *Val,
                 CompInst.getValue(Idx).map_error(ReportError));
    Args.push_back(*Val);
  }

  // The start function runs as a task under the instantiation entry.
  std::vector<ComponentValVariant> Results;
  EXPECTED_TRY(
      Runtime::Component::Task * T,
      liftCall(
          Func,
          [&Args]() -> Expect<std::vector<ComponentValVariant>> {
            return Args;
          },
          [&Results](std::optional<std::vector<ComponentValVariant>> Vals)
              -> Expect<void> {
            if (Vals.has_value()) {
              Results = std::move(*Vals);
            }
            return {};
          },
          nullptr, nullptr)
          .map_error(ReportError));
  Runtime::Component::Thread *Stack = T->getImplicitThread();
  EXPECTED_TRY(pump(
                   *CompInst.getRoot(),
                   [T, Stack]() {
                     return T->isResolved() || T->isAborted() ||
                            Stack == nullptr || Stack->isEnded();
                   },
                   T->isFuncTypeAsync() ? nullptr : Stack)
                   .map_error(ReportError));

  // The results become values of the instance.
  if (Results.size() != Start.getResult()) {
    spdlog::error(ErrCode::Value::ComponentUnexpectedType);
    spdlog::error(ErrInfo::InfoMismatch(static_cast<size_t>(Start.getResult()),
                                        Results.size()));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Start));
    return Unexpect(ErrCode::Value::ComponentUnexpectedType);
  }
  for (auto &Val : Results) {
    CompInst.addValue(std::move(Val));
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
