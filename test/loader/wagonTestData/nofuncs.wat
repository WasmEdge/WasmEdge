(module
  (import "ethereum" "finish" (func $finish (param i32 i32)))
  (memory 1)
  (export "memory" (memory 0))
    ;; no "main" export
)
