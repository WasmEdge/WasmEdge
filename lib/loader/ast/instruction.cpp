// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

// OpCode loader. See "include/loader/loader.h".
Expect<OpCode> Loader::loadOpCode() {
  uint8_t Prefix;
  if (auto B1 = FMgr.readByte()) {
    Prefix = (*B1);
  } else {
    return Unexpect(B1);
  }

  if (Prefix >= 0xFBU && Prefix <= 0xFEU) {
    // Multi-byte OpCode case.
    uint32_t Extend;
    if (auto B2 = FMgr.readU32()) {
      Extend = (*B2);
    } else {
      return Unexpect(B2);
    }
    if (Prefix == 0xFBU) {
      switch (Extend) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)
#define Line_FB(NAME, STRING, PREFIX, EXTEND)                                  \
  case EXTEND:                                                                 \
    return OpCode::NAME;
#define Line_FC(NAME, STRING, PREFIX, EXTEND)
#define Line_FD(NAME, STRING, PREFIX, EXTEND)
#define Line_FE(NAME, STRING, PREFIX, EXTEND)
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
      default:
        return Unexpect(ErrCode::Value::IllegalOpCode);
      }
    } else if (Prefix == 0xFCU) {
      switch (Extend) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)
#define Line_FB(NAME, STRING, PREFIX, EXTEND)
#define Line_FC(NAME, STRING, PREFIX, EXTEND)                                  \
  case EXTEND:                                                                 \
    return OpCode::NAME;
#define Line_FD(NAME, STRING, PREFIX, EXTEND)
#define Line_FE(NAME, STRING, PREFIX, EXTEND)
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
      default:
        return Unexpect(ErrCode::Value::IllegalOpCode);
      }
    } else if (Prefix == 0xFDU) {
      switch (Extend) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)
#define Line_FB(NAME, STRING, PREFIX, EXTEND)
#define Line_FC(NAME, STRING, PREFIX, EXTEND)
#define Line_FD(NAME, STRING, PREFIX, EXTEND)                                  \
  case EXTEND:                                                                 \
    return OpCode::NAME;
#define Line_FE(NAME, STRING, PREFIX, EXTEND)
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
      default:
        return Unexpect(ErrCode::Value::IllegalOpCode);
      }
    } else {
      switch (Extend) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)
#define Line_FB(NAME, STRING, PREFIX, EXTEND)
#define Line_FC(NAME, STRING, PREFIX, EXTEND)
#define Line_FD(NAME, STRING, PREFIX, EXTEND)
#define Line_FE(NAME, STRING, PREFIX, EXTEND)                                  \
  case EXTEND:                                                                 \
    return OpCode::NAME;
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
      default:
        return Unexpect(ErrCode::Value::IllegalOpCode);
      }
    }
  } else {
    // Single-byte OpCode case.
    switch (Prefix) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)                                             \
  case PREFIX:                                                                 \
    return OpCode::NAME;
#define Line_FB(NAME, STRING, PREFIX, EXTEND)
#define Line_FC(NAME, STRING, PREFIX, EXTEND)
#define Line_FD(NAME, STRING, PREFIX, EXTEND)
#define Line_FE(NAME, STRING, PREFIX, EXTEND)
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
    default:
      return Unexpect(ErrCode::Value::IllegalOpCode);
    }
  }
}

