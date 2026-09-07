// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "validator/component_name.h"

#include "spdlog/spdlog.h"

#include <cctype>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Validator {
namespace Component {

using namespace std::literals;

// label          ::= <first-fragment> ( '-' <fragment> )*
// first-fragment ::= <first-word> | <first-acronym>
// first-word     ::= [a-z] [0-9a-z]*
// first-acronym  ::= [A-Z] [0-9A-Z]*
// fragment       ::= <word> | <acronym>
// word           ::= [0-9a-z]+
// acronym        ::= [0-9A-Z]+
bool ExternName::isKebabString(std::string_view Input) noexcept {
  bool IsFirstPart = true;
  bool Uppercase = false;
  bool Lowercase = false;
  bool Digit = false;

  for (char C : Input) {
    if (islower(static_cast<unsigned char>(C))) {
      if (Uppercase) {
        return false;
      }
      Lowercase = true;
    } else if (isupper(static_cast<unsigned char>(C))) {
      if (Lowercase) {
        return false;
      }
      Uppercase = true;
    } else if (isdigit(static_cast<unsigned char>(C))) {
      if (IsFirstPart && !(Uppercase || Lowercase)) {
        return false;
      }
      Digit = true;
    } else if (C == '-') {
      if (Uppercase || Lowercase || Digit) {
        IsFirstPart = false;
        Uppercase = false;
        Lowercase = false;
        Digit = false;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  return Input.size() > 0 && Input.back() != '-';
}

Expect<void> ExternName::parse(std::string_view Name) noexcept {
  OriName = Name;
  NoTagName = {};
  NameKind = Kind::Invalid;
  NameDetail = {};
  Rest = Name;

  if (!Rest.empty() && Rest[0] == '[') {
    return parsePlainName();
  }

  if (tryRead("unlocked-dep="sv)) {
    return parseUnlockedDep();
  }
  if (tryRead("locked-dep="sv)) {
    return parseLockedDep();
  }
  if (tryRead("url="sv)) {
    return parseUrlName();
  }
  if (tryRead("integrity="sv)) {
    return parseHashName();
  }
  if (Rest.find(':') != std::string_view::npos) {
    return parseInterfaceName();
  }
  return parsePlainName();
}

// Consumes Prefix from Rest, or leaves Rest untouched and returns false.
bool ExternName::tryRead(std::string_view Prefix) noexcept {
  if (Prefix.size() > Rest.size()) {
    return false;
  }
  if (Prefix != Rest.substr(0, Prefix.size())) {
    return false;
  }

  Rest.remove_prefix(Prefix.size());
  return true;
}

// Consumes up to and including Delim, and reports the text before it.
bool ExternName::readUntil(char Delim, std::string_view &Output) noexcept {
  size_t Pos = Rest.find(Delim);
  if (Pos == Rest.npos) {
    return false;
  }

  Output = Rest.substr(0, Pos);
  Rest.remove_prefix(Pos + 1);
  return true;
}

// Consumes and returns the leading run of label characters ([0-9A-Za-z-]).
std::string_view ExternName::readLabelChars() noexcept {
  size_t Pos = 0;
  while (Pos < Rest.size() &&
         (isalnum(static_cast<unsigned char>(Rest[Pos])) || Rest[Pos] == '-')) {
    Pos++;
  }
  std::string_view Output = Rest.substr(0, Pos);
  Rest.remove_prefix(Pos);
  return Output;
}

// plainname ::= <label>
//             | '[constructor]' <label>
//             | '[method]' <label> '.' <label>
//             | '[static]' <label> '.' <label>
Expect<void> ExternName::parsePlainName() noexcept {
  if (tryRead("[constructor]"sv)) {
    if (!isKebabString(Rest)) {
      spdlog::error(ErrCode::Value::ComponentNameNotKebab);
      spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                    Rest);
      return Unexpect(ErrCode::Value::ComponentNameNotKebab);
    }
    NoTagName = Rest;
    NameDetail.Resource = Rest;
    NameKind = Kind::Constructor;
    return {};
  }

  // Once a '[method]' or '[static]' tag matched, the name must contain a
  // '.' separating two kebab labels.
  auto readResourceAndLabel = [this](std::string_view &Resource,
                                     std::string_view &Label) -> Expect<void> {
    NoTagName = Rest;
    if (!readUntil('.', Resource)) {
      spdlog::error(ErrCode::Value::NameFailedToFindDot);
      spdlog::error("    Component name: failed to find `.` character"sv);
      return Unexpect(ErrCode::Value::NameFailedToFindDot);
    }
    if (!isKebabString(Resource)) {
      spdlog::error(ErrCode::Value::ComponentNameNotKebab);
      spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                    Resource);
      return Unexpect(ErrCode::Value::ComponentNameNotKebab);
    }
    if (!isKebabString(Rest)) {
      spdlog::error(ErrCode::Value::ComponentNameNotKebab);
      spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                    Rest);
      return Unexpect(ErrCode::Value::ComponentNameNotKebab);
    }
    Label = Rest;
    return {};
  };

  if (tryRead("[method]"sv)) {
    std::string_view Resource, Label;
    EXPECTED_TRY(readResourceAndLabel(Resource, Label));
    NameDetail.Resource = Resource;
    NameDetail.Method = Label;
    NameKind = Kind::Method;
    return {};
  }

  if (tryRead("[static]"sv)) {
    std::string_view Resource, Label;
    EXPECTED_TRY(readResourceAndLabel(Resource, Label));
    NameDetail.Resource = Resource;
    NameDetail.Method = Label;
    NameKind = Kind::Static;
    return {};
  }

  if (!Rest.empty() && Rest[0] == '[') {
    spdlog::error(ErrCode::Value::ComponentInvalidName);
    spdlog::error("    Component name: unknown annotation"sv);
    return Unexpect(ErrCode::Value::ComponentInvalidName);
  }

  if (!isKebabString(Rest)) {
    spdlog::error(ErrCode::Value::ComponentNameNotKebab);
    spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                  Rest);
    return Unexpect(ErrCode::Value::ComponentNameNotKebab);
  }
  NameKind = Kind::Label;
  return {};
}

