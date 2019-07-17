(module
  (type (;0;) (func (param i32 i32) (result i32)))
  (type (;1;) (func (param i64 i64) (result i32)))
  (func (;0;) (type 0) (param i32 i32) (result i32)
    get_local 0
    i32.const 1
    i32.add
    get_local 1
    i32.const 1
    i32.add
    i32.lt_s)
  (func (;1;) (type 0) (param i32 i32) (result i32)
    get_local 0
    i32.const 1
    i32.add
    get_local 1
    i32.const 1
    i32.add
    i32.lt_u)
  (func (;2;) (type 1) (param i64 i64) (result i32)
    get_local 0
    i64.const 1
    i64.add
    get_local 1
    i64.const 1
    i64.add
    i64.lt_s)
  (func (;3;) (type 1) (param i64 i64) (result i32)
    get_local 0
    i64.const 1
    i64.add
    get_local 1
    i64.const 1
    i64.add
    i64.lt_u)
  (export "i32.no_fold_cmp_s_offset" (func 0))
  (export "i32.no_fold_cmp_u_offset" (func 1))
  (export "i64.no_fold_cmp_s_offset" (func 2))
  (export "i64.no_fold_cmp_u_offset" (func 3)))
