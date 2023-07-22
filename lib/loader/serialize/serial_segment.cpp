#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize global segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::GlobalSegment &Seg,
                                  std::vector<uint8_t> &OutVec) {
  // Global segment: globaltype + expr.
  serializeType(Seg.getGlobalType(), OutVec);
  serializeExpression(Seg.getExpr(), OutVec);
}

// Serialize element segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::ElementSegment &Seg,
                                  std::vector<uint8_t> &OutVec) {
  // Element segment: u32 + tableidx:u32 + offset:expr + elemkind:reftype + vec(u32) + vec(expr)
  // TODO: check proposals
  std::vector<uint8_t> Result;
  uint8_t Mode = 0x00;
  switch (Seg.getMode()) {
  case AST::ElementSegment::ElemMode::Declarative:
    Mode |= 0x02;
    [[fallthrough]];

  case AST::ElementSegment::ElemMode::Passive:
    Mode |= 0x01;

  default:
    break;
  }

  // Serialize idx.
  if (Seg.getIdx() != 0) {
    serializeU32(Seg.getIdx(), Result);
    if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
      Mode |= 0x02;
    } else {
      // TODO: return error
      return;
    }
  }

  // Serialize OffExpr.
  if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
    serializeExpression(Seg.getExpr(), Result);
  }

  if (Seg.getRefType() == RefType::FuncRef) {
    if (Mode != 0x00) {
      // Serialize ElemKind.
      serializeU32(0x00, Result);
    }
  } else {
    // Serialize RefType.
    serializeU32(static_cast<uint8_t>(Seg.getRefType()), Result);
  }

  // Distinguish between FuncIdx and Expr.
  if (Seg.getInitExprs().size() != 0) {
    auto IsExpr = false;
    for (auto Expr : Seg.getInitExprs()) {
      if (Expr.getInstrs().size() != 2 || Expr.getInstrs().at(0).getOpCode() != OpCode::Ref__func) {
        IsExpr = true;
        break;
      }
    }
    if (IsExpr) {
      Mode |= 0x04;
    }
  }

  serializeU32(Seg.getInitExprs().size(), Result);
  for (auto Expr : Seg.getInitExprs()) {
    if (Mode & 0x04) {
      // Serialize vec(expr).
      serializeExpression(Expr, Result);
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

  serializeU32(Mode, Result, Result.begin());
  OutVec.insert(OutVec.end(), Result.begin(), Result.end());
}

// Serialize code segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::CodeSegment &Seg,
                                  std::vector<uint8_t> &OutVec) {
  // Code segment: size:u32 + locals:vec(u32 + valtype) + body:expr.
  // TODO
  serializeU32(Seg.getSegSize(), OutVec);
  serializeU32(Seg.getLocals().size(), OutVec);
  for (auto Local : Seg.getLocals()) {
    serializeU32(Local.first, OutVec);
    serializeU32(static_cast<uint8_t>(Local.second), OutVec);
  }
  serializeExpression(Seg.getExpr(), OutVec);
}

// Serialize data segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::DataSegment &Seg,
                                  std::vector<uint8_t> &OutVec) {
  // Data segment: mode:u32 + memidx:u32 + expr + vec(byte)
  // TODO: check proposals
  switch (Seg.getMode()) {
  case AST::DataSegment::DataMode::Active:
    if (Seg.getIdx() != 0) {
      serializeU32(0x02, OutVec);
      serializeU32(Seg.getIdx(), OutVec);
    } else {
      serializeU32(0x00, OutVec);
    }
    serializeExpression(Seg.getExpr(), OutVec);
    break;

  case AST::DataSegment::DataMode::Passive:
    serializeU32(0x01, OutVec);
    break;

  default:
    break;
  }

  serializeU32(Seg.getData().size(), OutVec);
  OutVec.insert(OutVec.end(), Seg.getData().begin(), Seg.getData().end());
}

} // namespace Loader
} // namespace WasmEdge