// depname      ::= 'unlocked-dep=<' <pkgnamequery> '>'
// pkgnamequery ::= <pkgpath> <verrange>?
// verrange     ::= '@*' | '@{' <verlower> '}' | '@{' <verupper> '}'
//                | '@{' <verlower> ' ' <verupper> '}'
Expect<void> ExternName::parseUnlockedDep() noexcept {
  if (!tryRead("<"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedOpenAngle);
    spdlog::error("    Component name: expected `<` after unlocked-dep="sv);
    return Unexpect(ErrCode::Value::NameExpectedOpenAngle);
  }

  EXPECTED_TRY(auto Path, parsePkgPath("@>"sv));

  std::string_view VersionRange;
  if (!Rest.empty() && Rest[0] == '@') {
    auto VerStart = Rest;
    Rest.remove_prefix(1);

    if (!Rest.empty() && Rest[0] == '*') {
      Rest.remove_prefix(1);
    } else if (!Rest.empty() && Rest[0] == '{') {
      size_t ClosePos = Rest.find('}');
      if (ClosePos == Rest.npos) {
        spdlog::error(ErrCode::Value::ComponentInvalidName);
        spdlog::error(
            "    Component name: expected `}` in unlocked-dep version range"sv);
        return Unexpect(ErrCode::Value::ComponentInvalidName);
      }
      EXPECTED_TRY(checkVersionRange(Rest.substr(1, ClosePos - 1)));
      Rest.remove_prefix(ClosePos + 1);
    } else {
      spdlog::error(ErrCode::Value::NameExpectedOpenBrace);
      spdlog::error(
          "    Component name: expected `{` at start of version range"sv);
      return Unexpect(ErrCode::Value::NameExpectedOpenBrace);
    }
    VersionRange = VerStart.substr(0, VerStart.size() - Rest.size());
  }

  if (!tryRead(">"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedCloseAngle);
    spdlog::error("    Component name: expected `>` closing unlocked-dep"sv);
    return Unexpect(ErrCode::Value::NameExpectedCloseAngle);
  }
  if (!Rest.empty()) {
    spdlog::error(ErrCode::Value::NameTrailingCharacters);
    spdlog::error(
        "    Component name: trailing characters found after unlocked-dep"sv);
    return Unexpect(ErrCode::Value::NameTrailingCharacters);
  }

  NameDetail.Namespace = Path.Namespace;
  NameDetail.Package = Path.Package;
  NameDetail.VersionRange = VersionRange;
  NameKind = Kind::UnlockedDep;
  return {};
}

