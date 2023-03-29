#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize global segment. See "include/loader/serialize.h".
Expect<void> Serializer::serializeSegment(const AST::GlobalSegment &Seg,
                                          std::vector<uint8_t> &OutVec) {
  // Global segment: globaltype + expr.
  if (auto Res = serializeType(Seg.getGlobalType(), OutVec); unlikely(!Res)) {
    return Unexpect(Res);
  }
  if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  return {};
}

// Serialize element segment. See "include/loader/serialize.h".
Expect<void> Serializer::serializeSegment(const AST::ElementSegment &Seg,
                                          std::vector<uint8_t> &OutVec) {
  // Element segment: mode:u32 + tableidx:u32 + offset:expr + elemkind:reftype +
  // vec(u32) + vec(expr)
  std::vector<uint8_t> Result;
  uint8_t Mode = 0x00;
  switch (Seg.getMode()) {
  case AST::ElementSegment::ElemMode::Declarative:
    Mode |= 0x02;
    [[fallthrough]];

  case AST::ElementSegment::ElemMode::Passive:
    Mode |= 0x01;
    break;

  default:
    break;
  }

  // Serialize idx.
  if (Seg.getIdx() != 0) {
    serializeU32(Seg.getIdx(), Result);
    if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
      Mode |= 0x02;
    } else {
      return logSerializeError(ErrCode::Value::Unreachable,
                               ASTNodeAttr::Seg_Element);
    }
  }

  // Serialize OffExpr.
  if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
    if (auto Res = serializeExpression(Seg.getExpr(), Result); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
      return Unexpect(Res);
    }
  }

  if (Seg.getRefType() == RefType::FuncRef) {
    if (Mode != 0x00) {
      // Serialize ElemKind.
      Result.push_back(0x00);
    }
  } else {
    // Serialize RefType.
    if (auto Res = Conf.checkRefTypeProposals(Seg.getRefType(),
                                              ASTNodeAttr::Seg_Element);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    Result.push_back(static_cast<uint8_t>(Seg.getRefType()));
  }

  // Distinguish between FuncIdx and Expr.
  if (Seg.getInitExprs().size() != 0) {
    auto IsExpr = false;
    for (auto Expr : Seg.getInitExprs()) {
      if (Expr.getInstrs().size() != 2 ||
          Expr.getInstrs().at(0).getOpCode() != OpCode::Ref__func) {
        IsExpr = true;
        break;
      }
    }
    if (IsExpr) {
      Mode |= 0x04;
    }
  }

  //  Mode > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Mode > 0 && !Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    return Conf.logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                                Proposal::BulkMemoryOperations,
                                ASTNodeAttr::Seg_Element);
  }

  serializeU32(static_cast<uint32_t>(Seg.getInitExprs().size()), Result);
  for (auto Expr : Seg.getInitExprs()) {
    if (Mode & 0x04) {
      // Serialize vec(expr).
      if (auto Res = serializeExpression(Expr, Result); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return Unexpect(Res);
      }
    } else {
      // Serialize vec(FuncIdx).
      for (auto Instr : Expr.getInstrs()) {
        if (Instr.getOpCode() == OpCode::End) {
          break;
        }
        serializeU32(Instr.getTargetIndex(), Result);
      }
    }
  }

  Result.insert(Result.begin(), Mode);
  OutVec.insert(OutVec.end(), Result.begin(), Result.end());
  return {};
}

// Serialize code segment. See "include/loader/serialize.h".
Expect<void> Serializer::serializeSegment(const AST::CodeSegment &Seg,
                                          std::vector<uint8_t> &OutVec) {
  // Code segment: size:u32 + locals:vec(u32 + valtype) + body:expr.
  std::vector<uint8_t> Result;
  serializeU32(static_cast<uint32_t>(Seg.getLocals().size()), Result);

  uint32_t TotalLocalCnt = 0;
  for (auto Locals : Seg.getLocals()) {
    uint32_t LocalCnt = Locals.first;
    // Total local variables should not more than 2^32. Capped at 2^26.
    if (UINT32_C(67108864) - TotalLocalCnt < LocalCnt) {
      return logSerializeError(ErrCode::Value::TooManyLocals,
                               ASTNodeAttr::Seg_Code);
    }
    TotalLocalCnt += LocalCnt;
    if (auto Res =
            Conf.checkValTypeProposals(Locals.second, ASTNodeAttr::Seg_Code);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    serializeU32(Locals.first, Result);
    Result.push_back(static_cast<uint8_t>(Locals.second));
  }
  if (auto Res = serializeExpression(Seg.getExpr(), Result); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  serializeU32(static_cast<uint32_t>(Result.size()), OutVec);
  OutVec.insert(OutVec.end(), Result.begin(), Result.end());
  return {};
}

// Serialize data segment. See "include/loader/serialize.h".
Expect<void> Serializer::serializeSegment(const AST::DataSegment &Seg,
                                          std::vector<uint8_t> &OutVec) {
  // Data segment: mode:u32 + memidx:u32 + expr + vec(byte)
  switch (Seg.getMode()) {
  case AST::DataSegment::DataMode::Active:
    if (Seg.getIdx() != 0) {
      if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
          !Conf.hasProposal(Proposal::ReferenceTypes)) {
        return Conf.logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                                    Proposal::BulkMemoryOperations,
                                    ASTNodeAttr::Seg_Data);
      }
      OutVec.push_back(0x02);
      serializeU32(Seg.getIdx(), OutVec);
    } else {
      OutVec.push_back(0x00);
    }
    if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return Unexpect(Res);
    }
    break;

  case AST::DataSegment::DataMode::Passive:
    if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
        !Conf.hasProposal(Proposal::ReferenceTypes)) {
      return Conf.logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                                  Proposal::BulkMemoryOperations,
                                  ASTNodeAttr::Seg_Data);
    }
    OutVec.push_back(0x01);
    break;

  default:
    break;
  }

  serializeU32(static_cast<uint32_t>(Seg.getData().size()), OutVec);
  OutVec.insert(OutVec.end(), Seg.getData().begin(), Seg.getData().end());
  return {};
}

} // namespace Loader
} // namespace WasmEdge
