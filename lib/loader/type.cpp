#include "loader/type.h"

namespace AST {

/// Load binary to construct Limit node. See "include/loader/type.h".
bool Limit::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  /// Read limit type.
  if (!Mgr.readByte(Byte))
    return false;
  Type = static_cast<LimitType>(Byte);
  if (Type != LimitType::HasMin && Type != LimitType::HasMinMax)
    return false;

  /// Read min and max number.
  if (!Mgr.readU32(Min))
    return false;
  if (Type == LimitType::HasMinMax) {
    if (!Mgr.readU32(Max))
      return false;
  }
  return true;
}

/// Load binary to construct FunctionType node. See "include/loader/type.h".
bool FunctionType::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  unsigned int VecCnt = 0;
  /// Read function type (0x60).
  if (!Mgr.readByte(Byte) ||
      static_cast<ElemType>(Byte) != Base::ElemType::Func)
    return false;

  /// Read vector of parameter types.
  if (!Mgr.readU32(VecCnt))
    return false;
  for (int i = 0; i < VecCnt; i++) {
    Mgr.readByte(Byte);
    ParamTypes.push_back(static_cast<ValType>(Byte));
  }

  /// Read vector of result types.
  if (!Mgr.readU32(VecCnt))
    return false;
  for (int i = 0; i < VecCnt; i++) {
    Mgr.readByte(Byte);
    ReturnTypes.push_back(static_cast<ValType>(Byte));
  }
  return true;
}

/// Load binary to construct MemoryType node. See "include/loader/type.h".
bool MemoryType::loadBinary(FileMgr &Mgr) {
  /// Read limit.
  Memory = std::make_unique<Limit>();
  return Memory->loadBinary(Mgr);
}

/// Load binary to construct TableType node. See "include/loader/type.h".
bool TableType::loadBinary(FileMgr &Mgr) {
  /// Read element type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  Type = static_cast<ElemType>(Byte);
  if (Type != ElemType::FuncRef)
    return false;

  /// Read limit.
  Table = std::make_unique<Limit>();
  return Table->loadBinary(Mgr);
}

/// Load binary to construct GlobalType node. See "include/loader/type.h".
bool GlobalType::loadBinary(FileMgr &Mgr) {
  /// Read value type.
  unsigned char Byte = 0;
  if (!Mgr.readByte(Byte))
    return false;
  Type = static_cast<ValType>(Byte);

  /// Read mutability.
  if (!Mgr.readByte(Byte))
    return false;
  Mut = static_cast<ValMut>(Byte);
  if (Mut != ValMut::Const && Mut != ValMut::Var)
    return false;
  return true;
}

} // namespace AST