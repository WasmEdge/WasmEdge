// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum.inc - Enumerations ---------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains all enumerations of WasmEdge.
///
//===----------------------------------------------------------------------===//

#ifndef Line
#error "this header file must not be included directly"
#endif

// enum_ast.hpp

#ifdef UseASTNodeAttr
#define A Line

A(Component, "component")
A(Module, "module")
A(Sec_Custom, "custom section")
A(Sec_Type, "type section")
A(Sec_Import, "import section")
A(Sec_Function, "function section")
A(Sec_Table, "table section")
A(Sec_Memory, "memory section")
A(Sec_Global, "global section")
A(Sec_Export, "export section")
A(Sec_Start, "start section")
A(Sec_Element, "element section")
A(Sec_Code, "code section")
A(Sec_Data, "data section")
A(Sec_DataCount, "data count section")
A(Desc_Import, "import description")
A(Desc_Export, "export description")
A(Seg_Table, "table segment")
A(Seg_Global, "global segment")
A(Seg_Element, "element segment")
A(Seg_Code, "code segment")
A(Seg_Data, "data segment")
A(Type_Limit, "limit")
A(Type_Function, "function type")
A(Type_Memory, "memory type")
A(Type_Table, "table type")
A(Type_Global, "global type")
A(Expression, "expression")
A(Instruction, "instruction")
A(Sec_AOT, "aot section")

#undef A
#endif // UseASTNodeAttr

#ifdef UseOpCode
#define O Line

// Control instructions (part 1)
O(Unreachable, 0x00, "unreachable")
O(Nop, 0x01, "nop")
O(Block, 0x02, "block")
O(Loop, 0x03, "loop")
O(If, 0x04, "if")
O(Else, 0x05, "else")
O(End, 0x0B, "end")
O(Br, 0x0C, "br")
O(Br_if, 0x0D, "br_if")
O(Br_table, 0x0E, "br_table")
O(Return, 0x0F, "return")
O(Call, 0x10, "call")
O(Call_indirect, 0x11, "call_indirect")
O(Return_call, 0x12, "return_call")
O(Return_call_indirect, 0x13, "return_call_indirect")
O(Call_ref, 0x14, "call_ref")
O(Return_call_ref, 0x15, "return_call_ref")

// Parametric Instructions
O(Drop, 0x1A, "drop")
O(Select, 0x1B, "select")
O(Select_t, 0x1C, "select")

// Variable Instructions
O(Local__get, 0x20, "local.get")
O(Local__set, 0x21, "local.set")
O(Local__tee, 0x22, "local.tee")
O(Global__get, 0x23, "global.get")
O(Global__set, 0x24, "global.set")

// Table Instructions (part 1)
O(Table__get, 0x25, "table.get")
O(Table__set, 0x26, "table.set")

// Memory Instructions (part 1)
O(I32__load, 0x28, "i32.load")
O(I64__load, 0x29, "i64.load")
O(F32__load, 0x2A, "f32.load")
O(F64__load, 0x2B, "f64.load")
O(I32__load8_s, 0x2C, "i32.load8_s")
O(I32__load8_u, 0x2D, "i32.load8_u")
O(I32__load16_s, 0x2E, "i32.load16_s")
O(I32__load16_u, 0x2F, "i32.load16_u")
O(I64__load8_s, 0x30, "i64.load8_s")
O(I64__load8_u, 0x31, "i64.load8_u")
O(I64__load16_s, 0x32, "i64.load16_s")
O(I64__load16_u, 0x33, "i64.load16_u")
O(I64__load32_s, 0x34, "i64.load32_s")
O(I64__load32_u, 0x35, "i64.load32_u")
O(I32__store, 0x36, "i32.store")
O(I64__store, 0x37, "i64.store")
O(F32__store, 0x38, "f32.store")
O(F64__store, 0x39, "f64.store")
O(I32__store8, 0x3A, "i32.store8")
O(I32__store16, 0x3B, "i32.store16")
O(I64__store8, 0x3C, "i64.store8")
O(I64__store16, 0x3D, "i64.store16")
O(I64__store32, 0x3E, "i64.store32")
O(Memory__size, 0x3F, "memory.size")
O(Memory__grow, 0x40, "memory.grow")

// Const numeric instructions
O(I32__const, 0x41, "i32.const")
O(I64__const, 0x42, "i64.const")
O(F32__const, 0x43, "f32.const")
O(F64__const, 0x44, "f64.const")

