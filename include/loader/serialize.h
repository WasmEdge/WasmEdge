// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include <vector>

namespace WasmEdge {
namespace Loader {

class Serializer {
public:
  /// Serialize a WASM module.
  std::vector<uint8_t> serializeModule(const AST::Module &Mod);

  /// \name Serialize functions for WASM sections.
  /// @{
  std::vector<uint8_t> serializeSection(const AST::CustomSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::TypeSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::ImportSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::FunctionSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::TableSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::MemorySection &Sec);
  std::vector<uint8_t> serializeSection(const AST::GlobalSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::ExportSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::StartSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::ElementSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::CodeSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::DataSection &Sec);
  std::vector<uint8_t> serializeSection(const AST::DataCountSection &Sec);
  /// @}

private:
  /// \name Serialize functions for the other nodes of AST.
  /// @{
  void serializeSegment(const AST::GlobalSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  void serializeSegment(const AST::ElementSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  void serializeSegment(const AST::CodeSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  void serializeSegment(const AST::DataSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  void serializeDesc(const AST::ImportDesc &Desc, std::vector<uint8_t> &OutVec);
  void serializeDesc(const AST::ExportDesc &Desc, std::vector<uint8_t> &OutVec);
  void serializeLimit(const AST::Limit &Lim, std::vector<uint8_t> &OutVec);
  void serializeType(const AST::FunctionType &Type,
                     std::vector<uint8_t> &OutVec);
  void serializeType(const AST::TableType &Type, std::vector<uint8_t> &OutVec);
  void serializeType(const AST::MemoryType &Type, std::vector<uint8_t> &OutVec);
  void serializeType(const AST::GlobalType &Type, std::vector<uint8_t> &OutVec);
  void serializeExpression(const AST::Expression &Expr,
                           std::vector<uint8_t> &OutVec);
  void serializeInstruction(const AST::Instruction &Instr,
                            std::vector<uint8_t> &OutVec);
  /// @}

  /// \name Helper functions
  /// @{
  template <typename NumType, size_t N> void serializeUN(NumType Num,
                   std::vector<uint8_t> &OutVec, std::vector<uint8_t>::iterator It) {
    uint8_t Buf[N / 7 + 1];
    uint32_t Len = 0;
    do {
      uint8_t X = Num & 0x7FU;
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
                    std::vector<uint8_t>::iterator It) {
    serializeUN<uint32_t, 32>(Num, OutVec, It);
  }
  void serializeU32(uint32_t Num, std::vector<uint8_t> &OutVec) {
    serializeUN<uint32_t, 32>(Num, OutVec, OutVec.end());
  }

  void serializeU64(uint64_t Num, std::vector<uint8_t> &OutVec,
                    std::vector<uint8_t>::iterator It) {
    serializeUN<uint64_t, 64>(Num, OutVec, It);
  }
  void serializeU64(uint64_t Num, std::vector<uint8_t> &OutVec) {
    serializeUN<uint64_t, 64>(Num, OutVec, OutVec.end());
  }

  template <typename NumType, size_t N> void serializeSN(NumType Num,
                   std::vector<uint8_t> &OutVec, std::vector<uint8_t>::iterator It) {
    uint8_t Buf[N / 7 + 1];
    uint32_t Len = 0;
    int32_t More = 1;
    while (More) {
      uint8_t X = Num & 0x7FU;
      Num >>= 7;
      if ((Num == 0 && !(X & 0x40)) || (Num == -1 && X & 0x40)) {
        More = 0;
      } else {
        X |= 0x80;
      }
      Buf[Len] = X;
      Len++;
    }
    OutVec.insert(It, Buf, Buf + Len);
  }

  void serializeS32(int32_t Num, std::vector<uint8_t> &OutVec) {
    serializeSN<int32_t, 32>(Num, OutVec, OutVec.end());
  }

  void serializeS33(int64_t Num, std::vector<uint8_t> &OutVec) {
    serializeSN<int64_t, 33>(Num, OutVec, OutVec.end());
  }

  void serializeS64(int64_t Num, std::vector<uint8_t> &OutVec) {
    serializeSN<int64_t, 64>(Num, OutVec, OutVec.end());
  }

  void serializeVec(std::vector<uint8_t> &Vec, std::vector<uint8_t> &OutVec) {
    serializeU32(Vec.size(), OutVec);
    OutVec.insert(OutVec.end(), Vec.begin(), Vec.end());
  }

  template <typename NumType, size_t N> void serializeFN(NumType Num,
                   std::vector<uint8_t> &OutVec, std::vector<uint8_t>::iterator It) {
    uint8_t Buf[N / 8];
    const std::uint8_t* Ptr = reinterpret_cast<const uint8_t*>(&Num);
    for (uint32_t I = 0; I < N / 8; ++I) {
      Buf[I] = Ptr[I];
    }
    OutVec.insert(It, Buf, Buf + N / 8);
  }

  void serializeF32(float Num, std::vector<uint8_t> &OutVec,
                   std::vector<uint8_t>::iterator It) {
    serializeFN<float, 32>(Num, OutVec, It);
  }
  void serializeF32(float Num, std::vector<uint8_t> &OutVec) {
    serializeFN<float, 32>(Num, OutVec, OutVec.end());
  }

  void serializeF64(double Num, std::vector<uint8_t> &OutVec,
                    std::vector<uint8_t>::iterator It) {
    serializeFN<double, 64>(Num, OutVec, It);
  }
  void serializeF64(double Num, std::vector<uint8_t> &OutVec) {
    serializeFN<double, 64>(Num, OutVec, OutVec.end());
  }

  template <typename T, typename L>
  std::vector<uint8_t> serializeSectionContent(const T &Sec, uint8_t Code,
                                               L &&Func) {
    // Section: section_id + size:u32 + content.
    auto Content = Sec.getContent();
    if (Content.size()) {
      // Section ID.
      std::vector<uint8_t> OutVec = {Code};
      // Content: vec(T).
      serializeU32(Content.size(), OutVec);
      for (const auto &Item : Content) {
        Func(Item, OutVec);
      }
      // Backward insert the section size.
      serializeU32(OutVec.size() - 1, OutVec, std::next(OutVec.begin(), 1));
      return OutVec;
    }
    return {};
  }
  /// @}
};

} // namespace Loader
} // namespace WasmEdge
