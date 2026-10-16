// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/errcode.h"
#include "converter.h"
#include "wat/wat_util.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std::string_view_literals;

namespace WasmEdge {
namespace WAT {

namespace {

// Parse a lane index. The text format holds the index as a u8, and the index
// must be less than the count Lanes.
Expect<uint8_t> parseLaneIdx(std::string_view Text, uint32_t Lanes) {
  EXPECTED_TRY(auto Val, parseUint(Text));
  if (Val > std::numeric_limits<uint8_t>::max()) {
    return Unexpect(ErrCode::Value::WatI8ConstOutOfRange);
  }
  if (Val >= Lanes) {
    return Unexpect(ErrCode::Value::InvalidLaneIdx);
  }
  return static_cast<uint8_t>(Val);
}

enum class SimdShape { I8x16, I16x8, I32x4, I64x2, F32x4, F64x2 };
constexpr uint32_t simdShapeLanes(SimdShape Shape) {
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

/// Map the text of an instruction keyword to an OpCode. The map comes from the
/// enum.inc macros.
Expect<OpCode> keywordToOpCode(std::string_view Keyword) {
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
      // These entries are manual. The enum.inc strings contain spaces, so they
      // do not match the keywords.
      {"ref.test"sv, OpCode::Ref__test},
      {"ref.cast"sv, OpCode::Ref__cast},
  };
  auto It = Map.find(Keyword);
  if (unlikely(It == Map.end())) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  return It->second;
}

uint32_t naturalAlign(OpCode Code) {
  switch (Code) {
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
  case OpCode::V128__load8_lane:
  case OpCode::V128__store8_lane:
    return 0;

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
  case OpCode::V128__load16_lane:
  case OpCode::V128__store16_lane:
    return 1;

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
  case OpCode::V128__load32_lane:
  case OpCode::V128__store32_lane:
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
  case OpCode::V128__load64_lane:
  case OpCode::V128__store64_lane:
    return 3;

  case OpCode::V128__load:
  case OpCode::V128__store:
    return 4;

  default:
    return 0;
  }
}

// Give the lane count of a SIMD lane load or store, or 0 for any other
// instruction.
constexpr uint32_t laneMemOpLanes(OpCode Code) {
  switch (Code) {
  case OpCode::V128__load8_lane:
  case OpCode::V128__store8_lane:
    return simdShapeLanes(SimdShape::I8x16);
  case OpCode::V128__load16_lane:
  case OpCode::V128__store16_lane:
    return simdShapeLanes(SimdShape::I16x8);
  case OpCode::V128__load32_lane:
  case OpCode::V128__store32_lane:
    return simdShapeLanes(SimdShape::I32x4);
  case OpCode::V128__load64_lane:
  case OpCode::V128__store64_lane:
    return simdShapeLanes(SimdShape::I64x2);
  default:
    return 0;
  }
}

// This is a SIMD lane load or store. It also needs a lane byte immediate.
constexpr bool isSimdLaneMemOp(OpCode Code) {
  return laneMemOpLanes(Code) != 0;
}

// Give the shape of a SIMD extract_lane or replace_lane instruction.
SimdShape laneOpShape(OpCode Code) {
  switch (Code) {
  case OpCode::I8x16__extract_lane_s:
  case OpCode::I8x16__extract_lane_u:
  case OpCode::I8x16__replace_lane:
    return SimdShape::I8x16;
  case OpCode::I16x8__extract_lane_s:
  case OpCode::I16x8__extract_lane_u:
  case OpCode::I16x8__replace_lane:
    return SimdShape::I16x8;
  case OpCode::I32x4__extract_lane:
  case OpCode::I32x4__replace_lane:
    return SimdShape::I32x4;
  case OpCode::F32x4__extract_lane:
  case OpCode::F32x4__replace_lane:
    return SimdShape::F32x4;
  case OpCode::I64x2__extract_lane:
  case OpCode::I64x2__replace_lane:
    return SimdShape::I64x2;
  case OpCode::F64x2__extract_lane:
  case OpCode::F64x2__replace_lane:
    return SimdShape::F64x2;
  default:
    assumingUnreachable();
  }
}

// Return true if Text starts with Prefix.
bool startsWith(std::string_view Text, std::string_view Prefix) {
  return Text.substr(0, Prefix.size()) == Prefix;
}

// The numeral after offset= and align= comes in one keyword token, so the
// lexer does not examine it. The numeral must follow the lexical grammar of
// an unsigned integer, as a number token does.
Expect<uint64_t> parseMemArgNum(std::string_view Text) {
  if (!isUnsignedLiteral(Text)) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  return parseUint(Text).map_error(
      [](auto) -> ErrCode { return ErrCode::Value::WatUnknownOperator; });
}

Expect<uint32_t> parseAlign(std::string_view Text) {
  EXPECTED_TRY(auto Val, parseMemArgNum(Text));
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

// Parse the lanes Nums of a v128.const into Val with Parse. Each lane holds
// the bits UT. parseInt accepts a signed or an unsigned lane, and
// parseFloatBits gives the bit pattern of a float lane.
template <typename UT>
Expect<void> packLanes(Span<const std::string_view> Nums, uint128_t &Val,
                       Expect<UT> (*Parse)(std::string_view)) {
  for (size_t I = 0; I < Nums.size(); ++I) {
    EXPECTED_TRY(auto Bits, Parse(Nums[I]));
    setV128Lane(Val, I, Bits);
  }
  return {};
}

// Make the instruction Code with the target index Target.
AST::Instruction makeIndexed(OpCode Code, uint32_t Offset, uint32_t Target) {
  AST::Instruction Instr{Code, Offset};
  Instr.getTargetIndex() = Target;
  return Instr;
}

// Make the instruction Code with the target index Target and the source index
// Source.
AST::Instruction makeIndexed(OpCode Code, uint32_t Offset, uint32_t Target,
                             uint32_t Source) {
  AST::Instruction Instr = makeIndexed(Code, Offset, Target);
  Instr.getSourceIndex() = Source;
  return Instr;
}

} // namespace

void appendExprEnd(AST::InstrVec &Instrs) {
  Instrs.emplace_back(OpCode::End);
  Instrs.back().setExprLast(true);
}

// expr ::= instr*
// The function appends the implicit end.
Expect<void> Converter::convertExpression(Cursor &C, AST::Expression &Expr) {
  auto &Instrs = Expr.getInstrs();
  EXPECTED_TRY(convertMixedInstrSeq(C, Instrs));
  appendExprEnd(Instrs);
  return {};
}

// Parse the catch clauses of a try_table. The cursor C points at the first
// clause, and the function consumes the clauses. The clauses come before the
// body and resolve their label targets in the outer scope. As a result, the
// caller pushes the label of the block after this call.
Expect<void>
Converter::parseCatchClauses(Cursor &C,
                             AST::Instruction::TryDescriptor &TryDesc) {
  while (C.valid()) {
    Node CatchChild = C.node();
    if (nodeType(CatchChild) != NodeType::Sexpr) {
      break;
    }
    Cursor FC(CatchChild);
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
      EXPECTED_TRY(Catch.TagIndex, takeIdx(FC, SymbolTable::IndexSpace::Tag));
    }
    EXPECTED_TRY(Catch.LabelIndex, takeLabelIdx(FC));
    EXPECTED_TRY(expectEnd(FC));
    TryDesc.Catch.push_back(Catch);
  }
  return {};
}

// Close the block at BlockIdx with the End instruction at the back of Instrs.
// The function writes the jump offsets of the block, of its else, and of its
// end.
void Converter::closeBlock(AST::InstrVec &Instrs, uint32_t BlockIdx) {
  const auto EndIdx = static_cast<uint32_t>(Instrs.size() - 1);
  Instrs.back().setExprLast(false);
  const auto Code = Instrs[BlockIdx].getOpCode();
  if (Code == OpCode::Try_table) {
    Instrs.back().setTryBlockLast(true);
    Instrs[BlockIdx].getTryCatch().JumpEnd = EndIdx - BlockIdx;
    return;
  }
  Instrs.back().setTryBlockLast(false);
  Instrs[BlockIdx].setJumpEnd(EndIdx - BlockIdx);
  if (Code == OpCode::If) {
    if (Instrs[BlockIdx].getJumpElse() == 0) {
      Instrs[BlockIdx].setJumpElse(EndIdx - BlockIdx);
    } else {
      const uint32_t ElseIdx = BlockIdx + Instrs[BlockIdx].getJumpElse();
      Instrs[ElseIdx].setJumpEnd(EndIdx - ElseIdx);
    }
  }
}

// The frame of the explicit stack of the instruction converter. The stack
// replaces the native recursion, so the depth of the folded instructions has
// no bound, as in the binary loader.
struct Converter::InstrFrame {
  enum class Kind {
    // A flat instruction sequence. The folded instructions in it get a frame
    // of their own.
    Seq,
    // A folded plain instruction. The operands come first, then the
    // instruction.
    Folded,
    // A folded block instruction.
    Block,
  };
  enum class Stage {
    Start,
    Operands,  // Folded: the folded operands.
    Cond,      // Block: the folded condition of an if.
    Body,      // Block: the body of a block, loop, or try_table.
    AfterThen, // Block: the position after the (then ...) clause.
    AfterElse, // Block: the position after the (else ...) clause.
  };

