// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/errinfo.h - Error information definition ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of WasmEdge error information.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_ast.hpp"
#include "common/enum_configure.hpp"
#include "common/enum_errcode.hpp"
#include "common/enum_errinfo.hpp"
#include "common/enum_types.hpp"
#include "common/filesystem.h"
#include "common/fmt.h"
#include "common/spdlog.h"
#include "common/types.h"

#include <iterator>
#if !defined(__has_include) || __has_include(<spdlog/fmt/ranges.h>)
#include <spdlog/fmt/ranges.h>
#else
#include <fmt/ranges.h>
#endif

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace ErrInfo {

/// Information structures.
struct InfoFile {
  InfoFile() = delete;
  InfoFile(const std::filesystem::path &FName) noexcept : FileName(FName) {}

  std::filesystem::path FileName;
};

struct InfoLoading {
  InfoLoading() = delete;
  InfoLoading(const uint64_t Off) noexcept : Offset(Off) {}

  uint64_t Offset;
};

struct InfoAST {
  InfoAST() = delete;
  InfoAST(const ASTNodeAttr Attr) noexcept : NodeAttr(Attr) {}

  ASTNodeAttr NodeAttr;
};

struct InfoInstanceBound {
  InfoInstanceBound() = delete;
  InfoInstanceBound(const ExternalType Inst, const uint32_t Num,
                    const uint32_t Lim) noexcept
      : Instance(Inst), Number(Num), Limited(Lim) {}

  ExternalType Instance;
  uint32_t Number, Limited;
};

struct InfoForbidIndex {
  InfoForbidIndex() = delete;
  InfoForbidIndex(const IndexCategory Cate, const uint32_t Idx,
                  const uint32_t Bound) noexcept
      : Category(Cate), Index(Idx), Boundary(Bound) {}

  IndexCategory Category;
  uint32_t Index, Boundary;
};

struct InfoExporting {
  InfoExporting() = delete;
  InfoExporting(std::string_view Ext) noexcept : ExtName(Ext) {}

  std::string ExtName;
};

struct InfoLimit {
  InfoLimit() = delete;
  InfoLimit(const bool HasMax, const uint64_t Min,
            const uint64_t Max = 0) noexcept
      : LimHasMax(HasMax), LimMin(Min), LimMax(Max) {}

  bool LimHasMax;
  uint64_t LimMin, LimMax;
};

struct InfoRegistering {
  InfoRegistering() = delete;
  InfoRegistering(std::string_view Mod) noexcept : ModName(Mod) {}

  std::string ModName;
};

struct InfoLinking {
  InfoLinking() = delete;
  InfoLinking(std::string_view Mod, std::string_view Ext,
              const ExternalType ExtT = ExternalType::Function) noexcept
      : ModName(Mod), ExtName(Ext), ExtType(ExtT) {}

  std::string ModName;
  std::string ExtName;
  ExternalType ExtType;
};

struct InfoExecuting {
  InfoExecuting() = delete;
  InfoExecuting(std::string_view Mod, std::string_view Func) noexcept
      : ModName(Mod), FuncName(Func) {}
  InfoExecuting(std::string_view Func) noexcept : ModName(""), FuncName(Func) {}

  std::string ModName;
  std::string FuncName;
};

struct InfoMismatch {
  InfoMismatch() = delete;

  /// Case 1: unexpected alignment
  InfoMismatch(const uint8_t ExpAlign, const uint32_t GotAlign) noexcept
      : Category(MismatchCategory::Alignment), ExpAlignment(ExpAlign),
        GotAlignment(GotAlign) {}

  /// Case 2: unexpected value type
  InfoMismatch(const ValType &ExpVT, const ValType &GotVT) noexcept
      : Category(MismatchCategory::ValueType), ExpValType(ExpVT),
        GotValType(GotVT) {}

  /// Case 3: unexpected value type list
  InfoMismatch(const std::vector<ValType> &ExpV,
               const std::vector<ValType> &GotV) noexcept
      : Category(MismatchCategory::ValueTypes), ExpParams(ExpV),
        GotParams(GotV) {}

  /// Case 4: unexpected mutation settings
  InfoMismatch(const ValMut ExpM, const ValMut GotM) noexcept
      : Category(MismatchCategory::Mutation), ExpValMut(ExpM), GotValMut(GotM) {
  }

  /// Case 5: unexpected external types
  InfoMismatch(const ExternalType ExpExt, const ExternalType GotExt) noexcept
      : Category(MismatchCategory::ExternalType), ExpExtType(ExpExt),
        GotExtType(GotExt) {}

  /// Case 6: unexpected function types
  InfoMismatch(const std::vector<ValType> &ExpP,
               const std::vector<ValType> &ExpR,
               const std::vector<ValType> &GotP,
               const std::vector<ValType> &GotR) noexcept
      : Category(MismatchCategory::FunctionType), ExpParams(ExpP),
        GotParams(GotP), ExpReturns(ExpR), GotReturns(GotR) {}

  /// Case 7: unexpected table types
  InfoMismatch(const ValType &ExpRType,
               // Expected reference type
               const bool ExpHasMax, const uint64_t ExpMin,
               const uint64_t ExpMax,
               // Expected Limit
               const ValType &GotRType,
               // Got reference type
               const bool GotHasMax, const uint64_t GotMin,
               const uint64_t GotMax
               // Got limit
               ) noexcept
      : Category(MismatchCategory::Table), ExpValType(ExpRType),
        GotValType(GotRType), ExpLimHasMax(ExpHasMax), GotLimHasMax(GotHasMax),
        ExpLimMin(ExpMin), GotLimMin(GotMin), ExpLimMax(ExpMax),
        GotLimMax(GotMax) {}

  /// Case 8: unexpected memory limits
  InfoMismatch(const bool ExpHasMax, const uint64_t ExpMin,
               const uint64_t ExpMax,
               // Expect Limit
               const bool GotHasMax, const uint64_t GotMin,
               const uint64_t GotMax
               // Got limit
               ) noexcept
      : Category(MismatchCategory::Memory), ExpLimHasMax(ExpHasMax),
        GotLimHasMax(GotHasMax), ExpLimMin(ExpMin), GotLimMin(GotMin),
        ExpLimMax(ExpMax), GotLimMax(GotMax) {}

  /// Case 9: unexpected global types
  InfoMismatch(const ValType &ExpVType, const ValMut ExpVMut,
               const ValType &GotVType, const ValMut GotVMut) noexcept
      : Category(MismatchCategory::Global), ExpValType(ExpVType),
        GotValType(GotVType), ExpValMut(ExpVMut), GotValMut(GotVMut) {}

  /// Case 10: unexpected version
  InfoMismatch(const uint32_t ExpV, const uint32_t GotV) noexcept
      : Category(MismatchCategory::Version), ExpVersion(ExpV),
        GotVersion(GotV) {}

  /// Mismatched category
  MismatchCategory Category;

  /// Case 1: unexpected alignment
  uint8_t ExpAlignment;
  uint32_t GotAlignment;

  /// Case 5: unexpected external type
  ExternalType ExpExtType, GotExtType;

  /// Case 6: unexpected function type
  /// Case 3: unexpected value type list
  std::vector<ValType> ExpParams, GotParams;
  std::vector<ValType> ExpReturns, GotReturns;

  /// Case 2: unexpected value type
  /// Case 7: unexpected table type: reference type
  /// Case 9: unexpected global type: value type
  ValType ExpValType, GotValType;
  /// Case 4: unexpected mutation settings
  /// Case 9: unexpected global type: value mutation
  ValMut ExpValMut, GotValMut;
  /// Case 7 & 8: unexpected table or memory type: limit
  bool ExpLimHasMax, GotLimHasMax;
  uint64_t ExpLimMin, GotLimMin;
  uint64_t ExpLimMax, GotLimMax;

  /// Case 10: unexpected version
  uint32_t ExpVersion, GotVersion;
};

struct InfoInstruction {
  InfoInstruction() = delete;
  InfoInstruction(const OpCode Op, const uint64_t Off,
                  const std::vector<ValVariant> &ArgsVec = {},
                  const std::vector<ValType> &ArgsTypesVec = {},
                  const bool Signed = false) noexcept
      : Code(Op), Offset(Off), Args(ArgsVec), ArgsTypes(ArgsTypesVec),
        IsSigned(Signed) {}

  OpCode Code;
  uint64_t Offset;
  std::vector<ValVariant> Args;
  std::vector<ValType> ArgsTypes;
  bool IsSigned;
};

struct InfoBoundary {
  InfoBoundary() = delete;
  InfoBoundary(const uint64_t Off, const uint64_t Len = 0,
               const uint64_t Lim = std::numeric_limits<uint64_t>::max(),
               const bool IsOverflow = false) noexcept
      : IsOffsetOverflow(IsOverflow), Offset(Off), Size(Len), Limit(Lim) {}

  bool IsOffsetOverflow;
  uint64_t Offset;
  uint64_t Size;
  uint64_t Limit;
};

struct InfoProposal {
  InfoProposal() = delete;
  InfoProposal(Proposal P) noexcept : P(P) {}

  Proposal P;
};

} // namespace ErrInfo
} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoFile>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoFile &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "    File name: {}"sv,
                   Info.FileName);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLoading>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoLoading &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer),
                   "    Bytecode offset: 0x{:08x}"sv, Info.Offset);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoAST>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoAST &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "    At AST node: {}"sv,
                   Info.NodeAttr);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoInstanceBound>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoInstanceBound &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer),
                   "    Instance {} has limited number {} , Got: {}"sv,
                   Info.Instance, Info.Limited, Info.Number);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoForbidIndex>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoForbidIndex &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    auto Iter =
        fmt::format_to(std::back_inserter(Buffer),
                       "    When checking {} index: {} , Out of boundary: "sv,
                       Info.Category, Info.Index);
    if (Info.Boundary > 0) {
      fmt::format_to(Iter, "{}"sv, Info.Boundary - 1);
    } else {
      fmt::format_to(Iter, "empty"sv);
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoExporting>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoExporting &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer),
                   "    Duplicated exporting name: \"{}\""sv, Info.ExtName);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLimit>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoLimit &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    auto Iter = fmt::format_to(std::back_inserter(Buffer),
                               "    In Limit type: {{ min: {}"sv, Info.LimMin);
    if (Info.LimHasMax) {
      Iter = fmt::format_to(Iter, " , max: {}"sv, Info.LimMax);
    }
    fmt::format_to(Iter, " }}"sv);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoRegistering>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoRegistering &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "    Module name: \"{}\""sv,
                   Info.ModName);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLinking>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoLinking &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer),
                   "    When linking module: \"{}\" , {} name: \"{}\""sv,
                   Info.ModName, Info.ExtType, Info.ExtName);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoExecuting>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoExecuting &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    auto Iter =
        fmt::format_to(std::back_inserter(Buffer), "    When executing "sv);
    if (!Info.ModName.empty()) {
      Iter = fmt::format_to(Iter, "module name: \"{}\" , "sv, Info.ModName);
    }
    fmt::format_to(Iter, "function name: \"{}\""sv, Info.FuncName);
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoMismatch>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoMismatch &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    auto Iter = fmt::format_to(std::back_inserter(Buffer),
                               "    Mismatched {}. "sv, Info.Category);
    auto FormatLimit = [](auto Out, bool LimHasMax, uint64_t LimMin,
                          uint64_t LimMax) {
      Out = fmt::format_to(Out, "Limit{{{}"sv, LimMin);
      if (LimHasMax) {
        Out = fmt::format_to(Out, " , {}"sv, LimMax);
      }
      Out = fmt::format_to(Out, "}}"sv);
      return Out;
    };
    switch (Info.Category) {
    case WasmEdge::ErrInfo::MismatchCategory::Alignment:
      if (Info.GotAlignment < Info.ExpAlignment) {
        fmt::format_to(Iter, "Expected: need to == {} , Got: {}"sv,
                       static_cast<uint32_t>(Info.ExpAlignment),
                       static_cast<uint32_t>(Info.GotAlignment));
      } else {
        fmt::format_to(Iter, "Expected: need to <= {} , Got: {}"sv,
                       static_cast<uint32_t>(Info.ExpAlignment),
                       static_cast<uint32_t>(Info.GotAlignment));
      }
      break;
    case WasmEdge::ErrInfo::MismatchCategory::ValueType:
      fmt::format_to(Iter, "Expected: {} , Got: {}"sv, Info.ExpValType,
                     Info.GotValType);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::ValueTypes:
      fmt::format_to(Iter, "Expected: types{{{}}} , Got: types{{{}}}"sv,
                     fmt::join(Info.ExpParams, " , "sv),
                     fmt::join(Info.GotParams, " , "sv));
      break;
    case WasmEdge::ErrInfo::MismatchCategory::Mutation:
      fmt::format_to(Iter, "Expected: {} , Got: {}"sv, Info.ExpValMut,
                     Info.GotValMut);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::ExternalType:
      fmt::format_to(Iter, "Expected: {} , Got: {}", Info.ExpExtType,
                     Info.GotExtType);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::FunctionType:
      fmt::format_to(Iter,
                     "Expected: FuncType {{params{{{}}} returns{{{}}}}} , "
                     "Got: FuncType {{params{{{}}} returns{{{}}}}}"sv,
                     fmt::join(Info.ExpParams, " , "sv),
                     fmt::join(Info.ExpReturns, " , "sv),
                     fmt::join(Info.GotParams, " , "sv),
                     fmt::join(Info.GotReturns, " , "sv));
      break;
    case WasmEdge::ErrInfo::MismatchCategory::Table:
      Iter = fmt::format_to(Iter, "Expected: TableType {{RefType{{{}}} "sv,
                            static_cast<WasmEdge::ValType>(Info.ExpValType));
      Iter =
          FormatLimit(Iter, Info.ExpLimHasMax, Info.ExpLimMin, Info.ExpLimMax);
      Iter = fmt::format_to(Iter, "}} , Got: TableType {{RefType{{{}}} "sv,
                            static_cast<WasmEdge::ValType>(Info.GotValType));
      Iter =
          FormatLimit(Iter, Info.GotLimHasMax, Info.GotLimMin, Info.GotLimMax);
      fmt::format_to(Iter, "}}"sv);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::Memory:
      Iter = fmt::format_to(Iter, "Expected: MemoryType {{"sv);
      Iter =
          FormatLimit(Iter, Info.ExpLimHasMax, Info.ExpLimMin, Info.ExpLimMax);
      Iter = fmt::format_to(Iter, "}} , Got: MemoryType {{"sv);
      Iter =
          FormatLimit(Iter, Info.GotLimHasMax, Info.GotLimMin, Info.GotLimMax);
      fmt::format_to(Iter, "}}"sv);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::Global:
      fmt::format_to(Iter,
                     "Expected: GlobalType {{Mutation{{{}}} ValType{{{}}}}} , "
                     "Got: GlobalType {{Mutation{{{}}} ValType{{{}}}}}"sv,
                     Info.ExpValMut, Info.ExpValType, Info.GotValMut,
                     Info.GotValType);
      break;
    case WasmEdge::ErrInfo::MismatchCategory::Version:
      fmt::format_to(Iter, "Expected: {} , Got: {}"sv, Info.ExpVersion,
                     Info.GotVersion);
      break;
    default:
      break;
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoInstruction>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoInstruction &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    uint16_t Payload = static_cast<uint16_t>(Info.Code);
    fmt::memory_buffer Buffer;
    auto Iter = fmt::format_to(std::back_inserter(Buffer),
                               "    In instruction: {} ("sv, Info.Code);
    if ((Payload >> 8) >= static_cast<uint16_t>(0xFCU)) {
      Iter = fmt::format_to(Iter, "0x{:02x} "sv, Payload >> 8);
    }
    Iter = fmt::format_to(Iter, "0x{:02x}) , Bytecode offset: 0x{:08x}"sv,
                          Payload & 0xFFU, Info.Offset);
    if (!Info.Args.empty()) {
      Iter = fmt::format_to(Iter, " , Args: ["sv);
      for (uint32_t I = 0; I < Info.Args.size(); ++I) {
        switch (Info.ArgsTypes[I].getCode()) {
        case WasmEdge::TypeCode::I32:
          if (Info.IsSigned) {
            Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<int32_t>());
          } else {
            Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<uint32_t>());
          }
          break;
        case WasmEdge::TypeCode::I64:
          if (Info.IsSigned) {
            Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<int64_t>());
          } else {
            Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<uint64_t>());
          }
          break;
        case WasmEdge::TypeCode::F32:
          Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<float>());
          break;
        case WasmEdge::TypeCode::F64:
          Iter = fmt::format_to(Iter, "{}"sv, Info.Args[I].get<double>());
          break;
        case WasmEdge::TypeCode::V128: {
          const auto Value = Info.Args[I].get<WasmEdge::uint64x2_t>();
          Iter = fmt::format_to(Iter, "0x{:08x}{:08x}"sv, Value[1], Value[0]);
          break;
        }
        case WasmEdge::TypeCode::Ref:
        case WasmEdge::TypeCode::RefNull:
          Iter = fmt::format_to(Iter, "{}"sv, Info.ArgsTypes[I]);
          if (Info.Args[I].get<WasmEdge::RefVariant>().isNull()) {
            Iter = fmt::format_to(Iter, ":null"sv);
          } else {
            Iter = fmt::format_to(Iter, ":0x{:08x}"sv,
                                  Info.Args[I].get<uint64_t>());
          }
          break;
        default:
          break;
        }
        if (I < Info.Args.size() - 1) {
          Iter = fmt::format_to(Iter, " , "sv);
        }
      }
      Iter = fmt::format_to(Iter, "]"sv);
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoBoundary>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoBoundary &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    WasmEdge::uint128_t OffFrom = Info.Offset;
    if (Info.IsOffsetOverflow) {
      OffFrom += (WasmEdge::uint128_t(1ULL) << 64);
    }
    WasmEdge::uint128_t OffTo =
        OffFrom + WasmEdge::uint128_t(Info.Size > 0U ? Info.Size - 1U : 0U);
    uint64_t Bound = (Info.Limit > 0U ? Info.Limit - 1U : 0U);
