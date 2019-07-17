;; Test `get_local` operator

(module
  ;; Typing

  (func (export "type-local-i32") (result i32) (local i32) (get_local 0))
  (func (export "type-local-i64") (result i64) (local i64) (get_local 0))
  (func (export "type-local-f32") (result f32) (local f32) (get_local 0))
  (func (export "type-local-f64") (result f64) (local f64) (get_local 0))

  (func (export "type-param-i32") (param i32) (result i32) (get_local 0))
  (func (export "type-param-i64") (param i64) (result i64) (get_local 0))
  (func (export "type-param-f32") (param f32) (result f32) (get_local 0))
  (func (export "type-param-f64") (param f64) (result f64) (get_local 0))

  (func (export "type-mixed") (param i64 f32 f64 i32 i32)
    (local f32 i64 i64 f64)
    (drop (i64.eqz (get_local 0)))
    (drop (f32.neg (get_local 1)))
    (drop (f64.neg (get_local 2)))
    (drop (i32.eqz (get_local 3)))
    (drop (i32.eqz (get_local 4)))
    (drop (f32.neg (get_local 5)))
    (drop (i64.eqz (get_local 6)))
    (drop (i64.eqz (get_local 7)))
    (drop (f64.neg (get_local 8)))
  )

  ;; Reading

  (func (export "read") (param i64 f32 f64 i32 i32) (result f64)
    (local f32 i64 i64 f64)
    (set_local 5 (f32.const 5.5))
    (set_local 6 (i64.const 6))
    (set_local 8 (f64.const 8))
    (f64.add
      (f64.convert_u/i64 (get_local 0))
      (f64.add
        (f64.promote/f32 (get_local 1))
        (f64.add
          (get_local 2)
          (f64.add
            (f64.convert_u/i32 (get_local 3))
            (f64.add
              (f64.convert_s/i32 (get_local 4))
              (f64.add
                (f64.promote/f32 (get_local 5))
                (f64.add
                  (f64.convert_u/i64 (get_local 6))
                  (f64.add
                    (f64.convert_u/i64 (get_local 7))
                    (get_local 8)
                  )
                )
              )
            )
          )
        )
      )
    )
  )
)