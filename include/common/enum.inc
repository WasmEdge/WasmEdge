// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
A(Sec_CoreMod, "component core:module section")
A(Sec_CoreInstance, "component core:instance section")
A(Sec_Alias, "component alias section")
A(Sec_Comp, "nested component section")
A(Sec_Instance, "component instance section")
A(Sec_CompType, "component type section")
A(Sec_Canon, "component canonical section")
A(Sec_CompStart, "component start section")
A(Sec_CompImport, "component import section")
A(Sec_CompExport, "component export section")
A(ImportExportName, "import/export name")
A(Import, "import")
A(Export, "export")
A(Canonical, "canonical")
A(Alias, "alias")
A(DefType, "component defined type")
A(InstanceDecl, "instance decl")
A(ExternDesc, "extern desc")
A(Name, "import/export name")
A(AliasTarget, "alias target")
A(Sort, "sort")
A(Instance, "instance expr")
A(CoreInstance, "core:instance expr")
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
A(Sec_Tag, "tag section")
A(Desc_Import, "import description")
A(Desc_Export, "export description")
A(Seg_Table, "table segment")
A(Seg_Global, "global segment")
A(Seg_Element, "element segment")
A(Seg_Code, "code segment")
A(Seg_Data, "data segment")
A(Type_Module, "module type")
A(Type_Rec, "recursive type")
A(Type_Sub, "sub type")
A(Type_Limit, "limit")
A(Type_Function, "function type")
A(Type_Memory, "memory type")
A(Type_Table, "table type")
A(Type_Global, "global type")
A(Type_Instance, "instance type")
A(Type_Component, "component type")
A(Expression, "expression")
A(Instruction, "instruction")
A(Sec_AOT, "aot section")

#undef A
#endif // UseASTNodeAttr

#ifdef UseOpCode
#define O Line
#define OFB Line_FB
#define OFC Line_FC
#define OFD Line_FD
#define OFE Line_FE

// OpCode:
//   NAME | STRING | CODE [ | EXTEND ]

// Control instructions (part 1)
O(Unreachable, "unreachable", 0x00)
O(Nop, "nop", 0x01)
O(Block, "block", 0x02)
O(Loop, "loop", 0x03)
O(If, "if", 0x04)
O(Else, "else", 0x05)
O(Try, "try", 0x06)     // For legacy EH proposal
O(Catch, "catch", 0x07) // For legacy EH proposal
O(Throw, "throw", 0x08)
O(Rethrow, "rethrow", 0x09) // For legacy EH proposal
O(Throw_ref, "throw_ref", 0x0A)
O(End, "end", 0x0B)
O(Br, "br", 0x0C)
O(Br_if, "br_if", 0x0D)
O(Br_table, "br_table", 0x0E)
O(Return, "return", 0x0F)
O(Call, "call", 0x10)
O(Call_indirect, "call_indirect", 0x11)
O(Return_call, "return_call", 0x12)
O(Return_call_indirect, "return_call_indirect", 0x13)
O(Call_ref, "call_ref", 0x14)
O(Return_call_ref, "return_call_ref", 0x15)
// 0x16: Reserved
// 0x17: Reserved
O(Delegate, "delegate", 0x18)   // For legacy EH proposal
O(Catch_all, "catch_all", 0x19) // For legacy EH proposal

// Parametric Instructions
O(Drop, "drop", 0x1A)
O(Select, "select", 0x1B)
O(Select_t, "select", 0x1C)
// 0x1D: Reserved
// 0x1E: Reserved

// Control instructions (part 2)
O(Try_table, "try_table", 0x1F)

// Variable Instructions
O(Local__get, "local.get", 0x20)
O(Local__set, "local.set", 0x21)
O(Local__tee, "local.tee", 0x22)
O(Global__get, "global.get", 0x23)
O(Global__set, "global.set", 0x24)

// Table Instructions (part 1)
O(Table__get, "table.get", 0x25)
O(Table__set, "table.set", 0x26)
// 0x27: Reserved

// Memory Instructions (part 1)
O(I32__load, "i32.load", 0x28)
O(I64__load, "i64.load", 0x29)
O(F32__load, "f32.load", 0x2A)
O(F64__load, "f64.load", 0x2B)
O(I32__load8_s, "i32.load8_s", 0x2C)
O(I32__load8_u, "i32.load8_u", 0x2D)
O(I32__load16_s, "i32.load16_s", 0x2E)
O(I32__load16_u, "i32.load16_u", 0x2F)
O(I64__load8_s, "i64.load8_s", 0x30)
O(I64__load8_u, "i64.load8_u", 0x31)
O(I64__load16_s, "i64.load16_s", 0x32)
O(I64__load16_u, "i64.load16_u", 0x33)
O(I64__load32_s, "i64.load32_s", 0x34)
O(I64__load32_u, "i64.load32_u", 0x35)
O(I32__store, "i32.store", 0x36)
O(I64__store, "i64.store", 0x37)
O(F32__store, "f32.store", 0x38)
O(F64__store, "f64.store", 0x39)
O(I32__store8, "i32.store8", 0x3A)
O(I32__store16, "i32.store16", 0x3B)
O(I64__store8, "i64.store8", 0x3C)
O(I64__store16, "i64.store16", 0x3D)
O(I64__store32, "i64.store32", 0x3E)
O(Memory__size, "memory.size", 0x3F)
O(Memory__grow, "memory.grow", 0x40)

// Const numeric instructions
O(I32__const, "i32.const", 0x41)
O(I64__const, "i64.const", 0x42)
O(F32__const, "f32.const", 0x43)
O(F64__const, "f64.const", 0x44)

