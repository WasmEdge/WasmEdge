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

#include <optional>
#include <vector>

namespace WasmEdge {
namespace AST {
namespace Component {

class Memory {
  uint32_t MemIdx;

public:
  uint32_t getMemIndex() const noexcept { return MemIdx; }
  uint32_t &getMemIndex() noexcept { return MemIdx; }
};
class Realloc {
  uint32_t FnIdx;

public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }
};
class PostReturn {
  uint32_t FnIdx;

public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }
};
enum class StringEncoding : Byte {
  UTF8 = 0x00,
  UTF16 = 0x01,
  Latin1 = 0x02,
};
using CanonOpt = std::variant<StringEncoding, Memory, Realloc, PostReturn>;

class Lift {
  uint32_t CoreFnIdx;
  std::vector<CanonOpt> Opts;
  uint32_t FnTyIdx;

public:
  uint32_t getCoreFuncIndex() const noexcept { return CoreFnIdx; }
  uint32_t &getCoreFuncIndex() noexcept { return CoreFnIdx; }
  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOptions() noexcept { return Opts; }
  uint32_t getFuncTypeIndex() const noexcept { return FnTyIdx; }
  uint32_t &getFuncTypeIndex() noexcept { return FnTyIdx; }
};
class Lower {
  uint32_t FnIdx;
  std::vector<CanonOpt> Opts;

public:
  uint32_t getFuncIndex() const noexcept { return FnIdx; }
  uint32_t &getFuncIndex() noexcept { return FnIdx; }
  Span<const CanonOpt> getOptions() const noexcept { return Opts; }
  std::vector<CanonOpt> &getOptions() noexcept { return Opts; }
};

class ResourceNew {
  uint32_t TyIdx;

public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }
};
class ResourceDrop {
  uint32_t TyIdx;

public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }
};
class ResourceRep {
  uint32_t TyIdx;

public:
  uint32_t getTypeIndex() const noexcept { return TyIdx; }
  uint32_t &getTypeIndex() noexcept { return TyIdx; }
};

// 0x00 0x00 f:<core:funcidx> opts:<opts> ft:<typeidx>
// => (canon lift f opts type-index-space[ft])
// 0x01 0x00 f:<funcidx> opts:<opts> => (canon lower f opts (core func))
// 0x02 rt:<typeidx>                 => (canon resource.new rt (core func))
// 0x03 rt:<typdidx>                 => (canon resource.drop rt (core func))
// 0x04 rt:<typeidx>                 => (canon resource.rep rt (core func))
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
