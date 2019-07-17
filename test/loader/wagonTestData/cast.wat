(module
  (type (;0;) (func (result f32)))
  (type (;1;) (func (result i32)))
  (type (;2;) (func (result f64)))
  (type (;3;) (func (result i64)))
  (func (;0;) (type 0) (result f32)
    (f32.reinterpret_i32
      (i32.const 1083179008)))
  (func (;1;) (type 1) (result i32)
    (i32.reinterpret_f32
      (f32.const -0x1.cp+1 (;=-3.5;))))
  (func (;2;) (type 2) (result f64)
    (f64.reinterpret_i64
      (i64.const 4638505306052100096)))
  (func (;3;) (type 3) (result i64)
    (i64.reinterpret_f64
      (f64.const 0x1.99c82ccp+33 (;=1.375e+10;))))
  (export "f32_reinterpret_i32" (func 0))
  (export "i32_reinterpret_f32" (func 1))
  (export "f64_reinterpret_i64" (func 2))
  (export "i64_reinterpret_f64" (func 3)))
