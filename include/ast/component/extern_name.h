// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace AST {
namespace Component {

struct ExternNameAnnotations {
  std::optional<std::string_view> getVersionSuffix() const noexcept {
    if (!VersionSuffix.has_value()) {
      return std::nullopt;
    }
    return std::string_view(*VersionSuffix);
  }

  std::optional<std::string> &getVersionSuffixMut() noexcept {
    return VersionSuffix;
  }

  const std::optional<std::string> &getVersionSuffixOpt() const noexcept {
    return VersionSuffix;
  }

  void clearVersionSuffix() noexcept { VersionSuffix.reset(); }

private:
  std::optional<std::string> VersionSuffix;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
