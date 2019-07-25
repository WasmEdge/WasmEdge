#include "ast/type.h"

namespace AST {

/// Load binary to construct Limit node. See "include/loader/type.h".
Loader::ErrCode Limit::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read limit type.
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  Type = static_cast<LimitType>(Byte);
  if (Type != LimitType::HasMin && Type != LimitType::HasMinMax)
    return Loader::ErrCode::InvalidGrammar;

  /// Read min and max number.
  if ((Status = Mgr.readU32(Min)) != Loader::ErrCode::Success)
    return Status;
  if (Type == LimitType::HasMinMax) {
    if ((Status = Mgr.readU32(Max)) != Loader::ErrCode::Success)
      return Status;
  }
  return Status;
}

/// Load binary to construct FunctionType node. See "include/loader/type.h".
Loader::ErrCode FunctionType::loadBinary(FileMgr &Mgr) {
  unsigned char Byte = 0;
  unsigned int VecCnt = 0;
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read function type (0x60).
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  if (static_cast<ElemType>(Byte) != Base::ElemType::Func)
    return Loader::ErrCode::InvalidGrammar;

  /// Read vector of parameter types.
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
      return Status;
    ParamTypes.push_back(static_cast<ValType>(Byte));
  }

  /// Read vector of result types.
  if ((Status = Mgr.readU32(VecCnt)) != Loader::ErrCode::Success)
    return Status;
  for (int i = 0; i < VecCnt; i++) {
    if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
      return Status;
    ReturnTypes.push_back(static_cast<ValType>(Byte));
  }
  return Status;
}

/// Load binary to construct MemoryType node. See "include/loader/type.h".
Loader::ErrCode MemoryType::loadBinary(FileMgr &Mgr) {
  /// Read limit.
  Memory = std::make_unique<Limit>();
  return Memory->loadBinary(Mgr);
}

/// Load binary to construct TableType node. See "include/loader/type.h".
Loader::ErrCode TableType::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read element type.
  unsigned char Byte = 0;
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  Type = static_cast<ElemType>(Byte);
  if (Type != ElemType::FuncRef)
    return Loader::ErrCode::InvalidGrammar;

  /// Read limit.
  Table = std::make_unique<Limit>();
  return Table->loadBinary(Mgr);
}

/// Load binary to construct GlobalType node. See "include/loader/type.h".
Loader::ErrCode GlobalType::loadBinary(FileMgr &Mgr) {
  Loader::ErrCode Status = Loader::ErrCode::Success;

  /// Read value type.
  unsigned char Byte = 0;
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  Type = static_cast<ValType>(Byte);

  /// Read mutability.
  if ((Status = Mgr.readByte(Byte)) != Loader::ErrCode::Success)
    return Status;
  Mut = static_cast<ValMut>(Byte);
  if (Mut != ValMut::Const && Mut != ValMut::Var)
    return Loader::ErrCode::InvalidGrammar;
  return Status;
}

} // namespace AST