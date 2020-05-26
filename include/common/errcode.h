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
  SSVM = 0x00,
  Loading = 0x01,
  Validation = 0x02,
  Instantiation = 0x03,
  Execution = 0x04
};

/// Wasm runtime phasing enumeration string mapping.
static std::map<WasmPhase, std::string> WasmPhaseStr = {
    {WasmPhase::SSVM, "ssvm runtime"},
    {WasmPhase::Loading, "loading"},
    {WasmPhase::Validation, "validation"},
    {WasmPhase::Instantiation, "instantiation"},
    {WasmPhase::Execution, "execution"}};

/// Error code enumeration class.
enum class ErrCode : uint8_t {
  Success = 0x00,
  Terminated = 0x01,        /// Exit and return success.
  Revert = 0x02,            /// Revert by evm.
  CostLimitExceeded = 0x03, /// Exceeded cost limit (out of gas).
  WrongVMWorkflow = 0x04,   /// Wrong VM's workflow
  FuncNotFound = 0x05,      /// Wasm function not found
  /// Load phase
  InvalidPath = 0x10,    /// File not found
  ReadError = 0x11,      /// Error when reading
  EndOfFile = 0x12,      /// Reach end of file when reading
  InvalidGrammar = 0x13, /// Parsing error
  InvalidVersion = 0x14, /// Unsupported version
  /// Validation phase
  ValidationFailed = 0x20, /// Validation checking failed
  /// Instantiation phase
  ModuleNameConflict = 0x30,     /// Module name conflicted when importing.
  IncompatibleImportType = 0x31, /// Import matching failed
  UnknownImport = 0x32,          /// Unknown import instances
  DataSegDoesNotFit = 0x33,      /// Init failed when instantiating data segment
  ElemSegDoesNotFit = 0x34, /// Init failed when instantiating element segment
  WrongInstanceAddress = 0x35, /// Wrong access of instances
  /// Execution phase
  StackEmpty = 0x40,           /// Empry stack when get or pop entry
  InstrTypeMismatch = 0x41,    /// Instruction type not match
  FuncSigMismatch = 0x42,      /// Function signature not match when invoking
  DivideByZero = 0x43,         /// Divide by zero
  IntegerOverflow = 0x44,      /// Integer overflow
  InvalidConvToInt = 0x45,     /// Cannot do convert to integer
  MemoryOutOfBounds = 0x46,    /// Out of bounds memory access
  Unreachable = 0x47,          /// Meet an unreachable instruction
  UninitializedElement = 0x48, /// Uninitialized element in table instance
  UndefinedElement = 0x49,     /// Access undefined element in table instances
  IndirectCallTypeMismatch = 0x4A, /// Func type mismatch in call_indirect
  ExecutionFailed = 0x4B           /// Host function execution failed
};

/// Error code enumeration string mapping.
static std::map<ErrCode, std::string> ErrCodeStr = {
    {ErrCode::Success, "success"},
    {ErrCode::Terminated, "terminated"},
    {ErrCode::Revert, "revert"},
    {ErrCode::CostLimitExceeded, "cost limit exceeded"},
    {ErrCode::WrongVMWorkflow, "wrong VM workflow"},
    {ErrCode::FuncNotFound, "wasm function not found"},
    {ErrCode::InvalidPath, "invalid path"},
    {ErrCode::ReadError, "read error"},
    {ErrCode::EndOfFile, "read end of file"},
    {ErrCode::InvalidGrammar, "invalid wasm grammar"},
    {ErrCode::InvalidVersion, "invalid version"},
    {ErrCode::ValidationFailed, "validation failed"},
    {ErrCode::ModuleNameConflict, "module name conflict"},
    {ErrCode::IncompatibleImportType, "incompatible import type"},
    {ErrCode::UnknownImport, "unknown import"},
    {ErrCode::DataSegDoesNotFit, "data segment does not fit"},
    {ErrCode::ElemSegDoesNotFit, "elements segment does not fit"},
    {ErrCode::WrongInstanceAddress, "wrong instance address"},
    {ErrCode::StackEmpty, "stack empty"},
    {ErrCode::InstrTypeMismatch, "instruction type mismatch"},
    {ErrCode::FuncSigMismatch, "funcion signature mismatch"},
    {ErrCode::DivideByZero, "integer divide by zero"},
    {ErrCode::IntegerOverflow, "integer overflow"},
    {ErrCode::InvalidConvToInt, "invalid conversion to integer"},
    {ErrCode::MemoryOutOfBounds, "out of bounds memory access"},
    {ErrCode::Unreachable, "unreachable"},
    {ErrCode::UninitializedElement, "uninitialized element"},
    {ErrCode::UndefinedElement, "undefined element"},
    {ErrCode::IndirectCallTypeMismatch, "indirect call type mismatch"},
    {ErrCode::ExecutionFailed, "host function failed"}};

static inline WasmPhase getErrCodePhase(ErrCode Code) {
  return static_cast<WasmPhase>((static_cast<uint8_t>(Code) & 0xF0) >> 4);
}

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
