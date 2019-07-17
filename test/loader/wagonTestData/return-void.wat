(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func))
  (type (;2;) (func (result i32)))
  (func (;0;) (type 0) (param i32)
    (if  ;; label = @1
      (i32.eq
        (local.get 0)
        (i32.const 0))
      (then
        (return)))
    (i32.store
      (i32.const 0)
      (i32.const 1)))
  (func (;1;) (type 1)
    (call 0
      (i32.const 0)))
  (func (;2;) (type 2) (result i32)
    (i32.load
      (i32.const 0)))
  (func (;3;) (type 1)
    (call 0
      (i32.const 1)))
  (func (;4;) (type 2) (result i32)
    (i32.load
      (i32.const 0)))
  (memory (;0;) 1)
  (export "test1" (func 1))
  (export "check1" (func 2))
  (export "test2" (func 3))
  (export "check2" (func 4)))
