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
#include "common/configure.h"
#include "common/errinfo.h"

#include <vector>

namespace WasmEdge {
namespace Loader {

class Serializer {
public:
  Serializer(const Configure &Conf) noexcept : Conf(Conf) {}
  ~Serializer() noexcept = default;

  /// Serialize a WASM module.
  Expect<std::vector<uint8_t>> serializeModule(const AST::Module &Mod);

  /// \name Serialize functions for WASM sections.
  /// @{
  Expect<std::vector<uint8_t>> serializeSection(const AST::CustomSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::TypeSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::ImportSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::FunctionSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::TableSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::MemorySection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::GlobalSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::ExportSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::StartSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::ElementSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::CodeSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::DataSection &Sec);
  Expect<std::vector<uint8_t>> serializeSection(const AST::DataCountSection &Sec);
  /// @}

private:
  /// \name Serialize functions for the other nodes of AST.
  /// @{
  Expect<void> serializeSegment(const AST::GlobalSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  Expect<void> serializeSegment(const AST::ElementSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  Expect<void> serializeSegment(const AST::CodeSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  Expect<void> serializeSegment(const AST::DataSegment &Seg,
                        std::vector<uint8_t> &OutVec);
  Expect<void> serializeDesc(const AST::ImportDesc &Desc, std::vector<uint8_t> &OutVec);
  Expect<void> serializeDesc(const AST::ExportDesc &Desc, std::vector<uint8_t> &OutVec);
  Expect<void> serializeLimit(const AST::Limit &Lim, std::vector<uint8_t> &OutVec);
  Expect<void> serializeType(const AST::FunctionType &Type,
                     std::vector<uint8_t> &OutVec);
  Expect<void> serializeType(const AST::TableType &Type, std::vector<uint8_t> &OutVec);
  Expect<void> serializeType(const AST::MemoryType &Type, std::vector<uint8_t> &OutVec);
  Expect<void> serializeType(const AST::GlobalType &Type, std::vector<uint8_t> &OutVec);
  Expect<void> serializeExpression(const AST::Expression &Expr,
                           std::vector<uint8_t> &OutVec);
  Expect<void> serializeInstruction(const AST::Instruction &Instr,
                            std::vector<uint8_t> &OutVec);
  /// @}

  /// \name Helper functions
  /// @{
  inline auto logSerializeError(ErrCode Code, ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }
  inline auto logNeedProposal(ErrCode Code, Proposal Prop, ASTNodeAttr Node) const noexcept {
    spdlog::error(Code);
    spdlog::error(ErrInfo::InfoProposal(Prop));
    spdlog::error(ErrInfo::InfoAST(Node));
    return Unexpect(Code);
  }

  // Helper function of checking the valid value types.
  Expect<ValType> checkValTypeProposals(ValType VType, ASTNodeAttr Node) const noexcept {
    if (VType == ValType::V128 && !Conf.hasProposal(Proposal::SIMD)) {
      return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::SIMD, Node);
    }
    if ((VType == ValType::FuncRef &&
         !Conf.hasProposal(Proposal::ReferenceTypes) &&
         !Conf.hasProposal(Proposal::BulkMemoryOperations)) ||
        (VType == ValType::ExternRef &&
         !Conf.hasProposal(Proposal::ReferenceTypes))) {
      return logNeedProposal(ErrCode::Value::MalformedElemType,
                             Proposal::ReferenceTypes, Node);
    }
    switch (VType) {
    case ValType::I32:
    case ValType::I64:
    case ValType::F32:
    case ValType::F64:
    case ValType::V128:
    case ValType::ExternRef:
    case ValType::FuncRef:
      return VType;
    default:
      return logSerializeError(ErrCode::Value::MalformedValType, Node);
    }
  }

  // Helper function of checking the valid reference types.
  Expect<RefType> checkRefTypeProposals(RefType RType, ASTNodeAttr Node) const noexcept {
    switch (RType) {
    case RefType::ExternRef:
      if (!Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logNeedProposal(ErrCode::Value::MalformedElemType,
                               Proposal::ReferenceTypes, Node);
      }
      [[fallthrough]];
    case RefType::FuncRef:
      return RType;
    default:
      if (Conf.hasProposal(Proposal::ReferenceTypes)) {
        return logSerializeError(ErrCode::Value::MalformedRefType, Node);
      } else {
        return logSerializeError(ErrCode::Value::MalformedElemType, Node);
      }
    }
  }

  Expect<void> checkInstrProposals(OpCode Code) const noexcept {
    if (Code >= OpCode::I32__trunc_sat_f32_s &&
        Code <= OpCode::I64__trunc_sat_f64_u) {
      // These instructions are for NonTrapFloatToIntConversions proposal.
      if (unlikely(!Conf.hasProposal(Proposal::NonTrapFloatToIntConversions))) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode,
                               Proposal::NonTrapFloatToIntConversions,
                               ASTNodeAttr::Instruction);
      }
    } else if (Code >= OpCode::I32__extend8_s &&
               Code <= OpCode::I64__extend32_s) {
      // These instructions are for SignExtensionOperators proposal.
      if (unlikely(!Conf.hasProposal(Proposal::SignExtensionOperators))) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode,
                               Proposal::SignExtensionOperators,
                               ASTNodeAttr::Instruction);
      }
    } else if ((Code >= OpCode::Ref__null && Code <= OpCode::Ref__func) ||
               (Code >= OpCode::Table__init && Code <= OpCode::Table__copy) ||
               (Code >= OpCode::Memory__init && Code <= OpCode::Memory__fill)) {
      // These instructions are for ReferenceTypes or BulkMemoryOperations
      // proposal.
      if (unlikely(!Conf.hasProposal(Proposal::ReferenceTypes)) &&
          unlikely(!Conf.hasProposal(Proposal::BulkMemoryOperations))) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode,
                               Proposal::ReferenceTypes,
                               ASTNodeAttr::Instruction);
      }
    } else if (Code == OpCode::Select_t ||
               (Code >= OpCode::Table__get && Code <= OpCode::Table__set) ||
               (Code >= OpCode::Table__grow && Code <= OpCode::Table__fill)) {
      // These instructions are for ReferenceTypes proposal.
      if (unlikely(!Conf.hasProposal(Proposal::ReferenceTypes))) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode,
                               Proposal::ReferenceTypes,
                               ASTNodeAttr::Instruction);
      }
    } else if (Code >= OpCode::V128__load &&
               Code <= OpCode::F64x2__convert_low_i32x4_u) {
      // These instructions are for SIMD proposal.
      if (!Conf.hasProposal(Proposal::SIMD)) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode, Proposal::SIMD,
                               ASTNodeAttr::Instruction);
      }
    } else if (Code == OpCode::Return_call ||
               Code == OpCode::Return_call_indirect) {
      // These instructions are for TailCall proposal.
      if (!Conf.hasProposal(Proposal::TailCall)) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode, Proposal::TailCall,
                               ASTNodeAttr::Instruction);
      }
    } else if (Code >= OpCode::I32__atomic__load &&
               Code <= OpCode::I64__atomic__rmw32__cmpxchg_u) {
      // These instructions are for Thread proposal.
      if (!Conf.hasProposal(Proposal::Threads)) {
        return logNeedProposal(ErrCode::Value::IllegalOpCode, Proposal::Threads,
                               ASTNodeAttr::Instruction);
      }
    }
    return {};
  }

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
  Expect<std::vector<uint8_t>> serializeSectionContent(const T &Sec, uint8_t Code,
                                               L &&Func) {
    // Section: section_id + size:u32 + content.
    auto Content = Sec.getContent();
    if (Content.size()) {
      // Section ID.
      std::vector<uint8_t> OutVec = {Code};
      // Content: vec(T).
      serializeU32(Content.size(), OutVec);
      for (const auto &Item : Content) {
        if (auto Res = Func(Item, OutVec); unlikely(!Res)) {
          return Unexpect(Res);
        }
      }
      // Backward insert the section size.
      serializeU32(OutVec.size() - 1, OutVec, std::next(OutVec.begin(), 1));
      return OutVec;
    }
    return {};
  }

  template <typename C, typename T>
  bool instanceof(const std::shared_ptr<T> Ptr) {
    return std::dynamic_pointer_cast<const C>(Ptr) != nullptr;
  }
  /// @}

  /// \name Serializer members
  /// @{
  const Configure Conf;
  /// @}
};

} // namespace Loader
} // namespace WasmEdge