// Numeric instructions
O(I32__eqz, "i32.eqz", 0x45)
O(I32__eq, "i32.eq", 0x46)
O(I32__ne, "i32.ne", 0x47)
O(I32__lt_s, "i32.lt_s", 0x48)
O(I32__lt_u, "i32.lt_u", 0x49)
O(I32__gt_s, "i32.gt_s", 0x4A)
O(I32__gt_u, "i32.gt_u", 0x4B)
O(I32__le_s, "i32.le_s", 0x4C)
O(I32__le_u, "i32.le_u", 0x4D)
O(I32__ge_s, "i32.ge_s", 0x4E)
O(I32__ge_u, "i32.ge_u", 0x4F)
O(I64__eqz, "i64.eqz", 0x50)
O(I64__eq, "i64.eq", 0x51)
O(I64__ne, "i64.ne", 0x52)
O(I64__lt_s, "i64.lt_s", 0x53)
O(I64__lt_u, "i64.lt_u", 0x54)
O(I64__gt_s, "i64.gt_s", 0x55)
O(I64__gt_u, "i64.gt_u", 0x56)
O(I64__le_s, "i64.le_s", 0x57)
O(I64__le_u, "i64.le_u", 0x58)
O(I64__ge_s, "i64.ge_s", 0x59)
O(I64__ge_u, "i64.ge_u", 0x5A)
O(F32__eq, "f32.eq", 0x5B)
O(F32__ne, "f32.ne", 0x5C)
O(F32__lt, "f32.lt", 0x5D)
O(F32__gt, "f32.gt", 0x5E)
O(F32__le, "f32.le", 0x5F)
O(F32__ge, "f32.ge", 0x60)
O(F64__eq, "f64.eq", 0x61)
O(F64__ne, "f64.ne", 0x62)
O(F64__lt, "f64.lt", 0x63)
O(F64__gt, "f64.gt", 0x64)
O(F64__le, "f64.le", 0x65)
O(F64__ge, "f64.ge", 0x66)
O(I32__clz, "i32.clz", 0x67)
O(I32__ctz, "i32.ctz", 0x68)
O(I32__popcnt, "i32.popcnt", 0x69)
O(I32__add, "i32.add", 0x6A)
O(I32__sub, "i32.sub", 0x6B)
O(I32__mul, "i32.mul", 0x6C)
O(I32__div_s, "i32.div_s", 0x6D)
O(I32__div_u, "i32.div_u", 0x6E)
O(I32__rem_s, "i32.rem_s", 0x6F)
O(I32__rem_u, "i32.rem_u", 0x70)
O(I32__and, "i32.and", 0x71)
O(I32__or, "i32.or", 0x72)
O(I32__xor, "i32.xor", 0x73)
O(I32__shl, "i32.shl", 0x74)
O(I32__shr_s, "i32.shr_s", 0x75)
O(I32__shr_u, "i32.shr_u", 0x76)
O(I32__rotl, "i32.rotl", 0x77)
O(I32__rotr, "i32.rotr", 0x78)
O(I64__clz, "i64.clz", 0x79)
O(I64__ctz, "i64.ctz", 0x7A)
O(I64__popcnt, "i64.popcnt", 0x7B)
O(I64__add, "i64.add", 0x7C)
O(I64__sub, "i64.sub", 0x7D)
O(I64__mul, "i64.mul", 0x7E)
O(I64__div_s, "i64.div_s", 0x7F)
O(I64__div_u, "i64.div_u", 0x80)
O(I64__rem_s, "i64.rem_s", 0x81)
O(I64__rem_u, "i64.rem_u", 0x82)
O(I64__and, "i64.and", 0x83)
O(I64__or, "i64.or", 0x84)
O(I64__xor, "i64.xor", 0x85)
O(I64__shl, "i64.shl", 0x86)
O(I64__shr_s, "i64.shr_s", 0x87)
O(I64__shr_u, "i64.shr_u", 0x88)
O(I64__rotl, "i64.rotl", 0x89)
O(I64__rotr, "i64.rotr", 0x8A)
O(F32__abs, "f32.abs", 0x8B)
O(F32__neg, "f32.neg", 0x8C)
O(F32__ceil, "f32.ceil", 0x8D)
O(F32__floor, "f32.floor", 0x8E)
O(F32__trunc, "f32.trunc", 0x8F)
O(F32__nearest, "f32.nearest", 0x90)
O(F32__sqrt, "f32.sqrt", 0x91)
O(F32__add, "f32.add", 0x92)
O(F32__sub, "f32.sub", 0x93)
O(F32__mul, "f32.mul", 0x94)
O(F32__div, "f32.div", 0x95)
O(F32__min, "f32.min", 0x96)
O(F32__max, "f32.max", 0x97)
O(F32__copysign, "f32.copysign", 0x98)
O(F64__abs, "f64.abs", 0x99)
O(F64__neg, "f64.neg", 0x9A)
O(F64__ceil, "f64.ceil", 0x9B)
O(F64__floor, "f64.floor", 0x9C)
O(F64__trunc, "f64.trunc", 0x9D)
O(F64__nearest, "f64.nearest", 0x9E)
O(F64__sqrt, "f64.sqrt", 0x9F)
O(F64__add, "f64.add", 0xA0)
O(F64__sub, "f64.sub", 0xA1)
O(F64__mul, "f64.mul", 0xA2)
O(F64__div, "f64.div", 0xA3)
O(F64__min, "f64.min", 0xA4)
O(F64__max, "f64.max", 0xA5)
O(F64__copysign, "f64.copysign", 0xA6)
O(I32__wrap_i64, "i32.wrap_i64", 0xA7)
O(I32__trunc_f32_s, "i32.trunc_f32_s", 0xA8)
O(I32__trunc_f32_u, "i32.trunc_f32_u", 0xA9)
O(I32__trunc_f64_s, "i32.trunc_f64_s", 0xAA)
O(I32__trunc_f64_u, "i32.trunc_f64_u", 0xAB)
O(I64__extend_i32_s, "i64.extend_i32_s", 0xAC)
O(I64__extend_i32_u, "i64.extend_i32_u", 0xAD)
O(I64__trunc_f32_s, "i64.trunc_f32_s", 0xAE)
O(I64__trunc_f32_u, "i64.trunc_f32_u", 0xAF)
O(I64__trunc_f64_s, "i64.trunc_f64_s", 0xB0)
O(I64__trunc_f64_u, "i64.trunc_f64_u", 0xB1)
O(F32__convert_i32_s, "f32.convert_i32_s", 0xB2)
O(F32__convert_i32_u, "f32.convert_i32_u", 0xB3)
O(F32__convert_i64_s, "f32.convert_i64_s", 0xB4)
O(F32__convert_i64_u, "f32.convert_i64_u", 0xB5)
O(F32__demote_f64, "f32.demote_f64", 0xB6)
O(F64__convert_i32_s, "f64.convert_i32_s", 0xB7)
O(F64__convert_i32_u, "f64.convert_i32_u", 0xB8)
O(F64__convert_i64_s, "f64.convert_i64_s", 0xB9)
O(F64__convert_i64_u, "f64.convert_i64_u", 0xBA)
O(F64__promote_f32, "f64.promote_f32", 0xBB)
O(I32__reinterpret_f32, "i32.reinterpret_f32", 0xBC)
O(I64__reinterpret_f64, "i64.reinterpret_f64", 0xBD)
O(F32__reinterpret_i32, "f32.reinterpret_i32", 0xBE)
O(F64__reinterpret_i64, "f64.reinterpret_i64", 0xBF)
O(I32__extend8_s, "i32.extend8_s", 0xC0)
O(I32__extend16_s, "i32.extend16_s", 0xC1)
O(I64__extend8_s, "i64.extend8_s", 0xC2)
O(I64__extend16_s, "i64.extend16_s", 0xC3)
O(I64__extend32_s, "i64.extend32_s", 0xC4)
// 0xC5 ~ 0xCF: Reserved

// Reference Instructions
O(Ref__null, "ref.null", 0xD0)
O(Ref__is_null, "ref.is_null", 0xD1)
O(Ref__func, "ref.func", 0xD2)
O(Ref__eq, "ref.eq", 0xD3)
O(Ref__as_non_null, "ref.as_non_null", 0xD4)

// Control Instructions (part 3)
O(Br_on_null, "br_on_null", 0xD5)
O(Br_on_non_null, "br_on_non_null", 0xD6)
// 0xD7 ~ 0xFA: Reserved

