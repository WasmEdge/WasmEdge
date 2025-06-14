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

// TODO: COMPONENT - collect the canonopt into a class.

class Memory {
public:
  uint32_t getMemIndex() const noexcept { return MemIdx; }
  uint32_t &getMemIndex() noexcept { return MemIdx; }

private:
  uint32_t MemIdx;
};

class Realloc {
public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }

private:
  uint32_t FnIdx;
};

class PostReturn {
public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }

private:
  uint32_t FnIdx;
};

enum class StringEncoding : Byte {
  UTF8 = 0x00,
  UTF16 = 0x01,
  Latin1 = 0x02,
};

/// AST Component::CanonOpt aliasing.
using CanonOpt = std::variant<StringEncoding, Memory, Realloc, PostReturn>;

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
//         | 0x0c async?:<async>? => (canon yield async? (core func)) 🔀
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

// TODO: COMPONENT - collect the canon into a class.

// Currently implementing:
//   0x00 0x00 (canon lift f opts type-index-space[ft])
//   0x01 0x00 (canon lower f opts (core func))
//   0x02      (canon resource.new rt (core func))
//   0x03      (canon resource.drop rt (core func))
//   0x04      (canon resource.rep rt (core func))

class Lift {
public:
  uint32_t getCoreFuncIndex() const noexcept { return CoreFnIdx; }
  uint32_t &getCoreFuncIndex() noexcept { return CoreFnIdx; }
  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOptions() noexcept { return Opts; }
  uint32_t getFuncTypeIndex() const noexcept { return FnTyIdx; }
  uint32_t &getFuncTypeIndex() noexcept { return FnTyIdx; }

private:
  uint32_t CoreFnIdx;
  std::vector<CanonOpt> Opts;
  uint32_t FnTyIdx;
};

class Lower {
public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }
  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOptions() noexcept { return Opts; }

private:
  uint32_t FnIdx;
  std::vector<CanonOpt> Opts;
};

class ResourceNew {
public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }

private:
  uint32_t TyIdx;
};

class ResourceDrop {
public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }

private:
  uint32_t TyIdx;
};

class ResourceRep {
public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }

private:
  uint32_t TyIdx;
};

using Canon = std::variant<Lift, Lower, ResourceNew, ResourceDrop, ResourceRep>;

} // namespace Component
} // namespace AST
} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::AST::Component::Canon>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::AST::Component::Canon &Opt,
         fmt::format_context &Ctx) const noexcept {
    using namespace std::literals;

    fmt::memory_buffer Buffer;
    if (std::holds_alternative<WasmEdge::AST::Component::Lift>(Opt)) {
      fmt::format_to(std::back_inserter(Buffer), "lift"sv);
    } else if (std::holds_alternative<WasmEdge::AST::Component::Lower>(Opt)) {
      fmt::format_to(std::back_inserter(Buffer), "lower"sv);
    } else if (std::holds_alternative<WasmEdge::AST::Component::ResourceNew>(
                   Opt)) {
      fmt::format_to(std::back_inserter(Buffer), "resource-new"sv);
    } else if (std::holds_alternative<WasmEdge::AST::Component::ResourceDrop>(
                   Opt)) {
      fmt::format_to(std::back_inserter(Buffer), "resource-drop"sv);
    } else if (std::holds_alternative<WasmEdge::AST::Component::ResourceRep>(
                   Opt)) {
      fmt::format_to(std::back_inserter(Buffer), "resource-rep"sv);
    } else {
      fmt::format_to(std::back_inserter(Buffer), "!!!unknown"sv);
    }

    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
