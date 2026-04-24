// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "common/errcode.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <cstring>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge::WAT {

namespace {

enum class SimdShape { I8x16, I16x8, I32x4, I64x2, F32x4, F64x2 };
static inline constexpr uint32_t SimdShapeLanes(SimdShape Shape) {
  switch (Shape) {
  case SimdShape::I8x16:
    return 16;
  case SimdShape::I16x8:
    return 8;
  case SimdShape::I32x4:
  case SimdShape::F32x4:
    return 4;
  case SimdShape::I64x2:
  case SimdShape::F64x2:
    return 2;
  default:
    assumingUnreachable();
  }
}

/// Map instruction keyword text to OpCode using enum.inc macros.
static Expect<OpCode> keywordToOpCode(std::string_view Keyword) {
  static const std::unordered_map<std::string_view, OpCode, Hash::Hash> Map = {
#define UseOpCode
#define Line(NAME, STRING, HEX) {STRING##sv, OpCode::NAME},
#define Line_FB(NAME, STRING, B1, B2) {STRING##sv, OpCode::NAME},
#define Line_FC(NAME, STRING, B1, B2) {STRING##sv, OpCode::NAME},
#define Line_FD(NAME, STRING, B1, B2) {STRING##sv, OpCode::NAME},
#define Line_FE(NAME, STRING, B1, B2) {STRING##sv, OpCode::NAME},
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
      // Manual entries: enum.inc strings contain spaces, won't match WAT
      // keywords
      {"ref.test"sv, OpCode::Ref__test},
      {"ref.cast"sv, OpCode::Ref__cast},
  };
  auto It = Map.find(Keyword);
  if (unlikely(It == Map.end())) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  return It->second;
}

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

static uint8_t maxLaneForOp(OpCode Code) {
  switch (Code) {
  case OpCode::V128__load8_lane:
  case OpCode::V128__store8_lane:
    return 16;
  case OpCode::V128__load16_lane:
  case OpCode::V128__store16_lane:
    return 8;
  case OpCode::V128__load32_lane:
  case OpCode::V128__store32_lane:
    return 4;
  case OpCode::V128__load64_lane:
  case OpCode::V128__store64_lane:
    return 2;
  default:
    return 255;
  }
}

// Check if opcode is a SIMD lane load/store that also needs a lane byte.
static inline constexpr bool isSimdLaneMemOp(OpCode Code) {
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

static Expect<uint64_t> parseOffset(std::string_view Text) {
  return parseUint(Text).map_error(
      [](auto) -> ErrCode { return ErrCode::Value::WatUnknownOperator; });
}

static Expect<uint64_t> checkOffsetU32(uint64_t Off, bool Enforce) {
  if (Enforce &&
      Off > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())) {
    return Unexpect(ErrCode::Value::WatI32Constant);
  }
  return Off;
}

static Expect<uint32_t> parseAlign(std::string_view Text) {
  EXPECTED_TRY(auto Val, parseUint(Text).map_error([](auto) -> ErrCode {
    return ErrCode::Value::WatUnknownOperator;
  }));
  if (Val == 0 || (Val & (Val - 1)) != 0) {
    return Unexpect(ErrCode::Value::WatAlignmentPow2);
  }
  uint32_t Log2 = 0;
  while (Val > 1) {
    Val >>= 1;
    ++Log2;
  }
  return Log2;
}

} // namespace

// expr ::= instr*  (appends implicit end)
Expect<void> Converter::convertExpression(Cursor &C, AST::Expression &Expr) {
  auto &Instrs = Expr.getInstrs();
  EXPECTED_TRY(convertMixedInstrSeq(C, Instrs));
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);
  return {};
}

// instr ::= plaininstr | blockinstr | foldedplaininstr | foldedblockinstr
// Processes a mixed sequence of flat (keyword) and folded (sexpr) instructions.
Expect<void> Converter::convertMixedInstrSeq(Cursor &C, AST::InstrVec &Instrs) {
  // C iterates named children. They are keywords (flat instructions),
  // numbers/ids (operands), and sexprs (folded instructions or structural).
  // We process both folded (sexpr) and flat (keyword + operand tokens) forms.
  //
  // Block tracking for flat instructions:
  std::vector<uint32_t> BlockStack;

  while (C.valid()) {
    Node Child = C.node();
    if (auto Type = nodeType(Child); Type == NodeType::Sexpr) {
      EXPECTED_TRY(convertFoldedInstr(Child, Instrs));
      C.next();
    } else if (Type == NodeType::Keyword) {
      EXPECTED_TRY(const auto Code, keywordToOpCode(nodeText(Child)));
      C.next();
      // Handle flat block instructions
      switch (Code) {
      case OpCode::Block:
      case OpCode::Loop: {
        // Optional label: peek at next child
        std::string_view Label;
        if (auto Imm = C.node(); nodeType(Imm) == NodeType::Id) {
          Label = nodeText(Imm);
          C.next();
        }
        Syms.pushLabel(Label);
        Instrs.emplace_back(Code);
        BlockStack.push_back(Instrs.size() - 1);
        EXPECTED_TRY(Instrs.back().getBlockType(), parseBlockType(C));
        break;
      }
      case OpCode::If: {
        std::string_view Label;
        if (auto Imm = C.node(); nodeType(Imm) == NodeType::Id) {
          Label = nodeText(Imm);
          C.next();
        }
        Syms.pushLabel(Label);
        Instrs.emplace_back(OpCode::If);
        BlockStack.push_back(Instrs.size() - 1);
        EXPECTED_TRY(Instrs.back().getBlockType(), parseBlockType(C));
        break;
      }
      case OpCode::Else: {
        if (auto Imm = C.node(); Syms.isIndexOrId(Imm)) {
          if (auto Idx = Syms.resolveLabel(nodeText(Imm)); !Idx || *Idx != 0) {
            return Unexpect(ErrCode::Value::WatMismatchingLabel);
          }
          C.next();
        }
        Instrs.emplace_back(OpCode::Else);
        if (!BlockStack.empty()) {
          const auto BlockIdx = BlockStack.back();
          if (Instrs[BlockIdx].getOpCode() == OpCode::If) {
            Instrs[BlockIdx].setJumpElse(Instrs.size() - 1 - BlockIdx);
          }
        }
        break;
      }
      case OpCode::End: {
        if (auto Imm = C.node(); Syms.isIndexOrId(Imm)) {
          if (auto Idx = Syms.resolveLabel(nodeText(Imm)); !Idx || *Idx != 0) {
            return Unexpect(ErrCode::Value::WatMismatchingLabel);
          }
          C.next();
        }
        Instrs.emplace_back(OpCode::End);
        if (!BlockStack.empty()) {
          Instrs.back().setExprLast(false);
          const auto BlockIdx = BlockStack.back();
          if (const auto BackOp = Instrs[BlockIdx].getOpCode();
              BackOp == OpCode::Block || BackOp == OpCode::Loop ||
              BackOp == OpCode::If) {
            Instrs.back().setTryBlockLast(false);
            Instrs[BlockIdx].setJumpEnd(Instrs.size() - 1 - BlockIdx);
            if (BackOp == OpCode::If) {
              if (Instrs[BlockIdx].getJumpElse() == 0) {
                Instrs[BlockIdx].setJumpElse(Instrs.size() - 1 - BlockIdx);
              } else {
                uint32_t ElseIdx = BlockIdx + Instrs[BlockIdx].getJumpElse();
                Instrs[ElseIdx].setJumpEnd(Instrs.size() - 1 - ElseIdx);
              }
            }
          } else if (BackOp == OpCode::Try_table) {
            Instrs.back().setTryBlockLast(true);
            Instrs[BlockIdx].getTryCatch().JumpEnd =
                Instrs.size() - 1 - BlockIdx;
          }
          BlockStack.pop_back();
          Syms.popLabel();
        } else {
          Instrs.back().setExprLast(true);
        }
        break;
      }
      default: {
        EXPECTED_TRY(auto Instr, convertPlainInstr(C, Code, Child.startByte()));
        Instrs.push_back(std::move(Instr));
        break;
      }
      }
    } else {
      break;
    }
  }
  return {};
}