// Numeric instructions
O(I32__eqz, 0x45, "i32.eqz")
O(I32__eq, 0x46, "i32.eq")
O(I32__ne, 0x47, "i32.ne")
O(I32__lt_s, 0x48, "i32.lt_s")
O(I32__lt_u, 0x49, "i32.lt_u")
O(I32__gt_s, 0x4A, "i32.gt_s")
O(I32__gt_u, 0x4B, "i32.gt_u")
O(I32__le_s, 0x4C, "i32.le_s")
O(I32__le_u, 0x4D, "i32.le_u")
O(I32__ge_s, 0x4E, "i32.ge_s")
O(I32__ge_u, 0x4F, "i32.ge_u")
O(I64__eqz, 0x50, "i64.eqz")
O(I64__eq, 0x51, "i64.eq")
O(I64__ne, 0x52, "i64.ne")
O(I64__lt_s, 0x53, "i64.lt_s")
O(I64__lt_u, 0x54, "i64.lt_u")
O(I64__gt_s, 0x55, "i64.gt_s")
O(I64__gt_u, 0x56, "i64.gt_u")
O(I64__le_s, 0x57, "i64.le_s")
O(I64__le_u, 0x58, "i64.le_u")
O(I64__ge_s, 0x59, "i64.ge_s")
O(I64__ge_u, 0x5A, "i64.ge_u")
O(F32__eq, 0x5B, "f32.eq")
O(F32__ne, 0x5C, "f32.ne")
O(F32__lt, 0x5D, "f32.lt")
O(F32__gt, 0x5E, "f32.gt")
O(F32__le, 0x5F, "f32.le")
O(F32__ge, 0x60, "f32.ge")
O(F64__eq, 0x61, "f64.eq")
O(F64__ne, 0x62, "f64.ne")
O(F64__lt, 0x63, "f64.lt")
O(F64__gt, 0x64, "f64.gt")
O(F64__le, 0x65, "f64.le")
O(F64__ge, 0x66, "f64.ge")
O(I32__clz, 0x67, "i32.clz")
O(I32__ctz, 0x68, "i32.ctz")
O(I32__popcnt, 0x69, "i32.popcnt")
O(I32__add, 0x6A, "i32.add")
O(I32__sub, 0x6B, "i32.sub")
O(I32__mul, 0x6C, "i32.mul")
O(I32__div_s, 0x6D, "i32.div_s")
O(I32__div_u, 0x6E, "i32.div_u")
O(I32__rem_s, 0x6F, "i32.rem_s")
O(I32__rem_u, 0x70, "i32.rem_u")
O(I32__and, 0x71, "i32.and")
O(I32__or, 0x72, "i32.or")
O(I32__xor, 0x73, "i32.xor")
O(I32__shl, 0x74, "i32.shl")
O(I32__shr_s, 0x75, "i32.shr_s")
O(I32__shr_u, 0x76, "i32.shr_u")
O(I32__rotl, 0x77, "i32.rotl")
O(I32__rotr, 0x78, "i32.rotr")
O(I64__clz, 0x79, "i64.clz")
O(I64__ctz, 0x7a, "i64.ctz")
O(I64__popcnt, 0x7b, "i64.popcnt")
O(I64__add, 0x7c, "i64.add")
O(I64__sub, 0x7d, "i64.sub")
O(I64__mul, 0x7e, "i64.mul")
O(I64__div_s, 0x7f, "i64.div_s")
O(I64__div_u, 0x80, "i64.div_u")
O(I64__rem_s, 0x81, "i64.rem_s")
O(I64__rem_u, 0x82, "i64.rem_u")
O(I64__and, 0x83, "i64.and")
O(I64__or, 0x84, "i64.or")
O(I64__xor, 0x85, "i64.xor")
O(I64__shl, 0x86, "i64.shl")
O(I64__shr_s, 0x87, "i64.shr_s")
O(I64__shr_u, 0x88, "i64.shr_u")
O(I64__rotl, 0x89, "i64.rotl")
O(I64__rotr, 0x8A, "i64.rotr")
O(F32__abs, 0x8B, "f32.abs")
O(F32__neg, 0x8C, "f32.neg")
O(F32__ceil, 0x8D, "f32.ceil")
O(F32__floor, 0x8E, "f32.floor")
O(F32__trunc, 0x8F, "f32.trunc")
O(F32__nearest, 0x90, "f32.nearest")
O(F32__sqrt, 0x91, "f32.sqrt")
O(F32__add, 0x92, "f32.add")
O(F32__sub, 0x93, "f32.sub")
O(F32__mul, 0x94, "f32.mul")
O(F32__div, 0x95, "f32.div")
O(F32__min, 0x96, "f32.min")
O(F32__max, 0x97, "f32.max")
O(F32__copysign, 0x98, "f32.copysign")
O(F64__abs, 0x99, "f64.abs")
O(F64__neg, 0x9A, "f64.neg")
O(F64__ceil, 0x9B, "f64.ceil")
O(F64__floor, 0x9C, "f64.floor")
O(F64__trunc, 0x9D, "f64.trunc")
O(F64__nearest, 0x9E, "f64.nearest")
O(F64__sqrt, 0x9F, "f64.sqrt")
O(F64__add, 0xA0, "f64.add")
O(F64__sub, 0xA1, "f64.sub")
O(F64__mul, 0xA2, "f64.mul")
O(F64__div, 0xA3, "f64.div")
O(F64__min, 0xA4, "f64.min")
O(F64__max, 0xA5, "f64.max")
O(F64__copysign, 0xA6, "f64.copysign")
O(I32__wrap_i64, 0xA7, "i32.wrap_i64")
O(I32__trunc_f32_s, 0xA8, "i32.trunc_f32_s")
O(I32__trunc_f32_u, 0xA9, "i32.trunc_f32_u")
O(I32__trunc_f64_s, 0xAA, "i32.trunc_f64_s")
O(I32__trunc_f64_u, 0xAB, "i32.trunc_f64_u")
O(I64__extend_i32_s, 0xAC, "i64.extend_i32_s")
O(I64__extend_i32_u, 0xAD, "i64.extend_i32_u")
O(I64__trunc_f32_s, 0xAE, "i64.trunc_f32_s")
O(I64__trunc_f32_u, 0xAF, "i64.trunc_f32_u")
O(I64__trunc_f64_s, 0xB0, "i64.trunc_f64_s")
O(I64__trunc_f64_u, 0xB1, "i64.trunc_f64_u")
O(F32__convert_i32_s, 0xB2, "f32.convert_i32_s")
O(F32__convert_i32_u, 0xB3, "f32.convert_i32_u")
O(F32__convert_i64_s, 0xB4, "f32.convert_i64_s")
O(F32__convert_i64_u, 0xB5, "f32.convert_i64_u")
O(F32__demote_f64, 0xB6, "f32.demote_f64")
O(F64__convert_i32_s, 0xB7, "f64.convert_i32_s")
O(F64__convert_i32_u, 0xB8, "f64.convert_i32_u")
O(F64__convert_i64_s, 0xB9, "f64.convert_i64_s")
O(F64__convert_i64_u, 0xBA, "f64.convert_i64_u")
O(F64__promote_f32, 0xBB, "f64.promote_f32")
O(I32__reinterpret_f32, 0xBC, "i32.reinterpret_f32")
O(I64__reinterpret_f64, 0xBD, "i64.reinterpret_f64")
O(F32__reinterpret_i32, 0xBE, "f32.reinterpret_i32")
O(F64__reinterpret_i64, 0xBF, "f64.reinterpret_i64")
O(I32__extend8_s, 0xC0, "i32.extend8_s")
O(I32__extend16_s, 0xC1, "i32.extend16_s")
O(I64__extend8_s, 0xC2, "i64.extend8_s")
O(I64__extend16_s, 0xC3, "i64.extend16_s")
O(I64__extend32_s, 0xC4, "i64.extend32_s")

// Reference Instructions
O(Ref__null, 0xD0, "ref.null")
O(Ref__is_null, 0xD1, "ref.is_null")
O(Ref__func, 0xD2, "ref.func")
O(Ref__as_non_null, 0xD4, "ref.as_non_null")

// Control Instructions (part 2)
O(Br_on_null, 0xD5, "br_on_null")
O(Br_on_non_null, 0xD6, "br_on_non_null")

// Saturating Truncation Instructions
O(I32__trunc_sat_f32_s, 0xFC00, "i32.trunc_sat_f32_s")
O(I32__trunc_sat_f32_u, 0xFC01, "i32.trunc_sat_f32_u")
O(I32__trunc_sat_f64_s, 0xFC02, "i32.trunc_sat_f64_s")
O(I32__trunc_sat_f64_u, 0xFC03, "i32.trunc_sat_f64_u")
O(I64__trunc_sat_f32_s, 0xFC04, "i64.trunc_sat_f32_s")
O(I64__trunc_sat_f32_u, 0xFC05, "i64.trunc_sat_f32_u")
O(I64__trunc_sat_f64_s, 0xFC06, "i64.trunc_sat_f64_s")
O(I64__trunc_sat_f64_u, 0xFC07, "i64.trunc_sat_f64_u")

// Memory Instructions (part 2)
O(Memory__init, 0xFC08, "memory.init")
O(Data__drop, 0xFC09, "data.drop")
O(Memory__copy, 0xFC0A, "memory.copy")
O(Memory__fill, 0xFC0B, "memory.fill")

