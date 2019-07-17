(module
  (type (;0;) (func (param f32) (result i32)))
  (type (;1;) (func (param f64) (result i32)))
  (type (;2;) (func (result i32)))
  (type (;3;) (func (result i64)))
  (type (;4;) (func (result f32)))
  (type (;5;) (func (result f64)))
  (func (;0;) (type 0) (param f32) (result i32)
    (f32.ne
      (local.get 0)
      (local.get 0)))
  (func (;1;) (type 1) (param f64) (result i32)
    (f64.ne
      (local.get 0)
      (local.get 0)))
  (func (;2;) (type 2) (result i32)
    (i32.eqz
      (i32.const 100)))
  (func (;3;) (type 2) (result i32)
    (i32.eqz
      (i32.const 0)))
  (func (;4;) (type 2) (result i32)
    (i32.clz
      (i32.const 128)))
  (func (;5;) (type 2) (result i32)
    (i32.ctz
      (i32.const 128)))
  (func (;6;) (type 2) (result i32)
    (i32.popcnt
      (i32.const 128)))
  (func (;7;) (type 2) (result i32)
    (i64.eqz
      (i64.const 100)))
  (func (;8;) (type 2) (result i32)
    (i64.eqz
      (i64.const 0)))
  (func (;9;) (type 3) (result i64)
    (i64.clz
      (i64.const 128)))
  (func (;10;) (type 3) (result i64)
    (i64.ctz
      (i64.const 128)))
  (func (;11;) (type 3) (result i64)
    (i64.popcnt
      (i64.const 128)))
  (func (;12;) (type 4) (result f32)
    (f32.neg
      (f32.const 0x1.9p+6 (;=100;))))
  (func (;13;) (type 4) (result f32)
    (f32.abs
      (f32.const -0x1.9p+6 (;=-100;))))
  (func (;14;) (type 2) (result i32)
    (call 0
      (f32.sqrt
        (f32.const -0x1.9p+6 (;=-100;)))))
  (func (;15;) (type 4) (result f32)
    (f32.sqrt
      (f32.const 0x1.9p+6 (;=100;))))
  (func (;16;) (type 4) (result f32)
    (f32.ceil
      (f32.const -0x1.8p-1 (;=-0.75;))))
  (func (;17;) (type 4) (result f32)
    (f32.floor
      (f32.const -0x1.8p-1 (;=-0.75;))))
  (func (;18;) (type 4) (result f32)
    (f32.trunc
      (f32.const -0x1.8p-1 (;=-0.75;))))
  (func (;19;) (type 4) (result f32)
    (f32.nearest
      (f32.const 0x1.4p+0 (;=1.25;))))
  (func (;20;) (type 4) (result f32)
    (f32.nearest
      (f32.const 0x1.cp+0 (;=1.75;))))
  (func (;21;) (type 5) (result f64)
    (f64.neg
      (f64.const 0x1.9p+6 (;=100;))))
  (func (;22;) (type 5) (result f64)
    (f64.abs
      (f64.const -0x1.9p+6 (;=-100;))))
  (func (;23;) (type 2) (result i32)
    (call 1
      (f64.sqrt
        (f64.const -0x1.9p+6 (;=-100;)))))
  (func (;24;) (type 5) (result f64)
    (f64.sqrt
      (f64.const 0x1.9p+6 (;=100;))))
  (func (;25;) (type 5) (result f64)
    (f64.ceil
      (f64.const -0x1.8p-1 (;=-0.75;))))
  (func (;26;) (type 5) (result f64)
    (f64.floor
      (f64.const -0x1.8p-1 (;=-0.75;))))
  (func (;27;) (type 5) (result f64)
    (f64.trunc
      (f64.const -0x1.8p-1 (;=-0.75;))))
  (func (;28;) (type 5) (result f64)
    (f64.nearest
      (f64.const 0x1.4p+0 (;=1.25;))))
  (func (;29;) (type 5) (result f64)
    (f64.nearest
      (f64.const 0x1.cp+0 (;=1.75;))))
  (export "i32_eqz_100" (func 2))
  (export "i32_eqz_0" (func 3))
  (export "i32_clz" (func 4))
  (export "i32_ctz" (func 5))
  (export "i32_popcnt" (func 6))
  (export "i64_eqz_100" (func 7))
  (export "i64_eqz_0" (func 8))
  (export "i64_clz" (func 9))
  (export "i64_ctz" (func 10))
  (export "i64_popcnt" (func 11))
  (export "f32_neg" (func 12))
  (export "f32_abs" (func 13))
  (export "f32_sqrt_neg_is_nan" (func 14))
  (export "f32_sqrt_100" (func 15))
  (export "f32_ceil" (func 16))
  (export "f32_floor" (func 17))
  (export "f32_trunc" (func 18))
  (export "f32_nearest_lo" (func 19))
  (export "f32_nearest_hi" (func 20))
  (export "f64_neg" (func 21))
  (export "f64_abs" (func 22))
  (export "f64_sqrt_neg_is_nan" (func 23))
  (export "f64_sqrt_100" (func 24))
  (export "f64_ceil" (func 25))
  (export "f64_floor" (func 26))
  (export "f64_trunc" (func 27))
  (export "f64_nearest_lo" (func 28))
  (export "f64_nearest_hi" (func 29)))
