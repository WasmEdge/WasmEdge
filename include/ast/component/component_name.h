// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace AST {
namespace Component {

struct ComponentName {
  enum class Category : uint8_t { Plain, Scoped, Hash };
  Category Kind = Category::Plain;
  std::string Name;
  std::string Namespace;
  std::string Package;
  std::string Version;
  std::string Hash;

  ComponentName() = default;
  ComponentName(std::string_view N) { parse(N); }

  void parse(std::string_view N) {
    Name = std::string(N);
    Kind = Category::Plain;
    Namespace.clear();
    Package.clear();
    Version.clear();
    Hash.clear();

    if (auto AtPos = Name.find('@'); AtPos != std::string::npos) {
      Kind = Category::Scoped;
      Version = Name.substr(AtPos + 1);
      Name = Name.substr(0, AtPos);
    }

    if (auto ColonPos = Name.find(':'); ColonPos != std::string::npos) {
      if (Name.compare(0, 10, "integrity-") == 0) {
        Kind = Category::Hash;
        Hash = Name.substr(ColonPos + 1);
        Name = Name.substr(0, ColonPos);
      } else {
        Kind = Category::Scoped;
        Namespace = Name.substr(0, ColonPos);
        Package = Name.substr(ColonPos + 1);
      }
    }
  }

  std::string getFullName() const {
    if (Kind == Category::Plain) {
      return Name;
    }
    if (Kind == Category::Scoped) {
      if (!Namespace.empty()) {
        return Namespace + ":" + Package + "@" + Version;
      }
      return Name + "@" + Version;
    }
    if (Kind == Category::Hash) {
      return Name + ":" + Hash;
    }
    return Name;
  }
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
