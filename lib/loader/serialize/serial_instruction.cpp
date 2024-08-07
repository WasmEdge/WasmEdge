// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize instruction. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeInstruction(const AST::Instruction &Instr,
                                 std::vector<uint8_t> &OutVec) const noexcept {
  auto serializeMemImmediate = [this, &Instr, &OutVec]() -> Expect<void> {
    if (Conf.hasProposal(Proposal::MultiMemories) &&
        Instr.getMemoryAlign() < 64 && Instr.getTargetIndex() != 0) {
      serializeU32(Instr.getMemoryAlign() + 64, OutVec);
      serializeU32(Instr.getTargetIndex(), OutVec);
    } else {
      serializeU32(Instr.getMemoryAlign(), OutVec);
    }
    serializeU32(Instr.getMemoryOffset(), OutVec);
    return {};
  };

  auto serializeCheckZero = [this, &OutVec](uint32_t C) -> Expect<void> {
    if (C != 0) {
      return logSerializeError(ErrCode::Value::ExpectedZeroByte,
                               ASTNodeAttr::Instruction);
    }
    OutVec.push_back(0x00);
    return {};
  };

  // Check with proposals.
  if (auto Res = Conf.isInstrNeedProposal(Instr.getOpCode());
      unlikely(Res.has_value())) {
    return logNeedProposal(ErrCode::Value::IllegalOpCode, Res.value(),
                           ASTNodeAttr::Instruction);
  }

  // Serialize OpCode.
  switch (Instr.getOpCode()) {
#define UseOpCode
#define Line(NAME, STRING, PREFIX)                                             \
  case OpCode::NAME:                                                           \
    OutVec.push_back(static_cast<uint8_t>(PREFIX));                            \
    break;
#define Line_FB(NAME, STRING, PREFIX, EXTEND)                                  \
  case OpCode::NAME:                                                           \
    OutVec.push_back(static_cast<uint8_t>(PREFIX));                            \
    serializeU32(EXTEND, OutVec);                                              \
    break;
#define Line_FC(NAME, STRING, PREFIX, EXTEND)                                  \
  case OpCode::NAME:                                                           \
    OutVec.push_back(static_cast<uint8_t>(PREFIX));                            \
    serializeU32(EXTEND, OutVec);                                              \
    break;
#define Line_FD(NAME, STRING, PREFIX, EXTEND)                                  \
  case OpCode::NAME:                                                           \
    OutVec.push_back(static_cast<uint8_t>(PREFIX));                            \
    serializeU32(EXTEND, OutVec);                                              \
    break;
#define Line_FE(NAME, STRING, PREFIX, EXTEND)                                  \
  case OpCode::NAME:                                                           \
    OutVec.push_back(static_cast<uint8_t>(PREFIX));                            \
    serializeU32(EXTEND, OutVec);                                              \
    break;
#include "common/enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
  default:
    assumingUnreachable();
  }

  // Serialize immediate.
  switch (Instr.getOpCode()) {
  // Control instructions.
  case OpCode::Unreachable:
  case OpCode::Nop:
  case OpCode::Return:
  case OpCode::End:
  case OpCode::Else:
    return {};

  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If:
    if (Instr.getBlockType().isEmpty()) {
      OutVec.push_back(static_cast<uint8_t>(TypeCode::Epsilon));
    } else if (Instr.getBlockType().isValType()) {
      if (auto Res = serializeValType(Instr.getBlockType().getValType(),
                                      ASTNodeAttr::Instruction, OutVec);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    } else {
      if (unlikely(!Conf.hasProposal(Proposal::MultiValue))) {
        return logNeedProposal(ErrCode::Value::MalformedValType,
                               Proposal::MultiValue, ASTNodeAttr::Instruction);
      }
      serializeS33(static_cast<int64_t>(Instr.getBlockType().getTypeIndex()),
                   OutVec);
    }
    return {};

  case OpCode::Br:
  case OpCode::Br_if:
    serializeU32(Instr.getJump().TargetIndex, OutVec);
    return {};

  case OpCode::Br_table: {
    uint32_t VecCnt = static_cast<uint32_t>(Instr.getLabelList().size()) - 1;
    serializeU32(VecCnt, OutVec);
    for (auto &Label : Instr.getLabelList()) {
      serializeU32(Label.TargetIndex, OutVec);
    }
    return {};
  }

  case OpCode::Call:
  case OpCode::Return_call:
    serializeU32(Instr.getTargetIndex(), OutVec);
    return {};

  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect:
    // Serialize the type index.
    serializeU32(Instr.getTargetIndex(), OutVec);
    if (Instr.getSourceIndex() > 0 &&
        !Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                             Proposal::ReferenceTypes,
                             ASTNodeAttr::Instruction);
    }
    // Serialize the table index.
    serializeU32(Instr.getSourceIndex(), OutVec);
    return {};

  // Reference Instructions.
  case OpCode::Ref__null:
    if (auto Res = serializeRefType(Instr.getValType(),
                                    ASTNodeAttr::Instruction, OutVec);
        unlikely(!Res)) {
      return Unexpect(Res);
    }
    return {};
  case OpCode::Ref__is_null:
    return {};
  case OpCode::Ref__func:
    serializeU32(Instr.getTargetIndex(), OutVec);
    return {};

  // Parametric Instructions.
  case OpCode::Drop:
  case OpCode::Select:
    return {};
  case OpCode::Select_t: {
    uint32_t VecCnt = static_cast<uint32_t>(Instr.getValTypeList().size());
    serializeU32(VecCnt, OutVec);
    for (auto &VType : Instr.getValTypeList()) {
      if (auto Res = serializeValType(VType, ASTNodeAttr::Instruction, OutVec);
          unlikely(!Res)) {
        return Unexpect(Res);
      }
    }
    return {};
  }

  // Variable Instructions.
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    serializeU32(Instr.getTargetIndex(), OutVec);
    return {};

  // Table Instructions.
  case OpCode::Table__init:
    serializeU32(Instr.getSourceIndex(), OutVec);
    [[fallthrough]];
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  case OpCode::Elem__drop:
  case OpCode::Table__copy:
    serializeU32(Instr.getTargetIndex(), OutVec);
    return {};

  // Memory Instructions.
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
    return serializeMemImmediate();

  case OpCode::Memory__init:
    serializeU32(Instr.getTargetIndex(), OutVec);
    [[fallthrough]];
  case OpCode::Memory__grow:
  case OpCode::Memory__size:
  case OpCode::Memory__fill:
    if (Conf.hasProposal(Proposal::MultiMemories)) {
      serializeU32(Instr.getTargetIndex(), OutVec);
      return {};
    } else {
      return serializeCheckZero(Instr.getTargetIndex());
    }

  case OpCode::Memory__copy:
    if (Conf.hasProposal(Proposal::MultiMemories)) {
      serializeU32(Instr.getTargetIndex(), OutVec);
      serializeU32(Instr.getSourceIndex(), OutVec);
      return {};
    } else {
      if (auto Res = serializeCheckZero(Instr.getTargetIndex());
          unlikely(!Res)) {
        return Unexpect(Res);
      }
      return serializeCheckZero(Instr.getTargetIndex());
    }

  case OpCode::Data__drop:
    serializeU32(Instr.getTargetIndex(), OutVec);
    return {};

  // Const Instructions.
  case OpCode::I32__const:
    serializeS32(Instr.getNum().get<int32_t>(), OutVec);
    return {};

  case OpCode::I64__const:
    serializeS64(Instr.getNum().get<int64_t>(), OutVec);
    return {};
  case OpCode::F32__const:
    serializeF32(Instr.getNum().get<float>(), OutVec);
    return {};
  case OpCode::F64__const:
    serializeF64(Instr.getNum().get<double>(), OutVec);
    return {};

  // Unary Numeric Instructions.
  case OpCode::I32__eqz:
  case OpCode::I32__clz:
  case OpCode::I32__ctz:
  case OpCode::I32__popcnt:
  case OpCode::I64__eqz:
  case OpCode::I64__clz:
  case OpCode::I64__ctz:
  case OpCode::I64__popcnt:
  case OpCode::F32__abs:
  case OpCode::F32__neg:
  case OpCode::F32__ceil:
  case OpCode::F32__floor:
  case OpCode::F32__trunc:
  case OpCode::F32__nearest:
  case OpCode::F32__sqrt:
  case OpCode::F64__abs:
  case OpCode::F64__neg:
  case OpCode::F64__ceil:
  case OpCode::F64__floor:
  case OpCode::F64__trunc:
  case OpCode::F64__nearest:
  case OpCode::F64__sqrt:
  case OpCode::I32__wrap_i64:
  case OpCode::I32__trunc_f32_s:
  case OpCode::I32__trunc_f32_u:
  case OpCode::I32__trunc_f64_s:
  case OpCode::I32__trunc_f64_u:
  case OpCode::I64__extend_i32_s:
  case OpCode::I64__extend_i32_u:
  case OpCode::I64__trunc_f32_s:
  case OpCode::I64__trunc_f32_u:
  case OpCode::I64__trunc_f64_s:
  case OpCode::I64__trunc_f64_u:
  case OpCode::F32__convert_i32_s:
  case OpCode::F32__convert_i32_u:
  case OpCode::F32__convert_i64_s:
  case OpCode::F32__convert_i64_u:
  case OpCode::F32__demote_f64:
  case OpCode::F64__convert_i32_s:
  case OpCode::F64__convert_i32_u:
  case OpCode::F64__convert_i64_s:
  case OpCode::F64__convert_i64_u:
  case OpCode::F64__promote_f32:
  case OpCode::I32__reinterpret_f32:
  case OpCode::I64__reinterpret_f64:
  case OpCode::F32__reinterpret_i32:
  case OpCode::F64__reinterpret_i64:
  case OpCode::I32__extend8_s:
  case OpCode::I32__extend16_s:
  case OpCode::I64__extend8_s:
  case OpCode::I64__extend16_s:
  case OpCode::I64__extend32_s:
  case OpCode::I32__trunc_sat_f32_s:
  case OpCode::I32__trunc_sat_f32_u:
  case OpCode::I32__trunc_sat_f64_s:
  case OpCode::I32__trunc_sat_f64_u:
  case OpCode::I64__trunc_sat_f32_s:
  case OpCode::I64__trunc_sat_f32_u:
  case OpCode::I64__trunc_sat_f64_s:
  case OpCode::I64__trunc_sat_f64_u:

  // Binary Numeric Instructions.
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I32__lt_s:
  case OpCode::I32__lt_u:
  case OpCode::I32__gt_s:
  case OpCode::I32__gt_u:
  case OpCode::I32__le_s:
  case OpCode::I32__le_u:
  case OpCode::I32__ge_s:
  case OpCode::I32__ge_u:
  case OpCode::I64__eq:
  case OpCode::I64__ne:
  case OpCode::I64__lt_s:
  case OpCode::I64__lt_u:
  case OpCode::I64__gt_s:
  case OpCode::I64__gt_u:
  case OpCode::I64__le_s:
  case OpCode::I64__le_u:
  case OpCode::I64__ge_s:
  case OpCode::I64__ge_u:
  case OpCode::F32__eq:
  case OpCode::F32__ne:
  case OpCode::F32__lt:
  case OpCode::F32__gt:
  case OpCode::F32__le:
  case OpCode::F32__ge:
  case OpCode::F64__eq:
  case OpCode::F64__ne:
  case OpCode::F64__lt:
  case OpCode::F64__gt:
  case OpCode::F64__le:
  case OpCode::F64__ge:

  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I32__mul:
  case OpCode::I32__div_s:
  case OpCode::I32__div_u:
  case OpCode::I32__rem_s:
  case OpCode::I32__rem_u:
  case OpCode::I32__and:
  case OpCode::I32__or:
  case OpCode::I32__xor:
  case OpCode::I32__shl:
  case OpCode::I32__shr_s:
  case OpCode::I32__shr_u:
  case OpCode::I32__rotl:
  case OpCode::I32__rotr:
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_s:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_s:
  case OpCode::I64__rem_u:
  case OpCode::I64__and:
  case OpCode::I64__or:
  case OpCode::I64__xor:
  case OpCode::I64__shl:
  case OpCode::I64__shr_s:
  case OpCode::I64__shr_u:
  case OpCode::I64__rotl:
  case OpCode::I64__rotr:
  case OpCode::F32__add:
  case OpCode::F32__sub:
  case OpCode::F32__mul:
  case OpCode::F32__div:
  case OpCode::F32__min:
  case OpCode::F32__max:
  case OpCode::F32__copysign:
  case OpCode::F64__add:
  case OpCode::F64__sub:
  case OpCode::F64__mul:
  case OpCode::F64__div:
  case OpCode::F64__min:
  case OpCode::F64__max:
  case OpCode::F64__copysign:
    return {};

  // SIMD Memory Instruction.
  case OpCode::V128__load:
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
  case OpCode::V128__store:
    return serializeMemImmediate();
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
    if (auto Res = serializeMemImmediate(); unlikely(!Res)) {
      return Unexpect(Res);
    }
    OutVec.push_back(Instr.getMemoryLane());
    return {};

  // SIMD Const Instruction.
  case OpCode::V128__const:
  // SIMD Shuffle Instruction.
  case OpCode::I8x16__shuffle: {
    uint128_t Value = Instr.getNum().get<uint128_t>();
    const std::uint8_t *Ptr = reinterpret_cast<const uint8_t *>(&Value);
    for (uint32_t I = 0; I < 16; ++I) {
      OutVec.push_back(Ptr[15 - I]);
    }
    return {};
  }

  // SIMD Lane Instructions.
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
    OutVec.push_back(Instr.getMemoryLane());
    return {};

  // SIMD Numeric Instructions.
  case OpCode::I8x16__swizzle:
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
  case OpCode::I64x2__splat:
  case OpCode::F32x4__splat:
  case OpCode::F64x2__splat:

  case OpCode::I8x16__eq:
  case OpCode::I8x16__ne:
  case OpCode::I8x16__lt_s:
  case OpCode::I8x16__lt_u:
  case OpCode::I8x16__gt_s:
  case OpCode::I8x16__gt_u:
  case OpCode::I8x16__le_s:
  case OpCode::I8x16__le_u:
  case OpCode::I8x16__ge_s:
  case OpCode::I8x16__ge_u:

  case OpCode::I16x8__eq:
  case OpCode::I16x8__ne:
  case OpCode::I16x8__lt_s:
  case OpCode::I16x8__lt_u:
  case OpCode::I16x8__gt_s:
  case OpCode::I16x8__gt_u:
  case OpCode::I16x8__le_s:
  case OpCode::I16x8__le_u:
  case OpCode::I16x8__ge_s:
  case OpCode::I16x8__ge_u:

  case OpCode::I32x4__eq:
  case OpCode::I32x4__ne:
  case OpCode::I32x4__lt_s:
  case OpCode::I32x4__lt_u:
  case OpCode::I32x4__gt_s:
  case OpCode::I32x4__gt_u:
  case OpCode::I32x4__le_s:
  case OpCode::I32x4__le_u:
  case OpCode::I32x4__ge_s:
  case OpCode::I32x4__ge_u:

  case OpCode::F32x4__eq:
  case OpCode::F32x4__ne:
  case OpCode::F32x4__lt:
  case OpCode::F32x4__gt:
  case OpCode::F32x4__le:
  case OpCode::F32x4__ge:

  case OpCode::F64x2__eq:
  case OpCode::F64x2__ne:
  case OpCode::F64x2__lt:
  case OpCode::F64x2__gt:
  case OpCode::F64x2__le:
  case OpCode::F64x2__ge:

  case OpCode::V128__not:
  case OpCode::V128__and:
  case OpCode::V128__andnot:
  case OpCode::V128__or:
  case OpCode::V128__xor:
  case OpCode::V128__bitselect:
  case OpCode::V128__any_true:

  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I8x16__popcnt:
  case OpCode::I8x16__all_true:
  case OpCode::I8x16__bitmask:
  case OpCode::I8x16__narrow_i16x8_s:
  case OpCode::I8x16__narrow_i16x8_u:
  case OpCode::I8x16__shl:
  case OpCode::I8x16__shr_s:
  case OpCode::I8x16__shr_u:
  case OpCode::I8x16__add:
  case OpCode::I8x16__add_sat_s:
  case OpCode::I8x16__add_sat_u:
  case OpCode::I8x16__sub:
  case OpCode::I8x16__sub_sat_s:
  case OpCode::I8x16__sub_sat_u:
  case OpCode::I8x16__min_s:
  case OpCode::I8x16__min_u:
  case OpCode::I8x16__max_s:
  case OpCode::I8x16__max_u:
  case OpCode::I8x16__avgr_u:

  case OpCode::I16x8__abs:
  case OpCode::I16x8__neg:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I16x8__narrow_i32x4_s:
  case OpCode::I16x8__narrow_i32x4_u:
  case OpCode::I16x8__extend_low_i8x16_s:
  case OpCode::I16x8__extend_high_i8x16_s:
  case OpCode::I16x8__extend_low_i8x16_u:
  case OpCode::I16x8__extend_high_i8x16_u:
  case OpCode::I16x8__shl:
  case OpCode::I16x8__shr_s:
  case OpCode::I16x8__shr_u:
  case OpCode::I16x8__add:
  case OpCode::I16x8__add_sat_s:
  case OpCode::I16x8__add_sat_u:
  case OpCode::I16x8__sub:
  case OpCode::I16x8__sub_sat_s:
  case OpCode::I16x8__sub_sat_u:
  case OpCode::I16x8__mul:
  case OpCode::I16x8__min_s:
  case OpCode::I16x8__min_u:
  case OpCode::I16x8__max_s:
  case OpCode::I16x8__max_u:
  case OpCode::I16x8__avgr_u:
  case OpCode::I16x8__extmul_low_i8x16_s:
  case OpCode::I16x8__extmul_high_i8x16_s:
  case OpCode::I16x8__extmul_low_i8x16_u:
  case OpCode::I16x8__extmul_high_i8x16_u:
  case OpCode::I16x8__q15mulr_sat_s:
  case OpCode::I16x8__extadd_pairwise_i8x16_s:
  case OpCode::I16x8__extadd_pairwise_i8x16_u:

  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I32x4__extend_low_i16x8_s:
  case OpCode::I32x4__extend_high_i16x8_s:
  case OpCode::I32x4__extend_low_i16x8_u:
  case OpCode::I32x4__extend_high_i16x8_u:
  case OpCode::I32x4__shl:
  case OpCode::I32x4__shr_s:
  case OpCode::I32x4__shr_u:
  case OpCode::I32x4__add:
  case OpCode::I32x4__sub:
  case OpCode::I32x4__mul:
  case OpCode::I32x4__min_s:
  case OpCode::I32x4__min_u:
  case OpCode::I32x4__max_s:
  case OpCode::I32x4__max_u:
  case OpCode::I32x4__extmul_low_i16x8_s:
  case OpCode::I32x4__extmul_high_i16x8_s:
  case OpCode::I32x4__extmul_low_i16x8_u:
  case OpCode::I32x4__extmul_high_i16x8_u:
  case OpCode::I32x4__extadd_pairwise_i16x8_s:
  case OpCode::I32x4__extadd_pairwise_i16x8_u:

  case OpCode::I64x2__abs:
  case OpCode::I64x2__neg:
  case OpCode::I64x2__bitmask:
  case OpCode::I64x2__extend_low_i32x4_s:
  case OpCode::I64x2__extend_high_i32x4_s:
  case OpCode::I64x2__extend_low_i32x4_u:
  case OpCode::I64x2__extend_high_i32x4_u:
  case OpCode::I64x2__shl:
  case OpCode::I64x2__shr_s:
  case OpCode::I64x2__shr_u:
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:
  case OpCode::I64x2__eq:
  case OpCode::I64x2__ne:
  case OpCode::I64x2__lt_s:
  case OpCode::I64x2__gt_s:
  case OpCode::I64x2__le_s:
  case OpCode::I64x2__ge_s:
  case OpCode::I64x2__all_true:
  case OpCode::I64x2__extmul_low_i32x4_s:
  case OpCode::I64x2__extmul_high_i32x4_s:
  case OpCode::I64x2__extmul_low_i32x4_u:
  case OpCode::I64x2__extmul_high_i32x4_u:

  case OpCode::F32x4__abs:
  case OpCode::F32x4__neg:
  case OpCode::F32x4__sqrt:
  case OpCode::F32x4__add:
  case OpCode::F32x4__sub:
  case OpCode::F32x4__mul:
  case OpCode::F32x4__div:
  case OpCode::F32x4__min:
  case OpCode::F32x4__max:
  case OpCode::F32x4__pmin:
  case OpCode::F32x4__pmax:

  case OpCode::F64x2__abs:
  case OpCode::F64x2__neg:
  case OpCode::F64x2__sqrt:
  case OpCode::F64x2__add:
  case OpCode::F64x2__sub:
  case OpCode::F64x2__mul:
  case OpCode::F64x2__div:
  case OpCode::F64x2__min:
  case OpCode::F64x2__max:
  case OpCode::F64x2__pmin:
  case OpCode::F64x2__pmax:

  case OpCode::I32x4__trunc_sat_f32x4_s:
  case OpCode::I32x4__trunc_sat_f32x4_u:
  case OpCode::F32x4__convert_i32x4_s:
  case OpCode::F32x4__convert_i32x4_u:
  case OpCode::I32x4__trunc_sat_f64x2_s_zero:
  case OpCode::I32x4__trunc_sat_f64x2_u_zero:
  case OpCode::F64x2__convert_low_i32x4_s:
  case OpCode::F64x2__convert_low_i32x4_u:
  case OpCode::F32x4__demote_f64x2_zero:
  case OpCode::F64x2__promote_low_f32x4:

  case OpCode::I32x4__dot_i16x8_s:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
    return {};

  case OpCode::I8x16__relaxed_swizzle:
  case OpCode::I32x4__relaxed_trunc_f32x4_s:
  case OpCode::I32x4__relaxed_trunc_f32x4_u:
  case OpCode::I32x4__relaxed_trunc_f64x2_s_zero:
  case OpCode::I32x4__relaxed_trunc_f64x2_u_zero:
  case OpCode::F32x4__relaxed_madd:
  case OpCode::F32x4__relaxed_nmadd:
  case OpCode::F64x2__relaxed_madd:
  case OpCode::F64x2__relaxed_nmadd:
  case OpCode::I8x16__relaxed_laneselect:
  case OpCode::I16x8__relaxed_laneselect:
  case OpCode::I32x4__relaxed_laneselect:
  case OpCode::I64x2__relaxed_laneselect:
  case OpCode::F32x4__relaxed_min:
  case OpCode::F32x4__relaxed_max:
  case OpCode::F64x2__relaxed_min:
  case OpCode::F64x2__relaxed_max:
  case OpCode::I16x8__relaxed_q15mulr_s:
  case OpCode::I16x8__relaxed_dot_i8x16_i7x16_s:
  case OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s:
    return {};

  // Atomic Memory Instructions.
  case OpCode::Atomic__fence:
    return serializeCheckZero(Instr.getTargetIndex());

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
    return serializeMemImmediate();

  default:
    assumingUnreachable();
  }
}

} // namespace Loader
} // namespace WasmEdge