// 0xFB prefix - GC Instructions
OFB(Struct__new, "struct.new", 0xFB, 0)
OFB(Struct__new_default, "struct.new_default", 0xFB, 1)
OFB(Struct__get, "struct.get", 0xFB, 2)
OFB(Struct__get_s, "struct.get_s", 0xFB, 3)
OFB(Struct__get_u, "struct.get_u", 0xFB, 4)
OFB(Struct__set, "struct.set", 0xFB, 5)
OFB(Array__new, "array.new", 0xFB, 6)
OFB(Array__new_default, "array.new_default", 0xFB, 7)
OFB(Array__new_fixed, "array.new_fixed", 0xFB, 8)
OFB(Array__new_data, "array.new_data", 0xFB, 9)
OFB(Array__new_elem, "array.new_elem", 0xFB, 10)
OFB(Array__get, "array.get", 0xFB, 11)
OFB(Array__get_s, "array.get_s", 0xFB, 12)
OFB(Array__get_u, "array.get_u", 0xFB, 13)
OFB(Array__set, "array.set", 0xFB, 14)
OFB(Array__len, "array.len", 0xFB, 15)
OFB(Array__fill, "array.fill", 0xFB, 16)
OFB(Array__copy, "array.copy", 0xFB, 17)
OFB(Array__init_data, "array.init_data", 0xFB, 18)
OFB(Array__init_elem, "array.init_elem", 0xFB, 19)
OFB(Ref__test, "ref.test (ref)", 0xFB, 20)
OFB(Ref__test_null, "ref.test (ref.null)", 0xFB, 21)
OFB(Ref__cast, "ref.cast (ref)", 0xFB, 22)
OFB(Ref__cast_null, "ref.cast (ref.null)", 0xFB, 23)
OFB(Br_on_cast, "br_on_cast", 0xFB, 24)
OFB(Br_on_cast_fail, "br_on_cast_fail", 0xFB, 25)
OFB(Any__convert_extern, "any.convert_extern", 0xFB, 26)
OFB(Extern__convert_any, "extern.convert_any", 0xFB, 27)
OFB(Ref__i31, "ref.i31", 0xFB, 28)
OFB(I31__get_s, "i31.get_s", 0xFB, 29)
OFB(I31__get_u, "i31.get_u", 0xFB, 30)

// 0xFC prefix - Saturating Truncation Instructions
OFC(I32__trunc_sat_f32_s, "i32.trunc_sat_f32_s", 0xFC, 0)
OFC(I32__trunc_sat_f32_u, "i32.trunc_sat_f32_u", 0xFC, 1)
OFC(I32__trunc_sat_f64_s, "i32.trunc_sat_f64_s", 0xFC, 2)
OFC(I32__trunc_sat_f64_u, "i32.trunc_sat_f64_u", 0xFC, 3)
OFC(I64__trunc_sat_f32_s, "i64.trunc_sat_f32_s", 0xFC, 4)
OFC(I64__trunc_sat_f32_u, "i64.trunc_sat_f32_u", 0xFC, 5)
OFC(I64__trunc_sat_f64_s, "i64.trunc_sat_f64_s", 0xFC, 6)
OFC(I64__trunc_sat_f64_u, "i64.trunc_sat_f64_u", 0xFC, 7)

// 0xFC prefix - Memory Instructions (part 2)
OFC(Memory__init, "memory.init", 0xFC, 8)
OFC(Data__drop, "data.drop", 0xFC, 9)
OFC(Memory__copy, "memory.copy", 0xFC, 10)
OFC(Memory__fill, "memory.fill", 0xFC, 11)

// 0xFC prefix - Table Instructions (part 2)
OFC(Table__init, "table.init", 0xFC, 12)
OFC(Elem__drop, "elem.drop", 0xFC, 13)
OFC(Table__copy, "table.copy", 0xFC, 14)
OFC(Table__grow, "table.grow", 0xFC, 15)
OFC(Table__size, "table.size", 0xFC, 16)
OFC(Table__fill, "table.fill", 0xFC, 17)

// 0xFD prefix - Vector Memory Instructions (part 1)
OFD(V128__load, "v128.load", 0xFD, 0)
OFD(V128__load8x8_s, "v128.load8x8_s", 0xFD, 1)
OFD(V128__load8x8_u, "v128.load8x8_u", 0xFD, 2)
OFD(V128__load16x4_s, "v128.load16x4_s", 0xFD, 3)
OFD(V128__load16x4_u, "v128.load16x4_u", 0xFD, 4)
OFD(V128__load32x2_s, "v128.load32x2_s", 0xFD, 5)
OFD(V128__load32x2_u, "v128.load32x2_u", 0xFD, 6)
OFD(V128__load8_splat, "v128.load8_splat", 0xFD, 7)
OFD(V128__load16_splat, "v128.load16_splat", 0xFD, 8)
OFD(V128__load32_splat, "v128.load32_splat", 0xFD, 9)
OFD(V128__load64_splat, "v128.load64_splat", 0xFD, 10)
OFD(V128__store, "v128.store", 0xFD, 11)

// 0xFD prefix - Vector Numeric Instructions (part 1)
OFD(V128__const, "v128.const", 0xFD, 12)
OFD(I8x16__shuffle, "i8x16.shuffle", 0xFD, 13)
OFD(I8x16__swizzle, "i8x16.swizzle", 0xFD, 14)
OFD(I8x16__splat, "i8x16.splat", 0xFD, 15)
OFD(I16x8__splat, "i16x8.splat", 0xFD, 16)
OFD(I32x4__splat, "i32x4.splat", 0xFD, 17)
OFD(I64x2__splat, "i64x2.splat", 0xFD, 18)
OFD(F32x4__splat, "f32x4.splat", 0xFD, 19)
OFD(F64x2__splat, "f64x2.splat", 0xFD, 20)

// 0xFD prefix - Vector Lane Instructions
OFD(I8x16__extract_lane_s, "i8x16.extract_lane_s", 0xFD, 21)
OFD(I8x16__extract_lane_u, "i8x16.extract_lane_u", 0xFD, 22)
OFD(I8x16__replace_lane, "i8x16.replace_lane", 0xFD, 23)
OFD(I16x8__extract_lane_s, "i16x8.extract_lane_s", 0xFD, 24)
OFD(I16x8__extract_lane_u, "i16x8.extract_lane_u", 0xFD, 25)
OFD(I16x8__replace_lane, "i16x8.replace_lane", 0xFD, 26)
OFD(I32x4__extract_lane, "i32x4.extract_lane", 0xFD, 27)
OFD(I32x4__replace_lane, "i32x4.replace_lane", 0xFD, 28)
OFD(I64x2__extract_lane, "i64x2.extract_lane", 0xFD, 29)
OFD(I64x2__replace_lane, "i64x2.replace_lane", 0xFD, 30)
OFD(F32x4__extract_lane, "f32x4.extract_lane", 0xFD, 31)
OFD(F32x4__replace_lane, "f32x4.replace_lane", 0xFD, 32)
OFD(F64x2__extract_lane, "f64x2.extract_lane", 0xFD, 33)
OFD(F64x2__replace_lane, "f64x2.replace_lane", 0xFD, 34)

