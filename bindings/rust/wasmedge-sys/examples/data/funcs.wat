(module
  (type $t0 (func (param anyref i32 i32) (result i32)))
  (import "extern_module" "add" (func $extern_module.add (type $t0)))
  (func $call_add (type $t0) (param $p0 anyref) (param $p1 i32) (param $p2 i32) (result i32)
    local.get $p0
    local.get $p1
    local.get $p2
    call $extern_module.add)
  (memory $memory 1)
  (export "call_add" (func $call_add))
  (export "memory" (memory 0)))