// depname ::= 'locked-dep=<' <pkgname> '>' ( ',' <hashname> )?
// pkgname ::= <pkgpath> ( '@' <valid semver> )?
Expect<void> ExternName::parseLockedDep() noexcept {
  if (!tryRead("<"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedOpenAngle);
    spdlog::error("    Component name: expected `<` after locked-dep="sv);
    return Unexpect(ErrCode::Value::NameExpectedOpenAngle);
  }

  EXPECTED_TRY(auto Path, parsePkgPath("@>"sv));

  std::string_view Version;
  if (!Rest.empty() && Rest[0] == '@') {
    Rest.remove_prefix(1);
    size_t VerEnd = Rest.find('>');
    if (VerEnd == Rest.npos) {
      spdlog::error(ErrCode::Value::NameExpectedCloseAngle);
      spdlog::error(
          "    Component name: expected `>` after version in locked-dep"sv);
      return Unexpect(ErrCode::Value::NameExpectedCloseAngle);
    }
    Version = Rest.substr(0, VerEnd);
    Rest.remove_prefix(VerEnd);
    if (!scanSemver(Version)) {
      spdlog::error(ErrCode::Value::NameNotValidSemver);
      spdlog::error(
          "    Component name: locked-dep version '{}' is not a valid semver"sv,
          Version);
      return Unexpect(ErrCode::Value::NameNotValidSemver);
    }
  }

  if (!tryRead(">"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedCloseAngle);
    spdlog::error("    Component name: expected `>` closing locked-dep"sv);
    return Unexpect(ErrCode::Value::NameExpectedCloseAngle);
  }

  EXPECTED_TRY(auto Integrity, parseIntegritySuffix());

  NameDetail.Namespace = Path.Namespace;
  NameDetail.Package = Path.Package;
  NameDetail.Version = Version;
  NameDetail.Integrity = Integrity;
  NameKind = Kind::LockedDep;
  return {};
}

// urlname     ::= 'url=<' <nonbrackets> '>' ( ',' <hashname> )?
// nonbrackets ::= [^<>]*
Expect<void> ExternName::parseUrlName() noexcept {
  if (!tryRead("<"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedOpenAngle);
    spdlog::error("    Component name: expected `<` after url="sv);
    return Unexpect(ErrCode::Value::NameExpectedOpenAngle);
  }

  size_t ClosePos = Rest.find('>');
  if (ClosePos == Rest.npos) {
    spdlog::error(ErrCode::Value::NameFailedToFindCloseAngle);
    spdlog::error("    Component name: failed to find `>` closing url"sv);
    return Unexpect(ErrCode::Value::NameFailedToFindCloseAngle);
  }

  std::string_view Url = Rest.substr(0, ClosePos);
  if (Url.find('<') != Url.npos) {
    spdlog::error(ErrCode::Value::NameUrlContainsOpenAngle);
    spdlog::error("    Component name: url cannot contain `<`"sv);
    return Unexpect(ErrCode::Value::NameUrlContainsOpenAngle);
  }
  Rest.remove_prefix(ClosePos + 1);

  EXPECTED_TRY(auto Integrity, parseIntegritySuffix());

  NameDetail.Url = Url;
  NameDetail.Integrity = Integrity;
  NameKind = Kind::Url;
  return {};
}

// hashname ::= 'integrity=<' <integrity-metadata> '>'
Expect<void> ExternName::parseHashName() noexcept {
  if (!tryRead("<"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedOpenAngle);
    spdlog::error("    Component name: expected `<` after integrity="sv);
    return Unexpect(ErrCode::Value::NameExpectedOpenAngle);
  }

  EXPECTED_TRY(auto Integrity, parseIntegrityBody());

  NameDetail.Integrity = Integrity;
  NameKind = Kind::Integrity;
  return {};
}

