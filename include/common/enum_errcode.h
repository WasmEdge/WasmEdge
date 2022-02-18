// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_errcode.h - Error code enumerations ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of WasmEdge error code.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_ERRCODE_H
#define WASMEDGE_C_API_ENUM_ERRCODE_H

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
#include "common/dense_enum_map.h"
#include "common/spare_enum_map.h"
#include <cstdint>
#include <string_view>
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WasmEdge runtime phasing C++ enumeration class.
/// This enumeration is not exported to the C API.
enum class WasmPhase : uint8_t {
  WasmEdge = 0x00,
  Loading = 0x01,
  Validation = 0x02,
  Instantiation = 0x03,
  Execution = 0x04
};

static inline constexpr auto WasmPhaseStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<WasmPhase, std::string_view> Array[] = {
      {WasmPhase::WasmEdge, "wasmedge runtime"sv},
      {WasmPhase::Loading, "loading"sv},
      {WasmPhase::Validation, "validation"sv},
      {WasmPhase::Instantiation, "instantiation"sv},
      {WasmPhase::Execution, "execution"sv},
  };
  return DenseEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// Error code C++ enumeration class.
enum class ErrCode : uint8_t {
  Success = 0x00,
  Terminated = 0x01,        // Exit and return success.
  RuntimeError = 0x02,      // Generic runtime error.
  CostLimitExceeded = 0x03, // Exceeded cost limit (out of gas).
  WrongVMWorkflow = 0x04,   // Wrong VM's workflow
  FuncNotFound = 0x05,      // Wasm function not found
  AOTDisabled = 0x06,       // AOT runtime is disabled
  Interrupted = 0x07,       // Execution interrupted

  // Load phase
  IllegalPath = 0x20,           // File not found
  ReadError = 0x21,             // Error when reading
  UnexpectedEnd = 0x22,         // Reach end of file when reading
  MalformedMagic = 0x23,        // Not detected magic header
  MalformedVersion = 0x24,      // Unsupported version
  MalformedSection = 0x25,      // Malformed section ID
  SectionSizeMismatch = 0x26,   // Section size mismatched
  NameSizeOutOfBounds = 0x27,   // Name size out of bounds
  JunkSection = 0x28,           // Junk sections
  IncompatibleFuncCode = 0x29,  // Incompatible function and code section
  IncompatibleDataCount = 0x2A, // Incompatible data and datacount section
  DataCountRequired = 0x2B,     // Datacount section required
  MalformedImportKind = 0x2C,   // Malformed import kind
  MalformedExportKind = 0x2D,   // Malformed export kind
  ExpectedZeroByte = 0x2E,      // Not loaded an expected zero byte
  InvalidMut = 0x2F,            // Malformed mutability
  TooManyLocals = 0x30,         // Local size too large
  MalformedValType = 0x31,      // Malformed value type
  MalformedElemType = 0x32,     // Malformed element type (Bulk-mem proposal)
  MalformedRefType = 0x33,      // Malformed reference type (Ref-types proposal)
  MalformedUTF8 = 0x34,         // Invalid utf-8 encoding
  IntegerTooLarge = 0x35,       // Invalid too large integer
  IntegerTooLong = 0x36,        // Invalid presentation too long integer
  IllegalOpCode = 0x37,         // Illegal OpCode
  IllegalGrammar = 0x38,        // Parsing error

  // Validation phase
  InvalidAlignment = 0x40,   // Alignment > natural
  TypeCheckFailed = 0x41,    // Got unexpected type when checking
  InvalidLabelIdx = 0x42,    // Branch to unknown label index
  InvalidLocalIdx = 0x43,    // Access unknown local index
  InvalidFuncTypeIdx = 0x44, // Type index not defined
  InvalidFuncIdx = 0x45,     // Function index not defined
  InvalidTableIdx = 0x46,    // Table index not defined
  InvalidMemoryIdx = 0x47,   // Memory index not defined
  InvalidGlobalIdx = 0x48,   // Global index not defined
  InvalidElemIdx = 0x49,     // Element segment index not defined
  InvalidDataIdx = 0x4A,     // Data segment index not defined
  InvalidRefIdx = 0x4B,      // Undeclared reference
  ConstExprRequired = 0x4C,  // Should be constant expression
  DupExportName = 0x4D,      // Export name conflicted
  ImmutableGlobal = 0x4E,    // Tried to store to const global value
  InvalidResultArity = 0x4F, // Invalid result arity in select t* instruction
  MultiTables = 0x50,        // #Tables > 1 (without Ref-types proposal)
  MultiMemories = 0x51,      // #Memories > 1
  InvalidLimit = 0x52,       // Invalid Limit grammar
  InvalidMemPages = 0x53,    // Memory pages > 65536
  InvalidStartFunc = 0x54,   // Invalid start function signature
  InvalidLaneIdx = 0x55,     // Invalid lane index

