;; factorial_bench.wat
;; Recursive factorial with a main() that calls fac(10).
;;
;; Structural note for constant-folding analysis:
;;   Every arithmetic sub-expression in $fac involves local.get 0 (the
;;   parameter), so no const+const+binop triple ever appears in the body.
;;   main() contains only   i32.const 10 / call $fac  —  no binop follows
;;   the constant, so there are zero foldable patterns across the whole module.
;;
;; Expected optimizeConstantExpressions result:
;;   0 nops inserted, 0% instruction reduction.
;;   Factorial does NOT collapse to a single i32.const store for the same
;;   reason as fibonacci: the pass does not evaluate recursive calls.

(module
  (func $fac (export "fac") (param i32) (result i32)
    local.get 0
    i32.const 1
    i32.lt_s
    if (result i32)
      i32.const 1
    else
      local.get 0
      local.get 0
      i32.const 1
      i32.sub
      call $fac
      i32.mul
    end
  )

  ;; main: invoke fac with a single hardcoded constant argument.
  ;; fac(10) = 3628800.
  (func (export "main") (result i32)
    i32.const 10
    call $fac
  )
)
