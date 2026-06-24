(module
  (func $add (export "add") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add)
  (func $sub (export "sub") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.sub)
  (memory (export "memory") 1)
  (global $counter (export "counter") (mut i32) (i32.const 0))
)