  // Instantiation phase
  ModuleNameConflict = 0x60,     // Module name conflicted when importing.
  IncompatibleImportType = 0x61, // Import matching failed
  UnknownImport = 0x62,          // Unknown import instances
  DataSegDoesNotFit = 0x63,      // Init failed when instantiating data segment
  ElemSegDoesNotFit = 0x64, // Init failed when instantiating element segment

  // Execution phase
  WrongInstanceAddress = 0x80, // Wrong access of instances addresses
  WrongInstanceIndex = 0x81,   // Wrong access of instances indices
  InstrTypeMismatch = 0x82,    // Instruction type not match
  FuncSigMismatch = 0x83,      // Function signature not match when invoking
  DivideByZero = 0x84,         // Divide by zero
  IntegerOverflow = 0x85,      // Integer overflow
  InvalidConvToInt = 0x86,     // Cannot do convert to integer
  TableOutOfBounds = 0x87,     // Out of bounds table access
  MemoryOutOfBounds = 0x88,    // Out of bounds memory access
  Unreachable = 0x89,          // Meet an unreachable instruction
  UninitializedElement = 0x8A, // Uninitialized element in table instance
  UndefinedElement = 0x8B,     // Access undefined element in table instances
  IndirectCallTypeMismatch = 0x8C, // Func type mismatch in call_indirect
  ExecutionFailed = 0x8D,          // Host function execution failed
  RefTypeMismatch = 0x8E           // Reference type not match
};