// 0xFD prefix - Vector Numeric Instructions (part 2)
OFD(I8x16__eq, "i8x16.eq", 0xFD, 35)
OFD(I8x16__ne, "i8x16.ne", 0xFD, 36)
OFD(I8x16__lt_s, "i8x16.lt_s", 0xFD, 37)
OFD(I8x16__lt_u, "i8x16.lt_u", 0xFD, 38)
OFD(I8x16__gt_s, "i8x16.gt_s", 0xFD, 39)
OFD(I8x16__gt_u, "i8x16.gt_u", 0xFD, 40)
OFD(I8x16__le_s, "i8x16.le_s", 0xFD, 41)
OFD(I8x16__le_u, "i8x16.le_u", 0xFD, 42)
OFD(I8x16__ge_s, "i8x16.ge_s", 0xFD, 43)
OFD(I8x16__ge_u, "i8x16.ge_u", 0xFD, 44)
OFD(I16x8__eq, "i16x8.eq", 0xFD, 45)
OFD(I16x8__ne, "i16x8.ne", 0xFD, 46)
OFD(I16x8__lt_s, "i16x8.lt_s", 0xFD, 47)
OFD(I16x8__lt_u, "i16x8.lt_u", 0xFD, 48)
OFD(I16x8__gt_s, "i16x8.gt_s", 0xFD, 49)
OFD(I16x8__gt_u, "i16x8.gt_u", 0xFD, 50)
OFD(I16x8__le_s, "i16x8.le_s", 0xFD, 51)
OFD(I16x8__le_u, "i16x8.le_u", 0xFD, 52)
OFD(I16x8__ge_s, "i16x8.ge_s", 0xFD, 53)
OFD(I16x8__ge_u, "i16x8.ge_u", 0xFD, 54)
OFD(I32x4__eq, "i32x4.eq", 0xFD, 55)
OFD(I32x4__ne, "i32x4.ne", 0xFD, 56)
OFD(I32x4__lt_s, "i32x4.lt_s", 0xFD, 57)
OFD(I32x4__lt_u, "i32x4.lt_u", 0xFD, 58)
OFD(I32x4__gt_s, "i32x4.gt_s", 0xFD, 59)
OFD(I32x4__gt_u, "i32x4.gt_u", 0xFD, 60)
OFD(I32x4__le_s, "i32x4.le_s", 0xFD, 61)
OFD(I32x4__le_u, "i32x4.le_u", 0xFD, 62)
OFD(I32x4__ge_s, "i32x4.ge_s", 0xFD, 63)
OFD(I32x4__ge_u, "i32x4.ge_u", 0xFD, 64)
OFD(F32x4__eq, "f32x4.eq", 0xFD, 65)
OFD(F32x4__ne, "f32x4.ne", 0xFD, 66)
OFD(F32x4__lt, "f32x4.lt", 0xFD, 67)
OFD(F32x4__gt, "f32x4.gt", 0xFD, 68)
OFD(F32x4__le, "f32x4.le", 0xFD, 69)
OFD(F32x4__ge, "f32x4.ge", 0xFD, 70)
OFD(F64x2__eq, "f64x2.eq", 0xFD, 71)
OFD(F64x2__ne, "f64x2.ne", 0xFD, 72)
OFD(F64x2__lt, "f64x2.lt", 0xFD, 73)
OFD(F64x2__gt, "f64x2.gt", 0xFD, 74)
OFD(F64x2__le, "f64x2.le", 0xFD, 75)
OFD(F64x2__ge, "f64x2.ge", 0xFD, 76)
OFD(V128__not, "v128.not", 0xFD, 77)
OFD(V128__and, "v128.and", 0xFD, 78)
OFD(V128__andnot, "v128.andnot", 0xFD, 79)
OFD(V128__or, "v128.or", 0xFD, 80)
OFD(V128__xor, "v128.xor", 0xFD, 81)
OFD(V128__bitselect, "v128.bitselect", 0xFD, 82)
OFD(V128__any_true, "v128.any_true", 0xFD, 83)

// 0xFD prefix - Vector Memory Instructions (part 2)
OFD(V128__load8_lane, "v128.load8_lane", 0xFD, 84)
OFD(V128__load16_lane, "v128.load16_lane", 0xFD, 85)
OFD(V128__load32_lane, "v128.load32_lane", 0xFD, 86)
OFD(V128__load64_lane, "v128.load64_lane", 0xFD, 87)
OFD(V128__store8_lane, "v128.store8_lane", 0xFD, 88)
OFD(V128__store16_lane, "v128.store16_lane", 0xFD, 89)
OFD(V128__store32_lane, "v128.store32_lane", 0xFD, 90)
OFD(V128__store64_lane, "v128.store64_lane", 0xFD, 91)
OFD(V128__load32_zero, "v128.load32_zero", 0xFD, 92)
OFD(V128__load64_zero, "v128.load64_zero", 0xFD, 93)