// Table Instructions (part 2)
O(Table__init, 0xFC0C, "table.init")
O(Elem__drop, 0xFC0D, "elem.drop")
O(Table__copy, 0xFC0E, "table.copy")
O(Table__grow, 0xFC0F, "table.grow")
O(Table__size, 0xFC10, "table.size")
O(Table__fill, 0xFC11, "table.fill")

// SIMD Memory Instructions
O(V128__load, 0xFD00, "v128.load")
O(V128__load8x8_s, 0xFD01, "v128.load8x8_s")
O(V128__load8x8_u, 0xFD02, "v128.load8x8_u")
O(V128__load16x4_s, 0xFD03, "v128.load16x4_s")
O(V128__load16x4_u, 0xFD04, "v128.load16x4_u")
O(V128__load32x2_s, 0xFD05, "v128.load32x2_s")
O(V128__load32x2_u, 0xFD06, "v128.load32x2_u")
O(V128__load8_splat, 0xFD07, "v128.load8_splat")
O(V128__load16_splat, 0xFD08, "v128.load16_splat")
O(V128__load32_splat, 0xFD09, "v128.load32_splat")
O(V128__load64_splat, 0xFD0A, "v128.load64_splat")
O(V128__load32_zero, 0xFD5C, "v128.load32_zero")
O(V128__load64_zero, 0xFD5D, "v128.load64_zero")
O(V128__store, 0xFD0B, "v128.store")
O(V128__load8_lane, 0xFD54, "v128.load8_lane")
O(V128__load16_lane, 0xFD55, "v128.load16_lane")
O(V128__load32_lane, 0xFD56, "v128.load32_lane")
O(V128__load64_lane, 0xFD57, "v128.load64_lane")
O(V128__store8_lane, 0xFD58, "v128.store8_lane")
O(V128__store16_lane, 0xFD59, "v128.store16_lane")
O(V128__store32_lane, 0xFD5A, "v128.store32_lane")
O(V128__store64_lane, 0xFD5B, "v128.store64_lane")

// SIMD Const Instructions
O(V128__const, 0xFD0C, "v128.const")

// SIMD Shuffle Instructions
O(I8x16__shuffle, 0xFD0D, "i8x16.shuffle")

// SIMD Lane Instructions
O(I8x16__extract_lane_s, 0xFD15, "i8x16.extract_lane_s")
O(I8x16__extract_lane_u, 0xFD16, "i8x16.extract_lane_u")
O(I8x16__replace_lane, 0xFD17, "i8x16.replace_lane")
O(I16x8__extract_lane_s, 0xFD18, "i16x8.extract_lane_s")
O(I16x8__extract_lane_u, 0xFD19, "i16x8.extract_lane_u")
O(I16x8__replace_lane, 0xFD1A, "i16x8.replace_lane")
O(I32x4__extract_lane, 0xFD1B, "i32x4.extract_lane")
O(I32x4__replace_lane, 0xFD1C, "i32x4.replace_lane")
O(I64x2__extract_lane, 0xFD1D, "i64x2.extract_lane")
O(I64x2__replace_lane, 0xFD1E, "i64x2.replace_lane")
O(F32x4__extract_lane, 0xFD1F, "f32x4.extract_lane")
O(F32x4__replace_lane, 0xFD20, "f32x4.replace_lane")
O(F64x2__extract_lane, 0xFD21, "f64x2.extract_lane")
O(F64x2__replace_lane, 0xFD22, "f64x2.replace_lane")

// SIMD Numeric Instructions
O(I8x16__swizzle, 0xFD0E, "i8x16.swizzle")
O(I8x16__splat, 0xFD0F, "i8x16.splat")
O(I16x8__splat, 0xFD10, "i16x8.splat")
O(I32x4__splat, 0xFD11, "i32x4.splat")
O(I64x2__splat, 0xFD12, "i64x2.splat")
O(F32x4__splat, 0xFD13, "f32x4.splat")
O(F64x2__splat, 0xFD14, "f64x2.splat")

O(I8x16__eq, 0xFD23, "i8x16.eq")
O(I8x16__ne, 0xFD24, "i8x16.ne")
O(I8x16__lt_s, 0xFD25, "i8x16.lt_s")
O(I8x16__lt_u, 0xFD26, "i8x16.lt_u")
O(I8x16__gt_s, 0xFD27, "i8x16.gt_s")
O(I8x16__gt_u, 0xFD28, "i8x16.gt_u")
O(I8x16__le_s, 0xFD29, "i8x16.le_s")
O(I8x16__le_u, 0xFD2A, "i8x16.le_u")
O(I8x16__ge_s, 0xFD2B, "i8x16.ge_s")
O(I8x16__ge_u, 0xFD2C, "i8x16.ge_u")

O(I16x8__eq, 0xFD2D, "i16x8.eq")
O(I16x8__ne, 0xFD2E, "i16x8.ne")
O(I16x8__lt_s, 0xFD2F, "i16x8.lt_s")
O(I16x8__lt_u, 0xFD30, "i16x8.lt_u")
O(I16x8__gt_s, 0xFD31, "i16x8.gt_s")
O(I16x8__gt_u, 0xFD32, "i16x8.gt_u")
O(I16x8__le_s, 0xFD33, "i16x8.le_s")
O(I16x8__le_u, 0xFD34, "i16x8.le_u")
O(I16x8__ge_s, 0xFD35, "i16x8.ge_s")
O(I16x8__ge_u, 0xFD36, "i16x8.ge_u")

O(I32x4__eq, 0xFD37, "i32x4.eq")
O(I32x4__ne, 0xFD38, "i32x4.ne")
O(I32x4__lt_s, 0xFD39, "i32x4.lt_s")
O(I32x4__lt_u, 0xFD3A, "i32x4.lt_u")
O(I32x4__gt_s, 0xFD3B, "i32x4.gt_s")
O(I32x4__gt_u, 0xFD3C, "i32x4.gt_u")
O(I32x4__le_s, 0xFD3D, "i32x4.le_s")
O(I32x4__le_u, 0xFD3E, "i32x4.le_u")
O(I32x4__ge_s, 0xFD3F, "i32x4.ge_s")
O(I32x4__ge_u, 0xFD40, "i32x4.ge_u")

O(I64x2__eq, 0xFDD6, "i64x2.eq")
O(I64x2__ne, 0xFDD7, "i64x2.ne")
O(I64x2__lt_s, 0xFDD8, "i64x2.lt_s")
O(I64x2__gt_s, 0xFDD9, "i64x2.gt_s")
O(I64x2__le_s, 0xFDDA, "i64x2.le_s")
O(I64x2__ge_s, 0xFDDB, "i64x2.ge_s")

O(F32x4__eq, 0xFD41, "f32x4.eq")
O(F32x4__ne, 0xFD42, "f32x4.ne")
O(F32x4__lt, 0xFD43, "f32x4.lt")
O(F32x4__gt, 0xFD44, "f32x4.gt")
O(F32x4__le, 0xFD45, "f32x4.le")
O(F32x4__ge, 0xFD46, "f32x4.ge")

