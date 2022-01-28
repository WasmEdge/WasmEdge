(module
  (type $t1 (func (param externref i32 i32) (result i32)))
  (import "extern_module" "add" (func $add (type $t1)))
  (func $call_add (export "call_add") (type $t1) (param $p0 externref) (param $p1 i32) (param $p2 i32) (result i32)
    (call $add
      (local.get $p0)
      (local.get $p1)
      (local.get $p2)))
  (memory $memory (export "memory") 1))
