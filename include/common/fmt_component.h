// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "ast/component/component_name.h"
#include "common/spdlog.h"

template <>
struct fmt::formatter<WasmEdge::AST::Component::ComponentName>
    : fmt::formatter<std::string_view> {
  auto format(const WasmEdge::AST::Component::ComponentName &C,
              fmt::format_context &Ctx) const {
    return fmt::formatter<std::string_view>::format(C.getFullName(), Ctx);
  }
};
