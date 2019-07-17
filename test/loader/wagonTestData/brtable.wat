(module
  (type $t0 (func (param i32) (result i32)))
  (type $t1 (func (result i32)))
  (func $f0 (type $t0) (param $p0 i32) (result i32)
    (block $B0
      (block $B1
        (block $B2
          (block $B3
            (br_table $B3 $B2 $B1 $B0
              (local.get $p0)))
          (return
            (i32.const 0)))
        (return
          (i32.const 1))))
    (return
      (i32.const 2)))
  (func $test0 (export "test0") (type $t1) (result i32)
    (call $f0
      (i32.const 0)))
  (func $test1 (export "test1") (type $t1) (result i32)
    (call $f0
      (i32.const 1)))
  (func $test2 (export "test2") (type $t1) (result i32)
    (call $f0
      (i32.const 2)))
  (func $test3 (export "test3") (type $t1) (result i32)
    (call $f0
      (i32.const 3))))
