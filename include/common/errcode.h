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
  InvalidPath = 0x20,    /// File not found
  ReadError = 0x21,      /// Error when reading
  EndOfFile = 0x22,      /// Reach end of file when reading
  InvalidGrammar = 0x23, /// Parsing error
  InvalidVersion = 0x24, /// Unsupported version
  /// Validation phase
  InvalidOpCode = 0x40,      /// Invalid instruction type
  InvalidAlignment = 0x41,   /// Alignment > natural
  TypeCheckFailed = 0x42,    /// Got unexpected type when checking
  InvalidLabelIdx = 0x43,    /// Branch to unknown label index
  InvalidLocalIdx = 0x44,    /// Access unknown local index
  InvalidFuncTypeIdx = 0x45, /// Type index not defined
  InvalidFuncIdx = 0x46,     /// Function index not defined
  InvalidTableIdx = 0x47,    /// Table index not defined
  InvalidMemoryIdx = 0x48,   /// Memory index not defined
  InvalidGlobalIdx = 0x49,   /// Global index not defined
  InvalidElemIdx = 0x4A,     /// Element segment index not defined
  InvalidDataIdx = 0x4B,     /// Data segment index not defined
  InvalidRefIdx = 0x4C,      /// Undeclared reference
  ConstExprRequired = 0x4D,  /// Should be constant expression
  DupExportName = 0x4E,      /// Export name conflicted
  ImmutableGlobal = 0x4F,    /// Tried to store to const global value
  InvalidResultArity = 0x50, /// Invalid result arity in select t* instruction
  MultiMemories = 0x51,      /// #Memories > 1
  InvalidLimit = 0x52,       /// Invalid Limit grammar
  InvalidMemPages = 0x53,    /// Memory pages > 65536
  InvalidStartFunc = 0x54,   /// Invalid start function signature
  /// Instantiation phase
  ModuleNameConflict = 0x60,     /// Module name conflicted when importing.
  IncompatibleImportType = 0x61, /// Import matching failed
  UnknownImport = 0x62,          /// Unknown import instances
  DataSegDoesNotFit = 0x63,      /// Init failed when instantiating data segment
  ElemSegDoesNotFit = 0x64, /// Init failed when instantiating element segment
  /// Execution phase
  WrongInstanceAddress = 0x80, /// Wrong access of instances addresses
  WrongInstanceIndex = 0x81,   /// Wrong access of instances indices
  InstrTypeMismatch = 0x82,    /// Instruction type not match
  FuncSigMismatch = 0x83,      /// Function signature not match when invoking
  DivideByZero = 0x84,         /// Divide by zero
  IntegerOverflow = 0x85,      /// Integer overflow
  InvalidConvToInt = 0x86,     /// Cannot do convert to integer
  TableOutOfBounds = 0x87,     /// Out of bounds table access
  MemoryOutOfBounds = 0x88,    /// Out of bounds memory access
  Unreachable = 0x89,          /// Meet an unreachable instruction
  UninitializedElement = 0x8A, /// Uninitialized element in table instance
  UndefinedElement = 0x8B,     /// Access undefined element in table instances
  IndirectCallTypeMismatch = 0x8C, /// Func type mismatch in call_indirect
  ExecutionFailed = 0x8D           /// Host function execution failed
};

/// Error code enumeration string mapping.
static inline std::unordered_map<ErrCode, std::string> ErrCodeStr = {
    /// SSVM runtime
    {ErrCode::Success, "success"},
    {ErrCode::Terminated, "terminated"},
    {ErrCode::CostLimitExceeded, "cost limit exceeded"},
    {ErrCode::WrongVMWorkflow, "wrong VM workflow"},
    {ErrCode::FuncNotFound, "wasm function not found"},
    /// Load phase
    {ErrCode::InvalidPath, "invalid path"},
    {ErrCode::ReadError, "read error"},
    {ErrCode::EndOfFile, "read end of file"},
    {ErrCode::InvalidGrammar, "invalid wasm grammar"},
    {ErrCode::InvalidVersion, "invalid version"},
    /// Validation phase
    {ErrCode::InvalidOpCode, "invalid instruction opcode"},
    {ErrCode::InvalidAlignment, "alignment must not be larger than natural"},
    {ErrCode::TypeCheckFailed, "type mismatch"},
    {ErrCode::InvalidLabelIdx, "unknown label"},
    {ErrCode::InvalidLocalIdx, "unknown local"},
    {ErrCode::InvalidFuncTypeIdx, "unknown type"},
    {ErrCode::InvalidFuncIdx, "unknown function"},
    {ErrCode::InvalidTableIdx, "unknown table"},
    {ErrCode::InvalidMemoryIdx, "unknown memory"},
    {ErrCode::InvalidGlobalIdx, "unknown global"},
    {ErrCode::InvalidElemIdx, "unknown elem segment"},
    {ErrCode::InvalidDataIdx, "unknown data segment"},
    {ErrCode::InvalidRefIdx, "undeclared function reference"},
    {ErrCode::ConstExprRequired, "constant expression required"},
    {ErrCode::DupExportName, "duplicate export name"},
    {ErrCode::ImmutableGlobal, "global is immutable"},
    {ErrCode::InvalidResultArity, "invalid result arity"},
    {ErrCode::MultiMemories, "multiple memories"},
    {ErrCode::InvalidLimit, "size minimum must not be greater than maximum"},
    {ErrCode::InvalidMemPages,
     "memory size must be at most 65536 pages (4GiB)"},
    {ErrCode::InvalidStartFunc, "start function"},
    /// Instantiation phase
    {ErrCode::ModuleNameConflict, "module name conflict"},
    {ErrCode::IncompatibleImportType, "incompatible import type"},
    {ErrCode::UnknownImport, "unknown import"},
    {ErrCode::DataSegDoesNotFit, "data segment does not fit"},
    {ErrCode::ElemSegDoesNotFit, "elements segment does not fit"},
    /// Execution phase
    {ErrCode::WrongInstanceAddress, "wrong instance address"},
    {ErrCode::WrongInstanceIndex, "wrong instance index"},
    {ErrCode::InstrTypeMismatch, "instruction type mismatch"},
    {ErrCode::FuncSigMismatch, "function signature mismatch"},
    {ErrCode::DivideByZero, "integer divide by zero"},
    {ErrCode::IntegerOverflow, "integer overflow"},
    {ErrCode::InvalidConvToInt, "invalid conversion to integer"},
    {ErrCode::TableOutOfBounds, "out of bounds table access"},
    {ErrCode::MemoryOutOfBounds, "out of bounds memory access"},
    {ErrCode::Unreachable, "unreachable"},
    {ErrCode::UninitializedElement, "uninitialized element"},
    {ErrCode::UndefinedElement, "undefined element"},
    {ErrCode::IndirectCallTypeMismatch, "indirect call type mismatch"},
    {ErrCode::ExecutionFailed, "host function failed"}};

static inline WasmPhase getErrCodePhase(ErrCode Code) {
  return static_cast<WasmPhase>((static_cast<uint8_t>(Code) & 0xF0) >> 5);
}

static inline std::ostream &operator<<(std::ostream &OS, ErrCode Code) {
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
