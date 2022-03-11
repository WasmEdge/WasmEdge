(module
  (type $t0 (func (param i32 i32)))
  (type $t1 (func (param f64 f64)))
  (type $t2 (func (param i32)))
  (type $t3 (func (param f64)))
  (import "module" "impl_printAdd" (func $impl_printAdd (type $t0)))
  (import "module" "impl_printDiv" (func $impl_printDiv (type $t1)))
  (import "host" "host_printI32" (func $host_printI32 (type $t2)))
  (import "host" "host_printF64" (func $host_printF64 (type $t3)))
  (func $printAdd (type $t0) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $impl_printAdd)
  (func $printDiv (type $t1) (param $p0 f64) (param $p1 f64)
    local.get $p0
    local.get $p1
    call $impl_printDiv)
  (func $printI32 (type $t2) (param $p0 i32)
    local.get $p0
    call $host_printI32)
  (func $printF64 (type $t3) (param $p0 f64)
    local.get $p0
    call $host_printF64)
  (export "printAdd" (func $printAdd))
  (export "printDiv" (func $printDiv))
  (export "printI32" (func $printI32))
  (export "printF64" (func $printF64)))