// Load instruction sequence. See "include/loader/loader.h".
Expect<AST::InstrVec> Loader::loadInstrSeq(std::optional<uint64_t> SizeBound) {
  OpCode Code;
  AST::InstrVec Instrs;
  std::vector<std::pair<OpCode, uint32_t>> BlockStack;
  uint32_t Cnt = 0;
  bool IsReachEnd = false;
  // Read opcode until the End code of the top block.
  do {
    // Read the opcode and check if error.
    uint64_t Offset = FMgr.getOffset();
    if (auto Res = loadOpCode()) {
      Code = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }

    // Check with proposals.
    if (auto Res = Conf.isInstrNeedProposal(Code); unlikely(Res.has_value())) {
      return logNeedProposal(ErrCode::Value::IllegalOpCode, Res.value(), Offset,
                             ASTNodeAttr::Instruction);
    }

    auto logIllegalOpCode = [this, &Offset,
                             &SizeBound]() -> Unexpected<ErrCode> {
      if (SizeBound.has_value() && FMgr.getOffset() > SizeBound.value()) {
        return logLoadError(ErrCode::Value::ENDCodeExpected, Offset,
                            ASTNodeAttr::Instruction);
      } else {
        return logLoadError(ErrCode::Value::IllegalOpCode, Offset,
                            ASTNodeAttr::Instruction);
      }
    };

    // Process the instruction which contains a block.
    switch (Code) {
    case OpCode::Block:
    case OpCode::Loop:
    case OpCode::If:
    // LEGACY-EH: remove the `Try` after deprecating legacy EH.
    case OpCode::Try:
    case OpCode::Try_table:
      BlockStack.emplace_back(Code, Cnt);
      break;
    case OpCode::Else: {
      if (BlockStack.size() == 0 || BlockStack.back().first != OpCode::If) {
        // An Else instruction appeared outside the If-block.
        return logIllegalOpCode();
      }
      uint32_t Pos = BlockStack.back().second;
      if (Instrs[Pos].getJumpElse() > 0) {
        // An Else instruction appeared before in this If-block.
        return logIllegalOpCode();
      }
      Instrs[Pos].setJumpElse(Cnt - Pos);
      break;
    }
    // LEGACY-EH: remove the `Catch` cases after deprecating legacy EH.
    case OpCode::Catch:
    case OpCode::Catch_all: {
      if (BlockStack.size() == 0 || BlockStack.back().first != OpCode::Try) {
        // A Catch/Catch_all instruction appeared outside a try-block.
        return logIllegalOpCode();
      }
      auto Pos = BlockStack.back().second;
      auto &CatchClause = Instrs[Pos].getTryCatch().Catch;
      if (CatchClause.size() > 0 && CatchClause.back().IsAll) {
        // A Catch shouldn't behind a Catch_all in the same block.
        // And also a try block may contain only one Catch_all instruction.
        return logIllegalOpCode();
      }
      break;
    }
    default:
      break;
    }

    // Create the instruction node and load contents.
    Instrs.emplace_back(Code, static_cast<uint32_t>(Offset));
    if (auto Res = loadInstruction(Instrs.back()); !Res) {
      return Unexpect(Res);
    }

    if (Code == OpCode::End) {
      // Post process the End instruction.
      if (BlockStack.size() > 0) {
        Instrs.back().setExprLast(false);
        const auto &[BackOp, Pos] = BlockStack.back();
        if (BackOp == OpCode::Block || BackOp == OpCode::Loop ||
            BackOp == OpCode::If) {
          Instrs.back().setTryBlockLast(false);
          // LEGACY-EH: remove this after deprecating legacy EH.
          Instrs.back().setLegacyTryBlockLast(false);
          Instrs[Pos].setJumpEnd(Cnt - Pos);
          if (BackOp == OpCode::If) {
            if (Instrs[Pos].getJumpElse() == 0) {
              // If block without else. Set the else jump the same as end jump.
              Instrs[Pos].setJumpElse(Cnt - Pos);
            } else {
              const uint32_t ElsePos = Pos + Instrs[Pos].getJumpElse();
              Instrs[ElsePos].setJumpEnd(Cnt - ElsePos);
            }
          }
        } else if (BackOp == OpCode::Try_table) {
          Instrs.back().setTryBlockLast(true);
          // LEGACY-EH: remove this after deprecating legacy EH.
          Instrs.back().setLegacyTryBlockLast(false);
          Instrs[Pos].getTryCatch().JumpEnd = Cnt - Pos;
        } else if (BackOp == OpCode::Try) {
          // LEGACY-EH: remove the `Try` case after deprecating legacy EH.
          Instrs.back().setTryBlockLast(false);
          Instrs.back().setLegacyTryBlockLast(true);
          Instrs[Pos].getTryCatch().JumpEnd = Cnt - Pos;
        }
        BlockStack.pop_back();
      } else {
        Instrs.back().setExprLast(true);
        IsReachEnd = true;
      }
    } else if (Code == OpCode::Catch || Code == OpCode::Catch_all) {
      // LEGACY-EH: remove these cases after deprecating legacy EH.
      uint32_t Pos = BlockStack.back().second;
      auto &CatchClause = Instrs[Pos].getTryCatch().Catch;
      auto &CatchDesc = Instrs.back().getCatchLegacy();
      CatchDesc.CatchPCOffset = Cnt - Pos;
      CatchDesc.CatchIndex = static_cast<uint32_t>(CatchClause.size());
      CatchClause.push_back({true,
                             Code == OpCode::Catch_all,
                             false,
                             Code == OpCode::Catch ? CatchDesc.TagIndex : 0,
                             0,
                             {0, 0, 0, 0}});
    }
    Cnt++;
  } while (!IsReachEnd);

  // Check the loaded offset should match the segment boundary.
  if (SizeBound.has_value()) {
    auto Offset = FMgr.getOffset();
    if (Offset < SizeBound.value()) {
      return logLoadError(ErrCode::Value::JunkSection, Offset,
                          ASTNodeAttr::Instruction);
    } else if (Offset > SizeBound.value()) {
      return logLoadError(ErrCode::Value::SectionSizeMismatch, Offset,
                          ASTNodeAttr::Instruction);
    }
  }
  return Instrs;
}

