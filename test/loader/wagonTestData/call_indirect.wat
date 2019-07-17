(module
  ;; Auxiliary definitions
  (type $proc (func))
  (type $out-i32 (func (result i32)))
  (type $out-i64 (func (result i64)))
  (type $out-f32 (func (result f32)))
  (type $out-f64 (func (result f64)))
  (type $over-i32 (func (param i32) (result i32)))
  (type $over-i64 (func (param i64) (result i64)))
  (type $over-f32 (func (param f32) (result f32)))
  (type $over-f64 (func (param f64) (result f64)))
  (type $f32-i32 (func (param f32 i32) (result i32)))
  (type $i32-i64 (func (param i32 i64) (result i64)))
  (type $f64-f32 (func (param f64 f32) (result f32)))
  (type $i64-f64 (func (param i64 f64) (result f64)))
  (type $over-i32-duplicate (func (param i32) (result i32)))
  (type $over-i64-duplicate (func (param i64) (result i64)))
  (type $over-f32-duplicate (func (param f32) (result f32)))
  (type $over-f64-duplicate (func (param f64) (result f64)))

  (func $const-i32 (type $out-i32) (i32.const 0x132))
  (func $const-i64 (type $out-i64) (i64.const 0x164))
  (func $const-f32 (type $out-f32) (f32.const 0xf32))
  (func $const-f64 (type $out-f64) (f64.const 0xf64))

  (func $id-i32 (type $over-i32) (get_local 0))
  (func $id-i64 (type $over-i64) (get_local 0))
  (func $id-f32 (type $over-f32) (get_local 0))
  (func $id-f64 (type $over-f64) (get_local 0))

  (func $i32-i64 (type $i32-i64) (get_local 1))
  (func $i64-f64 (type $i64-f64) (get_local 1))
  (func $f32-i32 (type $f32-i32) (get_local 1))
  (func $f64-f32 (type $f64-f32) (get_local 1))

  (func $over-i32-duplicate (type $over-i32-duplicate) (get_local 0))
  (func $over-i64-duplicate (type $over-i64-duplicate) (get_local 0))
  (func $over-f32-duplicate (type $over-f32-duplicate) (get_local 0))
  (func $over-f64-duplicate (type $over-f64-duplicate) (get_local 0))

  (table anyfunc
    (elem
      $const-i32 $const-i64 $const-f32 $const-f64
      $id-i32 $id-i64 $id-f32 $id-f64
      $f32-i32 $i32-i64 $f64-f32 $i64-f64
      $fac $fib $even $odd
      $runaway $mutual-runaway1 $mutual-runaway2
      $over-i32-duplicate $over-i64-duplicate
      $over-f32-duplicate $over-f64-duplicate
    )
  )

  ;; Typing

  (func (export "type-i32") (result i32) (call_indirect $out-i32 (i32.const 0)))
  (func (export "type-i64") (result i64) (call_indirect $out-i64 (i32.const 1)))
  (func (export "type-f32") (result f32) (call_indirect $out-f32 (i32.const 2)))
  (func (export "type-f64") (result f64) (call_indirect $out-f64 (i32.const 3)))

  (func (export "type-index") (result i64)
    (call_indirect $over-i64 (i64.const 100) (i32.const 5))
  )

  (func (export "type-first-i32") (result i32)
    (call_indirect $over-i32 (i32.const 32) (i32.const 4))
  )
  (func (export "type-first-i64") (result i64)
    (call_indirect $over-i64 (i64.const 64) (i32.const 5))
  )
  (func (export "type-first-f32") (result f32)
    (call_indirect $over-f32 (f32.const 1.32) (i32.const 6))
  )
  (func (export "type-first-f64") (result f64)
    (call_indirect $over-f64 (f64.const 1.64) (i32.const 7))
  )

  (func (export "type-second-i32") (result i32)
    (call_indirect $f32-i32 (f32.const 32.1) (i32.const 32) (i32.const 8))
  )
  (func (export "type-second-i64") (result i64)
    (call_indirect $i32-i64 (i32.const 32) (i64.const 64) (i32.const 9))
  )
  (func (export "type-second-f32") (result f32)
    (call_indirect $f64-f32 (f64.const 64) (f32.const 32) (i32.const 10))
  )
  (func (export "type-second-f64") (result f64)
    (call_indirect $i64-f64 (i64.const 64) (f64.const 64.1) (i32.const 11))
  )

  ;; Dispatch

  (func (export "dispatch") (param i32 i64) (result i64)
    (call_indirect $over-i64 (get_local 1) (get_local 0))
  )

  (func (export "dispatch-structural") (param i32) (result i64)
    (call_indirect $over-i64-duplicate (i64.const 9) (get_local 0))
  )

  ;; Recursion

  (func $fac (export "fac") (type $over-i64)
    (if (result i64) (i64.eqz (get_local 0))
      (then (i64.const 1))
      (else
        (i64.mul
          (get_local 0)
          (call_indirect $over-i64
            (i64.sub (get_local 0) (i64.const 1))
            (i32.const 12)
          )
        )
      )
    )
  )

  (func $fib (export "fib") (type $over-i64)
    (if (result i64) (i64.le_u (get_local 0) (i64.const 1))
      (then (i64.const 1))
      (else
        (i64.add
          (call_indirect $over-i64
            (i64.sub (get_local 0) (i64.const 2))
            (i32.const 13)
          )
          (call_indirect $over-i64
            (i64.sub (get_local 0) (i64.const 1))
            (i32.const 13)
          )
        )
      )
    )
  )

  (func $even (export "even") (param i32) (result i32)
    (if (result i32) (i32.eqz (get_local 0))
      (then (i32.const 44))
      (else
        (call_indirect $over-i32
          (i32.sub (get_local 0) (i32.const 1))
          (i32.const 15)
        )
      )
    )
  )
  (func $odd (export "odd") (param i32) (result i32)
    (if (result i32) (i32.eqz (get_local 0))
      (then (i32.const 99))
      (else
        (call_indirect $over-i32
          (i32.sub (get_local 0) (i32.const 1))
          (i32.const 14)
        )
      )
    )
  )

  ;; Stack exhaustion

  ;; Implementations are required to have every call consume some abstract
  ;; resource towards exhausting some abstract finite limit, such that
  ;; infinitely recursive test cases reliably trap in finite time. This is
  ;; because otherwise applications could come to depend on it on those
  ;; implementations and be incompatible with implementations that don't do
  ;; it (or don't do it under the same circumstances).

  (func $runaway (export "runaway") (call_indirect $proc (i32.const 16)))

  (func $mutual-runaway1 (export "mutual-runaway") (call_indirect $proc (i32.const 18)))
  (func $mutual-runaway2 (call_indirect $proc (i32.const 17)))
)