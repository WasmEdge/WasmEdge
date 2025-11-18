// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"

#include <cctype>
#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Validator {

namespace ComponentNameParser {
bool isKebabString(std::string_view input);
bool isLowercaseKebabString(std::string_view input);
bool isEOF(std::string_view input);

bool readUntil(std::string_view &input, char delim, std::string_view &output);

bool tryRead(std::string_view prefix, std::string_view &name);
bool tryReadKebab(std::string_view &input, std::string_view &output);

}; // namespace ComponentNameParser

enum class ComponentNameKind {
  Invalid,
  Constructor,
  Method,
  Static,
  InterfaceType,
  Label
};

class ComponentName {
  const std::string_view Name;
  ComponentNameKind Kind;
  union Details {
    struct {
      // [constructor] <Label>
      std::string_view Label;
    } Constructor;
    struct {
      // [method] <Resource> '.' <Method>
      // [static] <Resource> '.' <Method>
      std::string_view Resource;
      std::string_view Method;
    } Method, Static;
    struct {
      // <Namespace> : <Package> / <interface> / <projection> @ <version>
      std::string_view Namespace;
      std::string_view Package;
      std::string_view Interface;
      std::string_view Projection;
      std::string_view Version;
    } Interface;
  } Detail;

  void parse();

public:
  ComponentName(std::string_view Name)
      : Name(Name), Kind(ComponentNameKind::Invalid), Detail({}) {
    parse();
  }

  ComponentNameKind getKind() const { return Kind; }
  std::string_view getName() const { return Name; }
  Details getDetails() const { return Detail; }
};

} // namespace Validator
} // namespace WasmEdge
