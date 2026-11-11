// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize table segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::TableSegment &Seg,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  // Table segment: tabletype
  //               |0x40 + 0x00 + tabletype + expr
  if (Seg.getExpr().getInstrs().size() > 0) {
    OutVec.push_back(0x40U);
    OutVec.push_back(0x00U);
    serializeType(Seg.getTableType(), OutVec);
    serializeExpression(Seg.getExpr(), OutVec);
  } else {
    serializeType(Seg.getTableType(), OutVec);
  }
}

// Serialize global segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::GlobalSegment &Seg,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  // Global segment: globaltype + expr.
  serializeType(Seg.getGlobalType(), OutVec);
  serializeExpression(Seg.getExpr(), OutVec);
}

// Serialize element segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::ElementSegment &Seg,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  // Element segment: mode:u32 + tableidx:u32 + offset:expr + elemkind:reftype +
  // vec(u32) + vec(expr)
  //
  // Modes 0x00 to 0x03 can only express ref.func initialisers.
  bool AllRefFunc = true;
  for (const auto &Expr : Seg.getInitExprs()) {
    const auto &Instrs = Expr.getInstrs();
    if (Instrs.size() != 2 || Instrs[0].getOpCode() != OpCode::Ref__func ||
        Instrs[1].getOpCode() != OpCode::End) {
      AllRefFunc = false;
      break;
    }
  }
  // The loader gives a segment of modes 0x00 to 0x03 the type (ref func), and
  // a segment of mode 0x04 the type funcref. Modes 0x00 and 0x04 also imply
  // table index 0. Another type takes modes 0x05 to 0x07 and an explicit
  // reftype. The binary then keeps the type of the segment, and a (ref func)
  // segment needs no Typed Function References proposal to load.
  const bool IsPassive =
      Seg.getMode() == AST::ElementSegment::ElemMode::Passive;
  const bool IsDeclarative =
      Seg.getMode() == AST::ElementSegment::ElemMode::Declarative;
  const uint8_t Kind = IsPassive           ? 0x01U
                       : IsDeclarative     ? 0x03U
                       : Seg.getIdx() != 0 ? 0x02U
                                           : 0x00U;

  uint8_t Mode;
  if (AllRefFunc &&
      Seg.getRefType() == ValType(TypeCode::Ref, TypeCode::FuncRef)) {
    Mode = Kind;
  } else if (Kind == 0x00U && Seg.getRefType() == ValType(TypeCode::RefNull,
                                                          TypeCode::FuncRef)) {
    Mode = 0x04U;
  } else {
    Mode = 0x04U | (Kind == 0x00U ? 0x02U : Kind);
  }
  OutVec.push_back(Mode);

  // Serialize the table index for the active modes carrying one.
  if ((Mode & 0x03U) == 0x02U) {
    serializeU32(Seg.getIdx(), OutVec);
  }

  // Serialize the offset expression for the active modes.
  if (!IsPassive && !IsDeclarative) {
    serializeExpression(Seg.getExpr(), OutVec);
  }

  // Serialize the element kind or the reference type.
  if (Mode & 0x03U) {
    if (Mode & 0x04U) {
      serializeRefType(Seg.getRefType(), OutVec);
    } else {
      OutVec.push_back(0x00U);
    }
  }

  // Serialize vec(funcidx) or vec(expr).
  assuming(Seg.getInitExprs().size() <= UINT32_MAX);
  serializeU32(static_cast<uint32_t>(Seg.getInitExprs().size()), OutVec);
  for (const auto &Expr : Seg.getInitExprs()) {
    if (Mode & 0x04U) {
      serializeExpression(Expr, OutVec);
    } else {
      serializeU32(Expr.getInstrs()[0].getTargetIndex(), OutVec);
    }
  }
}

// Serialize code segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::CodeSegment &Seg,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  // Code segment: size:u32 + locals:vec(u32 + valtype) + body:expr.
  assuming(Seg.getLocals().size() <= UINT32_MAX);
  auto OrgSize = OutVec.size();
  serializeU32(static_cast<uint32_t>(Seg.getLocals().size()), OutVec);
  for (auto &Locals : Seg.getLocals()) {
    serializeU32(Locals.first, OutVec);
    serializeValType(Locals.second, OutVec);
  }
  serializeExpression(Seg.getExpr(), OutVec);
  // Backward insert the code segment size.
  serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
               std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
}

// Serialize data segment. See "include/loader/serialize.h".
void Serializer::serializeSegment(const AST::DataSegment &Seg,
                                  std::vector<uint8_t> &OutVec) const noexcept {
  // Data segment: mode:u32 + memidx:u32 + expr + vec(byte)
  switch (Seg.getMode()) {
  case AST::DataSegment::DataMode::Active:
    if (Seg.getIdx() != 0) {
      OutVec.push_back(0x02U);
      serializeU32(Seg.getIdx(), OutVec);
    } else {
      OutVec.push_back(0x00U);
    }
    serializeExpression(Seg.getExpr(), OutVec);
    break;
  case AST::DataSegment::DataMode::Passive:
    OutVec.push_back(0x01U);
    break;
  default:
    assumingUnreachable();
  }

  assuming(Seg.getData().size() <= UINT32_MAX);
  serializeU32(static_cast<uint32_t>(Seg.getData().size()), OutVec);
  OutVec.insert(OutVec.end(), Seg.getData().begin(), Seg.getData().end());
}

} // namespace Loader
} // namespace WasmEdge
