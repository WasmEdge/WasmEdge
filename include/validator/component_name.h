// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "common/errcode.h"
#include "common/expected.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Validator {
namespace Component {

/// A parsed extern name of a component import or export.
/// exportname ::= <plainname> | <interfacename>
/// importname ::= <exportname> | <depname> | <urlname> | <hashname>
class ExternName {
public:
  enum class Kind : uint8_t {
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

  /// Fragments of the parsed name, all views into the input. The kind decides
  /// which ones are set. The rest stay empty.
  struct Detail {
    std::string_view Resource;     // Constructor, Method, Static
    std::string_view Method;       // Method, Static
    std::string_view Namespace;    // InterfaceType, LockedDep, UnlockedDep
    std::string_view Package;      // InterfaceType, LockedDep, UnlockedDep
    std::string_view Interface;    // InterfaceType
    std::string_view Version;      // InterfaceType, LockedDep
    std::string_view VersionRange; // UnlockedDep
    std::string_view Url;          // Url
    std::string_view Integrity;    // LockedDep, Url, Integrity
  };

  /// Returns true if Input is a valid kebab-case label per the Component Model
  /// spec: `label ::= <first-fragment> ( '-' <fragment> )*`
  static bool isKebabString(std::string_view Input) noexcept;

  /// Parses Name into this object, which stays Invalid on failure.
  Expect<void> parse(std::string_view Name) noexcept;

  Kind getKind() const noexcept { return NameKind; }
  std::string_view getOriginalName() const noexcept { return OriName; }
  /// The name with its annotation stripped; empty unless the name carries a
  /// `[constructor]`, `[method]`, or `[static]` tag.
  std::string_view getNoTagName() const noexcept { return NoTagName; }
  const Detail &getDetail() const noexcept { return NameDetail; }
  /// 🔗 The `versionsuffix` attribute. The name has to carry an
  /// `interfaceversion` in `canonversion` form, and the two concatenated
  /// have to be a valid semver.
  Expect<void> checkVersionSuffix(std::string_view Suffix) const noexcept;

private:
  // pkgpath ::= <namespace> <words>
  struct PkgPath {
    std::string_view Namespace;
    std::string_view Package;
  };

  // Cursor primitives, all consuming from Rest.
  bool tryRead(std::string_view Prefix) noexcept;
  bool readUntil(char Delim, std::string_view &Output) noexcept;
  std::string_view readLabelChars() noexcept;

  // One member per production. Each one consumes the rest of the input after
  // its leading tag, then fills in NameKind and Detail.
  Expect<void> parsePlainName() noexcept;
  Expect<void> parseUnlockedDep() noexcept;
  Expect<void> parseLockedDep() noexcept;
  Expect<void> parseUrlName() noexcept;
  Expect<void> parseHashName() noexcept;
  Expect<void> parseInterfaceName() noexcept;
  Expect<PkgPath> parsePkgPath(std::string_view StopChars) noexcept;
  Expect<std::string_view> parseIntegrityBody() noexcept;
  Expect<std::string_view> parseIntegritySuffix() noexcept;

  // Grammar checks over an explicit substring. A semver scanner returns its
  // granular code unlogged, and the caller picks the diagnostic.
  Expect<void> checkWordsLabel(std::string_view Label,
                               std::string_view What) const noexcept;
  Expect<void> checkVersionRange(std::string_view Body) const noexcept;
  bool isCanonVersion(std::string_view V) const noexcept;
  Expect<void> scanSemver(std::string_view V) const noexcept;
  Expect<void> scanSemverIdentifiers(std::string_view Idents,
                                     bool CheckLeadingZeros) const noexcept;
  Expect<void> checkIntegrityMetadata(std::string_view Input) const noexcept;

  std::string_view OriName;
  std::string_view NoTagName;
  std::string_view Rest;
  Kind NameKind = Kind::Invalid;
  Detail NameDetail;
};

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
