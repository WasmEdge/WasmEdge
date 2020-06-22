// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/po/helper.h - Helper for Initialize Option -------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include <string>

namespace SSVM {
namespace PO {

struct Description {
  Description(std::string Value) : Value(std::move(Value)) {}
  std::string Value;
};

struct MetaVar {
  MetaVar(std::string Value) : Value(std::move(Value)) {}
  std::string Value;
};

template <typename T> struct DefaultValue {
  DefaultValue(T Value) : Value(std::move(Value)) {}
  T Value;
};

struct ZeroOrMore {};
struct OneOrMore {};

struct Hidden {};

} // namespace PO
} // namespace SSVM
