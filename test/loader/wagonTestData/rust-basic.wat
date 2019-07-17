(module
  (type (;0;) (func))
  (type (;1;) (func (param i64 i64) (result i64)))
  (type (;2;) (func (param i64 f64) (result f64)))
  (type (;3;) (func (param i64 f32) (result f32)))
  (func $__wasm_call_ctors (type 0))
  (func $x2_plus_y2_minus_13 (type 1) (param i64 i64) (result i64)
    (i64.add
      (i64.and
        (i64.add
          (i64.add
            (i64.mul
              (local.get 0)
              (local.get 0))
            (i64.mul
              (local.get 1)
              (local.get 1)))
          (i64.const 243))
        (i64.const 255))
      (i64.const 1)))
  (func $loopedArithmeticI64Benchmark (type 1) (param i64 i64) (result i64)
    (local i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64 i64)
    (local.set 2
      (i64.add
        (local.get 1)
        (i64.const 2)))
    (block  ;; label = @1
      (br_if 0 (;@1;)
        (i64.eqz
          (local.get 0)))
      (local.set 3
        (i64.sub
          (i64.const 0)
          (local.get 1)))
      (local.set 5
        (i64.add
          (local.tee 4
            (i64.and
              (i64.mul
                (i64.add
                  (local.get 1)
                  (i64.const 13))
                (local.get 1))
              (i64.const 26367)))
          (i64.const 1)))
      (local.set 6
        (i64.const -6))
      (local.set 7
        (i64.const -4))
      (local.set 8
        (i64.const -5))
      (local.set 9
        (i64.const 0))
      (local.set 10
        (i64.const 0))
      (local.set 11
        (i64.const 0))
      (local.set 12
        (i64.const 0))
      (local.set 13
        (i64.const 0))
      (loop  ;; label = @2
        (local.set 2
          (i64.add
            (i64.add
              (i64.add
                (i64.sub
                  (i64.add
                    (local.tee 14
                      (local.get 2))
                    (local.get 4))
                  (local.get 13))
                (i64.and
                  (local.tee 2
                    (i64.shl
                      (local.tee 15
                        (i64.div_u
                          (local.get 12)
                          (i64.const 3)))
                      (i64.const 1)))
                  (i64.const 8)))
              (local.tee 16
                (select
                  (i64.sub
                    (i64.const 1)
                    (local.tee 15
                      (i64.shl
                        (local.get 15)
                        (i64.const 2))))
                  (i64.const 0)
                  (i64.gt_u
                    (local.get 13)
                    (i64.const 5)))))
            (i64.mul
              (i64.add
                (i64.add
                  (i64.shl
                    (i64.add
                      (local.get 2)
                      (i64.div_u
                        (local.get 9)
                        (i64.const 3)))
                    (i64.const 3))
                  (i64.and
                    (i64.add
                      (local.get 10)
                      (local.get 15))
                    (i64.const -2)))
                (i64.shl
                  (i64.add
                    (local.get 2)
                    (i64.div_u
                      (local.get 11)
                      (i64.const 6)))
                  (i64.const 11)))
              (i64.const 3))))
        (local.set 9
          (i64.add
            (local.get 9)
            (i64.const 4)))
        (local.set 10
          (i64.add
            (local.get 10)
            (i64.const 5)))
        (local.set 11
          (i64.add
            (local.get 11)
            (i64.const 6)))
        (local.set 12
          (i64.add
            (local.get 12)
            (local.get 1)))
        (local.set 8
          (i64.add
            (local.get 8)
            (i64.const 5)))
        (local.set 7
          (i64.add
            (local.get 7)
            (i64.const 4)))
        (local.set 6
          (i64.add
            (local.get 6)
            (i64.const 6)))
        (local.set 3
          (i64.add
            (local.get 3)
            (local.get 1)))
        (local.set 5
          (i64.add
            (local.get 5)
            (i64.const -1)))
        (br_if 0 (;@2;)
          (i64.ne
            (local.get 0)
            (local.tee 13
              (i64.add
                (local.get 13)
                (i64.const 1))))))
      (local.set 2
        (i64.add
          (i64.add
            (local.get 16)
            (local.get 14))
          (i64.add
            (i64.add
              (local.get 5)
              (i64.shl
                (i64.and
                  (i64.div_u
                    (local.get 3)
                    (i64.const 12))
                  (i64.const 1))
                (i64.const 3)))
            (i64.mul
              (i64.add
                (i64.add
                  (i64.add
                    (i64.mul
                      (local.tee 13
                        (i64.div_u
                          (local.get 3)
                          (i64.const 3)))
                      (i64.const 4112))
                    (i64.shl
                      (i64.div_u
                        (local.get 6)
                        (i64.const 6))
                      (i64.const 11)))
                  (i64.shl
                    (i64.div_u
                      (local.get 7)
                      (i64.const 3))
                    (i64.const 3)))
                (i64.and
                  (i64.add
                    (local.get 8)
                    (i64.shl
                      (local.get 13)
                      (i64.const 2)))
                  (i64.const -2)))
              (i64.const 3))))))
    (local.get 2))
  (func $loopedArithmeticF64Benchmark (type 2) (param i64 f64) (result f64)
    (local f64 f64 f64 f64 f64 f64 f64 i64 f64 f64)
    (local.set 2
      (f64.add
        (local.get 1)
        (f64.const 0x1p+1 (;=2;))))
    (block  ;; label = @1
      (br_if 0 (;@1;)
        (i64.eqz
          (local.get 0)))
      (local.set 3
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.271a9fbe76c8bp+3 (;=9.222;)))
          (local.get 1)))
      (local.set 4
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.e70a3d70a3d71p+3 (;=15.22;)))
          (local.get 1)))
      (local.set 5
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.43851eb851eb8p+3 (;=10.11;)))
          (local.get 1)))
      (local.set 6
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.c3851eb851eb8p+3 (;=14.11;)))
          (local.get 1)))
      (local.set 7
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.6p+3 (;=11;)))
          (local.get 1)))
      (local.set 8
        (f64.mul
          (f64.add
            (local.get 1)
            (f64.const 0x1.ap+3 (;=13;)))
          (local.get 1)))
      (local.set 9
        (i64.const 0))
      (loop  ;; label = @2
        (local.set 2
          (select
            (select
              (f64.sub
                (local.tee 10
                  (f64.mul
                    (f64.add
                      (f64.mul
                        (local.tee 11
                          (f64.mul
                            (f64.div
                              (f64.mul
                                (local.tee 10
                                  (f64.convert_i64_u
                                    (local.get 9)))
                                (local.get 1))
                              (f64.const 0x1.8f7ced916872bp+1 (;=3.121;)))
                            (f64.const 0x1.00624dd2f1aap+1 (;=2.003;))))
                        (local.get 1))
                      (f64.sub
                        (f64.add
                          (f64.sub
                            (local.get 4)
                            (local.get 10))
                          (f64.sub
                            (f64.add
                              (f64.sub
                                (local.get 6)
                                (local.get 10))
                              (f64.sub
                                (f64.add
                                  (local.get 2)
                                  (f64.sub
                                    (local.get 8)
                                    (local.get 10)))
                                (f64.sub
                                  (local.get 7)
                                  (local.get 10))))
                            (f64.sub
                              (local.get 5)
                              (local.get 10))))
                        (f64.sub
                          (local.get 3)
                          (local.get 10))))
                    (f64.const 0x1.ffbe76c8b4396p+0 (;=1.999;))))
                (f64.add
                  (local.get 11)
                  (local.get 11)))
              (local.get 10)
              (f64.gt
                (local.get 11)
                (f64.const 0x0p+0 (;=0;))))
            (local.get 10)
            (i64.gt_u
              (local.get 9)
              (i64.const 5))))
        (br_if 0 (;@2;)
          (i64.ne
            (local.get 0)
            (local.tee 9
              (i64.add
                (local.get 9)
                (i64.const 1)))))))
    (local.get 2))
  (func $loopedArithmeticF32Benchmark (type 3) (param i64 f32) (result f32)
    (local f32 f32 f32 f32 f32 f32 f32 i64 f32 f32)
    (local.set 2
      (f32.add
        (local.get 1)
        (f32.const 0x1p+1 (;=2;))))
    (block  ;; label = @1
      (br_if 0 (;@1;)
        (i64.eqz
          (local.get 0)))
      (local.set 3
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.271aap+3 (;=9.222;)))
          (local.get 1)))
      (local.set 4
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.e70a3ep+3 (;=15.22;)))
          (local.get 1)))
      (local.set 5
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.43851ep+3 (;=10.11;)))
          (local.get 1)))
      (local.set 6
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.c3851ep+3 (;=14.11;)))
          (local.get 1)))
      (local.set 7
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.6p+3 (;=11;)))
          (local.get 1)))
      (local.set 8
        (f32.mul
          (f32.add
            (local.get 1)
            (f32.const 0x1.ap+3 (;=13;)))
          (local.get 1)))
      (local.set 9
        (i64.const 0))
      (loop  ;; label = @2
        (local.set 2
          (select
            (select
              (f32.sub
                (local.tee 10
                  (f32.mul
                    (f32.add
                      (f32.mul
                        (local.tee 11
                          (f32.mul
                            (f32.div
                              (f32.mul
                                (local.tee 10
                                  (f32.convert_i64_u
                                    (local.get 9)))
                                (local.get 1))
                              (f32.const 0x1.8f7ceep+1 (;=3.121;)))
                            (f32.const 0x1.00624ep+1 (;=2.003;))))
                        (local.get 1))
                      (f32.sub
                        (f32.add
                          (f32.sub
                            (local.get 4)
                            (local.get 10))
                          (f32.sub
                            (f32.add
                              (f32.sub
                                (local.get 6)
                                (local.get 10))
                              (f32.sub
                                (f32.add
                                  (local.get 2)
                                  (f32.sub
                                    (local.get 8)
                                    (local.get 10)))
                                (f32.sub
                                  (local.get 7)
                                  (local.get 10))))
                            (f32.sub
                              (local.get 5)
                              (local.get 10))))
                        (f32.sub
                          (local.get 3)
                          (local.get 10))))
                    (f32.const 0x1.ffbe76p+0 (;=1.999;))))
                (f32.add
                  (local.get 11)
                  (local.get 11)))
              (local.get 10)
              (f32.gt
                (local.get 11)
                (f32.const 0x0p+0 (;=0;))))
            (local.get 10)
            (i64.gt_u
              (local.get 9)
              (i64.const 5))))
        (br_if 0 (;@2;)
          (i64.ne
            (local.get 0)
            (local.tee 9
              (i64.add
                (local.get 9)
                (i64.const 1)))))))
    (local.get 2))
  (table (;0;) 1 1 anyfunc)
  (memory (;0;) 16)
  (global (;0;) (mut i32) (i32.const 1048576))
  (global (;1;) i32 (i32.const 1048576))
  (global (;2;) i32 (i32.const 1048576))
  (export "memory" (memory 0))
  (export "__indirect_function_table" (table 0))
  (export "__heap_base" (global 1))
  (export "__data_end" (global 2))
  (export "x2_plus_y2_minus_13" (func 1))
  (export "loopedArithmeticI64Benchmark" (func 2))
  (export "loopedArithmeticF64Benchmark" (func 3))
  (export "loopedArithmeticF32Benchmark" (func 4)))
