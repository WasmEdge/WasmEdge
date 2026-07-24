// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "validator/component_name.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

// label          ::= <first-fragment> ( '-' <fragment> )*
// first-fragment ::= <first-word> | <first-acronym>
// first-word     ::= [a-z] [0-9a-z]*
// first-acronym  ::= [A-Z] [0-9A-Z]*
// fragment       ::= <word> | <acronym>
// word           ::= [0-9a-z]+
// acronym        ::= [0-9A-Z]+
bool isKebabString(std::string_view Input) {
  bool IsFirstPart = true;
  bool Uppercase = false;
  bool Lowercase = false;
  bool Digit = false;

  for (char C : Input) {
    if (islower(C)) {
      if (Uppercase) {
        return false;
      }
      Lowercase = true;
    } else if (isupper(C)) {
      if (Lowercase) {
        return false;
      }
      Uppercase = true;
    } else if (isdigit(C)) {
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

namespace {

// words      ::= <first-word> ( '-' <word> )*
// first-word ::= [a-z] [0-9a-z]*
// word       ::= [0-9a-z]+
bool isLowercaseKebabString(std::string_view Input) {
  if (Input.empty() || !islower(Input[0])) {
    return false;
  }
  for (char C : Input) {
    if (C != '-' && !islower(C) && !isdigit(C)) {
      return false;
    }
  }
  return Input.back() != '-' && Input.find("--"sv) == Input.npos;
}

bool isEOF(std::string_view Input) { return Input.empty(); }

bool readUntil(std::string_view &Input, char Delim, std::string_view &Output) {
  size_t Pos = Input.find(Delim);
  if (Pos == Input.npos) {
    return false;
  }

  Output = Input.substr(0, Pos);
  Input.remove_prefix(Pos + 1);
  return true;
}

bool tryRead(std::string_view Prefix, std::string_view &Name) {
  if (Prefix.size() > Name.size()) {
    return false;
  }
  if (Prefix != Name.substr(0, Prefix.size())) {
    return false;
  }

  Name.remove_prefix(Prefix.size());
  return true;
}

// Consumes and returns the leading run of label characters ([0-9A-Za-z-]).
std::string_view readLabelChars(std::string_view &Input) {
  size_t Pos = 0;
  while (Pos < Input.size() && (isalnum(Input[Pos]) || Input[Pos] == '-')) {
    Pos++;
  }
  std::string_view Output = Input.substr(0, Pos);
  Input.remove_prefix(Pos);
  return Output;
}

// Fallback for cases without a dedicated diagnostic code.
Unexpected<ErrCode> reportError(std::string_view Reason) {
  spdlog::error(ErrCode::Value::ComponentInvalidName);
  spdlog::error("    Component name: {}"sv, Reason);
  return Unexpect(ErrCode::Value::ComponentInvalidName);
}

// Emits a granular extern-name grammar diagnostic.
Unexpected<ErrCode> reportNameError(ErrCode::Value Code,
                                    std::string_view Detail) {
  spdlog::error(Code);
  spdlog::error("    Component name: {}"sv, Detail);
  return Unexpect(Code);
}

Unexpected<ErrCode> reportNotKebab(std::string_view Label) {
  spdlog::error(ErrCode::Value::ComponentNameNotKebab);
  spdlog::error("    Component name: label '{}' is not in kebab case"sv, Label);
  return Unexpect(ErrCode::Value::ComponentNameNotKebab);
}

Unexpected<ErrCode> reportNotLowercase(std::string_view What,
                                       std::string_view Label) {
  spdlog::error(ErrCode::Value::ComponentPackageNameNotLowercase);
  spdlog::error("    Component name: {} '{}' is not lowercase"sv, What, Label);
  return Unexpect(ErrCode::Value::ComponentPackageNameNotLowercase);
}

// Validates a namespace or package label (`<words>` position): a label that
// is not even kebab-shaped (e.g. `WaSi`) is "not in kebab case", while a
// well-formed kebab label with uppercase (e.g. `A`) is "not lowercase".
Expect<void> checkWordsLabel(std::string_view Label, std::string_view What) {
  if (!isKebabString(Label)) {
    return reportNotKebab(Label);
  }
  if (!isLowercaseKebabString(Label)) {
    return reportNotLowercase(What, Label);
  }
  return {};
}

// Parses a non-negative integer without leading zeros.
// Returns the end position, or npos on failure.
size_t parseNumeric(std::string_view V) {
  if (V.empty()) {
    return std::string_view::npos;
  }
  if (V[0] == '0') {
    return 1;
  }
  if (V[0] >= '1' && V[0] <= '9') {
    size_t Pos = 1;
    while (Pos < V.size() && isdigit(V[Pos])) {
      Pos++;
    }
    return Pos;
  }
  return std::string_view::npos;
}

// canonversion ::= [1-9] [0-9]*
//                | '0.' [1-9] [0-9]*
//                | '0.0.' [1-9] [0-9]*
bool isCanonVersion(std::string_view V) {
  if (V.empty()) {
    return false;
  }

  // canonversion ::= [1-9] [0-9]* | '0.' [1-9] [0-9]* | '0.0.' [1-9] [0-9]*
  for (int I = 0; I < 3; I++) {
    if (V[0] >= '1' && V[0] <= '9') {
      size_t End = parseNumeric(V);
      return End == V.size();
    }
    if (!tryRead("0."sv, V) || V.empty()) {
      return false;
    }
  }

  return false;
}

// Validates a dot-separated pre-release or build identifier segment.
// Each identifier is [0-9A-Za-z-]+.
// Numeric identifiers must not have leading zeros.
bool isPreReleaseOrBuild(std::string_view V, bool CheckLeadingZeros) {
  if (V.empty()) {
    return false;
  }
  size_t Start = 0;
  while (Start < V.size()) {
    size_t DotPos = V.find('.', Start);
    std::string_view Ident =
        (DotPos == V.npos) ? V.substr(Start) : V.substr(Start, DotPos - Start);
    if (Ident.empty()) {
      return false;
    }
    for (char C : Ident) {
      if (!isalnum(C) && C != '-') {
        return false;
      }
    }
    if (CheckLeadingZeros) {
      bool AllDigits = std::all_of(Ident.begin(), Ident.end(),
                                   [](char C) { return isdigit(C); });
      if (AllDigits && Ident.size() > 1 && Ident[0] == '0') {
        return false;
      }
    }
    if (DotPos == V.npos) {
      break;
    }
    Start = DotPos + 1;
  }
  return true;
}

// MAJOR.MINOR.PATCH[-prerelease][+build] per semver.org 2.0
bool isValidSemver(std::string_view V) {
  if (V.empty()) {
    return false;
  }

  // Parse MAJOR.MINOR.PATCH
  for (int I = 0; I < 3; I++) {
    size_t End = parseNumeric(V);
    if (End == std::string_view::npos) {
      return false;
    }
    if (I < 2) {
      if (End >= V.size() || V[End] != '.') {
        return false;
      }
      V.remove_prefix(End + 1);
    } else {
      V.remove_prefix(End);
    }
  }

  if (V.empty()) {
    return true;
  }

  if (V[0] == '-') {
    V.remove_prefix(1);
    size_t PlusPos = V.find('+');
    std::string_view PreRelease =
        (PlusPos == V.npos) ? V : V.substr(0, PlusPos);
    if (!isPreReleaseOrBuild(PreRelease, true)) {
      return false;
    }
    if (PlusPos == V.npos) {
      return true;
    }
    V.remove_prefix(PlusPos);
  }

  if (!V.empty() && V[0] == '+') {
    V.remove_prefix(1);
    return isPreReleaseOrBuild(V, false);
  }

  return V.empty();
}

// Consumes a numeric version part ([0-9]+, no leading zeros) with granular
// diagnostics matching the Rust `semver` crate wasm-tools relies on.
Expect<void> parseSemverNumeric(std::string_view &V) {
  size_t Len = 0;
  while (Len < V.size() && isdigit(V[Len])) {
    Len++;
  }
  if (Len == 0) {
    if (V.empty()) {
      return reportNameError(ErrCode::Value::NameUnexpectedEnd,
                             "unexpected end of input in version number"sv);
    }
    return reportNameError(ErrCode::Value::NameUnexpectedCharacter,
                           "unexpected character in version number"sv);
  }
  if (Len > 1 && V[0] == '0') {
    return reportError("invalid leading zero in version number"sv);
  }
  V.remove_prefix(Len);
  return {};
}

// Consumes the `.` separator between version numbers.
Expect<void> parseSemverDot(std::string_view &V) {
  if (V.empty()) {
    return reportNameError(ErrCode::Value::NameUnexpectedEnd,
                           "unexpected end of input after version number"sv);
  }
  if (V[0] != '.') {
    return reportNameError(ErrCode::Value::NameUnexpectedCharacter,
                           "unexpected character after version number"sv);
  }
  V.remove_prefix(1);
  return {};
}

// Validates a dot-separated pre-release or build identifier list with
// granular diagnostics. Identifiers are [0-9A-Za-z-]+ and, for pre-release,
// numeric identifiers must not have leading zeros.
Expect<void> checkSemverIdentifiers(std::string_view Idents,
                                    bool CheckLeadingZeros) {
  size_t Start = 0;
  while (true) {
    size_t DotPos = Idents.find('.', Start);
    std::string_view Ident = (DotPos == Idents.npos)
                                 ? Idents.substr(Start)
                                 : Idents.substr(Start, DotPos - Start);
    if (Ident.empty()) {
      return reportNameError(ErrCode::Value::NameEmptyIdentifierSegment,
                             "empty identifier segment in version suffix"sv);
    }
    bool AllDigits = true;
    for (char C : Ident) {
      if (!isdigit(C)) {
        AllDigits = false;
        if (!isalnum(C) && C != '-') {
          return reportNameError(
              ErrCode::Value::NameUnexpectedCharacter,
              "unexpected character in version suffix identifier"sv);
        }
      }
    }
    if (CheckLeadingZeros && AllDigits && Ident.size() > 1 && Ident[0] == '0') {
      return reportError("invalid leading zero in pre-release identifier"sv);
    }
    if (DotPos == Idents.npos) {
      break;
    }
    Start = DotPos + 1;
  }
  return {};
}

// MAJOR.MINOR.PATCH[-prerelease][+build] per semver.org 2.0, reporting a
// granular diagnostic for each failure mode.
Expect<void> parseSemver(std::string_view V) {
  if (V.empty()) {
    return reportNameError(ErrCode::Value::NameEmptyString,
                           "empty string, expected a semver version"sv);
  }

  // Parse MAJOR.MINOR.PATCH
  EXPECTED_TRY(parseSemverNumeric(V));
  EXPECTED_TRY(parseSemverDot(V));
  EXPECTED_TRY(parseSemverNumeric(V));
  EXPECTED_TRY(parseSemverDot(V));
  EXPECTED_TRY(parseSemverNumeric(V));

  if (V.empty()) {
    return {};
  }
  if (V[0] != '-' && V[0] != '+') {
    return reportNameError(ErrCode::Value::NameUnexpectedCharacter,
                           "unexpected character after patch version"sv);
  }

  if (V[0] == '-') {
    V.remove_prefix(1);
    size_t PlusPos = V.find('+');
    std::string_view PreRelease =
        (PlusPos == V.npos) ? V : V.substr(0, PlusPos);
    EXPECTED_TRY(checkSemverIdentifiers(PreRelease, true));
    if (PlusPos == V.npos) {
      return {};
    }
    V.remove_prefix(PlusPos);
  }

  // Here V starts with '+': validate the build metadata identifiers.
  V.remove_prefix(1);
  return checkSemverIdentifiers(V, false);
}

// verrange body ::= <verlower> | <verupper> | <verlower> ' ' <verupper>
// verlower      ::= '>=' <valid semver>
// verupper      ::= '<' <valid semver>
Expect<void> checkVersionRange(std::string_view Body) {
  std::string_view Remaining = Body;

  if (tryRead(">="sv, Remaining)) {
    size_t SpacePos = Remaining.find(' ');
    std::string_view Lower = (SpacePos == Remaining.npos)
                                 ? Remaining
                                 : Remaining.substr(0, SpacePos);
    if (!isValidSemver(Lower)) {
      return reportNameError(ErrCode::Value::NameNotValidSemver,
                             "version range lower bound is not valid"sv);
    }
    if (SpacePos == Remaining.npos) {
      return {};
    }
    Remaining.remove_prefix(SpacePos + 1);
    if (!tryRead("<"sv, Remaining)) {
      return reportNameError(ErrCode::Value::NameExpectedOpenAngle,
                             "expected `<` before version range upper bound"sv);
    }
    if (!isValidSemver(Remaining)) {
      return reportNameError(ErrCode::Value::NameNotValidSemver,
                             "version range upper bound is not valid"sv);
    }
    return {};
  }

  if (tryRead("<"sv, Remaining)) {
    if (!isValidSemver(Remaining)) {
      return reportNameError(ErrCode::Value::NameNotValidSemver,
                             "version range upper bound is not valid"sv);
    }
    return {};
  }

  return reportNameError(ErrCode::Value::NameExpectedVersionRangeOp,
                         "expected `>=` or `<` at start of version range"sv);
}

// base64-value ::= [A-Za-z0-9+/]+ ( '=' | '==' )?
// Non-empty, with `=` padding only at the end.
bool isBase64(std::string_view S) {
  if (S.empty()) {
    return false;
  }
  size_t Equals = 0;
  for (size_t I = 0; I < S.size(); I++) {
    char C = S[I];
    if ((isalnum(C) || C == '+' || C == '/') && Equals == 0) {
      continue;
    }
    if (C == '=' && I > 0 && Equals < 2) {
      Equals++;
      continue;
    }
    return false;
  }
  return true;
}

// integrity-metadata ::= *WSP hash-with-options *(1*WSP hash-with-options) *WSP
// hash-with-options  ::= hash-expression *("?" option-expression)
// hash-expression    ::= hash-algorithm "-" base64-value
// hash-algorithm     ::= "sha256" / "sha384" / "sha512"
Expect<void> checkIntegrityMetadata(std::string_view Input) {
  while (!Input.empty() && Input.front() == ' ') {
    Input.remove_prefix(1);
  }
  while (!Input.empty() && Input.back() == ' ') {
    Input.remove_suffix(1);
  }
  if (Input.empty()) {
    return reportNameError(ErrCode::Value::NameIntegrityEmpty,
                           "integrity hash cannot be empty"sv);
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
      return reportNameError(ErrCode::Value::NameExpectedDashAfterHash,
                             "expected `-` after hash algorithm"sv);
    }
    std::string_view Algo = HashExpr.substr(0, DashPos);
    if (Algo != "sha256"sv && Algo != "sha384"sv && Algo != "sha512"sv) {
      return reportNameError(ErrCode::Value::NameUnknownHashAlgorithm,
                             "unrecognized hash algorithm"sv);
    }
    if (!isBase64(HashExpr.substr(DashPos + 1))) {
      return reportNameError(ErrCode::Value::NameInvalidBase64,
                             "hash value is not valid base64"sv);
    }
  }

  return {};
}

// hashname ::= 'integrity=<' <integrity-metadata> '>'
// Parses optional ',integrity=<...>' suffix from Next.
// If Next is empty, returns true with empty Integrity.
// On success, Next is consumed and Integrity is set.
Expect<void> tryParseIntegritySuffix(std::string_view &Next,
                                     std::string_view &Integrity) {
  if (Next.empty()) {
    Integrity = {};
    return {};
  }
  if (Next[0] != ',') {
    return reportNameError(ErrCode::Value::NameTrailingCharacters,
                           "trailing characters found"sv);
  }
  Next.remove_prefix(1);
  if (!tryRead("integrity=<"sv, Next)) {
    return reportNameError(ErrCode::Value::NameExpectedIntegrity,
                           "expected `integrity=<` after `,`"sv);
  }
  std::string_view IntegrityData;
  if (!readUntil(Next, '>', IntegrityData)) {
    return reportNameError(ErrCode::Value::NameFailedToFindCloseAngle,
                           "failed to find `>` closing integrity"sv);
  }
  EXPECTED_TRY(checkIntegrityMetadata(IntegrityData));
  if (!isEOF(Next)) {
    return reportNameError(ErrCode::Value::NameTrailingCharacters,
                           "trailing characters found after integrity"sv);
  }
  Integrity = IntegrityData;
  return {};
}

// pkgpath ::= <namespace> <words>
// Parses 'namespace:package' from Next, stopping at delimiters in StopChars.
struct PkgPath {
  std::string_view Namespace;
  std::string_view Package;
};

Expect<PkgPath> parsePkgPath(std::string_view &Next,
                             std::string_view StopChars) {
  size_t ColonPos = Next.find(':');
  size_t StopPos = Next.find_first_of(StopChars);
  if (ColonPos == Next.npos || (StopPos != Next.npos && StopPos < ColonPos)) {
    // No namespace delimiter: diagnose the leading label run. This catches
    // inputs like `<`, `<>`, or a stray label without `:`.
    std::string_view Label = readLabelChars(Next);
    EXPECTED_TRY(checkWordsLabel(Label, "namespace"sv));
    return reportError("expected ':' after namespace"sv);
  }

  std::string_view Namespace = Next.substr(0, ColonPos);
  Next.remove_prefix(ColonPos + 1);
  EXPECTED_TRY(checkWordsLabel(Namespace, "namespace"sv));

  size_t PkgEnd = Next.find_first_of(StopChars);
  std::string_view Package =
      (PkgEnd == Next.npos) ? Next : Next.substr(0, PkgEnd);
  Next.remove_prefix(Package.size());
  EXPECTED_TRY(checkWordsLabel(Package, "package"sv));
  if (PkgEnd == std::string_view::npos) {
    // The package name ran to end of input without a closing delimiter.
    return reportNameError(ErrCode::Value::NameExpectedCloseAngle,
                           "expected `>` after package path"sv);
  }

  return PkgPath{Namespace, Package};
}

} // anonymous namespace

// exportname        ::= <plainname> | <interfacename>
// importname        ::= <exportname> | <depname> | <urlname> | <hashname>
Expect<ComponentName> ComponentName::parse(std::string_view Name) {
  ComponentName Result(Name);
  auto Next = Name;

  // plainname         ::= <label>
  //                     | '[constructor]' <label>
  //                     | '[method]' <label> '.' <label>
  //                     | '[static]' <label> '.' <label>

  if (tryRead("[constructor]"sv, Next)) {
    if (!isKebabString(Next)) {
      return reportNotKebab(Next);
    }
    Result.Detail.emplace<ConstructorDetail>(ConstructorDetail{Next});
    Result.NoTagName = Next;
    Result.Kind = ComponentNameKind::Constructor;
    return Result;
  }

  // Once a '[method]' or '[static]' tag matched, the name must contain a
  // '.' separating two kebab labels.
  auto parseResourceWithLabel = [&](std::string_view &Resource,
                                    std::string_view &Label) -> Expect<void> {
    Result.NoTagName = Next;
    if (!readUntil(Next, '.', Resource)) {
      return reportNameError(ErrCode::Value::NameFailedToFindDot,
                             "failed to find `.` character"sv);
    }
    if (!isKebabString(Resource)) {
      return reportNotKebab(Resource);
    }
    if (!isKebabString(Next)) {
      return reportNotKebab(Next);
    }
    Label = Next;
    return {};
  };

  if (tryRead("[method]"sv, Next)) {
    std::string_view Resource, Label;
    EXPECTED_TRY(parseResourceWithLabel(Resource, Label));
    Result.Detail.emplace<MethodDetail>(MethodDetail{Resource, Label});
    Result.Kind = ComponentNameKind::Method;
    return Result;
  }

  if (tryRead("[static]"sv, Next)) {
    std::string_view Resource, Label;
    EXPECTED_TRY(parseResourceWithLabel(Resource, Label));
    Result.Detail.emplace<StaticDetail>(StaticDetail{Resource, Label});
    Result.Kind = ComponentNameKind::Static;
    return Result;
  }

  if (tryRead("[async]"sv, Next)) {
    Result.NoTagName = Next;
    return reportError("[async] not supported yet"sv);
  }

  if (tryRead("[async method]"sv, Next)) {
    Result.NoTagName = Next;
    return reportError("[async method] not supported yet"sv);
  }

  if (tryRead("[async static]"sv, Next)) {
    Result.NoTagName = Next;
    return reportError("[async static] not supported yet"sv);
  }

  if (Next.size() != 0 && Next[0] == '[') {
    return reportError("unknown annotation"sv);
  }
  Result.NoTagName = Next;

  // depname ::= 'unlocked-dep=<' <pkgnamequery> '>'
  //           | 'locked-dep=<' <pkgname> '>' ( ',' <hashname> )?

  if (tryRead("unlocked-dep="sv, Next)) {
    if (!tryRead("<"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedOpenAngle,
                             "expected `<` after unlocked-dep="sv);
    }

    EXPECTED_TRY(auto Path, parsePkgPath(Next, "@>"sv));

    // verrange ::= '@*'
    //            | '@{' verlower '}'
    //            | '@{' verupper '}'
    //            | '@{' verlower ' ' verupper '}'
    std::string_view VersionRange;
    if (!Next.empty() && Next[0] == '@') {
      auto VerStart = Next;
      Next.remove_prefix(1);

      if (!Next.empty() && Next[0] == '*') {
        Next.remove_prefix(1);
      } else if (!Next.empty() && Next[0] == '{') {
        size_t ClosePos = Next.find('}');
        if (ClosePos == Next.npos) {
          return reportError("expected '}' in unlocked-dep version range"sv);
        }
        EXPECTED_TRY(checkVersionRange(Next.substr(1, ClosePos - 1)));
        Next.remove_prefix(ClosePos + 1);
      } else {
        return reportNameError(ErrCode::Value::NameExpectedOpenBrace,
                               "expected `{` at start of version range"sv);
      }
      VersionRange = VerStart.substr(0, VerStart.size() - Next.size());
    }

    if (!tryRead(">"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedCloseAngle,
                             "expected `>` closing unlocked-dep"sv);
    }

    if (!isEOF(Next)) {
      return reportNameError(ErrCode::Value::NameTrailingCharacters,
                             "trailing characters found after unlocked-dep"sv);
    }

    Result.Detail.emplace<UnlockedDepDetail>(
        UnlockedDepDetail{Path.Namespace, Path.Package, VersionRange});
    Result.Kind = ComponentNameKind::UnlockedDep;
    return Result;
  }

  if (tryRead("locked-dep="sv, Next)) {
    if (!tryRead("<"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedOpenAngle,
                             "expected `<` after locked-dep="sv);
    }

    EXPECTED_TRY(auto Path, parsePkgPath(Next, "@>"sv));

    std::string_view Version;
    if (!Next.empty() && Next[0] == '@') {
      Next.remove_prefix(1);
      size_t VerEnd = Next.find('>');
      if (VerEnd == Next.npos) {
        return reportNameError(ErrCode::Value::NameExpectedCloseAngle,
                               "expected `>` after version in locked-dep"sv);
      }
      Version = Next.substr(0, VerEnd);
      Next.remove_prefix(VerEnd);
      if (!isValidSemver(Version)) {
        return reportNameError(ErrCode::Value::NameNotValidSemver,
                               "locked-dep version is not a valid semver"sv);
      }
    }

    if (!tryRead(">"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedCloseAngle,
                             "expected `>` closing locked-dep"sv);
    }

    std::string_view Integrity;
    EXPECTED_TRY(tryParseIntegritySuffix(Next, Integrity));

    Result.Detail.emplace<LockedDepDetail>(
        LockedDepDetail{Path.Namespace, Path.Package, Version, Integrity});
    Result.Kind = ComponentNameKind::LockedDep;
    return Result;
  }

  // urlname ::= 'url=<' <nonbrackets> '>' (',' <hashname>)?
  // nonbrackets ::= [^<>]*
  if (tryRead("url="sv, Next)) {
    if (!tryRead("<"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedOpenAngle,
                             "expected `<` after url="sv);
    }

    size_t ClosePos = Next.find('>');
    if (ClosePos == Next.npos) {
      return reportNameError(ErrCode::Value::NameFailedToFindCloseAngle,
                             "failed to find `>` closing url"sv);
    }

    std::string_view UrlContent = Next.substr(0, ClosePos);
    if (UrlContent.find('<') != UrlContent.npos) {
      return reportNameError(ErrCode::Value::NameUrlContainsOpenAngle,
                             "url cannot contain `<`"sv);
    }
    Next.remove_prefix(ClosePos + 1);

    std::string_view Integrity;
    EXPECTED_TRY(tryParseIntegritySuffix(Next, Integrity));

    Result.Detail.emplace<UrlDetail>(UrlDetail{UrlContent, Integrity});
    Result.Kind = ComponentNameKind::Url;
    return Result;
  }

  // hashname ::= 'integrity=<' <integrity-metadata> '>'
  if (tryRead("integrity="sv, Next)) {
    if (!tryRead("<"sv, Next)) {
      return reportNameError(ErrCode::Value::NameExpectedOpenAngle,
                             "expected `<` after integrity="sv);
    }
    std::string_view IntegrityData;
    if (!readUntil(Next, '>', IntegrityData)) {
      return reportNameError(ErrCode::Value::NameFailedToFindCloseAngle,
                             "failed to find `>` closing integrity"sv);
    }
    EXPECTED_TRY(checkIntegrityMetadata(IntegrityData));
    if (!isEOF(Next)) {
      return reportNameError(ErrCode::Value::NameTrailingCharacters,
                             "trailing characters found after integrity"sv);
    }
    Result.Detail.emplace<IntegrityDetail>(IntegrityDetail{IntegrityData});
    Result.Kind = ComponentNameKind::Integrity;
    return Result;
  }

  // interfacename ::= <namespace> <label> <projection> <interfaceversion>?
  // namespace     ::= <words> ':'
  // projection    ::= '/' <label>
  // interfaceversion ::= '@' <valid semver> | '@' <canonversion>
  if (size_t ColonPos = Next.find(':'); ColonPos != Next.npos) {
    std::string_view Namespace = Next.substr(0, ColonPos);
    Next.remove_prefix(ColonPos + 1);
    EXPECTED_TRY(checkWordsLabel(Namespace, "namespace"sv));

    std::string_view Package = readLabelChars(Next);
    EXPECTED_TRY(checkWordsLabel(Package, "package"sv));

    // Nested namespaces (`a:b:c/d`) are feature-gated: after the package
    // name only a projection may follow.
    if (isEOF(Next) || Next[0] != '/') {
      return reportNameError(ErrCode::Value::NameExpectedSlashAfterPackage,
                             "expected `/` after package name"sv);
    }
    Next.remove_prefix(1);

    std::string_view Interface = readLabelChars(Next);
    if (!isKebabString(Interface)) {
      return reportNotKebab(Interface);
    }

    // Nested projections (`a:b/c/d`) are feature-gated: only a version may
    // follow the projection label.
    if (!isEOF(Next) && Next[0] != '@') {
      return reportNameError(ErrCode::Value::NameTrailingCharacters,
                             "trailing characters found after projection"sv);
    }

    std::string_view Version;
    if (!isEOF(Next)) {
      Next.remove_prefix(1);
      Version = Next;
      if (!isCanonVersion(Version)) {
        EXPECTED_TRY(parseSemver(Version));
      }
    }

    Result.Detail.emplace<InterfaceDetail>(
        InterfaceDetail{Namespace, Package, Interface, Version});
    Result.Kind = ComponentNameKind::InterfaceType;
    return Result;
  }

  if (!isKebabString(Next)) {
    return reportNotKebab(Next);
  }
  Result.Detail.emplace<LabelDetail>();
  Result.Kind = ComponentNameKind::Label;
  return Result;
}

} // namespace Validator
} // namespace WasmEdge
