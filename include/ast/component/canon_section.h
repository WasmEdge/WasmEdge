// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//=== wasmedge/ast/component/canon_section.h - Canon Section class definitions
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
#include <vector>

namespace WasmEdge {
namespace AST {

// canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
//           => (canon lift f opts type-index-space[ft])
//         | 0x01 0x00 f:<funcidx> opts:<opts>
//           => (canon lower f opts (core func))
class Canon {
public:
  class Lift;
  class Lower;
};

// opts     ::= opt*:vec(<canonopt>)
// canonopt ::= 0x00                  => string-encoding=utf8
//            | 0x01                  => string-encoding=utf16
//            | 0x02                  => string-encoding=latin1+utf16
//            | 0x03 m:<core:memidx>  => (memory m)
//            | 0x04 f:<core:funcidx> => (realloc f)
//            | 0x05 f:<core:funcidx> => (post-return f)
class CanonOpt {
public:
  class StringEncodingUTF8;
  class StringEncodingUTF16;
  class StringEncodingLatin1UTF16;
  class MemoryIndex;
  class ReallocFunc;
  class PostReturnFunc;
};
class CanonOpt::StringEncodingUTF8 : public CanonOpt {};
class CanonOpt::StringEncodingUTF16 : public CanonOpt {};
class CanonOpt::StringEncodingLatin1UTF16 : public CanonOpt {};
class CanonOpt::MemoryIndex : public CanonOpt {
public:
  MemoryIndex(uint32_t Idx) noexcept : CoreMemIdx{Idx} {}

  uint32_t getCoreMemIdx() const noexcept { return CoreMemIdx; }

private:
  uint32_t CoreMemIdx;
};
class CanonOpt::ReallocFunc : public CanonOpt {
public:
  ReallocFunc(uint32_t Idx) noexcept : CoreFuncIdx{Idx} {}

  uint32_t getCoreFuncIdx() const noexcept { return CoreFuncIdx; }

private:
  uint32_t CoreFuncIdx;
};
class CanonOpt::PostReturnFunc : public CanonOpt {
public:
  PostReturnFunc(uint32_t Idx) noexcept : CoreFuncIdx{Idx} {}

  uint32_t getCoreFuncIdx() const noexcept { return CoreFuncIdx; }

private:
  uint32_t CoreFuncIdx;
};

class Canon::Lift : public Canon {
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

class Canon::Lower : public Canon {
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

} // namespace AST
} // namespace WasmEdge