// plaininstr ::= keyword immediate*
// Shared dispatch: both flat and folded paths call this with appropriate
// Cursor. C points to the first immediate after the opcode keyword.
Expect<AST::Instruction> Converter::convertPlainInstr(Cursor &C, OpCode Code,
                                                      uint32_t Offset) {
  switch (Code) {
  // --- select ---
  case OpCode::Select:
  case OpCode::Select_t:
    return convertSelectOp(C, Offset);

  // --- Const ops ---
  case OpCode::I32__const:
  case OpCode::I64__const:
  case OpCode::F32__const:
  case OpCode::F64__const:
    return convertConstOp(C, Code, Offset);

  // --- Variable ops ---
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    return convertVarOp(C, Code, Offset);

  // --- Branch ops ---
  case OpCode::Br:
  case OpCode::Br_if:
  case OpCode::Br_on_null:
  case OpCode::Br_on_non_null:
  case OpCode::Br_table:
    return convertBranchOp(C, Code, Offset);

  // --- Call ops ---
  case OpCode::Call:
  case OpCode::Return_call:
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect:
  case OpCode::Call_ref:
  case OpCode::Return_call_ref:
    return convertCallOp(C, Code, Offset);

  // --- Ref ops ---
  case OpCode::Ref__test:
  case OpCode::Ref__cast:
  case OpCode::Ref__null:
  case OpCode::Ref__func:
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail:
    return convertRefOp(C, Code, Offset);

  // --- Table ops ---
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__size:
  case OpCode::Table__grow:
  case OpCode::Table__fill:
  case OpCode::Table__copy:
  case OpCode::Table__init:
  case OpCode::Elem__drop:
    return convertTableOp(C, Code, Offset);

  // --- Memory control ---
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
  case OpCode::Memory__fill:
  case OpCode::Memory__copy:
  case OpCode::Memory__init:
  case OpCode::Data__drop:
    return convertMemControlOp(C, Code, Offset);

  // --- GC ops ---
  case OpCode::Struct__new:
  case OpCode::Struct__new_default:
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u:
  case OpCode::Array__set:
  case OpCode::Array__fill:
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set:
  case OpCode::Array__new_fixed:
  case OpCode::Array__new_elem:
  case OpCode::Array__new_data:
  case OpCode::Array__init_data:
  case OpCode::Array__init_elem:
  case OpCode::Array__copy:
  case OpCode::Throw:
    return convertGCOp(C, Code, Offset);

  // --- SIMD const ops ---
  case OpCode::V128__const:
  case OpCode::I8x16__shuffle:
    return convertSimdConstOp(C, Code, Offset);

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
  case OpCode::I64__atomic__rmw32__cmpxchg_u:
    return convertMemLoadStoreOp(C, Code, Offset);

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
  case OpCode::F64x2__replace_lane:
    return convertSimdLaneOp(C, Code, Offset);

  // --- Default: no immediates ---
  default:
    return AST::Instruction{Code, Offset};
  }
}

// --- convertSelectOp ---
// select | select (result valtype+)
Expect<AST::Instruction> Converter::convertSelectOp(Cursor &C,
                                                    uint32_t Offset) {
  std::vector<ValType> ResultTypes;
  // Consume consecutive (result ...) sexprs
  for (; C.valid(); C.next()) {
    Node Child = C.node();
    if (nodeType(Child) != NodeType::Sexpr) {
      break;
    }
    Cursor RC(Child);
    auto KW = RC.node();
    if (nodeType(KW) != NodeType::Keyword || nodeText(KW) != "result"sv) {
      break;
    }
    for (RC.next(); RC.valid(); RC.next()) {
      EXPECTED_TRY(auto VT, convertValType(RC.node()));
      ResultTypes.push_back(VT);
    }
  }
  if (ResultTypes.empty()) {
    return AST::Instruction{OpCode::Select, Offset};
  } else {
    AST::Instruction Instr{OpCode::Select_t, Offset};
    Instr.setValTypeListSize(static_cast<uint32_t>(ResultTypes.size()));
    std::copy(ResultTypes.begin(), ResultTypes.end(),
              Instr.getValTypeList().begin());
    return Instr;
  }
}