static inline constexpr const auto ErrCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ErrCode, std::string_view> Array[] = {
      // WasmEdge runtime
      {ErrCode::Success, "success"sv},
      {ErrCode::Terminated, "terminated"sv},
      {ErrCode::RuntimeError, "generic runtime error"sv},
      {ErrCode::CostLimitExceeded, "cost limit exceeded"sv},
      {ErrCode::WrongVMWorkflow, "wrong VM workflow"sv},
      {ErrCode::FuncNotFound, "wasm function not found"sv},
      {ErrCode::AOTDisabled, "AOT runtime is disabled in this build"sv},
      {ErrCode::Interrupted, "execution interrupted"sv},
      // Load phase
      {ErrCode::IllegalPath, "invalid path"sv},
      {ErrCode::ReadError, "read error"sv},
      {ErrCode::UnexpectedEnd, "unexpected end"sv},
      {ErrCode::MalformedMagic, "magic header not detected"sv},
      {ErrCode::MalformedVersion, "unknown binary version"sv},
      {ErrCode::MalformedSection, "malformed section id"sv},
      {ErrCode::SectionSizeMismatch, "section size mismatch"sv},
      {ErrCode::NameSizeOutOfBounds, "length out of bounds"sv},
      {ErrCode::JunkSection, "unexpected content after last section"sv},
      {ErrCode::IncompatibleFuncCode,
       "function and code section have inconsistent lengths"sv},
      {ErrCode::IncompatibleDataCount,
       "data count and data section have inconsistent lengths"sv},
      {ErrCode::DataCountRequired, "data count section required"sv},
      {ErrCode::MalformedImportKind, "malformed import kind"sv},
      {ErrCode::MalformedExportKind, "malformed export kind"sv},
      {ErrCode::ExpectedZeroByte, "zero byte expected"sv},
      {ErrCode::InvalidMut, "malformed mutability"sv},
      {ErrCode::TooManyLocals, "too many locals"sv},
      {ErrCode::MalformedValType, "malformed value type"sv},
      {ErrCode::MalformedElemType, "malformed element type"sv},
      {ErrCode::MalformedRefType, "malformed reference type"sv},
      {ErrCode::MalformedUTF8, "malformed UTF-8 encoding"sv},
      {ErrCode::IntegerTooLarge, "integer too large"sv},
      {ErrCode::IntegerTooLong, "integer representation too long"sv},
      {ErrCode::IllegalOpCode, "illegal opcode"sv},
      {ErrCode::IllegalGrammar, "invalid wasm grammar"sv},
      // Validation phase
      {ErrCode::InvalidAlignment,
       "alignment must not be larger than natural"sv},
      {ErrCode::TypeCheckFailed, "type mismatch"sv},
      {ErrCode::InvalidLabelIdx, "unknown label"sv},
      {ErrCode::InvalidLocalIdx, "unknown local"sv},
      {ErrCode::InvalidFuncTypeIdx, "unknown type"sv},
      {ErrCode::InvalidFuncIdx, "unknown function"sv},
      {ErrCode::InvalidTableIdx, "unknown table"sv},
      {ErrCode::InvalidMemoryIdx, "unknown memory"sv},
      {ErrCode::InvalidGlobalIdx, "unknown global"sv},
      {ErrCode::InvalidElemIdx, "unknown elem segment"sv},
      {ErrCode::InvalidDataIdx, "unknown data segment"sv},
      {ErrCode::InvalidRefIdx, "undeclared function reference"sv},
      {ErrCode::ConstExprRequired, "constant expression required"sv},
      {ErrCode::DupExportName, "duplicate export name"sv},
      {ErrCode::ImmutableGlobal, "global is immutable"sv},
      {ErrCode::InvalidResultArity, "invalid result arity"sv},
      {ErrCode::MultiTables, "multiple tables"sv},
      {ErrCode::MultiMemories, "multiple memories"sv},
      {ErrCode::InvalidLimit,
       "size minimum must not be greater than maximum"sv},
      {ErrCode::InvalidMemPages,
       "memory size must be at most 65536 pages (4GiB)"sv},
      {ErrCode::InvalidStartFunc, "start function"sv},
      {ErrCode::InvalidLaneIdx, "invalid lane index"sv},
      // Instantiation phase
      {ErrCode::ModuleNameConflict, "module name conflict"sv},
      {ErrCode::IncompatibleImportType, "incompatible import type"sv},
      {ErrCode::UnknownImport, "unknown import"sv},
      {ErrCode::DataSegDoesNotFit, "data segment does not fit"sv},
      {ErrCode::ElemSegDoesNotFit, "elements segment does not fit"sv},
      // Execution phase
      {ErrCode::WrongInstanceAddress, "wrong instance address"sv},
      {ErrCode::WrongInstanceIndex, "wrong instance index"sv},
      {ErrCode::InstrTypeMismatch, "instruction type mismatch"sv},
      {ErrCode::FuncSigMismatch, "function signature mismatch"sv},
      {ErrCode::DivideByZero, "integer divide by zero"sv},
      {ErrCode::IntegerOverflow, "integer overflow"sv},
      {ErrCode::InvalidConvToInt, "invalid conversion to integer"sv},
      {ErrCode::TableOutOfBounds, "out of bounds table access"sv},
      {ErrCode::MemoryOutOfBounds, "out of bounds memory access"sv},
      {ErrCode::Unreachable, "unreachable"sv},
      {ErrCode::UninitializedElement, "uninitialized element"sv},
      {ErrCode::UndefinedElement, "undefined element"sv},
      {ErrCode::IndirectCallTypeMismatch, "indirect call type mismatch"sv},
      {ErrCode::ExecutionFailed, "host function failed"sv},
      {ErrCode::RefTypeMismatch, "reference type mismatch"sv},
  };
  return SpareEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

/// Error code C enumeration.
enum WasmEdge_ErrCode {
  WasmEdge_ErrCode_Success = 0x00,
  WasmEdge_ErrCode_Terminated = 0x01,
  WasmEdge_ErrCode_RuntimeError = 0x02,
  WasmEdge_ErrCode_CostLimitExceeded = 0x03,
  WasmEdge_ErrCode_WrongVMWorkflow = 0x04,
  WasmEdge_ErrCode_FuncNotFound = 0x05,
  WasmEdge_ErrCode_AOTDisabled = 0x06,
  WasmEdge_ErrCode_Interrupted = 0x07,

