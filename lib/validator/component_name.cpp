// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "validator/component_name.h"

#include "ast/component/component.h"
#include "ast/module.h"

#include <deque>
#include <optional>
#include <unordered_map>
#include <vector>

namespace WasmEdge {
namespace Validator {

namespace ComponentNameParser {

// label             ::= <fragment>
//                     | <label> '-' <fragment>
// fragment          ::= <word>
//                     | <acronym>
// word              ::= [a-z] [0-9a-z]*
// acronym           ::= [A-Z] [0-9A-Z]*
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

// words             ::= <word>
//                     | <words> '-' <word>
// word              ::= [a-z] [0-9a-z]*
bool isLowercaseKebabString(std::string_view Input) {
  return isKebabString(Input) &&
         std::all_of(Input.begin(), Input.end(), [](char c) {
           return c == '-' || islower(c) || isdigit(c);
         });
}

bool isEOF(std::string_view Input) { return Input.empty(); }

bool readUntil(std::string_view &Input, char delim, std::string_view &output) {
  size_t Pos = Input.find(delim);
  if (Pos == Input.npos) {
    return false;
  }

  output = Input.substr(0, Pos);
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

bool isSemVer(std::string_view Input) {
  auto IsNumeric = [](std::string_view S) {
    if (S.empty()) return false;
    if (S.size() > 1 && S[0] == '0') return false;
    return std::all_of(S.begin(), S.end(), [](char C) { return std::isdigit(C); });
  };

  auto IsIdentifier = [](std::string_view S) {
    if (S.empty()) return false;
    return std::all_of(S.begin(), S.end(), [](char C) {
      return std::isalnum(C) || C == '-';
    });
  };

  std::string_view MajorStr, MinorStr, PatchStr;

  size_t Dot1 = Input.find('.');
  if (Dot1 == std::string_view::npos) return false;
  MajorStr = Input.substr(0, Dot1);
  if (!IsNumeric(MajorStr)) return false;
  Input.remove_prefix(Dot1 + 1);

  size_t Dot2 = Input.find('.');
  if (Dot2 == std::string_view::npos) return false;
  MinorStr = Input.substr(0, Dot2);
  if (!IsNumeric(MinorStr)) return false;
  Input.remove_prefix(Dot2 + 1);

  size_t EndPatch = Input.find_first_of("-+");
  if (EndPatch == std::string_view::npos) {
    PatchStr = Input;
    Input = "";
  } else {
    PatchStr = Input.substr(0, EndPatch);
    Input.remove_prefix(EndPatch);
  }
  if (!IsNumeric(PatchStr)) return false;

  if (!Input.empty() && Input[0] == '-') {
    Input.remove_prefix(1);
    if (Input.empty()) return false;
    size_t EndPre = Input.find('+');
    std::string_view Prerelease;
    if (EndPre == std::string_view::npos) {
      Prerelease = Input;
      Input = "";
    } else {
      Prerelease = Input.substr(0, EndPre);
      Input.remove_prefix(EndPre);
    }
    
    while (!Prerelease.empty()) {
      size_t Dot = Prerelease.find('.');
      std::string_view Part = (Dot == std::string_view::npos) ? Prerelease : Prerelease.substr(0, Dot);
      if (!IsIdentifier(Part)) return false;
      
      bool AllDigits = !Part.empty() && std::all_of(Part.begin(), Part.end(), [](char C) { return std::isdigit(C); });
      if (AllDigits && Part.size() > 1 && Part[0] == '0') return false;
      
      if (Dot == std::string_view::npos) break;
      Prerelease.remove_prefix(Dot + 1);
      if (Prerelease.empty()) return false;
    }
  }

  if (!Input.empty() && Input[0] == '+') {
    Input.remove_prefix(1);
    if (Input.empty()) return false;
    std::string_view Build = Input;
    Input = "";
    
    while (!Build.empty()) {
      size_t Dot = Build.find('.');
      std::string_view Part = (Dot == std::string_view::npos) ? Build : Build.substr(0, Dot);
      if (!IsIdentifier(Part)) return false;
      if (Dot == std::string_view::npos) break;
      Build.remove_prefix(Dot + 1);
      if (Build.empty()) return false;
    }
  }

  return Input.empty();
}

} // namespace ComponentNameParser

using namespace std::literals;
using namespace ComponentNameParser;

// exportname        ::= <plainname>
//                     | <interfacename>
// importname        ::= <exportname>
//                     | <depname>
//                     | <urlname>
//                     | <hashname>
void ComponentName::parse() {
  auto Next = getOriginalName();
  Kind = ComponentNameKind::Invalid;
  Detail = {};

  // plainname         ::= <label>
  //                    | '[async]' <label> 🔀
  //                    | '[constructor]' <label>
  //                    | '[method]' <label> '.' <label>
  //                    | '[async method]' <label> '.' <label> 🔀
  //                    | '[static]' <label> '.' <label>
  //                    | '[async static]' <label> '.' <label> 🔀

  if (tryRead("[constructor]"sv, Next)) {
    if (!isKebabString(Next)) {
      return;
    }
    Detail.Constructor.Label = Next;
    NoTagName = Next;
    Kind = ComponentNameKind::Constructor;
    return;
  }

  auto tryReadResourceWithLabel = [&](std::string_view Tag,
                                      std::string_view &Resource,
                                      std::string_view &Label) -> bool {
    if (!tryRead(Tag, Next)) {
      return false;
    }
    auto TmpNoTagName = Next;
    if (!readUntil(Next, '.', Resource)) {
      return false;
    }
    if (!isKebabString(Resource) || !isKebabString(Next)) {
      return false;
    }
    NoTagName = TmpNoTagName;
    Label = Next;
    return true;
  };

  if (tryReadResourceWithLabel("[method]"sv, Detail.Method.Resource,
                               Detail.Method.Method)) {
    Kind = ComponentNameKind::Method;
    return;
  }

  if (tryReadResourceWithLabel("[static]"sv, Detail.Static.Resource,
                               Detail.Static.Method)) {
    Kind = ComponentNameKind::Static;
    return;
  }

  if (tryRead("[async]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  if (tryRead("[async method]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  if (tryRead("[async static]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  // No tag more
  if (Next.size() != 0 && Next[0] == '[') {
    return;
  }
  NoTagName = Next;

  // depname           ::= 'unlocked-dep=<' <pkgnamequery> '>'
  //                     | 'locked-dep=<' <pkgname> '>' ( ',' <hashname> )?

  if (tryRead("unlocked-dep="sv, Next)) {
    // Not supported yet
    return;
  }

  if (tryRead("locked-dep="sv, Next)) {
    // Not supported yet
    return;
  }

  // urlname           ::= 'url=<' <nonbrackets> '>' (',' <hashname>)?
  if (tryRead("url="sv, Next)) {
    // Not supported yet
    return;
  }

  // hashname          ::= 'integrity=<' <integrity-metadata> '>'
  if (tryRead("integrity="sv, Next)) {
    if (Next.size() >= 2 && Next.front() == '<' && Next.back() == '>') {
      Detail.Hash.HashValue = Next.substr(1, Next.size() - 2);
      Kind = ComponentNameKind::Hash;
      return;
    }
    return;
  }

  // Interface name or label
  // interfacename     ::= <namespace> <label> <projection> <interfaceversion>?
  //                     | <namespace>+ <label> <projection>+
  //                     <interfaceversion>? 🪺
  // namespace         ::= <words> ':'
  // words             ::= <word>
  //                   | <words> '-' <word>

  if (Next.find(':') != Next.npos) {
    std::string_view Namespace, Package, Interface, Projection, Version;

    // read [a:b:]c/d/e/f@g
    int Counter = 0;
    while (readUntil(Next, ':', Namespace)) {
      Counter++;
      if (!isLowercaseKebabString(Namespace)) {
        return;
      }
    }
    if (Counter == 0) {
      return;
    }
    if (Counter != 1) {
      // TODO: nest namespace not supported yet
      return;
    }

    // read a:b:[c]/d/e/f@g
    if (!tryReadKebab(Next, Package)) {
      return;
    }

    // read a:b:c[/d/e/f]@g
    Counter = 0;
    while (!isEOF(Next) && Next[0] == '/') {
      Next.remove_prefix(1);
      Counter++;
      if (!tryReadKebab(Next, Interface)) {
        return;
      }
    }

    if (Counter == 0) {
      return;
    }
    if (Counter != 1) {
      // TODO: nest interface not supported yet
      return;
    }

    // read a:b:c/d/e/f[@g]?
    if (!isEOF(Next) && Next[0] == '@') {
      Next.remove_prefix(1);
      if (!isSemVer(Next)) {
        return;
      }
      Version = Next;
    }

    Detail.Interface.Namespace = Namespace;
    Detail.Interface.Package = Package;
    Detail.Interface.Interface = Interface;
    Detail.Interface.Projection = Projection;
    Detail.Interface.Version = Version;
    Kind = ComponentNameKind::InterfaceType;
  } else {
    // label
    if (!isKebabString(Next)) {
      return;
    }
    Kind = ComponentNameKind::Label;
  }
}

} // namespace Validator
} // namespace WasmEdge
