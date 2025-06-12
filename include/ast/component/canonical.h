// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include "ast/expression.h"
#include "ast/type.h"

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
//            | 0x08                  => always-task-return 🔀

/// AST Component::CanonOpt definition.
class CanonOpt {
public:
  enum class OptCode : Byte {
    Encode_UTF8 = 0x00,
    Encode_UTF16 = 0x01,
    Encode_Latin1 = 0x02,
    Memory = 0x03,
    Realloc = 0x04,
    PostReturn = 0x05,
    Async = 0x06,
    Callback = 0x07,
  };

  CanonOpt() noexcept : Code(OptCode::Encode_UTF8), Idx(0) {}

  OptCode getCode() const noexcept { return Code; }
  void setCode(const OptCode C) noexcept { Code = C; }

  uint32_t getIndex() const noexcept { return Idx; }
  void setIndex(const uint32_t I) noexcept { Idx = I; }

private:
  OptCode Code;
  uint32_t Idx;
};

// canon ::= 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
//           => (canon lift f opts type-index-space[ft])
//         | 0x01 0x00 f:<funcidx> opts:<opts>
//           => (canon lower f opts (core func))
//         | 0x02 rt:<typeidx>    => (canon resource.new rt (core func))
//         | 0x03 rt:<typeidx>    => (canon resource.drop rt (core func))
//         | 0x07 rt:<typeidx>
//           => (canon resource.drop rt async (core func)) 🔀
//         | 0x04 rt:<typeidx>    => (canon resource.rep rt (core func))
//         | 0x08                 => (canon backpressure.set (core func)) 🔀
//         | 0x09 rs:<resultlist> opts:<opts>
//           => (canon task.return rs opts (core func)) 🔀
//         | 0x05                 => (canon task.cancel (core func)) 🔀
//         | 0x0a 0x7f i:<u32>    => (canon context.get i32 i (core func)) 🔀
//         | 0x0b 0x7f i:<u32>    => (canon context.set i32 i (core func)) 🔀
//         | 0x0c async?:<async?> => (canon yield async? (core func)) 🔀
//         | 0x06 async?:<async?>
//           => (canon subtask.cancel async? (core func)) 🔀
//         | 0x0d                 => (canon subtask.drop (core func)) 🔀
//         | 0x0e t:<typeidx>     => (canon stream.new t (core func)) 🔀
//         | 0x0f t:<typeidx> opts:<opts>
//           => (canon stream.read t opts (core func)) 🔀
//         | 0x10 t:<typeidx> opts:<opts>
//           => (canon stream.write t opts (core func)) 🔀
//         | 0x11 t:<typeidx> async?:<async?>
//           => (canon stream.cancel-read async? (core func)) 🔀
//         | 0x12 t:<typeidx> async?:<async?>
//           => (canon stream.cancel-write async? (core func)) 🔀
//         | 0x13 t:<typeidx> => (canon stream.close-readable t (core func)) 🔀
//         | 0x14 t:<typeidx> => (canon stream.close-writable t (core func)) 🔀
//         | 0x15 t:<typeidx> => (canon future.new t (core func)) 🔀
//         | 0x16 t:<typeidx> opts:<opts>
//           => (canon future.read t opts (core func)) 🔀
//         | 0x17 t:<typeidx> opts:<opts>
//           => (canon future.write t opts (core func)) 🔀
//         | 0x18 t:<typeidx> async?:<async?>
//           => (canon future.cancel-read async? (core func)) 🔀
//         | 0x19 t:<typeidx> async?:<async?>
//           => (canon future.cancel-write async? (core func)) 🔀
//         | 0x1a t:<typeidx> => (canon future.close-readable t (core func)) 🔀
//         | 0x1b t:<typeidx> => (canon future.close-writable t (core func)) 🔀
//         | 0x1c opts:<opts> => (canon error-context.new opts (core func)) 📝
//         | 0x1d opts:<opts>
//           => (canon error-context.debug-message opts (core func)) 📝
//         | 0x1e                 => (canon error-context.drop (core func)) 📝
//         | 0x1f                 => (canon waitable-set.new (core func)) 🔀
//         | 0x20 async?:<async>? m:<core:memidx>
//           => (canon waitable-set.wait async? (memory m) (core func)) 🔀
//         | 0x21 async?:<async>? m:<core:memidx>
//           => (canon waitable-set.poll async? (memory m) (core func)) 🔀
//         | 0x22                 => (canon waitable-set.drop (core func)) 🔀
//         | 0x23                 => (canon waitable.join (core func)) 🔀
//         | 0x40 ft:<typeidx>    => (canon thread.spawn_ref ft (core func)) 🧵
//         | 0x41 ft:<typeidx> tbl:<core:tableidx>
//           => (canon thread.spawn_indirect ft tbl (core func)) 🧵
//         | 0x42 => (canon thread.available_parallelism (core func)) 🧵
// opts   ::= opt*:vec(<canonopt>) => opt*
// async? ::= 0x00 => ϵ
//          | 0x01 => async

