(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func (result i32)))
  (type (;2;) (func (param i32 i32 i32) (result i32)))
  (type (;3;) (func))
  (type (;4;) (func (param i32 i32 i32)))
  (import "env" "io_get_stdout" (func $io_get_stdout (type 1)))
  (import "env" "resource_write" (func $resource_write (type 2)))
  (func $__wasm_call_ctors (type 3))
  (func $_start (type 3)
    (i32.store offset=65536
      (i32.const 0)
      (call 0)))
  (func $runtime.activateTask (type 4) (param i32 i32 i32)
    (local i32)
    (block  ;; label = @1
      (block  ;; label = @2
        (block  ;; label = @3
          (br_if 0 (;@3;)
            (i32.eqz
              (local.get 0)))
          (br_if 1 (;@2;)
            (i32.eqz
              (i32.load
                (local.get 0))))
          (br_if 2 (;@1;)
            (i32.eqz
              (local.tee 3
                (i32.load offset=65544
                  (i32.const 0)))))
          (i32.store offset=65544
            (i32.const 0)
            (local.get 0))
          (i32.store offset=8
            (local.get 3)
            (local.get 0)))
        (return))
      (call_indirect (type 0)
        (local.get 0)
        (i32.load offset=4
          (local.get 0)))
      (return))
    (i32.store offset=65540
      (i32.const 0)
      (local.get 0))
    (i32.store offset=65544
      (i32.const 0)
      (local.get 0)))
  (func $cwa_main (type 3)
    (local i32)
    (i32.store offset=65536
      (i32.const 0)
      (call 0))
    (local.set 0
      (i32.const -12))
    (block  ;; label = @1
      (loop  ;; label = @2
        (br_if 1 (;@1;)
          (i32.eqz
            (local.get 0)))
        (call 6
          (i32.load8_u
            (i32.add
              (local.get 0)
              (i32.const 65560))))
        (local.set 0
          (i32.add
            (local.get 0)
            (i32.const 1)))
        (br 0 (;@2;))))
    (call 6
      (i32.const 13))
    (call 6
      (i32.const 10)))
  (func $runtime.putchar (type 0) (param i32)
    (local i32)
    (global.set 0
      (local.tee 1
        (i32.sub
          (global.get 0)
          (i32.const 16))))
    (i32.store offset=12
      (local.get 1)
      (i32.const 0))
    (i32.store8 offset=12
      (local.get 1)
      (local.get 0))
    (drop
      (call 1
        (i32.load offset=65536
          (i32.const 0))
        (i32.add
          (local.get 1)
          (i32.const 12))
        (i32.const 1)))
    (global.set 0
      (i32.add
        (local.get 1)
        (i32.const 16))))
  (func $memset (type 2) (param i32 i32 i32) (result i32)
    (local i32 i32)
    (local.set 3
      (i32.const 0))
    (block  ;; label = @1
      (block  ;; label = @2
        (loop  ;; label = @3
          (br_if 1 (;@2;)
            (i32.ge_u
              (local.get 3)
              (local.get 2)))
          (br_if 2 (;@1;)
            (i32.eqz
              (local.tee 4
                (i32.add
                  (local.get 0)
                  (local.get 3)))))
          (i32.store8
            (local.get 4)
            (local.get 1))
          (local.set 3
            (i32.add
              (local.get 3)
              (i32.const 1)))
          (br 0 (;@3;))))
      (return
        (local.get 0)))
    (unreachable)
    (unreachable))
  (func $resume (type 3)
    (unreachable)
    (unreachable))
  (table (;0;) 1 1 anyfunc)
  (memory (;0;) 16)
  (global (;0;) (mut i32) (i32.const 65536))
  (global (;1;) i32 (i32.const 65560))
  (global (;2;) i32 (i32.const 65560))
  (global (;3;) i32 (i32.const 65536))
  (export "memory" (memory 0))
  (export "__heap_base" (global 1))
  (export "__data_end" (global 2))
  (export "__wasm_call_ctors" (func 2))
  (export "__dso_handle" (global 3))
  (export "_start" (func 3))
  (export "runtime.activateTask" (func 4))
  (export "cwa_main" (func 5))
  (export "memset" (func 7))
  (export "resume" (func 8))
  (data (;0;) (i32.const 65536) "\00\00\00\00\00\00\00\00\00\00\00\00")
  (data (;1;) (i32.const 65548) "Hello world!"))
