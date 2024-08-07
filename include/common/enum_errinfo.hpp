// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/enum_errinfo.hpp - ErrInfo C++ enumerations -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of ErrInfo used C++ enumerations.
///
//===----------------------------------------------------------------------===//

// This header is not exported to the C API.

#pragma once

#include "dense_enum_map.h"

#include <cstdint>
#include <string_view>

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
#define UsePtrType
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UsePtrType
};

static inline constexpr auto PtrTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<PtrType, std::string_view> Array[] = {
#define UsePtrType
#define Line(NAME, STRING) {PtrType::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UsePtrType
  };
  return DenseEnumMap(Array);
}();

/// Error info mismatch category C++ enumeration class.
enum class MismatchCategory : uint8_t {
#define UseMismatchCategory
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UseMismatchCategory
};

static inline constexpr auto MismatchCategoryStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<MismatchCategory, std::string_view> Array[] = {
#define UseMismatchCategory
#define Line(NAME, STRING) {MismatchCategory::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseMismatchCategory
  };
  return DenseEnumMap(Array);
}();

/// Error info index category C++ enumeration class.
enum class IndexCategory : uint8_t {
#define UseIndexCategory
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UseIndexCategory
};

static inline constexpr auto IndexCategoryStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<IndexCategory, std::string_view> Array[] = {
#define UseIndexCategory
#define Line(NAME, STRING) {IndexCategory::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseIndexCategory
  };
  return DenseEnumMap(Array);
}();

} // namespace ErrInfo
} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ErrInfo::PtrType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ErrInfo::PtrType &Type,
         fmt::format_context &Ctx) const noexcept {
    return formatter<std::string_view>::format(
        WasmEdge::ErrInfo::PtrTypeStr[Type], Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::ErrInfo::MismatchCategory>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ErrInfo::MismatchCategory &Category,
         fmt::format_context &Ctx) const noexcept {
    return formatter<std::string_view>::format(
        WasmEdge::ErrInfo::MismatchCategoryStr[Category], Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::ErrInfo::IndexCategory>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ErrInfo::IndexCategory &Category,
         fmt::format_context &Ctx) const noexcept {
    return formatter<std::string_view>::format(
        WasmEdge::ErrInfo::IndexCategoryStr[Category], Ctx);
  }
};
