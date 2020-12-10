// SPDX-License-Identifier: Apache-2.0
#include "ast/segment.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Load expression binary in segment. See "include/ast/segment.h".
Expect<void> Segment::loadExpression(FileMgr &Mgr) {
  return Expr.loadBinary(Mgr);
}

/// Load binary of GlobalSegment node. See "include/ast/segment.h".
Expect<void> GlobalSegment::loadBinary(FileMgr &Mgr) {
  /// Read global type node.
  if (auto Res = Global.loadBinary(Mgr); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the expression.
  if (auto Res = Segment::loadExpression(Mgr); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of ElementSegment node. See "include/ast/segment.h".
Expect<void> ElementSegment::loadBinary(FileMgr &Mgr) {
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
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
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
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }

  /// Read the table index.
  TableIdx = 0;
  switch (Check) {
  case 0x02:
  case 0x06:
    if (auto Res = Mgr.readU32()) {
      TableIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
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
    if (auto Res = Segment::loadExpression(Mgr); !Res) {
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
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    [[fallthrough]];

  case 0x00: {
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    for (uint32_t i = 0; i < VecCnt; ++i) {
      /// For each element in vec(funcidx), make expr(ref.func idx end).
      InitExprs.emplace_back();
      Instruction RefFunc(OpCode::Ref__func);
      Instruction End(OpCode::End);
      if (auto Res = RefFunc.loadBinary(Mgr); !Res) {
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
      switch (Type) {
      case RefType::FuncRef:
      case RefType::ExternRef:
        break;
      default:
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    [[fallthrough]];

  case 0x04: {
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
      InitExprs.reserve(VecCnt);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
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
Expect<void> CodeSegment::loadBinary(FileMgr &Mgr) {
  /// Read the code segment size.
  if (auto Res = Mgr.readU32()) {
    SegSize = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the vector of local variable counts and types.
  uint32_t VecCnt = 0;
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    Locals.reserve(VecCnt);
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    uint32_t LocalCnt = 0;
    ValType LocalType = ValType::None;
    if (auto Res = Mgr.readU32()) {
      LocalCnt = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    if (auto Res = Mgr.readByte()) {
      LocalType = static_cast<ValType>(*Res);
      switch (LocalType) {
      case ValType::I32:
      case ValType::I64:
      case ValType::F32:
      case ValType::F64:
      case ValType::V128:
      case ValType::ExternRef:
      case ValType::FuncRef:
        break;
      default:
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    Locals.push_back(std::make_pair(LocalCnt, LocalType));
  }

  /// Read function body.
  if (auto Res = Segment::loadExpression(Mgr); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  return {};
}

/// Load binary of DataSegment node. See "include/ast/segment.h".
Expect<void> DataSegment::loadBinary(FileMgr &Mgr) {
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
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  switch (Check) {
  case 0x02: /// 0x02 memidx expr vec(byte) , Active
    /// Read target memory index.
    if (auto Res = Mgr.readU32()) {
      MemoryIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    [[fallthrough]];

  case 0x00: /// 0x00 expr vec(byte) , Active
    /// Read the offset expression.
    if (auto Res = Segment::loadExpression(Mgr); !Res) {
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
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    if (auto Res = Mgr.readBytes(VecCnt)) {
      Data = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
    break;
  }
  default:
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }
  return {};
}

} // namespace AST
} // namespace SSVM
