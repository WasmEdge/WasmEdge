// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/compiler/common.h - common definitions in vm -----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the common-use definitions in Compiler.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace SSVM {
namespace Compiler {

/// Compiler error code enum class.
enum class ErrCode : unsigned int {
  Success = 0,
  Failed,
  TypeNotMatch, /// Value type between instructions and stack or store not match
  DivideByZero, /// Divide by zero
  FloatPointException, /// Floating point exception
  Unreachable,         /// Get a unreachable instruction
  FunctionInvalid,     /// Invalid operation to function instance.
  Terminated,          /// Forced terminated by program and return success.
};

template <typename T> class Span {
public:
  Span(T B, T E) : Begin(B), End(E) {}

  const T &begin() const { return Begin; }
  const T &end() const { return End; }
  auto size() const { return End - Begin; }

private:
  T Begin, End;
};

} // namespace Compiler
} // namespace SSVM
