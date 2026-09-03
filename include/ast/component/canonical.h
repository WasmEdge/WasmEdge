// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/ast/component/canonical.h - Canon class definitions ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Canon node related classes.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/valtype.h"
#include "common/enum_ast.hpp"
#include "common/span.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

// canonopt ::= 0x00                  => string-encoding=utf8
//            | 0x01                  => string-encoding=utf16
//            | 0x02                  => string-encoding=latin1+utf16
//            | 0x03 m:<core:memidx>  => (memory m)
//            | 0x04 f:<core:funcidx> => (realloc f)
//            | 0x05 f:<core:funcidx> => (post-return f)
//            | 0x06                  => async 🔀
//            | 0x07 f:<core:funcidx> => (callback f) 🔀

/// AST Component::CanonOpt definition.
class CanonOpt {
public:
  CanonOpt() noexcept : Code(ComponentCanonOptCode::Encode_UTF8), Idx(0) {}

  ComponentCanonOptCode getCode() const noexcept { return Code; }
  void setCode(const ComponentCanonOptCode C) noexcept { Code = C; }

  uint32_t getIndex() const noexcept { return Idx; }
  void setIndex(const uint32_t I) noexcept { Idx = I; }

private:
  ComponentCanonOptCode Code;
  uint32_t Idx;
};

// canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
//           => (canon lift f opts type-index-space[ft])
//         | 0x01 0x00 f:<funcidx> opts:<opts>
//           => (canon lower f opts (core func))
//         | 0x02 rt:<typeidx>    => (canon resource.new rt (core func))
//         | 0x03 rt:<typeidx>    => (canon resource.drop rt (core func))
//         | 0x04 rt:<typeidx>    => (canon resource.rep rt (core func))
//         | 0x09 rs:<resultlist> opts:<opts>
//           => (canon task.return rs opts (core func)) 🔀
//         | 0x05                 => (canon task.cancel (core func)) 🔀
//         | 0x0a t:<core:valtype> i:<u32>
//           => (canon context.get t i (core func)) 🔀
//         | 0x0b t:<core:valtype> i:<u32>
//           => (canon context.set t i (core func)) 🔀
//         | 0x0c cancel?:<cancel?>
//           => (canon thread.yield cancel? (core func)) 🔀
//         | 0x06 async?:<async?>
//           => (canon subtask.cancel async? (core func)) 🔀
//         | 0x0d                 => (canon subtask.drop (core func)) 🔀
//         | 0x0e t:<typeidx>     => (canon stream.new t (core func)) 🔀
//         | 0x0f t:<typeidx> opts:<opts>
//           => (canon stream.read t opts (core func)) 🔀
//         | 0x10 t:<typeidx> opts:<opts>
//           => (canon stream.write t opts (core func)) 🔀
//         | 0x11 t:<typeidx> async?:<async?>
//           => (canon stream.cancel-read t async? (core func)) 🔀
//         | 0x12 t:<typeidx> async?:<async?>
//           => (canon stream.cancel-write t async? (core func)) 🔀
//         | 0x13 t:<typeidx> => (canon stream.drop-readable t (core func)) 🔀
//         | 0x14 t:<typeidx> => (canon stream.drop-writable t (core func)) 🔀
//         | 0x15 t:<typeidx> => (canon future.new t (core func)) 🔀
//         | 0x16 t:<typeidx> opts:<opts>
//           => (canon future.read t opts (core func)) 🔀
//         | 0x17 t:<typeidx> opts:<opts>
//           => (canon future.write t opts (core func)) 🔀
//         | 0x18 t:<typeidx> async?:<async?>
//           => (canon future.cancel-read t async? (core func)) 🔀
//         | 0x19 t:<typeidx> async?:<async?>
//           => (canon future.cancel-write t async? (core func)) 🔀
//         | 0x1a t:<typeidx> => (canon future.drop-readable t (core func)) 🔀
//         | 0x1b t:<typeidx> => (canon future.drop-writable t (core func)) 🔀
//         | 0x1c opts:<opts> => (canon error-context.new opts (core func)) 📝
//         | 0x1d opts:<opts>
//           => (canon error-context.debug-message opts (core func)) 📝
//         | 0x1e                 => (canon error-context.drop (core func)) 📝
//         | 0x1f                 => (canon waitable-set.new (core func)) 🔀
//         | 0x20 cancel?:<cancel?> m:<core:memoryidx>
//           => (canon waitable-set.wait cancel? (memory m) (core func)) 🔀
//         | 0x21 cancel?:<cancel?> m:<core:memoryidx>
//           => (canon waitable-set.poll cancel? (memory m) (core func)) 🔀
//         | 0x22                 => (canon waitable-set.drop (core func)) 🔀
//         | 0x23                 => (canon waitable.join (core func)) 🔀
//         | 0x24                 => (canon backpressure.inc (core func)) 🔀
//         | 0x25                 => (canon backpressure.dec (core func)) 🔀
//         | 0x26                 => (canon thread.index (core func)) 🧵
//         | 0x27 ft:<core:typeidx> tbl:<core:tableidx>
//           => (canon thread.new-indirect ft tbl (core func)) 🧵
//         | 0x28                 => (canon thread.resume-later (core func)) 🧵
//         | 0x29 cancel?:<cancel?>
//           => (canon thread.suspend cancel? (core func)) 🧵
//         | 0x2a cancel?:<cancel?>
//           => (canon thread.suspend-then-resume cancel? (core func)) 🧵
//         | 0x2b cancel?:<cancel?>
//           => (canon thread.yield-then-resume cancel? (core func)) 🧵
//         | 0x2c cancel?:<cancel?>
//           => (canon thread.suspend-then-promote cancel? (core func)) 🧵
//         | 0x2d cancel?:<cancel?>
//           => (canon thread.yield-then-promote cancel? (core func)) 🧵
//         | 0x40 shared?:<sh?> ft:<core:typeidx>
//           => (canon thread.spawn-ref shared? ft (core func)) 🧵②
//         | 0x41 shared?:<sh?> ft:<core:typeidx> tbl:<core:tableidx>
//           => (canon thread.spawn-indirect shared? ft tbl (core func)) 🧵②
//         | 0x42 shared?:<sh?>
//           => (canon thread.available-parallelism shared? (core func)) 🧵②
// opts    ::= opt*:vec(<canonopt>) => opt*
// async?  ::= 0x00 => ϵ
//           | 0x01 => async
// cancel? ::= 0x00 => ϵ
//           | 0x01 => cancellable
// sh?     ::= 0x00 => ϵ
//           | 0x01 => shared 🧵②

