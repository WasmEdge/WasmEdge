// SPDX-License-Identifier: Apache-2.0
#include "ast/segment.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Load expression binary in segment. See "include/ast/segment.h".
Expect<void> Segment::loadExpression(FileMgr &Mgr,
                                     const ProposalConfigure &PConf) {
  return Expr.loadBinary(Mgr, PConf);
}

/// Load binary of GlobalSegment node. See "include/ast/segment.h".
Expect<void> GlobalSegment::loadBinary(FileMgr &Mgr,
                                       const ProposalConfigure &PConf) {
  /// Read global type node.
  if (auto Res = Global.loadBinary(Mgr, PConf); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the expression.
  if (auto Res = Segment::loadExpression(Mgr, PConf); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of ElementSegment node. See "include/ast/segment.h".
Expect<void> ElementSegment::loadBinary(FileMgr &Mgr,
                                        const ProposalConfigure &PConf) {
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
  uint8_t Check;
  if (auto Res = Mgr.readByte()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  /// Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !PConf.hasProposal(Proposal::BulkMemoryOperations) &&
      !PConf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::InvalidGrammar,
                           Proposal::BulkMemoryOperations, Mgr.getOffset() - 1,
                           NodeAttr);
  }

  /// Check the prefix byte.
  switch (Check) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    Mode = ElemMode::Active;
    break;

  case 0x01:
  case 0x05:
    Mode = ElemMode::Passive;
    break;

  case 0x03:
  case 0x07:
    Mode = ElemMode::Declarative;
    break;

  default:
    return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1, NodeAttr);
  }

  /// Read the table index.
  TableIdx = 0;
  switch (Check) {
  case 0x02:
  case 0x06:
    if (auto Res = Mgr.readU32()) {
      TableIdx = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset() - 1, NodeAttr);
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
    if (auto Res = Segment::loadExpression(Mgr, PConf); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    break;

  default:
    break;
  }

  /// Read element kind and init function indices.
  Type = RefType::FuncRef;
  switch (Check) {
  case 0x01:
  case 0x02:
  case 0x03:
    if (auto Res = Mgr.readByte()) {
      if (*Res != 0x00U) {
        return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                            NodeAttr);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    [[fallthrough]];

  case 0x00: {
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    for (uint32_t i = 0; i < VecCnt; ++i) {
      /// For each element in vec(funcidx), make expr(ref.func idx end).
      InitExprs.emplace_back();
      Instruction RefFunc(OpCode::Ref__func);
      Instruction End(OpCode::End);
      if (auto Res = RefFunc.loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
      InitExprs.back().pushInstr(std::move(RefFunc));
      InitExprs.back().pushInstr(std::move(End));
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
    if (auto Res = Mgr.readByte()) {
      Type = static_cast<RefType>(*Res);
      if (auto Check =
              checkRefTypeProposals(PConf, Type, Mgr.getOffset() - 1, NodeAttr);
          !Check) {
        return Unexpect(Check);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    [[fallthrough]];

  case 0x04: {
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
      InitExprs.reserve(VecCnt);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    for (uint32_t i = 0; i < VecCnt; ++i) {
      InitExprs.emplace_back();
      if (auto Res = InitExprs.back().loadBinary(Mgr, PConf); !Res) {
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Res);
      }
    }
    break;
  }

  default:
    break;
  }

  return {};
}

/// Load binary of CodeSegment node. See "include/ast/segment.h".
Expect<void> CodeSegment::loadBinary(FileMgr &Mgr,
                                     const ProposalConfigure &PConf) {
  /// Read the code segment size.
  if (auto Res = Mgr.readU32()) {
    SegSize = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }

  /// Read the vector of local variable counts and types.
  uint32_t VecCnt = 0;
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    Locals.reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    uint32_t LocalCnt = 0;
    ValType LocalType = ValType::None;
    if (auto Res = Mgr.readU32()) {
      LocalCnt = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    if (auto Res = Mgr.readByte()) {
      LocalType = static_cast<ValType>(*Res);
      if (auto Check = checkValTypeProposals(PConf, LocalType,
                                             Mgr.getOffset() - 1, NodeAttr);
          !Check) {
        return Unexpect(Check);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    Locals.push_back(std::make_pair(LocalCnt, LocalType));
  }

  /// Read function body.
  if (auto Res = Segment::loadExpression(Mgr, PConf); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of DataSegment node. See "include/ast/segment.h".
Expect<void> DataSegment::loadBinary(FileMgr &Mgr,
                                     const ProposalConfigure &PConf) {
  Mode = DataMode::Passive;
  MemoryIdx = 0;

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
  uint8_t Check;
  if (auto Res = Mgr.readByte()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
  }
  /// Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !PConf.hasProposal(Proposal::BulkMemoryOperations) &&
      !PConf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::InvalidGrammar,
                           Proposal::BulkMemoryOperations, Mgr.getOffset() - 1,
                           NodeAttr);
  }

  switch (Check) {
  case 0x02: /// 0x02 memidx expr vec(byte) , Active
    /// Read target memory index.
    if (auto Res = Mgr.readU32()) {
      MemoryIdx = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    [[fallthrough]];

  case 0x00: /// 0x00 expr vec(byte) , Active
    /// Read the offset expression.
    if (auto Res = Segment::loadExpression(Mgr, PConf); !Res) {
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    Mode = DataMode::Active;
    [[fallthrough]];

  case 0x01: /// 0x01 vec(byte) , Passive
  {
    /// Read initialization data.
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    if (auto Res = Mgr.readBytes(VecCnt)) {
      Data = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(), NodeAttr);
    }
    break;
  }
  default:
    return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1, NodeAttr);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
