// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/errcode.h"
#include "converter.h"

#include <cstring>
#include <string_view>
#include <unordered_map>

using namespace std::string_view_literals;

namespace WasmEdge::WAT {

// ---------------------------------------------------------------------------
// Static opcode lookup table (WAT text name -> OpCode)
// ---------------------------------------------------------------------------
static const std::unordered_map<std::string_view, OpCode, Hash::Hash> &
getOpcodeMap() {
  static const std::unordered_map<std::string_view, OpCode, Hash::Hash> Map = {
#define Line(Name, Str, ...) {Str##sv, OpCode::Name},
#define Line_FB(Name, Str, ...) {Str##sv, OpCode::Name},
#define Line_FC(Name, Str, ...) {Str##sv, OpCode::Name},
#define Line_FD(Name, Str, ...) {Str##sv, OpCode::Name},
#define Line_FE(Name, Str, ...) {Str##sv, OpCode::Name},
#define UseOpCode
#include "common/enum.inc"
  };
  return Map;
}

// ---------------------------------------------------------------------------
// Natural alignment (log2 of byte size) for memory instructions
// ---------------------------------------------------------------------------
static uint32_t naturalAlign(OpCode Code) {
  switch (Code) {
  case OpCode::I32__load:
  case OpCode::I32__store:
  case OpCode::F32__load:
  case OpCode::F32__store:
  case OpCode::I32__atomic__load:
  case OpCode::I32__atomic__store:
  case OpCode::I32__atomic__rmw__add:
  case OpCode::I32__atomic__rmw__sub:
  case OpCode::I32__atomic__rmw__and:
  case OpCode::I32__atomic__rmw__or:
  case OpCode::I32__atomic__rmw__xor:
  case OpCode::I32__atomic__rmw__xchg:
  case OpCode::I32__atomic__rmw__cmpxchg:
  case OpCode::V128__load32_splat:
  case OpCode::V128__load32_zero:
  case OpCode::Memory__atomic__notify:
  case OpCode::Memory__atomic__wait32:
    return 2;

  case OpCode::I64__load:
  case OpCode::I64__store:
  case OpCode::F64__load:
  case OpCode::F64__store:
  case OpCode::I64__atomic__load:
  case OpCode::I64__atomic__store:
  case OpCode::I64__atomic__rmw__add:
  case OpCode::I64__atomic__rmw__sub:
  case OpCode::I64__atomic__rmw__and:
  case OpCode::I64__atomic__rmw__or:
  case OpCode::I64__atomic__rmw__xor:
  case OpCode::I64__atomic__rmw__xchg:
  case OpCode::I64__atomic__rmw__cmpxchg:
  case OpCode::V128__load64_splat:
  case OpCode::V128__load64_zero:
  case OpCode::Memory__atomic__wait64:
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
    return 3;

  case OpCode::V128__load:
  case OpCode::V128__store:
    return 4;

  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I32__store16:
  case OpCode::I64__store16:
  case OpCode::I32__atomic__load16_u:
  case OpCode::I64__atomic__load16_u:
  case OpCode::I32__atomic__store16:
  case OpCode::I64__atomic__store16:
  case OpCode::I32__atomic__rmw16__add_u:
  case OpCode::I64__atomic__rmw16__add_u:
  case OpCode::I32__atomic__rmw16__sub_u:
  case OpCode::I64__atomic__rmw16__sub_u:
  case OpCode::I32__atomic__rmw16__and_u:
  case OpCode::I64__atomic__rmw16__and_u:
  case OpCode::I32__atomic__rmw16__or_u:
  case OpCode::I64__atomic__rmw16__or_u:
  case OpCode::I32__atomic__rmw16__xor_u:
  case OpCode::I64__atomic__rmw16__xor_u:
  case OpCode::I32__atomic__rmw16__xchg_u:
  case OpCode::I64__atomic__rmw16__xchg_u:
  case OpCode::I32__atomic__rmw16__cmpxchg_u:
  case OpCode::I64__atomic__rmw16__cmpxchg_u:
  case OpCode::V128__load16_splat:
    return 1;

  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I64__store32:
  case OpCode::I64__atomic__load32_u:
  case OpCode::I64__atomic__store32:
  case OpCode::I64__atomic__rmw32__add_u:
  case OpCode::I64__atomic__rmw32__sub_u:
  case OpCode::I64__atomic__rmw32__and_u:
  case OpCode::I64__atomic__rmw32__or_u:
  case OpCode::I64__atomic__rmw32__xor_u:
  case OpCode::I64__atomic__rmw32__xchg_u:
  case OpCode::I64__atomic__rmw32__cmpxchg_u:
    return 2;

  // 8-bit loads/stores
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I32__store8:
  case OpCode::I64__store8:
  case OpCode::I32__atomic__load8_u:
  case OpCode::I64__atomic__load8_u:
  case OpCode::I32__atomic__store8:
  case OpCode::I64__atomic__store8:
  case OpCode::I32__atomic__rmw8__add_u:
  case OpCode::I64__atomic__rmw8__add_u:
  case OpCode::I32__atomic__rmw8__sub_u:
  case OpCode::I64__atomic__rmw8__sub_u:
  case OpCode::I32__atomic__rmw8__and_u:
  case OpCode::I64__atomic__rmw8__and_u:
  case OpCode::I32__atomic__rmw8__or_u:
  case OpCode::I64__atomic__rmw8__or_u:
  case OpCode::I32__atomic__rmw8__xor_u:
  case OpCode::I64__atomic__rmw8__xor_u:
  case OpCode::I32__atomic__rmw8__xchg_u:
  case OpCode::I64__atomic__rmw8__xchg_u:
  case OpCode::I32__atomic__rmw8__cmpxchg_u:
  case OpCode::I64__atomic__rmw8__cmpxchg_u:
  case OpCode::V128__load8_splat:
    return 0;

  // SIMD lane load/store
  case OpCode::V128__load8_lane:
  case OpCode::V128__store8_lane:
    return 0;
  case OpCode::V128__load16_lane:
  case OpCode::V128__store16_lane:
    return 1;
  case OpCode::V128__load32_lane:
  case OpCode::V128__store32_lane:
    return 2;
  case OpCode::V128__load64_lane:
  case OpCode::V128__store64_lane:
    return 3;

  default:
    return 0;
  }
}

// ---------------------------------------------------------------------------
// Check if an opcode is a memory instruction needing memarg
// ---------------------------------------------------------------------------
// Check if opcode is a SIMD lane load/store that also needs a lane byte
static bool isSimdLaneMemOp(OpCode Code) {
  switch (Code) {
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
    return true;
  default:
    return false;
  }
}

// ---------------------------------------------------------------------------
// Helper: get the opcode text from a plain_instr node.
// The first anonymous child token gives the opcode keyword.
// For token rules like _numeric_op, _simd_op, the node text IS the opcode.
// ---------------------------------------------------------------------------
static std::string_view getOpcodeText(Node N, std::string_view Source) {
  // plain_instr -> _plain_op. The first child is the opcode keyword token.
  // For multi-child sequences like "br $label", child(0) is "br".
  // For bare tokens like _numeric_op, child(0) is the entire text.
  if (N.childCount() > 0) {
    Node First = N.child(0);
    return First.text(Source);
  }
  return N.text(Source);
}

// ---------------------------------------------------------------------------
// Helper: parse offset=N from an offset node
// ---------------------------------------------------------------------------
static Expect<uint64_t> parseOffset(std::string_view Text) {
  // Text is "offset=N"
  if (Text.size() > 7 && Text.substr(0, 7) == "offset="sv) {
    Text = Text.substr(7);
  }
  // Strip underscores
  std::string Cleaned;
  Cleaned.reserve(Text.size());
  for (char C : Text) {
    if (C != '_') {
      Cleaned.push_back(C);
    }
  }
  char *End = nullptr;
  errno = 0;
  uint64_t Val = std::strtoull(Cleaned.c_str(), &End, 0);
  if (errno == ERANGE || End != Cleaned.c_str() + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  return Val;
}

// ---------------------------------------------------------------------------
// Helper: parse align=N from an align node
// ---------------------------------------------------------------------------
static Expect<uint32_t> parseAlign(std::string_view Text) {
  // Text is "align=N"
  if (Text.size() > 6 && Text.substr(0, 6) == "align="sv) {
    Text = Text.substr(6);
  }
  std::string Cleaned;
  Cleaned.reserve(Text.size());
  for (char C : Text) {
    if (C != '_') {
      Cleaned.push_back(C);
    }
  }
  char *End = nullptr;
  errno = 0;
  uint64_t Val = std::strtoull(Cleaned.c_str(), &End, 0);
  if (errno == ERANGE || End != Cleaned.c_str() + Cleaned.size()) {
    return Unexpect(ErrCode::Value::WatMalformedNumber);
  }
  // align value in WAT is the actual alignment (power of 2).
  // We store log2 of the alignment.
  uint32_t Log2 = 0;
  uint64_t V = Val;
  while (V > 1) {
    V >>= 1;
    ++Log2;
  }
  return Log2;
}

// ---------------------------------------------------------------------------
// convertExpression
// ---------------------------------------------------------------------------
Expect<void> Converter::convertExpression(Node N, AST::Expression &Expr) {
  auto &Instrs = Expr.getInstrs();
  // N is typically a node whose named children are instructions.
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto Type = nodeType(Child);
    // Skip non-instruction children (identifiers, params, results, etc.)
    if (Type == "plain_instr"sv || Type == "block_instr"sv ||
        Type == "block_block"sv || Type == "block_loop"sv ||
        Type == "block_if"sv || Type == "block_try_table"sv ||
        Type == "folded_instr"sv) {
      EXPECTED_TRY(convertInstr(Child, Instrs));
    }
  }
  // Add End instruction with ExprLast flag.
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);
  return {};
}

