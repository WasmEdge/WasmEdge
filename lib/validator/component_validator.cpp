// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#include "validator/validator_component.h"

#include <variant>

namespace WasmEdge {
namespace Validator {

using namespace AST::Component;

Expect<void> Validator::validate(const AST::Component::Component &Comp) {
  spdlog::warn("component validation is not done yet.");

  for (auto &Sec : Comp.getSections()) {
    if (auto Res = std::visit(SectionVisitor{*this}, Sec); !Res) {
      return Unexpect(Res);
    }
  }

  return {};
}

} // namespace Validator
} // namespace WasmEdge