// 0xFD prefix - Vector Numeric Instructions (part 3)
OFD(F32x4__demote_f64x2_zero, "f32x4.demote_f64x2_zero", 0xFD, 94)
OFD(F64x2__promote_low_f32x4, "f64x2.promote_low_f32x4", 0xFD, 95)
OFD(I8x16__abs, "i8x16.abs", 0xFD, 96)
OFD(I8x16__neg, "i8x16.neg", 0xFD, 97)
OFD(I8x16__popcnt, "i8x16.popcnt", 0xFD, 98)
OFD(I8x16__all_true, "i8x16.all_true", 0xFD, 99)
OFD(I8x16__bitmask, "i8x16.bitmask", 0xFD, 100)
OFD(I8x16__narrow_i16x8_s, "i8x16.narrow_i16x8_s", 0xFD, 101)
OFD(I8x16__narrow_i16x8_u, "i8x16.narrow_i16x8_u", 0xFD, 102)
OFD(F32x4__ceil, "f32x4.ceil", 0xFD, 103)
OFD(F32x4__floor, "f32x4.floor", 0xFD, 104)
OFD(F32x4__trunc, "f32x4.trunc", 0xFD, 105)
OFD(F32x4__nearest, "f32x4.nearest", 0xFD, 106)
OFD(I8x16__shl, "i8x16.shl", 0xFD, 107)
OFD(I8x16__shr_s, "i8x16.shr_s", 0xFD, 108)
OFD(I8x16__shr_u, "i8x16.shr_u", 0xFD, 109)
OFD(I8x16__add, "i8x16.add", 0xFD, 110)
OFD(I8x16__add_sat_s, "i8x16.add_sat_s", 0xFD, 111)
OFD(I8x16__add_sat_u, "i8x16.add_sat_u", 0xFD, 112)
OFD(I8x16__sub, "i8x16.sub", 0xFD, 113)
OFD(I8x16__sub_sat_s, "i8x16.sub_sat_s", 0xFD, 114)
OFD(I8x16__sub_sat_u, "i8x16.sub_sat_u", 0xFD, 115)
OFD(F64x2__ceil, "f64x2.ceil", 0xFD, 116)
OFD(F64x2__floor, "f64x2.floor", 0xFD, 117)
OFD(I8x16__min_s, "i8x16.min_s", 0xFD, 118)
OFD(I8x16__min_u, "i8x16.min_u", 0xFD, 119)
OFD(I8x16__max_s, "i8x16.max_s", 0xFD, 120)
OFD(I8x16__max_u, "i8x16.max_u", 0xFD, 121)
OFD(F64x2__trunc, "f64x2.trunc", 0xFD, 122)
OFD(I8x16__avgr_u, "i8x16.avgr_u", 0xFD, 123)
OFD(I16x8__extadd_pairwise_i8x16_s, "i16x8.extadd_pairwise_i8x16_s", 0xFD, 124)
OFD(I16x8__extadd_pairwise_i8x16_u, "i16x8.extadd_pairwise_i8x16_u", 0xFD, 125)
OFD(I32x4__extadd_pairwise_i16x8_s, "i32x4.extadd_pairwise_i16x8_s", 0xFD, 126)
OFD(I32x4__extadd_pairwise_i16x8_u, "i32x4.extadd_pairwise_i16x8_u", 0xFD, 127)
OFD(I16x8__abs, "i16x8.abs", 0xFD, 128)
OFD(I16x8__neg, "i16x8.neg", 0xFD, 129)
OFD(I16x8__q15mulr_sat_s, "i16x8.q15mulr_sat_s", 0xFD, 130)
OFD(I16x8__all_true, "i16x8.all_true", 0xFD, 131)
OFD(I16x8__bitmask, "i16x8.bitmask", 0xFD, 132)
OFD(I16x8__narrow_i32x4_s, "i16x8.narrow_i32x4_s", 0xFD, 133)
OFD(I16x8__narrow_i32x4_u, "i16x8.narrow_i32x4_u", 0xFD, 134)
OFD(I16x8__extend_low_i8x16_s, "i16x8.extend_low_i8x16_s", 0xFD, 135)
OFD(I16x8__extend_high_i8x16_s, "i16x8.extend_high_i8x16_s", 0xFD, 136)
OFD(I16x8__extend_low_i8x16_u, "i16x8.extend_low_i8x16_u", 0xFD, 137)
OFD(I16x8__extend_high_i8x16_u, "i16x8.extend_high_i8x16_u", 0xFD, 138)
OFD(I16x8__shl, "i16x8.shl", 0xFD, 139)
OFD(I16x8__shr_s, "i16x8.shr_s", 0xFD, 140)
OFD(I16x8__shr_u, "i16x8.shr_u", 0xFD, 141)
OFD(I16x8__add, "i16x8.add", 0xFD, 142)
OFD(I16x8__add_sat_s, "i16x8.add_sat_s", 0xFD, 143)
OFD(I16x8__add_sat_u, "i16x8.add_sat_u", 0xFD, 144)
OFD(I16x8__sub, "i16x8.sub", 0xFD, 145)
OFD(I16x8__sub_sat_s, "i16x8.sub_sat_s", 0xFD, 146)
OFD(I16x8__sub_sat_u, "i16x8.sub_sat_u", 0xFD, 147)
OFD(F64x2__nearest, "f64x2.nearest", 0xFD, 148)
OFD(I16x8__mul, "i16x8.mul", 0xFD, 149)
OFD(I16x8__min_s, "i16x8.min_s", 0xFD, 150)
OFD(I16x8__min_u, "i16x8.min_u", 0xFD, 151)
OFD(I16x8__max_s, "i16x8.max_s", 0xFD, 152)
OFD(I16x8__max_u, "i16x8.max_u", 0xFD, 153)
// 0xFD 154: Reserved
OFD(I16x8__avgr_u, "i16x8.avgr_u", 0xFD, 155)
OFD(I16x8__extmul_low_i8x16_s, "i16x8.extmul_low_i8x16_s", 0xFD, 156)
OFD(I16x8__extmul_high_i8x16_s, "i16x8.extmul_high_i8x16_s", 0xFD, 157)
OFD(I16x8__extmul_low_i8x16_u, "i16x8.extmul_low_i8x16_u", 0xFD, 158)
OFD(I16x8__extmul_high_i8x16_u, "i16x8.extmul_high_i8x16_u", 0xFD, 159)
OFD(I32x4__abs, "i32x4.abs", 0xFD, 160)
OFD(I32x4__neg, "i32x4.neg", 0xFD, 161)
// 0xFD 162: Reserved
OFD(I32x4__all_true, "i32x4.all_true", 0xFD, 163)
OFD(I32x4__bitmask, "i32x4.bitmask", 0xFD, 164)
// 0xFD 165: Reserved
// 0xFD 166: Reserved
OFD(I32x4__extend_low_i16x8_s, "i32x4.extend_low_i16x8_s", 0xFD, 167)
OFD(I32x4__extend_high_i16x8_s, "i32x4.extend_high_i16x8_s", 0xFD, 168)
OFD(I32x4__extend_low_i16x8_u, "i32x4.extend_low_i16x8_u", 0xFD, 169)
OFD(I32x4__extend_high_i16x8_u, "i32x4.extend_high_i16x8_u", 0xFD, 170)
OFD(I32x4__shl, "i32x4.shl", 0xFD, 171)
OFD(I32x4__shr_s, "i32x4.shr_s", 0xFD, 172)
OFD(I32x4__shr_u, "i32x4.shr_u", 0xFD, 173)
OFD(I32x4__add, "i32x4.add", 0xFD, 174)
// 0xFD 175: Reserved
// 0xFD 176: Reserved
OFD(I32x4__sub, "i32x4.sub", 0xFD, 177)
// 0xFD 178: Reserved
// 0xFD 179: Reserved
// 0xFD 180: Reserved
OFD(I32x4__mul, "i32x4.mul", 0xFD, 181)
OFD(I32x4__min_s, "i32x4.min_s", 0xFD, 182)
OFD(I32x4__min_u, "i32x4.min_u", 0xFD, 183)
OFD(I32x4__max_s, "i32x4.max_s", 0xFD, 184)
OFD(I32x4__max_u, "i32x4.max_u", 0xFD, 185)
OFD(I32x4__dot_i16x8_s, "i32x4.dot_i16x8_s", 0xFD, 186)
// 0xFD 187: Reserved
OFD(I32x4__extmul_low_i16x8_s, "i32x4.extmul_low_i16x8_s", 0xFD, 188)
OFD(I32x4__extmul_high_i16x8_s, "i32x4.extmul_high_i16x8_s", 0xFD, 189)
OFD(I32x4__extmul_low_i16x8_u, "i32x4.extmul_low_i16x8_u", 0xFD, 190)
OFD(I32x4__extmul_high_i16x8_u, "i32x4.extmul_high_i16x8_u", 0xFD, 191)
OFD(I64x2__abs, "i64x2.abs", 0xFD, 192)
OFD(I64x2__neg, "i64x2.neg", 0xFD, 193)
// 0xFD 194: Reserved
OFD(I64x2__all_true, "i64x2.all_true", 0xFD, 195)
OFD(I64x2__bitmask, "i64x2.bitmask", 0xFD, 196)
// 0xFD 197: Reserved
// 0xFD 198: Reserved
OFD(I64x2__extend_low_i32x4_s, "i64x2.extend_low_i32x4_s", 0xFD, 199)
OFD(I64x2__extend_high_i32x4_s, "i64x2.extend_high_i32x4_s", 0xFD, 200)
OFD(I64x2__extend_low_i32x4_u, "i64x2.extend_low_i32x4_u", 0xFD, 201)
OFD(I64x2__extend_high_i32x4_u, "i64x2.extend_high_i32x4_u", 0xFD, 202)
OFD(I64x2__shl, "i64x2.shl", 0xFD, 203)
OFD(I64x2__shr_s, "i64x2.shr_s", 0xFD, 204)
OFD(I64x2__shr_u, "i64x2.shr_u", 0xFD, 205)
OFD(I64x2__add, "i64x2.add", 0xFD, 206)
// 0xFD 207: Reserved
// 0xFD 208: Reserved
OFD(I64x2__sub, "i64x2.sub", 0xFD, 209)
// 0xFD 210: Reserved
// 0xFD 211: Reserved
// 0xFD 212: Reserved
OFD(I64x2__mul, "i64x2.mul", 0xFD, 213)
OFD(I64x2__eq, "i64x2.eq", 0xFD, 214)
OFD(I64x2__ne, "i64x2.ne", 0xFD, 215)
OFD(I64x2__lt_s, "i64x2.lt_s", 0xFD, 216)
OFD(I64x2__gt_s, "i64x2.gt_s", 0xFD, 217)
OFD(I64x2__le_s, "i64x2.le_s", 0xFD, 218)
OFD(I64x2__ge_s, "i64x2.ge_s", 0xFD, 219)
OFD(I64x2__extmul_low_i32x4_s, "i64x2.extmul_low_i32x4_s", 0xFD, 220)
OFD(I64x2__extmul_high_i32x4_s, "i64x2.extmul_high_i32x4_s", 0xFD, 221)
OFD(I64x2__extmul_low_i32x4_u, "i64x2.extmul_low_i32x4_u", 0xFD, 222)
OFD(I64x2__extmul_high_i32x4_u, "i64x2.extmul_high_i32x4_u", 0xFD, 223)
OFD(F32x4__abs, "f32x4.abs", 0xFD, 224)
OFD(F32x4__neg, "f32x4.neg", 0xFD, 225)
// 0xFD 226: Reserved
OFD(F32x4__sqrt, "f32x4.sqrt", 0xFD, 227)
OFD(F32x4__add, "f32x4.add", 0xFD, 228)
OFD(F32x4__sub, "f32x4.sub", 0xFD, 229)
OFD(F32x4__mul, "f32x4.mul", 0xFD, 230)
OFD(F32x4__div, "f32x4.div", 0xFD, 231)
OFD(F32x4__min, "f32x4.min", 0xFD, 232)
OFD(F32x4__max, "f32x4.max", 0xFD, 233)
OFD(F32x4__pmin, "f32x4.pmin", 0xFD, 234)
OFD(F32x4__pmax, "f32x4.pmax", 0xFD, 235)
OFD(F64x2__abs, "f64x2.abs", 0xFD, 236)
OFD(F64x2__neg, "f64x2.neg", 0xFD, 237)
OFD(F64x2__sqrt, "f64x2.sqrt", 0xFD, 239)
OFD(F64x2__add, "f64x2.add", 0xFD, 240)
OFD(F64x2__sub, "f64x2.sub", 0xFD, 241)
OFD(F64x2__mul, "f64x2.mul", 0xFD, 242)
OFD(F64x2__div, "f64x2.div", 0xFD, 243)
OFD(F64x2__min, "f64x2.min", 0xFD, 244)
OFD(F64x2__max, "f64x2.max", 0xFD, 245)
OFD(F64x2__pmin, "f64x2.pmin", 0xFD, 246)
OFD(F64x2__pmax, "f64x2.pmax", 0xFD, 247)
OFD(I32x4__trunc_sat_f32x4_s, "i32x4.trunc_sat_f32x4_s", 0xFD, 248)
OFD(I32x4__trunc_sat_f32x4_u, "i32x4.trunc_sat_f32x4_u", 0xFD, 249)
OFD(F32x4__convert_i32x4_s, "f32x4.convert_i32x4_s", 0xFD, 250)
OFD(F32x4__convert_i32x4_u, "f32x4.convert_i32x4_u", 0xFD, 251)
OFD(I32x4__trunc_sat_f64x2_s_zero, "i32x4.trunc_sat_f64x2_s_zero", 0xFD, 252)
OFD(I32x4__trunc_sat_f64x2_u_zero, "i32x4.trunc_sat_f64x2_u_zero", 0xFD, 253)
OFD(F64x2__convert_low_i32x4_s, "f64x2.convert_low_i32x4_s", 0xFD, 254)
OFD(F64x2__convert_low_i32x4_u, "f64x2.convert_low_i32x4_u", 0xFD, 255)

