(module
  (type (;0;) (func (result i32)))
  (type (;1;) (func (param i32 i64 f32 f64) (result i32)))
  (type (;2;) (func (param i32) (result i32)))
  (func (;0;) (type 0) (result i32)
    (call 1
      (i32.const 1)
      (i64.const 2)
      (f32.const 0x1.8p+1 (;=3;))
      (f64.const 0x1p+2 (;=4;))))
  (func (;1;) (type 1) (param i32 i64 f32 f64) (result i32)
    (return
      (i32.add
        (i32.add
          (i32.add
            (i32.wrap_i64
              (local.get 1))
            (local.get 0))
          (i32.trunc_f32_s
            (local.get 2)))
        (i32.trunc_f64_s
          (local.get 3)))))
  (func (;2;) (type 0) (result i32)
    (call 3
      (i32.const 10)))
  (func (;3;) (type 2) (param i32) (result i32)
    (if (result i32)  ;; label = @1
      (i32.gt_s
        (local.get 0)
        (i32.const 0))
      (then
        (return
          (i32.mul
            (local.get 0)
            (call 3
              (i32.sub
                (local.get 0)
                (i32.const 1))))))
      (else
        (return
          (i32.const 1)))))
  (export "call" (func 0))
  (export "fac10" (func 2)))
