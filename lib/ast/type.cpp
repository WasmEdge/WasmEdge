// SPDX-License-Identifier: Apache-2.0
#include "ast/type.h"
#include "common/log.h"
#include "common/types.h"

namespace WasmEdge {
namespace AST {

/// Load binary to construct Limit node. See "include/ast/type.h".
Expect<void> Limit::loadBinary(FileMgr &Mgr, const Configure &) {
  /// Read limit type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<LimitType>(*Res);
    switch (Type) {
    case LimitType::HasMin:
    case LimitType::HasMinMax:
      break;
    default:
      if (*Res == 0x80 || *Res == 0x81) {
        /// LEB128 cases will fail.
        return logLoadError(ErrCode::IntegerTooLong, Mgr.getLastOffset(),
                            NodeAttr);
      } else {
        return logLoadError(ErrCode::IntegerTooLarge, Mgr.getLastOffset(),
                            NodeAttr);
      }
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read min and max number.
  if (auto Res = Mgr.readU32()) {
    Min = *Res;
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  if (Type == LimitType::HasMinMax) {
    if (auto Res = Mgr.readU32()) {
      Max = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }
  return {};
}

/// Load binary to construct FunctionType node. See "include/ast/type.h".
Expect<void> FunctionType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  uint32_t VecCnt = 0;

  /// Read function type (0x60).
  if (auto Res = Mgr.readByte()) {
    if (*Res != 0x60U) {
      return logLoadError(ErrCode::IntegerTooLong, Mgr.getLastOffset(),
                          NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read vector of parameter types.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    ParamTypes.reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check =
              checkValTypeProposals(Conf, Type, Mgr.getLastOffset(), NodeAttr);
          !Check) {
        return Unexpect(Check);
      }
      ParamTypes.push_back(Type);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }

  /// Read vector of result types.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    ReturnTypes.reserve(VecCnt);
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  if (unlikely(!Conf.hasProposal(Proposal::MultiValue)) && VecCnt > 1) {
    return logNeedProposal(ErrCode::MalformedValType, Proposal::MultiValue,
                           Mgr.getLastOffset(), NodeAttr);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check =
              checkValTypeProposals(Conf, Type, Mgr.getLastOffset(), NodeAttr);
          !Check) {
        return Unexpect(Check);
      }
      ReturnTypes.push_back(Type);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }
  return {};
}

/// Load binary to construct MemoryType node. See "include/ast/type.h".
Expect<void> MemoryType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read limit.
  return MemoryLim.loadBinary(Mgr, Conf);
}

/// Load binary to construct TableType node. See "include/ast/type.h".
Expect<void> TableType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read reference type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<RefType>(*Res);
    if (auto Check =
            checkRefTypeProposals(Conf, Type, Mgr.getLastOffset(), NodeAttr);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read limit.
  return TableLim.loadBinary(Mgr, Conf);
}

/// Load binary to construct GlobalType node. See "include/ast/type.h".
Expect<void> GlobalType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read value type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<ValType>(*Res);
    if (auto Check =
            checkValTypeProposals(Conf, Type, Mgr.getLastOffset(), NodeAttr);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read mutability.
  if (auto Res = Mgr.readByte()) {
    Mut = static_cast<ValMut>(*Res);
    switch (Mut) {
    case ValMut::Const:
    case ValMut::Var:
      break;
    default:
      return logLoadError(ErrCode::InvalidMut, Mgr.getLastOffset(), NodeAttr);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }
  return {};
}

} // namespace AST
} // namespace WasmEdge