// --- convertConstOp ---
// i32.const i32 | i64.const i64 | f32.const f32 | f64.const f64
Expect<AST::Instruction> Converter::convertConstOp(Cursor &C, OpCode Code,
                                                   uint32_t Offset) {
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto Child = C.node();
  C.next();
  auto Type = nodeType(Child);
  if (Type == WasmEdge::WAT::NodeType::Keyword) {
    if (nodeText(Child) == "nan:canonical"sv ||
        nodeText(Child) == "nan:arithmetic"sv) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }

  switch (Code) {
  case OpCode::I32__const: {
    if (Type != NodeType::U && Type != NodeType::S) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    EXPECTED_TRY(auto Val, parseInt(nodeText(Child)));
    if (Val < static_cast<int64_t>(std::numeric_limits<int32_t>::min()) ||
        Val > static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
      return Unexpect(ErrCode::Value::WatConstantOutOfRange);
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(static_cast<uint32_t>(static_cast<int32_t>(Val))));
    return Instr;
  }
  case OpCode::I64__const: {
    if (Type != NodeType::U && Type != NodeType::S) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    EXPECTED_TRY(auto Val, parseInt(nodeText(Child)));
    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(static_cast<uint64_t>(Val)));
    return Instr;
  }
  case OpCode::F32__const: {
    if (Type != NodeType::U && Type != NodeType::S && Type != NodeType::F) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    EXPECTED_TRY(auto Val, parseF32(nodeText(Child)));
    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  case OpCode::F64__const: {
    if (Type != NodeType::U && Type != NodeType::S && Type != NodeType::F) {
      return Unexpect(ErrCode::Value::WatUnknownOperator);
    }
    EXPECTED_TRY(auto Val, parseF64(nodeText(Child)));
    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertVarOp ---
// local.get localidx | local.set localidx | local.tee localidx
// global.get globalidx | global.set globalidx
Expect<AST::Instruction> Converter::convertVarOp(Cursor &C, OpCode Code,
                                                 uint32_t Offset) {
  switch (Code) {
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee: {
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    auto Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx,
                 Syms.resolve(SymbolTable::IndexSpace::Local, nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  case OpCode::Global__get:
  case OpCode::Global__set: {
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    auto Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx,
                 Syms.resolve(SymbolTable::IndexSpace::Global, nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertBranchOp ---
// br labelidx | br_if labelidx | br_on_null labelidx | br_on_non_null labelidx
// br_table labelidx+ labelidx
Expect<AST::Instruction> Converter::convertBranchOp(Cursor &C, OpCode Code,
                                                    uint32_t Offset) {
  switch (Code) {
  // br labelidx | br_if labelidx | br_on_null labelidx
  case OpCode::Br:
  case OpCode::Br_if:
  case OpCode::Br_on_null:
  case OpCode::Br_on_non_null: {
    Node Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getJump().TargetIndex = Idx;
    return Instr;
  }
  // br_table labelidx+
  case OpCode::Br_table: {
    std::vector<uint32_t> Labels;
    for (; C.valid(); C.next()) {
      Node Imm = C.node();
      if (!Syms.isIndexOrId(Imm)) {
        break;
      }
      EXPECTED_TRY(auto Idx, Syms.resolveLabel(nodeText(Imm)));
      Labels.push_back(Idx);
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setLabelListSize(static_cast<uint32_t>(Labels.size()));
    auto List = Instr.getLabelList();
    for (uint32_t I = 0; I < Labels.size(); ++I) {
      List[I].TargetIndex = Labels[I];
    }
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertCallOp ---
// call funcidx | return_call funcidx
// call_indirect tableidx? typeuse | return_call_indirect tableidx? typeuse
// call_ref typeidx | return_call_ref typeidx
Expect<AST::Instruction> Converter::convertCallOp(Cursor &C, OpCode Code,
                                                  uint32_t Offset) {
  switch (Code) {
  case OpCode::Call:
  case OpCode::Return_call: {
    Node Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx,
                 Syms.resolve(SymbolTable::IndexSpace::Func, nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect: {
    uint32_t TableIdx = 0;
    if (auto Imm = C.node(); Syms.isIndexOrId(Imm)) {
      EXPECTED_TRY(TableIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Table, nodeText(Imm)));
      C.next();
    }
    EXPECTED_TRY(auto TypeIdx,
                 resolveTypeUse(C, *CurMod, true, /*AcceptParamId=*/false));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TypeIdx;
    Instr.getSourceIndex() = TableIdx;
    return Instr;
  }
  case OpCode::Call_ref:
  case OpCode::Return_call_ref: {
    Node Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertRefOp ---
// ref.test heaptype | ref.cast heaptype
// ref.null heaptype | ref.func funcidx | ref.i31
// br_on_cast labelidx reftype reftype
// br_on_cast_fail labelidx reftype reftype
Expect<AST::Instruction> Converter::convertRefOp(Cursor &C, OpCode Code,
                                                 uint32_t Offset) {
  switch (Code) {
  // ref.test reftype
  case OpCode::Ref__test: {
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(auto VT, convertRefType(Imm));
    if (VT.getCode() != TypeCode::Ref) {
      Code = OpCode::Ref__test_null;
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setValType(VT);
    return Instr;
  }
  // ref.cast reftype
  case OpCode::Ref__cast: {
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(auto VT, convertRefType(Imm));
    if (VT.getCode() != TypeCode::Ref) {
      Code = OpCode::Ref__cast_null;
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setValType(VT);
    return Instr;
  }
  // ref.null heaptype
  case OpCode::Ref__null: {
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(auto VT, convertHeapType(Imm));
    AST::Instruction Instr{Code, Offset};
    Instr.setValType(VT);
    return Instr;
  }
  // ref.func funcidx
  case OpCode::Ref__func: {
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto FuncIdx,
                 Syms.resolve(SymbolTable::IndexSpace::Func, nodeText(Imm)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = FuncIdx;
    return Instr;
  }
  // br_on_cast labelidx reftype reftype
  // br_on_cast_fail labelidx reftype reftype
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail: {
    auto Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(uint32_t LabelIdx, Syms.resolveLabel(nodeText(Imm)));
    C.next();

    EXPECTED_TRY(ValType RT1, convertRefType(C.node()));
    C.next();

    EXPECTED_TRY(ValType RT2, convertRefType(C.node()));
    C.next();

    AST::Instruction Instr{Code, Offset};
    Instr.setBrCast(LabelIdx);
    Instr.getBrCast().RType1 = RT1;
    Instr.getBrCast().RType2 = RT2;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertTableOp ---
// table.get tableidx? | table.set tableidx? | table.size tableidx?
// table.grow tableidx? | table.fill tableidx?
// table.copy tableidx? tableidx? | table.init tableidx? elemidx
// elem.drop elemidx
Expect<AST::Instruction> Converter::convertTableOp(Cursor &C, OpCode Code,
                                                   uint32_t Offset) {
  switch (Code) {
  // table.{get,set,size,grow,fill} tableidx?
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__size:
  case OpCode::Table__grow:
  case OpCode::Table__fill: {
    uint32_t TableIdx = 0;
    Node Imm = C.node();
    if (Syms.isIndexOrId(Imm)) {
      EXPECTED_TRY(TableIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Table, nodeText(Imm)));
      C.next();
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TableIdx;
    return Instr;
  }
  // table.copy tableidx? tableidx?
  case OpCode::Table__copy: {
    uint32_t DstIdx = 0, SrcIdx = 0;
    Node Imm0 = C.node();
    if (Syms.isIndexOrId(Imm0)) {
      EXPECTED_TRY(
          DstIdx, Syms.resolve(SymbolTable::IndexSpace::Table, nodeText(Imm0)));
      C.next();
      Node Imm1 = C.node();
      if (Syms.isIndexOrId(Imm1)) {
        EXPECTED_TRY(SrcIdx, Syms.resolve(SymbolTable::IndexSpace::Table,
                                          nodeText(Imm1)));
        C.next();
      }
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = DstIdx;
    Instr.getSourceIndex() = SrcIdx;
    return Instr;
  }
  // table.init tableidx? elemidx
  case OpCode::Table__init: {
    uint32_t TableIdx = 0, ElemIdx = 0;
    Node Imm0 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    C.next();
    Node Imm1 = C.node();
    if (Syms.isIndexOrId(Imm1)) {
      EXPECTED_TRY(TableIdx, Syms.resolve(SymbolTable::IndexSpace::Table,
                                          nodeText(Imm0)));
      EXPECTED_TRY(ElemIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Elem, nodeText(Imm1)));
      C.next();
    } else {
      EXPECTED_TRY(ElemIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Elem, nodeText(Imm0)));
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getSourceIndex() = ElemIdx;
    Instr.getTargetIndex() = TableIdx;
    return Instr;
  }
  // elem.drop elemidx
  case OpCode::Elem__drop: {
    uint32_t ElemIdx = 0;
    Node Imm = C.node();
    C.next();
    if (auto Type = nodeType(Imm);
        Type != NodeType::Id && Type != NodeType::U) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(ElemIdx,
                 Syms.resolve(SymbolTable::IndexSpace::Elem, nodeText(Imm)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = ElemIdx;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertMemControlOp ---
// memory.size memidx? | memory.grow memidx? | memory.fill memidx?
// memory.copy memidx? memidx? | memory.init memidx? dataidx
// data.drop dataidx
Expect<AST::Instruction> Converter::convertMemControlOp(Cursor &C, OpCode Code,
                                                        uint32_t Offset) {
  switch (Code) {
  // memory.{size,grow,fill} memidx?
  case OpCode::Memory__size:
  case OpCode::Memory__grow:
  case OpCode::Memory__fill: {
    uint32_t MemIdx = 0;
    Node Imm = C.node();
    if (Syms.isIndexOrId(Imm)) {
      C.next();
      EXPECTED_TRY(
          MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory, nodeText(Imm)));
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = MemIdx;
    return Instr;
  }
  // memory.copy (memidx memidx)?
  case OpCode::Memory__copy: {
    uint32_t MemIdx0 = 0, MemIdx1 = 0;
    Node Imm0 = C.node();
    if (Syms.isIndexOrId(Imm0)) {
      EXPECTED_TRY(MemIdx0, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                         nodeText(Imm0)));
      C.next();
      Node Imm1 = C.node();
      EXPECTED_TRY(Syms.isIndexOrId(Imm1));
      EXPECTED_TRY(MemIdx1, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                         nodeText(Imm1)));
      C.next();
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = MemIdx0;
    Instr.getSourceIndex() = MemIdx1;
    return Instr;
  }
  // memory.init memidx? dataidx
  case OpCode::Memory__init: {
    uint32_t MemIdx = 0, DataIdx = 0;
    Node Imm0 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    C.next();
    Node Imm1 = C.node();
    if (Syms.isIndexOrId(Imm1)) {
      EXPECTED_TRY(MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                        nodeText(Imm0)));
      EXPECTED_TRY(DataIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Data, nodeText(Imm1)));
      C.next();
    } else {
      EXPECTED_TRY(DataIdx,
                   Syms.resolve(SymbolTable::IndexSpace::Data, nodeText(Imm0)));
    }
    AST::Instruction Instr{Code, Offset};
    Instr.getSourceIndex() = DataIdx;
    Instr.getTargetIndex() = MemIdx;
    return Instr;
  }
  // data.drop dataidx
  case OpCode::Data__drop: {
    uint32_t DataIdx = 0;
    Node Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(DataIdx,
                 Syms.resolve(SymbolTable::IndexSpace::Data, nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = DataIdx;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertGCOp ---
// struct.new typeidx | struct.new_default typeidx
// struct.get typeidx fieldidx | struct.set typeidx fieldidx
// array.new typeidx | array.new_default typeidx | array.new_fixed typeidx u32
// array.new_elem typeidx elemidx | array.new_data typeidx dataidx
// array.init_elem typeidx elemidx | array.init_data typeidx dataidx
// array.get typeidx | array.set typeidx | array.fill typeidx
// array.copy typeidx typeidx | throw tagidx
Expect<AST::Instruction> Converter::convertGCOp(Cursor &C, OpCode Code,
                                                uint32_t Offset) {
  switch (Code) {
  // {struct,array}.{new,new_default,get,get_s,get_u,set,fill} typeidx
  case OpCode::Struct__new:
  case OpCode::Struct__new_default:
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u:
  case OpCode::Array__set:
  case OpCode::Array__fill: {
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx, Syms.resolveType(nodeText(Imm)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  // struct.get{_s,_u} typeidx fieldidx | struct.set typeidx fieldidx
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set: {
    Node Imm0 = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    EXPECTED_TRY(auto TypeIdx, Syms.resolveType(nodeText(Imm0)));
    Node Imm1 = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm1));
    EXPECTED_TRY(auto FieldIdx, Syms.resolveField(TypeIdx, nodeText(Imm1)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TypeIdx;
    Instr.getSourceIndex() = FieldIdx;
    return Instr;
  }
  // array.new_fixed typeidx u32
  case OpCode::Array__new_fixed: {
    Node Imm0 = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    EXPECTED_TRY(auto TypeIdx, Syms.resolveType(nodeText(Imm0)));
    Node Imm1 = C.node();
    C.next();
    if (auto Type = nodeType(Imm1); Type != NodeType::U) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Count, parseUint(nodeText(Imm1)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TypeIdx;
    Instr.getSourceIndex() = static_cast<uint32_t>(Count);
    return Instr;
  }
  // array.new_elem typeidx elemidx | array.init_elem typeidx elemidx
  case OpCode::Array__new_elem:
  case OpCode::Array__init_elem: {
    Node Imm0 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    EXPECTED_TRY(auto TypeIdx, Syms.resolveType(nodeText(Imm0)));
    C.next();
    Node Imm1 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm1));
    EXPECTED_TRY(auto ElemIdx,
                 Syms.resolve(SymbolTable::IndexSpace::Elem, nodeText(Imm1)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TypeIdx;
    Instr.getSourceIndex() = ElemIdx;
    return Instr;
  }
  // array.new_data typeidx dataidx | array.init_data typeidx dataidx
  case OpCode::Array__new_data:
  case OpCode::Array__init_data: {
    Node Imm0 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    EXPECTED_TRY(auto TypeIdx, Syms.resolveType(nodeText(Imm0)));
    C.next();
    Node Imm1 = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm1));
    EXPECTED_TRY(auto DataIdx,
                 Syms.resolve(SymbolTable::IndexSpace::Data, nodeText(Imm1)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = TypeIdx;
    Instr.getSourceIndex() = DataIdx;
    return Instr;
  }
  // array.copy typeidx typeidx
  case OpCode::Array__copy: {
    Node Imm0 = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm0));
    EXPECTED_TRY(auto DstTypeIdx, Syms.resolveType(nodeText(Imm0)));
    Node Imm1 = C.node();
    C.next();
    EXPECTED_TRY(Syms.isIndexOrId(Imm1));
    EXPECTED_TRY(auto SrcTypeIdx, Syms.resolveType(nodeText(Imm1)));
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = DstTypeIdx;
    Instr.getSourceIndex() = SrcTypeIdx;
    return Instr;
  }
  // throw tagidx
  case OpCode::Throw: {
    Node Imm = C.node();
    EXPECTED_TRY(Syms.isIndexOrId(Imm));
    EXPECTED_TRY(auto Idx,
                 Syms.resolve(SymbolTable::IndexSpace::Tag, nodeText(Imm)));
    C.next();
    AST::Instruction Instr{Code, Offset};
    Instr.getTargetIndex() = Idx;
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// --- convertSimdConstOp ---
// v128.const shape lane+ | i8x16.shuffle laneidx{16}
Expect<AST::Instruction> Converter::convertSimdConstOp(Cursor &C, OpCode Code,
                                                       uint32_t Offset) {
  switch (Code) {
  // v128.const shape lane+
  case OpCode::V128__const: {
    // First immediate is the shape keyword (i8x16, i16x8, etc.)
    static const std::unordered_map<std::string_view, SimdShape, Hash::Hash>
        ShapeMap = {
            {"i8x16"sv, SimdShape::I8x16}, {"i16x8"sv, SimdShape::I16x8},
            {"i32x4"sv, SimdShape::I32x4}, {"i64x2"sv, SimdShape::I64x2},
            {"f32x4"sv, SimdShape::F32x4}, {"f64x2"sv, SimdShape::F64x2},
        };
    Node ShapeNode = C.node();
    C.next();
    if (auto Type = nodeType(ShapeNode); Type != NodeType::Keyword) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    auto ShapeIt = ShapeMap.find(nodeText(ShapeNode));
    if (ShapeIt == ShapeMap.end()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    const auto Shape = ShapeIt->second;
    const auto Lanes = SimdShapeLanes(Shape);

    // Remaining immediates are lane values (continue from Cursor)
    std::vector<std::string_view> Nums;
    for (uint32_t I = 0; I < Lanes; ++I) {
      if (!C.valid()) {
        return Unexpect(ErrCode::Value::WatWrongLaneCount);
      }
      Node Imm = C.node();
      C.next();
      auto CType = nodeType(Imm);
      if (CType != NodeType::U && CType != NodeType::S &&
          CType != NodeType::F) {
        return Unexpect(ErrCode::Value::WatUnknownOperator);
      }
      Nums.push_back(nodeText(Imm));
    }
    if (C.valid()) {
      if (auto Type = nodeType(C.node());
          Type == NodeType::U || Type == NodeType::S || Type == NodeType::F) {
        return Unexpect(ErrCode::Value::WatWrongLaneCount);
      } else if (Type != NodeType::Sexpr && Type != NodeType::Keyword) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }

    uint128_t Val{};
    uint8_t *Bytes = reinterpret_cast<uint8_t *>(&Val);

    switch (Shape) {
    case SimdShape::I8x16:
      assuming(Nums.size() == 16);
      for (uint32_t I = 0; I < 16; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        if (V < -128 || V > 255) {
          return Unexpect(ErrCode::Value::WatConstantOutOfRange);
        }
        Bytes[I] = static_cast<uint8_t>(V);
      }
      break;
    case SimdShape::I16x8:
      assuming(Nums.size() == 8);
      for (uint32_t I = 0; I < 8; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        if (V < -32768 || V > 65535) {
          return Unexpect(ErrCode::Value::WatConstantOutOfRange);
        }
        uint16_t U = static_cast<uint16_t>(V);
        std::memcpy(Bytes + I * 2, &U, 2);
      }
      break;
    case SimdShape::I32x4:
      assuming(Nums.size() == 4);
      for (uint32_t I = 0; I < 4; ++I) {
        EXPECTED_TRY(auto V, parseInt(Nums[I]));
        if (V < -2147483648LL || V > 4294967295LL) {
          return Unexpect(ErrCode::Value::WatConstantOutOfRange);
        }
        uint32_t U = static_cast<uint32_t>(V);
        std::memcpy(Bytes + I * 4, &U, 4);
      }
      break;
    case SimdShape::I64x2:
      assuming(Nums.size() == 2);
      for (uint32_t I = 0; I < 2; ++I) {
        if (auto R = parseInt(Nums[I])) {
          uint64_t U = static_cast<uint64_t>(*R);
          std::memcpy(Bytes + I * 8, &U, 8);
        } else if (auto R2 = parseUint(Nums[I])) {
          uint64_t U = *R2;
          std::memcpy(Bytes + I * 8, &U, 8);
        } else {
          return Unexpect(R.error());
        }
      }
      break;
    case SimdShape::F32x4:
      assuming(Nums.size() == 4);
      for (uint32_t I = 0; I < 4; ++I) {
        EXPECTED_TRY(auto V, parseF32(Nums[I]));
        std::memcpy(Bytes + I * 4, &V, 4);
      }
      break;
    case SimdShape::F64x2:
      assuming(Nums.size() == 2);
      for (uint32_t I = 0; I < 2; ++I) {
        EXPECTED_TRY(auto V, parseF64(Nums[I]));
        std::memcpy(Bytes + I * 8, &V, 8);
      }
      break;
    }

    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(Val));
    return Instr;
  }

  // i8x16.shuffle laneidx{16}
  case OpCode::I8x16__shuffle: {
    uint128_t Val{};
    uint8_t *Bytes = reinterpret_cast<uint8_t *>(&Val);
    for (uint32_t Idx = 0; Idx < 16; ++Idx) {
      if (!C.valid()) {
        return Unexpect(ErrCode::Value::WatInvalidLaneLength);
      }
      Node Imm = C.node();
      C.next();
      if (auto ImmType = nodeType(Imm);
          ImmType == NodeType::F || ImmType == NodeType::S) {
        return Unexpect(ErrCode::Value::WatI8ConstOutOfRange);
      } else if (ImmType != NodeType::U) {
        return Unexpect(ErrCode::Value::WatInvalidLaneLength);
      }
      EXPECTED_TRY(auto R, parseUint(nodeText(Imm)));
      if (R > 255) {
        return Unexpect(ErrCode::Value::WatI8ConstOutOfRange);
      }
      if (R > 31) {
        return Unexpect(ErrCode::Value::InvalidLaneIdx);
      }
      Bytes[Idx] = static_cast<uint8_t>(R);
    }
    if (C.valid()) {
      // Check for extra lane indices after the expected 16.
      if (auto Type = nodeType(C.node());
          Type == NodeType::U || Type == NodeType::S || Type == NodeType::F) {
        return Unexpect(ErrCode::Value::WatInvalidLaneLength);
      } else if (Type != NodeType::Sexpr && Type != NodeType::Keyword) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(Val));
    return Instr;
  }

  default:
    assumingUnreachable();
  }
}

// --- convertMemLoadStoreOp ---
// {i32,i64,f32,f64}.{load,store}* memidx? align=u? offset=u?
// v128.{load,store}*_lane memidx? align=u? offset=u? laneidx
Expect<AST::Instruction>
Converter::convertMemLoadStoreOp(Cursor &C, OpCode Code, uint32_t IOffset) {
  uint32_t MemIdx = 0;
  uint64_t Offset = 0;
  uint32_t Align = naturalAlign(Code);
  uint8_t Lane = 0;

  // Collect immediates: memidx?, offset=N?, align=N?, laneidx?
  std::string_view MaybeMemIdx;
  Node Imm = C.node();
  if (auto Type = nodeType(Imm); Type == NodeType::Id || Type == NodeType::U) {
    C.next();
    if (isSimdLaneMemOp(Code) && Type == NodeType::U) {
      MaybeMemIdx = nodeText(Imm);
    } else {
      EXPECTED_TRY(
          MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory, nodeText(Imm)));
    }
    Imm = C.node();
  }
  if (auto Type = nodeType(Imm); Type == NodeType::Keyword) {
    auto Text = nodeText(Imm);
    if (Text.substr(0, 7) == "offset="sv) {
      C.next();
      EXPECTED_TRY(auto Off, parseOffset(Text.substr(7)));
      EXPECTED_TRY(Offset,
                   checkOffsetU32(Off, !Conf.hasProposal(Proposal::Memory64)));
      Imm = C.node();
    }
  }
  if (auto Type = nodeType(Imm); Type == NodeType::Keyword) {
    auto Text = nodeText(Imm);
    if (Text.substr(0, 6) == "align="sv) {
      C.next();
      EXPECTED_TRY(Align, parseAlign(Text.substr(6)));
      Imm = C.node();
    }
  }
  if (isSimdLaneMemOp(Code)) {
    if (auto Type = nodeType(Imm);
        Type != NodeType::Id && Type != NodeType::U && MaybeMemIdx.empty()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    } else if (Type != NodeType::Id && Type != NodeType::U) {
      EXPECTED_TRY(Lane, parseUint(MaybeMemIdx));
      if (Lane >= maxLaneForOp(Code)) {
        return Unexpect(ErrCode::Value::InvalidLaneIdx);
      }
    } else {
      C.next();
      if (!MaybeMemIdx.empty()) {
        EXPECTED_TRY(
            MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory, MaybeMemIdx));
      }
      EXPECTED_TRY(Lane, parseUint(nodeText(Imm)));
      if (Lane >= maxLaneForOp(Code)) {
        return Unexpect(ErrCode::Value::InvalidLaneIdx);
      }
      Imm = C.node();
    }
  }

  AST::Instruction Instr{Code, IOffset};
  Instr.getMemoryOffset() = Offset;
  Instr.getMemoryAlign() = Align;
  Instr.getTargetIndex() = MemIdx;
  if (isSimdLaneMemOp(Code)) {
    Instr.getMemoryLane() = Lane;
  }
  return Instr;
}

// --- convertSimdLaneOp ---
// {i8x16,i16x8,...}.{extract_lane,replace_lane} laneidx
Expect<AST::Instruction> Converter::convertSimdLaneOp(Cursor &C, OpCode Code,
                                                      uint32_t Offset) {
  Node Imm = C.node();
  C.next();
  if (auto Type = nodeType(Imm); Type != NodeType::U) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto V, parseUint(nodeText(Imm)));
  if (V > 255) {
    return Unexpect(ErrCode::Value::WatI8ConstOutOfRange);
  }
  uint8_t MaxLane = [&]() -> uint8_t {
    switch (Code) {
    case OpCode::I8x16__extract_lane_s:
    case OpCode::I8x16__extract_lane_u:
    case OpCode::I8x16__replace_lane:
      return 16;
    case OpCode::I16x8__extract_lane_s:
    case OpCode::I16x8__extract_lane_u:
    case OpCode::I16x8__replace_lane:
      return 8;
    case OpCode::I32x4__extract_lane:
    case OpCode::I32x4__replace_lane:
    case OpCode::F32x4__extract_lane:
    case OpCode::F32x4__replace_lane:
      return 4;
    case OpCode::I64x2__extract_lane:
    case OpCode::I64x2__replace_lane:
    case OpCode::F64x2__extract_lane:
    case OpCode::F64x2__replace_lane:
      return 2;
    default:
      assumingUnreachable();
    }
  }();
  if (V >= MaxLane) {
    return Unexpect(ErrCode::Value::InvalidLaneIdx);
  }
  AST::Instruction Instr{Code, Offset};
  Instr.getMemoryLane() = static_cast<uint8_t>(V);
  return Instr;
}

// blocktype ::= (type typeidx)? (param valtype*)* (result valtype*)*
// For flat instructions: consumes type annotation sexprs from Cursor onward.
Expect<BlockType> Converter::parseBlockType(Cursor &C) {
  AST::FunctionType BlockFT;
  bool HasTypeUse = false;
  uint32_t TypeIdx = 0;
  bool HasParam = false;
  bool HasResult = false;

  while (C.valid()) {
    Node SChild = C.node();
    if (nodeType(SChild) != NodeType::Sexpr) {
      break;
    }
    Cursor FC(SChild);
    Node KChild = FC.node();
    if (nodeType(KChild) != NodeType::Keyword) {
      break;
    }
    auto KW = nodeText(FC.node());
    FC.next();
    if (KW == "type"sv) {
      if (HasParam || HasResult || !FC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      HasTypeUse = true;
      Node Imm = FC.node();
      EXPECTED_TRY(Syms.isIndexOrId(Imm));
      EXPECTED_TRY(TypeIdx, Syms.resolveType(nodeText(Imm)));
      C.next();
    } else if (KW == "param"sv) {
      if (HasResult) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      HasParam = true;
      for (; FC.valid(); FC.next()) {
        EXPECTED_TRY(auto VT, convertValType(FC.node()));
        BlockFT.getParamTypes().push_back(std::move(VT));
      }
      C.next();
    } else if (KW == "result"sv) {
      HasResult = true;
      for (; FC.valid(); FC.next()) {
        EXPECTED_TRY(auto VT, convertValType(FC.node()));
        BlockFT.getReturnTypes().push_back(std::move(VT));
      }
      C.next();
    } else {
      break; // Not a block type annotation, stop consuming
    }
  }

  BlockType BType;
  if (HasTypeUse) {
    if (HasParam || HasResult) {
      uint32_t TIdx = findOrCreateFuncType(std::move(BlockFT), *CurMod);
      if (TIdx != TypeIdx) {
        return Unexpect(ErrCode::Value::WatInlineFuncType);
      }
    }
    BType.setData(TypeIdx);
  } else if (!HasParam && BlockFT.getReturnTypes().size() <= 1) {
    if (BlockFT.getReturnTypes().size() == 1) {
      BType.setData(BlockFT.getReturnTypes()[0]);
    } else {
      BType.setEmpty();
    }
  } else {
    // Multi-value or has params: create function type
    if (CurMod) {
      uint32_t TIdx = findOrCreateFuncType(std::move(BlockFT), *CurMod);
      BType.setData(TIdx);
    }
  }
  return BType;
}

// blockinstr ::= block label blocktype instr* end id?
//              | loop  label blocktype instr* end id?
//              | if    label blocktype instr* (else id? instr*)? end id?
//              | try_table label blocktype catch* instr* end id?
// Folded form: '(' blockinstr ... ')'
Expect<void> Converter::convertBlockInstr(Cursor &C, OpCode Code,
                                          uint32_t Offset,
                                          AST::InstrVec &Instrs) {
  assuming(Code == OpCode::Block || Code == OpCode::Loop ||
           Code == OpCode::If || Code == OpCode::Try_table);
  // Parse optional label identifier (first Id child)
  std::string_view Label;
  if (auto Imm = C.node(); nodeType(Imm) == NodeType::Id) {
    Label = nodeText(Imm);
    C.next();
  }

  EXPECTED_TRY(auto BType, parseBlockType(C));

  // For folded if: first emit condition folded instructions
  if (Code == OpCode::If) {
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Sexpr) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      std::string_view ErrorKeyword;
      if (auto Res = convertFoldedInstr(Child, Instrs, &ErrorKeyword); !Res) {
        if (ErrorKeyword != "then"sv) {
          return Unexpect(Res.error());
        }
        break;
      }
    }
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
  }

  Instrs.emplace_back(Code, Offset);
  const auto BlockIdx = Instrs.size() - 1;

  // For try_table, handle catch clauses
  if (Code == OpCode::Try_table) {
    Instrs[BlockIdx].setTryCatch();
    Instrs[BlockIdx].getTryCatch().ResType = std::move(BType);
    auto &TryDesc = Instrs[BlockIdx].getTryCatch();

    while (C.valid()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Sexpr) {
        break;
      }
      Cursor FC(Child);
      Node KW = FC.node();
      if (nodeType(KW) != NodeType::Keyword) {
        break;
      }
      auto CKW = nodeText(KW);
      AST::Instruction::CatchDescriptor Catch{};
      if (CKW == "catch"sv) {
        Catch.IsAll = false;
        Catch.IsRef = false;
      } else if (CKW == "catch_ref"sv) {
        Catch.IsAll = false;
        Catch.IsRef = true;
      } else if (CKW == "catch_all"sv) {
        Catch.IsAll = true;
        Catch.IsRef = false;
      } else if (CKW == "catch_all_ref"sv) {
        Catch.IsAll = true;
        Catch.IsRef = true;
      } else {
        break;
      }
      C.next();
      FC.next();

      if (!Catch.IsAll) {
        if (!FC.valid()) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        auto Imm = FC.node();
        EXPECTED_TRY(Syms.isIndexOrId(Imm));
        EXPECTED_TRY(Catch.TagIndex,
                     Syms.resolve(SymbolTable::IndexSpace::Tag, nodeText(Imm)));
        FC.next();
      }
      if (!FC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      auto Imm = FC.node();
      EXPECTED_TRY(Syms.isIndexOrId(Imm));
      EXPECTED_TRY(Catch.LabelIndex, Syms.resolveLabel(nodeText(Imm)));
      FC.next();
      if (FC.valid()) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      TryDesc.Catch.push_back(Catch);
    }
  } else {
    Instrs[BlockIdx].getBlockType() = std::move(BType);
  }

  Syms.pushLabel(Label);

  // Convert body instructions.
  // For 'if', handle (then ...) and (else ...) sub-sexprs specially.
  if (Code == OpCode::If) {
    // then
    {
      Cursor FC(C.node());
      C.next();
      assuming(nodeText(FC.node()) == "then"sv);
      FC.next();
      EXPECTED_TRY(convertMixedInstrSeq(FC, Instrs));
    }
    for (; C.valid(); C.next()) {
      Node Child = C.node();
      if (nodeType(Child) != NodeType::Sexpr) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      std::string_view ErrorKeyword;
      if (auto Res = convertFoldedInstr(Child, Instrs, &ErrorKeyword); !Res) {
        if (ErrorKeyword != "else"sv) {
          return Unexpect(Res.error());
        }
        Cursor FC(Child);
        FC.next();
        Instrs.emplace_back(OpCode::Else);
        Instrs[BlockIdx].setJumpElse(Instrs.size() - 1 - BlockIdx);
        EXPECTED_TRY(convertMixedInstrSeq(FC, Instrs));
      }
    }
  } else {
    EXPECTED_TRY(convertMixedInstrSeq(C, Instrs));
  }

  Instrs.emplace_back(OpCode::End);
  Syms.popLabel();
  if (Code == OpCode::Block || Code == OpCode::Loop || Code == OpCode::If) {
    Instrs.back().setTryBlockLast(false);
    Instrs[BlockIdx].setJumpEnd(Instrs.size() - 1 - BlockIdx);
    if (Code == OpCode::If) {
      if (Instrs[BlockIdx].getJumpElse() == 0) {
        Instrs[BlockIdx].setJumpElse(Instrs.size() - 1 - BlockIdx);
      } else {
        uint32_t ElseIdx = BlockIdx + Instrs[BlockIdx].getJumpElse();
        Instrs[ElseIdx].setJumpEnd(Instrs.size() - 1 - ElseIdx);
      }
    }
  } else if (Code == OpCode::Try_table) {
    Instrs.back().setTryBlockLast(true);
    Instrs[BlockIdx].getTryCatch().JumpEnd = Instrs.size() - 1 - BlockIdx;
  }
  return {};
}

// foldedinstr ::= ( plaininstr foldedinstr* )
//               | ( blockinstr )
// Recursively converts nested sub-instructions, then emits this instruction.
Expect<void> Converter::convertFoldedInstr(Node N, AST::InstrVec &Instrs,
                                           std::string_view *ErrorKeyword) {
  Cursor C(N);
  auto KWNode = C.node();
  if (nodeType(KWNode) != NodeType::Keyword) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto KW = nodeText(KWNode);

  // Structural keywords appearing where an instruction is expected
  static const std::unordered_set<std::string_view, Hash::Hash>
      StructuralKeywords = {
          "local"sv,     "param"sv,     "result"sv,        "type"sv, "catch"sv,
          "catch_ref"sv, "catch_all"sv, "catch_all_ref"sv, "then"sv, "else"sv,
      };
  if (StructuralKeywords.count(KW) > 0) {
    if (ErrorKeyword) {
      *ErrorKeyword = KW;
    }
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }

  // parse this instruction itself
  EXPECTED_TRY(const auto Code, keywordToOpCode(KW));

  C.next();

  // Block-like folded instructions
  if (Code == OpCode::Block || Code == OpCode::Loop || Code == OpCode::If ||
      Code == OpCode::Try_table) {
    return convertBlockInstr(C, Code, KWNode.startByte(), Instrs);
  }

  EXPECTED_TRY(auto Instr, convertPlainInstr(C, Code, KWNode.startByte()));
  // Plain folded instruction: (op nested-instrs... immediates...)
  for (; C.valid(); C.next()) {
    auto Child = C.node();
    if (auto Type = nodeType(Child); Type != NodeType::Sexpr) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(convertFoldedInstr(Child, Instrs));
  }
  Instrs.push_back(std::move(Instr));
  return {};
}

} // namespace WasmEdge::WAT
