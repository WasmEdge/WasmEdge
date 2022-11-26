(module
  (type $type0 (func (result i32)))
  (type $type1 (func (param externref i32) (result i32)))
  (import "extern" "func-add" (func $e-f-add (type $type1)))
  (import "extern" "func-sub" (func $e-f-sub (type $type1)))
  (import "extern" "func-mul" (func $e-f-mul (type $type1)))
  (import "extern" "func-div" (func $e-f-div (type $type1)))
  (import "extern" "func-term" (func $e-f-term (type $type0)))
  (import "extern" "func-fail" (func $e-f-fail (type $type0)))
  (export "func-1" (func $f-1))
  (export "func-2" (func $f-2))
  (export "func-3" (func $f-3))
  (export "func-4" (func $f-4))
  (export "func-add" (func $f-add))
  (export "func-mul-2" (func $f-mul-2))
  (export "func-call-indirect" (func $f-call-ind))
  (export "func-host-add" (func $f-e-add))
  (export "func-host-sub" (func $f-e-sub))
  (export "func-host-mul" (func $f-e-mul))
  (export "func-host-div" (func $f-e-div))
  (export "tab-func" (table $t-f))
  (export "tab-ext" (table $t-e))
  (export "mem" (memory $m))
  (export "glob-mut-i32" (global $g-mi))
  (export "glob-const-f32" (global $g-cf))

  (func $f-1 (result i32) (i32.const 1))
  (func $f-2 (result i32) (i32.const 2))
  (func $f-3 (result i32) (i32.const 3))
  (func $f-4 (result i32) (i32.const 4))
  (func $f-add (param i32 i32) (result i32)
    (i32.add (local.get 0) (local.get 1))
  )
  (func $f-mul-2 (param i32 i32) (result i32 i32)
    (i32.mul (local.get 0) (i32.const 2))
    (i32.mul (local.get 1) (i32.const 2))
  )
  (func $f-call-ind (param i32) (result i32)
    ;; Call indirect to the index in table.
    (call_indirect $t-f (type $type0) (local.get 0))
  )
  (func $f-e-add (param i32) (result i32)
    ;; Call host function with 1st external reference in table.
    ;; Add the value by the argument.
    (call $e-f-add (table.get $t-e (i32.const 0)) (local.get 0))
  )
  (func $f-e-sub (param i32) (result i32)
    ;; Call host function with 2nd external reference in table.
    ;; Sub the value by the argument.
    (call $e-f-sub (table.get $t-e (i32.const 1)) (local.get 0))
  )
  (func $f-e-mul (param i32) (result i32)
    ;; Call host function with 3rd external reference in table.
    ;; Mul the value by the argument.
    (call $e-f-mul (table.get $t-e (i32.const 2)) (local.get 0))
  )
  (func $f-e-div (param i32) (result i32)
    ;; Call host function with 4th external reference in table.
    ;; Div the value by the argument.
    (call $e-f-div (table.get $t-e (i32.const 3)) (local.get 0))
  )

  (table $t-f 10 funcref)
  (elem (table $t-f) (i32.const 2) $f-1 $f-2 $f-3 $f-4)
  (table $t-e 10 externref)

  (memory $m 1 3)
  (data (i32.const 10) "\00\01\02\03\04\05\06\07\08\09")

  (global $g-mi (mut i32) (i32.const 142))
  (global $g-cf f32 (f32.const 789.12))
)
