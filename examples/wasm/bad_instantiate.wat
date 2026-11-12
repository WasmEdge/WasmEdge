(module
  (import "env" "memory" (memory 1))
  (import "env" "missing_func" (func $missing (param i32) (result i32)))
  (import "env" "table" (table 1 funcref))
  (import "env" "global" (global i32))
  (func (export "run") (result i32)
    i32.const 1
    call $missing
  )
)
