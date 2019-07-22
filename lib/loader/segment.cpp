#include "loader/segment.h"

namespace AST {

/// Load binary of Expression node in Segment. See "include/loader/segment.h".
Base::ErrCode Segment::loadExpression(FileMgr &Mgr) {
  Expr = std::make_unique<Expression>();
  return Expr->loadBinary(Mgr);
}

/// Load binary of GlobalSegment node. See "include/loader/segment.h".
Base::ErrCode GlobalSegment::loadBinary(FileMgr &Mgr) {
  /// Read global type node.
  Global = std::make_unique<GlobalType>();
  Base::ErrCode Status = Global->loadBinary(Mgr);
  if (Status != Base::ErrCode::Success)
    return Status;

  /// Read the expression.
  return Segment::loadExpression(Mgr);
}

/// Load binary of ElementSegment node. See "include/loader/segment.h".
Base::ErrCode ElementSegment::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the table index.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(TableIdx))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the expression.
  if ((Status = Segment::loadExpression(Mgr)) != Base::ErrCode::Success)
    return Status;

  /// Read the function indices.
  unsigned int VecCnt = 0;
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int Idx = 0;
    if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(Idx))) !=
        Base::ErrCode::Success)
      return Status;
    FuncIdxes.push_back(Idx);
  }
  return Status;
}

/// Load binary of CodeSegment node. See "include/loader/segment.h".
Base::ErrCode CodeSegment::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read the code segment size.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(SegSize))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the vector of local variable counts and types.
  unsigned int VecCnt = 0;
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int LocalCnt = 0;
    unsigned char LocalType = 0x40;
    if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(LocalCnt))) !=
        Base::ErrCode::Success)
      return Status;
    if ((Status = static_cast<Base::ErrCode>(Mgr.readByte(LocalType))) !=
        Base::ErrCode::Success)
      return Status;
    Locals.push_back(
        std::make_pair(LocalCnt, static_cast<Base::ValType>(LocalType)));
  }

  /// Read function body.
  return Segment::loadExpression(Mgr);
}

/// Load binary of DataSegment node. See "include/loader/segment.h".
Base::ErrCode DataSegment::loadBinary(FileMgr &Mgr) {
  Base::ErrCode Status = Base::ErrCode::Success;

  /// Read target memory index.
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(MemoryIdx))) !=
      Base::ErrCode::Success)
    return Status;

  /// Read the offset expression.
  if ((Status = Segment::loadExpression(Mgr)) != Base::ErrCode::Success)
    return Status;

  /// Read initialization data.
  unsigned int VecCnt = 0;
  if ((Status = static_cast<Base::ErrCode>(Mgr.readU32(VecCnt))) !=
      Base::ErrCode::Success)
    return Status;
  return static_cast<Base::ErrCode>(Mgr.readBytes(Data, VecCnt));
}

} // namespace AST