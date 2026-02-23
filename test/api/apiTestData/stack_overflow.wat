(module
  ;; Infinite recursion function
  (func $infinite_recursion (export "infinite_recursion") (result i32)
    (i32.add (i32.const 1) (call $infinite_recursion))
  )
  
  ;; Deep recursion function
  (func $deep_recursion (export "deep_recursion") (param $n i32) (result i32)
    (if (result i32)
      (i32.le_s (local.get $n) (i32.const 0))
      (then (i32.const 0))
      (else
        (i32.add
          (i32.const 1)
          (call $deep_recursion (i32.sub (local.get $n) (i32.const 1)))
        )
      )
    )
  )
)
