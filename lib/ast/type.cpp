// SPDX-License-Identifier: Apache-2.0
#include "common/ast/type.h"
#include "common/types.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Load binary to construct Limit node. See "include/common/ast/type.h".
Expect<void> Limit::loadBinary(FileMgr &Mgr) {
  /// Read limit type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<LimitType>(*Res);
    switch (Type) {
    case LimitType::HasMin:
    case LimitType::HasMinMax:
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
  if (Type != LimitType::HasMin && Type != LimitType::HasMinMax) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }

  /// Read min and max number.
  if (auto Res = Mgr.readU32()) {
    Min = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  if (Type == LimitType::HasMinMax) {
    if (auto Res = Mgr.readU32()) {
      Max = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Load binary to construct FunctionType node. See "include/common/ast/type.h".
Expect<void> FunctionType::loadBinary(FileMgr &Mgr) {
  uint32_t VecCnt = 0;

  /// Read function type (0x60).
  if (auto Res = Mgr.readByte()) {
    if (static_cast<ElemType>(*Res) != ElemType::Func) {
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

  /// Read vector of parameter types.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      switch (Type) {
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
      ParamTypes.push_back(Type);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Read vector of result types.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      switch (Type) {
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
      ReturnTypes.push_back(Type);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Res);
    }
  }
  return {};
}

/// Load binary to construct MemoryType node. See "include/common/ast/type.h".
Expect<void> MemoryType::loadBinary(FileMgr &Mgr) {
  /// Read limit.
  Memory = std::make_unique<Limit>();
  return Memory->loadBinary(Mgr);
}

/// Load binary to construct TableType node. See "include/common/ast/type.h".
Expect<void> TableType::loadBinary(FileMgr &Mgr) {
  /// Read element type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<ElemType>(*Res);
    switch (Type) {
    case ElemType::FuncRef:
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
  if (Type != ElemType::FuncRef) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(ErrCode::InvalidGrammar);
  }

  /// Read limit.
  Table = std::make_unique<Limit>();
  return Table->loadBinary(Mgr);
}

/// Load binary to construct GlobalType node. See "include/common/ast/type.h".
Expect<void> GlobalType::loadBinary(FileMgr &Mgr) {
  /// Read value type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<ValType>(*Res);
    switch (Type) {
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

  /// Read mutability.
  if (auto Res = Mgr.readByte()) {
    Mut = static_cast<ValMut>(*Res);
    switch (Mut) {
    case ValMut::Const:
    case ValMut::Var:
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
  return {};
}

} // namespace AST
} // namespace SSVM
