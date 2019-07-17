(module
  (type (;0;) (func (param i32 i32) (result i32)))
  (type (;1;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (param i32 i32) (result i32)
    (i32.add
      (local.get 0)
      (local.get 1)))
  (func (;1;) (type 0) (param i32 i32) (result i32)
    (i32.sub
      (local.get 0)
      (local.get 1)))
  (func (;2;) (type 0) (param i32 i32) (result i32)
    (i32.mul
      (local.get 0)
      (local.get 1)))
  (func (;3;) (type 0) (param i32 i32) (result i32)
    (i32.div_s
      (local.get 0)
      (local.get 1)))
  (func (;4;) (type 0) (param i32 i32) (result i32)
    (i32.div_u
      (local.get 0)
      (local.get 1)))
  (func (;5;) (type 0) (param i32 i32) (result i32)
    (i32.rem_s
      (local.get 0)
      (local.get 1)))
  (func (;6;) (type 0) (param i32 i32) (result i32)
    (i32.rem_u
      (local.get 0)
      (local.get 1)))
  (func (;7;) (type 0) (param i32 i32) (result i32)
    (i32.and
      (local.get 0)
      (local.get 1)))
  (func (;8;) (type 0) (param i32 i32) (result i32)
    (i32.or
      (local.get 0)
      (local.get 1)))
  (func (;9;) (type 0) (param i32 i32) (result i32)
    (i32.xor
      (local.get 0)
      (local.get 1)))
  (func (;10;) (type 0) (param i32 i32) (result i32)
    (i32.shl
      (local.get 0)
      (local.get 1)))
  (func (;11;) (type 0) (param i32 i32) (result i32)
    (i32.shr_s
      (local.get 0)
      (local.get 1)))
  (func (;12;) (type 0) (param i32 i32) (result i32)
    (i32.shr_u
      (local.get 0)
      (local.get 1)))
  (func (;13;) (type 0) (param i32 i32) (result i32)
    (i32.rotl
      (local.get 0)
      (local.get 1)))
  (func (;14;) (type 0) (param i32 i32) (result i32)
    (i32.rotr
      (local.get 0)
      (local.get 1)))
  (func (;15;) (type 1) (param i32) (result i32)
    (i32.clz
      (local.get 0)))
  (func (;16;) (type 1) (param i32) (result i32)
    (i32.ctz
      (local.get 0)))
  (func (;17;) (type 1) (param i32) (result i32)
    (i32.popcnt
      (local.get 0)))
  (func (;18;) (type 1) (param i32) (result i32)
    (i32.eqz
      (local.get 0)))
  (func (;19;) (type 0) (param i32 i32) (result i32)
    (i32.eq
      (local.get 0)
      (local.get 1)))
  (func (;20;) (type 0) (param i32 i32) (result i32)
    (i32.ne
      (local.get 0)
      (local.get 1)))
  (func (;21;) (type 0) (param i32 i32) (result i32)
    (i32.lt_s
      (local.get 0)
      (local.get 1)))
  (func (;22;) (type 0) (param i32 i32) (result i32)
    (i32.lt_u
      (local.get 0)
      (local.get 1)))
  (func (;23;) (type 0) (param i32 i32) (result i32)
    (i32.le_s
      (local.get 0)
      (local.get 1)))
  (func (;24;) (type 0) (param i32 i32) (result i32)
    (i32.le_u
      (local.get 0)
      (local.get 1)))
  (func (;25;) (type 0) (param i32 i32) (result i32)
    (i32.gt_s
      (local.get 0)
      (local.get 1)))
  (func (;26;) (type 0) (param i32 i32) (result i32)
    (i32.gt_u
      (local.get 0)
      (local.get 1)))
  (func (;27;) (type 0) (param i32 i32) (result i32)
    (i32.ge_s
      (local.get 0)
      (local.get 1)))
  (func (;28;) (type 0) (param i32 i32) (result i32)
    (i32.ge_u
      (local.get 0)
      (local.get 1)))
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
