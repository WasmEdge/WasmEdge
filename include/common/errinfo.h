// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
#include "common/log.h"
#include "common/types.h"

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

  friend std::ostream &operator<<(std::ostream &OS, const struct InfoFile &Rhs);

  std::filesystem::path FileName;
};

struct InfoLoading {
  InfoLoading() = delete;
  InfoLoading(const uint64_t Off) noexcept : Offset(Off) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoLoading &Rhs);

  uint64_t Offset;
};

struct InfoAST {
  InfoAST() = delete;
  InfoAST(const ASTNodeAttr Attr) noexcept : NodeAttr(Attr) {}

  friend std::ostream &operator<<(std::ostream &OS, const struct InfoAST &Rhs);

  ASTNodeAttr NodeAttr;
};

struct InfoInstanceBound {
  InfoInstanceBound() = delete;
  InfoInstanceBound(const ExternalType Inst, const uint32_t Num,
                    const uint32_t Lim) noexcept
      : Instance(Inst), Number(Num), Limited(Lim) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoInstanceBound &Rhs);

  ExternalType Instance;
  uint32_t Number, Limited;
};

struct InfoForbidIndex {
  InfoForbidIndex() = delete;
  InfoForbidIndex(const IndexCategory Cate, const uint32_t Idx,
                  const uint32_t Bound) noexcept
      : Category(Cate), Index(Idx), Boundary(Bound) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoForbidIndex &Rhs);

  IndexCategory Category;
  uint32_t Index, Boundary;
};

struct InfoExporting {
  InfoExporting() = delete;
  InfoExporting(std::string_view Ext) noexcept : ExtName(Ext) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoExporting &Rhs);

  std::string ExtName;
};

struct InfoLimit {
  InfoLimit() = delete;
  InfoLimit(const bool HasMax, const uint32_t Min,
            const uint32_t Max = 0) noexcept
      : LimHasMax(HasMax), LimMin(Min), LimMax(Max) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoLimit &Rhs);

  bool LimHasMax;
  uint32_t LimMin, LimMax;
};

struct InfoRegistering {
  InfoRegistering() = delete;
  InfoRegistering(std::string_view Mod) noexcept : ModName(Mod) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoRegistering &Rhs);

  std::string ModName;
};

struct InfoLinking {
  InfoLinking() = delete;
  InfoLinking(std::string_view Mod, std::string_view Ext,
              const ExternalType ExtT = ExternalType::Function) noexcept
      : ModName(Mod), ExtName(Ext), ExtType(ExtT) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoLinking &Rhs);

  std::string ModName;
  std::string ExtName;
  ExternalType ExtType;
};

struct InfoExecuting {
  InfoExecuting() = delete;
  InfoExecuting(std::string_view Mod, std::string_view Func) noexcept
      : ModName(Mod), FuncName(Func) {}
  InfoExecuting(std::string_view Func) noexcept : ModName(""), FuncName(Func) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoExecuting &Rhs);

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
  InfoMismatch(const ValType ExpVT, const ValType GotVT) noexcept
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
  InfoMismatch(const RefType ExpRType, /// Reference type
               const bool ExpHasMax, const uint32_t ExpMin,
               const uint32_t ExpMax,  /// Expect Limit
               const RefType GotRType, /// Got reference type
               const bool GotHasMax, const uint32_t GotMin,
               const uint32_t GotMax /// Got limit
               ) noexcept
      : Category(MismatchCategory::Table), ExpRefType(ExpRType),
        GotRefType(GotRType), ExpLimHasMax(ExpHasMax), GotLimHasMax(GotHasMax),
        ExpLimMin(ExpMin), GotLimMin(GotMin), ExpLimMax(ExpMax),
        GotLimMax(GotMax) {}

  /// Case 8: unexpected memory limits
  InfoMismatch(const bool ExpHasMax, const uint32_t ExpMin,
               const uint32_t ExpMax, /// Expect Limit
               const bool GotHasMax, const uint32_t GotMin,
               const uint32_t GotMax /// Got limit
               ) noexcept
      : Category(MismatchCategory::Memory), ExpLimHasMax(ExpHasMax),
        GotLimHasMax(GotHasMax), ExpLimMin(ExpMin), GotLimMin(GotMin),
        ExpLimMax(ExpMax), GotLimMax(GotMax) {}

  /// Case 9: unexpected global types
  InfoMismatch(const ValType ExpVType, const ValMut ExpVMut,
               const ValType GotVType, const ValMut GotVMut) noexcept
      : Category(MismatchCategory::Global), ExpValType(ExpVType),
        GotValType(GotVType), ExpValMut(ExpVMut), GotValMut(GotVMut) {}

  /// Case 10: unexpected version
  InfoMismatch(const uint32_t ExpV, const uint32_t GotV) noexcept
      : Category(MismatchCategory::Version), ExpVersion(ExpV),
        GotVersion(GotV) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoMismatch &Rhs);

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

  /// Case 7 & 8: unexpected table or memory limit
  RefType ExpRefType, GotRefType;
  bool ExpLimHasMax, GotLimHasMax;
  uint32_t ExpLimMin, GotLimMin;
  uint32_t ExpLimMax, GotLimMax;

  /// Case 2: unexpected value type
  /// Case 9: unexpected global type: value type
  ValType ExpValType, GotValType;
  /// Case 4: unexpected mutation settings
  /// Case 9: unexpected global type: value mutation
  ValMut ExpValMut, GotValMut;

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

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoInstruction &Rhs);

  OpCode Code;
  uint64_t Offset;
  std::vector<ValVariant> Args;
  std::vector<ValType> ArgsTypes;
  bool IsSigned;
};

struct InfoBoundary {
  InfoBoundary() = delete;
  InfoBoundary(
      const uint64_t Off, const uint32_t Len = 0,
      const uint32_t Lim = std::numeric_limits<uint32_t>::max()) noexcept
      : Offset(Off), Size(Len), Limit(Lim) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoBoundary &Rhs);

  uint64_t Offset;
  uint32_t Size;
  uint32_t Limit;
};

struct InfoProposal {
  InfoProposal() = delete;
  InfoProposal(Proposal P) noexcept : P(P) {}

  friend std::ostream &operator<<(std::ostream &OS,
                                  const struct InfoProposal &Rhs);
  Proposal P;
};

} // namespace ErrInfo
} // namespace WasmEdge

#if FMT_VERSION >= 90000
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoFile> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLoading> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoAST> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoInstanceBound>
    : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoForbidIndex> : ostream_formatter {
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoExporting> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLimit> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoRegistering> : ostream_formatter {
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoLinking> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoExecuting> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoMismatch> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoInstruction> : ostream_formatter {
};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoBoundary> : ostream_formatter {};
template <>
struct fmt::formatter<WasmEdge::ErrInfo::InfoProposal> : ostream_formatter {};
#endif
