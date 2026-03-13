// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC
//
// Tree-sitter grammar for WebAssembly Text Format (WAT)
// Covers: Core 1.0-3.0, SIMD, Bulk Memory, Reference Types, Tail Call,
// Extended Const, Function References, GC, Multi-memory, Relaxed SIMD,
// Memory64, Exception Handling, Threads, Annotations.

/// <reference types="tree-sitter-cli/dsl" />

const PREC = {
  COMMENT: -1,
};

// Identifier: $[0-9A-Za-z!#$%&'*+\-./:<=>?@\\^_`|~]+
const ID_CHAR = /[0-9A-Za-z!#$%&'*+\-.\/:<=>?@\\^_`|~]/;

module.exports = grammar({
  name: "wat",

  extras: $ => [
    /\s+/,
    $.line_comment,
    $.block_comment,
  ],

  word: $ => $.reserved,

  conflicts: $ => [
    [$._plain_op],
    [$._offset_expr, $._elem_item],
    [$.block_sig],
  ],

  rules: {
    // ---------------------------------------------------------------
    // Root – accept (module ...) or bare module fields
    // ---------------------------------------------------------------
    root: $ => choice(
      $.module,
      repeat1($._module_field),
    ),

    module: $ => seq(
      '(',
      'module',
      optional($.identifier),
      repeat($._module_field),
      ')',
    ),

    _module_field: $ => choice(
      $.type_definition,
      $.import,
      $.export,
      $.func,
      $.table,
      $.memory,
      $.global,
      $.start,
      $.elem,
      $.data,
      $.tag,
      $.rec,
      $.annotation,
    ),

    // ---------------------------------------------------------------
    // Identifiers and literals
    // ---------------------------------------------------------------
    identifier: _$ => token(seq('$', repeat1(ID_CHAR))),

    // Reserved word (for tree-sitter word detection)
    reserved: _$ => token(repeat1(ID_CHAR)),

    nat: _$ => token(choice(
      /[0-9][0-9_]*/,
      /0x[0-9A-Fa-f][0-9A-Fa-f_]*/,
    )),

    int: _$ => token(choice(
      /[+-]?[0-9][0-9_]*/,
      /[+-]?0x[0-9A-Fa-f][0-9A-Fa-f_]*/,
    )),

    float: _$ => token(choice(
      /[+-]?[0-9][0-9_]*(\.[0-9_]*)?([eE][+-]?[0-9_]+)?/,
      /[+-]?0x[0-9A-Fa-f][0-9A-Fa-f_]*(\.[0-9A-Fa-f_]*)?([pP][+-]?[0-9_]+)?/,
      /[+-]?inf/,
      /[+-]?nan/,
      /[+-]?nan:0x[0-9A-Fa-f][0-9A-Fa-f_]*/,
    )),

    // A numeric value (used wherever a number is expected)
    _num: $ => choice($.nat, $.int, $.float),

    string: _$ => token(seq(
      '"',
      repeat(choice(
        /[^"\\]+/,
        /\\[tnr"'\\]/,
        /\\[0-9A-Fa-f]{2}/,
        /\\u\{[0-9A-Fa-f]+\}/,
      )),
      '"',
    )),

    name: $ => $.string,

    // ---------------------------------------------------------------
    // Value types
    // ---------------------------------------------------------------
    _valtype: $ => choice(
      $.numtype,
      $.vectype,
      $.reftype,
    ),

    numtype: _$ => choice('i32', 'i64', 'f32', 'f64'),

    vectype: _$ => 'v128',

    reftype: $ => choice(
      $.ref_type_short,
      $.ref_type_full,
    ),

    ref_type_short: _$ => choice(
      'funcref', 'externref', 'anyref', 'eqref', 'i31ref',
      'structref', 'arrayref', 'nullfuncref', 'nullexternref', 'nullref',
    ),

    ref_type_full: $ => seq(
      '(', 'ref', optional('null'), $.heap_type, ')',
    ),

    heap_type: $ => choice(
      'func', 'extern', 'any', 'eq', 'i31', 'struct', 'array',
      'none', 'noextern', 'nofunc',
      $.identifier,
      $.nat,
    ),

    // ---------------------------------------------------------------
    // Function types / type use
    // ---------------------------------------------------------------
    func_type: $ => seq(
      '(', 'func', repeat($.param), repeat($.result), ')',
    ),

    param: $ => seq('(', 'param', optional($.identifier), repeat1($._valtype), ')'),

    result: $ => seq(
      '(', 'result', repeat1($._valtype), ')',
    ),

    type_use: $ => prec.left(seq(
      '(', 'type', choice($.identifier, $.nat), ')',
      repeat($.param),
      repeat($.result),
    )),

    // ---------------------------------------------------------------
    // Type definitions (rec, sub, struct, array, func)
    // ---------------------------------------------------------------
    type_definition: $ => seq(
      '(', 'type', optional($.identifier), $._deftype, ')',
    ),

    rec: $ => seq(
      '(', 'rec', repeat1($.type_definition), ')',
    ),

    _deftype: $ => choice(
      $.func_type,
      $.struct_type,
      $.array_type,
      $.sub_type,
    ),

    sub_type: $ => seq(
      '(', 'sub',
      optional('final'),
      repeat($._type_idx),
      choice($.func_type, $.struct_type, $.array_type),
      ')',
    ),

    _type_idx: $ => choice($.identifier, $.nat),

    struct_type: $ => seq(
      '(', 'struct', repeat($.field), ')',
    ),

    field: $ => choice(
      seq('(', 'field', optional($.identifier), $._storage_type, ')'),
      seq('(', 'field', optional($.identifier), $.mut_type, ')'),
    ),

    array_type: $ => seq(
      '(', 'array', choice($._storage_type, $.mut_type), ')',
    ),

    _storage_type: $ => choice(
      $._valtype,
      $.packed_type,
    ),

    packed_type: _$ => choice('i8', 'i16'),

    mut_type: $ => seq(
      '(', 'mut', choice($._valtype, $.packed_type), ')',
    ),

    // ---------------------------------------------------------------
    // Imports / Exports
    // ---------------------------------------------------------------
    import: $ => seq(
      '(', 'import', $.name, $.name, $._import_desc, ')',
    ),

    _import_desc: $ => choice(
      $.import_func,
      $.import_table,
      $.import_memory,
      $.import_global,
      $.import_tag,
    ),

    import_func: $ => seq(
      '(', 'func', optional($.identifier),
      optional($.type_use),
      repeat($.param),
      repeat($.result),
      ')',
    ),

    import_table: $ => seq(
      '(', 'table', optional($.identifier), $.table_type, ')',
    ),

    import_memory: $ => seq(
      '(', 'memory', optional($.identifier), $.memory_type, ')',
    ),

    import_global: $ => seq(
      '(', 'global', optional($.identifier), $._global_type, ')',
    ),

    import_tag: $ => seq(
      '(', 'tag', optional($.identifier),
      optional($.type_use),
      repeat($.param),
      ')',
    ),

    export: $ => seq(
      '(', 'export', $.name, $._export_desc, ')',
    ),

    _export_desc: $ => choice(
      seq('(', 'func', choice($.identifier, $.nat), ')'),
      seq('(', 'table', choice($.identifier, $.nat), ')'),
      seq('(', 'memory', choice($.identifier, $.nat), ')'),
      seq('(', 'global', choice($.identifier, $.nat), ')'),
      seq('(', 'tag', choice($.identifier, $.nat), ')'),
    ),

    // ---------------------------------------------------------------
    // Inline export / import
    // ---------------------------------------------------------------
    inline_export: $ => seq(
      '(', 'export', $.name, ')',
    ),

    inline_import: $ => seq(
      '(', 'import', $.name, $.name, ')',
    ),

    // ---------------------------------------------------------------
    // Functions
    // ---------------------------------------------------------------
    func: $ => seq(
      '(', 'func', optional($.identifier),
      repeat($.inline_export),
      optional($.inline_import),
      optional($.type_use),
      repeat($.param),
      repeat($.result),
      repeat($.local),
      repeat($._instr),
      ')',
    ),

    local: $ => seq('(', 'local', optional($.identifier), repeat1($._valtype), ')'),

    // ---------------------------------------------------------------
    // Tables
    // ---------------------------------------------------------------
    table: $ => seq(
      '(', 'table', optional($.identifier),
      repeat($.inline_export),
      optional($.inline_import),
      choice(
        $.table_type,
        // inline elem: reftype (elem ...)
        seq($.reftype, '(', 'elem', repeat($._elem_item), ')'),
      ),
      ')',
    ),

    table_type: $ => seq(
      $.limits,
      $.reftype,
    ),

    limits: $ => seq(
      $.nat,
      optional($.nat),
      optional('shared'),
    ),

    // ---------------------------------------------------------------
    // Memories
    // ---------------------------------------------------------------
    memory: $ => seq(
      '(', 'memory', optional($.identifier),
      repeat($.inline_export),
      optional($.inline_import),
      choice(
        $.memory_type,
        seq('(', 'data', repeat($.string), ')'),
      ),
      ')',
    ),

    memory_type: $ => $.limits,

    // ---------------------------------------------------------------
    // Globals
    // ---------------------------------------------------------------
    global: $ => seq(
      '(', 'global', optional($.identifier),
      repeat($.inline_export),
      optional($.inline_import),
      $._global_type,
      repeat($._instr),
      ')',
    ),

    _global_type: $ => choice(
      $._valtype,
      $.mut_type,
    ),

    // ---------------------------------------------------------------
    // Start
    // ---------------------------------------------------------------
    start: $ => seq(
      '(', 'start', choice($.identifier, $.nat), ')',
    ),

    // ---------------------------------------------------------------
    // Element segments
    // ---------------------------------------------------------------
    elem: $ => seq(
      '(', 'elem', optional($.identifier),
      repeat($._elem_field),
      ')',
    ),

    _elem_field: $ => choice(
      $._table_use,
      $._offset_expr,
      $._elem_item,
      $.reftype,
      'declare',
      'func',
      $.nat,
    ),

    _table_use: $ => seq(
      '(', 'table', choice($.identifier, $.nat), ')',
    ),

    _offset_expr: $ => choice(
      seq('(', 'offset', repeat1($._instr), ')'),
      $._folded_instr,
    ),

    _elem_item: $ => choice(
      seq('(', 'item', repeat1($._instr), ')'),
      $._folded_instr,
    ),

    // ---------------------------------------------------------------
    // Data segments
    // ---------------------------------------------------------------
    data: $ => seq(
      '(', 'data', optional($.identifier),
      optional(seq(
        optional($._memory_use),
        $._offset_expr,
      )),
      repeat($.string),
      ')',
    ),

    _memory_use: $ => seq(
      '(', 'memory', choice($.identifier, $.nat), ')',
    ),

    // ---------------------------------------------------------------
    // Tags (exception handling)
    // ---------------------------------------------------------------
    tag: $ => seq(
      '(', 'tag', optional($.identifier),
      repeat($.inline_export),
      optional($.inline_import),
      optional($.type_use),
      repeat($.param),
      repeat($.result),
      ')',
    ),

    // ---------------------------------------------------------------
    // Instructions
    // ---------------------------------------------------------------
    _instr: $ => choice(
      $.plain_instr,
      $.block_instr,
      $._folded_instr,
    ),

    // Folded instructions: (op ...)
    _folded_instr: $ => $.folded_instr,

    folded_instr: $ => seq(
      '(',
      choice(
        // block-like folded
        seq('block', optional($.identifier), optional($.block_sig), repeat($._instr)),
        seq('loop', optional($.identifier), optional($.block_sig), repeat($._instr)),
        seq('if', optional($.identifier), optional($.block_sig),
          repeat($._folded_instr),          // condition
          seq('(', 'then', repeat($._instr), ')'),
          optional(seq('(', 'else', repeat($._instr), ')')),
        ),
        seq('try_table', optional($.identifier), optional($.block_sig),
          repeat($.catch_clause),
          repeat($._instr),
        ),
        // plain folded
        seq($._plain_op, repeat($._folded_instr)),
      ),
      ')',
    ),

    // ---------------------------------------------------------------
    // Block instructions (non-folded)
    // ---------------------------------------------------------------
    block_instr: $ => choice(
      $.block_block,
      $.block_loop,
      $.block_if,
      $.block_try_table,
    ),

    block_block: $ => seq(
      'block', optional($.identifier), optional($.block_sig),
      repeat($._instr),
      'end', optional($.identifier),
    ),

    block_loop: $ => seq(
      'loop', optional($.identifier), optional($.block_sig),
      repeat($._instr),
      'end', optional($.identifier),
    ),

    block_if: $ => seq(
      'if', optional($.identifier), optional($.block_sig),
      repeat($._instr),
      optional(seq('else', optional($.identifier), repeat($._instr))),
      'end', optional($.identifier),
    ),

    block_try_table: $ => seq(
      'try_table', optional($.identifier), optional($.block_sig),
      repeat($.catch_clause),
      repeat($._instr),
      'end', optional($.identifier),
    ),

    block_sig: $ => repeat1(choice($.type_use, $.param, $.result)),

    catch_clause: $ => seq(
      '(',
      choice(
        seq('catch', choice($.identifier, $.nat), choice($.identifier, $.nat)),
        seq('catch_ref', choice($.identifier, $.nat), choice($.identifier, $.nat)),
        seq('catch_all', choice($.identifier, $.nat)),
        seq('catch_all_ref', choice($.identifier, $.nat)),
      ),
      ')',
    ),

    // ---------------------------------------------------------------
    // Plain (non-block, non-folded) instructions
    // ---------------------------------------------------------------
    plain_instr: $ => $._plain_op,

    _plain_op: $ => choice(
      // -- Control --
      'unreachable',
      'nop',
      seq('br', $._label_idx),
      seq('br_if', $._label_idx),
      seq('br_table', repeat1($._label_idx)),
      'return',
      seq('call', $._func_idx),
      seq('call_indirect', optional($._table_idx_arg), optional($.type_use), repeat(choice($.param, $.result))),
      // Tail call
      seq('return_call', $._func_idx),
      seq('return_call_indirect', optional($._table_idx_arg), optional($.type_use), repeat(choice($.param, $.result))),
      // Function references
      seq('call_ref', $._type_idx),
      seq('return_call_ref', $._type_idx),
      // Reference instructions
      seq('ref.null', $.heap_type),
      'ref.is_null',
      seq('ref.func', $._func_idx),
      'ref.as_non_null',
      seq('ref.cast', $._ref_cast_type),
      seq('ref.test', $._ref_cast_type),
      seq('br_on_null', $._label_idx),
      seq('br_on_non_null', $._label_idx),
      seq('br_on_cast', $._label_idx, $._ref_cast_type, $._ref_cast_type),
      seq('br_on_cast_fail', $._label_idx, $._ref_cast_type, $._ref_cast_type),

      // -- GC struct/array/i31/extern --
      seq('struct.new', $._type_idx),
      seq('struct.new_default', $._type_idx),
      seq('struct.get', $._type_idx, $._field_idx),
      seq('struct.get_s', $._type_idx, $._field_idx),
      seq('struct.get_u', $._type_idx, $._field_idx),
      seq('struct.set', $._type_idx, $._field_idx),
      seq('array.new', $._type_idx),
      seq('array.new_default', $._type_idx),
      seq('array.new_fixed', $._type_idx, $.nat),
      seq('array.new_elem', $._type_idx, $._elem_idx),
      seq('array.new_data', $._type_idx, $._data_idx),
      seq('array.get', $._type_idx),
      seq('array.get_s', $._type_idx),
      seq('array.get_u', $._type_idx),
      seq('array.set', $._type_idx),
      'array.len',
      seq('array.fill', $._type_idx),
      seq('array.copy', $._type_idx, $._type_idx),
      seq('array.init_data', $._type_idx, $._data_idx),
      seq('array.init_elem', $._type_idx, $._elem_idx),
      'i31.get_s',
      'i31.get_u',
      'extern.convert_any',
      'any.convert_extern',

      // -- Parametric --
      'drop',
      seq('select', optional(seq('(', 'result', repeat1($._valtype), ')'))),

      // -- Variable --
      seq('local.get', $._local_idx),
      seq('local.set', $._local_idx),
      seq('local.tee', $._local_idx),
      seq('global.get', $._global_idx),
      seq('global.set', $._global_idx),

      // -- Table --
      seq('table.get', optional($._table_idx_arg)),
      seq('table.set', optional($._table_idx_arg)),
      seq('table.size', optional($._table_idx_arg)),
      seq('table.grow', optional($._table_idx_arg)),
      seq('table.fill', optional($._table_idx_arg)),
      seq('table.copy', optional($._table_idx_arg), optional($._table_idx_arg)),
      seq('table.init', optional($._table_idx_arg), $._elem_idx),
      seq('elem.drop', $._elem_idx),

      // -- Memory load/store (multi-memory, memory64) --
      seq($._mem_load_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq($._mem_store_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),

      // Memory control
      seq('memory.size', optional($._memory_idx_arg)),
      seq('memory.grow', optional($._memory_idx_arg)),
      seq('memory.fill', optional($._memory_idx_arg)),
      seq('memory.copy', optional($._memory_idx_arg), optional($._memory_idx_arg)),
      seq('memory.init', optional($._memory_idx_arg), $._data_idx),
      seq('data.drop', $._data_idx),

      // -- Numeric const --
      seq('i32.const', $.int),
      seq('i64.const', $.int),
      seq('f32.const', $.float),
      seq('f64.const', $.float),

      // -- Numeric ops (i32, i64, f32, f64) --
      $._numeric_op,

      // -- SIMD --
      seq('v128.const', $._v128_shape, repeat($._num)),
      seq('v128.load', optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq('v128.store', optional($._memory_idx_arg), optional($.offset), optional($.align)),
      $._simd_load_op,
      $._simd_op,

      // -- Bulk memory (standalone) --

      // -- Threads / Atomics --
      seq('atomic.fence', optional('0x0')),
      seq($._atomic_load_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq($._atomic_store_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq($._atomic_rmw_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq($._atomic_cmpxchg_op, optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq('memory.atomic.notify', optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq('memory.atomic.wait32', optional($._memory_idx_arg), optional($.offset), optional($.align)),
      seq('memory.atomic.wait64', optional($._memory_idx_arg), optional($.offset), optional($.align)),

      // -- Exception handling --
      seq('throw', $._tag_idx),
      'throw_ref',
    ),

    // ref cast type: (ref null? heaptype) or just heaptype shorthand
    _ref_cast_type: $ => choice(
      $.reftype,
      $.heap_type,
    ),

    // ---------------------------------------------------------------
    // Memory load/store op tokens
    // ---------------------------------------------------------------
    _mem_load_op: _$ => token(choice(
      'i32.load', 'i64.load', 'f32.load', 'f64.load',
      'i32.load8_s', 'i32.load8_u', 'i32.load16_s', 'i32.load16_u',
      'i64.load8_s', 'i64.load8_u', 'i64.load16_s', 'i64.load16_u',
      'i64.load32_s', 'i64.load32_u',
    )),

    _mem_store_op: _$ => token(choice(
      'i32.store', 'i64.store', 'f32.store', 'f64.store',
      'i32.store8', 'i32.store16',
      'i64.store8', 'i64.store16', 'i64.store32',
    )),

    // ---------------------------------------------------------------
    // SIMD load ops (with mem arg)
    // ---------------------------------------------------------------
    _simd_load_op: $ => seq(
      $._simd_load_op_name,
      optional($._memory_idx_arg),
      optional($.offset),
      optional($.align),
    ),

    _simd_load_op_name: _$ => token(choice(
      'v128.load8x8_s', 'v128.load8x8_u',
      'v128.load16x4_s', 'v128.load16x4_u',
      'v128.load32x2_s', 'v128.load32x2_u',
      'v128.load8_splat', 'v128.load16_splat',
      'v128.load32_splat', 'v128.load64_splat',
      'v128.load32_zero', 'v128.load64_zero',
      'v128.load8_lane', 'v128.load16_lane',
      'v128.load32_lane', 'v128.load64_lane',
      'v128.store8_lane', 'v128.store16_lane',
      'v128.store32_lane', 'v128.store64_lane',
    )),

    // ---------------------------------------------------------------
    // Numeric ALU ops (all flavors)
    // ---------------------------------------------------------------
    _numeric_op: _$ => token(choice(
      // i32
      'i32.clz', 'i32.ctz', 'i32.popcnt',
      'i32.add', 'i32.sub', 'i32.mul',
      'i32.div_s', 'i32.div_u', 'i32.rem_s', 'i32.rem_u',
      'i32.and', 'i32.or', 'i32.xor',
      'i32.shl', 'i32.shr_s', 'i32.shr_u',
      'i32.rotl', 'i32.rotr',
      'i32.eqz', 'i32.eq', 'i32.ne',
      'i32.lt_s', 'i32.lt_u', 'i32.gt_s', 'i32.gt_u',
      'i32.le_s', 'i32.le_u', 'i32.ge_s', 'i32.ge_u',
      'i32.wrap_i64',
      'i32.trunc_f32_s', 'i32.trunc_f32_u',
      'i32.trunc_f64_s', 'i32.trunc_f64_u',
      'i32.reinterpret_f32',
      'i32.extend8_s', 'i32.extend16_s',
      'i32.trunc_sat_f32_s', 'i32.trunc_sat_f32_u',
      'i32.trunc_sat_f64_s', 'i32.trunc_sat_f64_u',
      // i64
      'i64.clz', 'i64.ctz', 'i64.popcnt',
      'i64.add', 'i64.sub', 'i64.mul',
      'i64.div_s', 'i64.div_u', 'i64.rem_s', 'i64.rem_u',
      'i64.and', 'i64.or', 'i64.xor',
      'i64.shl', 'i64.shr_s', 'i64.shr_u',
      'i64.rotl', 'i64.rotr',
      'i64.eqz', 'i64.eq', 'i64.ne',
      'i64.lt_s', 'i64.lt_u', 'i64.gt_s', 'i64.gt_u',
      'i64.le_s', 'i64.le_u', 'i64.ge_s', 'i64.ge_u',
      'i64.extend_i32_s', 'i64.extend_i32_u',
      'i64.trunc_f32_s', 'i64.trunc_f32_u',
      'i64.trunc_f64_s', 'i64.trunc_f64_u',
      'i64.reinterpret_f64',
      'i64.extend8_s', 'i64.extend16_s', 'i64.extend32_s',
      'i64.trunc_sat_f32_s', 'i64.trunc_sat_f32_u',
      'i64.trunc_sat_f64_s', 'i64.trunc_sat_f64_u',
      // f32
      'f32.abs', 'f32.neg', 'f32.ceil', 'f32.floor', 'f32.trunc', 'f32.nearest', 'f32.sqrt',
      'f32.add', 'f32.sub', 'f32.mul', 'f32.div',
      'f32.min', 'f32.max', 'f32.copysign',
      'f32.eq', 'f32.ne', 'f32.lt', 'f32.gt', 'f32.le', 'f32.ge',
      'f32.convert_i32_s', 'f32.convert_i32_u',
      'f32.convert_i64_s', 'f32.convert_i64_u',
      'f32.demote_f64', 'f32.reinterpret_i32',
      // f64
      'f64.abs', 'f64.neg', 'f64.ceil', 'f64.floor', 'f64.trunc', 'f64.nearest', 'f64.sqrt',
      'f64.add', 'f64.sub', 'f64.mul', 'f64.div',
      'f64.min', 'f64.max', 'f64.copysign',
      'f64.eq', 'f64.ne', 'f64.lt', 'f64.gt', 'f64.le', 'f64.ge',
      'f64.convert_i32_s', 'f64.convert_i32_u',
      'f64.convert_i64_s', 'f64.convert_i64_u',
      'f64.promote_f32', 'f64.reinterpret_i64',
    )),

    // ---------------------------------------------------------------
    // SIMD ops (no memory arg)
    // ---------------------------------------------------------------
    _simd_op: _$ => token(choice(
      'v128.not', 'v128.and', 'v128.andnot', 'v128.or', 'v128.xor',
      'v128.bitselect', 'v128.any_true',
      // i8x16
      'i8x16.splat', 'i8x16.extract_lane_s', 'i8x16.extract_lane_u', 'i8x16.replace_lane',
      'i8x16.eq', 'i8x16.ne', 'i8x16.lt_s', 'i8x16.lt_u', 'i8x16.gt_s', 'i8x16.gt_u',
      'i8x16.le_s', 'i8x16.le_u', 'i8x16.ge_s', 'i8x16.ge_u',
      'i8x16.abs', 'i8x16.neg', 'i8x16.popcnt',
      'i8x16.all_true', 'i8x16.bitmask',
      'i8x16.narrow_i16x8_s', 'i8x16.narrow_i16x8_u',
      'i8x16.shl', 'i8x16.shr_s', 'i8x16.shr_u',
      'i8x16.add', 'i8x16.add_sat_s', 'i8x16.add_sat_u',
      'i8x16.sub', 'i8x16.sub_sat_s', 'i8x16.sub_sat_u',
      'i8x16.min_s', 'i8x16.min_u', 'i8x16.max_s', 'i8x16.max_u',
      'i8x16.avgr_u',
      'i8x16.shuffle', 'i8x16.swizzle',
      // i16x8
      'i16x8.splat', 'i16x8.extract_lane_s', 'i16x8.extract_lane_u', 'i16x8.replace_lane',
      'i16x8.eq', 'i16x8.ne', 'i16x8.lt_s', 'i16x8.lt_u', 'i16x8.gt_s', 'i16x8.gt_u',
      'i16x8.le_s', 'i16x8.le_u', 'i16x8.ge_s', 'i16x8.ge_u',
      'i16x8.abs', 'i16x8.neg',
      'i16x8.all_true', 'i16x8.bitmask',
      'i16x8.narrow_i32x4_s', 'i16x8.narrow_i32x4_u',
      'i16x8.extend_low_i8x16_s', 'i16x8.extend_low_i8x16_u',
      'i16x8.extend_high_i8x16_s', 'i16x8.extend_high_i8x16_u',
      'i16x8.shl', 'i16x8.shr_s', 'i16x8.shr_u',
      'i16x8.add', 'i16x8.add_sat_s', 'i16x8.add_sat_u',
      'i16x8.sub', 'i16x8.sub_sat_s', 'i16x8.sub_sat_u',
      'i16x8.mul',
      'i16x8.min_s', 'i16x8.min_u', 'i16x8.max_s', 'i16x8.max_u',
      'i16x8.avgr_u',
      'i16x8.extmul_low_i8x16_s', 'i16x8.extmul_low_i8x16_u',
      'i16x8.extmul_high_i8x16_s', 'i16x8.extmul_high_i8x16_u',
      'i16x8.q15mulr_sat_s',
      'i16x8.extadd_pairwise_i8x16_s', 'i16x8.extadd_pairwise_i8x16_u',
      // i32x4
      'i32x4.splat', 'i32x4.extract_lane', 'i32x4.replace_lane',
      'i32x4.eq', 'i32x4.ne', 'i32x4.lt_s', 'i32x4.lt_u', 'i32x4.gt_s', 'i32x4.gt_u',
      'i32x4.le_s', 'i32x4.le_u', 'i32x4.ge_s', 'i32x4.ge_u',
      'i32x4.abs', 'i32x4.neg',
      'i32x4.all_true', 'i32x4.bitmask',
      'i32x4.extend_low_i16x8_s', 'i32x4.extend_low_i16x8_u',
      'i32x4.extend_high_i16x8_s', 'i32x4.extend_high_i16x8_u',
      'i32x4.shl', 'i32x4.shr_s', 'i32x4.shr_u',
      'i32x4.add', 'i32x4.sub', 'i32x4.mul',
      'i32x4.min_s', 'i32x4.min_u', 'i32x4.max_s', 'i32x4.max_u',
      'i32x4.dot_i16x8_s',
      'i32x4.extmul_low_i16x8_s', 'i32x4.extmul_low_i16x8_u',
      'i32x4.extmul_high_i16x8_s', 'i32x4.extmul_high_i16x8_u',
      'i32x4.extadd_pairwise_i16x8_s', 'i32x4.extadd_pairwise_i16x8_u',
      'i32x4.trunc_sat_f32x4_s', 'i32x4.trunc_sat_f32x4_u',
      'i32x4.trunc_sat_f64x2_s_zero', 'i32x4.trunc_sat_f64x2_u_zero',
      // i64x2
      'i64x2.splat', 'i64x2.extract_lane', 'i64x2.replace_lane',
      'i64x2.eq', 'i64x2.ne', 'i64x2.lt_s', 'i64x2.gt_s', 'i64x2.le_s', 'i64x2.ge_s',
      'i64x2.abs', 'i64x2.neg',
      'i64x2.all_true', 'i64x2.bitmask',
      'i64x2.extend_low_i32x4_s', 'i64x2.extend_low_i32x4_u',
      'i64x2.extend_high_i32x4_s', 'i64x2.extend_high_i32x4_u',
      'i64x2.shl', 'i64x2.shr_s', 'i64x2.shr_u',
      'i64x2.add', 'i64x2.sub', 'i64x2.mul',
      'i64x2.extmul_low_i32x4_s', 'i64x2.extmul_low_i32x4_u',
      'i64x2.extmul_high_i32x4_s', 'i64x2.extmul_high_i32x4_u',
      // f32x4
      'f32x4.splat', 'f32x4.extract_lane', 'f32x4.replace_lane',
      'f32x4.eq', 'f32x4.ne', 'f32x4.lt', 'f32x4.gt', 'f32x4.le', 'f32x4.ge',
      'f32x4.abs', 'f32x4.neg', 'f32x4.sqrt',
      'f32x4.add', 'f32x4.sub', 'f32x4.mul', 'f32x4.div',
      'f32x4.min', 'f32x4.max', 'f32x4.pmin', 'f32x4.pmax',
      'f32x4.ceil', 'f32x4.floor', 'f32x4.trunc', 'f32x4.nearest',
      'f32x4.convert_i32x4_s', 'f32x4.convert_i32x4_u',
      'f32x4.demote_f64x2_zero',
      // f64x2
      'f64x2.splat', 'f64x2.extract_lane', 'f64x2.replace_lane',
      'f64x2.eq', 'f64x2.ne', 'f64x2.lt', 'f64x2.gt', 'f64x2.le', 'f64x2.ge',
      'f64x2.abs', 'f64x2.neg', 'f64x2.sqrt',
      'f64x2.add', 'f64x2.sub', 'f64x2.mul', 'f64x2.div',
      'f64x2.min', 'f64x2.max', 'f64x2.pmin', 'f64x2.pmax',
      'f64x2.ceil', 'f64x2.floor', 'f64x2.trunc', 'f64x2.nearest',
      'f64x2.convert_low_i32x4_s', 'f64x2.convert_low_i32x4_u',
      'f64x2.promote_low_f32x4',
      // Relaxed SIMD
      'i8x16.relaxed_swizzle',
      'i32x4.relaxed_trunc_f32x4_s', 'i32x4.relaxed_trunc_f32x4_u',
      'i32x4.relaxed_trunc_f64x2_s_zero', 'i32x4.relaxed_trunc_f64x2_u_zero',
      'f32x4.relaxed_madd', 'f32x4.relaxed_nmadd',
      'f64x2.relaxed_madd', 'f64x2.relaxed_nmadd',
      'i8x16.relaxed_laneselect', 'i16x8.relaxed_laneselect',
      'i32x4.relaxed_laneselect', 'i64x2.relaxed_laneselect',
      'f32x4.relaxed_min', 'f32x4.relaxed_max',
      'f64x2.relaxed_min', 'f64x2.relaxed_max',
      'i16x8.relaxed_q15mulr_s',
      'i16x8.relaxed_dot_i8x16_i7x16_s',
      'i32x4.relaxed_dot_i8x16_i7x16_add_s',
    )),

    // v128.const shape
    _v128_shape: _$ => token(choice(
      'i8x16', 'i16x8', 'i32x4', 'i64x2', 'f32x4', 'f64x2',
    )),

    // ---------------------------------------------------------------
    // Atomic ops
    // ---------------------------------------------------------------
    _atomic_load_op: _$ => token(choice(
      'i32.atomic.load', 'i64.atomic.load',
      'i32.atomic.load8_u', 'i32.atomic.load16_u',
      'i64.atomic.load8_u', 'i64.atomic.load16_u', 'i64.atomic.load32_u',
    )),

    _atomic_store_op: _$ => token(choice(
      'i32.atomic.store', 'i64.atomic.store',
      'i32.atomic.store8', 'i32.atomic.store16',
      'i64.atomic.store8', 'i64.atomic.store16', 'i64.atomic.store32',
    )),

    _atomic_rmw_op: _$ => token(choice(
      'i32.atomic.rmw.add', 'i64.atomic.rmw.add',
      'i32.atomic.rmw.sub', 'i64.atomic.rmw.sub',
      'i32.atomic.rmw.and', 'i64.atomic.rmw.and',
      'i32.atomic.rmw.or', 'i64.atomic.rmw.or',
      'i32.atomic.rmw.xor', 'i64.atomic.rmw.xor',
      'i32.atomic.rmw.xchg', 'i64.atomic.rmw.xchg',
      'i32.atomic.rmw8.add_u', 'i32.atomic.rmw16.add_u',
      'i64.atomic.rmw8.add_u', 'i64.atomic.rmw16.add_u', 'i64.atomic.rmw32.add_u',
      'i32.atomic.rmw8.sub_u', 'i32.atomic.rmw16.sub_u',
      'i64.atomic.rmw8.sub_u', 'i64.atomic.rmw16.sub_u', 'i64.atomic.rmw32.sub_u',
      'i32.atomic.rmw8.and_u', 'i32.atomic.rmw16.and_u',
      'i64.atomic.rmw8.and_u', 'i64.atomic.rmw16.and_u', 'i64.atomic.rmw32.and_u',
      'i32.atomic.rmw8.or_u', 'i32.atomic.rmw16.or_u',
      'i64.atomic.rmw8.or_u', 'i64.atomic.rmw16.or_u', 'i64.atomic.rmw32.or_u',
      'i32.atomic.rmw8.xor_u', 'i32.atomic.rmw16.xor_u',
      'i64.atomic.rmw8.xor_u', 'i64.atomic.rmw16.xor_u', 'i64.atomic.rmw32.xor_u',
      'i32.atomic.rmw8.xchg_u', 'i32.atomic.rmw16.xchg_u',
      'i64.atomic.rmw8.xchg_u', 'i64.atomic.rmw16.xchg_u', 'i64.atomic.rmw32.xchg_u',
    )),

    _atomic_cmpxchg_op: _$ => token(choice(
      'i32.atomic.rmw.cmpxchg', 'i64.atomic.rmw.cmpxchg',
      'i32.atomic.rmw8.cmpxchg_u', 'i32.atomic.rmw16.cmpxchg_u',
      'i64.atomic.rmw8.cmpxchg_u', 'i64.atomic.rmw16.cmpxchg_u', 'i64.atomic.rmw32.cmpxchg_u',
    )),

    // ---------------------------------------------------------------
    // Memory arguments
    // ---------------------------------------------------------------
    offset: _$ => token(seq('offset=', /[0-9][0-9_]*|0x[0-9A-Fa-f][0-9A-Fa-f_]*/)),
    align: _$ => token(seq('align=', /[0-9][0-9_]*|0x[0-9A-Fa-f][0-9A-Fa-f_]*/)),

    // ---------------------------------------------------------------
    // Index helpers
    // ---------------------------------------------------------------
    _label_idx: $ => choice($.identifier, $.nat),
    _func_idx: $ => choice($.identifier, $.nat),
    _local_idx: $ => choice($.identifier, $.nat),
    _global_idx: $ => choice($.identifier, $.nat),
    _table_idx_arg: $ => choice($.identifier, $.nat),
    _memory_idx_arg: $ => choice($.identifier, $.nat),
    _data_idx: $ => choice($.identifier, $.nat),
    _elem_idx: $ => choice($.identifier, $.nat),
    _tag_idx: $ => choice($.identifier, $.nat),
    _field_idx: $ => choice($.identifier, $.nat),

    // ---------------------------------------------------------------
    // Comments
    // ---------------------------------------------------------------
    line_comment: _$ => token(prec(PREC.COMMENT, seq(';;', /[^\n]*/))),

    // Non-nested block comments: (;  ...  ;)
    // Tree-sitter doesn't support lookahead in regexes, so we match
    // the shortest sequence ending in ;) by consuming characters that
    // are not ';', or ';' not followed by ')'.
    // We approximate by matching any chars (non-greedy isn't available either)
    // so we use repeat with character classes.
    block_comment: _$ => token(prec(PREC.COMMENT, seq(
      '(;',
      repeat(choice(
        /[^;]/,
        /;[^)]/,
      )),
      ';)',
    ))),

    // ---------------------------------------------------------------
    // Annotations: (@id ...)
    // ---------------------------------------------------------------
    annotation: $ => seq(
      '(',
      token.immediate(seq('@', repeat1(ID_CHAR))),
      repeat($._annotation_content),
      ')',
    ),

    _annotation_content: $ => choice(
      $.identifier,
      $.nat,
      $.int,
      $.float,
      $.string,
      $.reserved,
      seq('(', repeat($._annotation_content), ')'),
    ),
  },
});
