(module
  (type $t0 (func (param i32)))
  (type $t1 (func (param f64)))
  (type $t2 (func (param i32 i32)))
  (type $t3 (func (param f64 f64)))
  (import "host" "host_printI32" (func $host_printI32 (type $t0)))
  (import "host" "host_printF64" (func $host_printF64 (type $t1)))
  (func $impl_printAdd (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    i32.add
    call $host_printI32)
  (func $impl_printDiv (type $t3) (param $p0 f64) (param $p1 f64)
    local.get $p0
    local.get $p1
    f64.div
    call $host_printF64)
  (export "impl_printAdd" (func $impl_printAdd))
  (export "impl_printDiv" (func $impl_printDiv)))
