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

/// Value types enumeration class.
enum class ErrCode : unsigned char {
  Success = 0,
  Terminated, /// Exit and return success.
  /// Load phase
  InvalidPath,    /// File not found
  ReadError,      /// Error when reading
  EndOfFile,      /// Reach end of file when reading
  InvalidGrammar, /// Parsing error
  /// Validation phase
  ValidationFailed, /// Validation checking failed
  /// Execution phase
  WrongExecutorFlow,    /// Wrong executor's flow
  WrongWorkerFlow,      /// Wrong worker's flow
  InstantiateFailed,    /// Fail when instantiating
  WrongInstanceAddress, /// Wrong access of instances
  ImportNotMatch,       /// Import matching failed
  ExecutionFailed,      /// Runtime error when executing
  TypeNotMatch, /// Value type between instructions and stack or store not match
  StackWrongEntry,         /// Entry type not match when get or pop entry
  StackEmpty,              /// Empry stack when get or pop entry
  WrongLocalAddress,       /// Wrong access of local variable
  InstructionTypeMismatch, /// Instruction type not match
  DivideByZero,            /// Divide by zero
  FloatPointException,     /// Floating point exception
  CastingError,            /// Cannot do type casting
  Unimplemented,           /// Instruction is unimplemented
  AccessForbidMemory,      /// Access the forbid memory section
  SliceDataFailed,         /// Fail to get slice from memory.data
  Unreachable,             /// Get a unreachable instruction
  WrongInstructionCounter, /// Instruction counter error
  TableSizeExceeded,       /// Exceeded limit of table size in table instance.
  MemorySizeExceeded,      /// Exceeded limit of memory page in memory instance.
  FunctionInvalid,         /// Invalid operation to function instance.
  CallFunctionError,       /// Arguement not match function type.
  CostLimitExceeded,       /// Exceeded cost limit (out of gas).
  Revert                   /// Revert by evm.
};

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
