#include "ast/segment.h"

namespace AST {

/// Load binary of Expression node in Segment. See "include/ast/segment.h".
Loader::ErrCode Segment::loadExpression(FileMgr &Mgr) {
  Expr = std::make_unique<Expression>();
  return Expr->loadBinary(Mgr);
}

/// Load binary of GlobalSegment node. See "include/ast/segment.h".
Loader::ErrCode GlobalSegment::loadBinary(FileMgr &Mgr) {
  /// Read global type node.
  Global = std::make_unique<GlobalType>();
  Loader::ErrCode Status = Global->loadBinary(Mgr);
  if (Status != Loader::ErrCode::Success)
    return Status;

  /// Read the expression.
  return Segment::loadExpression(Mgr);
}

/// Load binary of ElementSegment node. See "include/ast/segment.h".
Loader::ErrCode ElementSegment::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the table index.
  if ((Status = Mgr.readU32(TableIdx)) != Loader::ErrCode::Success)
    return Status;

  /// Read the expression.
  if ((Status = Segment::loadExpression(Mgr)) != Loader::ErrCode::Success)
    return Status;

  /// Read the function indices.
  unsigned int VecCnt = 0;
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int Idx = 0;
    if ((Status = Mgr.readU32(Idx)) != Loader::ErrCode::Success)
      return Status;
    FuncIdxes.push_back(Idx);
  }
  return Status;
}

/// Load binary of CodeSegment node. See "include/ast/segment.h".
Loader::ErrCode CodeSegment::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read the code segment size.
  if ((Status = Mgr.readU32(SegSize)) != Loader::ErrCode::Success)
    return Status;

  /// Read the vector of local variable counts and types.
  unsigned int VecCnt = 0;
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int LocalCnt = 0;
    unsigned char LocalType = 0x40;
    if ((Status = Mgr.readU32(LocalCnt)) != Loader::ErrCode::Success)
      return Status;
    if ((Status = Mgr.readByte(LocalType)) != Loader::ErrCode::Success)
      return Status;
    Locals.push_back(std::make_pair(LocalCnt, static_cast<ValType>(LocalType)));
  }

  /// Read function body.
  return Segment::loadExpression(Mgr);
}

/// Load binary of DataSegment node. See "include/ast/segment.h".
Loader::ErrCode DataSegment::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read target memory index.
  if ((Status = Mgr.readU32(MemoryIdx)) != Loader::ErrCode::Success)
    return Status;

  /// Read the offset expression.
  if ((Status = Segment::loadExpression(Mgr)) != Loader::ErrCode::Success)
    return Status;

  /// Read initialization data.
  unsigned int VecCnt = 0;
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  return Mgr.readBytes(Data, VecCnt);
}

} // namespace AST