// interfacename    ::= <namespace> <label> <projection> <interfaceversion>?
// namespace        ::= <words> ':'
// projection       ::= '/' <label>
// interfaceversion ::= '@' <valid semver> | '@' <canonversion>
Expect<void> ExternName::parseInterfaceName() noexcept {
  size_t ColonPos = Rest.find(':');
  std::string_view Namespace = Rest.substr(0, ColonPos);
  Rest.remove_prefix(ColonPos + 1);
  EXPECTED_TRY(checkWordsLabel(Namespace, "namespace"sv));

  std::string_view Package = readLabelChars();
  EXPECTED_TRY(checkWordsLabel(Package, "package"sv));

  // Nested namespaces (`a:b:c/d`) are feature-gated. Only a projection can
  // follow the package name.
  if (Rest.empty() || Rest[0] != '/') {
    spdlog::error(ErrCode::Value::NameExpectedSlashAfterPackage);
    spdlog::error("    Component name: expected `/` after package name"sv);
    return Unexpect(ErrCode::Value::NameExpectedSlashAfterPackage);
  }
  Rest.remove_prefix(1);

  std::string_view Interface = readLabelChars();
  if (!isKebabString(Interface)) {
    spdlog::error(ErrCode::Value::ComponentNameNotKebab);
    spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                  Interface);
    return Unexpect(ErrCode::Value::ComponentNameNotKebab);
  }

  // Nested projections (`a:b/c/d`) are feature-gated. Only a version can
  // follow the projection label.
  if (!Rest.empty() && Rest[0] != '@') {
    spdlog::error(ErrCode::Value::NameTrailingCharacters);
    spdlog::error(
        "    Component name: trailing characters found after projection"sv);
    return Unexpect(ErrCode::Value::NameTrailingCharacters);
  }

  std::string_view Version;
  if (!Rest.empty()) {
    Rest.remove_prefix(1);
    Version = Rest;
    if (!isCanonVersion(Version)) {
      if (auto Res = scanSemver(Version); !Res) {
        spdlog::error(Res.error().getEnum());
        spdlog::error("    Component name: version '{}' is not valid"sv,
                      Version);
        return Unexpect(Res);
      }
    }
  }

  NameDetail.Namespace = Namespace;
  NameDetail.Package = Package;
  NameDetail.Interface = Interface;
  NameDetail.Version = Version;
  NameKind = Kind::InterfaceType;
  return {};
}

// Parses 'namespace:package', stopping at the delimiters in StopChars.
Expect<ExternName::PkgPath>
ExternName::parsePkgPath(std::string_view StopChars) noexcept {
  size_t ColonPos = Rest.find(':');
  size_t StopPos = Rest.find_first_of(StopChars);
  if (ColonPos == Rest.npos || (StopPos != Rest.npos && StopPos < ColonPos)) {
    // No namespace delimiter: diagnose the leading label run. This catches
    // inputs like `<`, `<>`, or a stray label without `:`.
    std::string_view Label = readLabelChars();
    EXPECTED_TRY(checkWordsLabel(Label, "namespace"sv));
    spdlog::error(ErrCode::Value::ComponentInvalidName);
    spdlog::error("    Component name: expected `:` after namespace"sv);
    return Unexpect(ErrCode::Value::ComponentInvalidName);
  }

  std::string_view Namespace = Rest.substr(0, ColonPos);
  Rest.remove_prefix(ColonPos + 1);
  EXPECTED_TRY(checkWordsLabel(Namespace, "namespace"sv));

  size_t PkgEnd = Rest.find_first_of(StopChars);
  std::string_view Package =
      (PkgEnd == Rest.npos) ? Rest : Rest.substr(0, PkgEnd);
  Rest.remove_prefix(Package.size());
  EXPECTED_TRY(checkWordsLabel(Package, "package"sv));
  if (PkgEnd == std::string_view::npos) {
    // The package name ran to end of input without a closing delimiter.
    spdlog::error(ErrCode::Value::NameExpectedCloseAngle);
    spdlog::error("    Component name: expected `>` after package path"sv);
    return Unexpect(ErrCode::Value::NameExpectedCloseAngle);
  }

  return PkgPath{Namespace, Package};
}

// Parses the `<integrity-metadata> '>'` body, which must end the name, and
// returns the metadata.
Expect<std::string_view> ExternName::parseIntegrityBody() noexcept {
  std::string_view Data;
  if (!readUntil('>', Data)) {
    spdlog::error(ErrCode::Value::NameFailedToFindCloseAngle);
    spdlog::error("    Component name: failed to find `>` closing integrity"sv);
    return Unexpect(ErrCode::Value::NameFailedToFindCloseAngle);
  }
  EXPECTED_TRY(checkIntegrityMetadata(Data));
  if (!Rest.empty()) {
    spdlog::error(ErrCode::Value::NameTrailingCharacters);
    spdlog::error(
        "    Component name: trailing characters found after integrity"sv);
    return Unexpect(ErrCode::Value::NameTrailingCharacters);
  }
  return Data;
}