// Load instruction node. See "include/loader/loader.h".
Expect<void> Loader::loadInstruction(AST::Instruction &Instr) {
  // Node: The instruction has checked for the proposals. Need to check their
  // immediates.

  auto readU8 = [this](uint8_t &Dst) -> Expect<void> {
    if (auto Res = FMgr.readByte()) {
      Dst = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  };

  auto readU32 = [this](uint32_t &Dst) -> Expect<void> {
    if (auto Res = FMgr.readU32()) {
      Dst = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  };

  auto readMemImmediate = [this, readU32, &Instr]() -> Expect<void> {
    Instr.getTargetIndex() = 0;
    if (auto Res = readU32(Instr.getMemoryAlign()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    if (Instr.getMemoryAlign() >= 128) {
      return logLoadError(ErrCode::Value::InvalidStoreAlignment,
                          FMgr.getLastOffset(), ASTNodeAttr::Instruction);
    } else if (Instr.getMemoryAlign() >= 64) {
      if (Conf.hasProposal(Proposal::MultiMemories)) {
        Instr.getMemoryAlign() -= 64;
        if (auto Res = readU32(Instr.getTargetIndex()); unlikely(!Res)) {
          return Unexpect(Res);
        }
      } else {
        return logLoadError(ErrCode::Value::InvalidStoreAlignment,
                            FMgr.getLastOffset(), ASTNodeAttr::Instruction);
      }
    }
    if (auto Res = readU32(Instr.getMemoryOffset()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    return {};
  };

  auto readCheckZero = [this, readU8](uint32_t &Dst) -> Expect<void> {
    uint8_t C = 0;
    if (auto Res = readU8(C); unlikely(!Res)) {
      return Unexpect(Res);
    }
    if (C != UINT8_C(0)) {
      return logLoadError(ErrCode::Value::ExpectedZeroByte,
                          FMgr.getLastOffset(), ASTNodeAttr::Instruction);
    }
    Dst = 0;
    return {};
  };

  auto readBlockType = [this](BlockType &Dst) -> Expect<void> {
    auto StartOffset = FMgr.getOffset();
    // Read the block return type.
    if (auto Res = FMgr.readS33()) {
      if (*Res < 0) {
        TypeCode TypeByte = static_cast<TypeCode>((*Res) & INT64_C(0x7F));
        if (TypeByte == TypeCode::Epsilon) {
          // Empty case.
          Dst.setEmpty();
        } else {
          // Value type case. Seek back to the origin offset and read the
          // valtype.
          FMgr.seek(StartOffset);
          if (auto TypeRes = loadValType(ASTNodeAttr::Instruction)) {
            Dst.setData(*TypeRes);
          } else {
            // The AST node information is handled.
            return Unexpect(TypeRes);
          }
        }
      } else {
        // Type index case.
        if (unlikely(!Conf.hasProposal(Proposal::MultiValue))) {
          return logNeedProposal(ErrCode::Value::MalformedValType,
                                 Proposal::MultiValue, FMgr.getLastOffset(),
                                 ASTNodeAttr::Instruction);
        }
        Dst.setData(static_cast<uint32_t>(*Res));
      }
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  };

  switch (Instr.getOpCode()) {
  // Control instructions.
  case OpCode::Unreachable:
  case OpCode::Nop:
  case OpCode::Return:
  case OpCode::Throw_ref:
  case OpCode::End:
  case OpCode::Else:
  // LEGACY-EH: remove the `Catch_all` case after deprecating legacy EH.
  case OpCode::Catch_all:
    return {};

  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If:
    return readBlockType(Instr.getBlockType());

  case OpCode::Try_table: {
    Instr.setTryCatch();
    // Read the result type.
    if (auto Res = readBlockType(Instr.getTryCatch().ResType); !Res) {
      return Unexpect(Res);
    }
    uint32_t VecCnt = 0;
    // Read the vector of catch.
    if (auto Res = loadVecCnt()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    Instr.getTryCatch().Catch.resize(VecCnt);
    for (uint32_t I = 0; I < VecCnt; ++I) {
      auto &Desc = Instr.getTryCatch().Catch[I];
      // Read the catch flag.
      if (auto Res = FMgr.readByte()) {
        // LEGACY-EH: remove this flag after deprecating legacy EH.
        Desc.IsLegacy = false;
        Desc.IsRef = (*Res & 0x01U) ? true : false;
        Desc.IsAll = (*Res & 0x02U) ? true : false;
      } else {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Instruction);
      }
      if (!Desc.IsAll) {
        // Read the tag index.
        if (auto Res = readU32(Desc.TagIndex); !Res) {
          return Unexpect(Res);
        }
      }
      // Read the label index.
      if (auto Res = readU32(Desc.LabelIndex); !Res) {
        return Unexpect(Res);
      }
    }
    return {};
  }

  // LEGACY-EH: remove the `Try` case after deprecating legacy EH.
  case OpCode::Try:
    Instr.setTryCatch();
    return readBlockType(Instr.getTryCatch().ResType);

  // LEGACY-EH: remove the `Catch` case after deprecating legacy EH.
  case OpCode::Catch:
    return readU32(Instr.getCatchLegacy().TagIndex);

  case OpCode::Throw:
    return readU32(Instr.getTargetIndex());

  // LEGACY-EH: remove the `Rethrow` case after deprecating legacy EH.
  case OpCode::Rethrow:
    spdlog::error(ErrCode::Value::IllegalOpCode);
    spdlog::error("    Deprecated `rethrow` instruction.");
    return Unexpect(ErrCode::Value::IllegalOpCode);

  case OpCode::Br:
  case OpCode::Br_if:
  case OpCode::Br_on_null:
  case OpCode::Br_on_non_null:
    return readU32(Instr.getJump().TargetIndex);

  // LEGACY-EH: remove the `Delegate` case after deprecating legacy EH.
  case OpCode::Delegate:
    spdlog::error(ErrCode::Value::IllegalOpCode);
    spdlog::error("    Deprecated `delegate` instruction.");
    return Unexpect(ErrCode::Value::IllegalOpCode);

  case OpCode::Br_table: {
    uint32_t VecCnt = 0;
    // Read the vector of labels.
    if (auto Res = loadVecCnt()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    Instr.setLabelListSize(VecCnt + 1);
    for (uint32_t I = 0; I < VecCnt; ++I) {
      if (auto Res = readU32(Instr.getLabelList()[I].TargetIndex);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    }
    // Read default label.
    return readU32(Instr.getLabelList()[VecCnt].TargetIndex);
  }

  case OpCode::Call:
  case OpCode::Return_call:
  case OpCode::Call_ref:
  case OpCode::Return_call_ref:
    return readU32(Instr.getTargetIndex());

  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect: {
    // Read the type index.
    if (auto Res = readU32(Instr.getTargetIndex()); !Res) {
      return Unexpect(Res);
    }
    uint64_t SrcIdxOffset = FMgr.getOffset();
    // Read the table index.
    if (auto Res = readU32(Instr.getSourceIndex()); !Res) {
      return Unexpect(Res);
    }
    if ((Instr.getSourceIndex() > 0 || FMgr.getOffset() - SrcIdxOffset > 1) &&
        !Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                             Proposal::ReferenceTypes, FMgr.getLastOffset(),
                             ASTNodeAttr::Instruction);
    }
    return {};
  }

  // Reference Instructions.
  case OpCode::Ref__null:
  case OpCode::Ref__test_null:
  case OpCode::Ref__cast_null:
    if (auto Res = loadHeapType(TypeCode::RefNull, ASTNodeAttr::Instruction)) {
      Instr.setValType(*Res);
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
    }
    return {};
  case OpCode::Ref__test:
  case OpCode::Ref__cast:
    if (auto Res = loadHeapType(TypeCode::Ref, ASTNodeAttr::Instruction)) {
      Instr.setValType(*Res);
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
    }
    return {};
  case OpCode::Ref__is_null:
  case OpCode::Ref__eq:
  case OpCode::Ref__as_non_null:
    return {};
  case OpCode::Ref__func:
  case OpCode::Struct__new:
  case OpCode::Struct__new_default:
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u:
  case OpCode::Array__set:
  case OpCode::Array__fill:
    return readU32(Instr.getTargetIndex());
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set:
  case OpCode::Array__new_fixed:
  case OpCode::Array__new_data:
  case OpCode::Array__new_elem:
  case OpCode::Array__copy:
  case OpCode::Array__init_data:
  case OpCode::Array__init_elem:
    if (auto Res = readU32(Instr.getTargetIndex()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    return readU32(Instr.getSourceIndex());
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail: {
    // Read the flag.
    uint8_t Flag = 0U;
    if (auto Res = readU8(Flag); !Res) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    // Read the label index.
    uint32_t LabelIdx = 0U;
    if (auto Res = readU32(LabelIdx); !Res) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    // Read the heap types.
    Instr.setBrCast(LabelIdx);
    if (auto Res =
            loadHeapType(((Flag & 0x01U) ? TypeCode::RefNull : TypeCode::Ref),
                         ASTNodeAttr::Instruction)) {
      Instr.getBrCast().RType1 = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    if (auto Res =
            loadHeapType(((Flag & 0x02U) ? TypeCode::RefNull : TypeCode::Ref),
                         ASTNodeAttr::Instruction)) {
      Instr.getBrCast().RType2 = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  }
  case OpCode::Array__len:
  case OpCode::Any__convert_extern:
  case OpCode::Extern__convert_any:
  case OpCode::Ref__i31:
  case OpCode::I31__get_s:
  case OpCode::I31__get_u:
    return {};

  // Parametric Instructions.
  case OpCode::Drop:
  case OpCode::Select:
    return {};
  case OpCode::Select_t: {
    // Read the vector of value types.
    uint32_t VecCnt = 0;
    if (auto Res = loadVecCnt()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    }
    Instr.setValTypeListSize(VecCnt);
    for (uint32_t I = 0; I < VecCnt; ++I) {
      if (auto Res = loadValType(ASTNodeAttr::Instruction)) {
        Instr.getValTypeList()[I] = *Res;
      } else {
        // The AST node information is handled.
        return Unexpect(Res);
      }
    }
    return {};
  }

  // Variable Instructions.
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    return readU32(Instr.getTargetIndex());

  // Table Instructions.
  case OpCode::Table__init:
    if (auto Res = readU32(Instr.getSourceIndex()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  case OpCode::Elem__drop:
    return readU32(Instr.getTargetIndex());
  case OpCode::Table__copy:
    if (auto Res = readU32(Instr.getTargetIndex()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    return readU32(Instr.getSourceIndex());

  // Memory Instructions.
  case OpCode::I32__load:
  case OpCode::I64__load:
  case OpCode::F32__load:
  case OpCode::F64__load:
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I32__store:
  case OpCode::I64__store:
  case OpCode::F32__store:
  case OpCode::F64__store:
  case OpCode::I32__store8:
  case OpCode::I32__store16:
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
    return readMemImmediate();

  case OpCode::Memory__init:
    if (!HasDataSection) {
      return logLoadError(ErrCode::Value::DataCountRequired, Instr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    if (auto Res = readU32(Instr.getSourceIndex()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Memory__grow:
  case OpCode::Memory__size:
  case OpCode::Memory__fill:
    if (Conf.hasProposal(Proposal::MultiMemories)) {
      return readU32(Instr.getTargetIndex());
    }
    return readCheckZero(Instr.getTargetIndex());
  case OpCode::Memory__copy:
    if (Conf.hasProposal(Proposal::MultiMemories)) {
      if (auto Res = readU32(Instr.getTargetIndex()); unlikely(!Res)) {
        return Unexpect(Res);
      }
      return readU32(Instr.getSourceIndex());
    }
    if (auto Res = readCheckZero(Instr.getTargetIndex()); unlikely(!Res)) {
      return Unexpect(Res);
    }
    return readCheckZero(Instr.getSourceIndex());
  case OpCode::Data__drop:
    if (!HasDataSection) {
      return logLoadError(ErrCode::Value::DataCountRequired, Instr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return readU32(Instr.getTargetIndex());

  // Const Instructions.
  case OpCode::I32__const:
    if (auto Res = FMgr.readS32(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    } else {
      Instr.setNum(static_cast<uint128_t>(static_cast<uint32_t>(*Res)));
    }
    return {};
  case OpCode::I64__const:
    if (auto Res = FMgr.readS64(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    } else {
      Instr.setNum(static_cast<uint128_t>(static_cast<uint64_t>(*Res)));
    }
    return {};
  case OpCode::F32__const:
    if (auto Res = FMgr.readF32(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    } else {
      Instr.setNum(*Res);
    }
    return {};
  case OpCode::F64__const:
    if (auto Res = FMgr.readF64(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Instruction);
    } else {
      Instr.setNum(*Res);
    }
    return {};

  // Unary Numeric Instructions.
  case OpCode::I32__eqz:
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
  case OpCode::I64__eqz:
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
  case OpCode::I32__wrap_i64:
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
  case OpCode::F32__demote_f64:
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
  case OpCode::F64__promote_f32:
  case OpCode::I32__reinterpret_f32:
  case OpCode::I64__reinterpret_f64:
  case OpCode::F32__reinterpret_i32:
  case OpCode::F64__reinterpret_i64:
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:

  // Binary Numeric Instructions.
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I32__lt_s:
  case OpCode::I32__lt_u:
  case OpCode::I32__gt_s:
  case OpCode::I32__gt_u:
  case OpCode::I32__le_s:
  case OpCode::I32__le_u:
  case OpCode::I32__ge_s:
  case OpCode::I32__ge_u:
  case OpCode::I64__eq:
  case OpCode::I64__ne:
  case OpCode::I64__lt_s:
  case OpCode::I64__lt_u:
  case OpCode::I64__gt_s:
  case OpCode::I64__gt_u:
  case OpCode::I64__le_s:
  case OpCode::I64__le_u:
  case OpCode::I64__ge_s:
  case OpCode::I64__ge_u:
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:

  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
  case OpCode::I64__and:
  case OpCode::I64__or:
  case OpCode::I64__xor:
  case OpCode::I64__shl:
  case OpCode::I64__shr_s:
  case OpCode::I64__shr_u:
  case OpCode::I64__rotl:
  case OpCode::I64__rotr:
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return {};

  // SIMD Memory Instruction.
  case OpCode::V128__load:
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
  case OpCode::V128__load8_splat:
  case OpCode::V128__load16_splat:
  case OpCode::V128__load32_splat:
  case OpCode::V128__load64_splat:
  case OpCode::V128__load32_zero:
  case OpCode::V128__load64_zero:
  case OpCode::V128__store:
    return readMemImmediate();
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
    // Read memory immediate.
    if (auto Res = readMemImmediate(); unlikely(!Res)) {
      return Unexpect(Res);
    }
    // Read lane index.
    return readU8(Instr.getMemoryLane());

  // SIMD Const Instruction.
  case OpCode::V128__const:
  // SIMD Shuffle Instruction.
  case OpCode::I8x16__shuffle: {
    // Read value.
    uint128_t Value = 0;
    for (uint32_t I = 0; I < 16; ++I) {
      if (auto Res = FMgr.readByte(); unlikely(!Res)) {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Instruction);
      } else {
        Value |= static_cast<uint128_t>(*Res) << (I * 8);
      }
    }
    Instr.setNum(Value);
    return {};
  }

  // SIMD Lane Instructions.
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
  case OpCode::I8x16__replace_lane:
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
  case OpCode::I16x8__replace_lane:
  case OpCode::I32x4__extract_lane:
  case OpCode::I32x4__replace_lane:
  case OpCode::I64x2__extract_lane:
  case OpCode::I64x2__replace_lane:
  case OpCode::F32x4__extract_lane:
  case OpCode::F32x4__replace_lane:
  case OpCode::F64x2__extract_lane:
  case OpCode::F64x2__replace_lane:
    // Read lane index.
    return readU8(Instr.getMemoryLane());

  // SIMD Numeric Instructions.
  case OpCode::I8x16__swizzle:
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
  case OpCode::I64x2__splat:
  case OpCode::F32x4__splat:
  case OpCode::F64x2__splat:

  case OpCode::I8x16__eq:
  case OpCode::I8x16__ne:
  case OpCode::I8x16__lt_s:
  case OpCode::I8x16__lt_u:
  case OpCode::I8x16__gt_s:
  case OpCode::I8x16__gt_u:
  case OpCode::I8x16__le_s:
  case OpCode::I8x16__le_u:
  case OpCode::I8x16__ge_s:
  case OpCode::I8x16__ge_u:

  case OpCode::I16x8__eq:
  case OpCode::I16x8__ne:
  case OpCode::I16x8__lt_s:
  case OpCode::I16x8__lt_u:
  case OpCode::I16x8__gt_s:
  case OpCode::I16x8__gt_u:
  case OpCode::I16x8__le_s:
  case OpCode::I16x8__le_u:
  case OpCode::I16x8__ge_s:
  case OpCode::I16x8__ge_u:

  case OpCode::I32x4__eq:
  case OpCode::I32x4__ne:
  case OpCode::I32x4__lt_s:
  case OpCode::I32x4__lt_u:
  case OpCode::I32x4__gt_s:
  case OpCode::I32x4__gt_u:
  case OpCode::I32x4__le_s:
  case OpCode::I32x4__le_u:
  case OpCode::I32x4__ge_s:
  case OpCode::I32x4__ge_u:

  case OpCode::F32x4__eq:
  case OpCode::F32x4__ne:
  case OpCode::F32x4__lt:
  case OpCode::F32x4__gt:
  case OpCode::F32x4__le:
  case OpCode::F32x4__ge:

  case OpCode::F64x2__eq:
  case OpCode::F64x2__ne:
  case OpCode::F64x2__lt:
  case OpCode::F64x2__gt:
  case OpCode::F64x2__le:
  case OpCode::F64x2__ge:

  case OpCode::V128__not:
  case OpCode::V128__and:
  case OpCode::V128__andnot:
  case OpCode::V128__or:
  case OpCode::V128__xor:
  case OpCode::V128__bitselect:
  case OpCode::V128__any_true:

  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I8x16__popcnt:
  case OpCode::I8x16__all_true:
  case OpCode::I8x16__bitmask:
  case OpCode::I8x16__narrow_i16x8_s:
  case OpCode::I8x16__narrow_i16x8_u:
  case OpCode::I8x16__shl:
  case OpCode::I8x16__shr_s:
  case OpCode::I8x16__shr_u:
  case OpCode::I8x16__add:
  case OpCode::I8x16__add_sat_s:
  case OpCode::I8x16__add_sat_u:
  case OpCode::I8x16__sub:
  case OpCode::I8x16__sub_sat_s:
  case OpCode::I8x16__sub_sat_u:
  case OpCode::I8x16__min_s:
  case OpCode::I8x16__min_u:
  case OpCode::I8x16__max_s:
  case OpCode::I8x16__max_u:
  case OpCode::I8x16__avgr_u:

  case OpCode::I16x8__abs:
  case OpCode::I16x8__neg:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I16x8__narrow_i32x4_s:
  case OpCode::I16x8__narrow_i32x4_u:
  case OpCode::I16x8__extend_low_i8x16_s:
  case OpCode::I16x8__extend_high_i8x16_s:
  case OpCode::I16x8__extend_low_i8x16_u:
  case OpCode::I16x8__extend_high_i8x16_u:
  case OpCode::I16x8__shl:
  case OpCode::I16x8__shr_s:
  case OpCode::I16x8__shr_u:
  case OpCode::I16x8__add:
  case OpCode::I16x8__add_sat_s:
  case OpCode::I16x8__add_sat_u:
  case OpCode::I16x8__sub:
  case OpCode::I16x8__sub_sat_s:
  case OpCode::I16x8__sub_sat_u:
  case OpCode::I16x8__mul:
  case OpCode::I16x8__min_s:
  case OpCode::I16x8__min_u:
  case OpCode::I16x8__max_s:
  case OpCode::I16x8__max_u:
  case OpCode::I16x8__avgr_u:
  case OpCode::I16x8__extmul_low_i8x16_s:
  case OpCode::I16x8__extmul_high_i8x16_s:
  case OpCode::I16x8__extmul_low_i8x16_u:
  case OpCode::I16x8__extmul_high_i8x16_u:
  case OpCode::I16x8__q15mulr_sat_s:
  case OpCode::I16x8__extadd_pairwise_i8x16_s:
  case OpCode::I16x8__extadd_pairwise_i8x16_u:

  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I32x4__extend_low_i16x8_s:
  case OpCode::I32x4__extend_high_i16x8_s:
  case OpCode::I32x4__extend_low_i16x8_u:
  case OpCode::I32x4__extend_high_i16x8_u:
  case OpCode::I32x4__shl:
  case OpCode::I32x4__shr_s:
  case OpCode::I32x4__shr_u:
  case OpCode::I32x4__add:
  case OpCode::I32x4__sub:
  case OpCode::I32x4__mul:
  case OpCode::I32x4__min_s:
  case OpCode::I32x4__min_u:
  case OpCode::I32x4__max_s:
  case OpCode::I32x4__max_u:
  case OpCode::I32x4__extmul_low_i16x8_s:
  case OpCode::I32x4__extmul_high_i16x8_s:
  case OpCode::I32x4__extmul_low_i16x8_u:
  case OpCode::I32x4__extmul_high_i16x8_u:
  case OpCode::I32x4__extadd_pairwise_i16x8_s:
  case OpCode::I32x4__extadd_pairwise_i16x8_u:

  case OpCode::I64x2__abs:
  case OpCode::I64x2__neg:
  case OpCode::I64x2__bitmask:
  case OpCode::I64x2__extend_low_i32x4_s:
  case OpCode::I64x2__extend_high_i32x4_s:
  case OpCode::I64x2__extend_low_i32x4_u:
  case OpCode::I64x2__extend_high_i32x4_u:
  case OpCode::I64x2__shl:
  case OpCode::I64x2__shr_s:
  case OpCode::I64x2__shr_u:
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:
  case OpCode::I64x2__eq:
  case OpCode::I64x2__ne:
  case OpCode::I64x2__lt_s:
  case OpCode::I64x2__gt_s:
  case OpCode::I64x2__le_s:
  case OpCode::I64x2__ge_s:
  case OpCode::I64x2__all_true:
  case OpCode::I64x2__extmul_low_i32x4_s:
  case OpCode::I64x2__extmul_high_i32x4_s:
  case OpCode::I64x2__extmul_low_i32x4_u:
  case OpCode::I64x2__extmul_high_i32x4_u:

  case OpCode::F32x4__abs:
  case OpCode::F32x4__neg:
  case OpCode::F32x4__sqrt:
  case OpCode::F32x4__add:
  case OpCode::F32x4__sub:
  case OpCode::F32x4__mul:
  case OpCode::F32x4__div:
  case OpCode::F32x4__min:
  case OpCode::F32x4__max:
  case OpCode::F32x4__pmin:
  case OpCode::F32x4__pmax:

  case OpCode::F64x2__abs:
  case OpCode::F64x2__neg:
  case OpCode::F64x2__sqrt:
  case OpCode::F64x2__add:
  case OpCode::F64x2__sub:
  case OpCode::F64x2__mul:
  case OpCode::F64x2__div:
  case OpCode::F64x2__min:
  case OpCode::F64x2__max:
  case OpCode::F64x2__pmin:
  case OpCode::F64x2__pmax:

  case OpCode::I32x4__trunc_sat_f32x4_s:
  case OpCode::I32x4__trunc_sat_f32x4_u:
  case OpCode::F32x4__convert_i32x4_s:
  case OpCode::F32x4__convert_i32x4_u:
  case OpCode::I32x4__trunc_sat_f64x2_s_zero:
  case OpCode::I32x4__trunc_sat_f64x2_u_zero:
  case OpCode::F64x2__convert_low_i32x4_s:
  case OpCode::F64x2__convert_low_i32x4_u:
  case OpCode::F32x4__demote_f64x2_zero:
  case OpCode::F64x2__promote_low_f32x4:

  case OpCode::I32x4__dot_i16x8_s:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
    return {};

  case OpCode::I8x16__relaxed_swizzle:
  case OpCode::I32x4__relaxed_trunc_f32x4_s:
  case OpCode::I32x4__relaxed_trunc_f32x4_u:
  case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
  case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
  case OpCode::F32x4__relaxed_madd:
  case OpCode::F32x4__relaxed_nmadd:
  case OpCode::F64x2__relaxed_madd:
  case OpCode::F64x2__relaxed_nmadd:
  case OpCode::I8x16__relaxed_laneselect:
  case OpCode::I16x8__relaxed_laneselect:
  case OpCode::I32x4__relaxed_laneselect:
  case OpCode::I64x2__relaxed_laneselect:
  case OpCode::F32x4__relaxed_min:
  case OpCode::F32x4__relaxed_max:
  case OpCode::F64x2__relaxed_min:
  case OpCode::F64x2__relaxed_max:
  case OpCode::I16x8__relaxed_q15mulr_s:
  case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
  case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
    return {};

  // Atomic Memory Instructions.
  case OpCode::Atomic__fence:
    return readCheckZero(Instr.getTargetIndex());

  case OpCode::Memory__atomic__notify:
  case OpCode::Memory__atomic__wait32:
  case OpCode::Memory__atomic__wait64:

  case OpCode::I32__atomic__load:
  case OpCode::I64__atomic__load:
  case OpCode::I32__atomic__load8_u:
  case OpCode::I32__atomic__load16_u:
  case OpCode::I64__atomic__load8_u:
  case OpCode::I64__atomic__load16_u:
  case OpCode::I64__atomic__load32_u:
  case OpCode::I32__atomic__store:
  case OpCode::I64__atomic__store:
  case OpCode::I32__atomic__store8:
  case OpCode::I32__atomic__store16:
  case OpCode::I64__atomic__store8:
  case OpCode::I64__atomic__store16:
  case OpCode::I64__atomic__store32:
  case OpCode::I32__atomic__rmw__add:
  case OpCode::I64__atomic__rmw__add:
  case OpCode::I32__atomic__rmw8__add_u:
  case OpCode::I32__atomic__rmw16__add_u:
  case OpCode::I64__atomic__rmw8__add_u:
  case OpCode::I64__atomic__rmw16__add_u:
  case OpCode::I64__atomic__rmw32__add_u:
  case OpCode::I32__atomic__rmw__sub:
  case OpCode::I64__atomic__rmw__sub:
  case OpCode::I32__atomic__rmw8__sub_u:
  case OpCode::I32__atomic__rmw16__sub_u:
  case OpCode::I64__atomic__rmw8__sub_u:
  case OpCode::I64__atomic__rmw16__sub_u:
  case OpCode::I64__atomic__rmw32__sub_u:
  case OpCode::I32__atomic__rmw__and:
  case OpCode::I64__atomic__rmw__and:
  case OpCode::I32__atomic__rmw8__and_u:
  case OpCode::I32__atomic__rmw16__and_u:
  case OpCode::I64__atomic__rmw8__and_u:
  case OpCode::I64__atomic__rmw16__and_u:
  case OpCode::I64__atomic__rmw32__and_u:
  case OpCode::I32__atomic__rmw__or:
  case OpCode::I64__atomic__rmw__or:
  case OpCode::I32__atomic__rmw8__or_u:
  case OpCode::I32__atomic__rmw16__or_u:
  case OpCode::I64__atomic__rmw8__or_u:
  case OpCode::I64__atomic__rmw16__or_u:
  case OpCode::I64__atomic__rmw32__or_u:
  case OpCode::I32__atomic__rmw__xor:
  case OpCode::I64__atomic__rmw__xor:
  case OpCode::I32__atomic__rmw8__xor_u:
  case OpCode::I32__atomic__rmw16__xor_u:
  case OpCode::I64__atomic__rmw8__xor_u:
  case OpCode::I64__atomic__rmw16__xor_u:
  case OpCode::I64__atomic__rmw32__xor_u:
  case OpCode::I32__atomic__rmw__xchg:
  case OpCode::I64__atomic__rmw__xchg:
  case OpCode::I32__atomic__rmw8__xchg_u:
  case OpCode::I32__atomic__rmw16__xchg_u:
  case OpCode::I64__atomic__rmw8__xchg_u:
  case OpCode::I64__atomic__rmw16__xchg_u:
  case OpCode::I64__atomic__rmw32__xchg_u:
  case OpCode::I32__atomic__rmw__cmpxchg:
  case OpCode::I64__atomic__rmw__cmpxchg:
  case OpCode::I32__atomic__rmw8__cmpxchg_u:
  case OpCode::I32__atomic__rmw16__cmpxchg_u:
  case OpCode::I64__atomic__rmw8__cmpxchg_u:
  case OpCode::I64__atomic__rmw16__cmpxchg_u:
  case OpCode::I64__atomic__rmw32__cmpxchg_u:
    return readMemImmediate();

  default:
    assumingUnreachable();
  }
}

} // namespace Loader
} // namespace WasmEdge
