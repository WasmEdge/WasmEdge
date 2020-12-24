// SPDX-License-Identifier: Apache-2.0
#include "ast/type.h"
#include "common/log.h"
#include "common/types.h"

namespace SSVM {
namespace AST {

namespace {

Expect<ValType> checkValTypeProposals(ValType VType,
                                      const ProposalConfigure &PConf) {
  if (VType == ValType::V128 && !PConf.hasProposal(Proposal::SIMD)) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoProposal(Proposal::SIMD);
    return Unexpect(ErrCode::InvalidGrammar);
  }
  if ((VType == ValType::FuncRef &&
       !PConf.hasProposal(Proposal::ReferenceTypes) &&
       !PConf.hasProposal(Proposal::BulkMemoryOperations)) ||
      (VType == ValType::ExternRef &&
       !PConf.hasProposal(Proposal::ReferenceTypes))) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
    return Unexpect(ErrCode::InvalidGrammar);
  }
  switch (VType) {
  case ValType::None:
  case ValType::I32:
  case ValType::I64:
  case ValType::F32:
  case ValType::F64:
  case ValType::V128:
  case ValType::ExternRef:
  case ValType::FuncRef:
    return VType;
  default:
    LOG(ERROR) << ErrCode::InvalidGrammar;
    return Unexpect(ErrCode::InvalidGrammar);
  }
}

} // namespace

/// Load binary to construct Limit node. See "include/ast/type.h".
Expect<void> Limit::loadBinary(FileMgr &Mgr, const ProposalConfigure &PConf) {
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

/// Load binary to construct FunctionType node. See "include/ast/type.h".
Expect<void> FunctionType::loadBinary(FileMgr &Mgr,
                                      const ProposalConfigure &PConf) {
  uint32_t VecCnt = 0;

  /// Read function type (0x60).
  if (auto Res = Mgr.readByte()) {
    if (*Res != 0x60U) {
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
    ParamTypes.reserve(VecCnt);
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check = checkValTypeProposals(Type, PConf); !Check) {
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Check);
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
    ReturnTypes.reserve(VecCnt);
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readByte()) {
      ValType Type = static_cast<ValType>(*Res);
      if (auto Check = checkValTypeProposals(Type, PConf); !Check) {
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
        return Unexpect(Check);
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

/// Load binary to construct MemoryType node. See "include/ast/type.h".
Expect<void> MemoryType::loadBinary(FileMgr &Mgr,
                                    const ProposalConfigure &PConf) {
  /// Read limit.
  return MemoryLim.loadBinary(Mgr, PConf);
}

/// Load binary to construct TableType node. See "include/ast/type.h".
Expect<void> TableType::loadBinary(FileMgr &Mgr,
                                   const ProposalConfigure &PConf) {
  /// Read reference type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<RefType>(*Res);
    switch (Type) {
    case RefType::ExternRef:
      if (!PConf.hasProposal(Proposal::ReferenceTypes)) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoProposal(Proposal::ReferenceTypes);
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
      [[fallthrough]];
    case RefType::FuncRef:
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

  /// Read limit.
  return TableLim.loadBinary(Mgr, PConf);
}

/// Load binary to construct GlobalType node. See "include/ast/type.h".
Expect<void> GlobalType::loadBinary(FileMgr &Mgr,
                                    const ProposalConfigure &PConf) {
  /// Read value type.
  if (auto Res = Mgr.readByte()) {
    Type = static_cast<ValType>(*Res);
    if (auto Check = checkValTypeProposals(Type, PConf); !Check) {
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
      LOG(ERROR) << ErrInfo::InfoAST(NodeAttr);
      return Unexpect(Check);
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
