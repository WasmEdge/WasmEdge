// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/loader/serialize.h - Serializer class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Serializer class, which serialize
/// a AST Module into a buffer.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/errinfo.h"

#include <vector>

namespace WasmEdge {
namespace Loader {

class Serializer {
public:
  Serializer(const Configure &Conf) noexcept : Conf(Conf) {}
  ~Serializer() noexcept = default;

  /// Serialize a WASM module.
  Expect<std::vector<uint8_t>>
  serializeModule(const AST::Module &Mod) const noexcept;

  /// \name Serialize functions for WASM sections.
  /// @{
  Expect<void> serializeSection(const AST::CustomSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::TypeSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::ImportSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::FunctionSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::TableSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::MemorySection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::GlobalSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::ExportSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::StartSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::ElementSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::CodeSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::DataSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSection(const AST::DataCountSection &Sec,
                                std::vector<uint8_t> &OutVec) const noexcept;
  /// @}

private:
  /// \name Serialize functions for the other nodes of AST.
  /// @{
  Expect<void> serializeSegment(const AST::TableSegment &Seg,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSegment(const AST::GlobalSegment &Seg,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSegment(const AST::ElementSegment &Seg,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSegment(const AST::CodeSegment &Seg,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeSegment(const AST::DataSegment &Seg,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeDesc(const AST::ImportDesc &Desc,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeDesc(const AST::ExportDesc &Desc,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeHeapType(const ValType &Type, ASTNodeAttr From,
                                 std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeRefType(const ValType &Type, ASTNodeAttr From,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeValType(const ValType &Type, ASTNodeAttr From,
                                std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeLimit(const AST::Limit &Lim,
                              std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeType(const AST::SubType &SType,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeType(const AST::FunctionType &Type,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeType(const AST::TableType &Type,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeType(const AST::MemoryType &Type,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeType(const AST::GlobalType &Type,
                             std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void> serializeExpression(const AST::Expression &Expr,
                                   std::vector<uint8_t> &OutVec) const noexcept;
  Expect<void>
  serializeInstruction(const AST::Instruction &Instr,
                       std::vector<uint8_t> &OutVec) const noexcept;
  /// @}

  /// \name Helper functions
  /// @{
  inline Unexpected<ErrCode>
  logSerializeError(ErrCode Code, ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  inline Unexpected<ErrCode> logNeedProposal(ErrCode Code, Proposal Prop,
                                             ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoProposal(Prop));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }

  template <typename NumType, size_t N>
  void serializeUN(NumType Num, std::vector<uint8_t> &OutVec,
                   std::vector<uint8_t>::iterator It) const noexcept {
    uint8_t Buf[N / 7 + 1];
    uint32_t Len = 0;
    do {
      uint8_t X = std::make_unsigned_t<NumType>(Num) & 0x7FU;
      Num >>= 7;
      if (Num) {
        X |= 0x80U;
      }
      Buf[Len] = X;
      Len++;
    } while (Num);
    OutVec.insert(It, Buf, Buf + Len);
  }

  void serializeU32(uint32_t Num, std::vector<uint8_t> &OutVec,
                    std::vector<uint8_t>::iterator It) const noexcept {
    serializeUN<uint32_t, 32>(Num, OutVec, It);
  }
  void serializeU32(uint32_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeUN<uint32_t, 32>(Num, OutVec, OutVec.end());
  }
  void serializeU64(uint64_t Num, std::vector<uint8_t> &OutVec,
                    std::vector<uint8_t>::iterator It) const noexcept {
    serializeUN<uint64_t, 64>(Num, OutVec, It);
  }
  void serializeU64(uint64_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeUN<uint64_t, 64>(Num, OutVec, OutVec.end());
  }

  template <typename NumType, size_t N>
  void serializeSN(NumType Num, std::vector<uint8_t> &OutVec) const noexcept {
    uint8_t Buf[N / 7 + 1];
    uint32_t Len = 0;
    bool More = true;
    while (More) {
      uint8_t X = static_cast<std::make_unsigned_t<NumType>>(Num) & 0x7FU;
      Num >>= 7;
      if ((Num == 0 && !(X & 0x40)) || (Num == -1 && X & 0x40)) {
        More = false;
      } else {
        X |= 0x80;
      }
      Buf[Len] = X;
      Len++;
    }
    OutVec.insert(OutVec.end(), Buf, Buf + Len);
  }

  void serializeS32(int32_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeSN<int32_t, 32>(Num, OutVec);
  }
  void serializeS33(int64_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeSN<int64_t, 33>(Num, OutVec);
  }
  void serializeS64(int64_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeSN<int64_t, 64>(Num, OutVec);
  }

  template <typename NumType, typename IntType>
  typename std::enable_if_t<
      sizeof(NumType) == sizeof(IntType) && std::is_integral_v<IntType>, void>
  serializeFN(NumType Num, std::vector<uint8_t> &OutVec) const noexcept {
    std::make_unsigned_t<IntType> Buf = 0;
    std::memcpy(&Buf, &Num, sizeof(NumType));
    // Forcing convert into little endian.
    for (uint32_t I = 0; I < sizeof(NumType); I++) {
      OutVec.push_back(static_cast<uint8_t>(Buf & 0xFFU));
      Buf = Buf >> 8;
    }
  }

  void serializeF32(float Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeFN<float, uint32_t>(Num, OutVec);
  }
  void serializeF64(double Num, std::vector<uint8_t> &OutVec) const noexcept {
    serializeFN<double, uint64_t>(Num, OutVec);
  }

  template <typename T, typename L>
  Expect<void> serializeSectionContent(const T &Sec, uint8_t Code,
                                       std::vector<uint8_t> &OutVec,
                                       L &&Func) const noexcept {
    // Section: section_id + size:u32 + content.
    auto Content = Sec.getContent();
    if (Content.size()) {
      // Section ID.
      OutVec.push_back(Code);
      auto OrgSize = OutVec.size();
      // Content: vec(T).
      serializeU32(static_cast<uint32_t>(Content.size()), OutVec);
      for (const auto &Item : Content) {
        if (auto Res = Func(Item, OutVec); unlikely(!Res)) {
          return Unexpect(Res);
        }
      }
      // Backward insert the section size.
      serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
                   std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
    }
    return {};
  }
  /// @}

  /// \name Serializer members
  /// @{
  const Configure &Conf;
  /// @}
};

} // namespace Loader
} // namespace WasmEdge
