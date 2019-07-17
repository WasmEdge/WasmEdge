(module
  (type $t0 (func (result i32)))
  (type $t1 (func (param i32 i32) (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.const 42))
  (func $f1 (type $t1) (param $p0 i32) (param $p1 i32) (result i32)
    (i32.add
      (local.get $p0)
      (local.get $p1)))
  (func $h (export "h") (type $t0) (result i32)
    (call $f1
      (i32.const 1)
      (call $f0))))