#if WASMEDGE_OS_WINDOWS
    uint64_t OffFromHigh = static_cast<uint64_t>(OffFrom >> 64);
    uint64_t OffFromLow = static_cast<uint64_t>(OffFrom);
    uint64_t OffToHigh = static_cast<uint64_t>(OffTo >> 64);
    uint64_t OffToLow = static_cast<uint64_t>(OffTo);
    if (OffFromHigh > 0) {
      fmt::format_to(std::back_inserter(Buffer),
                     "    Accessing offset from: 0x{:8x}{:016x} to: "
                     "0x{:8x}{:016x} , Out of "
                     "boundary: 0x{:016x}"sv,
                     OffFromHigh, OffFromLow, OffToHigh, OffToLow, Bound);
    } else {
      fmt::format_to(
          std::back_inserter(Buffer),
          "    Accessing offset from: 0x{:08x} to: 0x{:08x} , Out of "
          "boundary: 0x{:08x}"sv,
          OffFromLow, OffToLow, Bound);
    }
#elif defined(__x86_64__) || defined(__aarch64__) ||                           \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
    fmt::format_to(std::back_inserter(Buffer),
                   "    Accessing offset from: 0x{:08x} to: 0x{:08x} , Out of "
                   "boundary: 0x{:08x}"sv,
                   OffFrom, OffTo, Bound);
