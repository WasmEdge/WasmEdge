;; Test globals

(module
  (global $a i32 (i32.const -2))
  (global (;1;) f32 (f32.const -3))
  (global (;2;) f64 (f64.const -4))
  (global $b i64 (i64.const -5))

  (global $x (mut i32) (i32.const -12))
  (global (;5;) (mut f32) (f32.const -13))
  (global (;6;) (mut f64) (f64.const -14))
  (global $y (mut i64) (i64.const -15))

  (func (export "get-a") (result i32) (get_global $a))
  (func (export "get-b") (result i64) (get_global $b))
  (func (export "get-x") (result i32) (get_global $x))
  (func (export "get-y") (result i64) (get_global $y))
  (func (export "set-x") (param i32) (set_global $x (get_local 0)))
  (func (export "set-y") (param i64) (set_global $y (get_local 0)))

  (func (export "get-1") (result f32) (get_global 1))
  (func (export "get-2") (result f64) (get_global 2))
  (func (export "get-5") (result f32) (get_global 5))
  (func (export "get-6") (result f64) (get_global 6))
  (func (export "set-5") (param f32) (set_global 5 (get_local 0)))
  (func (export "set-6") (param f64) (set_global 6 (get_local 0)))
)