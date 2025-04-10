// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "validator/validator_component.h"
#include "common/spdlog.h"

#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;
using namespace AST::Component;

Expect<void> Validator::validate(const AST::Component::Component &Comp) {
  spdlog::warn("Component Model Validation is in active development."sv);

  Context Ctx;
  ComponentContextGuard guard(Comp, Ctx);
  for (auto &Sec : Comp.getSections()) {
    EXPECTED_TRY(std::visit(SectionVisitor{*this, Ctx}, Sec));
  }

  return {};
}

} // namespace Validator
} // namespace WasmEdge