  // Load phase
  WasmEdge_ErrCode_InvalidPath = 0x20,
  WasmEdge_ErrCode_ReadError = 0x21,
  WasmEdge_ErrCode_UnexpectedEnd = 0x22,
  WasmEdge_ErrCode_InvalidMagic = 0x23,
  WasmEdge_ErrCode_InvalidVersion = 0x24,
  WasmEdge_ErrCode_InvalidSection = 0x25,
  WasmEdge_ErrCode_SectionSizeMismatch = 0x26,
  WasmEdge_ErrCode_NameSizeOutOfBounds = 0x27,
  WasmEdge_ErrCode_JunkSection = 0x28,
  WasmEdge_ErrCode_IncompatibleFuncCode = 0x29,
  WasmEdge_ErrCode_IncompatibleDataCount = 0x2A,
  WasmEdge_ErrCode_DataCountRequired = 0x2B,
  WasmEdge_ErrCode_InvalidImportKind = 0x2C,
  WasmEdge_ErrCode_InvalidExportKind = 0x2D,
  WasmEdge_ErrCode_ExpectedZeroByte = 0x2E,
  WasmEdge_ErrCode_InvalidMut = 0x2F,
  WasmEdge_ErrCode_TooManyLocals = 0x30,
  WasmEdge_ErrCode_InvalidValType = 0x31,
  WasmEdge_ErrCode_InvalidElemType = 0x32,
  WasmEdge_ErrCode_InvalidRefType = 0x33,
  WasmEdge_ErrCode_InvalidUTF8 = 0x34,
  WasmEdge_ErrCode_IntegerTooLarge = 0x35,
  WasmEdge_ErrCode_IntegerTooLong = 0x36,
  WasmEdge_ErrCode_InvalidOpCode = 0x37,
  WasmEdge_ErrCode_InvalidGrammar = 0x38,

  // Validation phase
  WasmEdge_ErrCode_InvalidAlignment = 0x40,
  WasmEdge_ErrCode_TypeCheckFailed = 0x41,
  WasmEdge_ErrCode_InvalidLabelIdx = 0x42,
  WasmEdge_ErrCode_InvalidLocalIdx = 0x43,
  WasmEdge_ErrCode_InvalidFuncTypeIdx = 0x44,
  WasmEdge_ErrCode_InvalidFuncIdx = 0x45,
  WasmEdge_ErrCode_InvalidTableIdx = 0x46,
  WasmEdge_ErrCode_InvalidMemoryIdx = 0x47,
  WasmEdge_ErrCode_InvalidGlobalIdx = 0x48,
  WasmEdge_ErrCode_InvalidElemIdx = 0x49,
  WasmEdge_ErrCode_InvalidDataIdx = 0x4A,
  WasmEdge_ErrCode_InvalidRefIdx = 0x4B,
  WasmEdge_ErrCode_ConstExprRequired = 0x4C,
  WasmEdge_ErrCode_DupExportName = 0x4D,
  WasmEdge_ErrCode_ImmutableGlobal = 0x4E,
  WasmEdge_ErrCode_InvalidResultArity = 0x4F,
  WasmEdge_ErrCode_MultiTables = 0x50,
  WasmEdge_ErrCode_MultiMemories = 0x51,
  WasmEdge_ErrCode_InvalidLimit = 0x52,
  WasmEdge_ErrCode_InvalidMemPages = 0x53,
  WasmEdge_ErrCode_InvalidStartFunc = 0x54,
  WasmEdge_ErrCode_InvalidLaneIdx = 0x55,

  // Instantiation phase
  WasmEdge_ErrCode_ModuleNameConflict = 0x60,
  WasmEdge_ErrCode_IncompatibleImportType = 0x61,
  WasmEdge_ErrCode_UnknownImport = 0x62,
  WasmEdge_ErrCode_DataSegDoesNotFit = 0x63,
  WasmEdge_ErrCode_ElemSegDoesNotFit = 0x64,

  // Execution phase
  WasmEdge_ErrCode_WrongInstanceAddress = 0x80,
  WasmEdge_ErrCode_WrongInstanceIndex = 0x81,
  WasmEdge_ErrCode_InstrTypeMismatch = 0x82,
  WasmEdge_ErrCode_FuncSigMismatch = 0x83,
  WasmEdge_ErrCode_DivideByZero = 0x84,
  WasmEdge_ErrCode_IntegerOverflow = 0x85,
  WasmEdge_ErrCode_InvalidConvToInt = 0x86,
  WasmEdge_ErrCode_TableOutOfBounds = 0x87,
  WasmEdge_ErrCode_MemoryOutOfBounds = 0x88,
  WasmEdge_ErrCode_Unreachable = 0x89,
  WasmEdge_ErrCode_UninitializedElement = 0x8A,
  WasmEdge_ErrCode_UndefinedElement = 0x8B,
  WasmEdge_ErrCode_IndirectCallTypeMismatch = 0x8C,
  WasmEdge_ErrCode_ExecutionFailed = 0x8D,
  WasmEdge_ErrCode_RefTypeMismatch = 0x8E
};

#endif // WASMEDGE_C_API_ENUM_ERRCODE_H
