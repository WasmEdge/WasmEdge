(module
  (type (;0;) (func))
  (type (;1;) (func (result i32)))
  (func (;0;) (type 0)
    (i32.store
      (i32.const 0)
      (i32.const 42)))
  (func (;1;) (type 1) (result i32)
    (i32.load
      (i32.const 0)))
  (memory (;0;) 1)
  (export "get" (func 1))
  (start 0))