O(F64x2__eq, 0xFD47, "f64x2.eq")
O(F64x2__ne, 0xFD48, "f64x2.ne")
O(F64x2__lt, 0xFD49, "f64x2.lt")
O(F64x2__gt, 0xFD4A, "f64x2.gt")
O(F64x2__le, 0xFD4B, "f64x2.le")
O(F64x2__ge, 0xFD4C, "f64x2.ge")

O(V128__not, 0xFD4D, "v128.not")
O(V128__and, 0xFD4E, "v128.and")
O(V128__andnot, 0xFD4F, "v128.andnot")
O(V128__or, 0xFD50, "v128.or")
O(V128__xor, 0xFD51, "v128.xor")
O(V128__bitselect, 0xFD52, "v128.bitselect")
O(V128__any_true, 0xFD53, "v128.any_true")

O(I8x16__abs, 0xFD60, "i8x16.abs")
O(I8x16__neg, 0xFD61, "i8x16.neg")
O(I8x16__popcnt, 0xFD62, "i8x16.popcnt")
O(I8x16__all_true, 0xFD63, "i8x16.all_true")
O(I8x16__bitmask, 0xFD64, "i8x16.bitmask")
O(I8x16__narrow_i16x8_s, 0xFD65, "i8x16.narrow_i16x8_s")
O(I8x16__narrow_i16x8_u, 0xFD66, "i8x16.narrow_i16x8_u")
O(I8x16__shl, 0xFD6B, "i8x16.shl")
O(I8x16__shr_s, 0xFD6C, "i8x16.shr_s")
O(I8x16__shr_u, 0xFD6D, "i8x16.shr_u")
O(I8x16__add, 0xFD6E, "i8x16.add")
O(I8x16__add_sat_s, 0xFD6F, "i8x16.add_sat_s")
O(I8x16__add_sat_u, 0xFD70, "i8x16.add_sat_u")
O(I8x16__sub, 0xFD71, "i8x16.sub")
O(I8x16__sub_sat_s, 0xFD72, "i8x16.sub_sat_s")
O(I8x16__sub_sat_u, 0xFD73, "i8x16.sub_sat_u")
O(I8x16__min_s, 0xFD76, "i8x16.min_s")
O(I8x16__min_u, 0xFD77, "i8x16.min_u")
O(I8x16__max_s, 0xFD78, "i8x16.max_s")
O(I8x16__max_u, 0xFD79, "i8x16.max_u")
O(I8x16__avgr_u, 0xFD7B, "i8x16.avgr_u")

O(I16x8__abs, 0xFD80, "i16x8.abs")
O(I16x8__neg, 0xFD81, "i16x8.neg")
O(I16x8__all_true, 0xFD83, "i16x8.all_true")
O(I16x8__bitmask, 0xFD84, "i16x8.bitmask")
O(I16x8__narrow_i32x4_s, 0xFD85, "i16x8.narrow_i32x4_s")
O(I16x8__narrow_i32x4_u, 0xFD86, "i16x8.narrow_i32x4_u")
O(I16x8__extend_low_i8x16_s, 0xFD87, "i16x8.extend_low_i8x16_s")
O(I16x8__extend_high_i8x16_s, 0xFD88, "i16x8.extend_high_i8x16_s")
O(I16x8__extend_low_i8x16_u, 0xFD89, "i16x8.extend_low_i8x16_u")
O(I16x8__extend_high_i8x16_u, 0xFD8A, "i16x8.extend_high_i8x16_u")
O(I16x8__shl, 0xFD8B, "i16x8.shl")
O(I16x8__shr_s, 0xFD8C, "i16x8.shr_s")
O(I16x8__shr_u, 0xFD8D, "i16x8.shr_u")
O(I16x8__add, 0xFD8E, "i16x8.add")
O(I16x8__add_sat_s, 0xFD8F, "i16x8.add_sat_s")
O(I16x8__add_sat_u, 0xFD90, "i16x8.add_sat_u")
O(I16x8__sub, 0xFD91, "i16x8.sub")
O(I16x8__sub_sat_s, 0xFD92, "i16x8.sub_sat_s")
O(I16x8__sub_sat_u, 0xFD93, "i16x8.sub_sat_u")
O(I16x8__mul, 0xFD95, "i16x8.mul")
O(I16x8__min_s, 0xFD96, "i16x8.min_s")
O(I16x8__min_u, 0xFD97, "i16x8.min_u")
O(I16x8__max_s, 0xFD98, "i16x8.max_s")
O(I16x8__max_u, 0xFD99, "i16x8.max_u")
O(I16x8__avgr_u, 0xFD9B, "i16x8.avgr_u")
O(I16x8__extmul_low_i8x16_s, 0xFD9C, "i16x8.extmul_low_i8x16_s")
O(I16x8__extmul_high_i8x16_s, 0xFD9D, "i16x8.extmul_high_i8x16_s")
O(I16x8__extmul_low_i8x16_u, 0xFD9E, "i16x8.extmul_low_i8x16_u")
O(I16x8__extmul_high_i8x16_u, 0xFD9F, "i16x8.extmul_high_i8x16_u")
O(I16x8__q15mulr_sat_s, 0xFD82, "i16x8.q15mulr_sat_s")
O(I16x8__extadd_pairwise_i8x16_s, 0xFD7C, "i16x8.extadd_pairwise_i8x16_s")
O(I16x8__extadd_pairwise_i8x16_u, 0xFD7D, "i16x8.extadd_pairwise_i8x16_u")

O(I32x4__abs, 0xFDA0, "i32x4.abs")
O(I32x4__neg, 0xFDA1, "i32x4.neg")
O(I32x4__all_true, 0xFDA3, "i32x4.all_true")
O(I32x4__bitmask, 0xFDA4, "i32x4.bitmask")
O(I32x4__extend_low_i16x8_s, 0xFDA7, "i32x4.extend_low_i16x8_s")
O(I32x4__extend_high_i16x8_s, 0xFDA8, "i32x4.extend_high_i16x8_s")
O(I32x4__extend_low_i16x8_u, 0xFDA9, "i32x4.extend_low_i16x8_u")
O(I32x4__extend_high_i16x8_u, 0xFDAA, "i32x4.extend_high_i16x8_u")
O(I32x4__shl, 0xFDAB, "i32x4.shl")
O(I32x4__shr_s, 0xFDAC, "i32x4.shr_s")
O(I32x4__shr_u, 0xFDAD, "i32x4.shr_u")
O(I32x4__add, 0xFDAE, "i32x4.add")
O(I32x4__sub, 0xFDB1, "i32x4.sub")
O(I32x4__mul, 0xFDB5, "i32x4.mul")
O(I32x4__min_s, 0xFDB6, "i32x4.min_s")
O(I32x4__min_u, 0xFDB7, "i32x4.min_u")
O(I32x4__max_s, 0xFDB8, "i32x4.max_s")
O(I32x4__max_u, 0xFDB9, "i32x4.max_u")
O(I32x4__dot_i16x8_s, 0xFDBA, "i32x4.dot_i16x8_s")
O(I32x4__extmul_low_i16x8_s, 0xFDBC, "i32x4.extmul_low_i16x8_s")
O(I32x4__extmul_high_i16x8_s, 0xFDBD, "i32x4.extmul_high_i16x8_s")
O(I32x4__extmul_low_i16x8_u, 0xFDBE, "i32x4.extmul_low_i16x8_u")
O(I32x4__extmul_high_i16x8_u, 0xFDBF, "i32x4.extmul_high_i16x8_u")
O(I32x4__extadd_pairwise_i16x8_s, 0xFD7E, "i32x4.extadd_pairwise_i16x8_s")
O(I32x4__extadd_pairwise_i16x8_u, 0xFD7F, "i32x4.extadd_pairwise_i16x8_u")