// Currently implementing:
//   0x00 0x00 (canon lift f opts type-index-space[ft])
//   0x01 0x00 (canon lower f opts (core func))
//   0x02      (canon resource.new rt (core func))
//   0x03      (canon resource.drop rt (core func))
//   0x04      (canon resource.rep rt (core func))

class Canonical {
public:
  Canonical() noexcept = default;

  ComponentCanonOpCode getOpCode() const noexcept { return Code; }
  void setOpCode(const ComponentCanonOpCode C) noexcept { Code = C; }

  // The one-byte flag immediate. It reads as `async?` on subtask.cancel and
  // the stream and future cancel forms, and as `cancel?` on thread.yield,
  // waitable-set.wait and poll, and the thread.suspend forms.
  bool getFlagImmediate() const noexcept { return FlagImm; }
  void setFlagImmediate(const bool V) noexcept { FlagImm = V; }

  uint32_t getIndex() const noexcept { return Idx; }
  void setIndex(const uint32_t I) noexcept { Idx = I; }

  uint32_t getTargetIndex() const noexcept { return TargetIdx; }
  void setTargetIndex(const uint32_t I) noexcept { TargetIdx = I; }

  uint32_t getConstVal() const noexcept { return I32; }
  void setConstVal(const uint32_t V) noexcept { I32 = V; }

  ValType getContextType() const noexcept { return CtxType; }
  void setContextType(const ValType T) noexcept { CtxType = T; }

  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  void setOptions(std::vector<CanonOpt> &&List) noexcept {
    Opts = std::move(List);
  }

  Span<const LabelValType> getResultList() const noexcept { return ResultList; }
  void setResultList(std::vector<LabelValType> &&R) noexcept {
    ResultList = std::move(R);
  }
  void setResultList(const ComponentValType &VT) noexcept {
    ResultList.clear();
    ResultList.emplace_back(VT);
  }

private:
  ComponentCanonOpCode Code = ComponentCanonOpCode::Lift;
  bool FlagImm = false;
  uint32_t Idx = 0, TargetIdx = 0;
  uint32_t I32 = 0;
  ValType CtxType = TypeCode::I32;
  std::vector<CanonOpt> Opts;
  std::vector<LabelValType> ResultList;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
