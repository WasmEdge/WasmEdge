#include "loader/segment.h"

namespace AST {

/// Load binary of Expression node in Segment. See "include/loader/segment.h".
bool Segment::loadExpression(FileMgr &Mgr) {
  Expr = std::make_unique<Expression>();
  return Expr->loadBinary(Mgr);
}

/// Load binary of ElementSegment node. See "include/loader/segment.h".
bool ElementSegment::loadBinary(FileMgr &Mgr) {
  /// Read the table index.
  if (!Mgr.readU32(TableIdx))
    return false;

  /// Read the expression.
  if (!Segment::loadExpression(Mgr))
    return false;

  /// Read the function indices.
  unsigned int VecCnt = 0;
  if (!Mgr.readU32(VecCnt))
    return false;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int Idx = 0;
    if (!Mgr.readU32(Idx))
      return false;
    FuncIdxes.push_back(Idx);
  }
  return true;
}

/// Load binary of CodeSegment node. See "include/loader/segment.h".
bool CodeSegment::loadBinary(FileMgr &Mgr) {
  /// Read the code segment size.
  if (!Mgr.readU32(SegSize))
    return false;

  /// Read the vector of local variable counts and types.
  unsigned int VecCnt = 0;
  if (!Mgr.readU32(VecCnt))
    return false;
  for (int i = 0; i < VecCnt; i++) {
    unsigned int LocalCnt = 0;
    unsigned char LocalType = 0x40;
    if (!Mgr.readU32(LocalCnt))
      return false;
    if (!Mgr.readByte(LocalType))
      return false;
    Locals.push_back(
        std::make_pair(LocalCnt, static_cast<Base::ValType>(LocalType)));
  }

  /// Read function body.
  return Segment::loadExpression(Mgr);
}

/// Load binary of DataSegment node. See "include/loader/segment.h".
bool DataSegment::loadBinary(FileMgr &Mgr) {
  /// Read target memory index.
  if (!Mgr.readU32(MemoryIdx))
    return false;

  /// Read the offset expression.
  if (!Segment::loadExpression(Mgr))
    return false;

  /// Read initialization data.
  unsigned int VecCnt = 0;
  if (!Mgr.readU32(VecCnt))
    return false;
  return Mgr.readBytes(Data, VecCnt);
}

} // namespace AST