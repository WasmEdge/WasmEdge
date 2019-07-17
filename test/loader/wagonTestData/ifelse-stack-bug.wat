(module
  (type (;0;) (func (result i32)))
  (func $main (type 0) (result i32)
    (local $l0 i32)
    (block  ;; label = @1
      (i32.const 1)
      (i32.const 2)
      (i32.const 3)
      (local.set 0
        (if (result i32)  ;; label = @2
          (i32.eq
            (i32.const 65)
            (i32.const 185533156))
          (then
            (i32.const 65))
          (else
            (i32.const 66))))
      (drop)
      (drop)
      (drop))
    (local.get 0))
  (export "main" (func 0)))
