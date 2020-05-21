// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/errcode.h - Error code definition ---------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of SSVM error code and handler.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "support/expected.h"

namespace SSVM {

/// Error code enumeration class.
enum class ErrCode : uint32_t {
  Success = 0x0000,
  Terminated = 0x0001,        /// Exit and return success.
  Revert = 0x0002,            /// Revert by evm.
  CostLimitExceeded = 0x0003, /// Exceeded cost limit (out of gas).
  WrongVMWorkflow = 0x0004,   /// Wrong VM's workflow
  /// Load phase
  InvalidPath = 0x1000,    /// File not found
  ReadError = 0x1001,      /// Error when reading
  EndOfFile = 0x1002,      /// Reach end of file when reading
  InvalidGrammar = 0x1003, /// Parsing error
  InvalidVersion = 0x1004, /// Unsupported version
  /// Validation phase
  ValidationFailed = 0x2000, /// Validation checking failed
  /// Execution phase
  ImportNotMatch = 0x3000,          /// Import matching failed
  ModuleNameConflict = 0x3001,      /// Module name conflicted when importing.
  WrongInstanceAddress = 0x3002,    /// Wrong access of instances
  TypeNotMatch = 0x3003,            /// Value type not match
  StackEmpty = 0x3004,              /// Empry stack when get or pop entry
  InstructionTypeMismatch = 0x3005, /// Instruction type not match
  FunctionSignatureMismatch =
      0x3006,                   /// Function signature not match when invoking
  DivideByZero = 0x3007,        /// Divide by zero
  FloatPointException = 0x3008, /// Floating point exception
  CastingError = 0x3009,        /// Cannot do type casting
  AccessForbidMemory = 0x300A,  /// Access the forbid memory section
  Unreachable = 0x300B,         /// Get a unreachable instruction
  TableSizeExceeded =
      0x300C, /// Exceeded limit of table size in table instance.
  MemorySizeExceeded =
      0x300D /// Exceeded limit of memory page in memory instance.
};

static inline bool likely(bool V) { return __builtin_expect(V, true); }
static inline bool unlikely(bool V) { return __builtin_expect(V, false); }

/// Type aliasing for Expected<T, ErrMsg>.
template <typename T> using Expect = Support::Expected<T, ErrCode>;

/// Helper function for Unexpected<ErrMsg>.
constexpr auto Unexpect(const ErrCode &Val) {
  return Support::Unexpected<ErrCode>(Val);
}
template <typename T> constexpr auto Unexpect(const Expect<T> &Val) {
  return Support::Unexpected<ErrCode>(Val.error());
}

} // namespace SSVM
