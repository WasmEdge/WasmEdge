// SPDX-License-Identifier: Apache-2.0
#include "ast/instruction.h"
#include "ast/base.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

namespace {
Expect<void> checkInstrProposals(OpCode Code, const ProposalConfigure &PConf,
                                 uint32_t Offset) {
  if ((Code >= OpCode::Ref__null && Code <= OpCode::Ref__func) ||
      (Code >= OpCode::Table__init && Code <= OpCode::Table__copy) ||
      (Code >= OpCode::Memory__init && Code <= OpCode::Memory__fill)) {
    /// These instructions are for ReferenceTypes or BulkMemoryOperations
    /// proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes) &&
        !PConf.hasProposal(Proposal::BulkMemoryOperations)) {
      return logNeedProposal(ErrCode::InvalidOpCode, Proposal::ReferenceTypes,
                             Offset, ASTNodeAttr::Instruction);
    }
  } else if (Code == OpCode::Select_t ||
             (Code >= OpCode::Table__get && Code <= OpCode::Table__set) ||
             (Code >= OpCode::Table__grow && Code <= OpCode::Table__fill)) {
    /// These instructions are for ReferenceTypes proposal.
    if (!PConf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::InvalidOpCode, Proposal::ReferenceTypes,
                             Offset, ASTNodeAttr::Instruction);
    }
  } else if (Code >= OpCode::V128__load &&
             Code <= OpCode::F64x2__convert_i64x2_u) {
    /// These instructions are for SIMD proposal.
    if (!PConf.hasProposal(Proposal::SIMD)) {
      return logNeedProposal(ErrCode::InvalidOpCode, Proposal::SIMD, Offset,
                             ASTNodeAttr::Instruction);
    }
  }
  return {};
}
} // namespace