// Parse the optional ',' <hashname> suffix. An exhausted input gives no
// integrity metadata.
Expect<std::string_view> ExternName::parseIntegritySuffix() noexcept {
  if (Rest.empty()) {
    return std::string_view{};
  }
  if (Rest[0] != ',') {
    spdlog::error(ErrCode::Value::NameTrailingCharacters);
    spdlog::error("    Component name: trailing characters found"sv);
    return Unexpect(ErrCode::Value::NameTrailingCharacters);
  }
  Rest.remove_prefix(1);
  if (!tryRead("integrity=<"sv)) {
    spdlog::error(ErrCode::Value::NameExpectedIntegrity);
    spdlog::error("    Component name: expected `integrity=<` after `,`"sv);
    return Unexpect(ErrCode::Value::NameExpectedIntegrity);
  }
  return parseIntegrityBody();
}

// words ::= <first-word> ( '-' <word> )*
// Validate a namespace or package label. A non-kebab label is "not in kebab
// case". A kebab label with uppercase is "not lowercase".
Expect<void> ExternName::checkWordsLabel(std::string_view Label,
                                         std::string_view What) const noexcept {
  if (!isKebabString(Label)) {
    spdlog::error(ErrCode::Value::ComponentNameNotKebab);
    spdlog::error("    Component name: label '{}' is not in kebab case"sv,
                  Label);
    return Unexpect(ErrCode::Value::ComponentNameNotKebab);
  }
  // A kebab label already has the `<words>` shape, so only case is left.
  for (char C : Label) {
    if (isupper(static_cast<unsigned char>(C))) {
      spdlog::error(ErrCode::Value::ComponentPackageNameNotLowercase);
      spdlog::error("    Component name: {} '{}' is not lowercase"sv, What,
                    Label);
      return Unexpect(ErrCode::Value::ComponentPackageNameNotLowercase);
    }
  }
  return {};
}

// verrange body ::= <verlower> | <verupper> | <verlower> ' ' <verupper>
// verlower      ::= '>=' <valid semver>
// verupper      ::= '<' <valid semver>
Expect<void>
ExternName::checkVersionRange(std::string_view Body) const noexcept {
  if (Body.substr(0, 2) == ">="sv) {
    Body.remove_prefix(2);
    size_t SpacePos = Body.find(' ');
    std::string_view Lower =
        (SpacePos == Body.npos) ? Body : Body.substr(0, SpacePos);
    if (!scanSemver(Lower)) {
      spdlog::error(ErrCode::Value::NameNotValidSemver);
      spdlog::error(
          "    Component name: version range lower bound '{}' is not valid"sv,
          Lower);
      return Unexpect(ErrCode::Value::NameNotValidSemver);
    }
    if (SpacePos == Body.npos) {
      return {};
    }
    Body.remove_prefix(SpacePos + 1);
    if (Body.substr(0, 1) != "<"sv) {
      spdlog::error(ErrCode::Value::NameExpectedOpenAngle);
      spdlog::error(
          "    Component name: expected `<` before version range upper bound"sv);
      return Unexpect(ErrCode::Value::NameExpectedOpenAngle);
    }
    Body.remove_prefix(1);
    if (!scanSemver(Body)) {
      spdlog::error(ErrCode::Value::NameNotValidSemver);
      spdlog::error(
          "    Component name: version range upper bound '{}' is not valid"sv,
          Body);
      return Unexpect(ErrCode::Value::NameNotValidSemver);
    }
    return {};
  }

  if (Body.substr(0, 1) == "<"sv) {
    Body.remove_prefix(1);
    if (!scanSemver(Body)) {
      spdlog::error(ErrCode::Value::NameNotValidSemver);
      spdlog::error(
          "    Component name: version range upper bound '{}' is not valid"sv,
          Body);
      return Unexpect(ErrCode::Value::NameNotValidSemver);
    }
    return {};
  }

  spdlog::error(ErrCode::Value::NameExpectedVersionRangeOp);
  spdlog::error(
      "    Component name: expected `>=` or `<` at start of version range"sv);
  return Unexpect(ErrCode::Value::NameExpectedVersionRangeOp);
}

