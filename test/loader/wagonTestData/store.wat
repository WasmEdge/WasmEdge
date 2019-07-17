(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (result i64)))
  (func (;0;) (type 0) (result i32)
    (i32.store8
      (i32.const 0)
      (i32.const 251))
    (i32.store8
      (i32.const 1)
      (i32.const 252))
    (i32.store8
      (i32.const 2)
      (i32.const 253))
    (i32.store8
      (i32.const 3)
      (i32.const 254))
    (i32.load
      (i32.const 0)))
  (func (;1;) (type 0) (result i32)
    (i32.store16
      (i32.const 0)
      (i32.const 51913))
    (i32.store16
      (i32.const 2)
      (i32.const 52427))
    (i32.load
      (i32.const 0)))
  (func (;2;) (type 0) (result i32)
    (i32.store
      (i32.const 0)
      (i32.const -123456))
    (i32.load
      (i32.const 0)))
  (func (;3;) (type 0) (result i32)
    (i64.store8
      (i32.const 0)
      (i64.const -1229782938247303429))
    (i64.store8
      (i32.const 1)
      (i64.const -1229782938247303428))
    (i64.store8
      (i32.const 2)
      (i64.const -1229782938247303427))
    (i64.store8
      (i32.const 3)
      (i64.const -1229782938247303426))
    (i32.load
      (i32.const 0)))
  (func (;4;) (type 0) (result i32)
    (i64.store16
      (i32.const 0)
      (i64.const -1229782938247312695))
    (i64.store16
      (i32.const 2)
      (i64.const -1229782938247312181))
    (i32.load
      (i32.const 0)))
  (func (;5;) (type 0) (result i32)
    (i64.store32
      (i32.const 0)
      (i64.const -123456))
    (i32.load
      (i32.const 0)))
  (func (;6;) (type 1) (result i64)
    (i64.store
      (i32.const 0)
      (i64.const -4981613551475109875))
    (i64.load
      (i32.const 0)))
  (func (;7;) (type 0) (result i32)
    (f32.store
      (i32.const 0)
      (f32.const 0x1.8p+0 (;=1.5;)))
    (i32.load
      (i32.const 0)))
  (func (;8;) (type 0) (result i32)
    (f64.store
      (i32.const 0)
      (f64.const -0x1.f46p+9 (;=-1000.75;)))
    (i32.load
      (i32.const 4)))
  (memory (;0;) 1)
  (export "i32_store8" (func 0))
  (export "i32_store16" (func 1))
  (export "i32_store" (func 2))
  (export "i64_store8" (func 3))
  (export "i64_store16" (func 4))
  (export "i64_store32" (func 5))
  (export "i64_store" (func 6))
  (export "f32_store" (func 7))
  (export "f64_store" (func 8)))
