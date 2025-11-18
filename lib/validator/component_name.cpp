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
bool isKebabString(std::string_view input) {
  bool isFirstPart = true;
  bool Uppercase = false;
  bool Lowercase = false;
  bool Digit = false;

  for (char c : input) {
    if (islower(c)) {
      if (Uppercase)
        return false;
      Lowercase = true;
    } else if (isupper(c)) {
      if (Lowercase)
        return false;
      Uppercase = true;
    } else if (isdigit(c)) {
      if (isFirstPart && !(Uppercase || Lowercase))
        return false;
      Digit = true;
    } else if (c == '-') {
      if (Uppercase || Lowercase || Digit) {
        isFirstPart = false;
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

  return input.size() > 0 && input.back() != '-';
}

// words             ::= <word>
//                     | <words> '-' <word>
// word              ::= [a-z] [0-9a-z]*
bool isLowercaseKebabString(std::string_view input) {
  return isKebabString(input) &&
         std::all_of(input.begin(), input.end(), [](char c) {
           return c == '-' || islower(c) || isdigit(c);
         });
}

bool isEOF(std::string_view input) { return input.empty(); }

bool readUntil(std::string_view &input, char delim, std::string_view &output) {
  size_t Pos = input.find(delim);
  if (Pos == input.npos) {
    return false;
  }

  output = input.substr(0, Pos);
  input.remove_prefix(Pos + 1);
  return true;
}

bool tryRead(std::string_view prefix, std::string_view &name) {
  if (prefix.size() > name.size())
    return false;
  if (prefix != name.substr(0, prefix.size()))
    return false;

  name.remove_prefix(prefix.size());
  return true;
}

bool tryReadKebab(std::string_view &input, std::string_view &output) {
  size_t Pos = 0;
  while (Pos < input.size()) {
    if (isalnum(input[Pos]) || input[Pos] == '-') {
      Pos++;
    } else {
      break;
    }
  }
  output = input.substr(0, Pos);
  input.remove_prefix(Pos);
  return isKebabString(output);
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
  auto Next = Name;
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
    // Not supported yet
    return;
  }

  if (tryRead("[constructor]"sv, Next)) {
    if (!isKebabString(Next)) {
      return;
    }
    Detail.Constructor.Label = Next;
    Kind = ComponentNameKind::Constructor;
    return;
  }

  if (tryRead("[method]"sv, Next)) {
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
    // Not supported yet
    return;
  }

  if (tryRead("[static]"sv, Next)) {
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
    // Not supported yet
    return;
  }

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