// semversuffix ::= [0-9A-Za-z.+-]* 🔗
Expect<void>
ExternName::checkVersionSuffix(std::string_view Suffix) const noexcept {
  for (char C : Suffix) {
    if (!isalnum(static_cast<unsigned char>(C)) && C != '.' && C != '+' &&
        C != '-') {
      spdlog::error(ErrCode::Value::ComponentVersionSuffixInvalid);
      spdlog::error("    `versionsuffix` `{}` is not a semver suffix"sv,
                    Suffix);
      return Unexpect(ErrCode::Value::ComponentVersionSuffixInvalid);
    }
  }
  if (NameKind != Kind::InterfaceType || NameDetail.Version.empty() ||
      !isCanonVersion(NameDetail.Version)) {
    spdlog::error(ErrCode::Value::ComponentVersionSuffixInvalid);
    spdlog::error("    `versionsuffix` needs a preceding canonical version"sv);
    return Unexpect(ErrCode::Value::ComponentVersionSuffixInvalid);
  }
  std::string Full(NameDetail.Version);
  Full.append(Suffix);
  if (!scanSemver(Full)) {
    spdlog::error(ErrCode::Value::ComponentVersionSuffixInvalid);
    spdlog::error("    `{}` and `versionsuffix` `{}` are not a valid semver"sv,
                  NameDetail.Version, Suffix);
    return Unexpect(ErrCode::Value::ComponentVersionSuffixInvalid);
  }
  return {};
}

// canonversion ::= [1-9] [0-9]*
//                | '0.' [1-9] [0-9]*
//                | '0.0.' [1-9] [0-9]*
bool ExternName::isCanonVersion(std::string_view V) const noexcept {
  if (V.substr(0, 4) == "0.0."sv) {
    V.remove_prefix(4);
  } else if (V.substr(0, 2) == "0."sv) {
    V.remove_prefix(2);
  }
  if (V.empty() || V[0] < '1' || V[0] > '9') {
    return false;
  }
  for (char C : V) {
    if (!isdigit(static_cast<unsigned char>(C))) {
      return false;
    }
  }
  return true;
}

