// SPDX-License-Identifier: Apache-2.0

#include "ast/type.h"

namespace WasmEdge {
namespace AST {

namespace {
Expect<void> loadLimit(FileMgr &Mgr, Limit &Lim) {
  /// Read limit type.
  if (auto Res = Mgr.readByte()) {
    Lim.Type = static_cast<Limit::LimitType>(*Res);
    switch (Lim.Type) {
    case Limit::LimitType::HasMin:
    case Limit::LimitType::HasMinMax:
      break;
    default:
      if (*Res == 0x80 || *Res == 0x81) {
        /// LEB128 cases will fail.
        spdlog::error(ErrCode::IntegerTooLong);
        spdlog::error(ErrInfo::InfoLoading(Mgr.getLastOffset()));
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
        return Unexpect(ErrCode::IntegerTooLong);
      } else {
        spdlog::error(ErrCode::IntegerTooLarge);
        spdlog::error(ErrInfo::InfoLoading(Mgr.getLastOffset()));
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
        return Unexpect(ErrCode::IntegerTooLarge);
      }
    }
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoLoading(Mgr.getLastOffset()));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
    return Unexpect(Res);
  }

  /// Read min and max number.
  if (auto Res = Mgr.readU32()) {
    Lim.Min = *Res;
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoLoading(Mgr.getLastOffset()));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
    return Unexpect(Res);
  }
  if (Lim.Type == Limit::LimitType::HasMinMax) {
    if (auto Res = Mgr.readU32()) {
      Lim.Max = *Res;
    } else {
      spdlog::error(Res.error());
      spdlog::error(ErrInfo::InfoLoading(Mgr.getLastOffset()));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Type_Limit));
      return Unexpect(Res);
    }
  }
  return {};
}
} // namespace

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
    Inner.Params.reserve(VecCnt);
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
      Inner.Params.push_back(Type);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }

  /// Read vector of result types.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
    Inner.Returns.reserve(VecCnt);
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
      Inner.Returns.push_back(Type);
    } else {
      return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
    }
  }
  return {};
}

/// Load binary to construct MemoryType node. See "include/ast/type.h".
Expect<void> MemoryType::loadBinary(FileMgr &Mgr, const Configure &) {
  /// Read limit.
  if (auto Res = loadLimit(Mgr, Inner.Lim); !Res) {
    spdlog::error(ErrInfo::InfoAST(NodeAttr));
    return Unexpect(Res);
  }
  return {};
}

/// Load binary to construct TableType node. See "include/ast/type.h".
Expect<void> TableType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read reference type.
  if (auto Res = Mgr.readByte()) {
    Inner.Type = static_cast<RefType>(*Res);
    if (auto Check = checkRefTypeProposals(Conf, Inner.Type,
                                           Mgr.getLastOffset(), NodeAttr);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read limit.
  if (auto Res = loadLimit(Mgr, Inner.Lim); !Res) {
    spdlog::error(ErrInfo::InfoAST(NodeAttr));
    return Unexpect(Res);
  }
  return {};
}

/// Load binary to construct GlobalType node. See "include/ast/type.h".
Expect<void> GlobalType::loadBinary(FileMgr &Mgr, const Configure &Conf) {
  /// Read value type.
  if (auto Res = Mgr.readByte()) {
    Inner.Type = static_cast<ValType>(*Res);
    if (auto Check = checkValTypeProposals(Conf, Inner.Type,
                                           Mgr.getLastOffset(), NodeAttr);
        !Check) {
      return Unexpect(Check);
    }
  } else {
    return logLoadError(Res.error(), Mgr.getLastOffset(), NodeAttr);
  }

  /// Read mutability.
  if (auto Res = Mgr.readByte()) {
    Inner.Mut = static_cast<ValMut>(*Res);
    switch (Inner.Mut) {
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
