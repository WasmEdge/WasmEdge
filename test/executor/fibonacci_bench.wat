;; fibonacci_bench.wat
;; Recursive Fibonacci with a main() that calls fib(10).
;;
;; Structural note for constant-folding analysis:
;;   Every arithmetic sub-expression in $fib involves the parameter $n, so
;;   no const+const+binop triple ever appears in the function body.
;;   main() contains only   i32.const 10 / call $fib  —  no binop follows
;;   the constant, so there are zero foldable patterns across the whole module.
;;
;; Expected optimizeConstantExpressions result:
;;   0 nops inserted, 0% instruction reduction.
;;   Fibonacci does NOT collapse to a single i32.const store because the pass
;;   operates on instruction sequences only — it does not evaluate calls or
;;   perform inter-procedural analysis.

(module
  (func $fib (export "fib") (param $n i32) (result i32)
    local.get $n
    i32.const 2
    i32.lt_s
    if
      i32.const 1
      return
    end
    local.get $n
    i32.const 2
    i32.sub
    call $fib
    local.get $n
    i32.const 1
    i32.sub
    call $fib
    i32.add
    return
  )

  ;; main: invoke fib with a single hardcoded constant argument.
  ;; fib(10) = 89.
  (func (export "main") (result i32)
    i32.const 10
    call $fib
  )
)
