(module
  (type $type0 (func (param i32)))
  (type $type1 (func (param f64)))
  (type (;2;) (func (param i32 i32)))
  (type (;3;) (func (param f64 f64)))
  (import "host" "host_printI32" (func $host_printI32 (type $type0)))
  (import "host" "host_printF64" (func $host_printF64 (type $type1)))
  (func (;2;) (type 2) (param i32 i32)
    local.get 0
    local.get 1
    i32.add
    call $host_printI32)
  (func (;3;) (type 3) (param f64 f64)
    local.get 0
    local.get 1
    f64.div
    call $host_printF64)
  (export "impl_printAdd" (func 2))
  (export "impl_printDiv" (func 3)))
