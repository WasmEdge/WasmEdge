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

#include "errinfo.h"
#include "support/expected.h"
#include "support/hexstr.h"

#include <ostream>
#include <string>
#include <unordered_map>

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
static inline std::unordered_map<WasmPhase, std::string> WasmPhaseStr = {
    {WasmPhase::SSVM, "ssvm runtime"},
    {WasmPhase::Loading, "loading"},
    {WasmPhase::Validation, "validation"},
    {WasmPhase::Instantiation, "instantiation"},
    {WasmPhase::Execution, "execution"}};

/// Error code enumeration class.
enum class ErrCode : uint8_t {
  Success = 0x00,
  Terminated = 0x01,        /// Exit and return success.
  CostLimitExceeded = 0x02, /// Exceeded cost limit (out of gas).
  WrongVMWorkflow = 0x03,   /// Wrong VM's workflow
  FuncNotFound = 0x04,      /// Wasm function not found
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
  /// Execution phase
  WrongInstanceAddress = 0x40, /// Wrong access of instances addresses
  WrongInstanceIndex = 0x41,   /// Wrong access of instances indices
  InstrTypeMismatch = 0x42,    /// Instruction type not match
  FuncSigMismatch = 0x43,      /// Function signature not match when invoking
  DivideByZero = 0x44,         /// Divide by zero
  IntegerOverflow = 0x45,      /// Integer overflow
  InvalidConvToInt = 0x46,     /// Cannot do convert to integer
  MemoryOutOfBounds = 0x47,    /// Out of bounds memory access
  Unreachable = 0x48,          /// Meet an unreachable instruction
  UninitializedElement = 0x49, /// Uninitialized element in table instance
  UndefinedElement = 0x4A,     /// Access undefined element in table instances
  IndirectCallTypeMismatch = 0x4B, /// Func type mismatch in call_indirect
  ExecutionFailed = 0x4C           /// Host function execution failed
};

/// Error code enumeration string mapping.
static inline std::unordered_map<ErrCode, std::string> ErrCodeStr = {
    {ErrCode::Success, "success"},
    {ErrCode::Terminated, "terminated"},
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
    {ErrCode::WrongInstanceIndex, "wrong instance index"},
    {ErrCode::InstrTypeMismatch, "instruction type mismatch"},
    {ErrCode::FuncSigMismatch, "function signature mismatch"},
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

static std::ostream &operator<<(std::ostream &OS, ErrCode Code) {
  OS << WasmPhaseStr[getErrCodePhase(Code)] << " failed: " << ErrCodeStr[Code]
     << ", Code: "
     << Support::convertUIntToHexStr(static_cast<uint32_t>(Code), 2);
  return OS;
}

static inline constexpr bool likely(bool V) {
  return __builtin_expect(V, true);
}
static inline constexpr bool unlikely(bool V) {
  return __builtin_expect(V, false);
}

/// Type aliasing for Expected<T, ErrCode>.
template <typename T> using Expect = Support::Expected<T, ErrCode>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto Unexpect(const ErrCode &Val) {
  return Support::Unexpected<ErrCode>(Val);
}
template <typename T> constexpr auto Unexpect(const Expect<T> &Val) {
  return Support::Unexpected<ErrCode>(Val.error());
}

} // namespace SSVM
