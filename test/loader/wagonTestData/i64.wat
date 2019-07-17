(module
  (type (;0;) (func (param i64 i64) (result i64)))
  (type (;1;) (func (param i64) (result i64)))
  (type (;2;) (func (param i64) (result i32)))
  (type (;3;) (func (param i64 i64) (result i32)))
  (func (;0;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.add)
  (func (;1;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.sub)
  (func (;2;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.mul)
  (func (;3;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.div_s)
  (func (;4;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.div_u)
  (func (;5;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.rem_s)
  (func (;6;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.rem_u)
  (func (;7;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.and)
  (func (;8;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.or)
  (func (;9;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.xor)
  (func (;10;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.shl)
  (func (;11;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.shr_s)
  (func (;12;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.shr_u)
  (func (;13;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.rotl)
  (func (;14;) (type 0) (param i64 i64) (result i64)
    get_local 0
    get_local 1
    i64.rotr)
  (func (;15;) (type 1) (param i64) (result i64)
    get_local 0
    i64.clz)
  (func (;16;) (type 1) (param i64) (result i64)
    get_local 0
    i64.ctz)
  (func (;17;) (type 1) (param i64) (result i64)
    get_local 0
    i64.popcnt)
  (func (;18;) (type 2) (param i64) (result i32)
    get_local 0
    i64.eqz)
  (func (;19;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.eq)
  (func (;20;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.ne)
  (func (;21;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.lt_s)
  (func (;22;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.lt_u)
  (func (;23;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.le_s)
  (func (;24;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.le_u)
  (func (;25;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.gt_s)
  (func (;26;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.gt_u)
  (func (;27;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.ge_s)
  (func (;28;) (type 3) (param i64 i64) (result i32)
    get_local 0
    get_local 1
    i64.ge_u)
  (export "add" (func 0))
  (export "sub" (func 1))
  (export "mul" (func 2))
  (export "div_s" (func 3))
  (export "div_u" (func 4))
  (export "rem_s" (func 5))
  (export "rem_u" (func 6))
  (export "and" (func 7))
  (export "or" (func 8))
  (export "xor" (func 9))
  (export "shl" (func 10))
  (export "shr_s" (func 11))
  (export "shr_u" (func 12))
  (export "rotl" (func 13))
  (export "rotr" (func 14))
  (export "clz" (func 15))
  (export "ctz" (func 16))
  (export "popcnt" (func 17))
  (export "eqz" (func 18))
  (export "eq" (func 19))
  (export "ne" (func 20))
  (export "lt_s" (func 21))
  (export "lt_u" (func 22))
  (export "le_s" (func 23))
  (export "le_u" (func 24))
  (export "gt_s" (func 25))
  (export "gt_u" (func 26))
  (export "ge_s" (func 27))
  (export "ge_u" (func 28)))