O(I64x2__abs, 0xFDC0, "i64x2.abs")
O(I64x2__neg, 0xFDC1, "i64x2.neg")
O(I64x2__all_true, 0xFDC3, "i64x2.all_true")
O(I64x2__bitmask, 0xFDC4, "i64x2.bitmask")
O(I64x2__extend_low_i32x4_s, 0xFDC7, "i64x2.extend_low_i32x4_s")
O(I64x2__extend_high_i32x4_s, 0xFDC8, "i64x2.extend_high_i32x4_s")
O(I64x2__extend_low_i32x4_u, 0xFDC9, "i64x2.extend_low_i32x4_u")
O(I64x2__extend_high_i32x4_u, 0xFDCA, "i64x2.extend_high_i32x4_u")
O(I64x2__shl, 0xFDCB, "i64x2.shl")
O(I64x2__shr_s, 0xFDCC, "i64x2.shr_s")
O(I64x2__shr_u, 0xFDCD, "i64x2.shr_u")
O(I64x2__add, 0xFDCE, "i64x2.add")
O(I64x2__sub, 0xFDD1, "i64x2.sub")
O(I64x2__mul, 0xFDD5, "i64x2.mul")
O(I64x2__extmul_low_i32x4_s, 0xFDDC, "i64x2.extmul_low_i32x4_s")
O(I64x2__extmul_high_i32x4_s, 0xFDDD, "i64x2.extmul_high_i32x4_s")
O(I64x2__extmul_low_i32x4_u, 0xFDDE, "i64x2.extmul_low_i32x4_u")
O(I64x2__extmul_high_i32x4_u, 0xFDDF, "i64x2.extmul_high_i32x4_u")

O(F32x4__abs, 0xFDE0, "f32x4.abs")
O(F32x4__neg, 0xFDE1, "f32x4.neg")
O(F32x4__sqrt, 0xFDE3, "f32x4.sqrt")
O(F32x4__add, 0xFDE4, "f32x4.add")
O(F32x4__sub, 0xFDE5, "f32x4.sub")
O(F32x4__mul, 0xFDE6, "f32x4.mul")
O(F32x4__div, 0xFDE7, "f32x4.div")
O(F32x4__min, 0xFDE8, "f32x4.min")
O(F32x4__max, 0xFDE9, "f32x4.max")
O(F32x4__pmin, 0xFDEA, "f32x4.pmin")
O(F32x4__pmax, 0xFDEB, "f32x4.pmax")
O(F32x4__ceil, 0xFD67, "f32x4.ceil")
O(F32x4__floor, 0xFD68, "f32x4.floor")
O(F32x4__trunc, 0xFD69, "f32x4.trunc")
O(F32x4__nearest, 0xFD6A, "f32x4.nearest")

O(F64x2__abs, 0xFDEC, "f64x2.abs")
O(F64x2__neg, 0xFDED, "f64x2.neg")
O(F64x2__sqrt, 0xFDEF, "f64x2.sqrt")
O(F64x2__add, 0xFDF0, "f64x2.add")
O(F64x2__sub, 0xFDF1, "f64x2.sub")
O(F64x2__mul, 0xFDF2, "f64x2.mul")
O(F64x2__div, 0xFDF3, "f64x2.div")
O(F64x2__min, 0xFDF4, "f64x2.min")
O(F64x2__max, 0xFDF5, "f64x2.max")
O(F64x2__pmin, 0xFDF6, "f64x2.pmin")
O(F64x2__pmax, 0xFDF7, "f64x2.pmax")
O(F64x2__ceil, 0xFD74, "f64x2.ceil")
O(F64x2__floor, 0xFD75, "f64x2.floor")
O(F64x2__trunc, 0xFD7A, "f64x2.trunc")
O(F64x2__nearest, 0xFD94, "f64x2.nearest")

O(I32x4__trunc_sat_f32x4_s, 0xFDF8, "i32x4.trunc_sat_f32x4_s")
O(I32x4__trunc_sat_f32x4_u, 0xFDF9, "i32x4.trunc_sat_f32x4_u")
O(F32x4__convert_i32x4_s, 0xFDFA, "f32x4.convert_i32x4_s")
O(F32x4__convert_i32x4_u, 0xFDFB, "f32x4.convert_i32x4_u")
O(I32x4__trunc_sat_f64x2_s_zero, 0xFDFC, "i32x4.trunc_sat_f64x2_s_zero")
O(I32x4__trunc_sat_f64x2_u_zero, 0xFDFD, "i32x4.trunc_sat_f64x2_u_zero")
O(F64x2__convert_low_i32x4_s, 0xFDFE, "f64x2.convert_low_i32x4_s")
O(F64x2__convert_low_i32x4_u, 0xFDFF, "f64x2.convert_low_i32x4_u")
O(F32x4__demote_f64x2_zero, 0xFD5E, "f32x4.demote_f64x2_zero")
O(F64x2__promote_low_f32x4, 0xFD5F, "f64x2.promote_low_f32x4")

// Atomic instructions
O(Memory__atomic__notify, 0xFE00, "memory.atomic.notify")
O(Memory__atomic__wait32, 0xFE01, "memory.atomic.wait32")
O(Memory__atomic__wait64, 0xFE02, "memory.atomic.wait64")
O(Atomic__fence, 0xFE03, "atomic.fence")

O(I32__atomic__load, 0xFE10, "i32.atomic.load")
O(I64__atomic__load, 0xFE11, "i64.atomic.load")
O(I32__atomic__load8_u, 0xFE12, "i32.atomic.load8_u")
O(I32__atomic__load16_u, 0xFE13, "i32.atomic.load16_u")
O(I64__atomic__load8_u, 0xFE14, "i64.atomic.load8_u")
O(I64__atomic__load16_u, 0xFE15, "i64.atomic.load16_u")
O(I64__atomic__load32_u, 0xFE16, "i64.atomic.load32_u")
O(I32__atomic__store, 0xFE17, "i32.atomic.store")
O(I64__atomic__store, 0xFE18, "i64.atomic.store")
O(I32__atomic__store8, 0xFE19, "i32.atomic.store8")
O(I32__atomic__store16, 0xFE1A, "i32.atomic.store16")
O(I64__atomic__store8, 0xFE1B, "i64.atomic.store8")
O(I64__atomic__store16, 0xFE1C, "i64.atomic.store16")
O(I64__atomic__store32, 0xFE1D, "i64.atomic.store32")

