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
  if (tryRead("[async]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  if (tryRead("[constructor]"sv, Next)) {
    if (!isKebabString(Next)) {
      return;
    }
    Detail.Constructor.Label = Next;
    NoTagName = Next;
    Kind = ComponentNameKind::Constructor;
    return;
  }

  if (tryRead("[method]"sv, Next)) {
    NoTagName = Next;
    std::string_view Resource;
    if (!readUntil(Next, '.', Resource)) {
      return;
    }
    if (!isKebabString(Resource) || !isKebabString(Next)) {
      return;
    }
    Detail.Method.Resource = Resource;
    Detail.Method.Method = Next;
    Kind = ComponentNameKind::Method;
    return;
  }

  if (tryRead("[async method]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  if (tryRead("[static]"sv, Next)) {
    NoTagName = Next;
    std::string_view Resource;
    if (!readUntil(Next, '.', Resource)) {
      return;
    }
    if (!isKebabString(Resource) || !isKebabString(Next)) {
      return;
    }
    Detail.Static.Resource = Resource;
    Detail.Static.Method = Next;
    Kind = ComponentNameKind::Static;
    return;
  }

  if (tryRead("[async static]"sv, Next)) {
    NoTagName = Next;
    // Not supported yet
    return;
  }

  // No tag more
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
    // Not supported yet
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
      // TODO: semver format check
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