  Kind K;
  Stage S = Stage::Start;
  // The remaining children. For Seq, the sequence. For Folded and Block, the
  // children after the keyword.
  Cursor C;
  Node N;                                  // Folded and Block: the sexpr.
  std::vector<uint32_t> FlatBlocks;        // Seq: the open flat blocks.
  std::optional<AST::Instruction> Pending; // Folded: the instruction.
  OpCode Code = OpCode::Unreachable;       // Block: the opcode.
  uint32_t Offset = 0;    // Block: the byte offset of the keyword.
  uint32_t BlockIdx = 0;  // Block: the index of the instruction.
  std::string_view Label; // Block: the label.

  InstrFrame(Kind Kind, Cursor &&Cur) : K(Kind), C(std::move(Cur)) {}
  InstrFrame(Kind Kind, Node Sexpr) : K(Kind), C(Sexpr), N(Sexpr) {}
};

// instr ::= plaininstr | blockinstr | foldedplaininstr | foldedblockinstr
// This function converts the two forms. The flat form is a keyword with
// operand tokens. The folded form is a sexpr. The function consumes the whole
// sequence, so a caller never sees a token that is left over.
Expect<void> Converter::convertMixedInstrSeq(Cursor &C, AST::InstrVec &Instrs) {
  std::vector<InstrFrame> Stack;
  Stack.emplace_back(InstrFrame::Kind::Seq, C.copy());
  EXPECTED_TRY(runInstrFrames(Stack, Instrs));
  while (C.valid()) {
    C.next();
  }
  return {};
}

// foldedinstr ::= ( plaininstr foldedinstr* )
//               | ( blockinstr )
// This function converts one folded instruction. The nested sub-instructions
// come first. Then it writes this instruction.
Expect<void> Converter::convertFoldedInstr(Node N, AST::InstrVec &Instrs) {
  std::vector<InstrFrame> Stack;
  Stack.emplace_back(InstrFrame::Kind::Folded, N);
  return runInstrFrames(Stack, Instrs);
}

// Run the frames of the instruction converter until the stack is empty.
Expect<void> Converter::runInstrFrames(std::vector<InstrFrame> &Stack,
                                       AST::InstrVec &Instrs) {
  using Kind = InstrFrame::Kind;
  using Stage = InstrFrame::Stage;
  // These structural keywords are at a position that needs an instruction.
  static constexpr std::string_view StructuralKeywords[] = {
      "local"sv, "param"sv,     "result"sv,    "type"sv,
      "catch"sv, "catch_ref"sv, "catch_all"sv, "catch_all_ref"sv,
      "then"sv,  "else"sv,      "end"sv,
  };

  while (!Stack.empty()) {
    auto &F = Stack.back();
    switch (F.K) {
    case Kind::Seq: {
      if (!F.C.valid()) {
        // An open block that stays shows that a block, loop, if, or try_table
        // has no 'end'.
        if (!F.FlatBlocks.empty()) {
          return Unexpect(ErrCode::Value::WatUnexpectedEnd);
        }
        Stack.pop_back();
        break;
      }
      Node Child = F.C.node();
      const auto Type = nodeType(Child);
      if (Type == NodeType::Sexpr) {
        F.C.next();
        // The push can move the frames, so the code does not touch F after
        // it.
        Stack.emplace_back(Kind::Folded, Child);
        break;
      }
      if (Type != NodeType::Keyword) {
        // A token that is not an instruction is malformed here.
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      EXPECTED_TRY(convertFlatInstr(F, Child, Instrs));
      break;
    }
    case Kind::Folded: {
      if (F.S == Stage::Start) {
        auto KWNode = F.C.node();
        if (nodeType(KWNode) != NodeType::Keyword) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        auto KW = nodeText(KWNode);
        if (std::find(std::begin(StructuralKeywords),
                      std::end(StructuralKeywords),
                      KW) != std::end(StructuralKeywords)) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        EXPECTED_TRY(const auto Code, keywordToOpCode(KW));
        F.C.next();
        if (Code == OpCode::Block || Code == OpCode::Loop ||
            Code == OpCode::If || Code == OpCode::Try_table) {
          // This folded instruction is a block. The frame changes its kind.
          EXPECTED_TRY(checkInstrProposal(Code));
          F.K = Kind::Block;
          F.Code = Code;
          F.Offset = KWNode.startByte();
          break;
        }
        EXPECTED_TRY(auto Instr,
                     convertPlainInstr(F.C, Code, KWNode.startByte()));
        F.Pending.emplace(std::move(Instr));
        F.S = Stage::Operands;
        break;
      }
      // This is a plain folded instruction: (op immediates... operands...).
      // The operands are folded instructions, and they come before the
      // instruction.
      if (F.C.valid()) {
        Node Child = F.C.node();
        if (nodeType(Child) != NodeType::Sexpr) {
          return Unexpect(ErrCode::Value::WatUnexpectedToken);
        }
        F.C.next();
        Stack.emplace_back(Kind::Folded, Child);
        break;
      }
      Instrs.push_back(std::move(*F.Pending));
      Stack.pop_back();
      break;
    }
    case Kind::Block:
      EXPECTED_TRY(convertFoldedBlock(Stack, Instrs));
      break;
    }
  }
  return {};
}

// blockinstr ::= block label blocktype instr* end id?
//              | loop  label blocktype instr* end id?
//              | if    label blocktype instr* (else id? instr*)? end id?
//              | try_table label blocktype catch* instr* end id?
// Convert one flat instruction of the sequence frame F. The keyword is
// Child, and F.C points after it. A block instruction opens a flat block,
// and 'end' closes it.
Expect<void> Converter::convertFlatInstr(InstrFrame &F, Node Child,
                                         AST::InstrVec &Instrs) {
  auto &C = F.C;
  auto &BlockStack = F.FlatBlocks;
  EXPECTED_TRY(const auto Code, keywordToOpCode(nodeText(Child)));
  C.next();
  switch (Code) {
  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If: {
    EXPECTED_TRY(Syms.pushLabel(takeOptionalId(C)));
    Instrs.emplace_back(Code, Child.startByte());
    BlockStack.push_back(static_cast<uint32_t>(Instrs.size() - 1));
    EXPECTED_TRY(Instrs.back().getBlockType(), parseBlockType(C));
    return {};
  }
  case OpCode::Else: {
    // An 'else' is valid only if the innermost open block is an 'if', and
    // only if that 'if' has no 'else' yet.
    if (BlockStack.empty() ||
        Instrs[BlockStack.back()].getOpCode() != OpCode::If ||
        Instrs[BlockStack.back()].getJumpElse() != 0) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    // The grammar is else id?, so a numeric label is not an option.
    EXPECTED_TRY(checkClosingLabel(C));
    Instrs.emplace_back(OpCode::Else, Child.startByte());
    const auto BlockIdx = BlockStack.back();
    Instrs[BlockIdx].setJumpElse(
        static_cast<uint32_t>(Instrs.size() - 1 - BlockIdx));
    return {};
  }
  case OpCode::End: {
    // An 'end' closes the innermost open block. The end of the function body
    // is implicit in the text format, so an 'end' with no open block is
    // malformed. If the converter accepts it, the expression gets a second
    // final End, and the validator pops an empty control stack.
    if (BlockStack.empty()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    // The grammar is end id?, so a numeric label is not an option.
    EXPECTED_TRY(checkClosingLabel(C));
    Instrs.emplace_back(OpCode::End, Child.startByte());
    closeBlock(Instrs, BlockStack.back());
    BlockStack.pop_back();
    Syms.popLabel();
    return {};
  }
  case OpCode::Try_table: {
    // A flat try_table has the form label? blocktype catch* ... end. The
    // code for End completes it.
    EXPECTED_TRY(checkInstrProposal(Code));
    const auto Label = takeOptionalId(C);
    EXPECTED_TRY(auto BType, parseBlockType(C));
    Instrs.emplace_back(Code, Child.startByte());
    const auto BlockIdx = static_cast<uint32_t>(Instrs.size() - 1);
    Instrs[BlockIdx].setTryCatch();
    Instrs[BlockIdx].getTryCatch().ResType = std::move(BType);
    EXPECTED_TRY(parseCatchClauses(C, Instrs[BlockIdx].getTryCatch()));
    EXPECTED_TRY(Syms.pushLabel(Label));
    BlockStack.push_back(BlockIdx);
    return {};
  }
  default: {
    EXPECTED_TRY(auto Instr, convertPlainInstr(C, Code, Child.startByte()));
    Instrs.push_back(std::move(Instr));
    return {};
  }
  }
}

// The optional label after else or end. The label must name the innermost
// open block.
Expect<void> Converter::checkClosingLabel(Cursor &C) {
  if (peekType(C) == NodeType::Id) {
    if (auto Idx = Syms.resolveLabel(nodeText(C.node())); !Idx || *Idx != 0) {
      return Unexpect(ErrCode::Value::WatMismatchingLabel);
    }
    C.next();
  }
  return {};
}

// Folded form: ( blockinstr ... )
// Advance the folded block frame at the back of Stack by one step. For an
// if, the folded condition comes first, then the (then ...) clause, then the
// optional (else ...) clause. Nothing can come after the clauses. For the
// other blocks, the body is the rest of the sexpr.
Expect<void> Converter::convertFoldedBlock(std::vector<InstrFrame> &Stack,
                                           AST::InstrVec &Instrs) {
  using Kind = InstrFrame::Kind;
  using Stage = InstrFrame::Stage;
  auto &F = Stack.back();
  switch (F.S) {
  case Stage::Start: {
    F.Label = takeOptionalId(F.C);
    EXPECTED_TRY(auto BType, parseBlockType(F.C));
    if (F.Code == OpCode::If) {
      // The if instruction comes after its folded condition. Keep the block
      // type until then.
      F.Pending.emplace(OpCode::If, F.Offset);
      F.Pending->getBlockType() = std::move(BType);
      F.S = Stage::Cond;
      return {};
    }
    Instrs.emplace_back(F.Code, F.Offset);
    F.BlockIdx = static_cast<uint32_t>(Instrs.size() - 1);
    if (F.Code == OpCode::Try_table) {
      Instrs[F.BlockIdx].setTryCatch();
      Instrs[F.BlockIdx].getTryCatch().ResType = std::move(BType);
      EXPECTED_TRY(parseCatchClauses(F.C, Instrs[F.BlockIdx].getTryCatch()));
    } else {
      Instrs[F.BlockIdx].getBlockType() = std::move(BType);
    }
    EXPECTED_TRY(Syms.pushLabel(F.Label));
    F.S = Stage::Body;
    // The body takes the rest of the children.
    Cursor Body = std::move(F.C);
    Stack.emplace_back(Kind::Seq, std::move(Body));
    return {};
  }
  case Stage::Cond: {
    if (!F.C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Node Child = F.C.node();
    if (nodeType(Child) != NodeType::Sexpr) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    F.C.next();
    if (!sexprMatch(Child, "then"sv)) {
      // This is a folded operand of the condition.
      Stack.emplace_back(Kind::Folded, Child);
      return {};
    }
    // This is the (then ...) clause. Write the if instruction first.
    Instrs.push_back(std::move(*F.Pending));
    F.Pending.reset();
    F.BlockIdx = static_cast<uint32_t>(Instrs.size() - 1);
    EXPECTED_TRY(Syms.pushLabel(F.Label));
    F.S = Stage::AfterThen;
    Cursor Then(Child);
    Then.next(); // Skip the "then" keyword.
    Stack.emplace_back(Kind::Seq, std::move(Then));
    return {};
  }
  case Stage::AfterThen: {
    // This is the optional (else ...) clause. The grammar permits at most
    // one, and nothing can come after it. An instruction at this position is
    // malformed, and the converter must not splice it into a branch.
    if (F.C.valid() && sexprMatch(F.C.node(), "else"sv)) {
      Node Child = F.C.node();
      F.C.next();
      Instrs.emplace_back(OpCode::Else, Child.startByte());
      Instrs[F.BlockIdx].setJumpElse(
          static_cast<uint32_t>(Instrs.size() - 1 - F.BlockIdx));
      F.S = Stage::AfterElse;
      Cursor Else(Child);
      Else.next(); // Skip the "else" keyword.
      Stack.emplace_back(Kind::Seq, std::move(Else));
      return {};
    }
    [[fallthrough]];
  }
  case Stage::AfterElse:
    if (F.C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    [[fallthrough]];
  case Stage::Body:
    // The closing parenthesis of the block sexpr is the implicit end.
    Instrs.emplace_back(OpCode::End, F.N.endByte() - 1);
    Syms.popLabel();
    closeBlock(Instrs, F.BlockIdx);
    Stack.pop_back();
    return {};
  case Stage::Operands:
    break;
  }
  assumingUnreachable();
}

// plaininstr ::= keyword immediate*
// The flat path and the folded path both use this function. C points to the
// first immediate.
Expect<AST::Instruction> Converter::convertPlainInstr(Cursor &C, OpCode Code,
                                                      uint32_t Offset) {
  EXPECTED_TRY(auto Instr, dispatchPlainInstr(C, Code, Offset));
  // Test the final opcode, because select and ref.test can change it.
  EXPECTED_TRY(checkInstrProposal(Instr.getOpCode()));
  return Instr;
}

// Convert the immediates of the plain instruction Code with its category
// function.
Expect<AST::Instruction> Converter::dispatchPlainInstr(Cursor &C, OpCode Code,
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
    return convertGCOp(C, Code, Offset);

  // --- throw tagidx ---
  case OpCode::Throw: {
    EXPECTED_TRY(auto Idx, takeIdx(C, SymbolTable::IndexSpace::Tag));
    return makeIndexed(Code, Offset, Idx);
  }

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

// select | select (result valtype+)
Expect<AST::Instruction> Converter::convertSelectOp(Cursor &C,
                                                    uint32_t Offset) {
  std::vector<ValType> ResultTypes;
  // Consume consecutive (result ...) sexprs. A (result) with no type is
  // still a typed select. The validator then rejects the arity, as it does
  // for the binary form.
  bool HasResult = false;
  for (; sexprMatch(C, "result"sv); C.next()) {
    HasResult = true;
    Cursor RC(C.node());
    RC.next(); // Skip the "result" keyword.
    EXPECTED_TRY(parseResultList(RC, ResultTypes));
  }
  if (!HasResult) {
    return AST::Instruction{OpCode::Select, Offset};
  }
  AST::Instruction Instr{OpCode::Select_t, Offset};
  Instr.setValTypeListSize(static_cast<uint32_t>(ResultTypes.size()));
  std::copy(ResultTypes.begin(), ResultTypes.end(),
            Instr.getValTypeList().begin());
  return Instr;
}

// i32.const i32 | i64.const i64 | f32.const f32 | f64.const f64
Expect<AST::Instruction> Converter::convertConstOp(Cursor &C, OpCode Code,
                                                   uint32_t Offset) {
  if (!C.valid()) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  auto Child = C.node();
  C.next();
  auto Type = nodeType(Child);
  if (Type == NodeType::Keyword) {
    if (nodeText(Child) == "nan:canonical"sv ||
        nodeText(Child) == "nan:arithmetic"sv) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }
  const bool IsFloat = Code == OpCode::F32__const || Code == OpCode::F64__const;
  if (Type != NodeType::U && Type != NodeType::S &&
      !(IsFloat && Type == NodeType::F)) {
    return Unexpect(ErrCode::Value::WatUnknownOperator);
  }

  AST::Instruction Instr{Code, Offset};
  const auto Text = nodeText(Child);
  switch (Code) {
  case OpCode::I32__const: {
    EXPECTED_TRY(auto Val, parseInt<uint32_t>(Text));
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  case OpCode::I64__const: {
    EXPECTED_TRY(auto Val, parseInt<uint64_t>(Text));
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  case OpCode::F32__const: {
    EXPECTED_TRY(auto Val, parseF32(Text));
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  case OpCode::F64__const: {
    EXPECTED_TRY(auto Val, parseF64(Text));
    Instr.setNum(ValVariant(Val));
    return Instr;
  }
  default:
    assumingUnreachable();
  }
}

// local.get localidx | local.set localidx | local.tee localidx
// global.get globalidx | global.set globalidx
Expect<AST::Instruction> Converter::convertVarOp(Cursor &C, OpCode Code,
                                                 uint32_t Offset) {
  const auto Space =
      (Code == OpCode::Global__get || Code == OpCode::Global__set)
          ? SymbolTable::IndexSpace::Global
          : SymbolTable::IndexSpace::Local;
  EXPECTED_TRY(auto Idx, takeIdx(C, Space));
  return makeIndexed(Code, Offset, Idx);
}

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
    EXPECTED_TRY(auto Idx, takeLabelIdx(C));
    AST::Instruction Instr{Code, Offset};
    Instr.getJump().TargetIndex = Idx;
    return Instr;
  }
  // br_table labelidx+
  case OpCode::Br_table: {
    std::vector<uint32_t> Labels;
    while (isIndexOrId(C.node())) {
      EXPECTED_TRY(auto Idx, takeLabelIdx(C));
      Labels.push_back(Idx);
    }
    // The grammar needs one label or more. The last label is the default
    // target, and the validator reads it from the end of the list.
    if (Labels.empty()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
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

// call funcidx | return_call funcidx
// call_indirect tableidx? typeuse | return_call_indirect tableidx? typeuse
// call_ref typeidx | return_call_ref typeidx
Expect<AST::Instruction> Converter::convertCallOp(Cursor &C, OpCode Code,
                                                  uint32_t Offset) {
  switch (Code) {
  case OpCode::Call:
  case OpCode::Return_call: {
    EXPECTED_TRY(auto Idx, takeIdx(C, SymbolTable::IndexSpace::Func));
    return makeIndexed(Code, Offset, Idx);
  }
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect: {
    uint32_t TableIdx = 0;
    EXPECTED_TRY(takeOptIdx(C, SymbolTable::IndexSpace::Table, TableIdx));
    EXPECTED_TRY(auto TypeIdx,
                 resolveTypeUse(C, *CurMod, /*AcceptParamId=*/false));
    // The binary format holds a zero byte for the table before the Reference
    // Types proposal.
    if (TableIdx != 0) {
      EXPECTED_TRY(needProposal(Proposal::ReferenceTypes,
                                ErrCode::Value::ExpectedZeroByte));
    }
    return makeIndexed(Code, Offset, TypeIdx, TableIdx);
  }
  case OpCode::Call_ref:
  case OpCode::Return_call_ref: {
    EXPECTED_TRY(auto Idx, takeTypeIdx(C));
    return makeIndexed(Code, Offset, Idx);
  }
  default:
    assumingUnreachable();
  }
}

// ref.test heaptype | ref.cast heaptype
// ref.null heaptype | ref.func funcidx | ref.i31
// br_on_cast labelidx reftype reftype
// br_on_cast_fail labelidx reftype reftype
Expect<AST::Instruction> Converter::convertRefOp(Cursor &C, OpCode Code,
                                                 uint32_t Offset) {
  switch (Code) {
  // ref.test reftype | ref.cast reftype
  case OpCode::Ref__test:
  case OpCode::Ref__cast: {
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(auto VT, convertRefType(Imm));
    if (VT.getCode() != TypeCode::Ref) {
      Code = Code == OpCode::Ref__test ? OpCode::Ref__test_null
                                       : OpCode::Ref__cast_null;
    }
    AST::Instruction Instr{Code, Offset};
    Instr.setValType(VT);
    return Instr;
  }
  // ref.null heaptype
  case OpCode::Ref__null: {
    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    Node Imm = C.node();
    C.next();
    EXPECTED_TRY(auto VT, convertHeapType(Imm));
    AST::Instruction Instr{Code, Offset};
    Instr.setValType(VT);
    return Instr;
  }
  // ref.func funcidx
  case OpCode::Ref__func: {
    EXPECTED_TRY(auto FuncIdx, takeIdx(C, SymbolTable::IndexSpace::Func));
    return makeIndexed(Code, Offset, FuncIdx);
  }
  // br_on_cast labelidx reftype reftype
  // br_on_cast_fail labelidx reftype reftype
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail: {
    EXPECTED_TRY(uint32_t LabelIdx, takeLabelIdx(C));

    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(ValType RT1, convertRefType(C.node()));
    C.next();

    if (!C.valid()) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
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
    EXPECTED_TRY(takeOptIdx(C, SymbolTable::IndexSpace::Table, TableIdx));
    return makeIndexed(Code, Offset, TableIdx);
  }
  // table.copy (tableidx tableidx)?
  case OpCode::Table__copy: {
    EXPECTED_TRY(auto Idx, takeOptIdxPair(C, SymbolTable::IndexSpace::Table));
    const auto [DstIdx, SrcIdx] = Idx;
    return makeIndexed(Code, Offset, DstIdx, SrcIdx);
  }
  // table.init tableidx? elemidx
  case OpCode::Table__init: {
    EXPECTED_TRY(auto Idx, takeOptIdxThenIdx(C, SymbolTable::IndexSpace::Table,
                                             SymbolTable::IndexSpace::Elem));
    const auto [TableIdx, ElemIdx] = Idx;
    return makeIndexed(Code, Offset, TableIdx, ElemIdx);
  }
  // elem.drop elemidx
  case OpCode::Elem__drop: {
    EXPECTED_TRY(auto ElemIdx, takeIdx(C, SymbolTable::IndexSpace::Elem));
    return makeIndexed(Code, Offset, ElemIdx);
  }
  default:
    assumingUnreachable();
  }
}

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
    EXPECTED_TRY(takeOptIdx(C, SymbolTable::IndexSpace::Memory, MemIdx));
    EXPECTED_TRY(checkMemIdxProposal(MemIdx));
    return makeIndexed(Code, Offset, MemIdx);
  }
  // memory.copy (memidx memidx)?
  case OpCode::Memory__copy: {
    EXPECTED_TRY(auto Idx, takeOptIdxPair(C, SymbolTable::IndexSpace::Memory));
    const auto [MemIdx0, MemIdx1] = Idx;
    EXPECTED_TRY(checkMemIdxProposal(MemIdx0));
    EXPECTED_TRY(checkMemIdxProposal(MemIdx1));
    return makeIndexed(Code, Offset, MemIdx0, MemIdx1);
  }
  // memory.init memidx? dataidx
  case OpCode::Memory__init: {
    EXPECTED_TRY(auto Idx, takeOptIdxThenIdx(C, SymbolTable::IndexSpace::Memory,
                                             SymbolTable::IndexSpace::Data));
    const auto [MemIdx, DataIdx] = Idx;
    EXPECTED_TRY(checkMemIdxProposal(MemIdx));
    return makeIndexed(Code, Offset, MemIdx, DataIdx);
  }
  // data.drop dataidx
  case OpCode::Data__drop: {
    EXPECTED_TRY(auto DataIdx, takeIdx(C, SymbolTable::IndexSpace::Data));
    return makeIndexed(Code, Offset, DataIdx);
  }
  default:
    assumingUnreachable();
  }
}

// struct.new typeidx | struct.new_default typeidx
// struct.get typeidx fieldidx | struct.set typeidx fieldidx
// array.new typeidx | array.new_default typeidx | array.new_fixed typeidx u32
// array.new_elem typeidx elemidx | array.new_data typeidx dataidx
// array.init_elem typeidx elemidx | array.init_data typeidx dataidx
// array.get typeidx | array.set typeidx | array.fill typeidx
// array.copy typeidx typeidx
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
    EXPECTED_TRY(auto Idx, takeTypeIdx(C));
    return makeIndexed(Code, Offset, Idx);
  }
  // struct.get{_s,_u} typeidx fieldidx | struct.set typeidx fieldidx
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set: {
    EXPECTED_TRY(auto TypeIdx, takeTypeIdx(C));
    Node Imm1 = C.node();
    C.next();
    EXPECTED_TRY(expectIndexOrId(Imm1));
    EXPECTED_TRY(auto FieldIdx, Syms.resolveField(TypeIdx, nodeText(Imm1)));
    return makeIndexed(Code, Offset, TypeIdx, FieldIdx);
  }
  // array.new_fixed typeidx u32
  case OpCode::Array__new_fixed: {
    EXPECTED_TRY(auto TypeIdx, takeTypeIdx(C));
    Node Imm1 = C.node();
    C.next();
    if (auto Type = nodeType(Imm1); Type != NodeType::U) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    EXPECTED_TRY(auto Count, parseUint32(nodeText(Imm1)));
    return makeIndexed(Code, Offset, TypeIdx, Count);
  }
  // array.new_elem typeidx elemidx | array.init_elem typeidx elemidx
  case OpCode::Array__new_elem:
  case OpCode::Array__init_elem: {
    EXPECTED_TRY(auto TypeIdx, takeTypeIdx(C));
    EXPECTED_TRY(auto ElemIdx, takeIdx(C, SymbolTable::IndexSpace::Elem));
    return makeIndexed(Code, Offset, TypeIdx, ElemIdx);
  }
  // array.new_data typeidx dataidx | array.init_data typeidx dataidx
  case OpCode::Array__new_data:
  case OpCode::Array__init_data: {
    EXPECTED_TRY(auto TypeIdx, takeTypeIdx(C));
    EXPECTED_TRY(auto DataIdx, takeIdx(C, SymbolTable::IndexSpace::Data));
    return makeIndexed(Code, Offset, TypeIdx, DataIdx);
  }
  // array.copy typeidx typeidx
  case OpCode::Array__copy: {
    EXPECTED_TRY(auto DstTypeIdx, takeTypeIdx(C));
    EXPECTED_TRY(auto SrcTypeIdx, takeTypeIdx(C));
    return makeIndexed(Code, Offset, DstTypeIdx, SrcTypeIdx);
  }
  default:
    assumingUnreachable();
  }
}

// v128.const shape lane+ | i8x16.shuffle laneidx{16}
Expect<AST::Instruction> Converter::convertSimdConstOp(Cursor &C, OpCode Code,
                                                       uint32_t Offset) {
  switch (Code) {
  // v128.const shape lane+
  case OpCode::V128__const: {
    // The first immediate is the shape keyword, such as i8x16 or i16x8.
    static constexpr std::pair<std::string_view, SimdShape> Shapes[] = {
        {"i8x16"sv, SimdShape::I8x16}, {"i16x8"sv, SimdShape::I16x8},
        {"i32x4"sv, SimdShape::I32x4}, {"i64x2"sv, SimdShape::I64x2},
        {"f32x4"sv, SimdShape::F32x4}, {"f64x2"sv, SimdShape::F64x2},
    };
    Node ShapeNode = C.node();
    C.next();
    if (auto Type = nodeType(ShapeNode); Type != NodeType::Keyword) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    const auto FoundShape = lookupKeyword(Shapes, nodeText(ShapeNode));
    if (!FoundShape) {
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    }
    const auto Shape = *FoundShape;
    const auto Lanes = simdShapeLanes(Shape);

    // The immediates that come after are the lane values. A shape has 16
    // lanes or fewer.
    std::array<std::string_view, 16> Lexemes;
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
      Lexemes[I] = nodeText(Imm);
    }
    const Span<const std::string_view> Nums(Lexemes.data(), Lanes);
    if (C.valid()) {
      if (auto Type = nodeType(C.node());
          Type == NodeType::U || Type == NodeType::S || Type == NodeType::F) {
        return Unexpect(ErrCode::Value::WatWrongLaneCount);
      } else if (Type != NodeType::Sexpr && Type != NodeType::Keyword) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
    }

    uint128_t Val{};

    switch (Shape) {
    case SimdShape::I8x16:
      EXPECTED_TRY(packLanes(Nums, Val, &parseInt<uint8_t>));
      break;
    case SimdShape::I16x8:
      EXPECTED_TRY(packLanes(Nums, Val, &parseInt<uint16_t>));
      break;
    case SimdShape::I32x4:
      EXPECTED_TRY(packLanes(Nums, Val, &parseInt<uint32_t>));
      break;
    case SimdShape::I64x2:
      EXPECTED_TRY(packLanes(Nums, Val, &parseInt<uint64_t>));
      break;
    case SimdShape::F32x4:
      EXPECTED_TRY(packLanes(Nums, Val, &parseFloatBits<uint32_t>));
      break;
    case SimdShape::F64x2:
      EXPECTED_TRY(packLanes(Nums, Val, &parseFloatBits<uint64_t>));
      break;
    }

    AST::Instruction Instr{Code, Offset};
    Instr.setNum(ValVariant(Val));
    return Instr;
  }

  // i8x16.shuffle laneidx{16}
  case OpCode::I8x16__shuffle: {
    uint128_t Val{};
    // A lane index is an unsigned integer. A signed or a float token is not a
    // lane index. A sexpr or a keyword ends the lane indices too early.
    constexpr uint32_t Lanes = simdShapeLanes(SimdShape::I8x16);
    for (uint32_t Idx = 0; Idx < Lanes; ++Idx) {
      if (!C.valid()) {
        return Unexpect(ErrCode::Value::WatWrongLaneIndexCount);
      }
      Node Imm = C.node();
      if (auto ImmType = nodeType(Imm);
          ImmType == NodeType::F || ImmType == NodeType::S) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      } else if (ImmType == NodeType::Sexpr || ImmType == NodeType::Keyword) {
        return Unexpect(ErrCode::Value::WatWrongLaneIndexCount);
      } else if (ImmType != NodeType::U) {
        return Unexpect(ErrCode::Value::WatUnexpectedToken);
      }
      C.next();
      // A lane index selects a lane of the two operand vectors.
      EXPECTED_TRY(auto LaneIdx, parseLaneIdx(nodeText(Imm), 2 * Lanes));
      setV128Lane(Val, Idx, LaneIdx);
    }
    if (C.valid()) {
      // Examine the input for extra lane indices after the first 16.
      if (auto Type = nodeType(C.node()); Type == NodeType::U) {
        return Unexpect(ErrCode::Value::WatWrongLaneIndexCount);
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

// {i32,i64,f32,f64}.{load,store}* memidx? align=u? offset=u?
// v128.{load,store}*_lane memidx? align=u? offset=u? laneidx
Expect<AST::Instruction>
Converter::convertMemLoadStoreOp(Cursor &C, OpCode Code, uint32_t Offset) {
  uint32_t MemIdx = 0;
  uint64_t MemOffset = 0;
  uint32_t Align = naturalAlign(Code);
  uint8_t Lane = 0;

  constexpr auto OffsetPrefix = "offset="sv;
  constexpr auto AlignPrefix = "align="sv;
  const auto ParseLane = [Code](std::string_view Text) {
    return parseLaneIdx(Text, laneMemOpLanes(Code));
  };

  // Collect the immediates: memidx?, offset=N?, align=N?, and laneidx?
  // For a lane load or store, a first U token is the memory index only if a
  // lane index comes after the memarg. Otherwise, it is the lane index.
  std::string_view MemIdxOrLane;
  Node Imm = C.node();
  if (isIndexOrId(Imm)) {
    C.next();
    if (isSimdLaneMemOp(Code) && nodeType(Imm) == NodeType::U) {
      MemIdxOrLane = nodeText(Imm);
    } else {
      EXPECTED_TRY(
          MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory, nodeText(Imm)));
    }
    Imm = C.node();
  }
  // If Imm is the memarg keyword with the prefix Prefix, take it and give the
  // text of its value.
  const auto TakeMemArg =
      [&](std::string_view Prefix) -> std::optional<std::string_view> {
    if (nodeType(Imm) != NodeType::Keyword) {
      return std::nullopt;
    }
    auto Text = nodeText(Imm);
    if (!startsWith(Text, Prefix)) {
      return std::nullopt;
    }
    C.next();
    Imm = C.node();
    return Text.substr(Prefix.size());
  };
  bool HasMemArg = false;
  if (const auto Value = TakeMemArg(OffsetPrefix)) {
    HasMemArg = true;
    EXPECTED_TRY(MemOffset, parseMemArgNum(*Value));
    if (!Conf.hasProposal(Proposal::Memory64) &&
        MemOffset > std::numeric_limits<uint32_t>::max()) {
      return Unexpect(ErrCode::Value::WatI32Constant);
    }
  }
  if (const auto Value = TakeMemArg(AlignPrefix)) {
    HasMemArg = true;
    EXPECTED_TRY(Align, parseAlign(*Value));
  }
  if (isSimdLaneMemOp(Code)) {
    if (isIndexOrId(Imm)) {
      // The lane index comes after the memarg, so the first number is the
      // memory index.
      C.next();
      if (!MemIdxOrLane.empty()) {
        EXPECTED_TRY(MemIdx, Syms.resolve(SymbolTable::IndexSpace::Memory,
                                          MemIdxOrLane));
      }
      EXPECTED_TRY(Lane, ParseLane(nodeText(Imm)));
    } else if (MemIdxOrLane.empty() || HasMemArg) {
      // The lane index is missing. A number before the memarg is the memory
      // index, and it cannot be the lane index.
      return Unexpect(ErrCode::Value::WatUnexpectedToken);
    } else {
      EXPECTED_TRY(Lane, ParseLane(MemIdxOrLane));
    }
  }

  // The binary format holds the memory index in the memarg flags only with
  // the Multiple Memories proposal.
  if (MemIdx != 0) {
    EXPECTED_TRY(needProposal(Proposal::MultiMemories,
                              ErrCode::Value::MalformedMemoryOpFlags));
  }
  AST::Instruction Instr{Code, Offset};
  Instr.getMemoryOffset() = MemOffset;
  Instr.getMemoryAlign() = Align;
  Instr.getTargetIndex() = MemIdx;
  if (isSimdLaneMemOp(Code)) {
    Instr.getMemoryLane() = Lane;
  }
  return Instr;
}

// {i8x16,i16x8,...}.{extract_lane,replace_lane} laneidx
Expect<AST::Instruction> Converter::convertSimdLaneOp(Cursor &C, OpCode Code,
                                                      uint32_t Offset) {
  Node Imm = C.node();
  C.next();
  if (nodeType(Imm) != NodeType::U) {
    return Unexpect(ErrCode::Value::WatUnexpectedToken);
  }
  EXPECTED_TRY(auto Lane,
               parseLaneIdx(nodeText(Imm), simdShapeLanes(laneOpShape(Code))));
  AST::Instruction Instr{Code, Offset};
  Instr.getMemoryLane() = Lane;
  return Instr;
}

// blocktype ::= (type typeidx)? (param valtype*)* (result valtype*)*
// For a flat instruction, this function consumes the type annotation sexprs
// from the position of the cursor.
Expect<BlockType> Converter::parseBlockType(Cursor &C) {
  // A block parameter has no id.
  EXPECTED_TRY(auto Use, parseTypeUseClauses(C, /*AcceptParamId=*/false));
  auto &BlockFT = Use.Inline;

  BlockType BType;
  if (Use.TypeIdx) {
    // The type must be a function type, and an inline type must be equal to
    // it. A new type is never necessary here.
    EXPECTED_TRY(
        checkTypeUseIdx(*CurMod, *Use.TypeIdx, Use.hasInline(), BlockFT));
    BType.setData(*Use.TypeIdx);
  } else if (BlockFT.getParamTypes().empty() &&
             BlockFT.getReturnTypes().size() <= 1) {
    if (BlockFT.getReturnTypes().size() == 1) {
      BType.setData(BlockFT.getReturnTypes()[0]);
    } else {
      BType.setEmpty();
    }
    return BType;
  } else {
    // The block has parameters, or it has more than one result. Make a
    // function type.
    BType.setData(findOrCreateFuncType(std::move(BlockFT), *CurMod));
  }
  // The binary format holds a type index in a block type only with the
  // Multi-value proposal.
  EXPECTED_TRY(
      needProposal(Proposal::MultiValue, ErrCode::Value::MalformedValType));
  return BType;
}

} // namespace WAT
} // namespace WasmEdge