// 0xFE prefix - Relaxed SIMD Instructions (part 4)
OFD(I8x16__relaxed_swizzle, "i8x16.relaxed_swizzle", 0xFD, 256)
OFD(I32x4__relaxed_trunc_f32x4_s, "i32x4.relaxed_trunc_f32x4_s", 0xFD, 257)
OFD(I32x4__relaxed_trunc_f32x4_u, "i32x4.relaxed_trunc_f32x4_u", 0xFD, 258)
OFD(I32x4__relaxed_trunc_f64x2_s_zero, "i32x4.relaxed_trunc_f64x2_s_zero", 0xFD,
    259)
OFD(I32x4__relaxed_trunc_f64x2_u_zero, "i32x4.relaxed_trunc_f64x2_u_zero", 0xFD,
    260)
OFD(F32x4__relaxed_madd, "f32x4.relaxed_madd", 0xFD, 261)
OFD(F32x4__relaxed_nmadd, "f32x4.relaxed_nmadd", 0xFD, 262)
OFD(F64x2__relaxed_madd, "f32x4.relaxed_madd", 0xFD, 263)
OFD(F64x2__relaxed_nmadd, "f32x4.relaxed_nmadd", 0xFD, 264)
OFD(I8x16__relaxed_laneselect, "i8x16.relaxed_laneselect", 0xFD, 265)
OFD(I16x8__relaxed_laneselect, "i16x8.relaxed_laneselect", 0xFD, 266)
OFD(I32x4__relaxed_laneselect, "i32x4.relaxed_laneselect", 0xFD, 267)
OFD(I64x2__relaxed_laneselect, "i64x2.relaxed_laneselect", 0xFD, 268)
OFD(F32x4__relaxed_min, "f32x4.relaxed_min", 0xFD, 269)
OFD(F32x4__relaxed_max, "f32x4.relaxed_max", 0xFD, 270)
OFD(F64x2__relaxed_min, "f64x2.relaxed_min", 0xFD, 271)
OFD(F64x2__relaxed_max, "f64x2.relaxed_max", 0xFD, 272)
OFD(I16x8__relaxed_q15mulr_s, "i16x8.relaxed_q15mulr_s", 0xFD, 273)
OFD(I16x8__relaxed_dot_i8x16_i7x16_s, "i16x8.relaxed_dot_i8x16_i7x16_s", 0xFD,
    274)
OFD(I32x4__relaxed_dot_i8x16_i7x16_add_s, "i32x4.relaxed_dot_i8x16_i7x16_add_s",
    0xFD, 275)
// 0xFD 276 ~ 303: Reserved

