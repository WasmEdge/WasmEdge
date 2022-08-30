// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== Canon Section class definitions
//
// Part of the WasmEdge Project.
//
//===------------------------------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Canon node classes.
///
//===------------------------------------------------------------------------------------------===//
#pragma once

#include "common/span.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace AST {

class StringEncodingUTF8 {};
class StringEncodingUTF16 {};
class StringEncodingLatin1UTF16 {};
class MemoryIndex {
public:
  MemoryIndex() = default;
  MemoryIndex(uint32_t Idx) noexcept : CoreMemIdx{Idx} {}
  uint32_t getCoreMemIdx() const noexcept { return CoreMemIdx; }

private:
  uint32_t CoreMemIdx;
};
class ReallocFunc {
public:
  ReallocFunc() = default;
  ReallocFunc(uint32_t Idx) noexcept : CoreFuncIdx{Idx} {}
  uint32_t getCoreFuncIdx() const noexcept { return CoreFuncIdx; }

private:
  uint32_t CoreFuncIdx;
};
class PostReturnFunc {
public:
  PostReturnFunc() = default;
  PostReturnFunc(uint32_t Idx) noexcept : CoreFuncIdx{Idx} {}
  uint32_t getCoreFuncIdx() const noexcept { return CoreFuncIdx; }

private:
  uint32_t CoreFuncIdx;
};

// opts     ::= opt*:vec(<canonopt>)
// canonopt ::= 0x00                  => string-encoding=utf8
//            | 0x01                  => string-encoding=utf16
//            | 0x02                  => string-encoding=latin1+utf16
//            | 0x03 m:<core:memidx>  => (memory m)
//            | 0x04 f:<core:funcidx> => (realloc f)
//            | 0x05 f:<core:funcidx> => (post-return f)
using CanonOpt = std::variant<StringEncodingUTF8, StringEncodingUTF16,
                              StringEncodingLatin1UTF16, MemoryIndex,
                              ReallocFunc, PostReturnFunc>;

class Lift {
public:
  /// Setter/Getter of core func index
  void setCoreFuncIdx(uint32_t Idx) noexcept { CoreFuncIdx = Idx; }
  uint32_t getCoreFuncIdx() const noexcept { return CoreFuncIdx; }

  /// Setter/Getter of type index
  void setTypeIdx(uint32_t Idx) noexcept { TypeIdx = Idx; }
  uint32_t getTypeIdx() const noexcept { return TypeIdx; }

  /// Getter of options
  Span<const CanonOpt> getOpts() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOpts() noexcept { return Opts; }

private:
  uint32_t CoreFuncIdx;
  uint32_t TypeIdx;
  std::vector<CanonOpt> Opts;
};

class Lower {
public:
  /// Setter/Getter of func index
  void setFuncIdx(uint32_t Idx) noexcept { FuncIdx = Idx; }
  uint32_t getFuncIdx() const noexcept { return FuncIdx; }

  /// Getter of options
  Span<const CanonOpt> getOpts() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOpts() noexcept { return Opts; }

private:
  uint32_t FuncIdx;
  std::vector<CanonOpt> Opts;
};

// canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
//           => (canon lift f opts type-index-space[ft])
//         | 0x01 0x00 f:<funcidx> opts:<opts>
//           => (canon lower f opts (core func))
using Canon = std::variant<Lift, Lower>;

} // namespace AST
} // namespace WasmEdge
