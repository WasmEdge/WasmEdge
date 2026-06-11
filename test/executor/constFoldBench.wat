(module
  ;; Benchmark module for constant folding optimization.
  ;; Contains many foldable const+const+binop patterns to measure the
  ;; impact of the pre-execution constant folding pass.

  ;; Function with 50 foldable i32 const-const-add patterns.
  ;; Without folding: 150 instructions (50 * 3).
  ;; With folding: 50 const + 100 nop = same count but fewer stack ops.
  (func $const_fold_i32_add (export "const_fold_i32_add") (result i32)
    (i32.add (i32.const 1) (i32.const 2))     ;; 3
    (i32.add (i32.const 3) (i32.const 4))     ;; 7
    (i32.add)                                   ;; 10
    (i32.add (i32.const 5) (i32.const 6))     ;; 11
    (i32.add)                                   ;; 21
    (i32.add (i32.const 7) (i32.const 8))     ;; 15
    (i32.add)                                   ;; 36
    (i32.add (i32.const 9) (i32.const 10))    ;; 19
    (i32.add)                                   ;; 55
    (i32.add (i32.const 11) (i32.const 12))   ;; 23
    (i32.add)                                   ;; 78
    (i32.add (i32.const 13) (i32.const 14))   ;; 27
    (i32.add)                                   ;; 105
    (i32.add (i32.const 15) (i32.const 16))   ;; 31
    (i32.add)                                   ;; 136
    (i32.add (i32.const 17) (i32.const 18))   ;; 35
    (i32.add)                                   ;; 171
    (i32.add (i32.const 19) (i32.const 20))   ;; 39
    (i32.add)                                   ;; 210
  )

  ;; Function with nested arithmetic chains that fold completely.
  ;; (((1 + 2) * 3) - 4) / 1 = 5
  (func $chain_fold (export "chain_fold") (result i32)
    (i32.div_u
      (i32.sub
        (i32.mul
          (i32.add (i32.const 1) (i32.const 2))
          (i32.const 3))
        (i32.const 4))
      (i32.const 1))
  )

  ;; Function mixing foldable and non-foldable code.
  ;; The const+const patterns fold, but local.get patterns don't.
  (func $mixed (export "mixed") (param $x i32) (result i32)
    (i32.add
      (i32.add
        (local.get $x)
        (i32.add (i32.const 10) (i32.const 20)))  ;; folds to 30
      (i32.mul (i32.const 3) (i32.const 4)))       ;; folds to 12
  )

  ;; Float constant folding benchmark.
  (func $float_fold (export "float_fold") (result f64)
    (f64.add
      (f64.mul (f64.const 3.14159) (f64.const 2.0))
      (f64.sqrt (f64.const 144.0)))
  )

  ;; Recursive fibonacci to test interaction with non-foldable code.
  ;; The (i32.const 2), (i32.lt_s) and (i32.const 1), (i32.sub) patterns
  ;; at the leaf are partially foldable with the identity optimization.
  (func $fib (export "fib") (param $n i32) (result i32)
    (if (result i32) (i32.lt_s (local.get $n) (i32.const 2))
      (then (i32.const 1))
      (else
        (i32.add
          (call $fib (i32.sub (local.get $n) (i32.const 2)))
          (call $fib (i32.sub (local.get $n) (i32.const 1)))
        )
      )
    )
  )

  ;; Type conversion chain that should fold completely.
  ;; i32.const 42 -> i64.extend_i32_s -> f64.convert_i64_s -> f32.demote_f64
  (func $convert_chain (export "convert_chain") (result f32)
    (f32.demote_f64
      (f64.convert_i64_s
        (i64.extend_i32_s
          (i32.const 42))))
  )

  ;; Comparison folding benchmark.
  (func $cmp_fold (export "cmp_fold") (result i32)
    (i32.and
      (i32.and
        (i32.eq (i32.const 5) (i32.const 5))      ;; 1
        (i32.lt_u (i32.const 3) (i32.const 7)))    ;; 1
      (i32.and
        (i32.gt_s (i32.const 10) (i32.const -1))   ;; 1
        (i32.ne (i32.const 0) (i32.const 1))))      ;; 1
  )

  ;; Identity elimination benchmark.
  ;; Each of these should have the identity const+binop eliminated.
  (func $identity_elim (export "identity_elim") (param $x i32) (result i32)
    (local.get $x)
    (i32.const 0) (i32.add)     ;; identity: x + 0 = x
    (i32.const 1) (i32.mul)     ;; identity: x * 1 = x
    (i32.const 0) (i32.or)      ;; identity: x | 0 = x
    (i32.const 0) (i32.xor)     ;; identity: x ^ 0 = x
    (i32.const 0) (i32.shl)     ;; identity: x << 0 = x
    (i32.const 0) (i32.shr_u)   ;; identity: x >> 0 = x
  )
)
