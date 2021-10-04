// SPDX-License-Identifier: Apache-2.0

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

/// Load binary of GlobalSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::GlobalSegment &GlobSeg) {
  /// Read global type node.
  if (auto Res = loadType(GlobSeg.getGlobalType()); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }

  /// Read the expression.
  if (auto Res = loadExpression(GlobSeg.getExpr()); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of ElementSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::ElementSegment &ElemSeg) {
  /// Element segment binary format:
  /// ---------------------------------------------------------------------------
  ///  byte | TableIdx | OffExpr | ElemKind | RefType | vec(FuncIdx) | vec(expr)
  /// ------|----------|---------|----------|---------|--------------|-----------
  ///  0x00 |          |    v    |          |         |       v      |
  ///  0x01 |          |         |    v     |         |       v      |
  ///  0x02 |    v     |    v    |    v     |         |       v      |
  ///  0x03 |          |         |    v     |         |       v      |
  ///  0x04 |          |    v    |          |         |              |     v
  ///  0x05 |          |         |          |    v    |              |     v
  ///  0x06 |    v     |    v    |          |    v    |              |     v
  ///  0x07 |          |         |          |    v    |              |     v
  /// ---------------------------------------------------------------------------
  /// TableIdx: target table index, u32
  /// OffExpr: init offset expression, expr
  /// ElemKind: byte 0x00, RefType::FuncRef
  /// RefType: reference type, RefType
  /// vec(FuncIdx): function index vector, vec(u32)
  /// vec(expr): reference init list, vec(expr)

  /// Read the checking byte.
  uint32_t Check;
  if (auto Res = FMgr.readU32()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Element);
  }
  /// Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations, FMgr.getLastOffset(),
                           ASTNodeAttr::Seg_Element);
  }

  /// Check the prefix byte.
  switch (Check) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Active);
    break;

  case 0x01:
  case 0x05:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Passive);
    break;

  case 0x03:
  case 0x07:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Declarative);
    break;

  default:
    /// TODO: Correctness the error code once there's spec test.
    return logLoadError(ErrCode::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Element);
  }

  /// Read the table index.
  ElemSeg.setIdx(0);
  switch (Check) {
  case 0x02:
  case 0x06:
    if (auto Res = FMgr.readU32()) {
      ElemSeg.setIdx(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    break;

  default:
    break;
  }

  /// Read the expression.
  switch (Check) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    if (auto Res = loadExpression(ElemSeg.getExpr()); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return Unexpect(Res);
    }
    break;

  default:
    break;
  }

  /// Read element kind and init function indices.
  ElemSeg.setRefType(RefType::FuncRef);
  switch (Check) {
  case 0x01:
  case 0x02:
  case 0x03:
    if (auto Res = FMgr.readByte()) {
      if (*Res != 0x00U) {
        return logLoadError(ErrCode::ExpectedZeroByte, FMgr.getLastOffset(),
                            ASTNodeAttr::Seg_Element);
      }
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    [[fallthrough]];

  case 0x00: {
    uint32_t VecCnt = 0;
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    for (uint32_t I = 0; I < VecCnt; ++I) {
      /// For each element in vec(funcidx), make expr(ref.func idx end).
      ElemSeg.getInitExprs().emplace_back();
      AST::Instruction RefFunc(OpCode::Ref__func);
      AST::Instruction End(OpCode::End);
      if (auto Res = loadInstruction(RefFunc); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return Unexpect(Res);
      }
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(
          std::move(RefFunc));
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
    }
    break;
  }
  default:
    break;
  }

  /// Read the reference type and init expressions.
  switch (Check) {
  case 0x05:
  case 0x06:
  case 0x07:
    if (auto Res = FMgr.readByte(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    } else {
      ElemSeg.setRefType(static_cast<RefType>(*Res));
    }
    if (auto Res =
            checkRefTypeProposals(ElemSeg.getRefType(), FMgr.getLastOffset(),
                                  ASTNodeAttr::Seg_Element);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    [[fallthrough]];

  case 0x04: {
    uint32_t VecCnt = 0;
    if (auto Res = FMgr.readU32(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    } else {
      VecCnt = *Res;
    }
    ElemSeg.getInitExprs().reserve(VecCnt);
    for (uint32_t I = 0; I < VecCnt; ++I) {
      ElemSeg.getInitExprs().emplace_back();
      if (auto Res = loadExpression(ElemSeg.getInitExprs().back())) {
        for (auto &Instr : ElemSeg.getInitExprs().back().getInstrs()) {
          OpCode Code = Instr.getOpCode();
          if (Code != OpCode::Ref__func && Code != OpCode::Ref__null &&
              Code != OpCode::End) {
            return logLoadError(ErrCode::IllegalOpCode, Instr.getOffset(),
                                ASTNodeAttr::Seg_Element);
          }
        }
      } else {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return Unexpect(Res.error());
      }
    }
    break;
  }

  default:
    break;
  }

  return {};
}

/// Load binary of CodeSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::CodeSegment &CodeSeg) {
  /// Read the code segment size.
  if (auto Res = FMgr.readU32()) {
    CodeSeg.setSegSize(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Code);
  }

  /// Read the vector of local variable counts and types.
  uint32_t VecCnt = 0;
  if (auto Res = FMgr.readU32()) {
    VecCnt = *Res;
    CodeSeg.getLocals().reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Code);
  }
  uint32_t TotalLocalCnt = 0;
  for (uint32_t I = 0; I < VecCnt; ++I) {
    uint32_t LocalCnt = 0;
    ValType LocalType = ValType::None;
    if (auto Res = FMgr.readU32(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Code);
    } else {
      LocalCnt = *Res;
    }
    /// Total local variables should not more than 2^32.
    if (UINT32_MAX - TotalLocalCnt < LocalCnt) {
      return logLoadError(ErrCode::TooManyLocals, FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Code);
    }
    TotalLocalCnt += LocalCnt;
    /// Read the number type.
    if (auto Res = FMgr.readByte(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Code);
    } else {
      LocalType = static_cast<ValType>(*Res);
    }
    if (auto Res = checkValTypeProposals(LocalType, FMgr.getLastOffset(),
                                         ASTNodeAttr::Seg_Code);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    CodeSeg.getLocals().push_back(std::make_pair(LocalCnt, LocalType));
  }

  /// Read function body.
  if (auto Res = loadExpression(CodeSeg.getExpr()); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Code));
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of DataSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::DataSegment &DataSeg) {
  DataSeg.setMode(AST::DataSegment::DataMode::Passive);
  DataSeg.setIdx(0);

  /// Data segment binary format:
  /// ----------------------------------------
  ///  byte | MemoryIdx | OffExpr | vec(byte)
  /// ------|-----------|---------|-----------
  ///  0x00 |           |    v    |     v
  ///  0x01 |           |         |     v
  ///  0x02 |     v     |    v    |     v
  /// ----------------------------------------
  /// MemoryIdx: target memory index, u32
  /// OffExpr: init offset expression, expr
  /// vec(byte): init data, vec(u8)

  /// Read the checking byte.
  uint32_t Check;
  if (auto Res = FMgr.readU32()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Data);
  }
  /// Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations, FMgr.getLastOffset(),
                           ASTNodeAttr::Seg_Data);
  }

  switch (Check) {
  case 0x02: /// 0x02 memidx expr vec(byte) , Active
    /// Read target memory index.
    if (auto Res = FMgr.readU32()) {
      DataSeg.setIdx(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    [[fallthrough]];

  case 0x00: /// 0x00 expr vec(byte) , Active
    /// Read the offset expression.
    if (auto Res = loadExpression(DataSeg.getExpr()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return Unexpect(Res);
    }
    DataSeg.setMode(AST::DataSegment::DataMode::Active);
    [[fallthrough]];

  case 0x01: /// 0x01 vec(byte) , Passive
  {
    /// Read initialization data.
    uint32_t VecCnt = 0;
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    if (auto Res = FMgr.readBytes(VecCnt)) {
      DataSeg.getData() = std::move(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    break;
  }
  default:
    /// TODO: Correctness the error code once there's spec test.
    return logLoadError(ErrCode::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Data);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
