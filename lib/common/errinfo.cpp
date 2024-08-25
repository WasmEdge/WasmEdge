// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/errinfo.h"
#include "common/errcode.h"
#include "common/hexstr.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ranges.h>

using namespace std::literals;

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoFile>::format(
    const WasmEdge::ErrInfo::InfoFile &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer), "    File name: {}"sv,
                 Info.FileName);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoLoading>::format(
    const WasmEdge::ErrInfo::InfoLoading &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer), "    Bytecode offset: 0x{:08x}"sv,
                 Info.Offset);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoAST>::format(
    const WasmEdge::ErrInfo::InfoAST &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer), "    At AST node: {}"sv,
                 Info.NodeAttr);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoInstanceBound>::format(
    const WasmEdge::ErrInfo::InfoInstanceBound &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer),
                 "    Instance {} has limited number {} , Got: {}"sv,
                 Info.Instance, Info.Limited, Info.Number);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoForbidIndex>::format(
    const WasmEdge::ErrInfo::InfoForbidIndex &Info,
    fmt::format_context &Ctx) const noexcept {
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

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoExporting>::format(
    const WasmEdge::ErrInfo::InfoExporting &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer),
                 "    Duplicated exporting name: \"{}\""sv, Info.ExtName);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoLimit>::format(
    const WasmEdge::ErrInfo::InfoLimit &Info,
    fmt::format_context &Ctx) const noexcept {
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

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoRegistering>::format(
    const WasmEdge::ErrInfo::InfoRegistering &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer), "    Module name: \"{}\""sv,
                 Info.ModName);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoLinking>::format(
    const WasmEdge::ErrInfo::InfoLinking &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer),
                 "    When linking module: \"{}\" , {} name: \"{}\""sv,
                 Info.ModName, Info.ExtType, Info.ExtName);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoExecuting>::format(
    const WasmEdge::ErrInfo::InfoExecuting &Info,
    fmt::format_context &Ctx) const noexcept {
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

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoMismatch>::format(
    const WasmEdge::ErrInfo::InfoMismatch &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  auto Iter = fmt::format_to(std::back_inserter(Buffer),
                             "    Mismatched {}. "sv, Info.Category);
  auto FormatLimit = [](auto Out, bool LimHasMax, uint32_t LimMin,
                        uint32_t LimMax) {
    Out = fmt::format_to(Out, "Limit{{{}"sv, LimMin);
    if (LimHasMax) {
      Out = fmt::format_to(Out, " , {}"sv, LimMax);
    }
    Out = fmt::format_to(Out, "}}"sv);
    return Out;
  };
  switch (Info.Category) {
  case WasmEdge::ErrInfo::MismatchCategory::Alignment:
    fmt::format_to(Iter, "Expected: need to <= {} , Got: {}"sv,
                   static_cast<uint32_t>(Info.ExpAlignment),
                   1UL << Info.GotAlignment);
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
    Iter = FormatLimit(Iter, Info.ExpLimHasMax, Info.ExpLimMin, Info.ExpLimMax);
    Iter = fmt::format_to(Iter, "}} , Got: TableType {{RefType{{{}}} "sv,
                          static_cast<WasmEdge::ValType>(Info.GotValType));
    Iter = FormatLimit(Iter, Info.GotLimHasMax, Info.GotLimMin, Info.GotLimMax);
    fmt::format_to(Iter, "}}"sv);
    break;
  case WasmEdge::ErrInfo::MismatchCategory::Memory:
    Iter = fmt::format_to(Iter, "Expected: MemoryType {{"sv);
    Iter = FormatLimit(Iter, Info.ExpLimHasMax, Info.ExpLimMin, Info.ExpLimMax);
    Iter = fmt::format_to(Iter, "}} , Got: MemoryType {{"sv);
    Iter = FormatLimit(Iter, Info.GotLimHasMax, Info.GotLimMin, Info.GotLimMax);
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

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoInstruction>::format(
    const WasmEdge::ErrInfo::InfoInstruction &Info,
    fmt::format_context &Ctx) const noexcept {
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
          Iter =
              fmt::format_to(Iter, ":0x{:08x}"sv, Info.Args[I].get<uint64_t>());
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

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoBoundary>::format(
    const WasmEdge::ErrInfo::InfoBoundary &Info,
    fmt::format_context &Ctx) const noexcept {
  fmt::memory_buffer Buffer;
  fmt::format_to(std::back_inserter(Buffer),
                 "    Accessing offset from: 0x{:08x} to: 0x{:08x} , Out of "
                 "boundary: 0x{:08x}"sv,
                 Info.Offset,
                 Info.Offset + (Info.Size > 0U ? Info.Size - 1U : 0U),
                 Info.Limit);
  return formatter<std::string_view>::format(
      std::string_view(Buffer.data(), Buffer.size()), Ctx);
}

fmt::format_context::iterator
fmt::formatter<WasmEdge::ErrInfo::InfoProposal>::format(
    const WasmEdge::ErrInfo::InfoProposal &Info,
    fmt::format_context &Ctx) const noexcept {
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
