// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize table segment. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSegment(const AST::TableSegment &Seg,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Table segment: tabletype + expr.
  if (Seg.getExpr().getInstrs().size() > 0) {
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(ErrCode::Value::MalformedTable,
                             Proposal::FunctionReferences,
                             ASTNodeAttr::Seg_Table);
    }
    OutVec.push_back(0x40);
    OutVec.push_back(0x00);
  }
  if (auto Res = serializeType(Seg.getTableType(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
    return Unexpect(Res);
  }
  if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
    return Unexpect(Res);
  }
  return {};
}

// Serialize global segment. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSegment(const AST::GlobalSegment &Seg,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Global segment: globaltype + expr.
  if (auto Res = serializeType(Seg.getGlobalType(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }
  if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }
  return {};
}

// Serialize element segment. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSegment(const AST::ElementSegment &Seg,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Element segment: mode:u32 + tableidx:u32 + offset:expr + elemkind:reftype +
  // vec(u32) + vec(expr)
  if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes) &&
      (Seg.getMode() != AST::ElementSegment::ElemMode::Passive ||
       Seg.getIdx() != 0)) {
    return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations,
                           ASTNodeAttr::Seg_Element);
  }

  uint8_t Mode = 0x00;
  auto ModeIdx = OutVec.size();
  OutVec.push_back(Mode);
  switch (Seg.getMode()) {
  case AST::ElementSegment::ElemMode::Passive:
    Mode |= 0x01;
    break;
  case AST::ElementSegment::ElemMode::Declarative:
    Mode |= 0x03;
    break;
  default:
    break;
  }

  // Serialize idx.
  if (Seg.getIdx() != 0) {
    Mode |= 0x02;
    serializeU32(Seg.getIdx(), OutVec);
  }

  // Serialize OffExpr.
  if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
    if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return Unexpect(Res);
    }
  }

  // Distinguish between FuncIdx and Expr.
  if (Seg.getInitExprs().size() != 0) {
    auto IsInitExpr = false;
    for (auto &Expr : Seg.getInitExprs()) {
      if (Expr.getInstrs().size() != 2 ||
          Expr.getInstrs()[0].getOpCode() != OpCode::Ref__func ||
          Expr.getInstrs()[1].getOpCode() != OpCode::End) {
        IsInitExpr = true;
        break;
      }
    }
    if (IsInitExpr) {
      Mode |= 0x04;
    }
  }

  // Serialize ElemKind or RefType.
  if (Mode & 0x03) {
    if (Mode & 0x04) {
      // Serialize RefType.
      if (auto Res = serializeRefType(Seg.getRefType(),
                                      ASTNodeAttr::Seg_Element, OutVec);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    } else {
      // Serialize ElemKind.
      OutVec.push_back(0x00);
    }
  }

  // Serialize vec(FuncIdx) or vec(expr).
  serializeU32(static_cast<uint32_t>(Seg.getInitExprs().size()), OutVec);
  for (auto &Expr : Seg.getInitExprs()) {
    if (Mode & 0x04) {
      // Serialize vec(expr).
      if (auto Res = serializeExpression(Expr, OutVec); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return Unexpect(Res);
      }
    } else {
      // Serialize vec(FuncIdx).
      serializeU32(Expr.getInstrs()[0].getTargetIndex(), OutVec);
    }
  }

  OutVec[ModeIdx] = Mode;
  return {};
}

// Serialize code segment. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSegment(const AST::CodeSegment &Seg,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Code segment: size:u32 + locals:vec(u32 + valtype) + body:expr.
  auto OrgSize = OutVec.size();
  serializeU32(static_cast<uint32_t>(Seg.getLocals().size()), OutVec);
  for (auto &Locals : Seg.getLocals()) {
    serializeU32(Locals.first, OutVec);
    if (auto Res =
            serializeValType(Locals.second, ASTNodeAttr::Seg_Code, OutVec);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
  }
  if (auto Res = serializeExpression(Seg.getExpr(), OutVec); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Expression));
    return Unexpect(Res);
  }
  // Backward insert the section size.
  serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
               std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
  return {};
}

// Serialize data segment. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSegment(const AST::DataSegment &Seg,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Data segment: mode:u32 + memidx:u32 + expr + vec(byte)
  if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes) &&
      (Seg.getMode() != AST::DataSegment::DataMode::Active ||
       Seg.getIdx() != 0)) {
    return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations,
                           ASTNodeAttr::Seg_Data);
  }

  switch (Seg.getMode()) {
  case AST::DataSegment::DataMode::Active:
    if (Seg.getIdx() != 0) {
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
    OutVec.push_back(0x01);
    break;

  default:
    assumingUnreachable();
  }

  serializeU32(static_cast<uint32_t>(Seg.getData().size()), OutVec);
  OutVec.insert(OutVec.end(), Seg.getData().begin(), Seg.getData().end());
  return {};
}

} // namespace Loader
} // namespace WasmEdge
