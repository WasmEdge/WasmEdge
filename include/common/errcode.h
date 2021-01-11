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
#include "expected.h"
#include "hexstr.h"

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
  InvalidPath = 0x20,            /// File not found
  ReadError = 0x21,              /// Error when reading
  EndOfFile = 0x22,              /// Reach end of file when reading
  InvalidMagic = 0x23,           /// Not detected magic header
  InvalidVersion = 0x24,         /// Unsupported version
  InvalidSection = 0x25,         /// Malformed section ID
  SectionSizeMismatch = 0x26,    /// Section size mismatched
  SectionSizeOutOfBounds = 0x27, /// Section size out of bounds
  JunkSection = 0x28,            /// Junk sections
  IncompatibleFuncCode = 0x29,   /// Incompatible function and code section
  IncompatibleDataCount = 0x2A,  /// Incompatible data and datacount section
  DataCountRequired = 0x2B,      /// Datacount section required
  UnexpectedEndNode = 0x2C,      /// Unexpected end of sections or functions
  InvalidImportKind = 0x2D,      /// Malformed import kind
  ExpectedZeroFlag = 0x2E,       /// Not loaded an expected zero flag
  InvalidMut = 0x2F,             /// Malformed mutability
  DupGlobal = 0x30,              /// Global duplicated
  TooManyLocals = 0x31,          /// Local size too large
  InvalidElemInstr = 0x32,       /// Invalid instructions in element segments
  InvalidElemType = 0x33,        /// Malformed element type (Bulk-mem proposal)
  InvalidRefType = 0x34,     /// Malformed reference type (Ref-types proposal)
  InvalidElemSegKind = 0x35, /// Invalid element segment kind
  InvalidUTF8 = 0x36,        /// Invalid utf-8 encoding
  IntegerTooLarge = 0x37,    /// Invalid too large integer
  IntegerTooLong = 0x38,     /// Invalid presentation too long integer
  InvalidOpCode = 0x39,      /// Illegal OpCode
  InvalidGrammar = 0x3A,     /// Parsing error
  /// Validation phase
  InvalidAlignment = 0x40,   /// Alignment > natural
  TypeCheckFailed = 0x41,    /// Got unexpected type when checking
  InvalidLabelIdx = 0x42,    /// Branch to unknown label index
  InvalidLocalIdx = 0x43,    /// Access unknown local index
  InvalidFuncTypeIdx = 0x44, /// Type index not defined
  InvalidFuncIdx = 0x45,     /// Function index not defined
  InvalidTableIdx = 0x46,    /// Table index not defined
  InvalidMemoryIdx = 0x47,   /// Memory index not defined
  InvalidGlobalIdx = 0x48,   /// Global index not defined
  InvalidElemIdx = 0x49,     /// Element segment index not defined
  InvalidDataIdx = 0x4A,     /// Data segment index not defined
  InvalidRefIdx = 0x4B,      /// Undeclared reference
  ConstExprRequired = 0x4C,  /// Should be constant expression
  DupExportName = 0x4D,      /// Export name conflicted
  ImmutableGlobal = 0x4E,    /// Tried to store to const global value
  InvalidResultArity = 0x4F, /// Invalid result arity in select t* instruction
  MultiTables = 0x50,        /// #Tables > 1 (without Ref-types proposal)
  MultiMemories = 0x51,      /// #Memories > 1
  InvalidLimit = 0x52,       /// Invalid Limit grammar
  InvalidMemPages = 0x53,    /// Memory pages > 65536
  InvalidStartFunc = 0x54,   /// Invalid start function signature
  InvalidLaneIdx = 0x55,     /// Invalid lane index
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
    {ErrCode::EndOfFile, "unexpected end"},
    {ErrCode::InvalidMagic, "magic header not detected"},
    {ErrCode::InvalidVersion, "unknown binary version"},
    {ErrCode::InvalidSection, "malformed section id"},
    {ErrCode::SectionSizeMismatch, "section size mismatch"},
    {ErrCode::SectionSizeOutOfBounds, "length out of bounds"},
    {ErrCode::JunkSection, "junk after last section"},
    {ErrCode::IncompatibleFuncCode,
     "function and code section have inconsistent lengths"},
    {ErrCode::IncompatibleDataCount,
     "data count and data section have inconsistent lengths"},
    {ErrCode::DataCountRequired, "data count section required"},
    {ErrCode::UnexpectedEndNode, "unexpected end of section or function"},
    {ErrCode::InvalidImportKind, "malformed import kind"},
    {ErrCode::ExpectedZeroFlag, "zero flag expected"},
    {ErrCode::InvalidMut, "malformed mutability"},
    {ErrCode::DupGlobal, "duplicate global"},
    {ErrCode::TooManyLocals, "too many locals"},
    {ErrCode::InvalidElemInstr, "invalid elem"},
    {ErrCode::InvalidElemType, "malformed element type"},
    {ErrCode::InvalidRefType, "malformed reference type"},
    {ErrCode::InvalidElemSegKind, "invalid elements segment kind"},
    {ErrCode::InvalidUTF8, "malformed UTF-8 encoding"},
    {ErrCode::IntegerTooLarge, "integer too large"},
    {ErrCode::IntegerTooLong, "integer representation too long"},
    {ErrCode::InvalidOpCode, "illegal opcode"},
    {ErrCode::InvalidGrammar, "invalid wasm grammar"},
    /// Validation phase
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
    {ErrCode::MultiTables, "multiple tables"},
    {ErrCode::MultiMemories, "multiple memories"},
    {ErrCode::InvalidLimit, "size minimum must not be greater than maximum"},
    {ErrCode::InvalidMemPages,
     "memory size must be at most 65536 pages (4GiB)"},
    {ErrCode::InvalidStartFunc, "start function"},
    {ErrCode::InvalidLaneIdx, "invalid lane index"},
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
     << ", Code: " << convertUIntToHexStr(static_cast<uint32_t>(Code), 2);
  return OS;
}

static inline constexpr bool likely(bool V) {
  return __builtin_expect(V, true);
}
static inline constexpr bool unlikely(bool V) {
  return __builtin_expect(V, false);
}

/// Type aliasing for Expected<T, ErrCode>.
template <typename T> using Expect = Expected<T, ErrCode>;

/// Helper function for Unexpected<ErrCode>.
constexpr auto Unexpect(const ErrCode &Val) { return Unexpected<ErrCode>(Val); }
template <typename T> constexpr auto Unexpect(const Expect<T> &Val) {
  return Unexpected<ErrCode>(Val.error());
}

} // namespace SSVM
