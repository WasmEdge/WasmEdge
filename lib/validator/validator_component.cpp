// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "validator/validator_component.h"

#include <variant>

using namespace std::literals;

namespace WasmEdge {
namespace Validator {

using namespace AST::Component;

Expect<void> Validator::validate(const AST::Component::Component &Comp) {
  spdlog::warn("component validation is not done yet."sv);

  Context Ctx;

  for (auto &Sec : Comp.getSections()) {
    EXPECTED_TRY(std::visit(SectionVisitor{*this, Ctx}, Sec));
  }

  return {};
}

} // namespace Validator
} // namespace WasmEdge