// MAJOR.MINOR.PATCH[-prerelease][+build] per semver.org 2.0. The unlogged
// granular code reaches an interface version, but not a dep or range bound.
Expect<void> ExternName::scanSemver(std::string_view V) const noexcept {
  if (V.empty()) {
    return Unexpect(ErrCode::Value::NameEmptyString);
  }

  for (uint32_t I = 0; I < 3; I++) {
    if (I > 0) {
      if (V.empty()) {
        return Unexpect(ErrCode::Value::NameUnexpectedEnd);
      }
      if (V[0] != '.') {
        return Unexpect(ErrCode::Value::NameUnexpectedCharacter);
      }
      V.remove_prefix(1);
    }
    size_t Len = 0;
    while (Len < V.size() && isdigit(static_cast<unsigned char>(V[Len]))) {
      Len++;
    }
    if (Len == 0) {
      return Unexpect(V.empty() ? ErrCode::Value::NameUnexpectedEnd
                                : ErrCode::Value::NameUnexpectedCharacter);
    }
    if (Len > 1 && V[0] == '0') {
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    V.remove_prefix(Len);
  }

  if (V.empty()) {
    return {};
  }
  if (V[0] != '-' && V[0] != '+') {
    return Unexpect(ErrCode::Value::NameUnexpectedCharacter);
  }

  if (V[0] == '-') {
    V.remove_prefix(1);
    size_t PlusPos = V.find('+');
    std::string_view PreRelease =
        (PlusPos == V.npos) ? V : V.substr(0, PlusPos);
    EXPECTED_TRY(scanSemverIdentifiers(PreRelease, true));
    if (PlusPos == V.npos) {
      return {};
    }
    V.remove_prefix(PlusPos);
  }

  // Here V starts with '+': scan the build metadata identifiers.
  V.remove_prefix(1);
  return scanSemverIdentifiers(V, false);
}

// Scans a dot-separated pre-release or build identifier list. Identifiers are
// [0-9A-Za-z-]+ and, for pre-release, numeric ones have no leading zeros.
Expect<void>
ExternName::scanSemverIdentifiers(std::string_view Idents,
                                  bool CheckLeadingZeros) const noexcept {
  size_t Start = 0;
  while (true) {
    size_t DotPos = Idents.find('.', Start);
    std::string_view Ident = (DotPos == Idents.npos)
                                 ? Idents.substr(Start)
                                 : Idents.substr(Start, DotPos - Start);
    if (Ident.empty()) {
      return Unexpect(ErrCode::Value::NameEmptyIdentifierSegment);
    }
    bool AllDigits = true;
    for (char C : Ident) {
      if (!isdigit(static_cast<unsigned char>(C))) {
        AllDigits = false;
        if (!isalnum(static_cast<unsigned char>(C)) && C != '-') {
          return Unexpect(ErrCode::Value::NameUnexpectedCharacter);
        }
      }
    }
    if (CheckLeadingZeros && AllDigits && Ident.size() > 1 && Ident[0] == '0') {
      return Unexpect(ErrCode::Value::ComponentInvalidName);
    }
    if (DotPos == Idents.npos) {
      return {};
    }
    Start = DotPos + 1;
  }
}

// integrity-metadata ::= *WSP hash-with-options *(1*WSP hash-with-options) *WSP
// hash-with-options  ::= hash-expression *("?" option-expression)
// hash-expression    ::= hash-algorithm "-" base64-value
// hash-algorithm     ::= "sha256" / "sha384" / "sha512"
Expect<void>
ExternName::checkIntegrityMetadata(std::string_view Input) const noexcept {
  // base64-value ::= [A-Za-z0-9+/]+ ( '=' | '==' )?
  // Non-empty, with `=` padding only at the end.
  auto isBase64 = [](std::string_view S) noexcept {
    if (S.empty()) {
      return false;
    }
    size_t Equals = 0;
    for (size_t I = 0; I < S.size(); I++) {
      char C = S[I];
      if ((isalnum(static_cast<unsigned char>(C)) || C == '+' || C == '/') &&
          Equals == 0) {
        continue;
      }
      if (C == '=' && I > 0 && Equals < 2) {
        Equals++;
        continue;
      }
      return false;
    }
    return true;
  };

  while (!Input.empty() && Input.front() == ' ') {
    Input.remove_prefix(1);
  }
  while (!Input.empty() && Input.back() == ' ') {
    Input.remove_suffix(1);
  }
  if (Input.empty()) {
    spdlog::error(ErrCode::Value::NameIntegrityEmpty);
    spdlog::error("    Component name: integrity hash cannot be empty"sv);
    return Unexpect(ErrCode::Value::NameIntegrityEmpty);
  }

  while (!Input.empty()) {
    while (!Input.empty() && Input.front() == ' ') {
      Input.remove_prefix(1);
    }
    if (Input.empty()) {
      break;
    }

    size_t TokenEnd = Input.find(' ');
    std::string_view Token =
        (TokenEnd == Input.npos) ? Input : Input.substr(0, TokenEnd);
    Input =
        (TokenEnd == Input.npos) ? std::string_view{} : Input.substr(TokenEnd);

    size_t OptPos = Token.find('?');
    std::string_view HashExpr =
        (OptPos == Token.npos) ? Token : Token.substr(0, OptPos);

    size_t DashPos = HashExpr.find('-');
    if (DashPos == HashExpr.npos) {
      spdlog::error(ErrCode::Value::NameExpectedDashAfterHash);
      spdlog::error("    Component name: expected `-` after hash algorithm"sv);
      return Unexpect(ErrCode::Value::NameExpectedDashAfterHash);
    }
    std::string_view Algo = HashExpr.substr(0, DashPos);
    if (Algo != "sha256"sv && Algo != "sha384"sv && Algo != "sha512"sv) {
      spdlog::error(ErrCode::Value::NameUnknownHashAlgorithm);
      spdlog::error("    Component name: unrecognized hash algorithm '{}'"sv,
                    Algo);
      return Unexpect(ErrCode::Value::NameUnknownHashAlgorithm);
    }
    if (!isBase64(HashExpr.substr(DashPos + 1))) {
      spdlog::error(ErrCode::Value::NameInvalidBase64);
      spdlog::error("    Component name: hash value is not valid base64"sv);
      return Unexpect(ErrCode::Value::NameInvalidBase64);
    }
  }

  return {};
}

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
