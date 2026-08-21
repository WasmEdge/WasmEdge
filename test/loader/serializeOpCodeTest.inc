// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

// Exhaustive byte-exact coverage of every opcode. The prefix comes from the
// enum.inc macros and the immediate from the shape table below, so neither
// expectation is taken from the serializer.

namespace {

using namespace WasmEdge;

std::vector<uint8_t> leb(uint32_t N) {
  std::vector<uint8_t> Out;
  do {
    uint8_t B = N & 0x7FU;
    N >>= 7;
    if (N) {
      B |= 0x80U;
    }
    Out.push_back(B);
  } while (N);
  return Out;
}

std::vector<uint8_t> cat(std::vector<uint8_t> A,
                         const std::vector<uint8_t> &B) {
  A.insert(A.end(), B.begin(), B.end());
  return A;
}

// The immediate shapes of the binary format. Each opcode maps to exactly one.
enum class Imm {
  None,
  BlockType,    // blocktype
  TryTable,     // blocktype + vec(catch)
  LabelIdx,     // labelidx
  LabelVec,     // vec(labelidx) + labelidx
  TargetIdx,    // a single index
  TargetSrcIdx, // two indices, target then source
  SrcTargetIdx, // two indices, source then target
  TypeTableIdx, // typeidx + tableidx
  HeapType,     // heaptype
  BrOnCast,     // castflags + labelidx + heaptype + heaptype
  ValTypeVec,   // vec(valtype)
  MemArg,       // memarg
  MemArgLane,   // memarg + laneidx
  MemIdx,       // a single memory index
  MemIdxPair,   // two memory indices
  ReservedZero, // a reserved byte that is always zero
  I32Const,
  I64Const,
  F32Const,
  F64Const,
  V128Bytes, // 16 raw bytes
  LaneIdx,   // laneidx
};

// Derived from the grammar the loader implements in loadInstruction, so a
// disagreement between the two directions of the codec shows up here.
Imm immShapeOf(OpCode Code) {
  switch (Code) {
  case OpCode::Unreachable:
  case OpCode::Nop:
  case OpCode::Return:
  case OpCode::Throw_ref:
  case OpCode::End:
  case OpCode::Else:
    return Imm::None;
  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If:
    return Imm::BlockType;
  case OpCode::Try_table:
    return Imm::TryTable;
  case OpCode::Throw:
    return Imm::TargetIdx;
  case OpCode::Br:
  case OpCode::Br_if:
  case OpCode::Br_on_null:
  case OpCode::Br_on_non_null:
    return Imm::LabelIdx;
  case OpCode::Br_table:
    return Imm::LabelVec;
  case OpCode::Call:
  case OpCode::Return_call:
  case OpCode::Call_ref:
  case OpCode::Return_call_ref:
    return Imm::TargetIdx;
  case OpCode::Call_indirect:
  case OpCode::Return_call_indirect:
    return Imm::TypeTableIdx;
  case OpCode::Ref__null:
  case OpCode::Ref__test_null:
  case OpCode::Ref__cast_null:
    return Imm::HeapType;
  case OpCode::Ref__test:
  case OpCode::Ref__cast:
    return Imm::HeapType;
  case OpCode::Ref__is_null:
  case OpCode::Ref__eq:
  case OpCode::Ref__as_non_null:
    return Imm::None;
  case OpCode::Ref__func:
  case OpCode::Struct__new:
  case OpCode::Struct__new_default:
  case OpCode::Array__new:
  case OpCode::Array__new_default:
  case OpCode::Array__get:
  case OpCode::Array__get_s:
  case OpCode::Array__get_u:
  case OpCode::Array__set:
  case OpCode::Array__fill:
    return Imm::TargetIdx;
  case OpCode::Struct__get:
  case OpCode::Struct__get_s:
  case OpCode::Struct__get_u:
  case OpCode::Struct__set:
  case OpCode::Array__new_fixed:
  case OpCode::Array__new_data:
  case OpCode::Array__new_elem:
  case OpCode::Array__copy:
  case OpCode::Array__init_data:
  case OpCode::Array__init_elem:
    return Imm::TargetSrcIdx;
  case OpCode::Br_on_cast:
  case OpCode::Br_on_cast_fail:
    return Imm::BrOnCast;
  case OpCode::Array__len:
  case OpCode::Any__convert_extern:
  case OpCode::Extern__convert_any:
  case OpCode::Ref__i31:
  case OpCode::I31__get_s:
  case OpCode::I31__get_u:
    return Imm::None;
  case OpCode::Drop:
  case OpCode::Select:
    return Imm::None;
  case OpCode::Select_t:
    return Imm::ValTypeVec;
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    return Imm::TargetIdx;
  case OpCode::Table__init:
    return Imm::SrcTargetIdx;
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
  case OpCode::Elem__drop:
    return Imm::TargetIdx;
  case OpCode::Table__copy:
    return Imm::TargetSrcIdx;
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
    return Imm::MemArg;
  case OpCode::Memory__init:
    return Imm::SrcTargetIdx;
  case OpCode::Memory__grow:
  case OpCode::Memory__size:
  case OpCode::Memory__fill:
    return Imm::MemIdx;
  case OpCode::Memory__copy:
    return Imm::MemIdxPair;
  case OpCode::Data__drop:
    return Imm::TargetIdx;
  case OpCode::I32__const:
    return Imm::I32Const;
  case OpCode::I64__const:
    return Imm::I64Const;
  case OpCode::F32__const:
    return Imm::F32Const;
  case OpCode::F64__const:
    return Imm::F64Const;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::MemArg;
  case OpCode::V128__load8_lane:
  case OpCode::V128__load16_lane:
  case OpCode::V128__load32_lane:
  case OpCode::V128__load64_lane:
  case OpCode::V128__store8_lane:
  case OpCode::V128__store16_lane:
  case OpCode::V128__store32_lane:
  case OpCode::V128__store64_lane:
    return Imm::MemArgLane;
  case OpCode::V128__const:
    return Imm::V128Bytes;
  case OpCode::I8x16__shuffle:
    return Imm::V128Bytes;
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
    return Imm::LaneIdx;
  case OpCode::I8x16__swizzle:
  case OpCode::I8x16__splat:
  case OpCode::I16x8__splat:
  case OpCode::I32x4__splat:
  case OpCode::I64x2__splat:
  case OpCode::F32x4__splat:
  case OpCode::F64x2__splat:
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
  case OpCode::F32x4__eq:
  case OpCode::F32x4__ne:
  case OpCode::F32x4__lt:
  case OpCode::F32x4__gt:
  case OpCode::F32x4__le:
  case OpCode::F32x4__ge:
    return Imm::None;
  case OpCode::F64x2__eq:
  case OpCode::F64x2__ne:
  case OpCode::F64x2__lt:
  case OpCode::F64x2__gt:
  case OpCode::F64x2__le:
  case OpCode::F64x2__ge:
    return Imm::None;
  case OpCode::V128__not:
  case OpCode::V128__and:
  case OpCode::V128__andnot:
  case OpCode::V128__or:
  case OpCode::V128__xor:
  case OpCode::V128__bitselect:
  case OpCode::V128__any_true:
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
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
    return Imm::None;
  case OpCode::I32x4__dot_i16x8_s:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
    return Imm::None;
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
    return Imm::None;
  case OpCode::Atomic__fence:
    return Imm::ReservedZero;
  case OpCode::Memory__atomic__notify:
  case OpCode::Memory__atomic__wait32:
  case OpCode::Memory__atomic__wait64:
    return Imm::MemArg;
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
    return Imm::MemArg;
  default:
    ADD_FAILURE() << "opcode has no immediate shape entry";
    return Imm::None;
  }
}

// Values chosen so that a mis-ordered or dropped field shows up in the bytes.
constexpr uint32_t TargetIdxValue = 0x11U;
constexpr uint32_t SourceIdxValue = 0x22U;
constexpr uint32_t MemAlignValue = 0x03U;
constexpr uint64_t MemOffsetValue = 0x44U;
constexpr uint8_t LaneValue = 0x05U;

struct Case {
  AST::Instruction Instr;
  std::vector<uint8_t> ImmBytes;
};

Case makeCase(OpCode Code) {
  AST::Instruction Instr(Code);
  std::vector<uint8_t> Imm;

  switch (immShapeOf(Code)) {
  case Imm::None:
    break;

  case Imm::BlockType:
    Instr.getBlockType().setEmpty();
    Imm = {0x40U};
    break;

  case Imm::TryTable: {
    Instr.setTryCatch();
    Instr.getTryCatch().ResType.setEmpty();
    AST::Instruction::CatchDescriptor Catch;
    Catch.IsAll = false;
    Catch.IsRef = false;
    Catch.TagIndex = 0x07U;
    Catch.LabelIndex = 0x08U;
    Instr.getTryCatch().Catch.push_back(Catch);
    Imm = {0x40U, 0x01U, 0x00U, 0x07U, 0x08U};
    break;
  }

  case Imm::LabelIdx:
    Instr.getJump().TargetIndex = TargetIdxValue;
    Imm = leb(TargetIdxValue);
    break;

  case Imm::LabelVec:
    Instr.setLabelListSize(2);
    Instr.getLabelList()[0].TargetIndex = 0x09U;
    Instr.getLabelList()[1].TargetIndex = 0x0AU;
    // The list holds the branch targets followed by the default label.
    Imm = {0x01U, 0x09U, 0x0AU};
    break;

  case Imm::TargetIdx:
    Instr.getTargetIndex() = TargetIdxValue;
    Imm = leb(TargetIdxValue);
    break;

  case Imm::TargetSrcIdx:
    Instr.getTargetIndex() = TargetIdxValue;
    Instr.getSourceIndex() = SourceIdxValue;
    Imm = cat(leb(TargetIdxValue), leb(SourceIdxValue));
    break;

  case Imm::SrcTargetIdx:
    Instr.getTargetIndex() = TargetIdxValue;
    Instr.getSourceIndex() = SourceIdxValue;
    Imm = cat(leb(SourceIdxValue), leb(TargetIdxValue));
    break;

  case Imm::TypeTableIdx:
    Instr.getTargetIndex() = TargetIdxValue;
    Instr.getSourceIndex() = SourceIdxValue;
    Imm = cat(leb(TargetIdxValue), leb(SourceIdxValue));
    break;

  case Imm::HeapType:
    Instr.setValType(ValType(TypeCode::RefNull, TypeCode::FuncRef));
    Imm = {0x70U};
    break;

  case Imm::BrOnCast:
    Instr.setBrCast(TargetIdxValue);
    Instr.getBrCast().RType1 = ValType(TypeCode::RefNull, TypeCode::FuncRef);
    Instr.getBrCast().RType2 = ValType(TypeCode::Ref, TypeCode::FuncRef);
    // Only the first reference type is nullable, so the flag byte is 0x01.
    Imm = cat({0x01U}, cat(leb(TargetIdxValue), {0x70U, 0x70U}));
    break;

  case Imm::ValTypeVec:
    Instr.setValTypeListSize(1);
    Instr.getValTypeList()[0] = ValType(TypeCode::I32);
    Imm = {0x01U, 0x7FU};
    break;

  case Imm::MemArg:
    Instr.getMemoryAlign() = MemAlignValue;
    Instr.getMemoryOffset() = MemOffsetValue;
    Instr.getTargetIndex() = 0;
    Imm = {static_cast<uint8_t>(MemAlignValue),
           static_cast<uint8_t>(MemOffsetValue)};
    break;

  case Imm::MemArgLane:
    Instr.getMemoryAlign() = MemAlignValue;
    Instr.getMemoryOffset() = MemOffsetValue;
    Instr.getTargetIndex() = 0;
    Instr.getMemoryLane() = LaneValue;
    Imm = {static_cast<uint8_t>(MemAlignValue),
           static_cast<uint8_t>(MemOffsetValue), LaneValue};
    break;

  case Imm::MemIdx:
    Instr.getTargetIndex() = TargetIdxValue;
    Imm = leb(TargetIdxValue);
    break;

  case Imm::MemIdxPair:
    Instr.getTargetIndex() = TargetIdxValue;
    Instr.getSourceIndex() = SourceIdxValue;
    Imm = cat(leb(TargetIdxValue), leb(SourceIdxValue));
    break;

  case Imm::ReservedZero:
    Instr.getTargetIndex() = 0;
    Imm = {0x00U};
    break;

  case Imm::I32Const:
    Instr.setNum(static_cast<uint32_t>(1));
    Imm = {0x01U};
    break;

  case Imm::I64Const:
    Instr.setNum(static_cast<uint64_t>(1));
    Imm = {0x01U};
    break;

  case Imm::F32Const:
    Instr.setNum(1.0F);
    Imm = {0x00U, 0x00U, 0x80U, 0x3FU};
    break;

  case Imm::F64Const:
    Instr.setNum(1.0);
    Imm = {0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xF0U, 0x3FU};
    break;

  case Imm::V128Bytes:
    // 1U is an exact match for uint128(unsigned int); a plain 1 is
    // ambiguous where uint128_t is a class rather than a builtin.
    Instr.setNum(static_cast<uint128_t>(1U));
    Imm = {0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
           0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};
    break;

  case Imm::LaneIdx:
    Instr.getMemoryLane() = LaneValue;
    Imm = {LaneValue};
    break;
  }

  return {std::move(Instr), std::move(Imm)};
}

} // namespace