// 0xFE prefix - Atomic instructions
OFE(Memory__atomic__notify, "memory.atomic.notify", 0xFE, 0)
OFE(Memory__atomic__wait32, "memory.atomic.wait32", 0xFE, 1)
OFE(Memory__atomic__wait64, "memory.atomic.wait64", 0xFE, 2)
OFE(Atomic__fence, "atomic.fence", 0xFE, 3)
// 0xFE 4 ~ 15: Reserved
OFE(I32__atomic__load, "i32.atomic.load", 0xFE, 16)
OFE(I64__atomic__load, "i64.atomic.load", 0xFE, 17)
OFE(I32__atomic__load8_u, "i32.atomic.load8_u", 0xFE, 18)
OFE(I32__atomic__load16_u, "i32.atomic.load16_u", 0xFE, 19)
OFE(I64__atomic__load8_u, "i64.atomic.load8_u", 0xFE, 20)
OFE(I64__atomic__load16_u, "i64.atomic.load16_u", 0xFE, 21)
OFE(I64__atomic__load32_u, "i64.atomic.load32_u", 0xFE, 22)
OFE(I32__atomic__store, "i32.atomic.store", 0xFE, 23)
OFE(I64__atomic__store, "i64.atomic.store", 0xFE, 24)
OFE(I32__atomic__store8, "i32.atomic.store8", 0xFE, 25)
OFE(I32__atomic__store16, "i32.atomic.store16", 0xFE, 26)
OFE(I64__atomic__store8, "i64.atomic.store8", 0xFE, 27)
OFE(I64__atomic__store16, "i64.atomic.store16", 0xFE, 28)
OFE(I64__atomic__store32, "i64.atomic.store32", 0xFE, 29)
OFE(I32__atomic__rmw__add, "i32.atomic.rmw.add", 0xFE, 30)
OFE(I64__atomic__rmw__add, "i64.atomic.rmw.add", 0xFE, 31)
OFE(I32__atomic__rmw8__add_u, "i32.atomic.rmw8.add_u", 0xFE, 32)
OFE(I32__atomic__rmw16__add_u, "i32.atomic.rmw16.add_u", 0xFE, 33)
OFE(I64__atomic__rmw8__add_u, "i64.atomic.rmw8.add_u", 0xFE, 34)
OFE(I64__atomic__rmw16__add_u, "i64.atomic.rmw16.add_u", 0xFE, 35)
OFE(I64__atomic__rmw32__add_u, "i64.atomic.rmw32.add_u", 0xFE, 36)
OFE(I32__atomic__rmw__sub, "i32.atomic.rmw.sub", 0xFE, 37)
OFE(I64__atomic__rmw__sub, "i64.atomic.rmw.sub", 0xFE, 38)
OFE(I32__atomic__rmw8__sub_u, "i32.atomic.rmw8.sub_u", 0xFE, 39)
OFE(I32__atomic__rmw16__sub_u, "i32.atomic.rmw16.sub_u", 0xFE, 40)
OFE(I64__atomic__rmw8__sub_u, "i64.atomic.rmw8.sub_u", 0xFE, 41)
OFE(I64__atomic__rmw16__sub_u, "i64.atomic.rmw16.sub_u", 0xFE, 42)
OFE(I64__atomic__rmw32__sub_u, "i64.atomic.rmw32.sub_u", 0xFE, 43)
OFE(I32__atomic__rmw__and, "i32.atomic.rmw.and", 0xFE, 44)
OFE(I64__atomic__rmw__and, "i64.atomic.rmw.and", 0xFE, 45)
OFE(I32__atomic__rmw8__and_u, "i32.atomic.rmw8.and_u", 0xFE, 46)
OFE(I32__atomic__rmw16__and_u, "i32.atomic.rmw16.and_u", 0xFE, 47)
OFE(I64__atomic__rmw8__and_u, "i64.atomic.rmw8.and_u", 0xFE, 48)
OFE(I64__atomic__rmw16__and_u, "i64.atomic.rmw16.and_u", 0xFE, 49)
OFE(I64__atomic__rmw32__and_u, "i64.atomic.rmw32.and_u", 0xFE, 50)
OFE(I32__atomic__rmw__or, "i32.atomic.rmw.or", 0xFE, 51)
OFE(I64__atomic__rmw__or, "i64.atomic.rmw.or", 0xFE, 52)
OFE(I32__atomic__rmw8__or_u, "i32.atomic.rmw8.or_u", 0xFE, 53)
OFE(I32__atomic__rmw16__or_u, "i32.atomic.rmw16.or_u", 0xFE, 54)
OFE(I64__atomic__rmw8__or_u, "i64.atomic.rmw8.or_u", 0xFE, 55)
OFE(I64__atomic__rmw16__or_u, "i64.atomic.rmw16.or_u", 0xFE, 56)
OFE(I64__atomic__rmw32__or_u, "i64.atomic.rmw32.or_u", 0xFE, 57)
OFE(I32__atomic__rmw__xor, "i32.atomic.rmw.xor", 0xFE, 58)
OFE(I64__atomic__rmw__xor, "i64.atomic.rmw.xor", 0xFE, 59)
OFE(I32__atomic__rmw8__xor_u, "i32.atomic.rmw8.xor_u", 0xFE, 60)
OFE(I32__atomic__rmw16__xor_u, "i32.atomic.rmw16.xor_u", 0xFE, 61)
OFE(I64__atomic__rmw8__xor_u, "i64.atomic.rmw8.xor_u", 0xFE, 62)
OFE(I64__atomic__rmw16__xor_u, "i64.atomic.rmw16.xor_u", 0xFE, 63)
OFE(I64__atomic__rmw32__xor_u, "i64.atomic.rmw32.xor_u", 0xFE, 64)
OFE(I32__atomic__rmw__xchg, "i32.atomic.rmw.xchg", 0xFE, 65)
OFE(I64__atomic__rmw__xchg, "i64.atomic.rmw.xchg", 0xFE, 66)
OFE(I32__atomic__rmw8__xchg_u, "i32.atomic.rmw8.xchg_u", 0xFE, 67)
OFE(I32__atomic__rmw16__xchg_u, "i32.atomic.rmw16.xchg_u", 0xFE, 68)
OFE(I64__atomic__rmw8__xchg_u, "i64.atomic.rmw8.xchg_u", 0xFE, 69)
OFE(I64__atomic__rmw16__xchg_u, "i64.atomic.rmw16.xchg_u", 0xFE, 70)
OFE(I64__atomic__rmw32__xchg_u, "i64.atomic.rmw32.xchg_u", 0xFE, 71)
OFE(I32__atomic__rmw__cmpxchg, "i32.atomic.rmw.cmpxchg", 0xFE, 72)
OFE(I64__atomic__rmw__cmpxchg, "i64.atomic.rmw.cmpxchg", 0xFE, 73)
OFE(I32__atomic__rmw8__cmpxchg_u, "i32.atomic.rmw8.cmpxchg_u", 0xFE, 74)
OFE(I32__atomic__rmw16__cmpxchg_u, "i32.atomic.rmw16.cmpxchg_u", 0xFE, 75)
OFE(I64__atomic__rmw8__cmpxchg_u, "i64.atomic.rmw8.cmpxchg_u", 0xFE, 76)
OFE(I64__atomic__rmw16__cmpxchg_u, "i64.atomic.rmw16.cmpxchg_u", 0xFE, 77)
OFE(I64__atomic__rmw32__cmpxchg_u, "i64.atomic.rmw32.cmpxchg_u", 0xFE, 78)