#else
    if (OffFrom.high() > 0) {
      fmt::format_to(std::back_inserter(Buffer),
                     "    Accessing offset from: 0x{:8x}{:016x} to: "
                     "0x{:8x}{:016x} , Out of "
                     "boundary: 0x{:016x}"sv,
                     OffFrom.high(), OffFrom.low(), OffTo.high(), OffTo.low(),
                     Bound);
    } else {
      fmt::format_to(
          std::back_inserter(Buffer),
          "    Accessing offset from: 0x{:08x} to: 0x{:08x} , Out of "
          "boundary: 0x{:08x}"sv,
          OffFrom.low(), OffTo.low(), Bound);
    }
#endif
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoProposal>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ErrInfo::InfoProposal &Info,
              FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    using namespace std::literals;
    fmt::memory_buffer Buffer;
    if (auto Iter = WasmEdge::ProposalStr.find(Info.P);
        Iter != WasmEdge::ProposalStr.end()) {
      fmt::format_to(
          std::back_inserter(Buffer),
          "    This instruction or syntax requires enabling {} proposal"sv,
          Iter->second);
    } else {
      fmt::format_to(std::back_inserter(Buffer),
                     "    Unknown proposal, Code 0x{:08x}"sv,
                     static_cast<uint32_t>(Info.P));
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