Expect<void> Instruction::loadBinary(FileMgr &Mgr,
                                     const ProposalConfigure &PConf) {
  /// Node: The instruction has checked for the proposals. Need to check their
  /// immediates.

  auto readCheck = [&Mgr](uint8_t Val) -> Expect<void> {
    if (auto Res = Mgr.readByte()) {
      if (*Res != Val) {
        return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                            ASTNodeAttr::Instruction);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  };

  auto readU32 = [&Mgr](uint32_t &Dst) -> Expect<void> {
    if (auto Res = Mgr.readU32()) {
      Dst = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  };

  switch (Code) {
  /// Control instructions.
  case OpCode::Unreachable:
  case OpCode::Nop:
  case OpCode::Return:
  case OpCode::End:
  case OpCode::Else:
    return {};

  case OpCode::Block:
  case OpCode::Loop:
  case OpCode::If:
    /// Read the block return type.
    if (auto Res = Mgr.readS32()) {
      if (*Res < 0) {
        /// Value type case.
        ValType VType = static_cast<ValType>((*Res) & 0x7FU);
        if (auto Check = checkValTypeProposals(
                PConf, VType, Mgr.getOffset() - 1, ASTNodeAttr::Instruction);
            !Check) {
          return Unexpect(Check);
        }
        ResType = VType;
      } else {
        /// Type index case.
        ResType = static_cast<uint32_t>(*Res);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};

  case OpCode::Br:
  case OpCode::Br_if:
    return readU32(TargetIdx);

  case OpCode::Br_table:
    if (auto Res = Mgr.readU32()) {
      /// Read the vector of labels.
      uint32_t VecCnt = *Res;
      LabelList.reserve(VecCnt);
      for (uint32_t I = 0; I < VecCnt; ++I) {
        uint32_t Label = 0;
        if (auto Res = readU32(Label)) {
          LabelList.push_back(Label);
        } else {
          return Unexpect(Res);
        }
      }
      /// Read default label.
      return readU32(TargetIdx);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }

  case OpCode::Call:
    return readU32(TargetIdx);

  case OpCode::Call_indirect:
    /// Read function index.
    if (auto Res = readU32(TargetIdx); !Res) {
      return Unexpect(Res);
    }
    /// Read the table index.
    if (auto Res = readU32(SourceIdx); !Res) {
      return Unexpect(Res);
    }
    if (SourceIdx > 0 && !PConf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::InvalidGrammar, Proposal::ReferenceTypes,
                             Mgr.getOffset() - 1, ASTNodeAttr::Instruction);
    }
    return {};

  /// Reference Instructions.
  case OpCode::Ref__null:
    if (auto Res = Mgr.readByte()) {
      ReferenceType = static_cast<RefType>(*Res);
      if (auto Check =
              checkRefTypeProposals(PConf, ReferenceType, Mgr.getOffset() - 1,
                                    ASTNodeAttr::Instruction);
          !Check) {
        return Unexpect(Check);
      }
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  case OpCode::Ref__is_null:
    return {};
  case OpCode::Ref__func:
    return readU32(TargetIdx);

  /// Parametric Instructions.
  case OpCode::Drop:
  case OpCode::Select:
    return {};
  case OpCode::Select_t:
    if (auto Res = Mgr.readU32()) {
      /// Read the vector of value types.
      uint32_t VecCnt = *Res;
      ValTypeList.reserve(VecCnt);
      for (uint32_t I = 0; I < VecCnt; ++I) {
        if (auto T = Mgr.readByte()) {
          ValType VType = static_cast<ValType>(*T);
          if (auto Check = checkValTypeProposals(
                  PConf, VType, Mgr.getOffset() - 1, ASTNodeAttr::Instruction);
              !Check) {
            return Unexpect(Check);
          }
          ValTypeList.push_back(VType);
        } else {
          return logLoadError(Res.error(), Mgr.getOffset(),
                              ASTNodeAttr::Instruction);
        }
      }
      return {};
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }

  /// Variable Instructions.
  case OpCode::Local__get:
  case OpCode::Local__set:
  case OpCode::Local__tee:
  case OpCode::Global__get:
  case OpCode::Global__set:
    return readU32(TargetIdx);

  /// Table Instructions.
  case OpCode::Table__get:
  case OpCode::Table__set:
  case OpCode::Table__copy:
  case OpCode::Table__grow:
  case OpCode::Table__size:
  case OpCode::Table__fill:
    if (auto Res = readU32(TargetIdx); !Res) {
      return Unexpect(Res);
    }
    if (Code == OpCode::Table__copy) {
      return readU32(SourceIdx);
    }
    return {};
  case OpCode::Table__init:
    if (auto Res = readU32(SourceIdx); !Res) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Elem__drop:
    return readU32(TargetIdx);

  /// Memory Instructions.
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
    /// Read memory arguments.
    if (auto Res = readU32(MemAlign); !Res) {
      return Unexpect(Res);
    }
    return readU32(MemOffset);

  case OpCode::Memory__copy:
    if (auto Res = readCheck(0x00); !Res) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Memory__grow:
  case OpCode::Memory__size:
  case OpCode::Memory__fill:
    return readCheck(0x00);
  case OpCode::Memory__init:
    if (auto Res = readU32(SourceIdx); !Res) {
      return Unexpect(Res);
    }
    return readCheck(0x00);
  case OpCode::Data__drop:
    return readU32(TargetIdx);

  /// Const Instructions.
  case OpCode::I32__const:
    if (auto Res = Mgr.readS32()) {
      Num = static_cast<uint32_t>(*Res);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  case OpCode::I64__const:
    if (auto Res = Mgr.readS64()) {
      Num = static_cast<uint64_t>(*Res);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  case OpCode::F32__const:
    if (auto Res = Mgr.readF32()) {
      Num = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};
  case OpCode::F64__const:
    if (auto Res = Mgr.readF64()) {
      Num = *Res;
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};

  /// Unary Numeric Instructions.
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

  /// Binary Numeric Instructions.
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

  /// SIMD Memory Instruction.
  case OpCode::V128__load:
  case OpCode::I16x8__load8x8_s:
  case OpCode::I16x8__load8x8_u:
  case OpCode::I32x4__load16x4_s:
  case OpCode::I32x4__load16x4_u:
  case OpCode::I64x2__load32x2_s:
  case OpCode::I64x2__load32x2_u:
  case OpCode::I8x16__load_splat:
  case OpCode::I16x8__load_splat:
  case OpCode::I32x4__load_splat:
  case OpCode::I64x2__load_splat:
  case OpCode::V128__load32_zero:
  case OpCode::V128__load64_zero:
  case OpCode::V128__store:
    /// Read memory arguments.
    if (auto Res = readU32(MemAlign); !Res) {
      return Unexpect(Res);
    }
    return readU32(MemOffset);

  /// SIMD Const Instruction.
  case OpCode::V128__const:
  /// SIMD Shuffle Instruction.
  case OpCode::I8x16__shuffle: {
    /// Read value.
    uint128_t Value = 0;
    for (uint32_t I = 0; I < 16; ++I) {
      if (auto Res = Mgr.readByte()) {
        Value |= uint128_t(*Res) << (I * 8);
      } else {
        return logLoadError(Res.error(), Mgr.getOffset(),
                            ASTNodeAttr::Instruction);
      }
    }
    Num = Value;
    return {};
  }

  /// SIMD Lane Instructions.
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
    /// Read lane index.
    if (auto Res = Mgr.readByte()) {
      TargetIdx = static_cast<uint32_t>(*Res);
    } else {
      return logLoadError(Res.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
    return {};

  /// SIMD Numeric Instructions.
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

  case OpCode::I8x16__abs:
  case OpCode::I8x16__neg:
  case OpCode::I8x16__any_true:
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
  case OpCode::I16x8__any_true:
  case OpCode::I16x8__all_true:
  case OpCode::I16x8__bitmask:
  case OpCode::I16x8__narrow_i32x4_s:
  case OpCode::I16x8__narrow_i32x4_u:
  case OpCode::I16x8__widen_low_i8x16_s:
  case OpCode::I16x8__widen_high_i8x16_s:
  case OpCode::I16x8__widen_low_i8x16_u:
  case OpCode::I16x8__widen_high_i8x16_u:
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

  case OpCode::I32x4__abs:
  case OpCode::I32x4__neg:
  case OpCode::I32x4__any_true:
  case OpCode::I32x4__all_true:
  case OpCode::I32x4__bitmask:
  case OpCode::I32x4__widen_low_i16x8_s:
  case OpCode::I32x4__widen_high_i16x8_s:
  case OpCode::I32x4__widen_low_i16x8_u:
  case OpCode::I32x4__widen_high_i16x8_u:
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

  case OpCode::I64x2__neg:
  case OpCode::I64x2__shl:
  case OpCode::I64x2__shr_s:
  case OpCode::I64x2__shr_u:
  case OpCode::I64x2__add:
  case OpCode::I64x2__sub:
  case OpCode::I64x2__mul:

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

  case OpCode::I8x16__mul:
  case OpCode::I32x4__dot_i16x8_s:
  case OpCode::I64x2__any_true:
  case OpCode::I64x2__all_true:
  case OpCode::F32x4__qfma:
  case OpCode::F32x4__qfms:
  case OpCode::F64x2__qfma:
  case OpCode::F64x2__qfms:
  case OpCode::F32x4__ceil:
  case OpCode::F32x4__floor:
  case OpCode::F32x4__trunc:
  case OpCode::F32x4__nearest:
  case OpCode::F64x2__ceil:
  case OpCode::F64x2__floor:
  case OpCode::F64x2__trunc:
  case OpCode::F64x2__nearest:
  case OpCode::I64x2__trunc_sat_f64x2_s:
  case OpCode::I64x2__trunc_sat_f64x2_u:
  case OpCode::F64x2__convert_i64x2_s:
  case OpCode::F64x2__convert_i64x2_u:
    return {};

  default:
    return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                        ASTNodeAttr::Instruction);
  }
}

/// OpCode loader. See "include/ast/instruction.h".
Expect<OpCode> loadOpCode(FileMgr &Mgr) {
  uint16_t Payload;
  if (auto B1 = Mgr.readByte()) {
    Payload = (*B1);
  } else {
    return logLoadError(B1.error(), Mgr.getOffset(), ASTNodeAttr::Instruction);
  }

  if (Payload == 0xFCU || Payload == 0xFDU) {
    /// 2-bytes OpCode case.
    if (auto B2 = Mgr.readU32()) {
      Payload <<= 8;
      Payload += (*B2);
    } else {
      return logLoadError(B2.error(), Mgr.getOffset(),
                          ASTNodeAttr::Instruction);
    }
  }
  return static_cast<OpCode>(Payload);
}

Expect<InstrVec> loadInstrSeq(FileMgr &Mgr, const ProposalConfigure &PConf) {
  OpCode Code;
  InstrVec Instrs;
  std::vector<std::pair<OpCode, uint32_t>> BlockStack;
  uint32_t Cnt = 0;
  bool IsReachEnd = false;
  /// Read opcode until the End code of the top block.
  do {
    /// Read the opcode and check if error.
    uint32_t Offset = Mgr.getOffset();
    if (auto Res = loadOpCode(Mgr)) {
      Code = *Res;
    } else {
      return Unexpect(Res);
    }

    /// Check with proposals.
    if (auto Res = checkInstrProposals(Code, PConf, Mgr.getOffset() - 1);
        !Res) {
      return Unexpect(Res);
    }

    /// Process the instructions which contain a block.
    if (Code == OpCode::Block || Code == OpCode::Loop || Code == OpCode::If) {
      BlockStack.push_back(std::make_pair(Code, Cnt));
    } else if (Code == OpCode::Else) {
      if (BlockStack.size() == 0 || BlockStack.back().first != OpCode::If) {
        return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                            ASTNodeAttr::Instruction);
      }
      uint32_t Pos = BlockStack.back().second;
      if (Instrs[Pos].getJumpElse() > 0) {
        /// An Else instruction appeared before in this If-block.
        return logLoadError(ErrCode::InvalidGrammar, Mgr.getOffset() - 1,
                            ASTNodeAttr::Instruction);
      }
      Instrs[Pos].setJumpElse(Cnt - Pos);
    } else if (Code == OpCode::End) {
      if (BlockStack.size() > 0) {
        uint32_t Pos = BlockStack.back().second;
        Instrs[Pos].setJumpEnd(Cnt - Pos);
        if (BlockStack.back().first == OpCode::If &&
            Instrs[Pos].getJumpElse() == 0) {
          /// If block without else. Set the else jump the same as end jump.
          Instrs[Pos].setJumpElse(Cnt - Pos);
        }
        BlockStack.pop_back();
      } else {
        IsReachEnd = true;
      }
    }

    /// Create the instruction node and load contents.
    Instrs.emplace_back(Code, Offset);
    if (auto Res = Instrs.back().loadBinary(Mgr, PConf); !Res) {
      return Unexpect(Res);
    }
    Cnt++;
  } while (!IsReachEnd);
  return Instrs;
}

} // namespace AST
} // namespace SSVM
