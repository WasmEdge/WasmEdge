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
  uint8_t Mode = 0x00;
  auto ModePosition = OutVec.end();
  switch (Seg.getMode()) {
  case AST::ElementSegment::ElemMode::Declarative:
    Mode |= 0x02;
    [[fallthrough]];

  case AST::ElementSegment::ElemMode::Passive:
    Mode |= 0x01;

  default:
    break;
  }

  // Serialize idx
  if (Seg.getIdx() != 0) {
    serializeU32(Seg.getIdx(), OutVec);
    if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
      Mode |= 0x02;
    } else {
      // TODO: return error
      return;
    }
  }

  if (Seg.getMode() == AST::ElementSegment::ElemMode::Active) {
    serializeExpression(Seg.getExpr(), OutVec);
  }

  if (Mode == 0) {
    // TODO: not sure whether elementType can be a FuncRef.
    if (Seg.getRefType() == RefType::FuncRef) {
      serializeU32(0x00, OutVec);
    } else {
      serializeU32(static_cast<uint8_t>(Seg.getRefType()), OutVec);
      Mode |= 0x04;
    }

    for (auto Expr : Seg.getInitExprs()) {
      if (Mode & 0x04) {
        // vec(expr)
        serializeExpression(Expr, OutVec);
      } else {
        // vec(expr)
        for (auto Instr : Expr.getInstrs()) {
          serializeU32(Instr.getTargetIndex(), OutVec);
        }
        serializeU32(0x0B, OutVec);
      }
    }
    serializeU32(0x0B, OutVec);
  }

  serializeU32(Mode, OutVec, ModePosition);
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
