// SPDX-License-Identifier: Apache-2.0
#include "common/ast/segment.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Load expression binary in segment. See "include/common/ast/segment.h".
Expect<void> Segment::loadExpression(FileMgr &Mgr) {
  Expr = std::make_unique<Expression>();
  return Expr->loadBinary(Mgr);
}

/// Load binary of GlobalSegment node. See "include/common/ast/segment.h".
Expect<void> GlobalSegment::loadBinary(FileMgr &Mgr) {
  /// Read global type node.
  Global = std::make_unique<GlobalType>();
  if (auto Res = Global->loadBinary(Mgr); !Res) {
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

/// Load binary of ElementSegment node. See "include/common/ast/segment.h".
Expect<void> ElementSegment::loadBinary(FileMgr &Mgr) {
  /// Read the table index.
  if (auto Res = Mgr.readU32()) {
    TableIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the expression.
  if (auto Res = Segment::loadExpression(Mgr); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the function indices.
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
    if (auto Res = Mgr.readU32()) {
      FuncIdxes.push_back(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
  }

  return {};
}

/// Load binary of CodeSegment node. See "include/common/ast/segment.h".
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

/// Load binary of DataSegment node. See "include/common/ast/segment.h".
Expect<void> DataSegment::loadBinary(FileMgr &Mgr) {
  /// Read target memory index.
  if (auto Res = Mgr.readU32()) {
    MemoryIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

  /// Read the offset expression.
  if (auto Res = Segment::loadExpression(Mgr); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }

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

  return {};
}

} // namespace AST
} // namespace SSVM