O(I32__atomic__rmw__add, 0xFE1E, "i32.atomic.rmw.add")
O(I64__atomic__rmw__add, 0xFE1F, "i64.atomic.rmw.add")
O(I32__atomic__rmw8__add_u, 0xFE20, "i32.atomic.rmw8.add_u")
O(I32__atomic__rmw16__add_u, 0xFE21, "i32.atomic.rmw16.add_u")
O(I64__atomic__rmw8__add_u, 0xFE22, "i64.atomic.rmw8.add_u")
O(I64__atomic__rmw16__add_u, 0xFE23, "i64.atomic.rmw16.add_u")
O(I64__atomic__rmw32__add_u, 0xFE24, "i64.atomic.rmw32.add_u")
O(I32__atomic__rmw__sub, 0xFE25, "i32.atomic.rmw.sub")
O(I64__atomic__rmw__sub, 0xFE26, "i64.atomic.rmw.sub")
O(I32__atomic__rmw8__sub_u, 0xFE27, "i32.atomic.rmw8.sub_u")
O(I32__atomic__rmw16__sub_u, 0xFE28, "i32.atomic.rmw16.sub_u")
O(I64__atomic__rmw8__sub_u, 0xFE29, "i64.atomic.rmw8.sub_u")
O(I64__atomic__rmw16__sub_u, 0xFE2A, "i64.atomic.rmw16.sub_u")
O(I64__atomic__rmw32__sub_u, 0xFE2B, "i64.atomic.rmw32.sub_u")
O(I32__atomic__rmw__and, 0xFE2C, "i32.atomic.rmw.and")
O(I64__atomic__rmw__and, 0xFE2D, "i64.atomic.rmw.and")
O(I32__atomic__rmw8__and_u, 0xFE2E, "i32.atomic.rmw8.and_u")
O(I32__atomic__rmw16__and_u, 0xFE2F, "i32.atomic.rmw16.and_u")
O(I64__atomic__rmw8__and_u, 0xFE30, "i64.atomic.rmw8.and_u")
O(I64__atomic__rmw16__and_u, 0xFE31, "i64.atomic.rmw16.and_u")
O(I64__atomic__rmw32__and_u, 0xFE32, "i64.atomic.rmw32.and_u")
O(I32__atomic__rmw__or, 0xFE33, "i32.atomic.rmw.or")
O(I64__atomic__rmw__or, 0xFE34, "i64.atomic.rmw.or")
O(I32__atomic__rmw8__or_u, 0xFE35, "i32.atomic.rmw8.or_u")
O(I32__atomic__rmw16__or_u, 0xFE36, "i32.atomic.rmw16.or_u")
O(I64__atomic__rmw8__or_u, 0xFE37, "i64.atomic.rmw8.or_u")
O(I64__atomic__rmw16__or_u, 0xFE38, "i64.atomic.rmw16.or_u")
O(I64__atomic__rmw32__or_u, 0xFE39, "i64.atomic.rmw32.or_u")
O(I32__atomic__rmw__xor, 0xFE3A, "i32.atomic.rmw.xor")
O(I64__atomic__rmw__xor, 0xFE3B, "i64.atomic.rmw.xor")
O(I32__atomic__rmw8__xor_u, 0xFE3C, "i32.atomic.rmw8.xor_u")
O(I32__atomic__rmw16__xor_u, 0xFE3D, "i32.atomic.rmw16.xor_u")
O(I64__atomic__rmw8__xor_u, 0xFE3E, "i64.atomic.rmw8.xor_u")
O(I64__atomic__rmw16__xor_u, 0xFE3F, "i64.atomic.rmw16.xor_u")
O(I64__atomic__rmw32__xor_u, 0xFE40, "i64.atomic.rmw32.xor_u")
O(I32__atomic__rmw__xchg, 0xFE41, "i32.atomic.rmw.xchg")
O(I64__atomic__rmw__xchg, 0xFE42, "i64.atomic.rmw.xchg")
O(I32__atomic__rmw8__xchg_u, 0xFE43, "i32.atomic.rmw8.xchg_u")
O(I32__atomic__rmw16__xchg_u, 0xFE44, "i32.atomic.rmw16.xchg_u")
O(I64__atomic__rmw8__xchg_u, 0xFE45, "i64.atomic.rmw8.xchg_u")
O(I64__atomic__rmw16__xchg_u, 0xFE46, "i64.atomic.rmw16.xchg_u")
O(I64__atomic__rmw32__xchg_u, 0xFE47, "i64.atomic.rmw32.xchg_u")
O(I32__atomic__rmw__cmpxchg, 0xFE48, "i32.atomic.rmw.cmpxchg")
O(I64__atomic__rmw__cmpxchg, 0xFE49, "i64.atomic.rmw.cmpxchg")
O(I32__atomic__rmw8__cmpxchg_u, 0xFE4A, "i32.atomic.rmw8.cmpxchg_u")
O(I32__atomic__rmw16__cmpxchg_u, 0xFE4B, "i32.atomic.rmw16.cmpxchg_u")
O(I64__atomic__rmw8__cmpxchg_u, 0xFE4C, "i64.atomic.rmw8.cmpxchg_u")
O(I64__atomic__rmw16__cmpxchg_u, 0xFE4D, "i64.atomic.rmw16.cmpxchg_u")
O(I64__atomic__rmw32__cmpxchg_u, 0xFE4E, "i64.atomic.rmw32.cmpxchg_u")

#undef O
#endif // UseOpCode

// enum_configure.h

#ifdef UseProposal
#define P Line
P(ImportExportMutGlobals, "Import/Export of mutable globals")
P(NonTrapFloatToIntConversions, "Non-trapping float-to-int conversions")
P(SignExtensionOperators, "Sign-extension operators")
P(MultiValue, "Multi-value returns")
P(BulkMemoryOperations, "Bulk memory operations")
P(ReferenceTypes, "Reference types")
P(SIMD, "Fixed-width SIMD")
P(TailCall, "Tail call")
P(MultiMemories, "Multiple memories")
P(Annotations, "Custom Annotation Syntax in the Text Format")
P(Memory64, "Memory64")
P(ExceptionHandling, "Exception handling")
P(ExtendedConst, "Extended const")
P(Threads, "Threads")
P(FunctionReferences, "Typed Function References")
#undef P
#endif // UseProposal

#ifdef UseHostRegistration
#define H Line
H(Wasi)
#undef H
#endif // UseHostRegistration

// enum_errcode.h

#ifdef UseErrCategory
#define C Line

C(WASM, 0x00)
C(UserLevelError, 0x01)

#undef C
#endif // UseErrCategory

#ifdef UseErrCode
#define E Line