// ---------------------------------------------------------------------------
// convertInstr
// ---------------------------------------------------------------------------
Expect<void> Converter::convertInstr(Node N, AST::InstrVec &Instrs) {
  auto Type = nodeType(N);

  if (Type == "plain_instr"sv) {
    return convertPlainInstr(N, Instrs);
  } else if (Type == "block_block"sv || Type == "block_loop"sv ||
             Type == "block_if"sv || Type == "block_try_table"sv) {
    return convertBlockInstr(N, Instrs);
  } else if (Type == "block_instr"sv) {
    // block_instr is a wrapper around
    // block_block/block_loop/block_if/block_try_table
    if (N.namedChildCount() > 0) {
      return convertInstr(N.namedChild(0), Instrs);
    }
    // Descend into the single child
    for (uint32_t I = 0; I < N.childCount(); ++I) {
      Node Child = N.child(I);
      if (Child.isNamed()) {
        return convertInstr(Child, Instrs);
      }
    }
    return {};
  } else if (Type == "folded_instr"sv) {
    return convertFoldedInstr(N, Instrs);
  }

  // Unknown instruction type — skip
  return {};
}

// ---------------------------------------------------------------------------
// Helper: convert a block signature (block_sig node)
// ---------------------------------------------------------------------------
Expect<void> Converter::convertPlainInstr(Node N, AST::InstrVec &Instrs) {
  // Get the opcode text from the first child token.
  auto OpText = getOpcodeText(N, Source);

  // Look up opcode in the static map.
  auto &Map = getOpcodeMap();
  auto It = Map.find(OpText);

  // ref.test / ref.cast are not in the opcode map (their enum strings contain
  // extra suffixes like "(ref)" / "(ref.null)"), so handle them before the map
  // lookup failure check.
  if (It == Map.end()) {
    if (OpText == "ref.test"sv || OpText == "ref.cast"sv) {
      // The ref_cast_type child determines nullable vs non-nullable
      // _ref_cast_type = reftype | heap_type
      // If it's (ref null ht) -> nullable, if (ref ht) -> non-nullable
      bool IsNull = false;
      ValType HType;
      for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
        Node Child = N.namedChild(I);
        auto CType = nodeType(Child);
        if (CType == "reftype"sv || CType == "ref_type_short"sv ||
            CType == "ref_type_full"sv) {
          EXPECTED_TRY(auto VT, convertRefType(Child));
          HType = VT;
          // Check if it's nullable
          if (CType == "ref_type_full"sv) {
            for (uint32_t J = 0; J < Child.childCount(); ++J) {
              if (nodeText(Child.child(J)) == "null"sv) {
                IsNull = true;
                break;
              }
            }
          } else if (CType == "ref_type_short"sv) {
            // Shorthand ref types are all nullable
            IsNull = true;
          } else if (CType == "reftype"sv) {
            // Check the inner child
            if (Child.namedChildCount() > 0) {
              Node Inner = Child.namedChild(0);
              if (nodeType(Inner) == "ref_type_full"sv) {
                for (uint32_t J = 0; J < Inner.childCount(); ++J) {
                  if (nodeText(Inner.child(J)) == "null"sv) {
                    IsNull = true;
                    break;
                  }
                }
              } else if (nodeType(Inner) == "ref_type_short"sv) {
                IsNull = true;
              }
            }
          }
          break;
        } else if (CType == "heap_type"sv) {
          EXPECTED_TRY(auto VT, convertHeapType(Child));
          HType = VT;
          IsNull = false;
          break;
        }
      }

      OpCode Code;
      if (OpText == "ref.test"sv) {
        Code = IsNull ? OpCode::Ref__test_null : OpCode::Ref__test;
      } else {
        Code = IsNull ? OpCode::Ref__cast_null : OpCode::Ref__cast;
      }
      Instrs.emplace_back(Code);
      Instrs.back().setValType(HType);
      return {};
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  OpCode Code = It->second;

  // -----------------------------------------------------------------------
  // Dispatch based on opcode category
  // -----------------------------------------------------------------------

  switch (Code) {

  // --- select: both Select and Select_t share the same opcode string
  // "select" in enum.inc, so the map only stores one. Disambiguate by
  // checking whether result type annotations are present. ---
  case OpCode::Select:
  case OpCode::Select_t: {
    std::vector<ValType> ResultTypes;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if (CType == "numtype"sv || CType == "vectype"sv ||
          CType == "reftype"sv || CType == "ref_type_short"sv ||
          CType == "ref_type_full"sv) {
        EXPECTED_TRY(auto VT, convertValType(Child));
        ResultTypes.push_back(VT);
      }
    }
    if (ResultTypes.empty()) {
      Instrs.emplace_back(OpCode::Select);
    } else {
      Instrs.emplace_back(OpCode::Select_t);
      auto &Instr = Instrs.back();
      Instr.setValTypeListSize(static_cast<uint32_t>(ResultTypes.size()));
      auto List = Instr.getValTypeList();
      for (uint32_t I = 0; I < ResultTypes.size(); ++I) {
        List[I] = ResultTypes[I];
      }
    }
    return {};
  }

  // --- Const ops ---
  case OpCode::I32__const: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      EXPECTED_TRY(auto Val, parseInt(nodeText(N.namedChild(I))));
      Instrs.back().setNum(
          ValVariant(static_cast<uint32_t>(static_cast<int32_t>(Val))));
      break;
    }
    return {};
  }
  case OpCode::I64__const: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Val, parseInt(nodeText(Child)));
      Instrs.back().setNum(ValVariant(static_cast<uint64_t>(Val)));
      break;
    }
    return {};
  }
  case OpCode::F32__const: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Val, parseF32(nodeText(Child)));
      Instrs.back().setNum(ValVariant(Val));
      break;
    }
    return {};
  }
  case OpCode::F64__const: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Val, parseF64(nodeText(Child)));
      Instrs.back().setNum(ValVariant(Val));
      break;
    }
    return {};
  }

  // --- Variable ops ---
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveLocal(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }
  case OpCode::Global__get:
  case OpCode::Global__set: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveGlobal(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- Branch ops ---
  case OpCode::Br:
  case OpCode::Br_if:
  case OpCode::Br_on_null:
  case OpCode::Br_on_non_null: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Child)));
      Instrs.back().getJump().TargetIndex = Idx;
      break;
    }
    return {};
  }

  // --- br_table ---
  case OpCode::Br_table: {
    // Collect all label indices
    std::vector<uint32_t> Labels;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Child)));
      Labels.push_back(Idx);
    }
    Instrs.emplace_back(Code);
    auto &Instr = Instrs.back();
    Instr.setLabelListSize(static_cast<uint32_t>(Labels.size()));
    auto List = Instr.getLabelList();
    for (uint32_t I = 0; I < Labels.size(); ++I) {
      List[I].TargetIndex = Labels[I];
    }
    return {};
  }

  // --- Call ops ---
  case OpCode::Call:
  case OpCode::Return_call: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect: {
    // Optional table index, then type_use, then param/result
    uint32_t TableIdx = 0;
    uint32_t TypeIdx = 0;
    bool HasTypeUse = false;

    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if (CType == "type_use"sv) {
        HasTypeUse = true;
        // (type $idx)
        if (Child.namedChildCount() > 0) {
          EXPECTED_TRY(auto Idx,
                       Syms.resolveType(nodeText(Child.namedChild(0))));
          TypeIdx = Idx;
        }
      } else if (CType == "identifier"sv || CType == "nat"sv) {
        if (!HasTypeUse) {
          // This is the table index
          EXPECTED_TRY(auto Idx, Syms.resolveTable(nodeText(Child)));
          TableIdx = Idx;
        }
      } else if (CType == "param"sv || CType == "result"sv) {
        // Inline type — skip for now (type_use handles it)
      }
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = TableIdx;
    return {};
  }
  case OpCode::Call_ref:
  case OpCode::Return_call_ref: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- ref.null ---
  case OpCode::Ref__null: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "heap_type"sv) {
        EXPECTED_TRY(auto VT, convertHeapType(Child));
        Instrs.back().setValType(VT);
        break;
      }
    }
    return {};
  }

  // --- ref.func ---
  case OpCode::Ref__func: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveFunc(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- ref.i31 ---
  case OpCode::Ref__i31: {
    Instrs.emplace_back(Code);
    return {};
  }

  // --- br_on_cast / br_on_cast_fail ---
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail: {
    // label_idx, ref_cast_type, ref_cast_type
    uint32_t LabelIdx = 0;
    ValType RT1, RT2;
    int RefTypeCount = 0;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if ((CType == "identifier"sv || CType == "nat"sv) && RefTypeCount == 0) {
        // First idx-like child before any ref types is the label
        EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Child)));
        LabelIdx = Idx;
      } else if (CType == "reftype"sv || CType == "ref_type_short"sv ||
                 CType == "ref_type_full"sv || CType == "heap_type"sv) {
        ValType VT;
        if (CType == "heap_type"sv) {
          EXPECTED_TRY(auto T, convertHeapType(Child));
          VT = T;
        } else {
          EXPECTED_TRY(auto T, convertRefType(Child));
          VT = T;
        }
        if (RefTypeCount == 0) {
          RT1 = VT;
        } else {
          RT2 = VT;
        }
        ++RefTypeCount;
      }
    }
    Instrs.emplace_back(Code);
    Instrs.back().setBrCast(LabelIdx);
    Instrs.back().getBrCast().RType1 = RT1;
    Instrs.back().getBrCast().RType2 = RT2;
    return {};
  }

  // --- Table ops ---
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__size:
  case OpCode::Table__grow:
  case OpCode::Table__fill: {
    uint32_t TableIdx = 0;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveTable(nodeText(Child)));
      TableIdx = Idx;
      break;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TableIdx;
    return {};
  }
  case OpCode::Table__copy: {
    uint32_t DstIdx = 0, SrcIdx = 0;
    uint32_t Count = 0;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveTable(nodeText(Child)));
      if (Count == 0) {
        DstIdx = Idx;
      } else {
        SrcIdx = Idx;
      }
      ++Count;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = DstIdx;
    Instrs.back().getSourceIndex() = SrcIdx;
    return {};
  }
  case OpCode::Table__init: {
    // Optional table idx, then elem idx
    uint32_t TableIdx = 0;
    uint32_t ElemIdx = 0;
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveTable(Texts[0]));
      EXPECTED_TRY(auto EIdx, Syms.resolveElem(Texts[1]));
      TableIdx = TIdx;
      ElemIdx = EIdx;
    } else if (Texts.size() == 1) {
      EXPECTED_TRY(auto EIdx, Syms.resolveElem(Texts[0]));
      ElemIdx = EIdx;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = ElemIdx;
    Instrs.back().getSourceIndex() = TableIdx;
    return {};
  }
  case OpCode::Elem__drop: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveElem(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- Memory control ---
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
  case OpCode::Memory__fill: {
    uint32_t MemIdx = 0;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveMemory(nodeText(Child)));
      MemIdx = Idx;
      break;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = MemIdx;
    return {};
  }
  case OpCode::Memory__copy: {
    uint32_t DstIdx = 0, SrcIdx = 0;
    uint32_t Count = 0;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveMemory(nodeText(Child)));
      if (Count == 0) {
        DstIdx = Idx;
      } else {
        SrcIdx = Idx;
      }
      ++Count;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = DstIdx;
    Instrs.back().getSourceIndex() = SrcIdx;
    return {};
  }
  case OpCode::Memory__init: {
    // Optional memory idx, then data idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if (CType == "identifier"sv || CType == "nat"sv) {
        Texts.push_back(nodeText(Child));
      }
    }
    uint32_t MemIdx = 0, DataIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto MIdx, Syms.resolveMemory(Texts[0]));
      EXPECTED_TRY(auto DIdx, Syms.resolveData(Texts[1]));
      MemIdx = MIdx;
      DataIdx = DIdx;
    } else if (Texts.size() == 1) {
      EXPECTED_TRY(auto DIdx, Syms.resolveData(Texts[0]));
      DataIdx = DIdx;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = DataIdx;
    Instrs.back().getSourceIndex() = MemIdx;
    return {};
  }
  case OpCode::Data__drop: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveData(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- GC struct/array ops ---
  case OpCode::Struct__new:
  case OpCode::Struct__new_default:
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u:
  case OpCode::Array__set:
  case OpCode::Array__fill: {
    // Single type index
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set: {
    // type_idx + field_idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t TypeIdx = 0, FieldIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto FIdx, parseUint(Texts[1]));
      TypeIdx = TIdx;
      FieldIdx = static_cast<uint32_t>(FIdx);
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = FieldIdx;
    return {};
  }
  case OpCode::Array__new_fixed: {
    // type_idx + nat (count)
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t TypeIdx = 0, Count = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto Cnt, parseUint(Texts[1]));
      TypeIdx = TIdx;
      Count = static_cast<uint32_t>(Cnt);
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = Count;
    return {};
  }
  case OpCode::Array__new_elem: {
    // type_idx + elem_idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t TypeIdx = 0, ElemIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto EIdx, Syms.resolveElem(Texts[1]));
      TypeIdx = TIdx;
      ElemIdx = EIdx;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = ElemIdx;
    return {};
  }
  case OpCode::Array__new_data:
  case OpCode::Array__init_data: {
    // type_idx + data_idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t TypeIdx = 0, DataIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto DIdx, Syms.resolveData(Texts[1]));
      TypeIdx = TIdx;
      DataIdx = DIdx;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = DataIdx;
    return {};
  }
  case OpCode::Array__init_elem: {
    // type_idx + elem_idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t TypeIdx = 0, ElemIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto EIdx, Syms.resolveElem(Texts[1]));
      TypeIdx = TIdx;
      ElemIdx = EIdx;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = TypeIdx;
    Instrs.back().getSourceIndex() = ElemIdx;
    return {};
  }
  case OpCode::Array__copy: {
    // type_idx + type_idx
    std::vector<std::string_view> Texts;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Texts.push_back(nodeText(N.namedChild(I)));
    }
    uint32_t DstTypeIdx = 0, SrcTypeIdx = 0;
    if (Texts.size() >= 2) {
      EXPECTED_TRY(auto TIdx1, Syms.resolveType(Texts[0]));
      EXPECTED_TRY(auto TIdx2, Syms.resolveType(Texts[1]));
      DstTypeIdx = TIdx1;
      SrcTypeIdx = TIdx2;
    }
    Instrs.emplace_back(Code);
    Instrs.back().getTargetIndex() = DstTypeIdx;
    Instrs.back().getSourceIndex() = SrcTypeIdx;
    return {};
  }

  // --- throw ---
  case OpCode::Throw: {
    Instrs.emplace_back(Code);
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto Idx, Syms.resolveTag(nodeText(Child)));
      Instrs.back().getTargetIndex() = Idx;
      break;
    }
    return {};
  }

  // --- atomic.fence ---
  case OpCode::Atomic__fence: {
    Instrs.emplace_back(Code);
    return {};
  }

  // --- v128.const ---
  case OpCode::V128__const: {
    // v128.const shape num*
    // Parse all number children and pack into 128-bit value
    Instrs.emplace_back(Code);
    // Collect all named children after the shape
    std::vector<std::string_view> Nums;
    bool FoundShape = false;
    std::string_view Shape;
    for (uint32_t I = 0; I < N.childCount(); ++I) {
      Node Child = N.child(I);
      auto CText = Child.text(Source);
      if (!FoundShape) {
        if (CText == "i8x16"sv || CText == "i16x8"sv || CText == "i32x4"sv ||
            CText == "i64x2"sv || CText == "f32x4"sv || CText == "f64x2"sv) {
          Shape = CText;
          FoundShape = true;
        }
        continue;
      }
      // After shape, collect number values
      if (Child.isNamed()) {
        Nums.push_back(CText);
      }
    }

    uint128_t Val = static_cast<uint128_t>(0);
    uint8_t *Bytes = reinterpret_cast<uint8_t *>(&Val);

    if (Shape == "i8x16"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 16; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        Bytes[I] = static_cast<uint8_t>(V);
      }
    } else if (Shape == "i16x8"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 8; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        uint16_t U = static_cast<uint16_t>(V);
        std::memcpy(Bytes + I * 2, &U, 2);
      }
    } else if (Shape == "i32x4"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 4; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        uint32_t U = static_cast<uint32_t>(V);
        std::memcpy(Bytes + I * 4, &U, 4);
      }
    } else if (Shape == "i64x2"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 2; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        uint64_t U = static_cast<uint64_t>(V);
        std::memcpy(Bytes + I * 8, &U, 8);
      }
    } else if (Shape == "f32x4"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 4; ++I) {
        EXPECTED_TRY(auto V, parseF32(Nums[I]));
        std::memcpy(Bytes + I * 4, &V, 4);
      }
    } else if (Shape == "f64x2"sv) {
      for (uint32_t I = 0; I < Nums.size() && I < 2; ++I) {
        EXPECTED_TRY(auto V, parseF64(Nums[I]));
        std::memcpy(Bytes + I * 8, &V, 8);
      }
    }

    Instrs.back().setNum(ValVariant(Val));
    return {};
  }

  // --- i8x16.shuffle ---
  case OpCode::I8x16__shuffle: {
    // 16 lane indices packed into 128-bit value
    Instrs.emplace_back(Code);
    uint128_t Val = static_cast<uint128_t>(0);
    uint8_t *Bytes = reinterpret_cast<uint8_t *>(&Val);
    uint32_t Idx = 0;
    for (uint32_t I = 0; I < N.namedChildCount() && Idx < 16; ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto V, parseUint(nodeText(Child)));
      Bytes[Idx++] = static_cast<uint8_t>(V);
    }
    Instrs.back().setNum(ValVariant(Val));
    return {};
  }

  // --- Memory load/store ops (including SIMD and atomic) ---
  case OpCode::I32__load:
  case OpCode::I64__load:
  case OpCode::F32__load:
  case OpCode::F64__load:
  case OpCode::I32__load8_s:
  case OpCode::I32__load8_u:
  case OpCode::I32__load16_s:
  case OpCode::I32__load16_u:
  case OpCode::I64__load8_s:
  case OpCode::I64__load8_u:
  case OpCode::I64__load16_s:
  case OpCode::I64__load16_u:
  case OpCode::I64__load32_s:
  case OpCode::I64__load32_u:
  case OpCode::I32__store:
  case OpCode::I64__store:
  case OpCode::F32__store:
  case OpCode::F64__store:
  case OpCode::I32__store8:
  case OpCode::I32__store16:
  case OpCode::I64__store8:
  case OpCode::I64__store16:
  case OpCode::I64__store32:
  case OpCode::V128__load:
  case OpCode::V128__store:
  case OpCode::V128__load8x8_s:
  case OpCode::V128__load8x8_u:
  case OpCode::V128__load16x4_s:
  case OpCode::V128__load16x4_u:
  case OpCode::V128__load32x2_s:
  case OpCode::V128__load32x2_u:
  case OpCode::V128__load8_splat:
  case OpCode::V128__load16_splat:
  case OpCode::V128__load32_splat:
  case OpCode::V128__load64_splat:
  case OpCode::V128__load32_zero:
  case OpCode::V128__load64_zero:
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
  case OpCode::Memory__atomic__notify:
  case OpCode::Memory__atomic__wait32:
  case OpCode::Memory__atomic__wait64:
  case OpCode::I32__atomic__load:
  case OpCode::I64__atomic__load:
  case OpCode::I32__atomic__load8_u:
  case OpCode::I32__atomic__load16_u:
  case OpCode::I64__atomic__load8_u:
  case OpCode::I64__atomic__load16_u:
  case OpCode::I64__atomic__load32_u:
  case OpCode::I32__atomic__store:
  case OpCode::I64__atomic__store:
  case OpCode::I32__atomic__store8:
  case OpCode::I32__atomic__store16:
  case OpCode::I64__atomic__store8:
  case OpCode::I64__atomic__store16:
  case OpCode::I64__atomic__store32:
  case OpCode::I32__atomic__rmw__add:
  case OpCode::I64__atomic__rmw__add:
  case OpCode::I32__atomic__rmw8__add_u:
  case OpCode::I32__atomic__rmw16__add_u:
  case OpCode::I64__atomic__rmw8__add_u:
  case OpCode::I64__atomic__rmw16__add_u:
  case OpCode::I64__atomic__rmw32__add_u:
  case OpCode::I32__atomic__rmw__sub:
  case OpCode::I64__atomic__rmw__sub:
  case OpCode::I32__atomic__rmw8__sub_u:
  case OpCode::I32__atomic__rmw16__sub_u:
  case OpCode::I64__atomic__rmw8__sub_u:
  case OpCode::I64__atomic__rmw16__sub_u:
  case OpCode::I64__atomic__rmw32__sub_u:
  case OpCode::I32__atomic__rmw__and:
  case OpCode::I64__atomic__rmw__and:
  case OpCode::I32__atomic__rmw8__and_u:
  case OpCode::I32__atomic__rmw16__and_u:
  case OpCode::I64__atomic__rmw8__and_u:
  case OpCode::I64__atomic__rmw16__and_u:
  case OpCode::I64__atomic__rmw32__and_u:
  case OpCode::I32__atomic__rmw__or:
  case OpCode::I64__atomic__rmw__or:
  case OpCode::I32__atomic__rmw8__or_u:
  case OpCode::I32__atomic__rmw16__or_u:
  case OpCode::I64__atomic__rmw8__or_u:
  case OpCode::I64__atomic__rmw16__or_u:
  case OpCode::I64__atomic__rmw32__or_u:
  case OpCode::I32__atomic__rmw__xor:
  case OpCode::I64__atomic__rmw__xor:
  case OpCode::I32__atomic__rmw8__xor_u:
  case OpCode::I32__atomic__rmw16__xor_u:
  case OpCode::I64__atomic__rmw8__xor_u:
  case OpCode::I64__atomic__rmw16__xor_u:
  case OpCode::I64__atomic__rmw32__xor_u:
  case OpCode::I32__atomic__rmw__xchg:
  case OpCode::I64__atomic__rmw__xchg:
  case OpCode::I32__atomic__rmw8__xchg_u:
  case OpCode::I32__atomic__rmw16__xchg_u:
  case OpCode::I64__atomic__rmw8__xchg_u:
  case OpCode::I64__atomic__rmw16__xchg_u:
  case OpCode::I64__atomic__rmw32__xchg_u:
  case OpCode::I32__atomic__rmw__cmpxchg:
  case OpCode::I64__atomic__rmw__cmpxchg:
  case OpCode::I32__atomic__rmw8__cmpxchg_u:
  case OpCode::I32__atomic__rmw16__cmpxchg_u:
  case OpCode::I64__atomic__rmw8__cmpxchg_u:
  case OpCode::I64__atomic__rmw16__cmpxchg_u:
  case OpCode::I64__atomic__rmw32__cmpxchg_u: {
    uint32_t MemIdx = 0;
    uint64_t Offset = 0;
    uint32_t Align = naturalAlign(Code);
    uint8_t Lane = 0;

    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      auto CText = nodeText(Child);

      if (CType == "offset"sv) {
        EXPECTED_TRY(auto Off, parseOffset(CText));
        Offset = Off;
      } else if (CType == "align"sv) {
        EXPECTED_TRY(auto Al, parseAlign(CText));
        Align = Al;
      } else if (CType == "identifier"sv || CType == "nat"sv) {
        // Could be memory index or lane index for SIMD lane ops
        if (isSimdLaneMemOp(Code) && I == N.namedChildCount() - 1) {
          // Last named child is the lane for SIMD lane ops
          EXPECTED_TRY(auto L, parseUint(CText));
          Lane = static_cast<uint8_t>(L);
        } else {
          EXPECTED_TRY(auto Idx, Syms.resolveMemory(CText));
          MemIdx = Idx;
        }
      }
    }

    Instrs.emplace_back(Code);
    auto &Instr = Instrs.back();
    Instr.getMemoryOffset() = Offset;
    Instr.getMemoryAlign() = Align;
    Instr.getTargetIndex() = MemIdx;
    if (isSimdLaneMemOp(Code)) {
      Instr.getMemoryLane() = Lane;
    }
    return {};
  }

  // --- SIMD lane ops (extract/replace) ---
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
  case OpCode::I8x16__replace_lane:
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
  case OpCode::I16x8__replace_lane:
  case OpCode::I32x4__extract_lane:
  case OpCode::I32x4__replace_lane:
  case OpCode::I64x2__extract_lane:
  case OpCode::I64x2__replace_lane:
  case OpCode::F32x4__extract_lane:
  case OpCode::F32x4__replace_lane:
  case OpCode::F64x2__extract_lane:
  case OpCode::F64x2__replace_lane: {
    Instrs.emplace_back(Code);
    // The lane index is a named child (nat)
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      EXPECTED_TRY(auto V, parseUint(nodeText(Child)));
      Instrs.back().getMemoryLane() = static_cast<uint8_t>(V);
      break;
    }
    return {};
  }

  // --- Default: no immediates ---
  default:
    Instrs.emplace_back(Code);
    return {};
  }
}

