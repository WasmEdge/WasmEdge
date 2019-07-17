(module
  (type $t0 (func (param i32) (result i32)))
  (type $t1 (func (result i32)))
  (func $f0 (type $t0) (param $p0 i32) (result i32)
    (local $l1 i32)
    (loop $L0
      (local.set $l1
        (i32.add
          (local.get $l1)
          (i32.const 1)))
      (br_if $L0
        (i32.lt_s
          (local.get $l1)
          (local.get $p0))))
    (return
      (local.get $l1)))
  (func $test1 (export "test1") (type $t1) (result i32)
    (call $f0
      (i32.const 3)))
  (func $test2 (export "test2") (type $t1) (result i32)
    (call $f0
      (i32.const 10))))
