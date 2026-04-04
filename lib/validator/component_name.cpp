// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/component_name.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace WasmEdge {
namespace Validator {

using namespace std::literals;

namespace {

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
      if (Uppercase)
        return false;
      Lowercase = true;
    } else if (isupper(C)) {
      if (Lowercase)
        return false;
      Uppercase = true;
    } else if (isdigit(C)) {
      if (IsFirstPart && !(Uppercase || Lowercase))
        return false;
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

// words      ::= <first-word> ( '-' <word> )*
// first-word ::= [a-z] [0-9a-z]*
// word       ::= [0-9a-z]+
bool isLowercaseKebabString(std::string_view Input) {
  if (Input.empty() || !islower(Input[0]))
    return false;
  for (char C : Input) {
    if (C != '-' && !islower(C) && !isdigit(C))
      return false;
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
  if (Prefix.size() > Name.size())
    return false;
  if (Prefix != Name.substr(0, Prefix.size()))
    return false;

  Name.remove_prefix(Prefix.size());
  return true;
}

bool tryReadKebab(std::string_view &Input, std::string_view &Output) {
  size_t Pos = 0;
  while (Pos < Input.size()) {
    if (isalnum(Input[Pos]) || Input[Pos] == '-') {
      Pos++;
    } else {
      break;
    }
  }
  Output = Input.substr(0, Pos);
  Input.remove_prefix(Pos);
  return isKebabString(Output);
}

// integrity-metadata = *WSP hash-with-options *(1*WSP hash-with-options) *WSP
// hash-with-options   = hash-expression *("?" option-expression)
// hash-expression     = hash-algorithm "-" base64-value
// hash-algorithm      = "sha256" / "sha384" / "sha512"
// base64-value        = *VCHAR (visible chars, no whitespace)
bool isIntegrityMetadata(std::string_view Input) {
  while (!Input.empty() && Input.front() == ' ')
    Input.remove_prefix(1);
  while (!Input.empty() && Input.back() == ' ')
    Input.remove_suffix(1);
  if (Input.empty())
    return false;

  bool HasToken = false;
  while (!Input.empty()) {
    while (!Input.empty() && Input.front() == ' ')
      Input.remove_prefix(1);
    if (Input.empty())
      break;

    size_t TokenEnd = Input.find(' ');
    std::string_view Token =
        (TokenEnd == Input.npos) ? Input : Input.substr(0, TokenEnd);
    Input =
        (TokenEnd == Input.npos) ? std::string_view{} : Input.substr(TokenEnd);

    size_t OptPos = Token.find('?');
    std::string_view HashExpr =
        (OptPos == Token.npos) ? Token : Token.substr(0, OptPos);

    bool ValidAlgo = false;
    static constexpr std::string_view Algos[3] = {"sha256-", "sha384-",
                                                  "sha512-"};
    for (auto AlgoSV : Algos) {
      if (HashExpr.size() > AlgoSV.size() &&
          HashExpr.substr(0, AlgoSV.size()) == AlgoSV) {
        auto Value = HashExpr.substr(AlgoSV.size());
        if (std::all_of(Value.begin(), Value.end(),
                        [](char C) { return C >= 0x21 && C <= 0x7E; })) {
          ValidAlgo = true;
        }
        break;
      }
    }
    if (!ValidAlgo)
      return false;

    HasToken = true;
  }

  return HasToken;
}

// Parses a non-negative integer without leading zeros.
// Returns the end position, or npos on failure.
size_t parseNumeric(std::string_view V) {
  if (V.empty())
    return std::string_view::npos;
  if (V[0] == '0') {
    return 1;
  }
  if (V[0] >= '1' && V[0] <= '9') {
    size_t Pos = 1;
    while (Pos < V.size() && isdigit(V[Pos]))
      Pos++;
    return Pos;
  }
  return std::string_view::npos;
}

// canonversion ::= [1-9] [0-9]*
//                | '0.' [1-9] [0-9]*
//                | '0.0.' [1-9] [0-9]*
bool isCanonVersion(std::string_view V) {
  if (V.empty())
    return false;

  // canonversion ::= [1-9] [0-9]* | '0.' [1-9] [0-9]* | '0.0.' [1-9] [0-9]*
  for (int I = 0; I < 3; I++) {
    if (V[0] >= '1' && V[0] <= '9') {
      size_t End = parseNumeric(V);
      return End == V.size();
    }
    if (!tryRead("0."sv, V) || V.empty())
      return false;
  }

  return false;
}

// Validates a dot-separated pre-release or build identifier segment.
// Each identifier is [0-9A-Za-z-]+.
// Numeric identifiers must not have leading zeros.
bool isPreReleaseOrBuild(std::string_view V, bool CheckLeadingZeros) {
  if (V.empty())
    return false;
  size_t Start = 0;
  while (Start < V.size()) {
    size_t DotPos = V.find('.', Start);
    std::string_view Ident =
        (DotPos == V.npos) ? V.substr(Start) : V.substr(Start, DotPos - Start);
    if (Ident.empty())
      return false;
    for (char C : Ident) {
      if (!isalnum(C) && C != '-')
        return false;
    }
    if (CheckLeadingZeros) {
      bool AllDigits = std::all_of(Ident.begin(), Ident.end(),
                                   [](char C) { return isdigit(C); });
      if (AllDigits && Ident.size() > 1 && Ident[0] == '0')
        return false;
    }
    if (DotPos == V.npos)
      break;
    Start = DotPos + 1;
  }
  return true;
}

// MAJOR.MINOR.PATCH[-prerelease][+build] per semver.org 2.0
bool isValidSemver(std::string_view V) {
  if (V.empty())
    return false;

  // Parse MAJOR.MINOR.PATCH
  for (int I = 0; I < 3; I++) {
    size_t End = parseNumeric(V);
    if (End == std::string_view::npos)
      return false;
    if (I < 2) {
      if (End >= V.size() || V[End] != '.')
        return false;
      V.remove_prefix(End + 1);
    } else {
      V.remove_prefix(End);
    }
  }

  if (V.empty())
    return true;

  if (V[0] == '-') {
    V.remove_prefix(1);
    size_t PlusPos = V.find('+');
    std::string_view PreRelease =
        (PlusPos == V.npos) ? V : V.substr(0, PlusPos);
    if (!isPreReleaseOrBuild(PreRelease, true))
      return false;
    if (PlusPos == V.npos)
      return true;
    V.remove_prefix(PlusPos);
  }

  if (!V.empty() && V[0] == '+') {
    V.remove_prefix(1);
    return isPreReleaseOrBuild(V, false);
  }

  return V.empty();
}

bool isVersion(std::string_view V) {
  return isCanonVersion(V) || isValidSemver(V);
}

Unexpected<ErrCode> reportError(std::string_view Reason) {
  spdlog::error(ErrCode::Value::ComponentInvalidName);
  spdlog::error("    Component name: {}"sv, Reason);
  return Unexpect(ErrCode::Value::ComponentInvalidName);
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
  if (!tryRead(",integrity=<"sv, Next))
    return reportError("expected ',integrity=<' after "sv);
  std::string_view IntegrityData;
  if (!readUntil(Next, '>', IntegrityData))
    return reportError("expected '>' closing integrity"sv);
  if (!isIntegrityMetadata(IntegrityData))
    return reportError("invalid integrity metadata"sv);
  if (!isEOF(Next))
    return reportError("unexpected trailing content after integrity"sv);
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
  std::string_view Namespace;
  if (!readUntil(Next, ':', Namespace))
    return reportError("expected ':' in namespace"sv);
  if (!isLowercaseKebabString(Namespace))
    return reportError("invalid namespace"sv);

  size_t PkgEnd = Next.find_first_of(StopChars);
  if (PkgEnd == Next.npos)
    return reportError("unterminated package name"sv);
  std::string_view Package = Next.substr(0, PkgEnd);
  Next.remove_prefix(PkgEnd);
  if (!isLowercaseKebabString(Package))
    return reportError("invalid package name"sv);

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
      return reportError("invalid label after [constructor]"sv);
    }
    Result.Detail.emplace<ConstructorDetail>(ConstructorDetail{Next});
    Result.NoTagName = Next;
    Result.Kind = ComponentNameKind::Constructor;
    return Result;
  }

  auto tryReadResourceWithLabel = [&](std::string_view Tag,
                                      std::string_view &Resource,
                                      std::string_view &Label) -> bool {
    auto Saved = Next;
    if (!tryRead(Tag, Next)) {
      return false;
    }
    auto TmpNoTagName = Next;
    if (!readUntil(Next, '.', Resource)) {
      Next = Saved;
      return false;
    }
    if (!isKebabString(Resource) || !isKebabString(Next)) {
      Next = Saved;
      return false;
    }
    Result.NoTagName = TmpNoTagName;
    Label = Next;
    return true;
  };

  {
    std::string_view Resource, Label;
    if (tryReadResourceWithLabel("[method]"sv, Resource, Label)) {
      Result.Detail.emplace<MethodDetail>(MethodDetail{Resource, Label});
      Result.Kind = ComponentNameKind::Method;
      return Result;
    }
  }

  {
    std::string_view Resource, Label;
    if (tryReadResourceWithLabel("[static]"sv, Resource, Label)) {
      Result.Detail.emplace<StaticDetail>(StaticDetail{Resource, Label});
      Result.Kind = ComponentNameKind::Static;
      return Result;
    }
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
    if (!tryRead("<"sv, Next))
      return reportError("expected '<' after unlocked-dep="sv);

    EXPECTED_TRY(auto Path, parsePkgPath(Next, "@>"sv));

    // verrange ::= '@*'
    //            | '@{' verlower '}'
    //            | '@{' verupper '}'
    //            | '@{' verlower ' ' verupper '}'
    std::string_view VersionRange;
    if (!Next.empty() && Next[0] == '@') {
      auto VerStart = Next;
      Next.remove_prefix(1);
      if (Next.empty())
        return reportError(
            "expected version range after '@' in unlocked-dep"sv);

      if (Next[0] == '*') {
        Next.remove_prefix(1);
      } else if (Next[0] == '{') {
        size_t ClosePos = Next.find('}');
        if (ClosePos == Next.npos)
          return reportError("expected '}' in unlocked-dep version range"sv);
        auto RangeBody = Next.substr(1, ClosePos - 1);

        auto ValidateRange = [](std::string_view Body) -> bool {
          if (Body.empty())
            return false;
          auto Remaining = Body;

          if (tryRead(">="sv, Remaining)) {
            size_t SpacePos = Remaining.find(' ');
            std::string_view Lower = (SpacePos == Remaining.npos)
                                         ? Remaining
                                         : Remaining.substr(0, SpacePos);
            if (!isValidSemver(Lower))
              return false;
            if (SpacePos == Remaining.npos)
              return true;
            Remaining.remove_prefix(SpacePos + 1);
            if (!tryRead("<"sv, Remaining))
              return false;
            return isValidSemver(Remaining);
          }

          if (tryRead("<"sv, Remaining)) {
            return isValidSemver(Remaining);
          }

          return false;
        };

        if (!ValidateRange(RangeBody))
          return reportError("invalid version range in unlocked-dep"sv);

        Next.remove_prefix(ClosePos + 1);
      } else {
        return reportError("expected '*' or '{' after '@' in unlocked-dep"sv);
      }
      VersionRange = VerStart.substr(0, VerStart.size() - Next.size());
    }

    if (!tryRead(">"sv, Next))
      return reportError("expected '>' closing unlocked-dep"sv);

    if (!isEOF(Next))
      return reportError("unexpected trailing content after unlocked-dep"sv);

    Result.Detail.emplace<UnlockedDepDetail>(
        UnlockedDepDetail{Path.Namespace, Path.Package, VersionRange});
    Result.Kind = ComponentNameKind::UnlockedDep;
    return Result;
  }

  if (tryRead("locked-dep="sv, Next)) {
    if (!tryRead("<"sv, Next))
      return reportError("expected '<' after locked-dep="sv);

    EXPECTED_TRY(auto Path, parsePkgPath(Next, "@>"sv));

    std::string_view Version;
    if (!Next.empty() && Next[0] == '@') {
      Next.remove_prefix(1);
      size_t VerEnd = Next.find('>');
      if (VerEnd == Next.npos)
        return reportError("expected '>' after version in locked-dep"sv);
      Version = Next.substr(0, VerEnd);
      Next.remove_prefix(VerEnd);
      if (!isValidSemver(Version))
        return reportError("invalid semver in locked-dep"sv);
    }

    if (!tryRead(">"sv, Next))
      return reportError("expected '>' closing locked-dep"sv);

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
    if (!tryRead("<"sv, Next))
      return reportError("expected '<' after url="sv);

    size_t ClosePos = Next.find('>');
    if (ClosePos == Next.npos)
      return reportError("expected '>' closing url"sv);

    std::string_view UrlContent = Next.substr(0, ClosePos);
    if (UrlContent.find('<') != UrlContent.npos)
      return reportError("'<' not allowed inside url"sv);
    Next.remove_prefix(ClosePos + 1);

    std::string_view Integrity;
    EXPECTED_TRY(tryParseIntegritySuffix(Next, Integrity));

    Result.Detail.emplace<UrlDetail>(UrlDetail{UrlContent, Integrity});
    Result.Kind = ComponentNameKind::Url;
    return Result;
  }

  // hashname ::= 'integrity=<' <integrity-metadata> '>'
  if (tryRead("integrity="sv, Next)) {
    if (!tryRead("<"sv, Next))
      return reportError("expected '<' after integrity="sv);
    std::string_view IntegrityData;
    if (!readUntil(Next, '>', IntegrityData))
      return reportError("expected '>' closing integrity"sv);
    if (!isIntegrityMetadata(IntegrityData))
      return reportError("invalid integrity metadata"sv);
    if (!isEOF(Next))
      return reportError("unexpected trailing content after integrity"sv);
    Result.Detail.emplace<IntegrityDetail>(IntegrityDetail{IntegrityData});
    Result.Kind = ComponentNameKind::Integrity;
    return Result;
  }

  // interfacename ::= <namespace> <label> <projection> <interfaceversion>?
  // namespace     ::= <words> ':'
  // projection    ::= '/' <label>
  // interfaceversion ::= '@' <valid semver> | '@' <canonversion>
  {
    std::string_view Namespace, Package, Interface, Version;

    int Counter = 0;
    while (readUntil(Next, ':', Namespace)) {
      Counter++;
      if (!isLowercaseKebabString(Namespace)) {
        return reportError("invalid namespace in interface name"sv);
      }
    }
    if (Counter == 0) {
      // No ':' found — fall through to label parsing below.
      goto ParseLabel;
    }
    if (Counter != 1) {
      return reportError("nested namespaces not supported yet"sv);
    }

    // interfacename ::= <namespace> <words> <projection> ...
    if (!tryReadKebab(Next, Package) || !isLowercaseKebabString(Package)) {
      return reportError("invalid package in interface name"sv);
    }

    Counter = 0;
    while (!isEOF(Next) && Next[0] == '/') {
      Next.remove_prefix(1);
      Counter++;
      if (!tryReadKebab(Next, Interface)) {
        return reportError("invalid projection label in interface name"sv);
      }
    }

    if (Counter == 0) {
      return reportError("expected '/' projection in interface name"sv);
    }
    if (Counter != 1) {
      return reportError("nested projections not supported yet"sv);
    }

    if (!isEOF(Next) && Next[0] == '@') {
      Next.remove_prefix(1);
      Version = Next;
      if (!isVersion(Version)) {
        return reportError("invalid version in interface name"sv);
      }
    }

    Result.Detail.emplace<InterfaceDetail>(
        InterfaceDetail{Namespace, Package, Interface, Version});
    Result.Kind = ComponentNameKind::InterfaceType;
    return Result;
  }

ParseLabel:
  if (!isKebabString(Next)) {
    return reportError("invalid label"sv);
  }
  Result.Detail.emplace<LabelDetail>();
  Result.Kind = ComponentNameKind::Label;
  return Result;
}

} // namespace Validator
} // namespace WasmEdge