// ---------------------------------------------------------------------------
// convertBlockInstr
// ---------------------------------------------------------------------------
Expect<void> Converter::convertBlockInstr(Node N, AST::InstrVec &Instrs) {
  auto Type = nodeType(N);

  OpCode Code;
  if (Type == "block_block"sv) {
    Code = OpCode::Block;
  } else if (Type == "block_loop"sv) {
    Code = OpCode::Loop;
  } else if (Type == "block_if"sv) {
    Code = OpCode::If;
  } else if (Type == "block_try_table"sv) {
    Code = OpCode::Try_table;
  } else {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  // Parse optional label identifier and push to label stack
  std::string Label;
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "identifier"sv) {
      Label = std::string(nodeText(Child));
      break;
    }
  }
  Syms.Labels.push_back(Label);

  // Create the block instruction
  Instrs.emplace_back(Code);
  auto BlockIdx = Instrs.size() - 1;

  // Parse block signature
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "block_sig"sv) {
      // Parse the block signature
      bool HasTypeUse = false;
      std::vector<ValType> ResultTypes;
      bool HasParam = false;

      for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
        Node SChild = Child.namedChild(J);
        auto SType = nodeType(SChild);
        if (SType == "type_use"sv) {
          HasTypeUse = true;
          if (SChild.namedChildCount() > 0) {
            EXPECTED_TRY(auto Idx,
                         Syms.resolveType(nodeText(SChild.namedChild(0))));
            Instrs[BlockIdx].getBlockType().setData(Idx);
          }
        } else if (SType == "param"sv) {
          HasParam = true;
        } else if (SType == "result"sv) {
          for (uint32_t K = 0; K < SChild.namedChildCount(); ++K) {
            EXPECTED_TRY(auto VT, convertValType(SChild.namedChild(K)));
            ResultTypes.push_back(VT);
          }
        }
      }

      if (!HasTypeUse) {
        if (HasParam || ResultTypes.size() > 1) {
          // Multi-value block needs a type index — not supported in simple
          // form. Leave as empty for now (would need type section lookup).
          Instrs[BlockIdx].getBlockType().setEmpty();
        } else if (ResultTypes.size() == 1) {
          Instrs[BlockIdx].getBlockType().setData(ResultTypes[0]);
        } else {
          Instrs[BlockIdx].getBlockType().setEmpty();
        }
      }
      break;
    }
  }

  // For try_table, handle catch clauses and set up TryCatch
  if (Code == OpCode::Try_table) {
    Instrs[BlockIdx].setTryCatch();
    auto &TryDesc = Instrs[BlockIdx].getTryCatch();

    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) != "catch_clause"sv) {
        continue;
      }
      // Parse catch clause: (catch tag_idx label_idx) etc.
      AST::Instruction::CatchDescriptor Catch{};
      // Iterate children to find the catch kind and indices
      std::string_view CatchKind;
      std::vector<std::string_view> CatchArgs;
      for (uint32_t J = 0; J < Child.childCount(); ++J) {
        Node CChild = Child.child(J);
        auto CText = CChild.text(Source);
        if (CText == "("sv || CText == ")"sv) {
          continue;
        }
        if (CText == "catch"sv || CText == "catch_ref"sv ||
            CText == "catch_all"sv || CText == "catch_all_ref"sv) {
          CatchKind = CText;
        } else if (CChild.isNamed()) {
          CatchArgs.push_back(CText);
        }
      }

      if (CatchKind == "catch"sv && CatchArgs.size() >= 2) {
        Catch.IsAll = false;
        Catch.IsRef = false;
        EXPECTED_TRY(auto TagIdx, Syms.resolveTag(CatchArgs[0]));
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[1]));
        Catch.TagIndex = TagIdx;
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_ref"sv && CatchArgs.size() >= 2) {
        Catch.IsAll = false;
        Catch.IsRef = true;
        EXPECTED_TRY(auto TagIdx, Syms.resolveTag(CatchArgs[0]));
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[1]));
        Catch.TagIndex = TagIdx;
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_all"sv && CatchArgs.size() >= 1) {
        Catch.IsAll = true;
        Catch.IsRef = false;
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[0]));
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_all_ref"sv && CatchArgs.size() >= 1) {
        Catch.IsAll = true;
        Catch.IsRef = true;
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[0]));
        Catch.LabelIndex = LabelIdx;
      }

      TryDesc.Catch.push_back(Catch);
    }
  }

  // Convert body instructions
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    auto CType = nodeType(Child);
    if (CType == "identifier"sv || CType == "block_sig"sv ||
        CType == "catch_clause"sv) {
      continue;
    }
    if (CType == "plain_instr"sv || CType == "block_instr"sv ||
        CType == "block_block"sv || CType == "block_loop"sv ||
        CType == "block_if"sv || CType == "block_try_table"sv ||
        CType == "folded_instr"sv) {
      EXPECTED_TRY(convertInstr(Child, Instrs));
    }
  }

  // For if blocks, handle else branch
  if (Code == OpCode::If) {
    // Check for 'else' keyword in anonymous children
    bool HasElse = false;
    for (uint32_t I = 0; I < N.childCount(); ++I) {
      Node Child = N.child(I);
      if (Child.text(Source) == "else"sv) {
        HasElse = true;
        // Add Else instruction
        Instrs.emplace_back(OpCode::Else);
        // Continue collecting instructions after 'else'
        // The instructions after 'else' are also named children of the
        // if node. We need to identify which instructions come after else.
        // In the tree-sitter grammar, all instructions are flat children.
        // We need to re-walk and partition at the 'else' keyword.
        break;
      }
    }

    if (HasElse) {
      // Re-walk: we need to emit instructions that come after 'else'.
      // The approach: clear what we emitted after the block instruction,
      // re-emit before-else and after-else separately.
      // Actually, the simpler approach: the grammar puts all instructions
      // as named children. We need to find the position of 'else' in the
      // child list and split accordingly.

      // Remove the instructions we just added (including the Else we added)
      // and re-do properly.
      Instrs.erase(Instrs.begin() + static_cast<long>(BlockIdx + 1),
                   Instrs.end());

      // Walk all children, splitting at 'else'
      for (uint32_t I = 0; I < N.childCount(); ++I) {
        Node Child = N.child(I);
        auto CText = Child.text(Source);

        if (CText == "else"sv) {
          Instrs.emplace_back(OpCode::Else);
          continue;
        }
        if (CText == "end"sv || CText == "if"sv || CText == "("sv ||
            CText == ")"sv) {
          continue;
        }

        if (Child.isNamed()) {
          auto CType = nodeType(Child);
          if (CType == "identifier"sv || CType == "block_sig"sv) {
            continue;
          }
          if (CType == "plain_instr"sv || CType == "block_instr"sv ||
              CType == "block_block"sv || CType == "block_loop"sv ||
              CType == "block_if"sv || CType == "block_try_table"sv ||
              CType == "folded_instr"sv) {
            EXPECTED_TRY(convertInstr(Child, Instrs));
          }
        }
      }
    }
  }

  // Add End instruction
  Instrs.emplace_back(OpCode::End);

  // Pop label from stack
  Syms.Labels.pop_back();

  return {};
}