E(Success, 0x0000, "success")
// Exit and return success.
E(Terminated, 0x0001, "terminated")
// Generic runtime error.
E(RuntimeError, 0x0002, "generic runtime error")
// Exceeded cost limit (out of gas).
E(CostLimitExceeded, 0x0003, "cost limit exceeded")
// Wrong VM's workflow
E(WrongVMWorkflow, 0x0004, "wrong VM workflow")
// Wasm function not found
E(FuncNotFound, 0x0005, "wasm function not found")
// AOT runtime is disabled
E(AOTDisabled, 0x0006, "AOT runtime is disabled in this build")
// Execution interrupted
E(Interrupted, 0x0007, "execution interrupted")
// Not validated module
E(NotValidated, 0x0008, "wasm module hasn't passed validation yet")
// Non-Null value is required
E(NonNullRequired, 0x0009, "set null value into non-nullable value type")
// Unable to set value due to const
E(SetValueToConst, 0x000A, "set value into const")
// Set value failed due to mismatch value type
E(SetValueErrorType, 0x000B, "set value type mismatch")
// User defined error
E(UserDefError, 0x000C, "user defined error code")

// Load phase
// @{
// File not found
E(IllegalPath, 0x0100, "invalid path")
// Error when reading
E(ReadError, 0x0101, "read error")
// Reach end of file when reading
E(UnexpectedEnd, 0x0102, "unexpected end")
// Not detected magic header
E(MalformedMagic, 0x0103, "magic header not detected")
// Unsupported version
E(MalformedVersion, 0x0104, "unknown binary version")
// Malformed section ID
E(MalformedSection, 0x0105, "malformed section id")
// Section size mismatched
E(SectionSizeMismatch, 0x0106, "section size mismatch")
// Length out of bounds
E(LengthOutOfBounds, 0x0107, "length out of bounds")
// Junk sections
E(JunkSection, 0x0108, "unexpected content after last section")
// Incompatible function and code section
E(IncompatibleFuncCode, 0x0109,
  "function and code section have inconsistent lengths")
// Incompatible data and datacount section
E(IncompatibleDataCount, 0x010A,
  "data count and data section have inconsistent lengths")
// Datacount section required
E(DataCountRequired, 0x010B, "data count section required")
// Malformed import kind
E(MalformedImportKind, 0x010C, "malformed import kind")
// Malformed export kind
E(MalformedExportKind, 0x010D, "malformed export kind")
// Not loaded an expected zero byte
E(ExpectedZeroByte, 0x010E, "zero byte expected")
// Malformed mutability
E(InvalidMut, 0x010F, "malformed mutability")
// Local size too large
E(TooManyLocals, 0x0110, "too many locals")
// Malformed value type
E(MalformedValType, 0x0111, "malformed value type")
// Malformed element type (Bulk-mem proposal)
E(MalformedElemType, 0x0112, "malformed element type")
// Malformed reference type (Ref-types proposal)
E(MalformedRefType, 0x0113, "malformed reference type")
// Invalid utf-8 encoding
E(MalformedUTF8, 0x0114, "malformed UTF-8 encoding")
// Invalid too large integer
E(IntegerTooLarge, 0x0115, "integer too large")
// Invalid presentation too long integer
E(IntegerTooLong, 0x0116, "integer representation too long")
// Illegal OpCode
E(IllegalOpCode, 0x0117, "illegal opcode")
// END OpCode expected
E(ENDCodeExpected, 0x0118, "END opcode expected")
// Parsing error
E(IllegalGrammar, 0x0119, "invalid wasm grammar")
// Shared memory must have max
E(SharedMemoryNoMax, 0x011A, "shared memory must have maximum")
// Intrinsics table not found
E(IntrinsicsTableNotFound, 0x011B, "intrinsics table not found")
// Malformed table (Ref-types proposal)
E(MalformedTable, 0x011C, "malformed table")
// @}

// Validation phase
// @{
// Alignment > natural
E(InvalidAlignment, 0x0200, "alignment must not be larger than natural")
// Got unexpected type when checking
E(TypeCheckFailed, 0x0201, "type mismatch")
// Branch to unknown label index
E(InvalidLabelIdx, 0x0202, "unknown label")
// Access unknown local index
E(InvalidLocalIdx, 0x0203, "unknown local")
// Access unknown field index
E(InvalidFieldIdx, 0x0204, "unknown field")
// Type index not defined
E(InvalidFuncTypeIdx, 0x0205, "unknown type")
// Function index not defined
E(InvalidFuncIdx, 0x0206, "unknown function")
// Table index not defined
E(InvalidTableIdx, 0x0207, "unknown table")
// Memory index not defined
E(InvalidMemoryIdx, 0x0208, "unknown memory")
// Global index not defined
E(InvalidGlobalIdx, 0x0209, "unknown global")
// Element segment index not defined
E(InvalidElemIdx, 0x020A, "unknown elem segment")
// Data segment index not defined
E(InvalidDataIdx, 0x020B, "unknown data segment")
// Undeclared reference
E(InvalidRefIdx, 0x020C, "undeclared function reference")
// Should be constant expression
E(ConstExprRequired, 0x020D, "constant expression required")
// Export name conflicted
E(DupExportName, 0x020E, "duplicate export name")
// Tried to store to const global value
E(ImmutableGlobal, 0x020F, "global is immutable")
// Tried to store to const field of structure
E(ImmutableField, 0x0210, "field is immutable")
// Tried to store to const array
E(ImmutableArray, 0x0211, "array is immutable")
// Invalid result arity in select t* instruction
E(InvalidResultArity, 0x0212, "invalid result arity")
// #Tables > 1 (without Ref-types proposal)
E(MultiTables, 0x0213, "multiple tables")
// #Memories > 1
E(MultiMemories, 0x0214, "multiple memories")
// Invalid Limit grammar
E(InvalidLimit, 0x0215, "size minimum must not be greater than maximum")
// Memory pages > 65536
E(InvalidMemPages, 0x0216, "memory size must be at most 65536 pages (4GiB)")
// Invalid start function signature
E(InvalidStartFunc, 0x0217, "start function")
// Invalid lane index
E(InvalidLaneIdx, 0x0218, "invalid lane index")
// Invalid uninitialized local
E(InvalidUninitLocal, 0x0219, "uninitialized local")
// Defaultable field type required
E(InvalidNotDefaultableField, 0x021A, "field type is not defaultable")
// Defaultable array type required
E(InvalidNotDefaultableArray, 0x021B, "array type is not defaultable")
// Unpacked field type required, but got packed one
E(InvalidPackedField, 0x021C, "field is packed")
// Unpacked array type required, but got packed one
E(InvalidPackedArray, 0x021D, "array is packed")
// Packed field type required, but got unpacked one
E(InvalidUnpackedField, 0x021E, "field is unpacked")
// Packed array type required, but got unpacked one
E(InvalidUnpackedArray, 0x021F, "array is unpacked")
// Invalid Br ref type
E(InvalidBrRefType, 0x0220, "invalid br ref type")
// 2 array types not matched
E(ArrayTypesMismatch, 0x0221, "array types do not match")
// Should be numeric type in array type
E(ArrayTypesNumtypeRequired, 0x0222, "array type is not numeric or vector")
// Sub type matching or validation failed
E(InvalidSubType, 0x0223, "sub type")
// @}

