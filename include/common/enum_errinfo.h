// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_errinfo.h - ErrInfo enumeration definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of ErrInfo used enumerations.
///
//===----------------------------------------------------------------------===//

// This header is not exported to the C API.

#ifndef WASMEDGE_C_API_ENUM_ERRINFO_H
#define WASMEDGE_C_API_ENUM_ERRINFO_H

#ifdef __cplusplus
#include <cstdint>
#include <string>
#include <unordered_map>
#endif

namespace WasmEdge {
namespace ErrInfo {

/// Error info type C++ enumeration class.
enum class InfoType : uint8_t {
  File,          // Information about file name which loading from
  Loading,       // Information about bytecode offset
  AST,           // Information about tracing AST nodes
  InstanceBound, // Information about over boundary of limited #instances
  ForbidIndex,   // Information about forbidden accessing of indices
  Exporting,     // Information about exporting instances
  Limit,         // Information about Limit value
  Registering,   // Information about instantiating modules
  Linking,       // Information about linking instances
  Executing,     // Information about running functions
  Mismatch,      // Information about comparison error
  Instruction,   // Information about aborted instructions and parameters
  Boundary       // Information about forbidden offset accessing
};

/// Error info instance addressing type C++ enumeration class.
enum class PtrType : uint8_t {
  Index,  // Index of instances
  Address // Absolute address
};

static inline std::unordered_map<PtrType, std::string> PtrTypeStr = {
    {PtrType::Index, "index"}, {PtrType::Address, "address"}};

/// Error info mismatch category C++ enumeration class.
enum class MismatchCategory : uint8_t {
  Alignment,    // Alignment in memory instructions
  ValueType,    // Value type
  ValueTypes,   // Value type list
  Mutation,     // Const or Var
  ExternalType, // External typing
  FunctionType, // Function type
  Table,        // Table instance
  Memory,       // Memory instance
  Global,       // Global instance
  Version       // Versions
};

static inline std::unordered_map<MismatchCategory, std::string>
    MismatchCategoryStr = {{MismatchCategory::Alignment, "memory alignment"},
                           {MismatchCategory::ValueType, "value type"},
                           {MismatchCategory::ValueTypes, "value types"},
                           {MismatchCategory::Mutation, "mutation"},
                           {MismatchCategory::ExternalType, "external type"},
                           {MismatchCategory::FunctionType, "function type"},
                           {MismatchCategory::Table, "table"},
                           {MismatchCategory::Memory, "memory"},
                           {MismatchCategory::Global, "global"},
                           {MismatchCategory::Version, "version"}};

/// Error info index category C++ enumeration class.
enum class IndexCategory : uint8_t {
  Label,
  Local,
  FunctionType,
  Function,
  Table,
  Memory,
  Global,
  Element,
  Data
};

static inline std::unordered_map<IndexCategory, std::string> IndexCategoryStr =
    {{IndexCategory::Label, "label"},
     {IndexCategory::Local, "local"},
     {IndexCategory::FunctionType, "function type"},
     {IndexCategory::Function, "function"},
     {IndexCategory::Table, "table"},
     {IndexCategory::Memory, "memory"},
     {IndexCategory::Global, "global"},
     {IndexCategory::Element, "element"},
     {IndexCategory::Data, "data"}};

} // namespace ErrInfo
} // namespace WasmEdge

#endif // WASMEDGE_C_API_ENUM_ERRINFO_H