// ---------------------------------------------------------------------------
// convertFoldedInstr
// ---------------------------------------------------------------------------
Expect<void> Converter::convertFoldedInstr(Node N, AST::InstrVec &Instrs) {
  // folded_instr: ( choice( block..., loop..., if..., try_table..., plain_op
  // folded* ) )

  // Find the first meaningful token to determine the instruction type
  std::string_view FirstToken;
  for (uint32_t I = 0; I < N.childCount(); ++I) {
    Node Child = N.child(I);
    auto CText = Child.text(Source);
    if (CText == "("sv || CText == ")"sv) {
      continue;
    }
    FirstToken = CText;
    break;
  }

  // Block-like folded instructions
  if (FirstToken == "block"sv || FirstToken == "loop"sv) {
    OpCode Code = (FirstToken == "block"sv) ? OpCode::Block : OpCode::Loop;

    // Parse optional label
    std::string Label;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "identifier"sv) {
        Label = std::string(nodeText(Child));
        break;
      }
    }
    Syms.Labels.push_back(Label);

    Instrs.emplace_back(Code);
    auto BlockIdx = Instrs.size() - 1;

    // Parse block signature
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "block_sig"sv) {
        bool HasTypeUse = false;
        std::vector<ValType> ResultTypes;
        bool HasParam = false;
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node SChild = Child.namedChild(J);
          auto SType = nodeType(SChild);
          if (SType == "type_use"sv) {
            HasTypeUse = true;
            if (SChild.namedChildCount() > 0) {
              EXPECTED_TRY(auto Idx,
                           Syms.resolveType(nodeText(SChild.namedChild(0))));
              Instrs[BlockIdx].getBlockType().setData(Idx);
            }
          } else if (SType == "param"sv) {
            HasParam = true;
          } else if (SType == "result"sv) {
            for (uint32_t K = 0; K < SChild.namedChildCount(); ++K) {
              EXPECTED_TRY(auto VT, convertValType(SChild.namedChild(K)));
              ResultTypes.push_back(VT);
            }
          }
        }
        if (!HasTypeUse) {
          if (HasParam || ResultTypes.size() > 1) {
            Instrs[BlockIdx].getBlockType().setEmpty();
          } else if (ResultTypes.size() == 1) {
            Instrs[BlockIdx].getBlockType().setData(ResultTypes[0]);
          } else {
            Instrs[BlockIdx].getBlockType().setEmpty();
          }
        }
        break;
      }
    }

    // Convert body instructions
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if (CType == "identifier"sv || CType == "block_sig"sv) {
        continue;
      }
      if (CType == "plain_instr"sv || CType == "block_instr"sv ||
          CType == "block_block"sv || CType == "block_loop"sv ||
          CType == "block_if"sv || CType == "block_try_table"sv ||
          CType == "folded_instr"sv) {
        EXPECTED_TRY(convertInstr(Child, Instrs));
      }
    }

    Instrs.emplace_back(OpCode::End);
    Syms.Labels.pop_back();
    return {};
  }

  if (FirstToken == "if"sv) {
    // (if label? sig? folded_instr* (then instr*) (else instr*)?)
    std::string Label;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "identifier"sv) {
        Label = std::string(nodeText(Child));
        break;
      }
    }

    // First, emit condition instructions (folded_instr children before 'then')
    // Folded instrs that come before the (then ...) block are conditions
    bool SeenThen = false;
    for (uint32_t I = 0; I < N.childCount(); ++I) {
      Node Child = N.child(I);
      auto CText = Child.text(Source);
      if (CText == "then"sv) {
        SeenThen = true;
        break;
      }
      if (Child.isNamed() && nodeType(Child) == "folded_instr"sv && !SeenThen) {
        EXPECTED_TRY(convertFoldedInstr(Child, Instrs));
      }
    }

    Syms.Labels.push_back(Label);
    Instrs.emplace_back(OpCode::If);
    auto BlockIdx = Instrs.size() - 1;

    // Parse block signature
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "block_sig"sv) {
        bool HasTypeUse = false;
        std::vector<ValType> ResultTypes;
        bool HasParam = false;
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node SChild = Child.namedChild(J);
          auto SType = nodeType(SChild);
          if (SType == "type_use"sv) {
            HasTypeUse = true;
            if (SChild.namedChildCount() > 0) {
              EXPECTED_TRY(auto Idx,
                           Syms.resolveType(nodeText(SChild.namedChild(0))));
              Instrs[BlockIdx].getBlockType().setData(Idx);
            }
          } else if (SType == "param"sv) {
            HasParam = true;
          } else if (SType == "result"sv) {
            for (uint32_t K = 0; K < SChild.namedChildCount(); ++K) {
              EXPECTED_TRY(auto VT, convertValType(SChild.namedChild(K)));
              ResultTypes.push_back(VT);
            }
          }
        }
        if (!HasTypeUse) {
          if (HasParam || ResultTypes.size() > 1) {
            Instrs[BlockIdx].getBlockType().setEmpty();
          } else if (ResultTypes.size() == 1) {
            Instrs[BlockIdx].getBlockType().setData(ResultTypes[0]);
          }
        }
        break;
      }
    }

    // Convert 'then' body and optional 'else' body
    // Walk children looking for (then ...) and (else ...)
    bool InThen = false;
    bool InElse = false;

    for (uint32_t I = 0; I < N.childCount(); ++I) {
      Node Child = N.child(I);
      auto CText = Child.text(Source);

      if (CText == "then"sv) {
        InThen = true;
        InElse = false;
        continue;
      }
      if (CText == "else"sv) {
        InThen = false;
        InElse = true;
        Instrs.emplace_back(OpCode::Else);
        continue;
      }

      if ((InThen || InElse) && Child.isNamed()) {
        auto CType = nodeType(Child);
        if (CType == "plain_instr"sv || CType == "block_instr"sv ||
            CType == "block_block"sv || CType == "block_loop"sv ||
            CType == "block_if"sv || CType == "block_try_table"sv ||
            CType == "folded_instr"sv) {
          EXPECTED_TRY(convertInstr(Child, Instrs));
        }
      }
    }

    Instrs.emplace_back(OpCode::End);
    Syms.Labels.pop_back();
    return {};
  }

  if (FirstToken == "try_table"sv) {
    std::string Label;
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "identifier"sv) {
        Label = std::string(nodeText(Child));
        break;
      }
    }
    Syms.Labels.push_back(Label);

    Instrs.emplace_back(OpCode::Try_table);
    auto BlockIdx = Instrs.size() - 1;

    // Parse block signature (same as other blocks)
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) == "block_sig"sv) {
        std::vector<ValType> ResultTypes;
        bool HasTypeUse = false;
        bool HasParam = false;
        for (uint32_t J = 0; J < Child.namedChildCount(); ++J) {
          Node SChild = Child.namedChild(J);
          auto SType = nodeType(SChild);
          if (SType == "type_use"sv) {
            HasTypeUse = true;
            if (SChild.namedChildCount() > 0) {
              EXPECTED_TRY(auto Idx,
                           Syms.resolveType(nodeText(SChild.namedChild(0))));
              Instrs[BlockIdx].getBlockType().setData(Idx);
            }
          } else if (SType == "param"sv) {
            HasParam = true;
          } else if (SType == "result"sv) {
            for (uint32_t K = 0; K < SChild.namedChildCount(); ++K) {
              EXPECTED_TRY(auto VT, convertValType(SChild.namedChild(K)));
              ResultTypes.push_back(VT);
            }
          }
        }
        if (!HasTypeUse) {
          if (HasParam || ResultTypes.size() > 1) {
            Instrs[BlockIdx].getBlockType().setEmpty();
          } else if (ResultTypes.size() == 1) {
            Instrs[BlockIdx].getBlockType().setData(ResultTypes[0]);
          }
        }
        break;
      }
    }

    // Parse catch clauses
    Instrs[BlockIdx].setTryCatch();
    auto &TryDesc = Instrs[BlockIdx].getTryCatch();

    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      if (nodeType(Child) != "catch_clause"sv) {
        continue;
      }
      AST::Instruction::CatchDescriptor Catch{};
      std::string_view CatchKind;
      std::vector<std::string_view> CatchArgs;
      for (uint32_t J = 0; J < Child.childCount(); ++J) {
        Node CChild = Child.child(J);
        auto CText = CChild.text(Source);
        if (CText == "("sv || CText == ")"sv) {
          continue;
        }
        if (CText == "catch"sv || CText == "catch_ref"sv ||
            CText == "catch_all"sv || CText == "catch_all_ref"sv) {
          CatchKind = CText;
        } else if (CChild.isNamed()) {
          CatchArgs.push_back(CText);
        }
      }

      if (CatchKind == "catch"sv && CatchArgs.size() >= 2) {
        Catch.IsAll = false;
        Catch.IsRef = false;
        EXPECTED_TRY(auto TagIdx, Syms.resolveTag(CatchArgs[0]));
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[1]));
        Catch.TagIndex = TagIdx;
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_ref"sv && CatchArgs.size() >= 2) {
        Catch.IsAll = false;
        Catch.IsRef = true;
        EXPECTED_TRY(auto TagIdx, Syms.resolveTag(CatchArgs[0]));
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[1]));
        Catch.TagIndex = TagIdx;
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_all"sv && CatchArgs.size() >= 1) {
        Catch.IsAll = true;
        Catch.IsRef = false;
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[0]));
        Catch.LabelIndex = LabelIdx;
      } else if (CatchKind == "catch_all_ref"sv && CatchArgs.size() >= 1) {
        Catch.IsAll = true;
        Catch.IsRef = true;
        EXPECTED_TRY(auto LabelIdx, Syms.resolveLabel(CatchArgs[0]));
        Catch.LabelIndex = LabelIdx;
      }

      TryDesc.Catch.push_back(Catch);
    }

    // Convert body instructions (skip catch clauses)
    for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
      Node Child = N.namedChild(I);
      auto CType = nodeType(Child);
      if (CType == "identifier"sv || CType == "block_sig"sv ||
          CType == "catch_clause"sv) {
        continue;
      }
      if (CType == "plain_instr"sv || CType == "block_instr"sv ||
          CType == "block_block"sv || CType == "block_loop"sv ||
          CType == "block_if"sv || CType == "block_try_table"sv ||
          CType == "folded_instr"sv) {
        EXPECTED_TRY(convertInstr(Child, Instrs));
      }
    }

    Instrs.emplace_back(OpCode::End);
    Syms.Labels.pop_back();
    return {};
  }

  // Plain folded instruction: (op folded_instr*)
  // First, recursively convert all folded_instr children (operands)
  for (uint32_t I = 0; I < N.namedChildCount(); ++I) {
    Node Child = N.namedChild(I);
    if (nodeType(Child) == "folded_instr"sv) {
      EXPECTED_TRY(convertFoldedInstr(Child, Instrs));
    }
  }

  // Then emit the instruction itself.
  // Build a temporary plain_instr-like node from N (excluding folded children).
  // The opcode is in the anonymous children, and the named non-folded children
  // are the immediates.
  // We can call convertPlainInstr on N directly since it reads the opcode
  // from the first child token and named children for immediates.
  return convertPlainInstr(N, Instrs);
}

} // namespace WasmEdge::WAT