// Instantiation phase
// @{
// Module name conflicted when importing.
E(ModuleNameConflict, 0x0300, "module name conflict")
// Import matching failed
E(IncompatibleImportType, 0x0301, "incompatible import type")
// Unknown import instances
E(UnknownImport, 0x0302, "unknown import")
// Init failed when instantiating data segment
E(DataSegDoesNotFit, 0x0303, "data segment does not fit")
// Init failed when instantiating element segment
E(ElemSegDoesNotFit, 0x0304, "elements segment does not fit")
// @}

// Execution phase
// @{
// Wrong access of instances addresses
E(WrongInstanceAddress, 0x0400, "wrong instance address")
// Wrong access of instances indices
E(WrongInstanceIndex, 0x0401, "wrong instance index")
// Instruction type not match
E(InstrTypeMismatch, 0x0402, "instruction type mismatch")
// Function signature not match when invoking
E(FuncSigMismatch, 0x0403, "function signature mismatch")
// Divide by zero
E(DivideByZero, 0x0404, "integer divide by zero")
// Integer overflow
E(IntegerOverflow, 0x0405, "integer overflow")
// Cannot do convert to integer
E(InvalidConvToInt, 0x0406, "invalid conversion to integer")
// Out of bounds table access
E(TableOutOfBounds, 0x0407, "out of bounds table access")
// Out of bounds memory access
E(MemoryOutOfBounds, 0x0408, "out of bounds memory access")
// Out of bounds array access
E(ArrayOutOfBounds, 0x0409, "out of bounds array access")
// Meet an unreachable instruction
E(Unreachable, 0x040A, "unreachable")
// Uninitialized element in table instance
E(UninitializedElement, 0x040B, "uninitialized element")
// Access undefined element in table instances
E(UndefinedElement, 0x040C, "undefined element")
// Func type mismatch in call_indirect
E(IndirectCallTypeMismatch, 0x040D, "indirect call type mismatch")
// Host function execution failed
E(HostFuncError, 0x040E, "host function failed")
// Reference type not match
E(RefTypeMismatch, 0x040F, "reference type mismatch")
// Unaligned atomic memory access
E(UnalignedAtomicAccess, 0x0410, "unaligned atomic")
// wait32/wait64 on unshared memory
E(ExpectSharedMemory, 0x0411, "expected shared memory")
// Cast null pointer to non null
E(CastNullToNonNull, 0x0412, "null reference")
// Access to null function reference
E(AccessNullFunc, 0x0413, "null function reference")
// Access to null structure reference
E(AccessNullStruct, 0x0414, "null structure reference")
// Access to null array reference
E(AccessNullArray, 0x0415, "null array reference")
// Access to null i31 reference
E(AccessNullI31, 0x0416, "null i31 reference")
// Fail to cast reference
E(CastFailed, 0x0417, "cast failure")
// @}

#undef E
#endif // UseErrCode

#ifdef UseWasmPhase
#define P Line

P(WasmEdge, 0x00, "wasmedge runtime")
P(Loading, 0x01, "loading")
P(Validation, 0x02, "validation")
P(Instantiation, 0x03, "instantiation")
P(Execution, 0x04, "execution")
P(UserDefined, 0x05, "user defined")

#undef P
#endif // UseWasmPhase

// enum_errinfo.hpp

#ifdef UsePtrType
#define P Line

P(Index, "index")     // Index of instances
P(Address, "address") // Absolute address

#undef P
#endif // UsePtrType

#ifdef UseMismatchCategory
#define M Line

M(Alignment, "memory alignment") // Alignment in memory instructions
M(ValueType, "value type")       // Value type
M(ValueTypes, "value types")     // Value type list
M(Mutation, "mutation")          // Const or Var
M(ExternalType, "external type") // External typing
M(FunctionType, "function type") // Function type
M(Table, "table")                // Table instance
M(Memory, "memory")              // Memory instance
M(Global, "global")              // Global instance
M(Version, "version")            // Versions

#undef M
#endif // UseMismatchCategory

#ifdef UseIndexCategory
#define I Line

I(Label, "label")
I(Local, "local")
I(FunctionType, "function type")
I(Function, "function")
I(Table, "table")
I(Memory, "memory")
I(Global, "global")
I(Element, "element")
I(Data, "data")
I(Lane, "lane")

#undef I
#endif // UseIndexCategory

// enum_types.h

#ifdef UseTypeCode
#define T Line

T(TypeIndex, 0x00, "type_index")   // 0x00 reserved for type index case
T(I32, 0x7F, "i32")                // -0x01 for number type
T(I64, 0x7E, "i64")                // -0x02 for number type
T(F32, 0x7D, "f32")                // -0x03 for number type
T(F64, 0x7C, "f64")                // -0x04 for number type
T(V128, 0x7B, "v128")              // -0x05 for vector type
T(I8, 0x78, "i8")                  // -0x08 for packed type
T(I16, 0x77, "i16")                // -0x09 for packed type
T(NullFuncRef, 0x73, "nofunc")     // -0x0D for heap type
T(NullExternRef, 0x72, "noextern") // -0x0E for heap type
T(NullRef, 0x71, "none")           // -0x0F for heap type
T(FuncRef, 0x70, "func")           // -0x10 for heap type
T(ExternRef, 0x6F, "extern")       // -0x11 for heap type
T(AnyRef, 0x6E, "any")             // -0x12 for heap type
T(EqRef, 0x6D, "eq")               // -0x13 for heap type
T(I31Ref, 0x6C, "i31")             // -0x14 for heap type
T(StructRef, 0x6B, "struct")       // -0x15 for heap type
T(ArrayRef, 0x6A, "array")         // -0x16 for heap type
T(Ref, 0x64, "ref")                // -0x1C for reference type
T(RefNull, 0x63, "ref_null")       // -0x1D for reference type
T(Func, 0x60, "func")              // -0x20 for composite type
T(Struct, 0x5F, "struct")          // -0x21 for composite type
T(Array, 0x5E, "array")            // -0x22 for composite type
T(Sub, 0x50, "sub")                // -0x30 for sub type
T(SubFinal, 0x4F, "sub_final")     // -0x31 for sub type
T(Rec, 0x4E, "rec")                // -0x32 for recursive type
T(Epsilon, 0x40, "-")              // -0x40 for result type

#undef T
#endif // UseTypeCode

#ifdef UseValMut
#define M Line

M(Const, 0x00, "const")
M(Var, 0x01, "var")

#undef M
#endif // UseValMut

#ifdef UseExternalType
#define E Line

E(Function, 0x00U, "function")
E(Table, 0x01U, "table")
E(Memory, 0x02U, "memory")
E(Global, 0x03U, "global")

#undef E
#endif // UseExternalType
