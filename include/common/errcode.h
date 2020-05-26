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

#include <map>
#include <string>

#include "support/expected.h"

namespace SSVM {

/// Wasm runtime phasing enumeration class.
enum class WasmPhase : uint8_t {
  VM = 0x00,
  Loading = 0x01,
  Validation = 0x02,
  Execution = 0x03
};

static std::map<uint32_t, std::string> ErrStr = {
    {0x0000, "Success"},
    {0x0001, "Terminated"},
    {0x0002, "Revert"},
    {0x0003, "cost limit exceeded"},
    {0x0004, "wrong VM workflow"},
    {0x1000, "invalid path"},
    {0x1001, "read error"},
    {0x1002, "read end of file"},
    {0x1003, "invalid wasm grammar"},
    {0x1004, "invalid version"},
    {0x2000, "validation failed"},
    {0x3000, "module name conflict"},
    {0x3001, "incompatible import type"},
    {0x3002, "uninitialized element"},
    {0x3003, "undefined element"},
    {0x3004, "unknown import"},
    {0x3005, "data segment does not fit"},
    {0x3006, "elements segment does not fit"},
    {0x3007, "wrong instance address"},
    {0x3008, "value type mismatch"},
    {0x3009, "stack empty"},
    {0x300A, "instruction type mismatch"},
    {0x300B, "funcion signature mismatch"},
    {0x300C, "integer divide by zero"},
    {0x300D, "integer overflow"},
    {0x300E, "invalid conversion to integer"},
    {0x300F, "out of bounds memory access"},
    {0x3010, "unreachable"},
    {0x3011, "indirect call type mismatch"},
    {0x3012, "host function failed"}};

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
  ModuleNameConflict = 0x3000,     /// Module name conflicted when importing.
  IncompatibleImportType = 0x3001, /// Import matching failed
  UninitializedElement = 0x3002,   /// Uninitialized element in table instance
  UndefinedElement = 0x3003,  /// Access undefined element in table instances
  UnknownImport = 0x3004,     /// Unknown import instances
  DataSegDoesNotFit = 0x3005, /// Init failed when instantiating data segment
  ElemSegDoesNotFit = 0x3006, /// Init failed when instantiating element segment
  WrongInstanceAddress = 0x3007,    /// Wrong access of instances
  ValueTypeMismatch = 0x3008,       /// Value type not match
  StackEmpty = 0x3009,              /// Empry stack when get or pop entry
  InstructionTypeMismatch = 0x300A, /// Instruction type not match
  FuncSigMismatch = 0x300B,   /// Function signature not match when invoking
  DivideByZero = 0x300C,      /// Divide by zero
  IntegerOverflow = 0x300D,   /// Integer overflow
  InvalidConvToInt = 0x300E,  /// Cannot do convert to integer
  MemoryOutOfBounds = 0x300F, /// Out of bounds memory access
  Unreachable = 0x3010,       /// Get a unreachable instruction
  IndirectCallTypeMismatch = 0x3011, /// Func type mismatch in call_indirect
  ExecutionFailed = 0x3012           /// Host function execution failed
};

class ErrMsg {
public:
  ErrMsg() = delete;
  ErrMsg(const ErrMsg &EM) : Code(EM.Code), Msg(EM.Msg) {}
  ErrMsg(ErrMsg &&EM) : Code(EM.Code), Msg(std::move(EM.Msg)) {}

private:
  ErrCode Code;
  std::string Msg;
};

static inline constexpr bool likely(bool V) {
  return __builtin_expect(V, true);
}
static inline constexpr bool unlikely(bool V) {
  return __builtin_expect(V, false);
}

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