// Currently implementing:
//   0x00 0x00 (canon lift f opts type-index-space[ft])
//   0x01 0x00 (canon lower f opts (core func))
//   0x02      (canon resource.new rt (core func))
//   0x03      (canon resource.drop rt (core func))
//   0x04      (canon resource.rep rt (core func))

class Canonical {
public:
  // TODO: COMPONENT - move to enum.inc.
  enum class OpCode : Byte {
    Lift = 0x00,
    Lower = 0x01,
    Resource__new = 0x02,
    Resource__drop = 0x03,
    Resource__drop_async = 0x07,
    Resource__rep = 0x04,
    Backpressure__set = 0x08,
    Task__return = 0x09,
    Task__cancel = 0x05,
    Context__get = 0x0A,
    Context__set = 0x0B,
    Yield = 0x0C,
    Subtask__cancel = 0x06,
    Subtask__drop = 0x0D,
    Stream__new = 0x0E,
    Stream__read = 0x0F,
    Stream__write = 0x10,
    Stream__cancel_read = 0x11,
    Stream__cancel_write = 0x12,
    Stream__close_readable = 0x13,
    Stream__close_writable = 0x14,
    Future__new = 0x15,
    Future__read = 0x16,
    Future__write = 0x17,
    Future__cancel_read = 0x18,
    Future__cancel_write = 0x19,
    Future__close_readable = 0x1A,
    Future__close_writable = 0x1B,
    Error_context__new = 0x1C,
    Error_context__debug_message = 0x1D,
    Error_context__drop = 0x1E,
    Waitable_set__new = 0x1F,
    Waitable_set__wait = 0x20,
    Waitable_set__poll = 0x21,
    Waitable_set__drop = 0x22,
    Waitable__join = 0x23,
    Thread__spawn_ref = 0x40,
    Thread__spawn_indirect = 0x41,
    Thread__available_parallelism = 0x42,
  };

  Canonical() noexcept = default;

  OpCode getOpCode() const noexcept { return Code; }
  void setOpCode(const OpCode C) noexcept { Code = C; }

  bool isAsync() const noexcept { return IsAsync; }
  void setAsync(const bool A) noexcept { IsAsync = A; }

  uint32_t getIndex() const noexcept { return Idx; }
  void setIndex(const uint32_t I) noexcept { Idx = I; }

  uint32_t getTargetIndex() const noexcept { return TargetIdx; }
  void setTargetIndex(const uint32_t I) noexcept { TargetIdx = I; }

  uint32_t getConstVal() const noexcept { return I32; }
  void setConstVal(const uint32_t V) noexcept { I32 = V; }

  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  void setOptions(std::vector<CanonOpt> &&List) noexcept {
    Opts = std::move(List);
  }

private:
  OpCode Code;
  bool IsAsync;
  uint32_t Idx, TargetIdx;
  uint32_t I32;
  std::vector<CanonOpt> Opts;
};

} // namespace Component
} // namespace AST
} // namespace WasmEdge
