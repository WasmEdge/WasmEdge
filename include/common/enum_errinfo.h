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
#include "dense_enum_map.h"
#include <cstdint>
#include <string_view>
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

static inline constexpr auto PtrTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<PtrType, std::string_view> Array[] = {
      {PtrType::Index, "index"sv},
      {PtrType::Address, "address"sv},
  };
  return DenseEnumMap(Array);
}
();

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

static inline constexpr auto MismatchCategoryStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<MismatchCategory, std::string_view> Array[] = {
      {MismatchCategory::Alignment, "memory alignment"sv},
      {MismatchCategory::ValueType, "value type"sv},
      {MismatchCategory::ValueTypes, "value types"sv},
      {MismatchCategory::Mutation, "mutation"sv},
      {MismatchCategory::ExternalType, "external type"sv},
      {MismatchCategory::FunctionType, "function type"sv},
      {MismatchCategory::Table, "table"sv},
      {MismatchCategory::Memory, "memory"sv},
      {MismatchCategory::Global, "global"sv},
      {MismatchCategory::Version, "version"sv},
  };
  return DenseEnumMap(Array);
}
();

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
  Data,
  Lane
};

static inline constexpr auto IndexCategoryStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<IndexCategory, std::string_view> Array[] = {
      {IndexCategory::Label, "label"sv},
      {IndexCategory::Local, "local"sv},
      {IndexCategory::FunctionType, "function type"sv},
      {IndexCategory::Function, "function"sv},
      {IndexCategory::Table, "table"sv},
      {IndexCategory::Memory, "memory"sv},
      {IndexCategory::Global, "global"sv},
      {IndexCategory::Element, "element"sv},
      {IndexCategory::Data, "data"sv},
      {IndexCategory::Lane, "lane"sv},
  };
  return DenseEnumMap(Array);
}
();

} // namespace ErrInfo
} // namespace WasmEdge

#endif // WASMEDGE_C_API_ENUM_ERRINFO_H
