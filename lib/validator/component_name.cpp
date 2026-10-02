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

// label ::= <first-fragment> ( '-' <fragment> )*, each a word or an acronym.
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

// plainname ::= <label> | '[constructor|method|static]' <label> ('.' <label>)?
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

  // A '[method]' or '[static]' name needs a '.' between two kebab labels.
  auto ReadResourceAndLabel = [this](std::string_view &Resource,
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
    EXPECTED_TRY(ReadResourceAndLabel(Resource, Label));
    NameDetail.Resource = Resource;
    NameDetail.Method = Label;
    NameKind = Kind::Method;
    return {};
  }

  if (tryRead("[static]"sv)) {
    std::string_view Resource, Label;
    EXPECTED_TRY(ReadResourceAndLabel(Resource, Label));
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

// interfacename ::= <words> ':' <label> '/' <label> <interfaceversion>?
Expect<void> ExternName::parseInterfaceName() noexcept {
  size_t ColonPos = Rest.find(':');
  std::string_view Namespace = Rest.substr(0, ColonPos);
  Rest.remove_prefix(ColonPos + 1);
  EXPECTED_TRY(checkWordsLabel(Namespace, "namespace"sv));

  std::string_view Package = readLabelChars();
  EXPECTED_TRY(checkWordsLabel(Package, "package"sv));

  // Nested namespaces (`a:b:c/d`) are feature-gated; only a projection follows.
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

  // Nested projections (`a:b/c/d`) are feature-gated; only a version follows.
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

// words ::= <first-word> ( '-' <word> )*: a kebab label that is all lowercase.
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

// semversuffix ::= [0-9A-Za-z.+-]*
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

// canonversion ::= [1-9] [0-9]* | '0.' [1-9] [0-9]* | '0.0.' [1-9] [0-9]*
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

// Scans MAJOR.MINOR.PATCH[-prerelease][+build] per semver.org 2.0, unlogged.
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

// Scans a dot-separated identifier list, rejecting leading zeros when asked.
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

} // namespace Component
} // namespace Validator
} // namespace WasmEdge
