(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (result i64)))
  (type (;2;) (func (result f32)))
  (type (;3;) (func (result f64)))
  (func (;0;) (type 0) (result i32)
    (i32.load8_s
      (i32.const 0)))
  (func (;1;) (type 0) (result i32)
    (i32.load16_s
      (i32.const 0)))
  (func (;2;) (type 0) (result i32)
    (i32.load
      (i32.const 0)))
  (func (;3;) (type 0) (result i32)
    (i32.load8_u
      (i32.const 0)))
  (func (;4;) (type 0) (result i32)
    (i32.load16_u
      (i32.const 0)))
  (func (;5;) (type 1) (result i64)
    (i64.load8_s
      (i32.const 0)))
  (func (;6;) (type 1) (result i64)
    (i64.load16_s
      (i32.const 0)))
  (func (;7;) (type 1) (result i64)
    (i64.load32_s
      (i32.const 0)))
  (func (;8;) (type 1) (result i64)
    (i64.load
      (i32.const 16)))
  (func (;9;) (type 1) (result i64)
    (i64.load8_u
      (i32.const 0)))
  (func (;10;) (type 1) (result i64)
    (i64.load16_u
      (i32.const 0)))
  (func (;11;) (type 1) (result i64)
    (i64.load32_u
      (i32.const 0)))
  (func (;12;) (type 2) (result f32)
    (f32.load
      (i32.const 4)))
  (func (;13;) (type 3) (result f64)
    (f64.load
      (i32.const 8)))
  (memory (;0;) 1)
  (export "i32_load8_s" (func 0))
  (export "i32_load16_s" (func 1))
  (export "i32_load" (func 2))
  (export "i32_load8_u" (func 3))
  (export "i32_load16_u" (func 4))
  (export "i64_load8_s" (func 5))
  (export "i64_load16_s" (func 6))
  (export "i64_load32_s" (func 7))
  (export "i64_load" (func 8))
  (export "i64_load8_u" (func 9))
  (export "i64_load16_u" (func 10))
  (export "i64_load32_u" (func 11))
  (export "f32_load" (func 12))
  (export "f64_load" (func 13))
  (data (;0;) (i32.const 0) "\ff\ff\ff\ff")
  (data (;1;) (i32.const 4) "\00\00\ceA")
  (data (;2;) (i32.const 8) "\00\00\00\00\00\ff\8f@")
  (data (;3;) (i32.const 16) "\ff\ff\ff\ff\ff\ff\ff\ff"))
