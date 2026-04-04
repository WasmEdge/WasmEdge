// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "common/errcode.h"
#include "common/expected.h"
#include "common/variant.h"

#include <string_view>

namespace WasmEdge {
namespace Validator {

enum class ComponentNameKind {
  Invalid,
  Constructor,
  Method,
  Static,
  InterfaceType,
  Label,
  LockedDep,
  UnlockedDep,
  Url,
  Integrity
};

struct ConstructorDetail {
  std::string_view Label;
};
struct MethodDetail {
  std::string_view Resource;
  std::string_view Method;
};
struct StaticDetail {
  std::string_view Resource;
  std::string_view Method;
};
struct InterfaceDetail {
  std::string_view Namespace;
  std::string_view Package;
  std::string_view Interface;
  std::string_view Version;
};
struct LabelDetail {};
struct LockedDepDetail {
  std::string_view Namespace;
  std::string_view Package;
  std::string_view Version;
  std::string_view Integrity;
};
struct UnlockedDepDetail {
  std::string_view Namespace;
  std::string_view Package;
  std::string_view VersionRange;
};
struct UrlDetail {
  std::string_view Url;
  std::string_view Integrity;
};
struct IntegrityDetail {
  std::string_view Integrity;
};

using ComponentNameDetail =
    Variant<LabelDetail, ConstructorDetail, MethodDetail, StaticDetail,
            InterfaceDetail, LockedDepDetail, UnlockedDepDetail, UrlDetail,
            IntegrityDetail>;

class ComponentName {
  const std::string_view OriName;
  std::string_view NoTagName;
  ComponentNameKind Kind;
  ComponentNameDetail Detail;

  ComponentName(std::string_view Name)
      : OriName(Name), Kind(ComponentNameKind::Invalid) {}

public:
  static Expect<ComponentName> parse(std::string_view Name);

  ComponentNameKind getKind() const noexcept { return Kind; }
  std::string_view getOriginalName() const noexcept { return OriName; }
  std::string_view getNoTagName() const noexcept { return NoTagName; }
  const ComponentNameDetail &getDetail() const noexcept { return Detail; }
};

} // namespace Validator
} // namespace WasmEdge