#undef O
#undef OFB
#undef OFC
#undef OFD
#undef OFE
#endif // UseOpCode

// enum_configure.h

#ifdef UseProposal
#define P Line
// Finished and standardized proposals
P(ImportExportMutGlobals, "Import/Export of Mutable Globals")
P(NonTrapFloatToIntConversions, "Non-trapping float-to-int Conversions")
P(SignExtensionOperators, "Sign-extension Operators")
P(MultiValue, "Multi-value")
P(BulkMemoryOperations, "Bulk Memory Operations")
P(ReferenceTypes, "Reference Types")
P(SIMD, "Fixed-width SIMD")
// Phase 4 proposals
P(TailCall, "Tail Call")
P(ExtendedConst, "Extended Const Expressions")
P(FunctionReferences, "Typed Function References")
P(GC, "Garbage Collection")
P(MultiMemories, "Multiple Memories")
P(Threads, "Threads")
P(RelaxSIMD, "Relaxed SIMD")
// Phase 3 proposals
P(Annotations, "Custom Annotation Syntax in the Text Format")
P(Memory64, "Memory64")
P(ExceptionHandling, "Exception Handling")
// Phase 1 proposals
P(Component, "Component Model")
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
// Alignment must < 64 without and < 128 with multi-memory proposal.
E(InvalidStoreAlignment, 0x011D, "invalid store alignment")
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
// Tag index not defined
E(InvalidTagIdx, 0x020A, "unknown tag")
// Element segment index not defined
E(InvalidElemIdx, 0x020B, "unknown elem segment")
// Data segment index not defined
E(InvalidDataIdx, 0x020C, "unknown data segment")
// Undeclared reference
E(InvalidRefIdx, 0x020D, "undeclared function reference")
// Should be constant expression
E(ConstExprRequired, 0x020E, "constant expression required")
// Export name conflicted
E(DupExportName, 0x020F, "duplicate export name")
// Tried to store to const global value
E(ImmutableGlobal, 0x0210, "global is immutable")
// Tried to store to const field of structure
E(ImmutableField, 0x0211, "field is immutable")
// Tried to store to const array
E(ImmutableArray, 0x0212, "array is immutable")
// Invalid result arity in select t* instruction
E(InvalidResultArity, 0x0213, "invalid result arity")
// #Tables > 1 (without Ref-types proposal)
E(MultiTables, 0x0214, "multiple tables")
// #Memories > 1
E(MultiMemories, 0x0215, "multiple memories")
// Invalid Limit grammar
E(InvalidLimit, 0x0216, "size minimum must not be greater than maximum")
// Memory pages > 65536
E(InvalidMemPages, 0x0217, "memory size must be at most 65536 pages (4GiB)")
// Invalid start function signature
E(InvalidStartFunc, 0x0218, "start function")
// Invalid lane index
E(InvalidLaneIdx, 0x0219, "invalid lane index")
// Invalid uninitialized local
E(InvalidUninitLocal, 0x021A, "uninitialized local")
// Defaultable field type required
E(InvalidNotDefaultableField, 0x021B, "field type is not defaultable")
// Defaultable array type required
E(InvalidNotDefaultableArray, 0x021C, "array type is not defaultable")
// Unpacked field type required, but got packed one
E(InvalidPackedField, 0x021D, "field is packed")
// Unpacked array type required, but got packed one
E(InvalidPackedArray, 0x021E, "array is packed")
// Packed field type required, but got unpacked one
E(InvalidUnpackedField, 0x021F, "field is unpacked")
// Packed array type required, but got unpacked one
E(InvalidUnpackedArray, 0x0220, "array is unpacked")
// Invalid Br ref type
E(InvalidBrRefType, 0x0221, "invalid br ref type")
// 2 array types not matched
E(ArrayTypesMismatch, 0x0222, "array types do not match")
// Should be numeric type in array type
E(ArrayTypesNumtypeRequired, 0x0223, "array type is not numeric or vector")
// Sub type matching or validation failed
E(InvalidSubType, 0x0224, "sub type")
// Invalid Tag type
E(InvalidTagResultType, 0x0225, "non-empty tag result type")
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
// Invalid core sort for component export
E(InvalidCoreSort, 0x0305, "invalid core sort")
E(InvalidCanonOption, 0x0306, "invalid canonical option")
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
// Access to null exception reference
E(AccessNullException, 0x0417, "null exception reference")
// Fail to cast reference
E(CastFailed, 0x0418, "cast failure")
// Uncaught Exception
E(UncaughtException, 0x0419, "uncaught exception")
// @}

// Component model load phase
// @{
// Malformed sort
E(MalformedSort, 0x0500, "malformed sort")
// Malformed alias target
E(MalformedAliasTarget, 0x0501, "malformed alias target")
// Malformed core instance
E(MalformedCoreInstance, 0x0502, "malformed core instance")
// Malformed instance
E(MalformedInstance, 0x0503, "malformed instance")
// Malformed defType
E(MalformedDefType, 0x0504, "malformed defined type")
// Malformed record type
E(MalformedRecordType, 0x0505, "malformed record type")
// Malformed variant type
E(MalformedVariantType, 0x0506, "malformed variant type")
// Malformed tuple type
E(MalformedTupleType, 0x0507, "malformed tuple type")
// Malformed flags type
E(MalformedFlagsType, 0x0508, "malformed flags type")
// Malformed canonical
E(MalformedCanonical, 0x0509, "malformed canonical")
// Unknown canonical option
E(UnknownCanonicalOption, 0x050A, "unknown canonical option")
// Malformed name
E(MalformedName, 0x050B, "malformed name")
// @}

// Component model instantiation phase
// @{
E(CoreInvalidExport, 0x0600, "invalid export in core module")
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
I(DefinedType, "defined type")
I(FunctionType, "function type")
I(Function, "function")
I(Table, "table")
I(Memory, "memory")
I(Global, "global")
I(Element, "element")
I(Data, "data")
I(Lane, "lane")
I(Field, "field")
I(TagType, "tag type")
I(Tag, "tag")

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
T(ExnRef, 0x69, "exn")             // -0x17 for reference type
T(Ref, 0x64, "ref")                // -0x1C for reference type
T(RefNull, 0x63, "ref_null")       // -0x1D for reference type
T(Func, 0x60, "func")              // -0x20 for composite type
T(Struct, 0x5F, "struct")          // -0x21 for composite type
T(Array, 0x5E, "array")            // -0x22 for composite type
T(Sub, 0x50, "sub")                // -0x30 for sub type
T(SubFinal, 0x4F, "sub_final")     // -0x31 for sub type
T(Rec, 0x4E, "rec")                // -0x32 for recursive type
T(Epsilon, 0x40, "-")              // -0x40 for result type
// component model types
T(String, 0x80, "string") // string type

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
E(Tag, 0x04U, "tag")

#undef E
#endif // UseExternalType
