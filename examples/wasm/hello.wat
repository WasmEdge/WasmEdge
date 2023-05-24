(module
  (type (;0;) (func))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32) (result i64)))
  (type (;3;) (func (param i32 i32) (result i32)))
  (type (;4;) (func (param i32 i32 i32 i32)))
  (type (;5;) (func (param i32) (result i32)))
  (type (;6;) (func (param i32 i32)))
  (type (;7;) (func (param i32 i32 i32)))
  (type (;8;) (func (param i32 i32 i32) (result i32)))
  (type (;9;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;10;) (func (result i32)))
  (type (;11;) (func (param i32 i32 i32 i32 i32)))
  (type (;12;) (func (param i32 i32 i32 i32 i32) (result i32)))
  (type (;13;) (func (param i32 i32 i32 i32 i32 i32) (result i32)))
  (type (;14;) (func (param i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $__wasi_proc_exit (type 1)))
  (import "wasi_snapshot_preview1" "args_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hc630ba694bd01843E (type 3)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h9fcbedb77e7846e5E (type 3)))
  (import "wasi_snapshot_preview1" "fd_write" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h93016769784eae7aE (type 9)))
  (import "wasi_snapshot_preview1" "environ_sizes_get" (func $__wasi_environ_sizes_get (type 3)))
  (import "wasi_snapshot_preview1" "environ_get" (func $__wasi_environ_get (type 3)))
  (import "wasi_snapshot_preview1" "fd_prestat_get" (func $__wasi_fd_prestat_get (type 3)))
  (import "wasi_snapshot_preview1" "fd_prestat_dir_name" (func $__wasi_fd_prestat_dir_name (type 8)))
  (func $__wasm_call_ctors (type 0)
    call $__wasilibc_initialize_environ_eagerly
    call $__wasilibc_populate_preopens)
  (func $_start (type 0)
    (local i32)
    call $__wasm_call_ctors
    call $__original_main
    local.set 0
    call $__wasm_call_dtors
    block  ;; label = @1
      local.get 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      call $__wasi_proc_exit
      unreachable
    end)
  (func $_ZN3std2rt10lang_start17h1d2f778293486ad9E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    i32.const 1048576
    local.set 6
    local.get 6
    local.set 7
    local.get 5
    local.set 8
    local.get 5
    local.get 0
    i32.store offset=4
    local.get 5
    local.get 1
    i32.store offset=8
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 5
    local.get 0
    i32.store
    local.get 8
    local.get 7
    local.get 1
    local.get 2
    call $_ZN3std2rt19lang_start_internal17h5691b2189b3dd330E
    local.set 9
    i32.const 16
    local.set 10
    local.get 5
    local.get 10
    i32.add
    local.set 11
    local.get 11
    global.set 0
    local.get 9
    return)
  (func $_ZN3std2rt10lang_start28_$u7b$$u7b$closure$u7d$$u7d$17h6c6012a6d56d2797E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 4
    local.get 4
    call $_ZN3std10sys_common9backtrace28__rust_begin_short_backtrace17h8b9d330c14a0408eE
    call $_ZN54_$LT$$LP$$RP$$u20$as$u20$std..process..Termination$GT$6report17h05aa4c18cd4f7de7E
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h65966affeede9f70E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=8
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 1
    call $_ZN4core3fmt9Formatter15debug_lower_hex17hfc16dde1881b9db0E
    local.set 5
    i32.const 1
    local.set 6
    local.get 5
    local.get 6
    i32.and
    local.set 7
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 7
          br_if 0 (;@3;)
          local.get 1
          call $_ZN4core3fmt9Formatter15debug_upper_hex17hf900d6ea61ff8af8E
          local.set 8
          br 1 (;@2;)
        end
        local.get 0
        local.get 1
        call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17haf413b6a293768dfE
        local.set 9
        i32.const 1
        local.set 10
        local.get 9
        local.get 10
        i32.and
        local.set 11
        local.get 4
        local.get 11
        i32.store8 offset=7
        br 1 (;@1;)
      end
      i32.const 1
      local.set 12
      local.get 8
      local.get 12
      i32.and
      local.set 13
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 13
            br_if 0 (;@4;)
            local.get 0
            local.get 1
            call $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17hc79ab98580260a0dE
            local.set 14
            i32.const 1
            local.set 15
            local.get 14
            local.get 15
            i32.and
            local.set 16
            local.get 4
            local.get 16
            i32.store8 offset=7
            br 1 (;@3;)
          end
          local.get 0
          local.get 1
          call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17h75226882cdaf372cE
          local.set 17
          i32.const 1
          local.set 18
          local.get 17
          local.get 18
          i32.and
          local.set 19
          local.get 4
          local.get 19
          i32.store8 offset=7
          br 1 (;@2;)
        end
      end
    end
    local.get 4
    i32.load8_u offset=7
    local.set 20
    i32.const 1
    local.set 21
    local.get 20
    local.get 21
    i32.and
    local.set 22
    i32.const 16
    local.set 23
    local.get 4
    local.get 23
    i32.add
    local.set 24
    local.get 24
    global.set 0
    local.get 22
    return)
  (func $_ZN138_$LT$$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$..drop..DropGuard$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17hc4f4aa5da569096fE (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 32
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=28
    local.get 0
    i32.load
    local.set 4
    local.get 4
    i32.load
    local.set 5
    local.get 5
    call $_ZN4core3ptr8non_null16NonNull$LT$T$GT$6as_ptr17h0c0253e3be467355E
    local.set 6
    local.get 0
    i32.load
    local.set 7
    local.get 7
    i32.load offset=4
    local.set 8
    i32.const 8
    local.set 9
    local.get 3
    local.get 9
    i32.add
    local.set 10
    local.get 10
    local.get 6
    local.get 8
    call $_ZN5alloc7raw_vec15RawVec$LT$T$GT$14from_raw_parts17h907692b3aa1c9dedE
    local.get 3
    i32.load offset=8
    local.set 11
    local.get 3
    i32.load offset=12
    local.set 12
    local.get 3
    local.get 12
    i32.store offset=20
    local.get 3
    local.get 11
    i32.store offset=16
    i32.const 16
    local.set 13
    local.get 3
    local.get 13
    i32.add
    local.set 14
    local.get 14
    local.set 15
    local.get 15
    call $_ZN4core3ptr13drop_in_place17ha95781944b4b9ad6E
    i32.const 32
    local.set 16
    local.get 3
    local.get 16
    i32.add
    local.set 17
    local.get 17
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17ha95781944b4b9ad6E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN77_$LT$alloc..raw_vec..RawVec$LT$T$C$A$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h7af780047a05ee8aE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17haa826d9e64a47c7dE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 4
    local.get 4
    call $_ZN4core3ops8function6FnOnce9call_once17ha10d14492884038dE
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core3ops8function6FnOnce9call_once17ha10d14492884038dE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 4
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    local.set 6
    local.get 3
    local.get 0
    i32.store offset=4
    local.get 6
    call $_ZN3std2rt10lang_start28_$u7b$$u7b$closure$u7d$$u7d$17h6c6012a6d56d2797E
    local.set 7
    i32.const 16
    local.set 8
    local.get 3
    local.get 8
    i32.add
    local.set 9
    local.get 9
    global.set 0
    local.get 7
    return)
  (func $_ZN4core3ops8function6FnOnce9call_once17hc675bf150a2f99f2E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call_indirect (type 0)
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h0da137339743d96cE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN77_$LT$alloc..raw_vec..RawVec$LT$T$C$A$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h6bf85996c39de954E
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN77_$LT$alloc..raw_vec..RawVec$LT$T$C$A$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h6bf85996c39de954E (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 48
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    local.set 6
    local.get 3
    local.get 0
    i32.store offset=32
    local.get 6
    local.get 0
    call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$14current_memory17h1a81cdfaa1287fc9E
    i32.const 1
    local.set 7
    i32.const 0
    local.set 8
    local.get 3
    i32.load offset=16
    local.set 9
    local.get 9
    local.set 10
    local.get 8
    local.set 11
    local.get 10
    local.get 11
    i32.eq
    local.set 12
    i32.const 1
    local.set 13
    local.get 12
    local.get 13
    i32.and
    local.set 14
    local.get 8
    local.get 7
    local.get 14
    select
    local.set 15
    local.get 15
    local.set 16
    local.get 7
    local.set 17
    local.get 16
    local.get 17
    i32.eq
    local.set 18
    i32.const 1
    local.set 19
    local.get 18
    local.get 19
    i32.and
    local.set 20
    block  ;; label = @1
      block  ;; label = @2
        local.get 20
        br_if 0 (;@2;)
        br 1 (;@1;)
      end
      local.get 3
      i32.load offset=16
      local.set 21
      local.get 3
      local.get 21
      i32.store offset=36
      local.get 3
      i32.load offset=20
      local.set 22
      local.get 3
      i32.load offset=24
      local.set 23
      local.get 3
      local.get 22
      i32.store offset=40
      local.get 3
      local.get 23
      i32.store offset=44
      local.get 0
      local.get 21
      local.get 22
      local.get 23
      call $_ZN62_$LT$alloc..alloc..Global$u20$as$u20$core..alloc..AllocRef$GT$7dealloc17h67699f5eb98fb89bE
    end
    i32.const 48
    local.set 24
    local.get 3
    local.get 24
    i32.add
    local.set 25
    local.get 25
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h20b6ea38ce87805aE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN66_$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h69c89e61e1eb8703E
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h0da137339743d96cE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN66_$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h69c89e61e1eb8703E (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN5alloc3vec12Vec$LT$T$GT$10as_mut_ptr17h978888bef43aeb69E
    local.set 4
    local.get 0
    i32.load offset=8
    local.set 5
    local.get 3
    local.get 4
    local.get 5
    call $_ZN4core3ptr24slice_from_raw_parts_mut17hea988fd44615b44eE
    local.get 3
    i32.load offset=4
    drop
    local.get 3
    i32.load
    drop
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h2d17a0ebbb3caf98E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h20b6ea38ce87805aE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h3b8543a26b9f9585E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h4d06fa96f73c7394E
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h4d06fa96f73c7394E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h7188c6664697d3fbE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h43051133a41f39d7E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h69dc1d4c7f680003E
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h69dc1d4c7f680003E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h20b6ea38ce87805aE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h7188c6664697d3fbE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN71_$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h92a6ae0680d6235dE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h5c043f6f1b239cedE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h3b8543a26b9f9585E
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN71_$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h92a6ae0680d6235dE (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 3
    i32.load offset=8
    local.set 4
    local.get 3
    local.get 4
    call $_ZN5alloc3vec17IntoIter$LT$T$GT$16as_raw_mut_slice17h79f3c86057cf2815E
    local.get 3
    i32.load offset=4
    local.set 5
    local.get 3
    i32.load
    local.set 6
    local.get 6
    local.get 5
    call $_ZN4core3ptr13drop_in_place17h71d32ad0b7806a7aE
    i32.const 8
    local.set 7
    local.get 3
    local.get 7
    i32.add
    local.set 8
    local.get 8
    local.set 9
    local.get 9
    call $_ZN4core3ptr13drop_in_place17hc95895954ff56602E
    i32.const 16
    local.set 10
    local.get 3
    local.get 10
    i32.add
    local.set 11
    local.get 11
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17h71d32ad0b7806a7aE (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 32
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 0
    local.set 5
    local.get 4
    local.get 0
    i32.store offset=24
    local.get 4
    local.get 1
    i32.store offset=28
    i32.const 1
    local.set 6
    local.get 5
    local.get 6
    i32.and
    local.set 7
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 7
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          local.set 8
          local.get 4
          local.get 8
          i32.store offset=12
          br 1 (;@2;)
        end
        local.get 4
        local.get 0
        i32.store offset=16
        local.get 4
        i32.load offset=16
        local.set 9
        i32.const 12
        local.set 10
        local.get 1
        local.get 10
        i32.mul
        local.set 11
        local.get 9
        local.get 11
        i32.add
        local.set 12
        local.get 4
        local.get 12
        i32.store offset=20
        loop  ;; label = @3
          local.get 4
          i32.load offset=16
          local.set 13
          local.get 4
          i32.load offset=20
          local.set 14
          local.get 13
          local.set 15
          local.get 14
          local.set 16
          local.get 15
          local.get 16
          i32.eq
          local.set 17
          i32.const 1
          local.set 18
          local.get 17
          local.get 18
          i32.and
          local.set 19
          local.get 19
          br_if 2 (;@1;)
          local.get 4
          i32.load offset=16
          local.set 20
          local.get 4
          i32.load offset=16
          local.set 21
          i32.const 12
          local.set 22
          local.get 21
          local.get 22
          i32.add
          local.set 23
          local.get 4
          local.get 23
          i32.store offset=16
          local.get 20
          call $_ZN4core3ptr13drop_in_place17h43051133a41f39d7E
          br 0 (;@3;)
        end
      end
      loop  ;; label = @2
        local.get 4
        i32.load offset=12
        local.set 24
        local.get 24
        local.set 25
        local.get 1
        local.set 26
        local.get 25
        local.get 26
        i32.eq
        local.set 27
        i32.const 1
        local.set 28
        local.get 27
        local.get 28
        i32.and
        local.set 29
        local.get 29
        br_if 1 (;@1;)
        local.get 4
        i32.load offset=12
        local.set 30
        i32.const 12
        local.set 31
        local.get 30
        local.get 31
        i32.mul
        local.set 32
        local.get 0
        local.get 32
        i32.add
        local.set 33
        local.get 4
        i32.load offset=12
        local.set 34
        i32.const 1
        local.set 35
        local.get 34
        local.get 35
        i32.add
        local.set 36
        local.get 4
        local.get 36
        i32.store offset=12
        local.get 33
        call $_ZN4core3ptr13drop_in_place17h43051133a41f39d7E
        br 0 (;@2;)
      end
    end
    i32.const 32
    local.set 37
    local.get 4
    local.get 37
    i32.add
    local.set 38
    local.get 38
    global.set 0
    return)
  (func $_ZN77_$LT$alloc..raw_vec..RawVec$LT$T$C$A$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17h7af780047a05ee8aE (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 48
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    local.set 6
    local.get 3
    local.get 0
    i32.store offset=32
    local.get 6
    local.get 0
    call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$14current_memory17h67739c458ad72fc4E
    i32.const 1
    local.set 7
    i32.const 0
    local.set 8
    local.get 3
    i32.load offset=16
    local.set 9
    local.get 9
    local.set 10
    local.get 8
    local.set 11
    local.get 10
    local.get 11
    i32.eq
    local.set 12
    i32.const 1
    local.set 13
    local.get 12
    local.get 13
    i32.and
    local.set 14
    local.get 8
    local.get 7
    local.get 14
    select
    local.set 15
    local.get 15
    local.set 16
    local.get 7
    local.set 17
    local.get 16
    local.get 17
    i32.eq
    local.set 18
    i32.const 1
    local.set 19
    local.get 18
    local.get 19
    i32.and
    local.set 20
    block  ;; label = @1
      block  ;; label = @2
        local.get 20
        br_if 0 (;@2;)
        br 1 (;@1;)
      end
      local.get 3
      i32.load offset=16
      local.set 21
      local.get 3
      local.get 21
      i32.store offset=36
      local.get 3
      i32.load offset=20
      local.set 22
      local.get 3
      i32.load offset=24
      local.set 23
      local.get 3
      local.get 22
      i32.store offset=40
      local.get 3
      local.get 23
      i32.store offset=44
      local.get 0
      local.get 21
      local.get 22
      local.get 23
      call $_ZN62_$LT$alloc..alloc..Global$u20$as$u20$core..alloc..AllocRef$GT$7dealloc17h67699f5eb98fb89bE
    end
    i32.const 48
    local.set 24
    local.get 3
    local.get 24
    i32.add
    local.set 25
    local.get 25
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17ha9806990c369dcecE (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    return)
  (func $_ZN4core3ptr13drop_in_place17hc95895954ff56602E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN138_$LT$$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$..drop..DropGuard$LT$T$GT$$u20$as$u20$core..ops..drop..Drop$GT$4drop17hc4f4aa5da569096fE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17hd5d199551d68d220E (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 0
    local.set 4
    i32.const 1
    local.set 5
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 6
    local.get 6
    local.set 7
    local.get 4
    local.set 8
    local.get 7
    local.get 8
    i32.eq
    local.set 9
    i32.const 1
    local.set 10
    local.get 9
    local.get 10
    i32.and
    local.set 11
    local.get 4
    local.get 5
    local.get 11
    select
    local.set 12
    block  ;; label = @1
      local.get 12
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      call $_ZN4core3ptr13drop_in_place17h2d17a0ebbb3caf98E
    end
    i32.const 16
    local.set 13
    local.get 3
    local.get 13
    i32.add
    local.set 14
    local.get 14
    global.set 0
    return)
  (func $_ZN4core3ptr13drop_in_place17hde99e66a4e1519b0E (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    return)
  (func $_ZN4core3ptr13drop_in_place17heff59f9aa6dd2fceE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr13drop_in_place17h5c043f6f1b239cedE
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3ptr7mut_ptr31_$LT$impl$u20$$BP$mut$u20$T$GT$13guaranteed_eq17hfcb1b4389d9c112eE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    local.get 0
    i32.store offset=4
    local.get 4
    local.get 1
    i32.store offset=8
    local.get 0
    local.set 5
    local.get 1
    local.set 6
    local.get 5
    local.get 6
    i32.eq
    local.set 7
    i32.const 1
    local.set 8
    local.get 7
    local.get 8
    i32.and
    local.set 9
    local.get 4
    local.get 9
    i32.store8 offset=15
    local.get 4
    i32.load8_u offset=15
    local.set 10
    i32.const 1
    local.set 11
    local.get 10
    local.get 11
    i32.and
    local.set 12
    local.get 12
    return)
  (func $_ZN4core3ptr7mut_ptr31_$LT$impl$u20$$BP$mut$u20$T$GT$7is_null17hc75b11a05a4cbe5fE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    i32.const 0
    local.set 4
    local.get 0
    local.get 4
    call $_ZN4core3ptr7mut_ptr31_$LT$impl$u20$$BP$mut$u20$T$GT$13guaranteed_eq17hfcb1b4389d9c112eE
    local.set 5
    i32.const 1
    local.set 6
    local.get 5
    local.get 6
    i32.and
    local.set 7
    i32.const 16
    local.set 8
    local.get 3
    local.get 8
    i32.add
    local.set 9
    local.get 9
    global.set 0
    local.get 7
    return)
  (func $_ZN54_$LT$$LP$$RP$$u20$as$u20$std..process..Termination$GT$6report17h05aa4c18cd4f7de7E (type 10) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 0
    i32.const 16
    local.set 1
    local.get 0
    local.get 1
    i32.sub
    local.set 2
    local.get 2
    global.set 0
    i32.const 0
    local.set 3
    i32.const 1
    local.set 4
    local.get 3
    local.get 4
    i32.and
    local.set 5
    local.get 5
    call $_ZN68_$LT$std..process..ExitCode$u20$as$u20$std..process..Termination$GT$6report17h784a835abce7cfa9E
    local.set 6
    i32.const 16
    local.set 7
    local.get 2
    local.get 7
    i32.add
    local.set 8
    local.get 8
    global.set 0
    local.get 6
    return)
  (func $_ZN68_$LT$std..process..ExitCode$u20$as$u20$std..process..Termination$GT$6report17h784a835abce7cfa9E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 15
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    local.set 6
    local.get 0
    local.set 7
    local.get 3
    local.get 7
    i32.store8 offset=15
    local.get 6
    call $_ZN3std3sys4wasi7process8ExitCode6as_i3217h9815b96bea97c486E
    local.set 8
    i32.const 16
    local.set 9
    local.get 3
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    local.get 8
    return)
  (func $_ZN4core5slice14from_raw_parts17h0c33318ff019e7acE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.get 1
    i32.store offset=8
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 5
    local.get 1
    local.get 2
    call $_ZN4core3ptr20slice_from_raw_parts17h98bb478024e2a1eaE
    local.get 5
    i32.load offset=4
    local.set 6
    local.get 5
    i32.load
    local.set 7
    local.get 0
    local.get 6
    i32.store offset=4
    local.get 0
    local.get 7
    i32.store
    i32.const 16
    local.set 8
    local.get 5
    local.get 8
    i32.add
    local.set 9
    local.get 9
    global.set 0
    return)
  (func $_ZN4core5alloc6layout6Layout25from_size_align_unchecked17hf46ed6158af6bb77E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.get 1
    i32.store offset=8
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 2
    call $_ZN4core3num12NonZeroUsize13new_unchecked17hbe19c77300c2b215E
    local.set 6
    local.get 5
    local.get 1
    i32.store
    local.get 5
    local.get 6
    i32.store offset=4
    local.get 5
    i32.load
    local.set 7
    local.get 5
    i32.load offset=4
    local.set 8
    local.get 0
    local.get 8
    i32.store offset=4
    local.get 0
    local.get 7
    i32.store
    i32.const 16
    local.set 9
    local.get 5
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    return)
  (func $_ZN4core5alloc6layout6Layout4size17hf787f3512737537dE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 4
    local.get 4
    return)
  (func $_ZN4core5alloc6layout6Layout5align17hc0df0453cc5fb668E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load offset=4
    local.set 4
    local.get 4
    call $_ZN4core3num12NonZeroUsize3get17h84944ea0d27d3156E
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core4iter6traits8iterator8Iterator3nth17h001a7959d1922ee4E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 64
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 5
    local.get 1
    i32.store offset=60
    block  ;; label = @1
      loop  ;; label = @2
        i32.const 16
        local.set 6
        local.get 5
        local.get 6
        i32.add
        local.set 7
        local.get 7
        local.set 8
        local.get 8
        local.get 1
        call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha4eaab787b46e43fE
        i32.const 1
        local.set 9
        i32.const 0
        local.set 10
        local.get 5
        i32.load offset=16
        local.set 11
        local.get 11
        local.set 12
        local.get 10
        local.set 13
        local.get 12
        local.get 13
        i32.eq
        local.set 14
        i32.const 1
        local.set 15
        local.get 14
        local.get 15
        i32.and
        local.set 16
        local.get 10
        local.get 9
        local.get 16
        select
        local.set 17
        local.get 17
        local.set 18
        local.get 9
        local.set 19
        local.get 18
        local.get 19
        i32.eq
        local.set 20
        i32.const 1
        local.set 21
        local.get 20
        local.get 21
        i32.and
        local.set 22
        block  ;; label = @3
          block  ;; label = @4
            local.get 22
            br_if 0 (;@4;)
            i32.const 16
            local.set 23
            local.get 5
            local.get 23
            i32.add
            local.set 24
            local.get 24
            local.set 25
            local.get 25
            call $_ZN4core3ptr13drop_in_place17hd5d199551d68d220E
            br 1 (;@3;)
          end
          i32.const 32
          local.set 26
          local.get 5
          local.get 26
          i32.add
          local.set 27
          local.get 27
          local.set 28
          i32.const 16
          local.set 29
          local.get 5
          local.get 29
          i32.add
          local.set 30
          local.get 30
          local.set 31
          local.get 31
          i64.load align=4
          local.set 32
          local.get 28
          local.get 32
          i64.store align=4
          i32.const 8
          local.set 33
          local.get 28
          local.get 33
          i32.add
          local.set 34
          local.get 31
          local.get 33
          i32.add
          local.set 35
          local.get 35
          i32.load
          local.set 36
          local.get 34
          local.get 36
          i32.store
          local.get 5
          i32.load offset=12
          local.set 37
          block  ;; label = @4
            block  ;; label = @5
              local.get 37
              i32.eqz
              br_if 0 (;@5;)
              i32.const 32
              local.set 38
              local.get 5
              local.get 38
              i32.add
              local.set 39
              local.get 39
              local.set 40
              local.get 5
              i32.load offset=12
              local.set 41
              i32.const 1
              local.set 42
              local.get 41
              local.get 42
              i32.sub
              local.set 43
              local.get 5
              local.get 43
              i32.store offset=12
              local.get 40
              call $_ZN4core3ptr13drop_in_place17h2d17a0ebbb3caf98E
              br 1 (;@4;)
            end
            i32.const 48
            local.set 44
            local.get 5
            local.get 44
            i32.add
            local.set 45
            local.get 45
            local.set 46
            i32.const 32
            local.set 47
            local.get 5
            local.get 47
            i32.add
            local.set 48
            local.get 48
            local.set 49
            local.get 49
            i64.load align=4
            local.set 50
            local.get 46
            local.get 50
            i64.store align=4
            i32.const 8
            local.set 51
            local.get 46
            local.get 51
            i32.add
            local.set 52
            local.get 49
            local.get 51
            i32.add
            local.set 53
            local.get 53
            i32.load
            local.set 54
            local.get 52
            local.get 54
            i32.store
            local.get 46
            i64.load align=4
            local.set 55
            local.get 0
            local.get 55
            i64.store align=4
            i32.const 8
            local.set 56
            local.get 0
            local.get 56
            i32.add
            local.set 57
            local.get 46
            local.get 56
            i32.add
            local.set 58
            local.get 58
            i32.load
            local.set 59
            local.get 57
            local.get 59
            i32.store
            br 3 (;@1;)
          end
          br 1 (;@2;)
        end
      end
      i32.const 0
      local.set 60
      local.get 0
      local.get 60
      i32.store
    end
    i32.const 64
    local.set 61
    local.get 5
    local.get 61
    i32.add
    local.set 62
    local.get 62
    global.set 0
    return)
  (func $_ZN4core4iter6traits8iterator8Iterator4skip17he89a4d456349ba6aE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i64 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    i32.const 8
    local.set 6
    local.get 5
    local.get 6
    i32.add
    local.set 7
    local.get 7
    local.set 8
    local.get 5
    local.get 2
    i32.store offset=28
    local.get 1
    i64.load align=4
    local.set 9
    local.get 8
    local.get 9
    i64.store align=4
    i32.const 8
    local.set 10
    local.get 8
    local.get 10
    i32.add
    local.set 11
    local.get 1
    local.get 10
    i32.add
    local.set 12
    local.get 12
    i64.load align=4
    local.set 13
    local.get 11
    local.get 13
    i64.store align=4
    local.get 0
    local.get 8
    local.get 2
    call $_ZN4core4iter8adapters13Skip$LT$I$GT$3new17h256f29acda141f5bE
    i32.const 32
    local.set 14
    local.get 5
    local.get 14
    i32.add
    local.set 15
    local.get 15
    global.set 0
    return)
  (func $_ZN5alloc7raw_vec15RawVec$LT$T$GT$14from_raw_parts17h907692b3aa1c9dedE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.get 1
    i32.store offset=24
    local.get 5
    local.get 2
    i32.store offset=28
    i32.const 8
    local.set 6
    local.get 5
    local.get 6
    i32.add
    local.set 7
    local.get 7
    local.get 1
    local.get 2
    call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$17from_raw_parts_in17hd299b411a6019680E
    local.get 5
    i32.load offset=12
    local.set 8
    local.get 5
    i32.load offset=8
    local.set 9
    local.get 0
    local.get 8
    i32.store offset=4
    local.get 0
    local.get 9
    i32.store
    i32.const 32
    local.set 10
    local.get 5
    local.get 10
    i32.add
    local.set 11
    local.get 11
    global.set 0
    return)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$17from_raw_parts_in17hd299b411a6019680E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 1
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$13new_unchecked17h528903e2d7896e5bE
    local.set 6
    local.get 5
    local.get 6
    i32.store offset=8
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 7
    local.get 5
    i32.load offset=12
    local.set 8
    local.get 0
    local.get 8
    i32.store offset=4
    local.get 0
    local.get 7
    i32.store
    i32.const 32
    local.set 9
    local.get 5
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    return)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$14current_memory17h1a81cdfaa1287fc9E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 64
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 1
    local.set 5
    local.get 4
    local.get 1
    i32.store offset=28
    local.get 4
    local.get 5
    i32.store offset=52
    local.get 4
    i32.load offset=52
    local.set 6
    block  ;; label = @1
      block  ;; label = @2
        local.get 6
        i32.eqz
        br_if 0 (;@2;)
        i32.const 0
        local.set 7
        i32.const 0
        local.set 8
        local.get 1
        i32.load offset=4
        local.set 9
        local.get 9
        local.set 10
        local.get 8
        local.set 11
        local.get 10
        local.get 11
        i32.eq
        local.set 12
        i32.const 1
        local.set 13
        local.get 12
        local.get 13
        i32.and
        local.set 14
        i32.const 1
        local.set 15
        local.get 7
        local.get 15
        i32.and
        local.set 16
        local.get 14
        local.get 16
        i32.ne
        local.set 17
        i32.const 1
        local.set 18
        local.get 17
        local.get 18
        i32.and
        local.set 19
        local.get 4
        local.get 19
        i32.store8 offset=15
        br 1 (;@1;)
      end
      i32.const 1
      local.set 20
      local.get 4
      local.get 20
      i32.store8 offset=15
    end
    local.get 4
    i32.load8_u offset=15
    local.set 21
    i32.const 1
    local.set 22
    local.get 21
    local.get 22
    i32.and
    local.set 23
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 23
          br_if 0 (;@3;)
          i32.const 1
          local.set 24
          local.get 4
          local.get 24
          i32.store offset=60
          local.get 4
          i32.load offset=60
          local.set 25
          local.get 4
          local.get 25
          i32.store offset=32
          br 1 (;@2;)
        end
        i32.const 0
        local.set 26
        local.get 0
        local.get 26
        i32.store
        br 1 (;@1;)
      end
      i32.const 1
      local.set 27
      local.get 4
      local.get 27
      i32.store offset=56
      local.get 4
      i32.load offset=56
      local.set 28
      local.get 1
      i32.load offset=4
      local.set 29
      local.get 28
      local.get 29
      i32.mul
      local.set 30
      local.get 4
      local.get 30
      i32.store offset=36
      local.get 4
      local.get 30
      local.get 25
      call $_ZN4core5alloc6layout6Layout25from_size_align_unchecked17hf46ed6158af6bb77E
      local.get 4
      i32.load offset=4
      local.set 31
      local.get 4
      i32.load
      local.set 32
      local.get 4
      local.get 32
      i32.store offset=40
      local.get 4
      local.get 31
      i32.store offset=44
      local.get 1
      i32.load
      local.set 33
      local.get 33
      call $_ZN4core3ptr6unique15Unique$LT$T$GT$4cast17h6644a7d6033a716cE
      local.set 34
      local.get 34
      call $_ZN50_$LT$T$u20$as$u20$core..convert..Into$LT$U$GT$$GT$4into17heeb61d91d3aa4fb3E
      local.set 35
      i32.const 16
      local.set 36
      local.get 4
      local.get 36
      i32.add
      local.set 37
      local.get 37
      local.set 38
      local.get 4
      local.get 35
      i32.store offset=16
      local.get 4
      local.get 32
      i32.store offset=20
      local.get 4
      local.get 31
      i32.store offset=24
      local.get 38
      i64.load align=4
      local.set 39
      local.get 0
      local.get 39
      i64.store align=4
      i32.const 8
      local.set 40
      local.get 0
      local.get 40
      i32.add
      local.set 41
      local.get 38
      local.get 40
      i32.add
      local.set 42
      local.get 42
      i32.load
      local.set 43
      local.get 41
      local.get 43
      i32.store
    end
    i32.const 64
    local.set 44
    local.get 4
    local.get 44
    i32.add
    local.set 45
    local.get 45
    global.set 0
    return)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$14current_memory17h67739c458ad72fc4E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 64
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 12
    local.set 5
    local.get 4
    local.get 1
    i32.store offset=28
    local.get 4
    local.get 5
    i32.store offset=52
    local.get 4
    i32.load offset=52
    local.set 6
    block  ;; label = @1
      block  ;; label = @2
        local.get 6
        i32.eqz
        br_if 0 (;@2;)
        i32.const 0
        local.set 7
        i32.const 0
        local.set 8
        local.get 1
        i32.load offset=4
        local.set 9
        local.get 9
        local.set 10
        local.get 8
        local.set 11
        local.get 10
        local.get 11
        i32.eq
        local.set 12
        i32.const 1
        local.set 13
        local.get 12
        local.get 13
        i32.and
        local.set 14
        i32.const 1
        local.set 15
        local.get 7
        local.get 15
        i32.and
        local.set 16
        local.get 14
        local.get 16
        i32.ne
        local.set 17
        i32.const 1
        local.set 18
        local.get 17
        local.get 18
        i32.and
        local.set 19
        local.get 4
        local.get 19
        i32.store8 offset=15
        br 1 (;@1;)
      end
      i32.const 1
      local.set 20
      local.get 4
      local.get 20
      i32.store8 offset=15
    end
    local.get 4
    i32.load8_u offset=15
    local.set 21
    i32.const 1
    local.set 22
    local.get 21
    local.get 22
    i32.and
    local.set 23
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 23
          br_if 0 (;@3;)
          i32.const 4
          local.set 24
          local.get 4
          local.get 24
          i32.store offset=60
          local.get 4
          i32.load offset=60
          local.set 25
          local.get 4
          local.get 25
          i32.store offset=32
          br 1 (;@2;)
        end
        i32.const 0
        local.set 26
        local.get 0
        local.get 26
        i32.store
        br 1 (;@1;)
      end
      i32.const 12
      local.set 27
      local.get 4
      local.get 27
      i32.store offset=56
      local.get 4
      i32.load offset=56
      local.set 28
      local.get 1
      i32.load offset=4
      local.set 29
      local.get 28
      local.get 29
      i32.mul
      local.set 30
      local.get 4
      local.get 30
      i32.store offset=36
      local.get 4
      local.get 30
      local.get 25
      call $_ZN4core5alloc6layout6Layout25from_size_align_unchecked17hf46ed6158af6bb77E
      local.get 4
      i32.load offset=4
      local.set 31
      local.get 4
      i32.load
      local.set 32
      local.get 4
      local.get 32
      i32.store offset=40
      local.get 4
      local.get 31
      i32.store offset=44
      local.get 1
      i32.load
      local.set 33
      local.get 33
      call $_ZN4core3ptr6unique15Unique$LT$T$GT$4cast17hde5788bb012b3cf6E
      local.set 34
      local.get 34
      call $_ZN50_$LT$T$u20$as$u20$core..convert..Into$LT$U$GT$$GT$4into17heeb61d91d3aa4fb3E
      local.set 35
      i32.const 16
      local.set 36
      local.get 4
      local.get 36
      i32.add
      local.set 37
      local.get 37
      local.set 38
      local.get 4
      local.get 35
      i32.store offset=16
      local.get 4
      local.get 32
      i32.store offset=20
      local.get 4
      local.get 31
      i32.store offset=24
      local.get 38
      i64.load align=4
      local.set 39
      local.get 0
      local.get 39
      i64.store align=4
      i32.const 8
      local.set 40
      local.get 0
      local.get 40
      i32.add
      local.set 41
      local.get 38
      local.get 40
      i32.add
      local.set 42
      local.get 42
      i32.load
      local.set 43
      local.get 41
      local.get 43
      i32.store
    end
    i32.const 64
    local.set 44
    local.get 4
    local.get 44
    i32.add
    local.set 45
    local.get 45
    global.set 0
    return)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$3ptr17hce4b631693c59b34E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 4
    local.get 4
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h098b653e22b5e8d2E
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN5hello4main17h1bdeb991053409b3E (type 0)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i64 i32 i32 i32 i32 i64 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 0
    i32.const 240
    local.set 1
    local.get 0
    local.get 1
    i32.sub
    local.set 2
    local.get 2
    global.set 0
    i32.const 8
    local.set 3
    local.get 2
    local.get 3
    i32.add
    local.set 4
    local.get 4
    local.set 5
    i32.const 1
    local.set 6
    i32.const 0
    local.set 7
    i32.const 0
    local.set 8
    local.get 8
    i32.load offset=1048616
    local.set 9
    i32.const 0
    local.set 10
    local.get 10
    i32.load offset=1048620
    local.set 11
    local.get 5
    local.get 9
    local.get 6
    local.get 11
    local.get 7
    call $_ZN4core3fmt9Arguments6new_v117h1235a630ff649770E
    i32.const 8
    local.set 12
    local.get 2
    local.get 12
    i32.add
    local.set 13
    local.get 13
    local.set 14
    local.get 14
    call $_ZN3std2io5stdio6_print17h5ab2b84f030f9b17E
    i32.const 80
    local.set 15
    local.get 2
    local.get 15
    i32.add
    local.set 16
    local.get 16
    local.set 17
    local.get 17
    call $_ZN3std3env4args17h6df17352f1454ba8E
    i32.const 56
    local.set 18
    local.get 2
    local.get 18
    i32.add
    local.set 19
    local.get 19
    local.set 20
    i32.const 80
    local.set 21
    local.get 2
    local.get 21
    i32.add
    local.set 22
    local.get 22
    local.set 23
    i32.const 1
    local.set 24
    local.get 20
    local.get 23
    local.get 24
    call $_ZN4core4iter6traits8iterator8Iterator4skip17he89a4d456349ba6aE
    i32.const 32
    local.set 25
    local.get 2
    local.get 25
    i32.add
    local.set 26
    local.get 26
    local.set 27
    i32.const 56
    local.set 28
    local.get 2
    local.get 28
    i32.add
    local.set 29
    local.get 29
    local.set 30
    local.get 27
    local.get 30
    call $_ZN63_$LT$I$u20$as$u20$core..iter..traits..collect..IntoIterator$GT$9into_iter17hcd6d727ae252bd75E
    i32.const 32
    local.set 31
    local.get 2
    local.get 31
    i32.add
    local.set 32
    local.get 32
    local.set 33
    i32.const 96
    local.set 34
    local.get 2
    local.get 34
    i32.add
    local.set 35
    local.get 35
    local.set 36
    local.get 33
    i64.load align=4
    local.set 37
    local.get 36
    local.get 37
    i64.store align=4
    i32.const 16
    local.set 38
    local.get 36
    local.get 38
    i32.add
    local.set 39
    local.get 33
    local.get 38
    i32.add
    local.set 40
    local.get 40
    i32.load
    local.set 41
    local.get 39
    local.get 41
    i32.store
    i32.const 8
    local.set 42
    local.get 36
    local.get 42
    i32.add
    local.set 43
    local.get 33
    local.get 42
    i32.add
    local.set 44
    local.get 44
    i64.load align=4
    local.set 45
    local.get 43
    local.get 45
    i64.store align=4
    loop  ;; label = @1
      i32.const 136
      local.set 46
      local.get 2
      local.get 46
      i32.add
      local.set 47
      local.get 47
      local.set 48
      i32.const 96
      local.set 49
      local.get 2
      local.get 49
      i32.add
      local.set 50
      local.get 50
      local.set 51
      local.get 48
      local.get 51
      call $_ZN94_$LT$core..iter..adapters..Skip$LT$I$GT$$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h4b71f81035100e86E
      local.get 2
      i32.load offset=136
      local.set 52
      i32.const 0
      local.set 53
      local.get 52
      local.get 53
      i32.ne
      local.set 54
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 54
            br_table 0 (;@4;) 1 (;@3;) 0 (;@4;)
          end
          i32.const 96
          local.set 55
          local.get 2
          local.get 55
          i32.add
          local.set 56
          local.get 56
          local.set 57
          local.get 57
          call $_ZN4core3ptr13drop_in_place17heff59f9aa6dd2fceE
          br 1 (;@2;)
        end
        i32.const 184
        local.set 58
        local.get 2
        local.get 58
        i32.add
        local.set 59
        local.get 59
        local.set 60
        i32.const 120
        local.set 61
        local.get 2
        local.get 61
        i32.add
        local.set 62
        local.get 62
        local.set 63
        i32.const 168
        local.set 64
        local.get 2
        local.get 64
        i32.add
        local.set 65
        local.get 65
        local.set 66
        i32.const 152
        local.set 67
        local.get 2
        local.get 67
        i32.add
        local.set 68
        local.get 68
        local.set 69
        i32.const 136
        local.set 70
        local.get 2
        local.get 70
        i32.add
        local.set 71
        local.get 71
        local.set 72
        local.get 72
        i64.load align=4
        local.set 73
        local.get 69
        local.get 73
        i64.store align=4
        i32.const 8
        local.set 74
        local.get 69
        local.get 74
        i32.add
        local.set 75
        local.get 72
        local.get 74
        i32.add
        local.set 76
        local.get 76
        i32.load
        local.set 77
        local.get 75
        local.get 77
        i32.store
        local.get 69
        i64.load align=4
        local.set 78
        local.get 66
        local.get 78
        i64.store align=4
        i32.const 8
        local.set 79
        local.get 66
        local.get 79
        i32.add
        local.set 80
        local.get 69
        local.get 79
        i32.add
        local.set 81
        local.get 81
        i32.load
        local.set 82
        local.get 80
        local.get 82
        i32.store
        local.get 66
        i64.load align=4
        local.set 83
        local.get 63
        local.get 83
        i64.store align=4
        i32.const 8
        local.set 84
        local.get 63
        local.get 84
        i32.add
        local.set 85
        local.get 66
        local.get 84
        i32.add
        local.set 86
        local.get 86
        i32.load
        local.set 87
        local.get 85
        local.get 87
        i32.store
        local.get 63
        i64.load align=4
        local.set 88
        local.get 60
        local.get 88
        i64.store align=4
        i32.const 8
        local.set 89
        local.get 60
        local.get 89
        i32.add
        local.set 90
        local.get 63
        local.get 89
        i32.add
        local.set 91
        local.get 91
        i32.load
        local.set 92
        local.get 90
        local.get 92
        i32.store
        i32.const 0
        local.set 93
        local.get 93
        i32.load offset=1048644
        local.set 94
        local.get 2
        local.get 60
        i32.store offset=232
        local.get 2
        i32.load offset=232
        local.set 95
        local.get 2
        local.get 95
        i32.store offset=236
        i32.const 4
        local.set 96
        local.get 2
        local.get 95
        local.get 96
        call $_ZN4core3fmt10ArgumentV13new17hf05c719d0e6f4c6dE
        local.get 2
        i32.load offset=4
        local.set 97
        local.get 2
        i32.load
        local.set 98
        i32.const 200
        local.set 99
        local.get 2
        local.get 99
        i32.add
        local.set 100
        local.get 100
        local.set 101
        i32.const 2
        local.set 102
        i32.const 1
        local.set 103
        i32.const 224
        local.set 104
        local.get 2
        local.get 104
        i32.add
        local.set 105
        local.get 105
        local.set 106
        local.get 2
        local.get 98
        i32.store offset=224
        local.get 2
        local.get 97
        i32.store offset=228
        local.get 101
        local.get 94
        local.get 102
        local.get 106
        local.get 103
        call $_ZN4core3fmt9Arguments6new_v117h1235a630ff649770E
        i32.const 200
        local.set 107
        local.get 2
        local.get 107
        i32.add
        local.set 108
        local.get 108
        local.set 109
        local.get 109
        call $_ZN3std2io5stdio6_print17h5ab2b84f030f9b17E
        i32.const 184
        local.set 110
        local.get 2
        local.get 110
        i32.add
        local.set 111
        local.get 111
        local.set 112
        local.get 112
        call $_ZN4core3ptr13drop_in_place17h2d17a0ebbb3caf98E
        br 1 (;@1;)
      end
    end
    i32.const 240
    local.set 113
    local.get 2
    local.get 113
    i32.add
    local.set 114
    local.get 114
    global.set 0
    return
    unreachable)
  (func $__original_main (type 10) (result i32)
    (local i32 i32 i32)
    i32.const 5
    local.set 0
    i32.const 0
    local.set 1
    local.get 0
    local.get 1
    local.get 1
    call $_ZN3std2rt10lang_start17h1d2f778293486ad9E
    local.set 2
    local.get 2
    return)
  (func $main (type 3) (param i32 i32) (result i32)
    (local i32)
    call $__original_main
    local.set 2
    local.get 2
    return)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17hdd737011a291d185E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=8
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 4
    local.get 0
    call $_ZN65_$LT$alloc..string..String$u20$as$u20$core..ops..deref..Deref$GT$5deref17h598d32bd3ec7e4d2E
    local.get 4
    i32.load offset=4
    local.set 5
    local.get 4
    i32.load
    local.set 6
    local.get 6
    local.get 5
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hce18dc51dfa14637E
    local.set 7
    i32.const 1
    local.set 8
    local.get 7
    local.get 8
    i32.and
    local.set 9
    i32.const 16
    local.set 10
    local.get 4
    local.get 10
    i32.add
    local.set 11
    local.get 11
    global.set 0
    local.get 9
    return)
  (func $_ZN65_$LT$alloc..string..String$u20$as$u20$core..ops..deref..Deref$GT$5deref17h598d32bd3ec7e4d2E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 32
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 1
    i32.store offset=28
    i32.const 16
    local.set 5
    local.get 4
    local.get 5
    i32.add
    local.set 6
    local.get 6
    local.get 1
    call $_ZN68_$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$core..ops..deref..Deref$GT$5deref17h246cead344bb2ee4E
    local.get 4
    i32.load offset=20
    local.set 7
    local.get 4
    i32.load offset=16
    local.set 8
    i32.const 8
    local.set 9
    local.get 4
    local.get 9
    i32.add
    local.set 10
    local.get 10
    local.get 8
    local.get 7
    call $_ZN4core3str19from_utf8_unchecked17h5f5d405909b090c1E
    local.get 4
    i32.load offset=12
    local.set 11
    local.get 4
    i32.load offset=8
    local.set 12
    local.get 0
    local.get 11
    i32.store offset=4
    local.get 0
    local.get 12
    i32.store
    i32.const 32
    local.set 13
    local.get 4
    local.get 13
    i32.add
    local.set 14
    local.get 14
    global.set 0
    return)
  (func $_ZN3std10sys_common9backtrace28__rust_begin_short_backtrace17h8b9d330c14a0408eE (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 32
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=28
    local.get 0
    call $_ZN4core3ops8function6FnOnce9call_once17hc675bf150a2f99f2E
    call $_ZN4core4hint9black_box17hf2af035d523b2216E
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    global.set 0
    return)
  (func $_ZN4core3cmp5impls56_$LT$impl$u20$core..cmp..PartialEq$u20$for$u20$usize$GT$2eq17hb4950072e9d4ca56E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    local.get 0
    i32.store offset=8
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 0
    i32.load
    local.set 5
    local.get 1
    i32.load
    local.set 6
    local.get 5
    local.set 7
    local.get 6
    local.set 8
    local.get 7
    local.get 8
    i32.eq
    local.set 9
    i32.const 1
    local.set 10
    local.get 9
    local.get 10
    i32.and
    local.set 11
    local.get 11
    return)
  (func $_ZN4core4hint9black_box17hf2af035d523b2216E (type 0)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 0
    i32.const 16
    local.set 1
    local.get 0
    local.get 1
    i32.sub
    local.set 2
    i32.const 8
    local.set 3
    local.get 2
    local.get 3
    i32.add
    local.set 4
    local.get 4
    local.set 5
    return)
  (func $_ZN5alloc5alloc7dealloc17h6e6063d0c15c617cE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    global.set 0
    local.get 5
    local.set 6
    local.get 5
    local.get 1
    i32.store
    local.get 5
    local.get 2
    i32.store offset=4
    local.get 5
    local.get 0
    i32.store offset=12
    local.get 6
    call $_ZN4core5alloc6layout6Layout4size17hf787f3512737537dE
    local.set 7
    local.get 5
    local.set 8
    local.get 8
    call $_ZN4core5alloc6layout6Layout5align17hc0df0453cc5fb668E
    local.set 9
    local.get 0
    local.get 7
    local.get 9
    call $__rust_dealloc
    i32.const 16
    local.set 10
    local.get 5
    local.get 10
    i32.add
    local.set 11
    local.get 11
    global.set 0
    return)
  (func $_ZN62_$LT$alloc..alloc..Global$u20$as$u20$core..alloc..AllocRef$GT$7dealloc17h67699f5eb98fb89bE (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 4
    i32.const 32
    local.set 5
    local.get 4
    local.get 5
    i32.sub
    local.set 6
    local.get 6
    global.set 0
    i32.const 8
    local.set 7
    local.get 6
    local.get 7
    i32.add
    local.set 8
    local.get 8
    local.set 9
    local.get 6
    local.get 2
    i32.store offset=8
    local.get 6
    local.get 3
    i32.store offset=12
    local.get 6
    local.get 0
    i32.store offset=24
    local.get 6
    local.get 1
    i32.store offset=28
    local.get 9
    call $_ZN4core5alloc6layout6Layout4size17hf787f3512737537dE
    local.set 10
    block  ;; label = @1
      block  ;; label = @2
        local.get 10
        br_if 0 (;@2;)
        br 1 (;@1;)
      end
      local.get 1
      call $_ZN4core3ptr8non_null16NonNull$LT$T$GT$6as_ptr17h3ea2ddfe64e7c034E
      local.set 11
      local.get 6
      i32.load offset=8
      local.set 12
      local.get 6
      i32.load offset=12
      local.set 13
      local.get 11
      local.get 12
      local.get 13
      call $_ZN5alloc5alloc7dealloc17h6e6063d0c15c617cE
    end
    i32.const 32
    local.set 14
    local.get 6
    local.get 14
    i32.add
    local.set 15
    local.get 15
    global.set 0
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$13new_unchecked17h528903e2d7896e5bE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=4
    local.get 3
    i32.load offset=4
    local.set 4
    local.get 4
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$13new_unchecked17hf08b1dbbd9c77dc3E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=4
    local.get 3
    i32.load offset=4
    local.set 4
    local.get 4
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$4cast17h6644a7d6033a716cE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h098b653e22b5e8d2E
    local.set 4
    local.get 4
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$13new_unchecked17hf08b1dbbd9c77dc3E
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h098b653e22b5e8d2E (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$4cast17hde5788bb012b3cf6E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h10868c5030051137E
    local.set 4
    local.get 4
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$13new_unchecked17hf08b1dbbd9c77dc3E
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h10868c5030051137E (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    return)
  (func $_ZN50_$LT$T$u20$as$u20$core..convert..Into$LT$U$GT$$GT$4into17heeb61d91d3aa4fb3E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN119_$LT$core..ptr..non_null..NonNull$LT$T$GT$$u20$as$u20$core..convert..From$LT$core..ptr..unique..Unique$LT$T$GT$$GT$$GT$4from17hc24768b51add895dE
    local.set 4
    i32.const 16
    local.set 5
    local.get 3
    local.get 5
    i32.add
    local.set 6
    local.get 6
    global.set 0
    local.get 4
    return)
  (func $_ZN119_$LT$core..ptr..non_null..NonNull$LT$T$GT$$u20$as$u20$core..convert..From$LT$core..ptr..unique..Unique$LT$T$GT$$GT$$GT$4from17hc24768b51add895dE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    call $_ZN4core3ptr6unique15Unique$LT$T$GT$6as_ptr17h098b653e22b5e8d2E
    local.set 4
    local.get 4
    call $_ZN4core3ptr8non_null16NonNull$LT$T$GT$13new_unchecked17h0d604d602b8d96efE
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN4core3ptr8non_null16NonNull$LT$T$GT$13new_unchecked17h0d604d602b8d96efE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 3
    i32.load offset=8
    local.set 4
    local.get 4
    return)
  (func $_ZN4core3ptr8non_null16NonNull$LT$T$GT$6as_ptr17h0c0253e3be467355E (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    return)
  (func $_ZN4core3ptr8non_null16NonNull$LT$T$GT$6as_ptr17h3ea2ddfe64e7c034E (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    return)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h858d2d50d0fa93e7E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=8
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 0
    i32.load
    local.set 5
    local.get 5
    local.get 1
    call $_ZN66_$LT$core..option..Option$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h6114c7767e315ae5E
    local.set 6
    i32.const 1
    local.set 7
    local.get 6
    local.get 7
    i32.and
    local.set 8
    i32.const 16
    local.set 9
    local.get 4
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    local.get 8
    return)
  (func $_ZN66_$LT$core..option..Option$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h6114c7767e315ae5E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 64
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=52
    local.get 4
    local.get 1
    i32.store offset=56
    local.get 4
    local.get 0
    i32.store offset=12
    local.get 4
    i32.load offset=12
    local.set 5
    local.get 5
    i32.load
    local.set 6
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 6
            br_table 1 (;@3;) 0 (;@4;) 1 (;@3;)
          end
          i32.const 32
          local.set 7
          local.get 4
          local.get 7
          i32.add
          local.set 8
          local.get 8
          local.set 9
          i32.const 1048648
          local.set 10
          local.get 10
          local.set 11
          i32.const 4
          local.set 12
          local.get 4
          i32.load offset=12
          local.set 13
          i32.const 4
          local.set 14
          local.get 13
          local.get 14
          i32.add
          local.set 15
          local.get 4
          local.get 15
          i32.store offset=60
          local.get 9
          local.get 1
          local.get 11
          local.get 12
          call $_ZN4core3fmt9Formatter11debug_tuple17h687ff740dc6e8836E
          br 1 (;@2;)
        end
        i32.const 16
        local.set 16
        local.get 4
        local.get 16
        i32.add
        local.set 17
        local.get 17
        local.set 18
        i32.const 1048668
        local.set 19
        local.get 19
        local.set 20
        i32.const 4
        local.set 21
        local.get 18
        local.get 1
        local.get 20
        local.get 21
        call $_ZN4core3fmt9Formatter11debug_tuple17h687ff740dc6e8836E
        i32.const 16
        local.set 22
        local.get 4
        local.get 22
        i32.add
        local.set 23
        local.get 23
        local.set 24
        local.get 24
        call $_ZN4core3fmt8builders10DebugTuple6finish17hcd73b6e7d638f97cE
        local.set 25
        i32.const 1
        local.set 26
        local.get 25
        local.get 26
        i32.and
        local.set 27
        local.get 4
        local.get 27
        i32.store8 offset=11
        br 1 (;@1;)
      end
      i32.const 32
      local.set 28
      local.get 4
      local.get 28
      i32.add
      local.set 29
      local.get 29
      local.set 30
      i32.const 1048652
      local.set 31
      local.get 31
      local.set 32
      i32.const 48
      local.set 33
      local.get 4
      local.get 33
      i32.add
      local.set 34
      local.get 34
      local.set 35
      local.get 4
      local.get 15
      i32.store offset=48
      local.get 30
      local.get 35
      local.get 32
      call $_ZN4core3fmt8builders10DebugTuple5field17h0e16581ce3858b5aE
      drop
      i32.const 32
      local.set 36
      local.get 4
      local.get 36
      i32.add
      local.set 37
      local.get 37
      local.set 38
      local.get 38
      call $_ZN4core3fmt8builders10DebugTuple6finish17hcd73b6e7d638f97cE
      local.set 39
      i32.const 1
      local.set 40
      local.get 39
      local.get 40
      i32.and
      local.set 41
      local.get 4
      local.get 41
      i32.store8 offset=11
    end
    local.get 4
    i32.load8_u offset=11
    local.set 42
    i32.const 1
    local.set 43
    local.get 42
    local.get 43
    i32.and
    local.set 44
    i32.const 64
    local.set 45
    local.get 4
    local.get 45
    i32.add
    local.set 46
    local.get 46
    global.set 0
    local.get 44
    return
    unreachable)
  (func $_ZN70_$LT$core..option..Option$LT$T$GT$$u20$as$u20$core..cmp..PartialEq$GT$2eq17hbc95401444e93099E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 48
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=16
    local.get 4
    local.get 1
    i32.store offset=20
    local.get 0
    i32.load
    local.set 5
    local.get 4
    local.get 5
    i32.store offset=24
    local.get 4
    i32.load offset=24
    local.set 6
    local.get 4
    local.get 6
    i32.store offset=28
    local.get 1
    i32.load
    local.set 7
    local.get 4
    local.get 7
    i32.store offset=32
    local.get 4
    i32.load offset=32
    local.set 8
    local.get 4
    local.get 8
    i32.store offset=36
    i32.const 0
    local.set 9
    local.get 6
    local.set 10
    local.get 8
    local.set 11
    local.get 10
    local.get 11
    i32.eq
    local.set 12
    i32.const 1
    local.set 13
    local.get 12
    local.get 13
    i32.and
    local.set 14
    i32.const 1
    local.set 15
    local.get 9
    local.get 15
    i32.and
    local.set 16
    local.get 14
    local.get 16
    i32.ne
    local.set 17
    i32.const 1
    local.set 18
    local.get 17
    local.get 18
    i32.and
    local.set 19
    block  ;; label = @1
      block  ;; label = @2
        local.get 19
        br_if 0 (;@2;)
        i32.const 0
        local.set 20
        local.get 4
        local.get 20
        i32.store8 offset=7
        br 1 (;@1;)
      end
      i32.const 1
      local.set 21
      local.get 4
      local.get 0
      i32.store offset=8
      local.get 4
      local.get 1
      i32.store offset=12
      local.get 4
      i32.load offset=8
      local.set 22
      local.get 22
      i32.load
      local.set 23
      local.get 23
      local.set 24
      local.get 21
      local.set 25
      local.get 24
      local.get 25
      i32.eq
      local.set 26
      i32.const 1
      local.set 27
      local.get 26
      local.get 27
      i32.and
      local.set 28
      block  ;; label = @2
        block  ;; label = @3
          local.get 28
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1
          local.set 29
          local.get 4
          i32.load offset=12
          local.set 30
          local.get 30
          i32.load
          local.set 31
          local.get 31
          local.set 32
          local.get 29
          local.set 33
          local.get 32
          local.get 33
          i32.eq
          local.set 34
          i32.const 1
          local.set 35
          local.get 34
          local.get 35
          i32.and
          local.set 36
          local.get 36
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          i32.load offset=8
          local.set 37
          i32.const 4
          local.set 38
          local.get 37
          local.get 38
          i32.add
          local.set 39
          local.get 4
          local.get 39
          i32.store offset=40
          local.get 4
          i32.load offset=12
          local.set 40
          i32.const 4
          local.set 41
          local.get 40
          local.get 41
          i32.add
          local.set 42
          local.get 4
          local.get 42
          i32.store offset=44
          local.get 39
          local.get 42
          call $_ZN4core3cmp5impls56_$LT$impl$u20$core..cmp..PartialEq$u20$for$u20$usize$GT$2eq17hb4950072e9d4ca56E
          local.set 43
          i32.const 1
          local.set 44
          local.get 43
          local.get 44
          i32.and
          local.set 45
          local.get 4
          local.get 45
          i32.store8 offset=7
          br 1 (;@2;)
        end
        i32.const 1
        local.set 46
        local.get 4
        local.get 46
        i32.store8 offset=7
      end
    end
    local.get 4
    i32.load8_u offset=7
    local.set 47
    i32.const 1
    local.set 48
    local.get 47
    local.get 48
    i32.and
    local.set 49
    i32.const 48
    local.set 50
    local.get 4
    local.get 50
    i32.add
    local.set 51
    local.get 51
    global.set 0
    local.get 49
    return)
  (func $_ZN4core3num12NonZeroUsize13new_unchecked17hbe19c77300c2b215E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 3
    i32.load offset=8
    local.set 4
    local.get 4
    return)
  (func $_ZN4core3num12NonZeroUsize3get17h84944ea0d27d3156E (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    return)
  (func $_ZN4core3num23_$LT$impl$u20$usize$GT$12wrapping_sub17h9fa155a4799bc4beE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    local.get 0
    i32.store offset=4
    local.get 4
    local.get 1
    i32.store offset=8
    local.get 0
    local.get 1
    i32.sub
    local.set 5
    local.get 4
    local.get 5
    i32.store offset=12
    local.get 4
    i32.load offset=12
    local.set 6
    local.get 6
    return)
  (func $_ZN4core3ptr20slice_from_raw_parts17h98bb478024e2a1eaE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store offset=24
    local.get 5
    local.get 2
    i32.store offset=28
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 5
    i32.load offset=16
    local.set 6
    local.get 5
    i32.load offset=20
    local.set 7
    local.get 5
    local.get 6
    i32.store offset=8
    local.get 5
    local.get 7
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 8
    local.get 5
    i32.load offset=12
    local.set 9
    local.get 0
    local.get 9
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    return)
  (func $_ZN4core3ptr24slice_from_raw_parts_mut17h64a6a973012e64aaE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store offset=24
    local.get 5
    local.get 2
    i32.store offset=28
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 5
    i32.load offset=16
    local.set 6
    local.get 5
    i32.load offset=20
    local.set 7
    local.get 5
    local.get 6
    i32.store offset=8
    local.get 5
    local.get 7
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 8
    local.get 5
    i32.load offset=12
    local.set 9
    local.get 0
    local.get 9
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    return)
  (func $_ZN4core3ptr24slice_from_raw_parts_mut17hea988fd44615b44eE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store offset=24
    local.get 5
    local.get 2
    i32.store offset=28
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 5
    i32.load offset=16
    local.set 6
    local.get 5
    i32.load offset=20
    local.set 7
    local.get 5
    local.get 6
    i32.store offset=8
    local.get 5
    local.get 7
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 8
    local.get 5
    i32.load offset=12
    local.set 9
    local.get 0
    local.get 9
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    return)
  (func $_ZN4core3fmt9Arguments6new_v117h1235a630ff649770E (type 11) (param i32 i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 5
    i32.const 32
    local.set 6
    local.get 5
    local.get 6
    i32.sub
    local.set 7
    i32.const 0
    local.set 8
    local.get 7
    local.get 1
    i32.store offset=16
    local.get 7
    local.get 2
    i32.store offset=20
    local.get 7
    local.get 3
    i32.store offset=24
    local.get 7
    local.get 4
    i32.store offset=28
    local.get 7
    local.get 8
    i32.store offset=8
    local.get 0
    local.get 1
    i32.store
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 7
    i32.load offset=8
    local.set 9
    local.get 7
    i32.load offset=12
    local.set 10
    local.get 0
    local.get 9
    i32.store offset=8
    local.get 0
    local.get 10
    i32.store offset=12
    local.get 0
    local.get 3
    i32.store offset=16
    local.get 0
    local.get 4
    i32.store offset=20
    return)
  (func $_ZN4core4iter8adapters13Skip$LT$I$GT$3new17h256f29acda141f5bE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i64 i64 i32 i32 i32 i64)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    i32.const 8
    local.set 6
    local.get 5
    local.get 6
    i32.add
    local.set 7
    local.get 7
    local.set 8
    local.get 5
    local.get 2
    i32.store offset=28
    local.get 1
    i64.load align=4
    local.set 9
    local.get 8
    local.get 9
    i64.store align=4
    i32.const 8
    local.set 10
    local.get 8
    local.get 10
    i32.add
    local.set 11
    local.get 1
    local.get 10
    i32.add
    local.set 12
    local.get 12
    i64.load align=4
    local.set 13
    local.get 11
    local.get 13
    i64.store align=4
    local.get 8
    i64.load align=4
    local.set 14
    local.get 0
    local.get 14
    i64.store align=4
    i32.const 8
    local.set 15
    local.get 0
    local.get 15
    i32.add
    local.set 16
    local.get 8
    local.get 15
    i32.add
    local.set 17
    local.get 17
    i64.load align=4
    local.set 18
    local.get 16
    local.get 18
    i64.store align=4
    local.get 0
    local.get 2
    i32.store offset=16
    return)
  (func $_ZN63_$LT$I$u20$as$u20$core..iter..traits..collect..IntoIterator$GT$9into_iter17hcd6d727ae252bd75E (type 6) (param i32 i32)
    (local i64 i32 i32 i32 i32 i32 i32 i32 i64)
    local.get 1
    i64.load align=4
    local.set 2
    local.get 0
    local.get 2
    i64.store align=4
    i32.const 16
    local.set 3
    local.get 0
    local.get 3
    i32.add
    local.set 4
    local.get 1
    local.get 3
    i32.add
    local.set 5
    local.get 5
    i32.load
    local.set 6
    local.get 4
    local.get 6
    i32.store
    i32.const 8
    local.set 7
    local.get 0
    local.get 7
    i32.add
    local.set 8
    local.get 1
    local.get 7
    i32.add
    local.set 9
    local.get 9
    i64.load align=4
    local.set 10
    local.get 8
    local.get 10
    i64.store align=4
    return)
  (func $_ZN94_$LT$core..iter..adapters..Skip$LT$I$GT$$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h4b71f81035100e86E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 1
    i32.store offset=8
    local.get 1
    i32.load offset=16
    local.set 5
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 5
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          local.set 6
          local.get 1
          i32.load offset=16
          local.set 7
          local.get 4
          local.get 7
          i32.store offset=12
          local.get 1
          local.get 6
          i32.store offset=16
          local.get 0
          local.get 1
          local.get 7
          call $_ZN4core4iter6traits8iterator8Iterator3nth17h001a7959d1922ee4E
          br 1 (;@2;)
        end
        local.get 0
        local.get 1
        call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha4eaab787b46e43fE
        br 1 (;@1;)
      end
    end
    i32.const 16
    local.set 8
    local.get 4
    local.get 8
    i32.add
    local.set 9
    local.get 9
    global.set 0
    return)
  (func $_ZN4core3str19from_utf8_unchecked17h5f5d405909b090c1E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 16
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store
    local.get 5
    local.get 2
    i32.store offset=4
    local.get 5
    local.get 1
    i32.store offset=8
    local.get 5
    local.get 2
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 6
    local.get 5
    i32.load offset=12
    local.set 7
    local.get 0
    local.get 7
    i32.store offset=4
    local.get 0
    local.get 6
    i32.store
    return)
  (func $_ZN4core4iter6traits10exact_size17ExactSizeIterator3len17h4eb491ab2a95e50bE (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 144
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.add
    local.set 5
    local.get 5
    local.set 6
    local.get 3
    local.get 0
    i32.store offset=120
    local.get 6
    local.get 0
    call $_ZN88_$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..iter..traits..iterator..Iterator$GT$9size_hint17h5e93a2f1a9a4600fE
    i32.const 56
    local.set 7
    local.get 3
    local.get 7
    i32.add
    local.set 8
    local.get 8
    local.set 9
    i32.const 24
    local.set 10
    local.get 3
    local.get 10
    i32.add
    local.set 11
    local.get 11
    local.set 12
    i32.const 1
    local.set 13
    local.get 3
    i32.load offset=32
    local.set 14
    local.get 3
    local.get 14
    i32.store offset=124
    local.get 3
    i32.load offset=36
    local.set 15
    local.get 3
    i32.load offset=40
    local.set 16
    local.get 3
    local.get 15
    i32.store offset=24
    local.get 3
    local.get 16
    i32.store offset=28
    local.get 3
    local.get 14
    i32.store offset=60
    local.get 3
    local.get 13
    i32.store offset=56
    local.get 3
    local.get 12
    i32.store offset=48
    local.get 3
    local.get 9
    i32.store offset=52
    local.get 3
    i32.load offset=48
    local.set 17
    local.get 3
    local.get 17
    i32.store offset=128
    local.get 3
    i32.load offset=52
    local.set 18
    local.get 3
    local.get 18
    i32.store offset=132
    local.get 17
    local.get 18
    call $_ZN70_$LT$core..option..Option$LT$T$GT$$u20$as$u20$core..cmp..PartialEq$GT$2eq17hbc95401444e93099E
    local.set 19
    i32.const -1
    local.set 20
    local.get 19
    local.get 20
    i32.xor
    local.set 21
    i32.const 1
    local.set 22
    local.get 21
    local.get 22
    i32.and
    local.set 23
    block  ;; label = @1
      local.get 23
      br_if 0 (;@1;)
      i32.const 144
      local.set 24
      local.get 3
      local.get 24
      i32.add
      local.set 25
      local.get 25
      global.set 0
      local.get 14
      return
    end
    i32.const 116
    local.set 26
    local.get 3
    local.get 26
    i32.add
    local.set 27
    local.get 27
    local.set 28
    i32.const 112
    local.set 29
    local.get 3
    local.get 29
    i32.add
    local.set 30
    local.get 30
    local.set 31
    i32.const 0
    local.set 32
    local.get 32
    i32.load offset=1048756
    local.set 33
    local.get 3
    local.get 17
    i32.store offset=112
    local.get 3
    local.get 18
    i32.store offset=116
    local.get 3
    local.get 31
    i32.store offset=104
    local.get 3
    local.get 28
    i32.store offset=108
    local.get 3
    i32.load offset=104
    local.set 34
    local.get 3
    local.get 34
    i32.store offset=136
    local.get 3
    i32.load offset=108
    local.set 35
    local.get 3
    local.get 35
    i32.store offset=140
    i32.const 8
    local.set 36
    i32.const 16
    local.set 37
    local.get 3
    local.get 37
    i32.add
    local.set 38
    local.get 38
    local.get 34
    local.get 36
    call $_ZN4core3fmt10ArgumentV13new17h11b37a0f8190df73E
    local.get 3
    i32.load offset=20
    local.set 39
    local.get 3
    i32.load offset=16
    local.set 40
    i32.const 8
    local.set 41
    i32.const 8
    local.set 42
    local.get 3
    local.get 42
    i32.add
    local.set 43
    local.get 43
    local.get 35
    local.get 41
    call $_ZN4core3fmt10ArgumentV13new17h11b37a0f8190df73E
    local.get 3
    i32.load offset=12
    local.set 44
    local.get 3
    i32.load offset=8
    local.set 45
    i32.const 64
    local.set 46
    local.get 3
    local.get 46
    i32.add
    local.set 47
    local.get 47
    local.set 48
    i32.const 3
    local.set 49
    i32.const 2
    local.set 50
    i32.const 88
    local.set 51
    local.get 3
    local.get 51
    i32.add
    local.set 52
    local.get 52
    local.set 53
    local.get 3
    local.get 40
    i32.store offset=88
    local.get 3
    local.get 39
    i32.store offset=92
    local.get 3
    local.get 45
    i32.store offset=96
    local.get 3
    local.get 44
    i32.store offset=100
    local.get 48
    local.get 33
    local.get 49
    local.get 53
    local.get 50
    call $_ZN4core3fmt9Arguments6new_v117h1235a630ff649770E
    i32.const 64
    local.set 54
    local.get 3
    local.get 54
    i32.add
    local.set 55
    local.get 55
    local.set 56
    i32.const 1048852
    local.set 57
    local.get 57
    local.set 58
    local.get 56
    local.get 58
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN88_$LT$alloc..vec..IntoIter$LT$T$GT$$u20$as$u20$core..iter..traits..iterator..Iterator$GT$9size_hint17h5e93a2f1a9a4600fE (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 32
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 12
    local.set 5
    local.get 4
    local.get 1
    i32.store offset=24
    local.get 4
    local.get 5
    i32.store offset=28
    local.get 4
    i32.load offset=28
    local.set 6
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 6
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load offset=12
          local.set 7
          local.get 1
          i32.load offset=8
          local.set 8
          local.get 7
          local.get 8
          call $_ZN4core3ptr9const_ptr33_$LT$impl$u20$$BP$const$u20$T$GT$11offset_from17hf3ebb03db18755d9E
          local.set 9
          br 1 (;@2;)
        end
        local.get 1
        i32.load offset=12
        local.set 10
        local.get 1
        i32.load offset=8
        local.set 11
        local.get 10
        local.get 11
        call $_ZN4core3num23_$LT$impl$u20$usize$GT$12wrapping_sub17h9fa155a4799bc4beE
        local.set 12
        local.get 4
        local.get 12
        i32.store offset=12
        br 1 (;@1;)
      end
      local.get 4
      local.get 9
      i32.store offset=12
    end
    i32.const 1
    local.set 13
    local.get 4
    i32.load offset=12
    local.set 14
    local.get 4
    i32.load offset=12
    local.set 15
    local.get 4
    local.get 15
    i32.store offset=20
    local.get 4
    local.get 13
    i32.store offset=16
    local.get 0
    local.get 14
    i32.store
    local.get 4
    i32.load offset=16
    local.set 16
    local.get 4
    i32.load offset=20
    local.set 17
    local.get 0
    local.get 16
    i32.store offset=4
    local.get 0
    local.get 17
    i32.store offset=8
    i32.const 32
    local.set 18
    local.get 4
    local.get 18
    i32.add
    local.set 19
    local.get 19
    global.set 0
    return)
  (func $_ZN5alloc3vec12Vec$LT$T$GT$10as_mut_ptr17h978888bef43aeb69E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 0
    call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$3ptr17hce4b631693c59b34E
    local.set 4
    local.get 3
    local.get 4
    i32.store offset=12
    local.get 4
    call $_ZN4core3ptr7mut_ptr31_$LT$impl$u20$$BP$mut$u20$T$GT$7is_null17hc75b11a05a4cbe5fE
    drop
    i32.const 16
    local.set 5
    local.get 3
    local.get 5
    i32.add
    local.set 6
    local.get 6
    global.set 0
    local.get 4
    return)
  (func $_ZN5alloc3vec12Vec$LT$T$GT$6as_ptr17h40f0ac156d7ebe13E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 0
    call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$3ptr17hce4b631693c59b34E
    local.set 4
    local.get 3
    local.get 4
    i32.store offset=12
    local.get 4
    call $_ZN4core3ptr7mut_ptr31_$LT$impl$u20$$BP$mut$u20$T$GT$7is_null17hc75b11a05a4cbe5fE
    drop
    i32.const 16
    local.set 5
    local.get 3
    local.get 5
    i32.add
    local.set 6
    local.get 6
    global.set 0
    local.get 4
    return)
  (func $_ZN5alloc3vec17IntoIter$LT$T$GT$16as_raw_mut_slice17h79f3c86057cf2815E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 12
    local.set 5
    local.get 4
    local.get 5
    i32.add
    local.set 6
    local.get 6
    local.set 7
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 4
    i32.load offset=12
    local.set 8
    local.get 8
    i32.load offset=8
    local.set 9
    local.get 7
    call $_ZN83_$LT$$RF$mut$u20$I$u20$as$u20$core..iter..traits..exact_size..ExactSizeIterator$GT$3len17h92389381ace86938E
    local.set 10
    local.get 4
    local.get 9
    local.get 10
    call $_ZN4core3ptr24slice_from_raw_parts_mut17h64a6a973012e64aaE
    local.get 4
    i32.load offset=4
    local.set 11
    local.get 4
    i32.load
    local.set 12
    local.get 0
    local.get 11
    i32.store offset=4
    local.get 0
    local.get 12
    i32.store
    i32.const 16
    local.set 13
    local.get 4
    local.get 13
    i32.add
    local.set 14
    local.get 14
    global.set 0
    return)
  (func $_ZN83_$LT$$RF$mut$u20$I$u20$as$u20$core..iter..traits..exact_size..ExactSizeIterator$GT$3len17h92389381ace86938E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 1
    i32.const 16
    local.set 2
    local.get 1
    local.get 2
    i32.sub
    local.set 3
    local.get 3
    global.set 0
    local.get 3
    local.get 0
    i32.store offset=12
    local.get 0
    i32.load
    local.set 4
    local.get 4
    call $_ZN4core4iter6traits10exact_size17ExactSizeIterator3len17h4eb491ab2a95e50bE
    local.set 5
    i32.const 16
    local.set 6
    local.get 3
    local.get 6
    i32.add
    local.set 7
    local.get 7
    global.set 0
    local.get 5
    return)
  (func $_ZN68_$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$core..ops..deref..Deref$GT$5deref17h246cead344bb2ee4E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 1
    call $_ZN5alloc3vec12Vec$LT$T$GT$6as_ptr17h40f0ac156d7ebe13E
    local.set 5
    local.get 1
    i32.load offset=8
    local.set 6
    local.get 4
    local.get 5
    local.get 6
    call $_ZN4core5slice14from_raw_parts17h0c33318ff019e7acE
    local.get 4
    i32.load offset=4
    local.set 7
    local.get 4
    i32.load
    local.set 8
    local.get 0
    local.get 7
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    i32.const 16
    local.set 9
    local.get 4
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    return)
  (func $_ZN4core3ptr9const_ptr33_$LT$impl$u20$$BP$const$u20$T$GT$11offset_from17hf3ebb03db18755d9E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 32
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    i32.const 12
    local.set 5
    local.get 4
    local.get 0
    i32.store offset=12
    local.get 4
    local.get 1
    i32.store offset=16
    local.get 4
    local.get 5
    i32.store offset=28
    local.get 4
    i32.load offset=28
    local.set 6
    local.get 4
    local.get 6
    i32.store offset=20
    i32.const 0
    local.set 7
    local.get 7
    local.set 8
    local.get 6
    local.set 9
    local.get 8
    local.get 9
    i32.lt_u
    local.set 10
    i32.const 1
    local.set 11
    local.get 10
    local.get 11
    i32.and
    local.set 12
    block  ;; label = @1
      block  ;; label = @2
        local.get 12
        br_if 0 (;@2;)
        i32.const 0
        local.set 13
        local.get 4
        local.get 13
        i32.store8 offset=11
        br 1 (;@1;)
      end
      i32.const 0
      local.set 14
      i32.const 2147483647
      local.set 15
      local.get 6
      local.set 16
      local.get 15
      local.set 17
      local.get 16
      local.get 17
      i32.le_u
      local.set 18
      i32.const 1
      local.set 19
      local.get 18
      local.get 19
      i32.and
      local.set 20
      i32.const 1
      local.set 21
      local.get 14
      local.get 21
      i32.and
      local.set 22
      local.get 20
      local.get 22
      i32.ne
      local.set 23
      i32.const 1
      local.set 24
      local.get 23
      local.get 24
      i32.and
      local.set 25
      local.get 4
      local.get 25
      i32.store8 offset=11
    end
    local.get 4
    i32.load8_u offset=11
    local.set 26
    i32.const -1
    local.set 27
    local.get 26
    local.get 27
    i32.xor
    local.set 28
    i32.const 1
    local.set 29
    local.get 28
    local.get 29
    i32.and
    local.set 30
    block  ;; label = @1
      block  ;; label = @2
        local.get 30
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        i32.sub
        local.set 31
        i32.const 12
        local.set 32
        local.get 31
        local.get 32
        i32.div_s
        local.set 33
        local.get 4
        local.get 33
        i32.store offset=24
        local.get 4
        i32.load offset=24
        local.set 34
        br 1 (;@1;)
      end
      i32.const 1048868
      local.set 35
      local.get 35
      local.set 36
      i32.const 73
      local.set 37
      i32.const 1049024
      local.set 38
      local.get 38
      local.set 39
      local.get 36
      local.get 37
      local.get 39
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    i32.const 32
    local.set 40
    local.get 4
    local.get 40
    i32.add
    local.set 41
    local.get 41
    global.set 0
    local.get 34
    return)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h33d1e65ef0c901faE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 2
    i32.const 16
    local.set 3
    local.get 2
    local.get 3
    i32.sub
    local.set 4
    local.get 4
    global.set 0
    local.get 4
    local.get 0
    i32.store offset=8
    local.get 4
    local.get 1
    i32.store offset=12
    local.get 0
    i32.load
    local.set 5
    local.get 5
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h65966affeede9f70E
    local.set 6
    i32.const 1
    local.set 7
    local.get 6
    local.get 7
    i32.and
    local.set 8
    i32.const 16
    local.set 9
    local.get 4
    local.get 9
    i32.add
    local.set 10
    local.get 10
    global.set 0
    local.get 8
    return)
  (func $_ZN4core3fmt10ArgumentV13new17h11b37a0f8190df73E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 5
    local.get 2
    i32.store offset=24
    local.get 5
    i32.load offset=24
    local.set 6
    local.get 5
    local.get 1
    i32.store offset=28
    local.get 5
    i32.load offset=28
    local.set 7
    local.get 5
    local.get 7
    i32.store offset=8
    local.get 5
    local.get 6
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 8
    local.get 5
    i32.load offset=12
    local.set 9
    local.get 0
    local.get 9
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    return)
  (func $_ZN4core3fmt10ArgumentV13new17hf05c719d0e6f4c6dE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    local.set 3
    i32.const 32
    local.set 4
    local.get 3
    local.get 4
    i32.sub
    local.set 5
    local.get 5
    local.get 1
    i32.store offset=16
    local.get 5
    local.get 2
    i32.store offset=20
    local.get 5
    local.get 2
    i32.store offset=24
    local.get 5
    i32.load offset=24
    local.set 6
    local.get 5
    local.get 1
    i32.store offset=28
    local.get 5
    i32.load offset=28
    local.set 7
    local.get 5
    local.get 7
    i32.store offset=8
    local.get 5
    local.get 6
    i32.store offset=12
    local.get 5
    i32.load offset=8
    local.set 8
    local.get 5
    i32.load offset=12
    local.set 9
    local.get 0
    local.get 9
    i32.store offset=4
    local.get 0
    local.get 8
    i32.store
    return)
  (func $__rust_alloc (type 3) (param i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    call $__rdl_alloc
    local.set 2
    local.get 2
    return)
  (func $__rust_dealloc (type 7) (param i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    call $__rdl_dealloc
    return)
  (func $__rust_realloc (type 9) (param i32 i32 i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    local.get 2
    local.get 3
    call $__rdl_realloc
    local.set 4
    local.get 4
    return)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2c23cfb6ed5309fbE (type 2) (param i32) (result i64)
    i64.const 9147559743429524724)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h572695cb9d46eea3E (type 2) (param i32) (result i64)
    i64.const -2432489959605628846)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hfe13144b583fb519E (type 2) (param i32) (result i64)
    i64.const -1895584942197572387)
  (func $_ZN73_$LT$std..sys_common..os_str_bytes..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hf2181278eaad9e60E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
    global.get 0
    i32.const 80
    i32.sub
    local.tee 3
    global.set 0
    i32.const 1
    local.set 4
    block  ;; label = @1
      local.get 2
      i32.const 1049856
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17he7969cebfa3abe45E
      br_if 0 (;@1;)
      local.get 3
      i32.const 8
      i32.add
      local.get 0
      local.get 1
      call $_ZN4core3str5lossy9Utf8Lossy10from_bytes17hc86396f264469a03E
      local.get 3
      local.get 3
      i32.load offset=8
      local.get 3
      i32.load offset=12
      call $_ZN4core3str5lossy9Utf8Lossy6chunks17hc470c0fce5aec5cfE
      local.get 3
      local.get 3
      i64.load
      i64.store offset=16
      local.get 3
      i32.const 40
      i32.add
      local.get 3
      i32.const 16
      i32.add
      call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha10c9da4cfe4c60bE
      block  ;; label = @2
        local.get 3
        i32.load offset=40
        local.tee 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        i32.const 48
        i32.add
        local.set 5
        local.get 3
        i32.const 64
        i32.add
        local.set 6
        loop  ;; label = @3
          local.get 3
          i32.load offset=52
          local.set 7
          local.get 3
          i32.load offset=48
          local.set 8
          local.get 3
          i32.load offset=44
          local.set 0
          local.get 3
          i32.const 4
          i32.store offset=64
          local.get 3
          i32.const 4
          i32.store offset=48
          local.get 3
          local.get 4
          i32.store offset=40
          local.get 3
          local.get 4
          local.get 0
          i32.add
          i32.store offset=44
          i32.const 4
          local.set 4
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                loop  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      local.get 4
                                      i32.const 4
                                      i32.eq
                                      br_if 0 (;@17;)
                                      local.get 5
                                      call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17he27bc962589af5c1E
                                      local.tee 4
                                      i32.const 1114112
                                      i32.ne
                                      br_if 1 (;@16;)
                                      local.get 3
                                      i32.const 4
                                      i32.store offset=48
                                    end
                                    block  ;; label = @17
                                      local.get 3
                                      i32.load offset=44
                                      local.tee 0
                                      local.get 3
                                      i32.load offset=40
                                      local.tee 4
                                      i32.eq
                                      br_if 0 (;@17;)
                                      local.get 3
                                      local.get 4
                                      i32.const 1
                                      i32.add
                                      local.tee 9
                                      i32.store offset=40
                                      block  ;; label = @18
                                        block  ;; label = @19
                                          local.get 4
                                          i32.load8_s
                                          local.tee 1
                                          i32.const -1
                                          i32.le_s
                                          br_if 0 (;@19;)
                                          local.get 1
                                          i32.const 255
                                          i32.and
                                          local.set 0
                                          br 1 (;@18;)
                                        end
                                        block  ;; label = @19
                                          block  ;; label = @20
                                            local.get 9
                                            local.get 0
                                            i32.ne
                                            br_if 0 (;@20;)
                                            i32.const 0
                                            local.set 10
                                            local.get 0
                                            local.set 9
                                            br 1 (;@19;)
                                          end
                                          local.get 3
                                          local.get 4
                                          i32.const 2
                                          i32.add
                                          local.tee 9
                                          i32.store offset=40
                                          local.get 4
                                          i32.load8_u offset=1
                                          i32.const 63
                                          i32.and
                                          local.set 10
                                        end
                                        local.get 1
                                        i32.const 31
                                        i32.and
                                        local.set 4
                                        block  ;; label = @19
                                          local.get 1
                                          i32.const 255
                                          i32.and
                                          local.tee 1
                                          i32.const 223
                                          i32.gt_u
                                          br_if 0 (;@19;)
                                          local.get 10
                                          local.get 4
                                          i32.const 6
                                          i32.shl
                                          i32.or
                                          local.set 0
                                          br 1 (;@18;)
                                        end
                                        block  ;; label = @19
                                          block  ;; label = @20
                                            local.get 9
                                            local.get 0
                                            i32.ne
                                            br_if 0 (;@20;)
                                            i32.const 0
                                            local.set 9
                                            local.get 0
                                            local.set 11
                                            br 1 (;@19;)
                                          end
                                          local.get 3
                                          local.get 9
                                          i32.const 1
                                          i32.add
                                          local.tee 11
                                          i32.store offset=40
                                          local.get 9
                                          i32.load8_u
                                          i32.const 63
                                          i32.and
                                          local.set 9
                                        end
                                        local.get 9
                                        local.get 10
                                        i32.const 6
                                        i32.shl
                                        i32.or
                                        local.set 9
                                        block  ;; label = @19
                                          local.get 1
                                          i32.const 240
                                          i32.ge_u
                                          br_if 0 (;@19;)
                                          local.get 9
                                          local.get 4
                                          i32.const 12
                                          i32.shl
                                          i32.or
                                          local.set 0
                                          br 1 (;@18;)
                                        end
                                        block  ;; label = @19
                                          block  ;; label = @20
                                            local.get 11
                                            local.get 0
                                            i32.ne
                                            br_if 0 (;@20;)
                                            i32.const 0
                                            local.set 0
                                            br 1 (;@19;)
                                          end
                                          local.get 3
                                          local.get 11
                                          i32.const 1
                                          i32.add
                                          i32.store offset=40
                                          local.get 11
                                          i32.load8_u
                                          i32.const 63
                                          i32.and
                                          local.set 0
                                        end
                                        local.get 9
                                        i32.const 6
                                        i32.shl
                                        local.get 4
                                        i32.const 18
                                        i32.shl
                                        i32.const 1835008
                                        i32.and
                                        i32.or
                                        local.get 0
                                        i32.or
                                        local.set 0
                                      end
                                      i32.const 2
                                      local.set 4
                                      i32.const 116
                                      local.set 1
                                      block  ;; label = @18
                                        local.get 0
                                        i32.const -9
                                        i32.add
                                        br_table 10 (;@8;) 4 (;@14;) 5 (;@13;) 5 (;@13;) 3 (;@15;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 6 (;@12;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 5 (;@13;) 6 (;@12;) 0 (;@18;)
                                      end
                                      local.get 0
                                      i32.const 92
                                      i32.eq
                                      br_if 5 (;@12;)
                                      local.get 0
                                      i32.const 1114112
                                      i32.ne
                                      br_if 4 (;@13;)
                                    end
                                    local.get 3
                                    i32.load offset=64
                                    i32.const 4
                                    i32.eq
                                    br_if 10 (;@6;)
                                    local.get 6
                                    call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17he27bc962589af5c1E
                                    local.tee 4
                                    i32.const 1114112
                                    i32.eq
                                    br_if 10 (;@6;)
                                  end
                                  local.get 2
                                  local.get 4
                                  call $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h60363d204979c277E
                                  br_if 10 (;@5;)
                                  local.get 3
                                  i32.load offset=48
                                  local.set 4
                                  br 8 (;@7;)
                                end
                                i32.const 114
                                local.set 1
                                br 6 (;@8;)
                              end
                              i32.const 110
                              local.set 1
                              br 5 (;@8;)
                            end
                            block  ;; label = @13
                              local.get 0
                              call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h29890100314325f2E
                              i32.eqz
                              br_if 0 (;@13;)
                              local.get 0
                              i32.const 1
                              i32.or
                              i32.clz
                              i32.const 2
                              i32.shr_u
                              i32.const 7
                              i32.xor
                              i64.extend_i32_u
                              i64.const 21474836480
                              i64.or
                              local.set 12
                              br 3 (;@10;)
                            end
                            i32.const 1
                            local.set 4
                            local.get 0
                            call $_ZN4core7unicode9printable12is_printable17hbb0b3da4690fa839E
                            i32.eqz
                            br_if 1 (;@11;)
                          end
                          br 2 (;@9;)
                        end
                        local.get 0
                        i32.const 1
                        i32.or
                        i32.clz
                        i32.const 2
                        i32.shr_u
                        i32.const 7
                        i32.xor
                        i64.extend_i32_u
                        i64.const 21474836480
                        i64.or
                        local.set 12
                      end
                      i32.const 3
                      local.set 4
                    end
                    local.get 0
                    local.set 1
                  end
                  local.get 3
                  local.get 12
                  i64.store offset=56
                  local.get 3
                  local.get 1
                  i32.store offset=52
                  local.get 3
                  local.get 4
                  i32.store offset=48
                  br 0 (;@7;)
                end
              end
              loop  ;; label = @6
                local.get 7
                i32.eqz
                br_if 2 (;@4;)
                local.get 3
                local.get 8
                i32.store offset=28
                local.get 3
                i32.const 1
                i32.store offset=60
                local.get 3
                i32.const 1
                i32.store offset=52
                local.get 3
                i32.const 1051496
                i32.store offset=48
                local.get 3
                i32.const 1
                i32.store offset=44
                local.get 3
                i32.const 1051488
                i32.store offset=40
                local.get 3
                i32.const 9
                i32.store offset=36
                local.get 7
                i32.const -1
                i32.add
                local.set 7
                local.get 8
                i32.const 1
                i32.add
                local.set 8
                local.get 3
                local.get 3
                i32.const 32
                i32.add
                i32.store offset=56
                local.get 3
                local.get 3
                i32.const 28
                i32.add
                i32.store offset=32
                local.get 2
                local.get 3
                i32.const 40
                i32.add
                call $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E
                i32.eqz
                br_if 0 (;@6;)
              end
            end
            i32.const 1
            local.set 4
            br 3 (;@1;)
          end
          local.get 3
          i32.const 40
          i32.add
          local.get 3
          i32.const 16
          i32.add
          call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha10c9da4cfe4c60bE
          local.get 3
          i32.load offset=40
          local.tee 4
          br_if 0 (;@3;)
        end
      end
      local.get 2
      i32.const 1049856
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17he7969cebfa3abe45E
      local.set 4
    end
    local.get 3
    i32.const 80
    i32.add
    global.set 0
    local.get 4)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h31b8a240b20122ecE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.set 0
    block  ;; label = @1
      local.get 1
      call $_ZN4core3fmt9Formatter15debug_lower_hex17hfc16dde1881b9db0E
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 1
        call $_ZN4core3fmt9Formatter15debug_upper_hex17hf900d6ea61ff8af8E
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        call $_ZN4core3fmt3num3imp51_$LT$impl$u20$core..fmt..Display$u20$for$u20$u8$GT$3fmt17h891c553833cf799dE
        return
      end
      local.get 0
      local.get 1
      call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17h478aac1c493c7938E
      return
    end
    local.get 0
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i8$GT$3fmt17hfd3f164dab1c4039E)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17ha6ef0d9ad75832cdE (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.tee 0
    i32.load offset=8
    local.set 3
    local.get 0
    i32.load
    local.set 0
    local.get 2
    local.get 1
    call $_ZN4core3fmt9Formatter10debug_list17h030b105a753c1383E
    block  ;; label = @1
      local.get 3
      i32.eqz
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 2
        local.get 0
        i32.store offset=12
        local.get 2
        local.get 2
        i32.const 12
        i32.add
        i32.const 1049112
        call $_ZN4core3fmt8builders8DebugSet5entry17hefdeddab1ca2936fE
        drop
        local.get 0
        i32.const 1
        i32.add
        local.set 0
        local.get 3
        i32.const -1
        i32.add
        local.tee 3
        br_if 0 (;@2;)
      end
    end
    local.get 2
    call $_ZN4core3fmt8builders9DebugList6finish17ha88b4158b5009243E
    local.set 0
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hda98ecdf0621cbe4E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.set 0
    block  ;; label = @1
      local.get 1
      call $_ZN4core3fmt9Formatter15debug_lower_hex17hfc16dde1881b9db0E
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 1
        call $_ZN4core3fmt9Formatter15debug_upper_hex17hf900d6ea61ff8af8E
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        call $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17hc79ab98580260a0dE
        return
      end
      local.get 0
      local.get 1
      call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17h75226882cdaf372cE
      return
    end
    local.get 0
    local.get 1
    call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17haf413b6a293768dfE)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h2ffe9d8886f9157bE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN60_$LT$core..panic..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h24f4d50dfe86b3a3E)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h6a7113d5e1f92fc5E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hce18dc51dfa14637E)
  (func $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h228b462170cfd3d4E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17h478aac1c493c7938E)
  (func $_ZN4core3fmt5Write10write_char17h524458e201538bfcE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 0
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 128
            i32.lt_u
            br_if 0 (;@4;)
            local.get 1
            i32.const 2048
            i32.lt_u
            br_if 1 (;@3;)
            local.get 2
            i32.const 4
            i32.add
            local.set 3
            local.get 1
            i32.const 65536
            i32.ge_u
            br_if 2 (;@2;)
            local.get 2
            local.get 1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=6
            local.get 2
            local.get 1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=4
            local.get 2
            local.get 1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=5
            i32.const 3
            local.set 1
            br 3 (;@1;)
          end
          local.get 2
          local.get 1
          i32.store8 offset=4
          local.get 2
          i32.const 4
          i32.add
          local.set 3
          i32.const 1
          local.set 1
          br 2 (;@1;)
        end
        local.get 2
        local.get 1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=5
        local.get 2
        local.get 1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=4
        local.get 2
        i32.const 4
        i32.add
        local.set 3
        i32.const 2
        local.set 1
        br 1 (;@1;)
      end
      local.get 2
      local.get 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=7
      local.get 2
      local.get 1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=4
      local.get 2
      local.get 1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=6
      local.get 2
      local.get 1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=5
      i32.const 4
      local.set 1
    end
    local.get 2
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.get 3
    local.get 1
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h34620f531d384434E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 2
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 2
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 3
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 3
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h34620f531d384434E (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      local.get 1
      i32.load
      local.tee 5
      i32.load
      br_if 0 (;@1;)
      local.get 5
      i32.const -1
      i32.store
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 5
          i32.const 4
          i32.add
          local.set 6
          loop  ;; label = @4
            local.get 4
            local.get 6
            local.get 2
            local.get 3
            call $_ZN73_$LT$std..io..buffered..LineWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17ha396c4b89d5aac4fE
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 4
                          i32.load
                          i32.const 1
                          i32.eq
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            local.get 4
                            i32.load offset=4
                            local.tee 1
                            br_if 0 (;@12;)
                            i32.const 28
                            i32.const 1
                            call $__rust_alloc
                            local.tee 3
                            i32.eqz
                            br_if 4 (;@8;)
                            local.get 3
                            i32.const 24
                            i32.add
                            i32.const 0
                            i32.load offset=1050600 align=1
                            i32.store align=1
                            local.get 3
                            i32.const 16
                            i32.add
                            i32.const 0
                            i64.load offset=1050592 align=1
                            i64.store align=1
                            local.get 3
                            i32.const 8
                            i32.add
                            i32.const 0
                            i64.load offset=1050584 align=1
                            i64.store align=1
                            local.get 3
                            i32.const 0
                            i64.load offset=1050576 align=1
                            i64.store align=1
                            i32.const 12
                            i32.const 4
                            call $__rust_alloc
                            local.tee 2
                            i32.eqz
                            br_if 5 (;@7;)
                            local.get 2
                            i64.const 120259084316
                            i64.store offset=4 align=4
                            local.get 2
                            local.get 3
                            i32.store
                            i32.const 12
                            i32.const 4
                            call $__rust_alloc
                            local.tee 3
                            i32.eqz
                            br_if 6 (;@6;)
                            local.get 3
                            i32.const 14
                            i32.store8 offset=8
                            local.get 3
                            i32.const 1049980
                            i32.store offset=4
                            local.get 3
                            local.get 2
                            i32.store
                            local.get 3
                            local.get 4
                            i32.load16_u offset=13 align=1
                            i32.store16 offset=9 align=1
                            local.get 3
                            i32.const 11
                            i32.add
                            local.get 4
                            i32.const 13
                            i32.add
                            i32.const 2
                            i32.add
                            i32.load8_u
                            i32.store8
                            local.get 0
                            i32.const 4
                            i32.add
                            local.get 3
                            i32.store
                            local.get 0
                            i32.const 2
                            i32.store
                            br 10 (;@2;)
                          end
                          local.get 3
                          local.get 1
                          i32.ge_u
                          br_if 1 (;@10;)
                          local.get 1
                          local.get 3
                          i32.const 1050852
                          call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
                          unreachable
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                local.get 4
                                i32.load8_u offset=4
                                local.tee 7
                                br_table 1 (;@13;) 0 (;@14;) 2 (;@12;) 1 (;@13;)
                              end
                              local.get 4
                              i32.load8_u offset=5
                              local.set 1
                              br 2 (;@11;)
                            end
                            local.get 4
                            i32.load offset=8
                            call $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E
                            i32.const 255
                            i32.and
                            local.set 1
                            br 1 (;@11;)
                          end
                          local.get 4
                          i32.load offset=8
                          i32.load8_u offset=8
                          local.set 1
                        end
                        local.get 1
                        i32.const 255
                        i32.and
                        i32.const 15
                        i32.eq
                        br_if 1 (;@9;)
                        local.get 0
                        local.get 4
                        i64.load offset=4 align=4
                        i64.store align=4
                        br 8 (;@2;)
                      end
                      local.get 2
                      local.get 1
                      i32.add
                      local.set 2
                      local.get 3
                      local.get 1
                      i32.sub
                      local.set 3
                      br 4 (;@5;)
                    end
                    local.get 7
                    i32.const 2
                    i32.lt_u
                    br_if 3 (;@5;)
                    local.get 4
                    i32.load offset=8
                    local.tee 1
                    i32.load
                    local.get 1
                    i32.load offset=4
                    i32.load
                    call_indirect (type 1)
                    block  ;; label = @9
                      local.get 1
                      i32.load offset=4
                      local.tee 7
                      i32.load offset=4
                      local.tee 8
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 1
                      i32.load
                      local.get 8
                      local.get 7
                      i32.load offset=8
                      call $__rust_dealloc
                    end
                    local.get 1
                    i32.const 12
                    i32.const 4
                    call $__rust_dealloc
                    br 3 (;@5;)
                  end
                  i32.const 28
                  i32.const 1
                  call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                  unreachable
                end
                i32.const 12
                i32.const 4
                call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                unreachable
              end
              i32.const 12
              i32.const 4
              call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
              unreachable
            end
            local.get 3
            br_if 0 (;@4;)
          end
        end
        local.get 0
        i32.const 3
        i32.store8
      end
      local.get 5
      local.get 5
      i32.load
      i32.const 1
      i32.add
      i32.store
      local.get 4
      i32.const 16
      i32.add
      global.set 0
      return
    end
    i32.const 1049128
    i32.const 16
    local.get 4
    i32.const 1049328
    i32.const 1050684
    call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
    unreachable)
  (func $_ZN4core3fmt5Write10write_char17hae5a84beb8d1055fE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 0
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 128
            i32.lt_u
            br_if 0 (;@4;)
            local.get 1
            i32.const 2048
            i32.lt_u
            br_if 1 (;@3;)
            local.get 2
            i32.const 4
            i32.add
            local.set 3
            local.get 1
            i32.const 65536
            i32.ge_u
            br_if 2 (;@2;)
            local.get 2
            local.get 1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=6
            local.get 2
            local.get 1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=4
            local.get 2
            local.get 1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=5
            i32.const 3
            local.set 1
            br 3 (;@1;)
          end
          local.get 2
          local.get 1
          i32.store8 offset=4
          local.get 2
          i32.const 4
          i32.add
          local.set 3
          i32.const 1
          local.set 1
          br 2 (;@1;)
        end
        local.get 2
        local.get 1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=5
        local.get 2
        local.get 1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=4
        local.get 2
        i32.const 4
        i32.add
        local.set 3
        i32.const 2
        local.set 1
        br 1 (;@1;)
      end
      local.get 2
      local.get 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=7
      local.get 2
      local.get 1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=4
      local.get 2
      local.get 1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=6
      local.get 2
      local.get 1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=5
      i32.const 4
      local.set 1
    end
    local.get 2
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.get 3
    local.get 1
    call $_ZN3std2io5Write9write_all17h95d942a560570f98E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 2
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 2
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 3
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 3
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN3std2io5Write9write_all17h95d942a560570f98E (type 4) (param i32 i32 i32 i32)
    (local i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.eqz
              br_if 0 (;@5;)
              loop  ;; label = @6
                local.get 4
                local.get 3
                i32.store offset=12
                local.get 4
                local.get 2
                i32.store offset=8
                local.get 4
                i32.const 16
                i32.add
                i32.const 2
                local.get 4
                i32.const 8
                i32.add
                i32.const 1
                call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 4
                      i32.load16_u offset=16
                      i32.const 1
                      i32.eq
                      br_if 0 (;@9;)
                      block  ;; label = @10
                        local.get 4
                        i32.load offset=20
                        local.tee 5
                        br_if 0 (;@10;)
                        i32.const 28
                        i32.const 1
                        call $__rust_alloc
                        local.tee 3
                        i32.eqz
                        br_if 6 (;@4;)
                        local.get 3
                        i32.const 24
                        i32.add
                        i32.const 0
                        i32.load offset=1050600 align=1
                        i32.store align=1
                        local.get 3
                        i32.const 16
                        i32.add
                        i32.const 0
                        i64.load offset=1050592 align=1
                        i64.store align=1
                        local.get 3
                        i32.const 8
                        i32.add
                        i32.const 0
                        i64.load offset=1050584 align=1
                        i64.store align=1
                        local.get 3
                        i32.const 0
                        i64.load offset=1050576 align=1
                        i64.store align=1
                        i32.const 12
                        i32.const 4
                        call $__rust_alloc
                        local.tee 2
                        i32.eqz
                        br_if 7 (;@3;)
                        local.get 2
                        i64.const 120259084316
                        i64.store offset=4 align=4
                        local.get 2
                        local.get 3
                        i32.store
                        i32.const 12
                        i32.const 4
                        call $__rust_alloc
                        local.tee 3
                        i32.eqz
                        br_if 8 (;@2;)
                        local.get 3
                        i32.const 14
                        i32.store8 offset=8
                        local.get 3
                        i32.const 1049980
                        i32.store offset=4
                        local.get 3
                        local.get 2
                        i32.store
                        local.get 3
                        local.get 4
                        i32.load16_u offset=16 align=1
                        i32.store16 offset=9 align=1
                        local.get 3
                        i32.const 11
                        i32.add
                        local.get 4
                        i32.const 16
                        i32.add
                        i32.const 2
                        i32.add
                        i32.load8_u
                        i32.store8
                        local.get 0
                        i32.const 4
                        i32.add
                        local.get 3
                        i32.store
                        local.get 0
                        i32.const 2
                        i32.store
                        br 9 (;@1;)
                      end
                      local.get 3
                      local.get 5
                      i32.ge_u
                      br_if 1 (;@8;)
                      local.get 5
                      local.get 3
                      i32.const 1050852
                      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
                      unreachable
                    end
                    local.get 4
                    local.get 4
                    i32.load16_u offset=18
                    i32.store16 offset=30
                    local.get 4
                    i32.const 30
                    i32.add
                    call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
                    i32.const 65535
                    i32.and
                    local.tee 5
                    call $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E
                    i32.const 255
                    i32.and
                    i32.const 15
                    i32.eq
                    br_if 1 (;@7;)
                    local.get 0
                    i32.const 0
                    i32.store
                    local.get 0
                    i32.const 4
                    i32.add
                    local.get 5
                    i32.store
                    br 7 (;@1;)
                  end
                  local.get 2
                  local.get 5
                  i32.add
                  local.set 2
                  local.get 3
                  local.get 5
                  i32.sub
                  local.set 3
                end
                local.get 3
                br_if 0 (;@6;)
              end
            end
            local.get 0
            i32.const 3
            i32.store8
            br 3 (;@1;)
          end
          i32.const 28
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        i32.const 12
        i32.const 4
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 4
    i32.const 32
    i32.add
    global.set 0)
  (func $_ZN4core3fmt5Write9write_fmt17h0c8a6cfe66f8ee62E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1049040
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN4core3fmt5Write9write_fmt17hee3d2ccd7ee9e67cE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1049088
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN3std9panicking12default_hook17h8b278e563c507519E (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i64 i32)
    global.get 0
    i32.const 96
    i32.sub
    local.tee 1
    global.set 0
    i32.const 1
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058840
          i32.const 1
          i32.eq
          br_if 0 (;@3;)
          i32.const 0
          i64.const 1
          i64.store offset=1058840
          br 1 (;@2;)
        end
        i32.const 0
        i32.load offset=1058844
        i32.const 1
        i32.gt_u
        br_if 1 (;@1;)
      end
      i32.const 1
      local.set 2
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058764
            br_table 0 (;@4;) 1 (;@3;) 2 (;@2;) 3 (;@1;)
          end
          local.get 1
          i32.const 64
          i32.add
          i32.const 1049857
          i32.const 14
          call $_ZN3std3env7_var_os17hf271c90cefabfa97E
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load offset=64
              local.tee 3
              br_if 0 (;@5;)
              i32.const 5
              local.set 2
              br 1 (;@4;)
            end
            local.get 1
            i32.load offset=68
            local.set 4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 1
                    i32.const 72
                    i32.add
                    i32.load
                    i32.const -1
                    i32.add
                    br_table 0 (;@8;) 2 (;@6;) 2 (;@6;) 1 (;@7;) 2 (;@6;)
                  end
                  i32.const 4
                  local.set 2
                  i32.const 1
                  local.set 5
                  local.get 3
                  i32.const 1049871
                  i32.eq
                  br_if 2 (;@5;)
                  local.get 3
                  i32.load8_u
                  i32.const 48
                  i32.ne
                  br_if 1 (;@6;)
                  br 2 (;@5;)
                end
                i32.const 1
                local.set 2
                i32.const 3
                local.set 5
                local.get 3
                i32.const 1051480
                i32.eq
                br_if 1 (;@5;)
                local.get 3
                i32.load align=1
                i32.const 1819047270
                i32.eq
                br_if 1 (;@5;)
              end
              i32.const 0
              local.set 2
              i32.const 2
              local.set 5
            end
            local.get 4
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            local.get 4
            i32.const 1
            call $__rust_dealloc
          end
          i32.const 0
          i32.const 1
          local.get 5
          local.get 2
          i32.const 5
          i32.eq
          local.tee 3
          select
          i32.store offset=1058764
          i32.const 4
          local.get 2
          local.get 3
          select
          local.set 2
          br 2 (;@1;)
        end
        i32.const 4
        local.set 2
        br 1 (;@1;)
      end
      i32.const 0
      local.set 2
    end
    local.get 1
    local.get 2
    i32.store8 offset=35
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          call $_ZN4core5panic9PanicInfo8location17h289acf84f373c045E
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          local.get 2
          i32.store offset=36
          local.get 1
          i32.const 24
          i32.add
          local.get 0
          call $_ZN4core5panic8Location4file17h70cec7a74da50847E
          local.get 1
          i32.load offset=24
          local.tee 2
          local.get 1
          i32.load offset=28
          i32.load offset=12
          call_indirect (type 2)
          local.set 6
          block  ;; label = @4
            local.get 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            i64.const 9147559743429524724
            i64.eq
            br_if 2 (;@2;)
          end
          local.get 1
          i32.const 16
          i32.add
          local.get 0
          call $_ZN4core5panic8Location4file17h70cec7a74da50847E
          i32.const 8
          local.set 0
          i32.const 1051884
          local.set 3
          local.get 1
          i32.load offset=16
          local.tee 2
          local.get 1
          i32.load offset=20
          i32.load offset=12
          call_indirect (type 2)
          local.set 6
          block  ;; label = @4
            local.get 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            i64.const -2432489959605628846
            i64.ne
            br_if 0 (;@4;)
            local.get 2
            i32.load offset=8
            local.set 0
            local.get 2
            i32.load
            local.set 3
          end
          local.get 1
          local.get 3
          i32.store offset=40
          br 2 (;@1;)
        end
        i32.const 1049268
        i32.const 43
        i32.const 1051868
        call $_ZN4core9panicking5panic17heeaec3885c636092E
        unreachable
      end
      local.get 1
      local.get 2
      i32.load
      i32.store offset=40
      local.get 2
      i32.load offset=4
      local.set 0
    end
    local.get 1
    local.get 0
    i32.store offset=44
    i32.const 0
    local.set 0
    block  ;; label = @1
      i32.const 0
      i32.load offset=1058824
      i32.const 1
      i32.eq
      br_if 0 (;@1;)
      i32.const 0
      i64.const 1
      i64.store offset=1058824 align=4
      i32.const 0
      i32.const 0
      i32.store offset=1058832
    end
    local.get 1
    i32.const 1058828
    call $_ZN3std10sys_common11thread_info10ThreadInfo4with28_$u7b$$u7b$closure$u7d$$u7d$17h74ef4e5ac2243bc9E
    local.tee 2
    i32.store offset=52
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.load offset=16
          local.tee 3
          br_if 0 (;@3;)
          br 1 (;@2;)
        end
        local.get 2
        i32.const 16
        i32.add
        i32.const 0
        local.get 3
        select
        local.tee 0
        i32.load offset=4
        local.tee 5
        i32.const -1
        i32.add
        local.set 3
        local.get 5
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.set 0
      end
      local.get 1
      local.get 3
      i32.const 9
      local.get 0
      select
      i32.store offset=60
      local.get 1
      local.get 0
      i32.const 1051892
      local.get 0
      select
      i32.store offset=56
      local.get 1
      local.get 1
      i32.const 35
      i32.add
      i32.store offset=76
      local.get 1
      local.get 1
      i32.const 36
      i32.add
      i32.store offset=72
      local.get 1
      local.get 1
      i32.const 40
      i32.add
      i32.store offset=68
      local.get 1
      local.get 1
      i32.const 56
      i32.add
      i32.store offset=64
      i32.const 0
      local.set 5
      local.get 1
      i32.const 8
      i32.add
      i32.const 0
      local.get 1
      call $_ZN3std2io5stdio9set_panic17hdc080de43bd7c368E
      local.get 1
      i32.load offset=12
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.load offset=8
          local.tee 0
          br_if 0 (;@3;)
          local.get 1
          i32.const 64
          i32.add
          local.get 1
          i32.const 88
          i32.add
          i32.const 1051904
          call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h7aaaaa870ab86598E
          br 1 (;@2;)
        end
        local.get 1
        local.get 3
        i32.store offset=84
        local.get 1
        local.get 0
        i32.store offset=80
        local.get 1
        i32.const 64
        i32.add
        local.get 1
        i32.const 80
        i32.add
        i32.const 1051948
        call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h7aaaaa870ab86598E
        local.get 1
        local.get 1
        i32.load offset=80
        local.get 1
        i32.load offset=84
        call $_ZN3std2io5stdio9set_panic17hdc080de43bd7c368E
        block  ;; label = @3
          local.get 1
          i32.load
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 5
          local.get 1
          i32.load offset=4
          local.tee 4
          i32.load
          call_indirect (type 1)
          local.get 4
          i32.load offset=4
          local.tee 7
          i32.eqz
          br_if 0 (;@3;)
          local.get 5
          local.get 7
          local.get 4
          i32.load offset=8
          call $__rust_dealloc
        end
        i32.const 1
        local.set 5
      end
      local.get 2
      local.get 2
      i32.load
      local.tee 4
      i32.const -1
      i32.add
      i32.store
      block  ;; label = @2
        local.get 4
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 1
        i32.const 52
        i32.add
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
      end
      block  ;; label = @2
        local.get 0
        i32.const 0
        i32.ne
        local.get 5
        i32.const 1
        i32.xor
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 3
        i32.load
        call_indirect (type 1)
        local.get 3
        i32.load offset=4
        local.tee 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 2
        local.get 3
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 1
      i32.const 96
      i32.add
      global.set 0
      return
    end
    local.get 3
    i32.const 0
    i32.const 1050084
    call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
    unreachable)
  (func $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 2
    call $_ZN4core5panic8Location6caller17h071f67e970dcf83bE
    i32.store offset=8
    local.get 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    call $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17h5116a6a256f0a4bcE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h89bb37873cac3f06E (type 6) (param i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.load
    i32.store offset=12
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $_ZN3std4sync4once4Once9call_once28_$u7b$$u7b$closure$u7d$$u7d$17h707f5a9b7e72888fE
    local.get 2
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN3std4sync4once4Once9call_once28_$u7b$$u7b$closure$u7d$$u7d$17h707f5a9b7e72888fE (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    local.get 0
    i32.load
    local.tee 0
    i32.load8_u
    local.set 2
    local.get 0
    i32.const 0
    i32.store8
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.const 1
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1
        local.set 3
        loop  ;; label = @3
          i32.const 0
          i32.load8_u offset=1058849
          local.set 0
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                i32.const 9
                i32.gt_u
                br_if 0 (;@6;)
                local.get 0
                i32.const 1
                i32.and
                i32.eqz
                br_if 1 (;@5;)
                br 5 (;@1;)
              end
              i32.const 1
              local.set 4
              local.get 0
              i32.const 1
              i32.and
              br_if 4 (;@1;)
              i32.const 10
              local.set 3
              br 1 (;@4;)
            end
            local.get 3
            i32.const 1
            i32.add
            local.set 3
            i32.const 0
            local.set 4
          end
          i32.const 0
          i32.load offset=1058760
          local.set 5
          i32.const 0
          local.get 4
          i32.store offset=1058760
          i32.const 0
          i32.const 0
          i32.store8 offset=1058849
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 5
                br_table 2 (;@4;) 0 (;@6;) 1 (;@5;)
              end
              i32.const 1051268
              i32.const 31
              i32.const 1051340
              call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
              unreachable
            end
            local.get 5
            i32.load
            local.tee 6
            local.get 5
            i32.load offset=8
            local.tee 2
            i32.const 3
            i32.shl
            i32.add
            local.set 7
            local.get 5
            i32.load offset=4
            local.set 8
            local.get 6
            local.set 0
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.eqz
                br_if 0 (;@6;)
                local.get 6
                local.set 0
                loop  ;; label = @7
                  block  ;; label = @8
                    local.get 0
                    i32.load
                    local.tee 2
                    br_if 0 (;@8;)
                    local.get 0
                    i32.const 8
                    i32.add
                    local.set 0
                    br 2 (;@6;)
                  end
                  local.get 2
                  local.get 0
                  i32.const 4
                  i32.add
                  i32.load
                  local.tee 9
                  i32.load offset=12
                  call_indirect (type 1)
                  block  ;; label = @8
                    local.get 9
                    i32.load offset=4
                    local.tee 10
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 2
                    local.get 10
                    local.get 9
                    i32.load offset=8
                    call $__rust_dealloc
                  end
                  local.get 0
                  i32.const 8
                  i32.add
                  local.tee 0
                  local.get 7
                  i32.ne
                  br_if 0 (;@7;)
                  br 2 (;@5;)
                end
              end
              local.get 7
              local.get 0
              i32.eq
              br_if 0 (;@5;)
              loop  ;; label = @6
                local.get 0
                i32.load
                local.get 0
                i32.const 4
                i32.add
                local.tee 2
                i32.load
                i32.load
                call_indirect (type 1)
                block  ;; label = @7
                  local.get 2
                  i32.load
                  local.tee 2
                  i32.load offset=4
                  local.tee 9
                  i32.eqz
                  br_if 0 (;@7;)
                  local.get 0
                  i32.load
                  local.get 9
                  local.get 2
                  i32.load offset=8
                  call $__rust_dealloc
                end
                local.get 0
                i32.const 8
                i32.add
                local.tee 0
                local.get 7
                i32.ne
                br_if 0 (;@6;)
              end
            end
            block  ;; label = @5
              local.get 8
              i32.eqz
              br_if 0 (;@5;)
              local.get 8
              i32.const 3
              i32.shl
              local.tee 0
              i32.eqz
              br_if 0 (;@5;)
              local.get 6
              local.get 0
              i32.const 4
              call $__rust_dealloc
            end
            local.get 5
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          local.get 3
          i32.const 11
          i32.lt_u
          local.get 4
          i32.const 1
          i32.xor
          i32.and
          br_if 0 (;@3;)
        end
        return
      end
      i32.const 1049268
      i32.const 43
      i32.const 1051084
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    i32.const 1052596
    i32.const 32
    i32.const 1052676
    call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
    unreachable)
  (func $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h8f451917953880c1E (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      local.get 0
      i32.const 0
      i32.store8 offset=4
      local.get 0
      i32.load
      local.set 1
      local.get 0
      i32.const 1
      i32.store
      local.get 1
      i32.load
      local.tee 0
      local.get 0
      i32.load
      local.tee 0
      i32.const -1
      i32.add
      i32.store
      block  ;; label = @2
        local.get 0
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 1
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h5481941dbae3eca9E
      end
      local.get 1
      i32.const 4
      i32.const 4
      call $__rust_dealloc
      return
    end
    i32.const 1052596
    i32.const 32
    i32.const 1052676
    call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
    unreachable)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h5481941dbae3eca9E (type 1) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 2
      i32.load8_u offset=24
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      i32.const 25
      i32.add
      i32.load8_u
      i32.const 255
      i32.and
      br_if 0 (;@1;)
      local.get 1
      i32.const 8
      i32.add
      local.get 2
      i32.const 12
      i32.add
      call $_ZN3std2io8buffered18BufWriter$LT$W$GT$9flush_buf17h0d5bd5c28054fc9aE
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        local.get 1
        i32.load8_u offset=8
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      local.get 1
      i32.load offset=12
      local.tee 3
      i32.load
      local.get 3
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 3
        i32.load offset=4
        local.tee 4
        i32.load offset=4
        local.tee 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        i32.load
        local.get 5
        local.get 4
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 3
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    block  ;; label = @1
      local.get 2
      i32.load offset=12
      local.tee 3
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      i32.const 16
      i32.add
      i32.load
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      local.get 2
      i32.const 1
      call $__rust_dealloc
    end
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 2
      i32.const -1
      i32.eq
      br_if 0 (;@1;)
      local.get 2
      local.get 2
      i32.load offset=4
      local.tee 0
      i32.const -1
      i32.add
      i32.store offset=4
      local.get 0
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      local.get 2
      i32.const 32
      i32.const 4
      call $__rust_dealloc
    end
    local.get 1
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN4core3ptr13drop_in_place17h01aa9f0df2a3d41bE (type 1) (param i32))
  (func $_ZN4core3ptr13drop_in_place17h0efdcecac9224e6cE (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      local.get 0
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr13drop_in_place17h35bf666c94b799f0E (type 1) (param i32)
    (local i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        local.get 0
        i32.load8_u offset=4
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      local.get 0
      i32.const 8
      i32.add
      i32.load
      local.tee 1
      i32.load
      local.get 1
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 1
        i32.load offset=4
        local.tee 2
        i32.load offset=4
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load
        local.get 3
        local.get 2
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 0
      i32.load offset=8
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr13drop_in_place17h43e1c981bf433273E (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      i32.const 0
      i32.load offset=1058788
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load
      local.set 1
      call $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE
      br_if 0 (;@1;)
      local.get 1
      i32.const 1
      i32.store8 offset=4
    end
    local.get 0
    i32.load
    i32.load
    i32.const 0
    i32.store8)
  (func $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE (type 10) (result i32)
    block  ;; label = @1
      i32.const 0
      i32.load offset=1058840
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      i32.const 0
      i32.load offset=1058844
      i32.eqz
      return
    end
    i32.const 0
    i64.const 1
    i64.store offset=1058840
    i32.const 1)
  (func $_ZN4core3ptr13drop_in_place17h461604a72e03a0f3E (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load offset=4
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 8
      i32.add
      i32.load
      local.tee 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      local.get 0
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr13drop_in_place17h722a6486d237df62E (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load offset=4
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 8
      i32.add
      i32.load
      local.tee 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      local.get 0
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr13drop_in_place17he59ca02f76ff0db5E (type 1) (param i32)
    (local i32 i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    i32.load
    call_indirect (type 1)
    block  ;; label = @1
      local.get 0
      i32.load offset=4
      local.tee 1
      i32.load offset=4
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load
      local.get 2
      local.get 1
      i32.load offset=8
      call $__rust_dealloc
    end)
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17h8e11d4729ddc2cb0E (type 5) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      i32.const 1049268
      i32.const 43
      i32.const 1052156
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    local.get 0)
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17hff9de88d0e2f6d3eE (type 3) (param i32 i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      i32.const 1049268
      i32.const 43
      local.get 1
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    local.get 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h37a7d86d4a86eeb1E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 1
                  i32.const 128
                  i32.lt_u
                  br_if 0 (;@7;)
                  local.get 2
                  i32.const 0
                  i32.store offset=12
                  local.get 1
                  i32.const 2048
                  i32.lt_u
                  br_if 1 (;@6;)
                  local.get 2
                  i32.const 12
                  i32.add
                  local.set 3
                  block  ;; label = @8
                    local.get 1
                    i32.const 65536
                    i32.ge_u
                    br_if 0 (;@8;)
                    local.get 2
                    local.get 1
                    i32.const 63
                    i32.and
                    i32.const 128
                    i32.or
                    i32.store8 offset=14
                    local.get 2
                    local.get 1
                    i32.const 12
                    i32.shr_u
                    i32.const 224
                    i32.or
                    i32.store8 offset=12
                    local.get 2
                    local.get 1
                    i32.const 6
                    i32.shr_u
                    i32.const 63
                    i32.and
                    i32.const 128
                    i32.or
                    i32.store8 offset=13
                    i32.const 3
                    local.set 1
                    br 6 (;@2;)
                  end
                  local.get 2
                  local.get 1
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=15
                  local.get 2
                  local.get 1
                  i32.const 18
                  i32.shr_u
                  i32.const 240
                  i32.or
                  i32.store8 offset=12
                  local.get 2
                  local.get 1
                  i32.const 6
                  i32.shr_u
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=14
                  local.get 2
                  local.get 1
                  i32.const 12
                  i32.shr_u
                  i32.const 63
                  i32.and
                  i32.const 128
                  i32.or
                  i32.store8 offset=13
                  i32.const 4
                  local.set 1
                  br 5 (;@2;)
                end
                block  ;; label = @7
                  local.get 0
                  i32.load offset=8
                  local.tee 3
                  local.get 0
                  i32.const 4
                  i32.add
                  i32.load
                  i32.eq
                  br_if 0 (;@7;)
                  local.get 0
                  i32.load
                  local.set 4
                  br 4 (;@3;)
                end
                block  ;; label = @7
                  local.get 3
                  i32.const 1
                  i32.add
                  local.tee 4
                  local.get 3
                  i32.lt_u
                  br_if 0 (;@7;)
                  local.get 3
                  i32.const 1
                  i32.shl
                  local.tee 5
                  local.get 4
                  local.get 5
                  local.get 4
                  i32.gt_u
                  select
                  local.tee 4
                  i32.const 8
                  local.get 4
                  i32.const 8
                  i32.gt_u
                  select
                  local.set 5
                  block  ;; label = @8
                    local.get 3
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 5
                    i32.const 0
                    i32.lt_s
                    br_if 1 (;@7;)
                    local.get 0
                    i32.load
                    local.tee 4
                    i32.eqz
                    br_if 3 (;@5;)
                    local.get 4
                    local.get 3
                    i32.const 1
                    local.get 5
                    call $__rust_realloc
                    local.set 4
                    br 4 (;@4;)
                  end
                  local.get 5
                  i32.const 0
                  i32.ge_s
                  br_if 2 (;@5;)
                end
                call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
                unreachable
              end
              local.get 2
              local.get 1
              i32.const 63
              i32.and
              i32.const 128
              i32.or
              i32.store8 offset=13
              local.get 2
              local.get 1
              i32.const 6
              i32.shr_u
              i32.const 192
              i32.or
              i32.store8 offset=12
              local.get 2
              i32.const 12
              i32.add
              local.set 3
              i32.const 2
              local.set 1
              br 3 (;@2;)
            end
            local.get 5
            i32.const 1
            call $__rust_alloc
            local.set 4
          end
          block  ;; label = @4
            local.get 4
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            local.get 4
            i32.store
            local.get 0
            i32.const 4
            i32.add
            local.get 5
            i32.store
            local.get 0
            i32.load offset=8
            local.set 3
            br 1 (;@3;)
          end
          local.get 5
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        local.get 4
        local.get 3
        i32.add
        local.get 1
        i32.store8
        local.get 0
        local.get 0
        i32.load offset=8
        i32.const 1
        i32.add
        i32.store offset=8
        br 1 (;@1;)
      end
      local.get 0
      local.get 3
      local.get 1
      call $_ZN5alloc3vec12Vec$LT$T$GT$17extend_from_slice17hd567b84a391d6697E
    end
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    i32.const 0)
  (func $_ZN5alloc3vec12Vec$LT$T$GT$17extend_from_slice17hd567b84a391d6697E (type 7) (param i32 i32 i32)
    (local i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 4
        i32.add
        i32.load
        local.tee 3
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 4
        i32.sub
        local.get 2
        i32.lt_u
        br_if 0 (;@2;)
        local.get 0
        i32.load
        local.set 3
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 4
            local.get 2
            i32.add
            local.tee 5
            local.get 4
            i32.lt_u
            br_if 0 (;@4;)
            local.get 3
            i32.const 1
            i32.shl
            local.tee 4
            local.get 5
            local.get 4
            local.get 5
            i32.gt_u
            select
            local.tee 4
            i32.const 8
            local.get 4
            i32.const 8
            i32.gt_u
            select
            local.set 4
            block  ;; label = @5
              local.get 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 4
              i32.const 0
              i32.lt_s
              br_if 1 (;@4;)
              local.get 0
              i32.load
              local.tee 5
              i32.eqz
              br_if 2 (;@3;)
              local.get 5
              local.get 3
              i32.const 1
              local.get 4
              call $__rust_realloc
              local.set 3
              br 3 (;@2;)
            end
            local.get 4
            i32.const 0
            i32.ge_s
            br_if 1 (;@3;)
          end
          call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
          unreachable
        end
        local.get 4
        i32.const 1
        call $__rust_alloc
        local.set 3
      end
      block  ;; label = @2
        local.get 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 3
        i32.store
        local.get 0
        i32.const 4
        i32.add
        local.get 4
        i32.store
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.set 4
        br 1 (;@1;)
      end
      local.get 4
      i32.const 1
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 3
    local.get 4
    i32.add
    local.get 1
    local.get 2
    call $memcpy
    drop
    local.get 0
    i32.const 8
    i32.add
    local.get 4
    local.get 2
    i32.add
    i32.store)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h878a07cb96977ed8E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt5Write10write_char17hae5a84beb8d1055fE)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h99f2437e301ed84bE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt5Write10write_char17h524458e201538bfcE)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h07cd168c2ba71c1aE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.load
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1049040
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h2383e42b263403e5E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.load
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1049064
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h9ca5c664748f585bE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.load
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1049088
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5a6e30684d6b4715E (type 8) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN5alloc3vec12Vec$LT$T$GT$17extend_from_slice17hd567b84a391d6697E
    i32.const 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h839b33d7928966c7E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.tee 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h34620f531d384434E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 2
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 2
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 3
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17hfde7181541e0be0aE (type 8) (param i32 i32 i32) (result i32)
    (local i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.tee 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN3std2io5Write9write_all17h95d942a560570f98E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 2
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 2
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 3
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE (type 1) (param i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 1
      i32.load offset=16
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      i32.const 0
      i32.store8
      local.get 1
      i32.load offset=20
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.load offset=16
      local.get 2
      i32.const 1
      call $__rust_dealloc
    end
    local.get 1
    i32.load offset=28
    i32.const 1
    i32.const 1
    call $__rust_dealloc
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 1
      i32.const -1
      i32.eq
      br_if 0 (;@1;)
      local.get 1
      local.get 1
      i32.load offset=4
      local.tee 0
      i32.const -1
      i32.add
      i32.store offset=4
      local.get 0
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      local.get 1
      i32.const 48
      i32.const 8
      call $__rust_dealloc
    end)
  (func $_ZN3std2io8buffered18BufWriter$LT$W$GT$9flush_buf17h0d5bd5c28054fc9aE (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.const 3
    i32.store8
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 8
        i32.add
        local.tee 3
        i32.load
        local.tee 4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 0
        local.set 5
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    loop  ;; label = @9
                      local.get 1
                      i32.const 1
                      i32.store8 offset=13
                      local.get 1
                      i32.load8_u offset=12
                      i32.const 1
                      i32.ne
                      br_if 1 (;@8;)
                      local.get 3
                      i32.load
                      local.tee 6
                      local.get 5
                      i32.lt_u
                      br_if 2 (;@7;)
                      local.get 1
                      i32.load
                      local.set 7
                      local.get 2
                      local.get 6
                      local.get 5
                      i32.sub
                      local.tee 6
                      i32.store offset=12
                      local.get 2
                      local.get 7
                      local.get 5
                      i32.add
                      i32.store offset=8
                      local.get 2
                      i32.const 16
                      i32.add
                      i32.const 1
                      local.get 2
                      i32.const 8
                      i32.add
                      i32.const 1
                      call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                local.get 2
                                i32.load16_u offset=16
                                i32.const 1
                                i32.ne
                                br_if 0 (;@14;)
                                local.get 2
                                local.get 2
                                i32.load16_u offset=18
                                i32.store16 offset=30
                                local.get 2
                                i32.const 30
                                i32.add
                                call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
                                local.tee 8
                                i32.const 65535
                                i32.and
                                local.tee 7
                                i32.const 8
                                i32.eq
                                br_if 1 (;@13;)
                                local.get 1
                                i32.const 0
                                i32.store8 offset=13
                                local.get 7
                                call $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E
                                i32.const 255
                                i32.and
                                i32.const 15
                                i32.eq
                                br_if 4 (;@10;)
                                local.get 8
                                i32.const 65535
                                i32.and
                                local.set 6
                                i32.const 0
                                local.set 7
                                br 2 (;@12;)
                              end
                              local.get 2
                              i32.load offset=20
                              local.set 6
                            end
                            local.get 1
                            i32.const 0
                            i32.store8 offset=13
                            local.get 6
                            br_if 1 (;@11;)
                            i32.const 33
                            i32.const 1
                            call $__rust_alloc
                            local.tee 6
                            i32.eqz
                            br_if 6 (;@6;)
                            local.get 6
                            i32.const 32
                            i32.add
                            i32.const 0
                            i32.load8_u offset=1050196
                            i32.store8
                            local.get 6
                            i32.const 24
                            i32.add
                            i32.const 0
                            i64.load offset=1050188 align=1
                            i64.store align=1
                            local.get 6
                            i32.const 16
                            i32.add
                            i32.const 0
                            i64.load offset=1050180 align=1
                            i64.store align=1
                            local.get 6
                            i32.const 8
                            i32.add
                            i32.const 0
                            i64.load offset=1050172 align=1
                            i64.store align=1
                            local.get 6
                            i32.const 0
                            i64.load offset=1050164 align=1
                            i64.store align=1
                            i32.const 12
                            i32.const 4
                            call $__rust_alloc
                            local.tee 7
                            i32.eqz
                            br_if 7 (;@5;)
                            local.get 7
                            i64.const 141733920801
                            i64.store offset=4 align=4
                            local.get 7
                            local.get 6
                            i32.store
                            i32.const 12
                            i32.const 4
                            call $__rust_alloc
                            local.tee 6
                            i32.eqz
                            br_if 8 (;@4;)
                            local.get 6
                            i32.const 14
                            i32.store8 offset=8
                            local.get 6
                            i32.const 1049980
                            i32.store offset=4
                            local.get 6
                            local.get 7
                            i32.store
                            local.get 6
                            local.get 2
                            i32.load16_u offset=16 align=1
                            i32.store16 offset=9 align=1
                            i32.const 2
                            local.set 7
                            local.get 6
                            i32.const 11
                            i32.add
                            local.get 2
                            i32.const 16
                            i32.add
                            i32.const 2
                            i32.add
                            i32.load8_u
                            i32.store8
                          end
                          local.get 0
                          local.get 7
                          i32.store
                          local.get 0
                          i32.const 4
                          i32.add
                          local.get 6
                          i32.store
                          br 8 (;@3;)
                        end
                        local.get 6
                        local.get 5
                        i32.add
                        local.set 5
                      end
                      local.get 5
                      local.get 4
                      i32.lt_u
                      br_if 0 (;@9;)
                      br 6 (;@3;)
                    end
                  end
                  i32.const 1049268
                  i32.const 43
                  i32.const 1050132
                  call $_ZN4core9panicking5panic17heeaec3885c636092E
                  unreachable
                end
                local.get 5
                local.get 6
                i32.const 1050148
                call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
                unreachable
              end
              i32.const 33
              i32.const 1
              call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
              unreachable
            end
            i32.const 12
            i32.const 4
            call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
            unreachable
          end
          i32.const 12
          i32.const 4
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        local.get 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.const 8
        i32.add
        local.tee 6
        i32.load
        local.tee 7
        local.get 5
        i32.lt_u
        br_if 1 (;@1;)
        local.get 6
        i32.const 0
        i32.store
        local.get 7
        local.get 5
        i32.sub
        local.tee 6
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load
        local.tee 7
        local.get 7
        local.get 5
        i32.add
        local.get 6
        call $memmove
        drop
        local.get 1
        i32.const 8
        i32.add
        local.get 6
        i32.store
      end
      local.get 2
      i32.const 32
      i32.add
      global.set 0
      return
    end
    local.get 5
    local.get 7
    call $_ZN5alloc3vec12Vec$LT$T$GT$5drain17end_assert_failed17hc6f56e66601fd97cE
    unreachable)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17he76d4dd82f8e989eE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=8
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hce18dc51dfa14637E)
  (func $_ZN3std10sys_common11thread_info10ThreadInfo4with28_$u7b$$u7b$closure$u7d$$u7d$17h74ef4e5ac2243bc9E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.load
            local.tee 2
            i32.const 1
            i32.add
            i32.const 0
            i32.le_s
            br_if 0 (;@4;)
            local.get 0
            local.get 2
            i32.store
            block  ;; label = @5
              local.get 0
              i32.load offset=4
              local.tee 3
              br_if 0 (;@5;)
              local.get 1
              i32.const 0
              i32.store offset=8
              local.get 1
              i32.const 8
              i32.add
              call $_ZN3std6thread6Thread3new17hd3000777d889ebe5E
              local.set 3
              local.get 0
              i32.load
              br_if 2 (;@3;)
              local.get 0
              i32.const -1
              i32.store
              block  ;; label = @6
                local.get 0
                i32.load offset=4
                local.tee 2
                i32.eqz
                br_if 0 (;@6;)
                local.get 2
                local.get 2
                i32.load
                local.tee 4
                i32.const -1
                i32.add
                i32.store
                local.get 4
                i32.const 1
                i32.ne
                br_if 0 (;@6;)
                local.get 0
                i32.const 4
                i32.add
                call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
              end
              local.get 0
              local.get 3
              i32.store offset=4
              local.get 0
              local.get 0
              i32.load
              i32.const 1
              i32.add
              local.tee 2
              i32.store
            end
            local.get 2
            br_if 2 (;@2;)
            local.get 0
            i32.const -1
            i32.store
            local.get 3
            local.get 3
            i32.load
            local.tee 2
            i32.const 1
            i32.add
            i32.store
            local.get 2
            i32.const -1
            i32.le_s
            br_if 3 (;@1;)
            local.get 0
            local.get 0
            i32.load
            i32.const 1
            i32.add
            i32.store
            local.get 1
            i32.const 32
            i32.add
            global.set 0
            local.get 3
            return
          end
          i32.const 1049144
          i32.const 24
          local.get 1
          i32.const 24
          i32.add
          i32.const 1049312
          i32.const 1051596
          call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
          unreachable
        end
        i32.const 1049128
        i32.const 16
        local.get 1
        i32.const 24
        i32.add
        i32.const 1049328
        i32.const 1051612
        call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
        unreachable
      end
      i32.const 1049128
      i32.const 16
      local.get 1
      i32.const 24
      i32.add
      i32.const 1049328
      i32.const 1051628
      call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
      unreachable
    end
    unreachable
    unreachable)
  (func $_ZN3std6thread4park17hed7c6d597e3e40eaE (type 0)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 96
    i32.sub
    local.tee 0
    global.set 0
    block  ;; label = @1
      i32.const 0
      i32.load offset=1058824
      i32.const 1
      i32.eq
      br_if 0 (;@1;)
      i32.const 0
      i64.const 1
      i64.store offset=1058824 align=4
      i32.const 0
      i32.const 0
      i32.store offset=1058832
    end
    i32.const 1058828
    call $_ZN3std10sys_common11thread_info10ThreadInfo4with28_$u7b$$u7b$closure$u7d$$u7d$17h74ef4e5ac2243bc9E
    local.tee 1
    i32.const 0
    local.get 1
    i32.load offset=24
    local.tee 2
    local.get 2
    i32.const 2
    i32.eq
    local.tee 2
    select
    i32.store offset=24
    local.get 0
    local.get 1
    i32.store offset=8
    block  ;; label = @1
      local.get 2
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load offset=8
              local.tee 1
              i32.const 28
              i32.add
              local.tee 3
              i32.load
              local.tee 2
              i32.load8_u
              br_if 0 (;@5;)
              local.get 2
              i32.const 1
              i32.store8
              i32.const 0
              local.set 4
              block  ;; label = @6
                i32.const 0
                i32.load offset=1058788
                i32.eqz
                br_if 0 (;@6;)
                call $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE
                i32.const 1
                i32.xor
                local.set 4
              end
              local.get 1
              i32.load8_u offset=32
              br_if 1 (;@4;)
              local.get 1
              local.get 1
              i32.load offset=24
              local.tee 2
              i32.const 1
              local.get 2
              select
              i32.store offset=24
              block  ;; label = @6
                local.get 2
                br_if 0 (;@6;)
                local.get 0
                i32.load offset=8
                i32.const 36
                i32.add
                local.get 3
                i32.load
                call $_ZN3std4sync7condvar7Condvar6verify17hcf1d29122829d29fE
                call $_ZN3std10sys_common7condvar7Condvar4wait17h6394f43212c3566cE
                unreachable
              end
              local.get 2
              i32.const 2
              i32.ne
              br_if 2 (;@3;)
              local.get 0
              i32.load offset=8
              local.tee 5
              i32.load offset=24
              local.set 2
              local.get 5
              i32.const 0
              i32.store offset=24
              local.get 0
              local.get 2
              i32.store offset=12
              local.get 2
              i32.const 2
              i32.ne
              br_if 3 (;@2;)
              block  ;; label = @6
                local.get 4
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1058788
                i32.eqz
                br_if 0 (;@6;)
                call $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE
                br_if 0 (;@6;)
                local.get 1
                i32.const 1
                i32.store8 offset=32
              end
              local.get 3
              i32.load
              i32.const 0
              i32.store8
              br 4 (;@1;)
            end
            i32.const 1052596
            i32.const 32
            i32.const 1052676
            call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
            unreachable
          end
          local.get 0
          local.get 4
          i32.store8 offset=76
          local.get 0
          local.get 3
          i32.store offset=72
          i32.const 1049360
          i32.const 43
          local.get 0
          i32.const 72
          i32.add
          i32.const 1049404
          i32.const 1049500
          call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
          unreachable
        end
        i32.const 1049516
        i32.const 23
        i32.const 1049540
        call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
        unreachable
      end
      local.get 0
      i32.const 40
      i32.add
      i32.const 20
      i32.add
      i32.const 10
      i32.store
      local.get 0
      i32.const 52
      i32.add
      i32.const 11
      i32.store
      local.get 0
      i32.const 16
      i32.add
      i32.const 20
      i32.add
      i32.const 3
      i32.store
      local.get 0
      local.get 0
      i32.const 12
      i32.add
      i32.store offset=64
      local.get 0
      i32.const 1049556
      i32.store offset=68
      local.get 0
      i32.const 72
      i32.add
      i32.const 20
      i32.add
      i32.const 0
      i32.store
      local.get 0
      i64.const 3
      i64.store offset=20 align=4
      local.get 0
      i32.const 1049564
      i32.store offset=16
      local.get 0
      i32.const 11
      i32.store offset=44
      local.get 0
      i32.const 1049252
      i32.store offset=88
      local.get 0
      i64.const 1
      i64.store offset=76 align=4
      local.get 0
      i32.const 1049620
      i32.store offset=72
      local.get 0
      local.get 0
      i32.const 40
      i32.add
      i32.store offset=32
      local.get 0
      local.get 0
      i32.const 72
      i32.add
      i32.store offset=56
      local.get 0
      local.get 0
      i32.const 68
      i32.add
      i32.store offset=48
      local.get 0
      local.get 0
      i32.const 64
      i32.add
      i32.store offset=40
      local.get 0
      i32.const 16
      i32.add
      i32.const 1049628
      call $_ZN3std9panicking15begin_panic_fmt17h0c385246197edc82E
      unreachable
    end
    local.get 0
    i32.load offset=8
    local.tee 1
    local.get 1
    i32.load
    local.tee 1
    i32.const -1
    i32.add
    i32.store
    block  ;; label = @1
      local.get 1
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      local.get 0
      i32.const 8
      i32.add
      call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
    end
    local.get 0
    i32.const 96
    i32.add
    global.set 0)
  (func $_ZN3std4sync7condvar7Condvar6verify17hcf1d29122829d29fE (type 6) (param i32 i32)
    (local i32)
    local.get 0
    local.get 0
    i32.load offset=4
    local.tee 2
    local.get 1
    local.get 2
    select
    i32.store offset=4
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      local.get 1
      i32.eq
      br_if 0 (;@1;)
      i32.const 1050932
      i32.const 54
      i32.const 1051020
      call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
      unreachable
    end)
  (func $_ZN3std10sys_common7condvar7Condvar4wait17h6394f43212c3566cE (type 0)
    (local i32)
    local.get 0
    local.get 0
    call $_ZN3std3sys4wasi7condvar7Condvar4wait17h30ac79ba8be4b6e9E
    unreachable)
  (func $_ZN3std9panicking15begin_panic_fmt17h0c385246197edc82E (type 6) (param i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 1
    call $_ZN4core5panic8Location6caller17h071f67e970dcf83bE
    i32.store offset=12
    local.get 2
    local.get 0
    i32.store offset=8
    local.get 2
    i32.const 1049252
    i32.store offset=4
    local.get 2
    i32.const 1049252
    i32.store
    local.get 2
    call $rust_begin_unwind
    unreachable)
  (func $_ZN3std6thread6Thread3new17hd3000777d889ebe5E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i64)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.load
                local.tee 2
                br_if 0 (;@6;)
                i32.const 0
                local.set 3
                br 1 (;@5;)
              end
              local.get 1
              local.get 0
              i64.load offset=4 align=4
              i64.store offset=36 align=4
              local.get 1
              local.get 2
              i32.store offset=32
              local.get 1
              i32.const 16
              i32.add
              local.get 1
              i32.const 32
              i32.add
              call $_ZN5alloc6string104_$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..vec..Vec$LT$u8$GT$$GT$4from17h4cb8310fd36b5288E
              local.get 1
              i32.const 8
              i32.add
              i32.const 0
              local.get 1
              i32.load offset=16
              local.tee 0
              local.get 1
              i32.load offset=24
              call $_ZN4core5slice6memchr6memchr17h55b5fdbafce1735dE
              local.get 1
              i32.load offset=8
              br_if 1 (;@4;)
              local.get 1
              i32.const 32
              i32.add
              i32.const 8
              i32.add
              local.get 1
              i32.const 16
              i32.add
              i32.const 8
              i32.add
              i32.load
              i32.store
              local.get 1
              local.get 1
              i64.load offset=16
              i64.store offset=32
              local.get 1
              local.get 1
              i32.const 32
              i32.add
              call $_ZN3std3ffi5c_str7CString18from_vec_unchecked17h90197a6ae9ab1825E
              local.get 1
              i32.load offset=4
              local.set 4
              local.get 1
              i32.load
              local.set 3
            end
            i32.const 0
            i32.load8_u offset=1058848
            br_if 1 (;@3;)
            i32.const 0
            i32.const 1
            i32.store8 offset=1058848
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i64.load offset=1058736
                local.tee 5
                i64.const -1
                i64.eq
                br_if 0 (;@6;)
                i32.const 0
                local.get 5
                i64.const 1
                i64.add
                i64.store offset=1058736
                local.get 5
                i64.const 0
                i64.ne
                br_if 1 (;@5;)
                i32.const 1049268
                i32.const 43
                i32.const 1049716
                call $_ZN4core9panicking5panic17heeaec3885c636092E
                unreachable
              end
              i32.const 1049644
              i32.const 55
              i32.const 1049700
              call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
              unreachable
            end
            i32.const 0
            i32.const 0
            i32.store8 offset=1058848
            i32.const 1
            i32.const 1
            call $__rust_alloc
            local.tee 2
            i32.eqz
            br_if 2 (;@2;)
            local.get 2
            i32.const 0
            i32.store8
            i32.const 48
            i32.const 8
            call $__rust_alloc
            local.tee 0
            i32.eqz
            br_if 3 (;@1;)
            local.get 0
            i64.const 1
            i64.store offset=36 align=4
            local.get 0
            i32.const 0
            i32.store offset=24
            local.get 0
            local.get 4
            i32.store offset=20
            local.get 0
            local.get 3
            i32.store offset=16
            local.get 0
            local.get 5
            i64.store offset=8
            local.get 0
            i64.const 4294967297
            i64.store
            local.get 0
            local.get 2
            i64.extend_i32_u
            i64.store offset=28 align=4
            local.get 1
            i32.const 48
            i32.add
            global.set 0
            local.get 0
            return
          end
          local.get 1
          i32.load offset=12
          local.set 2
          local.get 1
          i32.const 40
          i32.add
          local.get 1
          i64.load offset=20 align=4
          i64.store
          local.get 1
          local.get 0
          i32.store offset=36
          local.get 1
          local.get 2
          i32.store offset=32
          i32.const 1049732
          i32.const 47
          local.get 1
          i32.const 32
          i32.add
          i32.const 1049344
          i32.const 1049780
          call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
          unreachable
        end
        i32.const 1052596
        i32.const 32
        i32.const 1052676
        call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
        unreachable
      end
      i32.const 1
      i32.const 1
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    i32.const 48
    i32.const 8
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN3std3ffi5c_str7CString18from_vec_unchecked17h90197a6ae9ab1825E (type 6) (param i32 i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 4
              i32.add
              i32.load
              local.tee 2
              local.get 1
              i32.load offset=8
              local.tee 3
              i32.ne
              br_if 0 (;@5;)
              local.get 3
              i32.const 1
              i32.add
              local.tee 2
              local.get 3
              i32.lt_u
              br_if 3 (;@2;)
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 3
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 2
                    i32.const 0
                    i32.lt_s
                    br_if 6 (;@2;)
                    local.get 1
                    i32.load
                    local.tee 4
                    i32.eqz
                    br_if 1 (;@7;)
                    local.get 4
                    local.get 3
                    i32.const 1
                    local.get 2
                    call $__rust_realloc
                    local.set 4
                    br 2 (;@6;)
                  end
                  local.get 2
                  i32.const 0
                  i32.lt_s
                  br_if 5 (;@2;)
                end
                local.get 2
                i32.const 1
                call $__rust_alloc
                local.set 4
              end
              local.get 4
              i32.eqz
              br_if 1 (;@4;)
              local.get 1
              local.get 4
              i32.store
              local.get 1
              i32.const 4
              i32.add
              local.get 2
              i32.store
            end
            local.get 3
            local.get 2
            i32.eq
            br_if 1 (;@3;)
            local.get 3
            i32.const 1
            i32.add
            local.set 2
            local.get 1
            i32.load
            local.set 4
            br 3 (;@1;)
          end
          local.get 2
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        local.get 3
        i32.const 1
        i32.add
        local.tee 2
        local.get 3
        i32.lt_u
        br_if 0 (;@2;)
        local.get 3
        i32.const 1
        i32.shl
        local.tee 4
        local.get 2
        local.get 4
        local.get 2
        i32.gt_u
        select
        local.tee 4
        i32.const 8
        local.get 4
        i32.const 8
        i32.gt_u
        select
        local.set 5
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 5
              i32.const 0
              i32.lt_s
              br_if 3 (;@2;)
              local.get 1
              i32.load
              local.tee 4
              i32.eqz
              br_if 1 (;@4;)
              local.get 4
              local.get 3
              i32.const 1
              local.get 5
              call $__rust_realloc
              local.set 4
              br 2 (;@3;)
            end
            local.get 5
            i32.const 0
            i32.lt_s
            br_if 2 (;@2;)
          end
          local.get 5
          i32.const 1
          call $__rust_alloc
          local.set 4
        end
        block  ;; label = @3
          local.get 4
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          local.get 4
          i32.store
          local.get 1
          i32.const 4
          i32.add
          local.get 5
          i32.store
          br 2 (;@1;)
        end
        local.get 5
        i32.const 1
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
      unreachable
    end
    local.get 4
    local.get 3
    i32.add
    i32.const 0
    i32.store8
    local.get 1
    local.get 2
    i32.store offset=8
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 4
        i32.add
        i32.load
        local.tee 3
        local.get 2
        i32.gt_u
        br_if 0 (;@2;)
        local.get 4
        local.set 1
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 2
        br_if 0 (;@2;)
        i32.const 1
        local.set 1
        local.get 4
        local.get 3
        i32.const 1
        call $__rust_dealloc
        br 1 (;@1;)
      end
      local.get 4
      local.get 3
      i32.const 1
      local.get 2
      call $__rust_realloc
      local.tee 1
      br_if 0 (;@1;)
      local.get 2
      i32.const 1
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN3std6thread6Thread6unpark17hda97b6cd219be17eE (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 0
    i32.load
    local.tee 2
    i32.load offset=24
    local.set 3
    local.get 2
    i32.const 2
    i32.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              br_table 2 (;@3;) 1 (;@4;) 2 (;@3;) 0 (;@5;)
            end
            i32.const 1049796
            i32.const 28
            i32.const 1049824
            call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
            unreachable
          end
          local.get 0
          i32.load
          local.tee 0
          i32.const 28
          i32.add
          local.tee 2
          i32.load
          local.tee 3
          i32.load8_u
          br_if 1 (;@2;)
          local.get 3
          i32.const 1
          i32.store8
          i32.const 0
          local.set 3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1058788
                  i32.eqz
                  br_if 0 (;@7;)
                  call $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE
                  local.set 3
                  local.get 0
                  i32.load8_u offset=32
                  i32.eqz
                  br_if 1 (;@6;)
                  local.get 3
                  i32.const 1
                  i32.xor
                  local.set 3
                  br 6 (;@1;)
                end
                local.get 0
                i32.load8_u offset=32
                br_if 5 (;@1;)
                local.get 0
                i32.const 32
                i32.add
                local.set 0
                br 1 (;@5;)
              end
              local.get 3
              i32.eqz
              br_if 1 (;@4;)
              local.get 0
              i32.const 32
              i32.add
              local.set 0
            end
            i32.const 0
            i32.load offset=1058788
            i32.eqz
            br_if 0 (;@4;)
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17hd0e90234253361ceE
            br_if 0 (;@4;)
            local.get 0
            i32.const 1
            i32.store8
          end
          local.get 2
          i32.load
          i32.const 0
          i32.store8
        end
        local.get 1
        i32.const 16
        i32.add
        global.set 0
        return
      end
      i32.const 1052596
      i32.const 32
      i32.const 1052676
      call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
      unreachable
    end
    local.get 1
    local.get 3
    i32.store8 offset=12
    local.get 1
    local.get 2
    i32.store offset=8
    i32.const 1049360
    i32.const 43
    local.get 1
    i32.const 8
    i32.add
    i32.const 1049404
    i32.const 1049840
    call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
    unreachable)
  (func $_ZN3std3env11current_dir17h57d19aca547a0569E (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 40
          i32.const 1
          call $__rust_alloc
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          i32.const 32
          i32.add
          i32.const 0
          i64.load offset=1052840 align=1
          i64.store align=1
          local.get 2
          i32.const 24
          i32.add
          i32.const 0
          i64.load offset=1052832 align=1
          i64.store align=1
          local.get 2
          i32.const 16
          i32.add
          i32.const 0
          i64.load offset=1052824 align=1
          i64.store align=1
          local.get 2
          i32.const 8
          i32.add
          i32.const 0
          i64.load offset=1052816 align=1
          i64.store align=1
          local.get 2
          i32.const 0
          i64.load offset=1052808 align=1
          i64.store align=1
          i32.const 12
          i32.const 4
          call $__rust_alloc
          local.tee 3
          i32.eqz
          br_if 1 (;@2;)
          local.get 3
          i64.const 171798691880
          i64.store offset=4 align=4
          local.get 3
          local.get 2
          i32.store
          i32.const 12
          i32.const 4
          call $__rust_alloc
          local.tee 2
          i32.eqz
          br_if 2 (;@1;)
          local.get 2
          i32.const 16
          i32.store8 offset=8
          local.get 2
          i32.const 1049980
          i32.store offset=4
          local.get 2
          local.get 3
          i32.store
          local.get 2
          local.get 1
          i32.load16_u offset=13 align=1
          i32.store16 offset=9 align=1
          local.get 2
          i32.const 11
          i32.add
          local.get 1
          i32.const 15
          i32.add
          i32.load8_u
          i32.store8
          local.get 0
          i32.const 8
          i32.add
          local.get 2
          i32.store
          local.get 0
          i64.const 8589934593
          i64.store align=4
          local.get 1
          i32.const 16
          i32.add
          global.set 0
          return
        end
        i32.const 40
        i32.const 1
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    i32.const 12
    i32.const 4
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN3std3env7_var_os17hf271c90cefabfa97E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i64 i32 i32)
    global.get 0
    i32.const 80
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 2
    i32.store offset=28
    local.get 3
    local.get 1
    i32.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              i32.const 1
              i32.add
              local.tee 4
              i32.const -1
              i32.le_s
              br_if 0 (;@5;)
              local.get 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 4
              i32.const 1
              call $__rust_alloc
              local.tee 5
              i32.eqz
              br_if 4 (;@1;)
              i32.const 0
              local.set 6
              local.get 3
              i32.const 16
              i32.add
              i32.const 0
              local.get 5
              local.get 1
              local.get 2
              call $memcpy
              local.tee 1
              local.get 2
              call $_ZN4core5slice6memchr6memchr17h55b5fdbafce1735dE
              local.get 2
              i64.extend_i32_u
              i64.const 32
              i64.shl
              local.get 4
              i64.extend_i32_u
              i64.or
              local.set 7
              local.get 3
              i32.load offset=16
              br_if 3 (;@2;)
              local.get 3
              local.get 7
              i64.store offset=44 align=4
              local.get 3
              local.get 1
              i32.store offset=40
              local.get 3
              i32.const 8
              i32.add
              local.get 3
              i32.const 40
              i32.add
              call $_ZN3std3ffi5c_str7CString18from_vec_unchecked17h90197a6ae9ab1825E
              local.get 3
              i32.load offset=12
              local.set 8
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  i32.load offset=8
                  local.tee 9
                  call $getenv
                  local.tee 5
                  br_if 0 (;@7;)
                  br 1 (;@6;)
                end
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 5
                      i32.load8_u
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const 1
                      i32.add
                      local.set 6
                      i32.const 0
                      local.set 2
                      loop  ;; label = @10
                        local.get 6
                        local.get 2
                        i32.add
                        local.set 4
                        local.get 2
                        i32.const 1
                        i32.add
                        local.tee 1
                        local.set 2
                        local.get 4
                        i32.load8_u
                        br_if 0 (;@10;)
                      end
                      local.get 1
                      i32.const -1
                      i32.eq
                      br_if 5 (;@4;)
                      local.get 1
                      i32.const -1
                      i32.le_s
                      br_if 4 (;@5;)
                      local.get 1
                      br_if 1 (;@8;)
                    end
                    i32.const 1
                    local.set 6
                    i32.const 0
                    local.set 1
                    br 1 (;@7;)
                  end
                  local.get 1
                  i32.const 1
                  call $__rust_alloc
                  local.tee 6
                  i32.eqz
                  br_if 4 (;@3;)
                end
                local.get 6
                local.get 5
                local.get 1
                call $memcpy
                drop
                local.get 1
                i64.extend_i32_u
                local.tee 7
                i64.const 32
                i64.shl
                local.get 7
                i64.or
                local.set 7
              end
              local.get 9
              i32.const 0
              i32.store8
              block  ;; label = @6
                local.get 8
                i32.eqz
                br_if 0 (;@6;)
                local.get 9
                local.get 8
                i32.const 1
                call $__rust_dealloc
              end
              local.get 0
              local.get 7
              i64.store offset=4 align=4
              local.get 0
              local.get 6
              i32.store
              local.get 3
              i32.const 80
              i32.add
              global.set 0
              return
            end
            call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
            unreachable
          end
          local.get 1
          i32.const 0
          i32.const 1050084
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        local.get 1
        i32.const 1
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      local.get 3
      i32.load offset=20
      local.set 2
      local.get 3
      i32.const 48
      i32.add
      local.get 7
      i64.store
      local.get 3
      local.get 1
      i32.store offset=44
      local.get 3
      local.get 2
      i32.store offset=40
      local.get 3
      i32.const 64
      i32.add
      local.get 3
      i32.const 40
      i32.add
      call $_ZN3std3ffi5c_str104_$LT$impl$u20$core..convert..From$LT$std..ffi..c_str..NulError$GT$$u20$for$u20$std..io..error..Error$GT$4from17hd268abd50b0962a2E
      local.get 3
      local.get 3
      i64.load offset=64
      i64.store offset=32
      local.get 3
      i32.const 60
      i32.add
      i32.const 2
      i32.store
      local.get 3
      i32.const 76
      i32.add
      i32.const 12
      i32.store
      local.get 3
      i64.const 2
      i64.store offset=44 align=4
      local.get 3
      i32.const 1049932
      i32.store offset=40
      local.get 3
      i32.const 13
      i32.store offset=68
      local.get 3
      local.get 3
      i32.const 64
      i32.add
      i32.store offset=56
      local.get 3
      local.get 3
      i32.const 32
      i32.add
      i32.store offset=72
      local.get 3
      local.get 3
      i32.const 24
      i32.add
      i32.store offset=64
      local.get 3
      i32.const 40
      i32.add
      i32.const 1049948
      call $_ZN3std9panicking15begin_panic_fmt17h0c385246197edc82E
      unreachable
    end
    local.get 4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN3std3ffi5c_str104_$LT$impl$u20$core..convert..From$LT$std..ffi..c_str..NulError$GT$$u20$for$u20$std..io..error..Error$GT$4from17hd268abd50b0962a2E (type 6) (param i32 i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 33
          i32.const 1
          call $__rust_alloc
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          i32.const 32
          i32.add
          i32.const 0
          i32.load8_u offset=1050080
          i32.store8
          local.get 3
          i32.const 24
          i32.add
          i32.const 0
          i64.load offset=1050072 align=1
          i64.store align=1
          local.get 3
          i32.const 16
          i32.add
          i32.const 0
          i64.load offset=1050064 align=1
          i64.store align=1
          local.get 3
          i32.const 8
          i32.add
          i32.const 0
          i64.load offset=1050056 align=1
          i64.store align=1
          local.get 3
          i32.const 0
          i64.load offset=1050048 align=1
          i64.store align=1
          i32.const 12
          i32.const 4
          call $__rust_alloc
          local.tee 4
          i32.eqz
          br_if 1 (;@2;)
          local.get 4
          i64.const 141733920801
          i64.store offset=4 align=4
          local.get 4
          local.get 3
          i32.store
          i32.const 12
          i32.const 4
          call $__rust_alloc
          local.tee 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 3
          i32.const 11
          i32.store8 offset=8
          local.get 3
          i32.const 1049980
          i32.store offset=4
          local.get 3
          local.get 4
          i32.store
          local.get 3
          local.get 2
          i32.load16_u offset=13 align=1
          i32.store16 offset=9 align=1
          local.get 3
          i32.const 11
          i32.add
          local.get 2
          i32.const 13
          i32.add
          i32.const 2
          i32.add
          i32.load8_u
          i32.store8
          local.get 0
          i32.const 2
          i32.store8
          local.get 0
          local.get 2
          i32.load16_u offset=10 align=1
          i32.store16 offset=1 align=1
          local.get 0
          i32.const 3
          i32.add
          local.get 2
          i32.const 10
          i32.add
          i32.const 2
          i32.add
          i32.load8_u
          i32.store8
          local.get 0
          i32.const 4
          i32.add
          local.get 3
          i32.store
          block  ;; label = @4
            local.get 1
            i32.load offset=4
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            i32.const 8
            i32.add
            i32.load
            local.tee 0
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            local.get 0
            i32.const 1
            call $__rust_dealloc
          end
          local.get 2
          i32.const 16
          i32.add
          global.set 0
          return
        end
        i32.const 33
        i32.const 1
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    i32.const 12
    i32.const 4
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h3cc362ab1df73571E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.load8_u
            br_table 1 (;@3;) 0 (;@4;) 2 (;@2;) 1 (;@3;)
          end
          i32.const 1050232
          local.set 3
          i32.const 22
          local.set 4
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        block  ;; label = @19
                                          block  ;; label = @20
                                            block  ;; label = @21
                                              block  ;; label = @22
                                                local.get 0
                                                i32.load8_u offset=1
                                                br_table 0 (;@22;) 1 (;@21;) 2 (;@20;) 3 (;@19;) 4 (;@18;) 5 (;@17;) 6 (;@16;) 7 (;@15;) 8 (;@14;) 9 (;@13;) 10 (;@12;) 11 (;@11;) 12 (;@10;) 13 (;@9;) 14 (;@8;) 15 (;@7;) 16 (;@6;) 18 (;@4;) 0 (;@22;)
                                              end
                                              i32.const 1050513
                                              local.set 3
                                              i32.const 16
                                              local.set 4
                                              br 17 (;@4;)
                                            end
                                            i32.const 1050496
                                            local.set 3
                                            i32.const 17
                                            local.set 4
                                            br 16 (;@4;)
                                          end
                                          i32.const 1050478
                                          local.set 3
                                          i32.const 18
                                          local.set 4
                                          br 15 (;@4;)
                                        end
                                        i32.const 1050462
                                        local.set 3
                                        i32.const 16
                                        local.set 4
                                        br 14 (;@4;)
                                      end
                                      i32.const 1050444
                                      local.set 3
                                      i32.const 18
                                      local.set 4
                                      br 13 (;@4;)
                                    end
                                    i32.const 1050431
                                    local.set 3
                                    i32.const 13
                                    local.set 4
                                    br 12 (;@4;)
                                  end
                                  i32.const 1050417
                                  local.set 3
                                  br 10 (;@5;)
                                end
                                i32.const 1050396
                                local.set 3
                                i32.const 21
                                local.set 4
                                br 10 (;@4;)
                              end
                              i32.const 1050385
                              local.set 3
                              i32.const 11
                              local.set 4
                              br 9 (;@4;)
                            end
                            i32.const 1050364
                            local.set 3
                            i32.const 21
                            local.set 4
                            br 8 (;@4;)
                          end
                          i32.const 1050343
                          local.set 3
                          i32.const 21
                          local.set 4
                          br 7 (;@4;)
                        end
                        i32.const 1050320
                        local.set 3
                        i32.const 23
                        local.set 4
                        br 6 (;@4;)
                      end
                      i32.const 1050308
                      local.set 3
                      i32.const 12
                      local.set 4
                      br 5 (;@4;)
                    end
                    i32.const 1050299
                    local.set 3
                    i32.const 9
                    local.set 4
                    br 4 (;@4;)
                  end
                  i32.const 1050289
                  local.set 3
                  i32.const 10
                  local.set 4
                  br 3 (;@4;)
                end
                i32.const 1050268
                local.set 3
                i32.const 21
                local.set 4
                br 2 (;@4;)
              end
              i32.const 1050254
              local.set 3
            end
            i32.const 14
            local.set 4
          end
          local.get 2
          i32.const 60
          i32.add
          i32.const 1
          i32.store
          local.get 2
          local.get 4
          i32.store offset=28
          local.get 2
          local.get 3
          i32.store offset=24
          local.get 2
          i32.const 14
          i32.store offset=12
          local.get 2
          i64.const 1
          i64.store offset=44 align=4
          local.get 2
          i32.const 1050532
          i32.store offset=40
          local.get 2
          local.get 2
          i32.const 24
          i32.add
          i32.store offset=8
          local.get 2
          local.get 2
          i32.const 8
          i32.add
          i32.store offset=56
          local.get 1
          local.get 2
          i32.const 40
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E
          local.set 0
          br 2 (;@1;)
        end
        local.get 2
        local.get 0
        i32.const 4
        i32.add
        i32.load
        local.tee 0
        i32.store offset=4
        local.get 2
        i32.const 8
        i32.add
        local.get 0
        call $_ZN3std3sys4wasi2os12error_string17h9c43b25761d31a0dE
        local.get 2
        i32.const 60
        i32.add
        i32.const 2
        i32.store
        local.get 2
        i32.const 36
        i32.add
        i32.const 15
        i32.store
        local.get 2
        i64.const 3
        i64.store offset=44 align=4
        local.get 2
        i32.const 1050552
        i32.store offset=40
        local.get 2
        i32.const 16
        i32.store offset=28
        local.get 2
        local.get 2
        i32.const 24
        i32.add
        i32.store offset=56
        local.get 2
        local.get 2
        i32.const 4
        i32.add
        i32.store offset=32
        local.get 2
        local.get 2
        i32.const 8
        i32.add
        i32.store offset=24
        local.get 1
        local.get 2
        i32.const 40
        i32.add
        call $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E
        local.set 0
        local.get 2
        i32.load offset=8
        local.tee 1
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.load offset=12
        local.tee 3
        i32.eqz
        br_if 1 (;@1;)
        local.get 1
        local.get 3
        i32.const 1
        call $__rust_dealloc
        br 1 (;@1;)
      end
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 0
      i32.load
      local.get 1
      local.get 0
      i32.load offset=4
      i32.load offset=32
      call_indirect (type 3)
      local.set 0
    end
    local.get 2
    i32.const 64
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN55_$LT$std..path..Display$u20$as$u20$core..fmt..Debug$GT$3fmt17h853c8bfc67334964E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.const 4
    i32.add
    i32.load
    local.get 1
    call $_ZN73_$LT$std..sys_common..os_str_bytes..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hf2181278eaad9e60E)
  (func $_ZN3std3env4args17h6df17352f1454ba8E (type 1) (param i32)
    local.get 0
    call $_ZN3std3env7args_os17haaaabf8dc2b6b227E)
  (func $_ZN3std3env7args_os17haaaabf8dc2b6b227E (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    call $_ZN4wasi13lib_generated14args_sizes_get17h67a6e4a49c03ee4dE
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 1
                    i32.load16_u
                    i32.const 1
                    i32.eq
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 1
                        i32.load offset=4
                        local.tee 2
                        i32.const 1073741823
                        i32.and
                        local.get 2
                        i32.eq
                        local.tee 3
                        i32.eqz
                        br_if 0 (;@10;)
                        local.get 2
                        i32.const 2
                        i32.shl
                        local.tee 4
                        i32.const -1
                        i32.le_s
                        br_if 0 (;@10;)
                        local.get 3
                        i32.const 2
                        i32.shl
                        local.set 3
                        local.get 1
                        i32.const 8
                        i32.add
                        i32.load
                        local.set 5
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 4
                            i32.eqz
                            br_if 0 (;@12;)
                            local.get 4
                            local.get 3
                            call $__rust_alloc
                            local.set 6
                            br 1 (;@11;)
                          end
                          local.get 3
                          local.set 6
                        end
                        local.get 6
                        i32.eqz
                        br_if 1 (;@9;)
                        local.get 5
                        i32.const -1
                        i32.le_s
                        br_if 0 (;@10;)
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    local.get 5
                                    i32.eqz
                                    br_if 0 (;@16;)
                                    local.get 5
                                    i32.const 1
                                    call $__rust_alloc
                                    local.tee 7
                                    br_if 1 (;@15;)
                                    local.get 5
                                    i32.const 1
                                    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                                    unreachable
                                  end
                                  i32.const 1
                                  local.set 7
                                  local.get 6
                                  i32.const 1
                                  call $_ZN4wasi13lib_generated8args_get17h8303b93f14c625f0E
                                  i32.const 65535
                                  i32.and
                                  i32.eqz
                                  br_if 1 (;@14;)
                                  br 3 (;@12;)
                                end
                                local.get 6
                                local.get 7
                                call $_ZN4wasi13lib_generated8args_get17h8303b93f14c625f0E
                                i32.const 65535
                                i32.and
                                br_if 1 (;@13;)
                              end
                              local.get 2
                              i64.extend_i32_u
                              i64.const 12
                              i64.mul
                              local.tee 8
                              i64.const 32
                              i64.shr_u
                              i32.wrap_i64
                              local.tee 9
                              br_if 3 (;@10;)
                              local.get 8
                              i32.wrap_i64
                              local.tee 3
                              i32.const -1
                              i32.le_s
                              br_if 3 (;@10;)
                              local.get 9
                              i32.eqz
                              i32.const 2
                              i32.shl
                              local.set 9
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 3
                                  i32.eqz
                                  br_if 0 (;@15;)
                                  local.get 3
                                  local.get 9
                                  call $__rust_alloc
                                  local.set 10
                                  br 1 (;@14;)
                                end
                                local.get 9
                                local.set 10
                              end
                              local.get 10
                              i32.eqz
                              br_if 6 (;@7;)
                              local.get 3
                              i32.const 12
                              i32.div_u
                              local.set 11
                              local.get 2
                              br_if 2 (;@11;)
                              i32.const 0
                              local.set 2
                              br 11 (;@2;)
                            end
                            local.get 7
                            local.get 5
                            i32.const 1
                            call $__rust_dealloc
                          end
                          i32.const 0
                          local.set 11
                          i32.const 4
                          local.set 10
                          local.get 4
                          i32.eqz
                          br_if 8 (;@3;)
                          local.get 6
                          local.get 4
                          i32.const 4
                          call $__rust_dealloc
                          br 8 (;@3;)
                        end
                        local.get 6
                        local.get 2
                        i32.const 2
                        i32.shl
                        i32.add
                        local.set 12
                        local.get 6
                        local.set 13
                        i32.const 0
                        local.set 14
                        loop  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                local.get 13
                                i32.load
                                local.tee 15
                                i32.load8_u
                                i32.eqz
                                br_if 0 (;@14;)
                                local.get 15
                                i32.const 1
                                i32.add
                                local.set 16
                                i32.const 0
                                local.set 2
                                loop  ;; label = @15
                                  local.get 16
                                  local.get 2
                                  i32.add
                                  local.set 3
                                  local.get 2
                                  i32.const 1
                                  i32.add
                                  local.tee 9
                                  local.set 2
                                  local.get 3
                                  i32.load8_u
                                  br_if 0 (;@15;)
                                end
                                local.get 9
                                i32.const -1
                                i32.eq
                                br_if 8 (;@6;)
                                local.get 9
                                i32.const -1
                                i32.le_s
                                br_if 4 (;@10;)
                                local.get 9
                                br_if 1 (;@13;)
                              end
                              i32.const 1
                              local.set 2
                              i32.const 0
                              local.set 9
                              br 1 (;@12;)
                            end
                            local.get 9
                            i32.const 1
                            call $__rust_alloc
                            local.tee 2
                            i32.eqz
                            br_if 7 (;@5;)
                          end
                          local.get 2
                          local.get 15
                          local.get 9
                          call $memcpy
                          local.set 16
                          local.get 14
                          i32.const 1
                          i32.add
                          local.set 2
                          block  ;; label = @12
                            local.get 14
                            local.get 11
                            i32.ne
                            br_if 0 (;@12;)
                            local.get 14
                            i32.const 1
                            i32.shl
                            local.tee 3
                            local.get 2
                            local.get 3
                            local.get 2
                            i32.gt_u
                            select
                            local.tee 3
                            i32.const 4
                            local.get 3
                            i32.const 4
                            i32.gt_u
                            select
                            i64.extend_i32_u
                            i64.const 12
                            i64.mul
                            local.tee 8
                            i64.const 32
                            i64.shr_u
                            i32.wrap_i64
                            local.tee 15
                            br_if 2 (;@10;)
                            local.get 8
                            i32.wrap_i64
                            local.tee 3
                            i32.const 0
                            i32.lt_s
                            br_if 2 (;@10;)
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 10
                                  i32.const 0
                                  local.get 14
                                  select
                                  local.tee 10
                                  br_if 0 (;@15;)
                                  local.get 15
                                  i32.eqz
                                  i32.const 2
                                  i32.shl
                                  local.set 10
                                  local.get 3
                                  i32.eqz
                                  br_if 1 (;@14;)
                                  local.get 3
                                  local.get 10
                                  call $__rust_alloc
                                  local.set 10
                                  br 1 (;@14;)
                                end
                                block  ;; label = @15
                                  local.get 14
                                  i32.eqz
                                  br_if 0 (;@15;)
                                  local.get 10
                                  local.get 14
                                  i32.const 12
                                  i32.mul
                                  i32.const 4
                                  local.get 3
                                  call $__rust_realloc
                                  local.set 10
                                  br 1 (;@14;)
                                end
                                block  ;; label = @15
                                  local.get 3
                                  br_if 0 (;@15;)
                                  i32.const 4
                                  local.set 10
                                  br 2 (;@13;)
                                end
                                local.get 3
                                i32.const 4
                                call $__rust_alloc
                                local.set 10
                              end
                              local.get 10
                              i32.eqz
                              br_if 9 (;@4;)
                            end
                            local.get 3
                            i32.const 12
                            i32.div_u
                            local.set 11
                          end
                          local.get 10
                          local.get 14
                          i32.const 12
                          i32.mul
                          i32.add
                          local.tee 3
                          local.get 9
                          i32.store offset=8
                          local.get 3
                          local.get 9
                          i32.store offset=4
                          local.get 3
                          local.get 16
                          i32.store
                          local.get 2
                          local.set 14
                          local.get 13
                          i32.const 4
                          i32.add
                          local.tee 13
                          local.get 12
                          i32.eq
                          br_if 9 (;@2;)
                          br 0 (;@11;)
                        end
                      end
                      call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
                      unreachable
                    end
                    local.get 4
                    local.get 3
                    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                    unreachable
                  end
                  i32.const 4
                  local.set 10
                  i32.const 0
                  local.set 11
                  br 4 (;@3;)
                end
                local.get 3
                local.get 9
                call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                unreachable
              end
              local.get 9
              i32.const 0
              i32.const 1050084
              call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
              unreachable
            end
            local.get 9
            i32.const 1
            call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
            unreachable
          end
          local.get 3
          i32.const 4
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 6
        local.get 4
        i32.const 4
        call $__rust_dealloc
      end
      local.get 5
      i32.eqz
      br_if 0 (;@1;)
      local.get 7
      local.get 5
      i32.const 1
      call $__rust_dealloc
    end
    local.get 0
    local.get 10
    i32.store offset=8
    local.get 0
    local.get 11
    i32.store offset=4
    local.get 0
    local.get 10
    i32.store
    local.get 0
    local.get 10
    local.get 2
    i32.const 12
    i32.mul
    i32.add
    i32.store offset=12
    local.get 1
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha4eaab787b46e43fE (type 6) (param i32 i32)
    (local i32 i32 i32 i64)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.load offset=8
            local.tee 3
            local.get 1
            i32.load offset=12
            i32.eq
            br_if 0 (;@4;)
            local.get 1
            local.get 3
            i32.const 12
            i32.add
            i32.store offset=8
            local.get 3
            i32.load
            local.tee 1
            br_if 1 (;@3;)
          end
          local.get 0
          i32.const 0
          i32.store
          br 1 (;@2;)
        end
        local.get 3
        i32.load offset=4
        local.set 4
        local.get 2
        i32.const 24
        i32.add
        local.get 1
        local.get 3
        i32.load offset=8
        local.tee 3
        call $_ZN4core3str9from_utf817haf80b3a8845ce8ecE
        local.get 2
        i32.load offset=24
        i32.const 1
        i32.eq
        br_if 1 (;@1;)
        local.get 0
        local.get 4
        i32.store offset=4
        local.get 0
        local.get 1
        i32.store
        local.get 0
        i32.const 8
        i32.add
        local.get 3
        i32.store
      end
      local.get 2
      i32.const 48
      i32.add
      global.set 0
      return
    end
    local.get 2
    local.get 2
    i64.load offset=28 align=4
    i64.store offset=36 align=4
    local.get 2
    local.get 3
    i32.store offset=32
    local.get 2
    local.get 4
    i32.store offset=28
    local.get 2
    local.get 1
    i32.store offset=24
    local.get 2
    i32.const 8
    i32.add
    local.get 2
    i32.const 24
    i32.add
    call $_ZN5alloc6string13FromUtf8Error10into_bytes17heafb6e86be24df0cE
    local.get 2
    i64.load offset=8
    local.set 5
    local.get 2
    local.get 2
    i32.load offset=16
    i32.store offset=32
    local.get 2
    local.get 5
    i64.store offset=24
    i32.const 1049360
    i32.const 43
    local.get 2
    i32.const 24
    i32.add
    i32.const 1049420
    i32.const 1049964
    call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
    unreachable)
  (func $_ZN3std5error5Error7type_id17he6dcf01a791cbadbE (type 2) (param i32) (result i64)
    i64.const 6505047904689562866)
  (func $_ZN3std5error5Error9backtrace17h3b27bb4b1eb1fdc9E (type 5) (param i32) (result i32)
    i32.const 0)
  (func $_ZN3std5error5Error5cause17hb938bd2ebabb958dE (type 6) (param i32 i32)
    local.get 0
    i32.const 0
    i32.store)
  (func $_ZN243_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$std..error..Error$GT$11description17h8d8d7033eb4c2897E (type 6) (param i32 i32)
    local.get 0
    local.get 1
    i32.load offset=8
    i32.store offset=4
    local.get 0
    local.get 1
    i32.load
    i32.store)
  (func $_ZN244_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$core..fmt..Display$GT$3fmt17h19e8603e84100fa4E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=8
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hce18dc51dfa14637E)
  (func $_ZN242_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$core..fmt..Debug$GT$3fmt17h31e0ae2091b2beacE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=8
    local.get 1
    call $_ZN40_$LT$str$u20$as$u20$core..fmt..Debug$GT$3fmt17h9a17d4d05f37d658E)
  (func $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E (type 5) (param i32) (result i32)
    (local i32)
    i32.const 16
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.const 65535
      i32.gt_u
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 0
                                  i32.const 65535
                                  i32.and
                                  i32.const -2
                                  i32.add
                                  br_table 2 (;@13;) 7 (;@8;) 6 (;@9;) 14 (;@1;) 13 (;@2;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 5 (;@10;) 0 (;@15;) 1 (;@14;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 12 (;@3;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 9 (;@6;) 10 (;@5;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 8 (;@7;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 4 (;@11;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 2 (;@13;) 3 (;@12;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 14 (;@1;) 11 (;@4;) 14 (;@1;)
                                end
                                i32.const 2
                                return
                              end
                              i32.const 3
                              return
                            end
                            i32.const 1
                            return
                          end
                          i32.const 8
                          return
                        end
                        i32.const 5
                        return
                      end
                      i32.const 4
                      return
                    end
                    i32.const 7
                    return
                  end
                  i32.const 6
                  return
                end
                i32.const 0
                return
              end
              i32.const 15
              return
            end
            i32.const 11
            return
          end
          i32.const 13
          return
        end
        i32.const 9
        return
      end
      i32.const 10
      local.set 1
    end
    local.get 1)
  (func $_ZN72_$LT$std..io..buffered..BufWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17h4c33c330bd78f65cE (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i64)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 8
            i32.add
            i32.load
            local.get 3
            i32.add
            local.get 1
            i32.const 4
            i32.add
            i32.load
            local.tee 5
            i32.le_u
            br_if 0 (;@4;)
            local.get 4
            i32.const 16
            i32.add
            local.get 1
            call $_ZN3std2io8buffered18BufWriter$LT$W$GT$9flush_buf17h0d5bd5c28054fc9aE
            local.get 4
            i64.load offset=16
            local.tee 6
            i32.wrap_i64
            i32.const 255
            i32.and
            i32.const 3
            i32.ne
            br_if 1 (;@3;)
            local.get 1
            i32.const 4
            i32.add
            i32.load
            local.set 5
          end
          local.get 5
          local.get 3
          i32.le_u
          br_if 1 (;@2;)
          local.get 1
          local.get 2
          local.get 3
          call $_ZN5alloc3vec12Vec$LT$T$GT$17extend_from_slice17hd567b84a391d6697E
          local.get 0
          i32.const 0
          i32.store
          local.get 0
          local.get 3
          i32.store offset=4
          br 2 (;@1;)
        end
        local.get 0
        i32.const 1
        i32.store
        local.get 0
        local.get 6
        i64.store offset=4 align=4
        br 1 (;@1;)
      end
      local.get 1
      i32.const 1
      i32.store8 offset=13
      block  ;; label = @2
        local.get 1
        i32.load8_u offset=12
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 4
        local.get 3
        i32.store offset=12
        local.get 4
        local.get 2
        i32.store offset=8
        local.get 4
        i32.const 16
        i32.add
        i32.const 1
        local.get 4
        i32.const 8
        i32.add
        i32.const 1
        call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
        block  ;; label = @3
          block  ;; label = @4
            local.get 4
            i32.load16_u offset=16
            i32.const 1
            i32.eq
            br_if 0 (;@4;)
            local.get 4
            i64.load32_u offset=20
            local.set 6
            i32.const 0
            local.set 3
            br 1 (;@3;)
          end
          local.get 4
          local.get 4
          i32.load16_u offset=18
          i32.store16 offset=30
          local.get 3
          i64.extend_i32_u
          local.get 4
          i32.const 30
          i32.add
          call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
          local.tee 3
          i64.extend_i32_u
          i64.const 65535
          i64.and
          i64.const 32
          i64.shl
          local.get 3
          i32.const 65535
          i32.and
          local.tee 3
          i32.const 8
          i32.eq
          select
          local.set 6
          local.get 3
          i32.const 8
          i32.ne
          local.set 3
        end
        local.get 0
        local.get 6
        i64.store offset=4 align=4
        local.get 0
        local.get 3
        i32.store
        local.get 1
        i32.const 0
        i32.store8 offset=13
        br 1 (;@1;)
      end
      i32.const 1049268
      i32.const 43
      i32.const 1050200
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    local.get 4
    i32.const 32
    i32.add
    global.set 0)
  (func $_ZN73_$LT$std..io..buffered..LineWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17ha396c4b89d5aac4fE (type 4) (param i32 i32 i32 i32)
    (local i32 i64 i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.load8_u offset=16
        i32.eqz
        br_if 0 (;@2;)
        local.get 4
        i32.const 16
        i32.add
        local.get 1
        call $_ZN3std2io8buffered18BufWriter$LT$W$GT$9flush_buf17h0d5bd5c28054fc9aE
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 4
              i32.load8_u offset=16
              i32.const 3
              i32.ne
              br_if 0 (;@5;)
              local.get 1
              i32.load8_u offset=12
              i32.const 1
              i32.eq
              br_if 1 (;@4;)
              i32.const 1049268
              i32.const 43
              i32.const 1050200
              call $_ZN4core9panicking5panic17heeaec3885c636092E
              unreachable
            end
            local.get 4
            i64.load offset=16
            local.tee 5
            i32.wrap_i64
            i32.const 255
            i32.and
            local.tee 6
            i32.const 3
            i32.ne
            br_if 1 (;@3;)
          end
          local.get 1
          i32.const 0
          i32.store8 offset=16
          br 1 (;@2;)
        end
        local.get 6
        i32.const 3
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        i32.const 1
        i32.store
        local.get 0
        local.get 5
        i64.store offset=4 align=4
        br 1 (;@1;)
      end
      local.get 4
      i32.const 8
      i32.add
      i32.const 10
      local.get 2
      local.get 3
      call $_ZN4core5slice6memchr7memrchr17hb78711c2471586dcE
      block  ;; label = @2
        local.get 4
        i32.load offset=8
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        local.get 2
        local.get 3
        call $_ZN72_$LT$std..io..buffered..BufWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17h4c33c330bd78f65cE
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.load offset=12
          local.tee 6
          i32.const -1
          i32.eq
          br_if 0 (;@3;)
          local.get 6
          i32.const 1
          i32.add
          local.set 7
          local.get 6
          local.get 3
          i32.lt_u
          br_if 1 (;@2;)
          local.get 7
          local.get 3
          i32.const 1050216
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        i32.const 1050216
        call $_ZN4core5slice25slice_index_overflow_fail17hc610e4c3b7a5b2c6E
        unreachable
      end
      local.get 4
      i32.const 16
      i32.add
      local.get 1
      local.get 2
      local.get 7
      call $_ZN72_$LT$std..io..buffered..BufWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17h4c33c330bd78f65cE
      local.get 4
      i64.load offset=20 align=4
      local.set 5
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 4
              i32.load offset=16
              i32.const 1
              i32.eq
              br_if 0 (;@5;)
              local.get 1
              i32.const 1
              i32.store8 offset=16
              local.get 4
              i32.const 16
              i32.add
              local.get 1
              call $_ZN3std2io8buffered18BufWriter$LT$W$GT$9flush_buf17h0d5bd5c28054fc9aE
              local.get 5
              i32.wrap_i64
              local.set 6
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 4
                    i32.load8_u offset=16
                    i32.const 3
                    i32.ne
                    br_if 0 (;@8;)
                    local.get 1
                    i32.load8_u offset=12
                    i32.const 1
                    i32.eq
                    br_if 1 (;@7;)
                    i32.const 1049268
                    i32.const 43
                    i32.const 1050200
                    call $_ZN4core9panicking5panic17heeaec3885c636092E
                    unreachable
                  end
                  local.get 4
                  i32.load8_u offset=16
                  i32.const 3
                  i32.ne
                  br_if 1 (;@6;)
                end
                local.get 1
                i32.const 0
                i32.store8 offset=16
                local.get 7
                local.get 6
                i32.ne
                br_if 3 (;@3;)
                br 4 (;@2;)
              end
              local.get 4
              i32.load offset=16
              local.tee 8
              i32.const 255
              i32.and
              i32.const 3
              i32.eq
              br_if 1 (;@4;)
              local.get 4
              i32.load offset=20
              local.set 1
              block  ;; label = @6
                i32.const 0
                br_if 0 (;@6;)
                local.get 8
                i32.const 3
                i32.and
                i32.const 2
                i32.ne
                br_if 3 (;@3;)
              end
              local.get 1
              i32.load
              local.get 1
              i32.load offset=4
              i32.load
              call_indirect (type 1)
              block  ;; label = @6
                local.get 1
                i32.load offset=4
                local.tee 2
                i32.load offset=4
                local.tee 3
                i32.eqz
                br_if 0 (;@6;)
                local.get 1
                i32.load
                local.get 3
                local.get 2
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get 1
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              br 2 (;@3;)
            end
            local.get 0
            i32.const 1
            i32.store
            local.get 0
            local.get 5
            i64.store offset=4 align=4
            br 3 (;@1;)
          end
          local.get 7
          local.get 6
          i32.eq
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 0
        i32.store
        local.get 0
        local.get 6
        i32.store offset=4
        br 1 (;@1;)
      end
      local.get 4
      i32.const 16
      i32.add
      local.get 1
      local.get 2
      local.get 7
      i32.add
      local.get 3
      local.get 7
      i32.sub
      call $_ZN72_$LT$std..io..buffered..BufWriter$LT$W$GT$$u20$as$u20$std..io..Write$GT$5write17h4c33c330bd78f65cE
      block  ;; label = @2
        local.get 4
        i32.load offset=16
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        i32.const 0
        i32.store
        local.get 0
        local.get 4
        i32.load offset=20
        local.get 6
        i32.add
        i32.store offset=4
        br 1 (;@1;)
      end
      local.get 0
      i32.const 0
      i32.store
      local.get 0
      local.get 6
      i32.store offset=4
      local.get 4
      i32.load8_u offset=20
      i32.const 2
      i32.lt_u
      br_if 0 (;@1;)
      local.get 4
      i32.const 24
      i32.add
      i32.load
      local.tee 1
      i32.load
      local.get 1
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 1
        i32.load offset=4
        local.tee 2
        i32.load offset=4
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load
        local.get 3
        local.get 2
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 1
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    local.get 4
    i32.const 32
    i32.add
    global.set 0)
  (func $_ZN3std3sys4wasi2os12error_string17h9c43b25761d31a0dE (type 6) (param i32 i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 1056
    i32.sub
    local.tee 2
    global.set 0
    i32.const 0
    local.set 3
    local.get 2
    i32.const 8
    i32.add
    i32.const 0
    i32.const 1024
    call $memset
    drop
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              local.get 2
              i32.const 8
              i32.add
              i32.const 1024
              call $strerror_r
              i32.const 0
              i32.lt_s
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 2
                i32.load8_u offset=8
                i32.eqz
                br_if 0 (;@6;)
                local.get 2
                i32.const 8
                i32.add
                i32.const 1
                i32.add
                local.set 4
                i32.const 0
                local.set 1
                loop  ;; label = @7
                  local.get 4
                  local.get 1
                  i32.add
                  local.set 5
                  local.get 1
                  i32.const 1
                  i32.add
                  local.tee 3
                  local.set 1
                  local.get 5
                  i32.load8_u
                  br_if 0 (;@7;)
                end
                local.get 3
                i32.const -1
                i32.eq
                br_if 2 (;@4;)
              end
              local.get 2
              i32.const 1032
              i32.add
              local.get 2
              i32.const 8
              i32.add
              local.get 3
              call $_ZN4core3str9from_utf817haf80b3a8845ce8ecE
              local.get 2
              i32.load offset=1032
              i32.const 1
              i32.eq
              br_if 2 (;@3;)
              local.get 2
              i32.const 1040
              i32.add
              i32.load
              local.tee 1
              i32.const -1
              i32.le_s
              br_if 3 (;@2;)
              local.get 2
              i32.load offset=1036
              local.set 5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 1
                  br_if 0 (;@7;)
                  i32.const 1
                  local.set 3
                  br 1 (;@6;)
                end
                local.get 1
                i32.const 1
                call $__rust_alloc
                local.tee 3
                i32.eqz
                br_if 5 (;@1;)
              end
              local.get 3
              local.get 5
              local.get 1
              call $memcpy
              local.set 5
              local.get 0
              local.get 1
              i32.store offset=8
              local.get 0
              local.get 1
              i32.store offset=4
              local.get 0
              local.get 5
              i32.store
              local.get 2
              i32.const 1056
              i32.add
              global.set 0
              return
            end
            i32.const 1052692
            i32.const 18
            i32.const 1052740
            call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
            unreachable
          end
          local.get 3
          i32.const 0
          i32.const 1050084
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        local.get 2
        local.get 2
        i64.load offset=1036 align=4
        i64.store offset=1048
        i32.const 1049360
        i32.const 43
        local.get 2
        i32.const 1048
        i32.add
        i32.const 1049436
        i32.const 1052756
        call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
      unreachable
    end
    local.get 1
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$5write17h8c2c31ee81abd4daE (type 4) (param i32 i32 i32 i32)
    local.get 0
    local.get 1
    i32.load
    local.get 2
    local.get 3
    local.get 1
    i32.load offset=4
    i32.load offset=12
    call_indirect (type 4))
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$14write_vectored17h5349500dd10151ecE (type 4) (param i32 i32 i32 i32)
    local.get 0
    local.get 1
    i32.load
    local.get 2
    local.get 3
    local.get 1
    i32.load offset=4
    i32.load offset=16
    call_indirect (type 4))
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$17is_write_vectored17h068b3aff18149e2dE (type 5) (param i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    i32.load offset=20
    call_indirect (type 5))
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$5flush17h5099216c82f203e7E (type 6) (param i32 i32)
    local.get 0
    local.get 1
    i32.load
    local.get 1
    i32.load offset=4
    i32.load offset=24
    call_indirect (type 6))
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$9write_all17h6c0f53b93c085afcE (type 4) (param i32 i32 i32 i32)
    local.get 0
    local.get 1
    i32.load
    local.get 2
    local.get 3
    local.get 1
    i32.load offset=4
    i32.load offset=28
    call_indirect (type 4))
  (func $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$9write_fmt17hf7c0a8c6ed5ea434E (type 7) (param i32 i32 i32)
    (local i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 3
    global.set 0
    local.get 1
    i32.load
    local.set 4
    local.get 1
    i32.load offset=4
    local.set 1
    local.get 3
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    local.get 2
    i64.load align=4
    i64.store offset=8
    local.get 0
    local.get 4
    local.get 3
    i32.const 8
    i32.add
    local.get 1
    i32.load offset=36
    call_indirect (type 7)
    local.get 3
    i32.const 32
    i32.add
    global.set 0)
  (func $_ZN3std2io5Write18write_all_vectored17h22801b0a1a1e68b9E (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            br_if 0 (;@4;)
            i32.const 0
            local.set 5
            br 1 (;@3;)
          end
          local.get 2
          i32.const 4
          i32.add
          local.set 6
          local.get 3
          i32.const 3
          i32.shl
          i32.const -8
          i32.add
          i32.const 3
          i32.shr_u
          i32.const 1
          i32.add
          local.set 7
          i32.const 0
          local.set 5
          block  ;; label = @4
            loop  ;; label = @5
              local.get 6
              i32.load
              br_if 1 (;@4;)
              local.get 6
              i32.const 8
              i32.add
              local.set 6
              local.get 7
              local.get 5
              i32.const 1
              i32.add
              local.tee 5
              i32.ne
              br_if 0 (;@5;)
            end
            local.get 7
            local.set 5
          end
          local.get 5
          local.get 3
          i32.gt_u
          br_if 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              local.get 5
              i32.sub
              local.tee 8
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              local.get 5
              i32.const 3
              i32.shl
              i32.add
              local.set 9
              loop  ;; label = @6
                local.get 4
                i32.const 2
                local.get 9
                local.get 8
                call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 4
                            i32.load16_u
                            i32.const 1
                            i32.eq
                            br_if 0 (;@12;)
                            block  ;; label = @13
                              local.get 4
                              i32.load offset=4
                              local.tee 2
                              br_if 0 (;@13;)
                              i32.const 28
                              i32.const 1
                              call $__rust_alloc
                              local.tee 6
                              i32.eqz
                              br_if 2 (;@11;)
                              local.get 6
                              i32.const 24
                              i32.add
                              i32.const 0
                              i32.load offset=1050600 align=1
                              i32.store align=1
                              local.get 6
                              i32.const 16
                              i32.add
                              i32.const 0
                              i64.load offset=1050592 align=1
                              i64.store align=1
                              local.get 6
                              i32.const 8
                              i32.add
                              i32.const 0
                              i64.load offset=1050584 align=1
                              i64.store align=1
                              local.get 6
                              i32.const 0
                              i64.load offset=1050576 align=1
                              i64.store align=1
                              i32.const 12
                              i32.const 4
                              call $__rust_alloc
                              local.tee 5
                              i32.eqz
                              br_if 3 (;@10;)
                              local.get 5
                              i64.const 120259084316
                              i64.store offset=4 align=4
                              local.get 5
                              local.get 6
                              i32.store
                              i32.const 12
                              i32.const 4
                              call $__rust_alloc
                              local.tee 6
                              i32.eqz
                              br_if 4 (;@9;)
                              local.get 6
                              i32.const 14
                              i32.store8 offset=8
                              local.get 6
                              i32.const 1049980
                              i32.store offset=4
                              local.get 6
                              local.get 5
                              i32.store
                              local.get 6
                              local.get 4
                              i32.load16_u offset=13 align=1
                              i32.store16 offset=9 align=1
                              local.get 6
                              i32.const 11
                              i32.add
                              local.get 4
                              i32.const 13
                              i32.add
                              i32.const 2
                              i32.add
                              i32.load8_u
                              i32.store8
                              local.get 0
                              i32.const 4
                              i32.add
                              local.get 6
                              i32.store
                              local.get 0
                              i32.const 2
                              i32.store
                              br 12 (;@1;)
                            end
                            local.get 9
                            i32.const 4
                            i32.add
                            local.set 6
                            local.get 8
                            i32.const 3
                            i32.shl
                            i32.const -8
                            i32.add
                            i32.const 3
                            i32.shr_u
                            i32.const 1
                            i32.add
                            local.set 10
                            i32.const 0
                            local.set 5
                            i32.const 0
                            local.set 7
                            loop  ;; label = @13
                              local.get 6
                              i32.load
                              local.get 7
                              i32.add
                              local.tee 3
                              local.get 2
                              i32.gt_u
                              br_if 5 (;@8;)
                              local.get 6
                              i32.const 8
                              i32.add
                              local.set 6
                              local.get 3
                              local.set 7
                              local.get 10
                              local.get 5
                              i32.const 1
                              i32.add
                              local.tee 5
                              i32.ne
                              br_if 0 (;@13;)
                            end
                            local.get 3
                            local.set 7
                            local.get 10
                            local.set 5
                            br 4 (;@8;)
                          end
                          local.get 4
                          local.get 4
                          i32.load16_u offset=2
                          i32.store16 offset=10
                          local.get 4
                          i32.const 10
                          i32.add
                          call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
                          i32.const 65535
                          i32.and
                          local.tee 6
                          call $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E
                          i32.const 255
                          i32.and
                          i32.const 15
                          i32.eq
                          br_if 4 (;@7;)
                          local.get 0
                          i32.const 0
                          i32.store
                          local.get 0
                          i32.const 4
                          i32.add
                          local.get 6
                          i32.store
                          br 10 (;@1;)
                        end
                        i32.const 28
                        i32.const 1
                        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                        unreachable
                      end
                      i32.const 12
                      i32.const 4
                      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                      unreachable
                    end
                    i32.const 12
                    i32.const 4
                    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                    unreachable
                  end
                  local.get 8
                  local.get 5
                  i32.lt_u
                  br_if 3 (;@4;)
                  local.get 9
                  local.get 5
                  i32.const 3
                  i32.shl
                  i32.add
                  local.set 9
                  local.get 8
                  local.get 5
                  i32.sub
                  local.tee 8
                  i32.eqz
                  br_if 0 (;@7;)
                  local.get 9
                  i32.load offset=4
                  local.tee 5
                  local.get 2
                  local.get 7
                  i32.sub
                  local.tee 6
                  i32.lt_u
                  br_if 4 (;@3;)
                  local.get 9
                  i32.const 4
                  i32.add
                  local.get 5
                  local.get 6
                  i32.sub
                  i32.store
                  local.get 9
                  local.get 9
                  i32.load
                  local.get 6
                  i32.add
                  i32.store
                end
                local.get 8
                br_if 0 (;@6;)
              end
            end
            local.get 0
            i32.const 3
            i32.store8
            br 3 (;@1;)
          end
          local.get 5
          local.get 8
          i32.const 1050836
          call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
          unreachable
        end
        i32.const 1052512
        i32.const 35
        i32.const 1052580
        call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
        unreachable
      end
      local.get 5
      local.get 3
      i32.const 1050836
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 4
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN3std2io5Write9write_fmt17h44e9ca74e44479e5E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 3
    i32.store8 offset=12
    local.get 3
    local.get 1
    i32.store offset=8
    local.get 3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    local.get 2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.get 2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    local.get 2
    i64.load align=4
    i64.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.const 8
              i32.add
              i32.const 1050908
              local.get 3
              i32.const 24
              i32.add
              call $_ZN4core3fmt5write17h1626e57fa473d161E
              i32.eqz
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 3
                i32.load8_u offset=12
                i32.const 3
                i32.ne
                br_if 0 (;@6;)
                i32.const 15
                i32.const 1
                call $__rust_alloc
                local.tee 2
                i32.eqz
                br_if 2 (;@4;)
                local.get 2
                i32.const 7
                i32.add
                i32.const 0
                i64.load offset=1050899 align=1
                i64.store align=1
                local.get 2
                i32.const 0
                i64.load offset=1050892 align=1
                i64.store align=1
                i32.const 12
                i32.const 4
                call $__rust_alloc
                local.tee 1
                i32.eqz
                br_if 3 (;@3;)
                local.get 1
                i64.const 64424509455
                i64.store offset=4 align=4
                local.get 1
                local.get 2
                i32.store
                i32.const 12
                i32.const 4
                call $__rust_alloc
                local.tee 2
                i32.eqz
                br_if 4 (;@2;)
                local.get 2
                i32.const 16
                i32.store8 offset=8
                local.get 2
                i32.const 1049980
                i32.store offset=4
                local.get 2
                local.get 1
                i32.store
                local.get 2
                local.get 3
                i32.load16_u offset=24 align=1
                i32.store16 offset=9 align=1
                local.get 2
                i32.const 11
                i32.add
                local.get 3
                i32.const 24
                i32.add
                i32.const 2
                i32.add
                i32.load8_u
                i32.store8
                local.get 0
                i32.const 4
                i32.add
                local.get 2
                i32.store
                local.get 0
                i32.const 2
                i32.store
                br 5 (;@1;)
              end
              local.get 0
              local.get 3
              i64.load offset=12 align=4
              i64.store align=4
              br 4 (;@1;)
            end
            local.get 0
            i32.const 3
            i32.store8
            block  ;; label = @5
              i32.const 0
              br_if 0 (;@5;)
              local.get 3
              i32.load8_u offset=12
              i32.const 2
              i32.ne
              br_if 4 (;@1;)
            end
            local.get 3
            i32.const 16
            i32.add
            i32.load
            local.tee 2
            i32.load
            local.get 2
            i32.load offset=4
            i32.load
            call_indirect (type 1)
            block  ;; label = @5
              local.get 2
              i32.load offset=4
              local.tee 0
              i32.load offset=4
              local.tee 1
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.get 1
              local.get 0
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get 3
            i32.load offset=16
            i32.const 12
            i32.const 4
            call $__rust_dealloc
            br 3 (;@1;)
          end
          i32.const 15
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        i32.const 12
        i32.const 4
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 3
    i32.const 48
    i32.add
    global.set 0)
  (func $_ZN3std10sys_common11at_exit_imp4push17h5823376636150c69E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.load8_u offset=1058849
          br_if 0 (;@3;)
          i32.const 0
          i32.const 1
          i32.store8 offset=1058849
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i32.load offset=1058760
                local.tee 2
                br_table 0 (;@6;) 2 (;@4;) 1 (;@5;)
              end
              i32.const 12
              i32.const 4
              call $__rust_alloc
              local.tee 2
              i32.eqz
              br_if 3 (;@2;)
              local.get 2
              i32.const 0
              i32.store offset=8
              local.get 2
              i64.const 4
              i64.store align=4
              i32.const 0
              local.get 2
              i32.store offset=1058760
            end
            block  ;; label = @5
              local.get 2
              i32.load offset=8
              local.tee 3
              local.get 2
              i32.const 4
              i32.add
              i32.load
              i32.eq
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.set 4
              br 4 (;@1;)
            end
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                i32.const 1
                i32.add
                local.tee 4
                local.get 3
                i32.lt_u
                br_if 0 (;@6;)
                local.get 3
                i32.const 1
                i32.shl
                local.tee 5
                local.get 4
                local.get 5
                local.get 4
                i32.gt_u
                select
                local.tee 4
                i32.const 4
                local.get 4
                i32.const 4
                i32.gt_u
                select
                local.tee 4
                i32.const 536870911
                i32.and
                local.tee 6
                local.get 4
                i32.ne
                br_if 0 (;@6;)
                local.get 4
                i32.const 3
                i32.shl
                local.tee 5
                i32.const 0
                i32.lt_s
                br_if 0 (;@6;)
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 2
                      i32.load
                      i32.const 0
                      local.get 3
                      select
                      local.tee 7
                      br_if 0 (;@9;)
                      local.get 6
                      local.get 4
                      i32.eq
                      i32.const 2
                      i32.shl
                      local.set 4
                      local.get 5
                      i32.eqz
                      br_if 1 (;@8;)
                      local.get 5
                      local.get 4
                      call $__rust_alloc
                      local.set 4
                      br 1 (;@8;)
                    end
                    block  ;; label = @9
                      local.get 3
                      i32.const 3
                      i32.shl
                      local.tee 4
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 7
                      local.get 4
                      i32.const 4
                      local.get 5
                      call $__rust_realloc
                      local.set 4
                      br 1 (;@8;)
                    end
                    block  ;; label = @9
                      local.get 5
                      br_if 0 (;@9;)
                      i32.const 4
                      local.set 4
                      br 2 (;@7;)
                    end
                    local.get 5
                    i32.const 4
                    call $__rust_alloc
                    local.set 4
                  end
                  local.get 4
                  i32.eqz
                  br_if 2 (;@5;)
                  local.get 2
                  i32.load offset=8
                  local.set 3
                end
                local.get 2
                local.get 4
                i32.store
                local.get 2
                i32.const 4
                i32.add
                local.get 5
                i32.const 3
                i32.shr_u
                i32.store
                br 5 (;@1;)
              end
              call $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E
              unreachable
            end
            local.get 5
            i32.const 4
            call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
            unreachable
          end
          i32.const 0
          i32.const 0
          i32.store8 offset=1058849
          local.get 0
          local.get 1
          i32.load
          call_indirect (type 1)
          block  ;; label = @4
            local.get 1
            i32.load offset=4
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            local.get 2
            local.get 1
            i32.load offset=8
            call $__rust_dealloc
          end
          i32.const 0
          return
        end
        i32.const 1052596
        i32.const 32
        i32.const 1052676
        call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 4
    local.get 3
    i32.const 3
    i32.shl
    i32.add
    local.tee 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 2
    local.get 2
    i32.load offset=8
    i32.const 1
    i32.add
    i32.store offset=8
    i32.const 0
    i32.const 0
    i32.store8 offset=1058849
    i32.const 1)
  (func $_ZN3std2io5stdio6stdout17hfb1b82be3b7f9203E (type 10) (result i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 0
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  i32.const 0
                  i32.load8_u offset=1058756
                  br_if 0 (;@7;)
                  i32.const 0
                  i32.const 1
                  i32.store8 offset=1058756
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1058752
                          local.tee 1
                          br_table 1 (;@10;) 0 (;@11;) 2 (;@9;)
                        end
                        i32.const 0
                        i32.const 0
                        i32.store8 offset=1058756
                        br 9 (;@1;)
                      end
                      i32.const 4
                      i32.const 4
                      call $__rust_alloc
                      local.tee 1
                      i32.eqz
                      br_if 3 (;@6;)
                      local.get 1
                      i32.const 1058752
                      i32.store
                      local.get 1
                      i32.const 1051772
                      call $_ZN3std10sys_common11at_exit_imp4push17h5823376636150c69E
                      local.set 2
                      i32.const 1024
                      i32.const 1
                      call $__rust_alloc
                      local.tee 3
                      i32.eqz
                      br_if 4 (;@5;)
                      local.get 0
                      i32.const 10
                      i32.add
                      i32.const 2
                      i32.add
                      local.tee 4
                      local.get 0
                      i32.const 13
                      i32.add
                      i32.const 2
                      i32.add
                      i32.load8_u
                      i32.store8
                      local.get 0
                      local.get 0
                      i32.load16_u offset=13 align=1
                      i32.store16 offset=10
                      i32.const 32
                      i32.const 4
                      call $__rust_alloc
                      local.tee 1
                      i32.eqz
                      br_if 5 (;@4;)
                      local.get 1
                      i32.const 0
                      i32.store8 offset=28
                      local.get 1
                      i32.const 1
                      i32.store16 offset=24
                      local.get 1
                      i64.const 1024
                      i64.store offset=16 align=4
                      local.get 1
                      local.get 3
                      i32.store offset=12
                      local.get 1
                      i32.const 0
                      i32.store offset=8
                      local.get 1
                      i64.const 4294967297
                      i64.store align=4
                      local.get 1
                      local.get 0
                      i32.load16_u offset=10
                      i32.store16 offset=29 align=1
                      local.get 1
                      i32.const 31
                      i32.add
                      local.get 4
                      i32.load8_u
                      i32.store8
                      local.get 2
                      i32.eqz
                      br_if 1 (;@8;)
                      local.get 1
                      local.get 1
                      i32.load
                      local.tee 2
                      i32.const 1
                      i32.add
                      i32.store
                      local.get 2
                      i32.const -1
                      i32.le_s
                      br_if 6 (;@3;)
                      i32.const 4
                      i32.const 4
                      call $__rust_alloc
                      local.tee 2
                      i32.eqz
                      br_if 7 (;@2;)
                      i32.const 0
                      local.get 2
                      i32.store offset=1058752
                      local.get 2
                      local.get 1
                      i32.store
                      br 1 (;@8;)
                    end
                    local.get 1
                    i32.load
                    local.tee 1
                    local.get 1
                    i32.load
                    local.tee 2
                    i32.const 1
                    i32.add
                    i32.store
                    local.get 2
                    i32.const -1
                    i32.le_s
                    br_if 5 (;@3;)
                  end
                  i32.const 0
                  i32.const 0
                  i32.store8 offset=1058756
                  local.get 1
                  i32.eqz
                  br_if 6 (;@1;)
                  local.get 0
                  i32.const 16
                  i32.add
                  global.set 0
                  local.get 1
                  return
                end
                i32.const 1052596
                i32.const 32
                i32.const 1052676
                call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
                unreachable
              end
              i32.const 4
              i32.const 4
              call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
              unreachable
            end
            i32.const 1024
            i32.const 1
            call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
            unreachable
          end
          i32.const 32
          i32.const 4
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        unreachable
        unreachable
      end
      i32.const 4
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    i32.const 1050631
    i32.const 36
    i32.const 1050668
    call $_ZN4core6option13expect_failed17h6275fb248fa0b383E
    unreachable)
  (func $_ZN57_$LT$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17h3e93df57d79d3c72E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.load
    i32.const 8
    i32.add
    i32.store offset=4
    local.get 3
    i32.const 3
    i32.store8 offset=12
    local.get 3
    local.get 3
    i32.const 4
    i32.add
    i32.store offset=8
    local.get 3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    local.get 2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.get 2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    local.get 2
    i64.load align=4
    i64.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.const 8
              i32.add
              i32.const 1050868
              local.get 3
              i32.const 24
              i32.add
              call $_ZN4core3fmt5write17h1626e57fa473d161E
              i32.eqz
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 3
                i32.load8_u offset=12
                i32.const 3
                i32.ne
                br_if 0 (;@6;)
                i32.const 15
                i32.const 1
                call $__rust_alloc
                local.tee 2
                i32.eqz
                br_if 3 (;@3;)
                local.get 2
                i32.const 7
                i32.add
                i32.const 0
                i64.load offset=1050899 align=1
                i64.store align=1
                local.get 2
                i32.const 0
                i64.load offset=1050892 align=1
                i64.store align=1
                i32.const 12
                i32.const 4
                call $__rust_alloc
                local.tee 1
                i32.eqz
                br_if 4 (;@2;)
                local.get 1
                i64.const 64424509455
                i64.store offset=4 align=4
                local.get 1
                local.get 2
                i32.store
                i32.const 12
                i32.const 4
                call $__rust_alloc
                local.tee 2
                i32.eqz
                br_if 5 (;@1;)
                local.get 2
                i32.const 16
                i32.store8 offset=8
                local.get 2
                i32.const 1049980
                i32.store offset=4
                local.get 2
                local.get 1
                i32.store
                local.get 2
                local.get 3
                i32.load16_u offset=24 align=1
                i32.store16 offset=9 align=1
                local.get 2
                i32.const 11
                i32.add
                local.get 3
                i32.const 24
                i32.add
                i32.const 2
                i32.add
                i32.load8_u
                i32.store8
                local.get 0
                i32.const 4
                i32.add
                local.get 2
                i32.store
                local.get 0
                i32.const 2
                i32.store
                br 2 (;@4;)
              end
              local.get 0
              local.get 3
              i64.load offset=12 align=4
              i64.store align=4
              br 1 (;@4;)
            end
            local.get 0
            i32.const 3
            i32.store8
            block  ;; label = @5
              i32.const 0
              br_if 0 (;@5;)
              local.get 3
              i32.load8_u offset=12
              i32.const 2
              i32.ne
              br_if 1 (;@4;)
            end
            local.get 3
            i32.const 16
            i32.add
            i32.load
            local.tee 2
            i32.load
            local.get 2
            i32.load offset=4
            i32.load
            call_indirect (type 1)
            block  ;; label = @5
              local.get 2
              i32.load offset=4
              local.tee 0
              i32.load offset=4
              local.tee 1
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.get 1
              local.get 0
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get 3
            i32.load offset=16
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          local.get 3
          i32.const 48
          i32.add
          global.set 0
          return
        end
        i32.const 15
        i32.const 1
        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
        unreachable
      end
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    i32.const 12
    i32.const 4
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN3std4sync4once4Once10call_inner17h7c0ab88e215d467dE (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 4
    global.set 0
    local.get 4
    i32.const 16
    i32.add
    i32.const 2
    i32.or
    local.set 5
    local.get 0
    i32.load
    local.set 6
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 1
                br_if 0 (;@6;)
                br 1 (;@5;)
              end
              loop  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 6
                    local.tee 1
                    br_table 0 (;@8;) 0 (;@8;) 1 (;@7;) 6 (;@2;) 1 (;@7;)
                  end
                  local.get 0
                  i32.const 2
                  local.get 0
                  i32.load
                  local.tee 6
                  local.get 6
                  local.get 1
                  i32.eq
                  local.tee 7
                  select
                  i32.store
                  local.get 7
                  i32.eqz
                  br_if 1 (;@6;)
                  br 4 (;@3;)
                end
                local.get 1
                i32.const 3
                i32.and
                i32.const 2
                i32.ne
                br_if 2 (;@4;)
                block  ;; label = @7
                  loop  ;; label = @8
                    local.get 1
                    local.set 6
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1058824
                      i32.const 1
                      i32.eq
                      br_if 0 (;@9;)
                      i32.const 0
                      i64.const 1
                      i64.store offset=1058824 align=4
                      i32.const 0
                      i32.const 0
                      i32.store offset=1058832
                    end
                    i32.const 1058828
                    call $_ZN3std10sys_common11thread_info10ThreadInfo4with28_$u7b$$u7b$closure$u7d$$u7d$17h74ef4e5ac2243bc9E
                    local.set 7
                    local.get 0
                    local.get 5
                    local.get 0
                    i32.load
                    local.tee 1
                    local.get 1
                    local.get 6
                    i32.eq
                    local.tee 8
                    select
                    i32.store
                    local.get 4
                    i32.const 0
                    i32.store8 offset=24
                    local.get 4
                    local.get 7
                    i32.store offset=16
                    local.get 4
                    local.get 6
                    i32.const -4
                    i32.and
                    i32.store offset=20
                    block  ;; label = @9
                      local.get 8
                      br_if 0 (;@9;)
                      block  ;; label = @10
                        local.get 4
                        i32.load offset=16
                        local.tee 6
                        i32.eqz
                        br_if 0 (;@10;)
                        local.get 6
                        local.get 6
                        i32.load
                        local.tee 7
                        i32.const -1
                        i32.add
                        i32.store
                        local.get 7
                        i32.const 1
                        i32.ne
                        br_if 0 (;@10;)
                        local.get 4
                        i32.const 16
                        i32.add
                        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
                      end
                      local.get 1
                      i32.const 3
                      i32.and
                      i32.const 2
                      i32.eq
                      br_if 1 (;@8;)
                      br 2 (;@7;)
                    end
                  end
                  block  ;; label = @8
                    local.get 4
                    i32.load8_u offset=24
                    br_if 0 (;@8;)
                    loop  ;; label = @9
                      call $_ZN3std6thread4park17hed7c6d597e3e40eaE
                      local.get 4
                      i32.load8_u offset=24
                      i32.eqz
                      br_if 0 (;@9;)
                    end
                  end
                  local.get 4
                  i32.load offset=16
                  local.tee 1
                  i32.eqz
                  br_if 0 (;@7;)
                  local.get 1
                  local.get 1
                  i32.load
                  local.tee 6
                  i32.const -1
                  i32.add
                  i32.store
                  local.get 6
                  i32.const 1
                  i32.ne
                  br_if 0 (;@7;)
                  local.get 4
                  i32.const 16
                  i32.add
                  call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
                end
                local.get 0
                i32.load
                local.set 6
                br 0 (;@6;)
              end
            end
            loop  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 6
                  br_table 0 (;@7;) 6 (;@1;) 1 (;@6;) 5 (;@2;) 1 (;@6;)
                end
                local.get 0
                local.get 0
                i32.load
                local.tee 6
                i32.const 2
                local.get 6
                select
                i32.store
                local.get 6
                br_if 1 (;@5;)
                i32.const 0
                local.set 1
                br 3 (;@3;)
              end
              local.get 6
              i32.const 3
              i32.and
              i32.const 2
              i32.ne
              br_if 1 (;@4;)
              block  ;; label = @6
                block  ;; label = @7
                  loop  ;; label = @8
                    local.get 6
                    local.set 1
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1058824
                      i32.const 1
                      i32.eq
                      br_if 0 (;@9;)
                      i32.const 0
                      i64.const 1
                      i64.store offset=1058824 align=4
                      i32.const 0
                      i32.const 0
                      i32.store offset=1058832
                    end
                    i32.const 1058828
                    call $_ZN3std10sys_common11thread_info10ThreadInfo4with28_$u7b$$u7b$closure$u7d$$u7d$17h74ef4e5ac2243bc9E
                    local.set 7
                    local.get 0
                    local.get 5
                    local.get 0
                    i32.load
                    local.tee 6
                    local.get 6
                    local.get 1
                    i32.eq
                    select
                    i32.store
                    local.get 4
                    i32.const 0
                    i32.store8 offset=24
                    local.get 4
                    local.get 7
                    i32.store offset=16
                    local.get 4
                    local.get 1
                    i32.const -4
                    i32.and
                    i32.store offset=20
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 6
                        local.get 1
                        i32.ne
                        br_if 0 (;@10;)
                        local.get 4
                        i32.load8_u offset=24
                        i32.eqz
                        br_if 1 (;@9;)
                        br 3 (;@7;)
                      end
                      block  ;; label = @10
                        local.get 4
                        i32.load offset=16
                        local.tee 1
                        i32.eqz
                        br_if 0 (;@10;)
                        local.get 1
                        local.get 1
                        i32.load
                        local.tee 7
                        i32.const -1
                        i32.add
                        i32.store
                        local.get 7
                        i32.const 1
                        i32.ne
                        br_if 0 (;@10;)
                        local.get 4
                        i32.const 16
                        i32.add
                        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
                      end
                      local.get 6
                      i32.const 3
                      i32.and
                      i32.const 2
                      i32.eq
                      br_if 1 (;@8;)
                      br 3 (;@6;)
                    end
                  end
                  loop  ;; label = @8
                    call $_ZN3std6thread4park17hed7c6d597e3e40eaE
                    local.get 4
                    i32.load8_u offset=24
                    i32.eqz
                    br_if 0 (;@8;)
                  end
                end
                local.get 4
                i32.load offset=16
                local.tee 1
                i32.eqz
                br_if 0 (;@6;)
                local.get 1
                local.get 1
                i32.load
                local.tee 6
                i32.const -1
                i32.add
                i32.store
                local.get 6
                i32.const 1
                i32.ne
                br_if 0 (;@6;)
                local.get 4
                i32.const 16
                i32.add
                call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
              end
              local.get 0
              i32.load
              local.set 6
              br 0 (;@5;)
            end
          end
          i32.const 1051100
          i32.const 57
          i32.const 1051160
          call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
          unreachable
        end
        local.get 4
        local.get 0
        i32.store offset=8
        local.get 4
        i32.const 3
        i32.store offset=16
        local.get 4
        local.get 1
        i32.const 1
        i32.eq
        i32.store8 offset=20
        local.get 2
        local.get 4
        i32.const 16
        i32.add
        local.get 3
        i32.load offset=12
        call_indirect (type 6)
        local.get 4
        local.get 4
        i32.load offset=16
        i32.store offset=12
        local.get 4
        i32.const 8
        i32.add
        call $_ZN70_$LT$std..sync..once..WaiterQueue$u20$as$u20$core..ops..drop..Drop$GT$4drop17ha86032f5632a1e79E
      end
      local.get 4
      i32.const 32
      i32.add
      global.set 0
      return
    end
    i32.const 1051176
    i32.const 42
    i32.const 1051220
    call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
    unreachable)
  (func $_ZN3std2io5stdio9set_panic17hdc080de43bd7c368E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    i32.const 0
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058808
          i32.const 1
          i32.eq
          br_if 0 (;@3;)
          i32.const 0
          i64.const 1
          i64.store offset=1058808 align=4
          i32.const 0
          i32.const 0
          i32.store offset=1058816
          br 1 (;@2;)
        end
        i32.const 0
        i32.load offset=1058812
        br_if 1 (;@1;)
        i32.const 0
        i32.load offset=1058816
        local.set 4
      end
      i32.const 0
      local.get 1
      i32.store offset=1058816
      i32.const 0
      i32.load offset=1058820
      local.set 1
      i32.const 0
      local.get 2
      i32.store offset=1058820
      i32.const 0
      i32.const 0
      i32.store offset=1058812
      block  ;; label = @2
        local.get 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 4
        local.get 1
        i32.load offset=24
        call_indirect (type 6)
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 3
          i32.load8_u
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 3
        i32.load offset=4
        local.tee 2
        i32.load
        local.get 2
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 2
          i32.load offset=4
          local.tee 5
          i32.load offset=4
          local.tee 6
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          i32.load
          local.get 6
          local.get 5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 2
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 1
      i32.store offset=4
      local.get 0
      local.get 4
      i32.store
      local.get 3
      i32.const 16
      i32.add
      global.set 0
      return
    end
    i32.const 1049128
    i32.const 16
    local.get 3
    i32.const 8
    i32.add
    i32.const 1049328
    i32.const 1050700
    call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
    unreachable)
  (func $_ZN3std2io5stdio6_print17h5ab2b84f030f9b17E (type 1) (param i32)
    (local i32 i32 i64 i32 i32)
    global.get 0
    i32.const 96
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 1
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 1
    local.get 0
    i64.load align=4
    i64.store offset=8
    local.get 1
    i32.const 6
    i32.store offset=36
    local.get 1
    i32.const 1050804
    i32.store offset=32
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1058792
                  i32.const 1
                  i32.eq
                  br_if 0 (;@7;)
                  i32.const 0
                  i64.const 1
                  i64.store offset=1058792 align=4
                  i32.const 0
                  i32.const 0
                  i32.store offset=1058800
                  br 1 (;@6;)
                end
                i32.const 0
                i32.load offset=1058796
                br_if 3 (;@3;)
                i32.const 0
                i32.const 0
                i32.store offset=1058796
                i32.const 0
                i32.load offset=1058800
                local.set 0
                i32.const 0
                i32.const 0
                i32.store offset=1058800
                local.get 0
                br_if 1 (;@5;)
              end
              local.get 1
              call $_ZN3std2io5stdio6stdout17hfb1b82be3b7f9203E
              local.tee 0
              i32.store offset=48
              local.get 1
              i32.const 72
              i32.add
              i32.const 16
              i32.add
              local.get 1
              i32.const 8
              i32.add
              i32.const 16
              i32.add
              i64.load
              i64.store
              local.get 1
              i32.const 72
              i32.add
              i32.const 8
              i32.add
              local.get 1
              i32.const 8
              i32.add
              i32.const 8
              i32.add
              i64.load
              i64.store
              local.get 1
              local.get 1
              i64.load offset=8
              i64.store offset=72
              local.get 1
              i32.const 64
              i32.add
              local.get 1
              i32.const 48
              i32.add
              local.get 1
              i32.const 72
              i32.add
              call $_ZN57_$LT$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17h3e93df57d79d3c72E
              local.get 0
              local.get 0
              i32.load
              local.tee 2
              i32.const -1
              i32.add
              i32.store
              block  ;; label = @6
                local.get 2
                i32.const 1
                i32.ne
                br_if 0 (;@6;)
                local.get 1
                i32.const 48
                i32.add
                call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h5481941dbae3eca9E
              end
              local.get 1
              i64.load offset=64
              local.set 3
              br 1 (;@4;)
            end
            i32.const 0
            i32.load offset=1058804
            local.set 2
            local.get 1
            i32.const 72
            i32.add
            i32.const 16
            i32.add
            local.get 1
            i32.const 8
            i32.add
            i32.const 16
            i32.add
            i64.load
            i64.store
            local.get 1
            i32.const 72
            i32.add
            i32.const 8
            i32.add
            local.get 1
            i32.const 8
            i32.add
            i32.const 8
            i32.add
            i64.load
            i64.store
            local.get 1
            local.get 1
            i64.load offset=8
            i64.store offset=72
            local.get 1
            i32.const 48
            i32.add
            local.get 0
            local.get 1
            i32.const 72
            i32.add
            local.get 2
            i32.load offset=36
            call_indirect (type 7)
            i32.const 0
            i32.load offset=1058796
            br_if 2 (;@2;)
            i32.const 0
            i32.const -1
            i32.store offset=1058796
            block  ;; label = @5
              i32.const 0
              i32.load offset=1058800
              local.tee 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 4
              i32.const 0
              i32.load offset=1058804
              i32.load
              call_indirect (type 1)
              i32.const 0
              i32.load offset=1058804
              local.tee 4
              i32.load offset=4
              local.tee 5
              i32.eqz
              br_if 0 (;@5;)
              i32.const 0
              i32.load offset=1058800
              local.get 5
              local.get 4
              i32.load offset=8
              call $__rust_dealloc
            end
            i32.const 0
            local.get 2
            i32.store offset=1058804
            i32.const 0
            local.get 0
            i32.store offset=1058800
            i32.const 0
            i32.const 0
            i32.load offset=1058796
            i32.const 1
            i32.add
            i32.store offset=1058796
            local.get 1
            local.get 1
            i64.load offset=48
            local.tee 3
            i64.store offset=64
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.wrap_i64
              local.tee 0
              i32.const 255
              i32.and
              i32.const 4
              i32.ne
              br_if 0 (;@5;)
              local.get 1
              call $_ZN3std2io5stdio6stdout17hfb1b82be3b7f9203E
              local.tee 0
              i32.store offset=48
              local.get 1
              i32.const 72
              i32.add
              i32.const 16
              i32.add
              local.get 1
              i32.const 8
              i32.add
              i32.const 16
              i32.add
              i64.load
              i64.store
              local.get 1
              i32.const 72
              i32.add
              i32.const 8
              i32.add
              local.get 1
              i32.const 8
              i32.add
              i32.const 8
              i32.add
              i64.load
              i64.store
              local.get 1
              local.get 1
              i64.load offset=8
              i64.store offset=72
              local.get 1
              i32.const 40
              i32.add
              local.get 1
              i32.const 48
              i32.add
              local.get 1
              i32.const 72
              i32.add
              call $_ZN57_$LT$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17h3e93df57d79d3c72E
              local.get 0
              local.get 0
              i32.load
              local.tee 2
              i32.const -1
              i32.add
              i32.store
              block  ;; label = @6
                local.get 2
                i32.const 1
                i32.ne
                br_if 0 (;@6;)
                local.get 1
                i32.const 48
                i32.add
                call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h5481941dbae3eca9E
              end
              local.get 1
              i32.load8_u offset=40
              local.tee 2
              local.set 0
              br 1 (;@4;)
            end
            local.get 1
            local.get 3
            i64.store offset=40
            local.get 3
            i32.wrap_i64
            local.set 2
          end
          local.get 0
          i32.const 255
          i32.and
          i32.const 3
          i32.ne
          br_if 2 (;@1;)
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              br_if 0 (;@5;)
              local.get 2
              i32.const 3
              i32.and
              i32.const 2
              i32.ne
              br_if 1 (;@4;)
            end
            local.get 1
            i32.load offset=44
            local.tee 0
            i32.load
            local.get 0
            i32.load offset=4
            i32.load
            call_indirect (type 1)
            block  ;; label = @5
              local.get 0
              i32.load offset=4
              local.tee 2
              i32.load offset=4
              local.tee 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 0
              i32.load
              local.get 4
              local.get 2
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get 0
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          local.get 1
          i32.const 96
          i32.add
          global.set 0
          return
        end
        i32.const 1049128
        i32.const 16
        local.get 1
        i32.const 72
        i32.add
        i32.const 1049328
        i32.const 1050772
        call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
        unreachable
      end
      i32.const 1049128
      i32.const 16
      local.get 1
      i32.const 72
      i32.add
      i32.const 1049328
      i32.const 1050788
      call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
      unreachable
    end
    local.get 1
    local.get 1
    i64.load offset=40
    i64.store offset=64
    local.get 1
    i32.const 92
    i32.add
    i32.const 2
    i32.store
    local.get 1
    i32.const 60
    i32.add
    i32.const 12
    i32.store
    local.get 1
    i64.const 2
    i64.store offset=76 align=4
    local.get 1
    i32.const 1050740
    i32.store offset=72
    local.get 1
    i32.const 14
    i32.store offset=52
    local.get 1
    local.get 1
    i32.const 48
    i32.add
    i32.store offset=88
    local.get 1
    local.get 1
    i32.const 64
    i32.add
    i32.store offset=56
    local.get 1
    local.get 1
    i32.const 32
    i32.add
    i32.store offset=48
    local.get 1
    i32.const 72
    i32.add
    i32.const 1050756
    call $_ZN3std9panicking15begin_panic_fmt17h0c385246197edc82E
    unreachable)
  (func $_ZN3std2io5Write18write_all_vectored17hb3da9e95bb72fe29E (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          br_if 0 (;@3;)
          i32.const 0
          local.set 5
          br 1 (;@2;)
        end
        local.get 2
        i32.const 4
        i32.add
        local.set 6
        local.get 3
        i32.const 3
        i32.shl
        i32.const -8
        i32.add
        i32.const 3
        i32.shr_u
        i32.const 1
        i32.add
        local.set 7
        i32.const 0
        local.set 5
        block  ;; label = @3
          loop  ;; label = @4
            local.get 6
            i32.load
            br_if 1 (;@3;)
            local.get 6
            i32.const 8
            i32.add
            local.set 6
            local.get 7
            local.get 5
            i32.const 1
            i32.add
            local.tee 5
            i32.ne
            br_if 0 (;@4;)
          end
          local.get 7
          local.set 5
        end
        local.get 5
        local.get 3
        i32.gt_u
        br_if 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          local.get 5
          i32.sub
          local.tee 8
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          local.get 5
          i32.const 3
          i32.shl
          i32.add
          local.set 9
          loop  ;; label = @4
            local.get 4
            local.get 1
            i32.load
            local.get 9
            local.get 8
            local.get 1
            i32.load offset=4
            i32.load offset=16
            call_indirect (type 4)
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 4
                              i32.load
                              local.tee 10
                              i32.const 1
                              i32.eq
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                local.get 4
                                i32.load offset=4
                                local.tee 2
                                br_if 0 (;@14;)
                                i32.const 28
                                i32.const 1
                                call $__rust_alloc
                                local.tee 6
                                i32.eqz
                                br_if 2 (;@12;)
                                local.get 6
                                i32.const 24
                                i32.add
                                i32.const 0
                                i32.load offset=1050600 align=1
                                i32.store align=1
                                local.get 6
                                i32.const 16
                                i32.add
                                i32.const 0
                                i64.load offset=1050592 align=1
                                i64.store align=1
                                local.get 6
                                i32.const 8
                                i32.add
                                i32.const 0
                                i64.load offset=1050584 align=1
                                i64.store align=1
                                local.get 6
                                i32.const 0
                                i64.load offset=1050576 align=1
                                i64.store align=1
                                i32.const 12
                                i32.const 4
                                call $__rust_alloc
                                local.tee 5
                                i32.eqz
                                br_if 3 (;@11;)
                                local.get 5
                                i64.const 120259084316
                                i64.store offset=4 align=4
                                local.get 5
                                local.get 6
                                i32.store
                                i32.const 12
                                i32.const 4
                                call $__rust_alloc
                                local.tee 6
                                i32.eqz
                                br_if 4 (;@10;)
                                local.get 6
                                i32.const 14
                                i32.store8 offset=8
                                local.get 6
                                i32.const 1049980
                                i32.store offset=4
                                local.get 6
                                local.get 5
                                i32.store
                                local.get 6
                                local.get 4
                                i32.load16_u offset=13 align=1
                                i32.store16 offset=9 align=1
                                local.get 6
                                i32.const 11
                                i32.add
                                local.get 4
                                i32.const 13
                                i32.add
                                i32.const 2
                                i32.add
                                i32.load8_u
                                i32.store8
                                local.get 0
                                i32.const 4
                                i32.add
                                local.get 6
                                i32.store
                                local.get 0
                                i32.const 2
                                i32.store
                                br 12 (;@2;)
                              end
                              local.get 9
                              i32.const 4
                              i32.add
                              local.set 6
                              local.get 8
                              i32.const 3
                              i32.shl
                              i32.const -8
                              i32.add
                              i32.const 3
                              i32.shr_u
                              i32.const 1
                              i32.add
                              local.set 11
                              i32.const 0
                              local.set 5
                              i32.const 0
                              local.set 7
                              loop  ;; label = @14
                                local.get 6
                                i32.load
                                local.get 7
                                i32.add
                                local.tee 3
                                local.get 2
                                i32.gt_u
                                br_if 5 (;@9;)
                                local.get 6
                                i32.const 8
                                i32.add
                                local.set 6
                                local.get 3
                                local.set 7
                                local.get 11
                                local.get 5
                                i32.const 1
                                i32.add
                                local.tee 5
                                i32.ne
                                br_if 0 (;@14;)
                              end
                              local.get 3
                              local.set 7
                              local.get 11
                              local.set 5
                              br 4 (;@9;)
                            end
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    local.get 4
                                    i32.load8_u offset=4
                                    local.tee 2
                                    br_table 1 (;@15;) 0 (;@16;) 2 (;@14;) 1 (;@15;)
                                  end
                                  local.get 4
                                  i32.load8_u offset=5
                                  local.set 6
                                  br 2 (;@13;)
                                end
                                local.get 4
                                i32.load offset=8
                                call $_ZN3std3sys4wasi17decode_error_kind17h06c50c94aefe6e05E
                                i32.const 255
                                i32.and
                                local.set 6
                                br 1 (;@13;)
                              end
                              local.get 4
                              i32.load offset=8
                              i32.load8_u offset=8
                              local.set 6
                            end
                            i32.const 1
                            local.set 10
                            local.get 6
                            i32.const 255
                            i32.and
                            i32.const 15
                            i32.eq
                            br_if 4 (;@8;)
                            local.get 0
                            local.get 4
                            i64.load offset=4 align=4
                            i64.store align=4
                            br 10 (;@2;)
                          end
                          i32.const 28
                          i32.const 1
                          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                          unreachable
                        end
                        i32.const 12
                        i32.const 4
                        call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                        unreachable
                      end
                      i32.const 12
                      i32.const 4
                      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
                      unreachable
                    end
                    local.get 8
                    local.get 5
                    i32.lt_u
                    br_if 1 (;@7;)
                    local.get 9
                    local.get 5
                    i32.const 3
                    i32.shl
                    i32.add
                    local.set 9
                    local.get 8
                    local.get 5
                    i32.sub
                    local.tee 8
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 9
                    i32.load offset=4
                    local.tee 5
                    local.get 2
                    local.get 7
                    i32.sub
                    local.tee 6
                    i32.lt_u
                    br_if 2 (;@6;)
                    local.get 9
                    i32.const 4
                    i32.add
                    local.get 5
                    local.get 6
                    i32.sub
                    i32.store
                    local.get 9
                    local.get 9
                    i32.load
                    local.get 6
                    i32.add
                    i32.store
                    local.get 4
                    i32.load8_u offset=4
                    local.set 2
                    local.get 4
                    i32.load
                    local.set 10
                  end
                  local.get 10
                  i32.eqz
                  br_if 2 (;@5;)
                  local.get 2
                  i32.const 255
                  i32.and
                  i32.const 2
                  i32.lt_u
                  br_if 2 (;@5;)
                  local.get 4
                  i32.load offset=8
                  local.tee 6
                  i32.load
                  local.get 6
                  i32.load offset=4
                  i32.load
                  call_indirect (type 1)
                  block  ;; label = @8
                    local.get 6
                    i32.load offset=4
                    local.tee 5
                    i32.load offset=4
                    local.tee 7
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 6
                    i32.load
                    local.get 7
                    local.get 5
                    i32.load offset=8
                    call $__rust_dealloc
                  end
                  local.get 6
                  i32.const 12
                  i32.const 4
                  call $__rust_dealloc
                  br 2 (;@5;)
                end
                local.get 5
                local.get 8
                i32.const 1050836
                call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
                unreachable
              end
              i32.const 1052512
              i32.const 35
              i32.const 1052580
              call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
              unreachable
            end
            local.get 8
            br_if 0 (;@4;)
          end
        end
        local.get 0
        i32.const 3
        i32.store8
      end
      local.get 4
      i32.const 16
      i32.add
      global.set 0
      return
    end
    local.get 5
    local.get 3
    i32.const 1050836
    call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
    unreachable)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adaptor$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hab706aac4ccd318bE (type 8) (param i32 i32 i32) (result i32)
    (local i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h34620f531d384434E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 2
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 2
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 3
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adaptor$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb4871d4b0022c40fE (type 8) (param i32 i32 i32) (result i32)
    (local i32 i64 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN3std2io5Write9write_all17h95d942a560570f98E
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 1
          i32.load offset=4
          local.tee 2
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 5
          local.get 2
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.load offset=8
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 4
      i64.store offset=4 align=4
      i32.const 1
      local.set 1
    end
    local.get 3
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17hd52bd4df9fd4eb9eE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=8
    local.get 1
    call $_ZN73_$LT$std..sys_common..os_str_bytes..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hf2181278eaad9e60E)
  (func $_ZN59_$LT$std..process..ChildStdin$u20$as$u20$std..io..Write$GT$5flush17h5817e30417877f3fE (type 6) (param i32 i32)
    local.get 0
    i32.const 3
    i32.store8)
  (func $_ZN3std7process5abort17h3956b458a2b8fd63E (type 0)
    call $_ZN3std3sys4wasi14abort_internal17hd8cbb8f7e8c57b29E
    unreachable)
  (func $_ZN3std3sys4wasi14abort_internal17hd8cbb8f7e8c57b29E (type 0)
    call $abort
    unreachable)
  (func $_ZN70_$LT$std..sync..once..WaiterQueue$u20$as$u20$core..ops..drop..Drop$GT$4drop17ha86032f5632a1e79E (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 1
    global.set 0
    local.get 0
    i32.load
    local.tee 2
    i32.load
    local.set 3
    local.get 2
    local.get 0
    i32.load offset=4
    i32.store
    local.get 1
    local.get 3
    i32.const 3
    i32.and
    local.tee 0
    i32.store offset=12
    block  ;; label = @1
      local.get 0
      i32.const 2
      i32.ne
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          i32.const -4
          i32.and
          local.tee 0
          i32.eqz
          br_if 0 (;@3;)
          loop  ;; label = @4
            local.get 0
            i32.load offset=4
            local.set 3
            local.get 0
            i32.load
            local.set 2
            local.get 0
            i32.const 0
            i32.store
            local.get 2
            i32.eqz
            br_if 2 (;@2;)
            local.get 0
            i32.const 1
            i32.store8 offset=8
            local.get 1
            local.get 2
            i32.store offset=16
            local.get 1
            i32.const 16
            i32.add
            call $_ZN3std6thread6Thread6unpark17hda97b6cd219be17eE
            local.get 1
            i32.load offset=16
            local.tee 0
            local.get 0
            i32.load
            local.tee 0
            i32.const -1
            i32.add
            i32.store
            block  ;; label = @5
              local.get 0
              i32.const 1
              i32.ne
              br_if 0 (;@5;)
              local.get 1
              i32.const 16
              i32.add
              call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h3cc73ca709c5831aE
            end
            local.get 3
            local.set 0
            local.get 3
            br_if 0 (;@4;)
          end
        end
        local.get 1
        i32.const 64
        i32.add
        global.set 0
        return
      end
      i32.const 1049268
      i32.const 43
      i32.const 1051252
      call $_ZN4core9panicking5panic17heeaec3885c636092E
      unreachable
    end
    local.get 1
    i32.const 52
    i32.add
    i32.const 11
    i32.store
    local.get 1
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    local.get 1
    i64.const 3
    i64.store offset=20 align=4
    local.get 1
    i32.const 1049228
    i32.store offset=16
    local.get 1
    i32.const 11
    i32.store offset=44
    local.get 1
    local.get 1
    i32.const 12
    i32.add
    i32.store offset=56
    local.get 1
    i32.const 1049556
    i32.store offset=60
    local.get 1
    local.get 1
    i32.const 40
    i32.add
    i32.store offset=32
    local.get 1
    local.get 1
    i32.const 60
    i32.add
    i32.store offset=48
    local.get 1
    local.get 1
    i32.const 56
    i32.add
    i32.store offset=40
    local.get 1
    i32.const 16
    i32.add
    i32.const 1051236
    call $_ZN3std9panicking15begin_panic_fmt17h0c385246197edc82E
    unreachable)
  (func $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hc70038f3e0b70d11E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load8_u
    local.set 3
    local.get 2
    i32.const 8
    i32.add
    call $_ZN3std3env11current_dir17h57d19aca547a0569E
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.load offset=8
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        local.get 2
        i32.const 16
        i32.add
        i32.load
        local.set 0
        local.get 2
        i32.load offset=12
        local.set 4
        br 1 (;@1;)
      end
      i32.const 0
      local.set 4
      block  ;; label = @2
        local.get 2
        i32.load8_u offset=12
        i32.const 2
        i32.lt_u
        br_if 0 (;@2;)
        local.get 2
        i32.const 16
        i32.add
        i32.load
        local.tee 0
        i32.load
        local.get 0
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 0
          i32.load offset=4
          local.tee 5
          i32.load offset=4
          local.tee 6
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load
          local.get 6
          local.get 5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
    end
    local.get 2
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get 2
    i32.const 1049252
    i32.store offset=24
    local.get 2
    i64.const 1
    i64.store offset=12 align=4
    local.get 2
    i32.const 1051376
    i32.store offset=8
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        local.get 2
        i32.const 8
        i32.add
        call $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 3
          i32.const 255
          i32.and
          br_if 0 (;@3;)
          local.get 2
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get 2
          i32.const 1049252
          i32.store offset=24
          local.get 2
          i64.const 1
          i64.store offset=12 align=4
          local.get 2
          i32.const 1051472
          i32.store offset=8
          local.get 1
          local.get 2
          i32.const 8
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E
          br_if 1 (;@2;)
        end
        i32.const 0
        local.set 1
        local.get 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        local.get 0
        i32.const 1
        call $__rust_dealloc
        br 1 (;@1;)
      end
      i32.const 1
      local.set 1
      local.get 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 4
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1
      local.set 1
      local.get 4
      local.get 0
      i32.const 1
      call $__rust_dealloc
    end
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17h5116a6a256f0a4bcE (type 1) (param i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i32.load
    i32.store
    local.get 1
    local.get 0
    i64.load align=4
    i64.store
    local.get 1
    call $_ZN3std9panicking11begin_panic28_$u7b$$u7b$closure$u7d$$u7d$17h6fd6f7fd6e085452E
    unreachable)
  (func $_ZN3std9panicking11begin_panic28_$u7b$$u7b$closure$u7d$$u7d$17h6fd6f7fd6e085452E (type 1) (param i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    local.get 0
    i64.load align=4
    i64.store offset=8
    local.get 1
    i32.const 8
    i32.add
    i32.const 1052208
    i32.const 0
    local.get 0
    i32.load offset=8
    call $_ZN3std9panicking20rust_panic_with_hook17h56625e96833ade19E
    unreachable)
  (func $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17h7b78cfba6afb8434E (type 1) (param i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i32.load
    i32.store
    local.get 1
    local.get 0
    i64.load align=4
    i64.store
    local.get 1
    call $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h41d10910709301bfE
    unreachable)
  (func $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h41d10910709301bfE (type 1) (param i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 0
    i32.store offset=4
    local.get 1
    local.get 0
    i32.load
    i32.store
    local.get 1
    i32.const 1052172
    local.get 0
    i32.load offset=4
    call $_ZN4core5panic9PanicInfo7message17h576f4b436581989eE
    local.get 0
    i32.load offset=8
    call $_ZN3std9panicking20rust_panic_with_hook17h56625e96833ade19E
    unreachable)
  (func $_ZN3std3sys4wasi7condvar7Condvar4wait17h30ac79ba8be4b6e9E (type 6) (param i32 i32)
    i32.const 1052420
    i32.const 26
    i32.const 1052496
    call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
    unreachable)
  (func $_ZN82_$LT$std..sys_common..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h442c1cc490bb7041E (type 3) (param i32 i32) (result i32)
    i32.const 1051528
    i32.const 25
    local.get 1
    call $_ZN40_$LT$str$u20$as$u20$core..fmt..Debug$GT$3fmt17h9a17d4d05f37d658E)
  (func $_ZN3std10sys_common4util10dumb_print17h31d401d3573033e3E (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 16
    i32.add
    i32.const 16
    i32.add
    local.get 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 1
    i32.const 16
    i32.add
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 1
    local.get 0
    i64.load align=4
    i64.store offset=16
    local.get 1
    i32.const 8
    i32.add
    local.get 1
    i32.const 40
    i32.add
    local.get 1
    i32.const 16
    i32.add
    call $_ZN3std2io5Write9write_fmt17h44e9ca74e44479e5E
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        local.get 1
        i32.load8_u offset=8
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      local.get 1
      i32.load offset=12
      local.tee 0
      i32.load
      local.get 0
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 0
        i32.load offset=4
        local.tee 2
        i32.load offset=4
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.load
        local.get 3
        local.get 2
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 0
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    local.get 1
    i32.const 48
    i32.add
    global.set 0)
  (func $_ZN3std10sys_common4util5abort17ha9873acc1ebcd69dE (type 1) (param i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 1
    global.set 0
    local.get 1
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get 1
    i64.const 2
    i64.store offset=4 align=4
    local.get 1
    i32.const 1051756
    i32.store
    local.get 1
    i32.const 10
    i32.store offset=28
    local.get 1
    local.get 0
    i32.store offset=24
    local.get 1
    local.get 1
    i32.const 24
    i32.add
    i32.store offset=16
    local.get 1
    call $_ZN3std10sys_common4util10dumb_print17h31d401d3573033e3E
    call $_ZN3std3sys4wasi14abort_internal17hd8cbb8f7e8c57b29E
    unreachable)
  (func $_ZN3std5alloc24default_alloc_error_hook17h3c9cdc508dd65ebfE (type 6) (param i32 i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 17
    i32.store offset=12
    local.get 2
    local.get 0
    i32.store offset=20
    local.get 2
    local.get 2
    i32.const 20
    i32.add
    i32.store offset=8
    local.get 2
    i32.const 52
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=36 align=4
    local.get 2
    i32.const 1051824
    i32.store offset=32
    local.get 2
    local.get 2
    i32.const 8
    i32.add
    i32.store offset=48
    local.get 2
    i32.const 24
    i32.add
    local.get 2
    i32.const 56
    i32.add
    local.get 2
    i32.const 32
    i32.add
    call $_ZN3std2io5Write9write_fmt17h44e9ca74e44479e5E
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        local.get 2
        i32.load8_u offset=24
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      local.get 2
      i32.load offset=28
      local.tee 0
      i32.load
      local.get 0
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 0
        i32.load offset=4
        local.tee 3
        i32.load offset=4
        local.tee 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.load
        local.get 4
        local.get 3
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 0
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    local.get 2
    i32.const 64
    i32.add
    global.set 0)
  (func $rust_oom (type 6) (param i32 i32)
    (local i32)
    local.get 0
    local.get 1
    i32.const 0
    i32.load offset=1058772
    local.tee 2
    i32.const 18
    local.get 2
    select
    call_indirect (type 6)
    call $_ZN3std7process5abort17h3956b458a2b8fd63E
    unreachable)
  (func $__rdl_alloc (type 3) (param i32 i32) (result i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 8
        i32.gt_u
        br_if 0 (;@2;)
        local.get 1
        local.get 0
        i32.le_u
        br_if 1 (;@1;)
      end
      local.get 1
      local.get 0
      call $aligned_alloc
      return
    end
    local.get 0
    call $malloc)
  (func $__rdl_dealloc (type 7) (param i32 i32 i32)
    local.get 0
    call $free)
  (func $__rdl_realloc (type 9) (param i32 i32 i32 i32) (result i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.const 8
        i32.gt_u
        br_if 0 (;@2;)
        local.get 2
        local.get 3
        i32.le_u
        br_if 1 (;@1;)
      end
      block  ;; label = @2
        local.get 2
        local.get 3
        call $aligned_alloc
        local.tee 2
        br_if 0 (;@2;)
        i32.const 0
        return
      end
      local.get 2
      local.get 0
      local.get 3
      local.get 1
      local.get 1
      local.get 3
      i32.gt_u
      select
      call $memcpy
      local.set 3
      local.get 0
      call $free
      local.get 3
      return
    end
    local.get 0
    local.get 3
    call $realloc)
  (func $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h7aaaaa870ab86598E (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 20
    i32.add
    i32.const 3
    i32.store
    local.get 3
    i32.const 32
    i32.add
    i32.const 20
    i32.add
    i32.const 19
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 14
    i32.store
    local.get 3
    i64.const 4
    i64.store offset=4 align=4
    local.get 3
    i32.const 1052020
    i32.store
    local.get 3
    i32.const 14
    i32.store offset=36
    local.get 3
    local.get 0
    i32.load offset=8
    i32.store offset=48
    local.get 3
    local.get 0
    i32.load offset=4
    i32.store offset=40
    local.get 3
    local.get 0
    i32.load
    i32.store offset=32
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    i32.store offset=16
    local.get 3
    i32.const 24
    i32.add
    local.get 1
    local.get 3
    local.get 2
    i32.load offset=36
    local.tee 2
    call_indirect (type 7)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        local.get 3
        i32.load8_u offset=24
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      local.get 3
      i32.load offset=28
      local.tee 4
      i32.load
      local.get 4
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        local.get 4
        i32.load offset=4
        local.tee 5
        i32.load offset=4
        local.tee 6
        i32.eqz
        br_if 0 (;@2;)
        local.get 4
        i32.load
        local.get 6
        local.get 5
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get 4
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.load offset=12
            i32.load8_u
            local.tee 4
            i32.const -3
            i32.add
            i32.const 255
            i32.and
            local.tee 0
            i32.const 1
            i32.add
            i32.const 0
            local.get 0
            i32.const 2
            i32.lt_u
            select
            br_table 0 (;@4;) 2 (;@2;) 1 (;@3;) 0 (;@4;)
          end
          i32.const 0
          i32.load8_u offset=1058850
          br_if 2 (;@1;)
          i32.const 0
          i32.const 1
          i32.store8 offset=1058850
          local.get 3
          i32.const 52
          i32.add
          i32.const 1
          i32.store
          local.get 3
          i64.const 1
          i64.store offset=36 align=4
          local.get 3
          i32.const 1050532
          i32.store offset=32
          local.get 3
          i32.const 20
          i32.store offset=4
          local.get 3
          local.get 4
          i32.store8 offset=63
          local.get 3
          local.get 3
          i32.store offset=48
          local.get 3
          local.get 3
          i32.const 63
          i32.add
          i32.store
          local.get 3
          i32.const 24
          i32.add
          local.get 1
          local.get 3
          i32.const 32
          i32.add
          local.get 2
          call_indirect (type 7)
          i32.const 0
          i32.const 0
          i32.store8 offset=1058850
          block  ;; label = @4
            i32.const 0
            br_if 0 (;@4;)
            local.get 3
            i32.load8_u offset=24
            i32.const 2
            i32.ne
            br_if 2 (;@2;)
          end
          local.get 3
          i32.load offset=28
          local.tee 0
          i32.load
          local.get 0
          i32.load offset=4
          i32.load
          call_indirect (type 1)
          block  ;; label = @4
            local.get 0
            i32.load offset=4
            local.tee 1
            i32.load offset=4
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            i32.load
            local.get 2
            local.get 1
            i32.load offset=8
            call $__rust_dealloc
          end
          local.get 0
          i32.const 12
          i32.const 4
          call $__rust_dealloc
          br 1 (;@2;)
        end
        i32.const 0
        i32.load8_u offset=1058744
        local.set 0
        i32.const 0
        i32.const 0
        i32.store8 offset=1058744
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        i32.const 52
        i32.add
        i32.const 0
        i32.store
        local.get 3
        i32.const 1049252
        i32.store offset=48
        local.get 3
        i64.const 1
        i64.store offset=36 align=4
        local.get 3
        i32.const 1052132
        i32.store offset=32
        local.get 3
        local.get 1
        local.get 3
        i32.const 32
        i32.add
        local.get 2
        call_indirect (type 7)
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          local.get 3
          i32.load8_u
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        local.get 3
        i32.load offset=4
        local.tee 0
        i32.load
        local.get 0
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          local.get 0
          i32.load offset=4
          local.tee 1
          i32.load offset=4
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load
          local.get 2
          local.get 1
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 0
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 3
      i32.const 64
      i32.add
      global.set 0
      return
    end
    i32.const 1052596
    i32.const 32
    i32.const 1052676
    call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
    unreachable)
  (func $rust_begin_unwind (type 1) (param i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    local.get 0
    call $_ZN4core5panic9PanicInfo8location17h289acf84f373c045E
    i32.const 1052140
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17hff9de88d0e2f6d3eE
    local.set 2
    local.get 0
    call $_ZN4core5panic9PanicInfo7message17h576f4b436581989eE
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17h8e11d4729ddc2cb0E
    local.set 3
    local.get 1
    local.get 2
    i32.store offset=8
    local.get 1
    local.get 0
    i32.store offset=4
    local.get 1
    local.get 3
    i32.store
    local.get 1
    call $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17h7b78cfba6afb8434E
    unreachable)
  (func $_ZN3std9panicking20rust_panic_with_hook17h56625e96833ade19E (type 4) (param i32 i32 i32 i32)
    (local i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 4
    global.set 0
    i32.const 1
    local.set 5
    i32.const 0
    i32.const 0
    i32.load offset=1058788
    i32.const 1
    i32.add
    i32.store offset=1058788
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058840
            i32.const 1
            i32.eq
            br_if 0 (;@4;)
            i32.const 0
            i64.const 4294967297
            i64.store offset=1058840
            br 1 (;@3;)
          end
          i32.const 0
          i32.const 0
          i32.load offset=1058844
          i32.const 1
          i32.add
          local.tee 5
          i32.store offset=1058844
          local.get 5
          i32.const 2
          i32.gt_u
          br_if 1 (;@2;)
        end
        local.get 4
        local.get 3
        i32.store offset=36
        local.get 4
        local.get 2
        i32.store offset=32
        local.get 4
        i32.const 1049252
        i32.store offset=28
        local.get 4
        i32.const 1049252
        i32.store offset=24
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058776
          local.tee 2
          i32.const -1
          i32.le_s
          br_if 0 (;@3;)
          i32.const 0
          local.get 2
          i32.const 1
          i32.add
          i32.store offset=1058776
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              i32.load offset=1058784
              local.tee 2
              i32.eqz
              br_if 0 (;@5;)
              i32.const 0
              i32.load offset=1058780
              local.set 3
              local.get 4
              i32.const 16
              i32.add
              local.get 0
              local.get 1
              i32.load offset=16
              call_indirect (type 6)
              local.get 4
              local.get 4
              i64.load offset=16
              i64.store offset=24
              local.get 3
              local.get 4
              i32.const 24
              i32.add
              local.get 2
              i32.load offset=12
              call_indirect (type 6)
              br 1 (;@4;)
            end
            local.get 4
            i32.const 8
            i32.add
            local.get 0
            local.get 1
            i32.load offset=16
            call_indirect (type 6)
            local.get 4
            local.get 4
            i64.load offset=8
            i64.store offset=24
            local.get 4
            i32.const 24
            i32.add
            call $_ZN3std9panicking12default_hook17h8b278e563c507519E
          end
          i32.const 0
          i32.const 0
          i32.load offset=1058776
          i32.const -1
          i32.add
          i32.store offset=1058776
          local.get 5
          i32.const 1
          i32.le_u
          br_if 2 (;@1;)
          local.get 4
          i32.const 60
          i32.add
          i32.const 0
          i32.store
          local.get 4
          i32.const 1049252
          i32.store offset=56
          local.get 4
          i64.const 1
          i64.store offset=44 align=4
          local.get 4
          i32.const 1052348
          i32.store offset=40
          local.get 4
          i32.const 40
          i32.add
          call $_ZN3std10sys_common4util10dumb_print17h31d401d3573033e3E
          unreachable
          unreachable
        end
        local.get 4
        i32.const 60
        i32.add
        i32.const 0
        i32.store
        local.get 4
        i32.const 1049252
        i32.store offset=56
        local.get 4
        i64.const 1
        i64.store offset=44 align=4
        local.get 4
        i32.const 1052800
        i32.store offset=40
        local.get 4
        i32.const 40
        i32.add
        call $_ZN3std10sys_common4util5abort17ha9873acc1ebcd69dE
        unreachable
      end
      local.get 4
      i32.const 60
      i32.add
      i32.const 0
      i32.store
      local.get 4
      i32.const 1049252
      i32.store offset=56
      local.get 4
      i64.const 1
      i64.store offset=44 align=4
      local.get 4
      i32.const 1052296
      i32.store offset=40
      local.get 4
      i32.const 40
      i32.add
      call $_ZN3std10sys_common4util10dumb_print17h31d401d3573033e3E
      unreachable
      unreachable
    end
    local.get 0
    local.get 1
    call $rust_panic
    unreachable)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h18a2bce81be58b94E (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      local.get 1
      i32.load offset=4
      local.tee 3
      br_if 0 (;@1;)
      local.get 1
      i32.const 4
      i32.add
      local.set 3
      local.get 1
      i32.load
      local.set 4
      local.get 2
      i32.const 0
      i32.store offset=32
      local.get 2
      i64.const 1
      i64.store offset=24
      local.get 2
      local.get 2
      i32.const 24
      i32.add
      i32.store offset=36
      local.get 2
      i32.const 40
      i32.add
      i32.const 16
      i32.add
      local.get 4
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      i32.const 40
      i32.add
      i32.const 8
      i32.add
      local.get 4
      i32.const 8
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      local.get 4
      i64.load align=4
      i64.store offset=40
      local.get 2
      i32.const 36
      i32.add
      i32.const 1049064
      local.get 2
      i32.const 40
      i32.add
      call $_ZN4core3fmt5write17h1626e57fa473d161E
      drop
      local.get 2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee 4
      local.get 2
      i32.load offset=32
      i32.store
      local.get 2
      local.get 2
      i64.load offset=24
      i64.store offset=8
      block  ;; label = @2
        local.get 1
        i32.load offset=4
        local.tee 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.const 8
        i32.add
        i32.load
        local.tee 6
        i32.eqz
        br_if 0 (;@2;)
        local.get 5
        local.get 6
        i32.const 1
        call $__rust_dealloc
      end
      local.get 3
      local.get 2
      i64.load offset=8
      i64.store align=4
      local.get 3
      i32.const 8
      i32.add
      local.get 4
      i32.load
      i32.store
      local.get 3
      i32.load
      local.set 3
    end
    local.get 1
    i32.const 1
    i32.store offset=4
    local.get 1
    i32.const 12
    i32.add
    i32.load
    local.set 4
    local.get 1
    i32.const 8
    i32.add
    local.tee 1
    i32.load
    local.set 5
    local.get 1
    i64.const 0
    i64.store align=4
    block  ;; label = @1
      i32.const 12
      i32.const 4
      call $__rust_alloc
      local.tee 1
      br_if 0 (;@1;)
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
      unreachable
    end
    local.get 1
    local.get 4
    i32.store offset=8
    local.get 1
    local.get 5
    i32.store offset=4
    local.get 1
    local.get 3
    i32.store
    local.get 0
    i32.const 1052192
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store
    local.get 2
    i32.const 64
    i32.add
    global.set 0)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17he307532799611685E (type 6) (param i32 i32)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 2
    global.set 0
    local.get 1
    i32.const 4
    i32.add
    local.set 3
    block  ;; label = @1
      local.get 1
      i32.load offset=4
      br_if 0 (;@1;)
      local.get 1
      i32.load
      local.set 4
      local.get 2
      i32.const 0
      i32.store offset=32
      local.get 2
      i64.const 1
      i64.store offset=24
      local.get 2
      local.get 2
      i32.const 24
      i32.add
      i32.store offset=36
      local.get 2
      i32.const 40
      i32.add
      i32.const 16
      i32.add
      local.get 4
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      i32.const 40
      i32.add
      i32.const 8
      i32.add
      local.get 4
      i32.const 8
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      local.get 4
      i64.load align=4
      i64.store offset=40
      local.get 2
      i32.const 36
      i32.add
      i32.const 1049064
      local.get 2
      i32.const 40
      i32.add
      call $_ZN4core3fmt5write17h1626e57fa473d161E
      drop
      local.get 2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee 4
      local.get 2
      i32.load offset=32
      i32.store
      local.get 2
      local.get 2
      i64.load offset=24
      i64.store offset=8
      block  ;; label = @2
        local.get 1
        i32.load offset=4
        local.tee 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.const 8
        i32.add
        i32.load
        local.tee 1
        i32.eqz
        br_if 0 (;@2;)
        local.get 5
        local.get 1
        i32.const 1
        call $__rust_dealloc
      end
      local.get 3
      local.get 2
      i64.load offset=8
      i64.store align=4
      local.get 3
      i32.const 8
      i32.add
      local.get 4
      i32.load
      i32.store
    end
    local.get 0
    i32.const 1052192
    i32.store offset=4
    local.get 0
    local.get 3
    i32.store
    local.get 2
    i32.const 64
    i32.add
    global.set 0)
  (func $_ZN91_$LT$std..panicking..begin_panic..PanicPayload$LT$A$GT$$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17hf2bd9ca407affc5aE (type 6) (param i32 i32)
    (local i32 i32)
    local.get 1
    i32.load
    local.set 2
    local.get 1
    i32.const 0
    i32.store
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load offset=4
        local.set 3
        i32.const 8
        i32.const 4
        call $__rust_alloc
        local.tee 1
        i32.eqz
        br_if 1 (;@1;)
        local.get 1
        local.get 3
        i32.store offset=4
        local.get 1
        local.get 2
        i32.store
        local.get 0
        i32.const 1052228
        i32.store offset=4
        local.get 0
        local.get 1
        i32.store
        return
      end
      call $_ZN3std7process5abort17h3956b458a2b8fd63E
      unreachable
    end
    i32.const 8
    i32.const 4
    call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
    unreachable)
  (func $_ZN91_$LT$std..panicking..begin_panic..PanicPayload$LT$A$GT$$u20$as$u20$core..panic..BoxMeUp$GT$3get17h65f3fed9b8093d68E (type 6) (param i32 i32)
    block  ;; label = @1
      local.get 1
      i32.load
      br_if 0 (;@1;)
      call $_ZN3std7process5abort17h3956b458a2b8fd63E
      unreachable
    end
    local.get 0
    i32.const 1052228
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $rust_panic (type 6) (param i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 1
    i32.store offset=4
    local.get 2
    local.get 0
    i32.store
    local.get 2
    local.get 2
    call $__rust_start_panic
    i32.store offset=12
    local.get 2
    i32.const 36
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i64.const 1
    i64.store offset=20 align=4
    local.get 2
    i32.const 1052388
    i32.store offset=16
    local.get 2
    i32.const 17
    i32.store offset=44
    local.get 2
    local.get 2
    i32.const 40
    i32.add
    i32.store offset=32
    local.get 2
    local.get 2
    i32.const 12
    i32.add
    i32.store offset=40
    local.get 2
    i32.const 16
    i32.add
    call $_ZN3std10sys_common4util5abort17ha9873acc1ebcd69dE
    unreachable)
  (func $_ZN3std2rt19lang_start_internal17h5691b2189b3dd330E (type 9) (param i32 i32 i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 4
            i32.const 1
            call $__rust_alloc
            local.tee 5
            i32.eqz
            br_if 0 (;@4;)
            local.get 5
            i32.const 1852399981
            i32.store align=1
            local.get 4
            i64.const 17179869188
            i64.store offset=4 align=4
            local.get 4
            local.get 5
            i32.store
            local.get 4
            call $_ZN3std6thread6Thread3new17hd3000777d889ebe5E
            local.set 5
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i32.load offset=1058824
                i32.const 1
                i32.eq
                br_if 0 (;@6;)
                i32.const 0
                i64.const 1
                i64.store offset=1058824 align=4
                i32.const 0
                i32.const 0
                i32.store offset=1058832
                br 1 (;@5;)
              end
              i32.const 0
              i32.load offset=1058828
              local.tee 6
              i32.const 1
              i32.add
              i32.const 0
              i32.le_s
              br_if 2 (;@3;)
              i32.const 0
              i32.load offset=1058832
              br_if 3 (;@2;)
              local.get 6
              br_if 4 (;@1;)
            end
            i32.const 0
            local.get 5
            i32.store offset=1058832
            i32.const 0
            i32.const 0
            i32.store offset=1058828
            local.get 0
            local.get 1
            i32.load offset=12
            call_indirect (type 5)
            local.set 5
            block  ;; label = @5
              i32.const 0
              i32.load offset=1058768
              i32.const 3
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              i32.const 1
              i32.store8 offset=15
              local.get 4
              local.get 4
              i32.const 15
              i32.add
              i32.store
              i32.const 1058768
              i32.const 0
              local.get 4
              i32.const 1051036
              call $_ZN3std4sync4once4Once10call_inner17h7c0ab88e215d467dE
            end
            local.get 4
            i32.const 16
            i32.add
            global.set 0
            local.get 5
            return
          end
          i32.const 4
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E
          unreachable
        end
        i32.const 1049144
        i32.const 24
        local.get 4
        i32.const 1049312
        i32.const 1051644
        call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
        unreachable
      end
      i32.const 1051660
      i32.const 38
      i32.const 1051700
      call $_ZN3std9panicking11begin_panic17h0bfea34e6ecdcac6E
      unreachable
    end
    i32.const 1049128
    i32.const 16
    local.get 4
    i32.const 1049328
    i32.const 1051716
    call $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E
    unreachable)
  (func $_ZN62_$LT$std..ffi..c_str..NulError$u20$as$u20$core..fmt..Debug$GT$3fmt17he93c4a7fdb5537e7E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 1
    i32.const 1052396
    i32.const 8
    call $_ZN4core3fmt9Formatter11debug_tuple17h687ff740dc6e8836E
    local.get 2
    local.get 0
    i32.store offset=12
    local.get 2
    local.get 2
    i32.const 12
    i32.add
    i32.const 1049452
    call $_ZN4core3fmt8builders10DebugTuple5field17h0e16581ce3858b5aE
    drop
    local.get 2
    local.get 0
    i32.const 4
    i32.add
    i32.store offset=12
    local.get 2
    local.get 2
    i32.const 12
    i32.add
    i32.const 1052404
    call $_ZN4core3fmt8builders10DebugTuple5field17h0e16581ce3858b5aE
    drop
    local.get 2
    call $_ZN4core3fmt8builders10DebugTuple6finish17hcd73b6e7d638f97cE
    local.set 0
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN3std3sys4wasi7process8ExitCode6as_i3217h9815b96bea97c486E (type 5) (param i32) (result i32)
    local.get 0
    i32.load8_u)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17hb8c1ea81fb56a93eE (type 4) (param i32 i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 4
    global.set 0
    local.get 4
    local.get 3
    i32.store offset=12
    local.get 4
    local.get 2
    i32.store offset=8
    i32.const 1
    local.set 2
    local.get 4
    i32.const 16
    i32.add
    i32.const 2
    local.get 4
    i32.const 8
    i32.add
    i32.const 1
    call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
    block  ;; label = @1
      block  ;; label = @2
        local.get 4
        i32.load16_u offset=16
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 4
        local.get 4
        i32.load16_u offset=18
        i32.store16 offset=30
        local.get 0
        local.get 4
        i32.const 30
        i32.add
        call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
        i64.extend_i32_u
        i64.const 65535
        i64.and
        i64.const 32
        i64.shl
        i64.store offset=4 align=4
        br 1 (;@1;)
      end
      local.get 0
      local.get 4
      i32.load offset=20
      i32.store offset=4
      i32.const 0
      local.set 2
    end
    local.get 0
    local.get 2
    i32.store
    local.get 4
    i32.const 32
    i32.add
    global.set 0)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17hd5dec2ceaf8b717dE (type 4) (param i32 i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    local.get 4
    i32.const 2
    local.get 2
    local.get 3
    call $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E
    i32.const 1
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        local.get 4
        i32.load16_u
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 4
        local.get 4
        i32.load16_u offset=2
        i32.store16 offset=14
        local.get 0
        local.get 4
        i32.const 14
        i32.add
        call $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE
        i64.extend_i32_u
        i64.const 65535
        i64.and
        i64.const 32
        i64.shl
        i64.store offset=4 align=4
        br 1 (;@1;)
      end
      local.get 0
      local.get 4
      i32.load offset=4
      i32.store offset=4
      i32.const 0
      local.set 2
    end
    local.get 0
    local.get 2
    i32.store
    local.get 4
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h3a56ad24f6b6aad4E (type 5) (param i32) (result i32)
    i32.const 1)
  (func $__rust_start_panic (type 5) (param i32) (result i32)
    unreachable
    unreachable)
  (func $_ZN4wasi5error5Error9raw_error17h74928eb98d3d9daeE (type 5) (param i32) (result i32)
    local.get 0
    i32.load16_u)
  (func $_ZN4wasi13lib_generated8args_get17h8303b93f14c625f0E (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hc630ba694bd01843E)
  (func $_ZN4wasi13lib_generated14args_sizes_get17h67a6e4a49c03ee4dE (type 1) (param i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 8
        i32.add
        local.get 1
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h9fcbedb77e7846e5E
        local.tee 2
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.add
        local.get 1
        i32.load offset=12
        i32.store
        local.get 0
        i32.const 4
        i32.add
        local.get 1
        i32.load offset=8
        i32.store
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      local.get 0
      local.get 2
      i32.store16 offset=2
      i32.const 1
      local.set 2
    end
    local.get 0
    local.get 2
    i32.store16
    local.get 1
    i32.const 16
    i32.add
    global.set 0)
  (func $_ZN4wasi13lib_generated8fd_write17he11aebb3c2194dd7E (type 4) (param i32 i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 4
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        local.get 2
        local.get 3
        local.get 4
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h93016769784eae7aE
        local.tee 1
        br_if 0 (;@2;)
        local.get 0
        i32.const 4
        i32.add
        local.get 4
        i32.load offset=12
        i32.store
        i32.const 0
        local.set 1
        br 1 (;@1;)
      end
      local.get 0
      local.get 1
      i32.store16 offset=2
      i32.const 1
      local.set 1
    end
    local.get 0
    local.get 1
    i32.store16
    local.get 4
    i32.const 16
    i32.add
    global.set 0)
  (func $malloc (type 5) (param i32) (result i32)
    local.get 0
    call $dlmalloc)
  (func $dlmalloc (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 1
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 0
                            i32.const 236
                            i32.gt_u
                            br_if 0 (;@12;)
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1058852
                              local.tee 2
                              i32.const 16
                              local.get 0
                              i32.const 19
                              i32.add
                              i32.const -16
                              i32.and
                              local.get 0
                              i32.const 11
                              i32.lt_u
                              select
                              local.tee 3
                              i32.const 3
                              i32.shr_u
                              local.tee 4
                              i32.shr_u
                              local.tee 0
                              i32.const 3
                              i32.and
                              i32.eqz
                              br_if 0 (;@13;)
                              local.get 0
                              i32.const 1
                              i32.and
                              local.get 4
                              i32.or
                              i32.const 1
                              i32.xor
                              local.tee 3
                              i32.const 3
                              i32.shl
                              local.tee 5
                              i32.const 1058900
                              i32.add
                              i32.load
                              local.tee 4
                              i32.const 8
                              i32.add
                              local.set 0
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 4
                                  i32.load offset=8
                                  local.tee 6
                                  local.get 5
                                  i32.const 1058892
                                  i32.add
                                  local.tee 5
                                  i32.ne
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.get 2
                                  i32.const -2
                                  local.get 3
                                  i32.rotl
                                  i32.and
                                  i32.store offset=1058852
                                  br 1 (;@14;)
                                end
                                i32.const 0
                                i32.load offset=1058868
                                local.get 6
                                i32.gt_u
                                drop
                                local.get 5
                                local.get 6
                                i32.store offset=8
                                local.get 6
                                local.get 5
                                i32.store offset=12
                              end
                              local.get 4
                              local.get 3
                              i32.const 3
                              i32.shl
                              local.tee 6
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get 4
                              local.get 6
                              i32.add
                              local.tee 4
                              local.get 4
                              i32.load offset=4
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              br 12 (;@1;)
                            end
                            local.get 3
                            i32.const 0
                            i32.load offset=1058860
                            local.tee 7
                            i32.le_u
                            br_if 1 (;@11;)
                            block  ;; label = @13
                              local.get 0
                              i32.eqz
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 0
                                  local.get 4
                                  i32.shl
                                  i32.const 2
                                  local.get 4
                                  i32.shl
                                  local.tee 0
                                  i32.const 0
                                  local.get 0
                                  i32.sub
                                  i32.or
                                  i32.and
                                  local.tee 0
                                  i32.const 0
                                  local.get 0
                                  i32.sub
                                  i32.and
                                  i32.const -1
                                  i32.add
                                  local.tee 0
                                  local.get 0
                                  i32.const 12
                                  i32.shr_u
                                  i32.const 16
                                  i32.and
                                  local.tee 0
                                  i32.shr_u
                                  local.tee 4
                                  i32.const 5
                                  i32.shr_u
                                  i32.const 8
                                  i32.and
                                  local.tee 6
                                  local.get 0
                                  i32.or
                                  local.get 4
                                  local.get 6
                                  i32.shr_u
                                  local.tee 0
                                  i32.const 2
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 0
                                  local.get 4
                                  i32.shr_u
                                  local.tee 0
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 2
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 0
                                  local.get 4
                                  i32.shr_u
                                  local.tee 0
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 1
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 0
                                  local.get 4
                                  i32.shr_u
                                  i32.add
                                  local.tee 6
                                  i32.const 3
                                  i32.shl
                                  local.tee 5
                                  i32.const 1058900
                                  i32.add
                                  i32.load
                                  local.tee 4
                                  i32.load offset=8
                                  local.tee 0
                                  local.get 5
                                  i32.const 1058892
                                  i32.add
                                  local.tee 5
                                  i32.ne
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.get 2
                                  i32.const -2
                                  local.get 6
                                  i32.rotl
                                  i32.and
                                  local.tee 2
                                  i32.store offset=1058852
                                  br 1 (;@14;)
                                end
                                i32.const 0
                                i32.load offset=1058868
                                local.get 0
                                i32.gt_u
                                drop
                                local.get 5
                                local.get 0
                                i32.store offset=8
                                local.get 0
                                local.get 5
                                i32.store offset=12
                              end
                              local.get 4
                              i32.const 8
                              i32.add
                              local.set 0
                              local.get 4
                              local.get 3
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get 4
                              local.get 6
                              i32.const 3
                              i32.shl
                              local.tee 6
                              i32.add
                              local.get 6
                              local.get 3
                              i32.sub
                              local.tee 6
                              i32.store
                              local.get 4
                              local.get 3
                              i32.add
                              local.tee 5
                              local.get 6
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              block  ;; label = @14
                                local.get 7
                                i32.eqz
                                br_if 0 (;@14;)
                                local.get 7
                                i32.const 3
                                i32.shr_u
                                local.tee 8
                                i32.const 3
                                i32.shl
                                i32.const 1058892
                                i32.add
                                local.set 3
                                i32.const 0
                                i32.load offset=1058872
                                local.set 4
                                block  ;; label = @15
                                  block  ;; label = @16
                                    local.get 2
                                    i32.const 1
                                    local.get 8
                                    i32.shl
                                    local.tee 8
                                    i32.and
                                    br_if 0 (;@16;)
                                    i32.const 0
                                    local.get 2
                                    local.get 8
                                    i32.or
                                    i32.store offset=1058852
                                    local.get 3
                                    local.set 8
                                    br 1 (;@15;)
                                  end
                                  local.get 3
                                  i32.load offset=8
                                  local.set 8
                                end
                                local.get 8
                                local.get 4
                                i32.store offset=12
                                local.get 3
                                local.get 4
                                i32.store offset=8
                                local.get 4
                                local.get 3
                                i32.store offset=12
                                local.get 4
                                local.get 8
                                i32.store offset=8
                              end
                              i32.const 0
                              local.get 5
                              i32.store offset=1058872
                              i32.const 0
                              local.get 6
                              i32.store offset=1058860
                              br 12 (;@1;)
                            end
                            i32.const 0
                            i32.load offset=1058856
                            local.tee 9
                            i32.eqz
                            br_if 1 (;@11;)
                            local.get 9
                            i32.const 0
                            local.get 9
                            i32.sub
                            i32.and
                            i32.const -1
                            i32.add
                            local.tee 0
                            local.get 0
                            i32.const 12
                            i32.shr_u
                            i32.const 16
                            i32.and
                            local.tee 0
                            i32.shr_u
                            local.tee 4
                            i32.const 5
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee 6
                            local.get 0
                            i32.or
                            local.get 4
                            local.get 6
                            i32.shr_u
                            local.tee 0
                            i32.const 2
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 0
                            local.get 4
                            i32.shr_u
                            local.tee 0
                            i32.const 1
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 0
                            local.get 4
                            i32.shr_u
                            local.tee 0
                            i32.const 1
                            i32.shr_u
                            i32.const 1
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 0
                            local.get 4
                            i32.shr_u
                            i32.add
                            i32.const 2
                            i32.shl
                            i32.const 1059156
                            i32.add
                            i32.load
                            local.tee 5
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            local.get 3
                            i32.sub
                            local.set 4
                            local.get 5
                            local.set 6
                            block  ;; label = @13
                              loop  ;; label = @14
                                block  ;; label = @15
                                  local.get 6
                                  i32.load offset=16
                                  local.tee 0
                                  br_if 0 (;@15;)
                                  local.get 6
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee 0
                                  i32.eqz
                                  br_if 2 (;@13;)
                                end
                                local.get 0
                                i32.load offset=4
                                i32.const -8
                                i32.and
                                local.get 3
                                i32.sub
                                local.tee 6
                                local.get 4
                                local.get 6
                                local.get 4
                                i32.lt_u
                                local.tee 6
                                select
                                local.set 4
                                local.get 0
                                local.get 5
                                local.get 6
                                select
                                local.set 5
                                local.get 0
                                local.set 6
                                br 0 (;@14;)
                              end
                            end
                            local.get 5
                            i32.load offset=24
                            local.set 10
                            block  ;; label = @13
                              local.get 5
                              i32.load offset=12
                              local.tee 8
                              local.get 5
                              i32.eq
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                i32.const 0
                                i32.load offset=1058868
                                local.get 5
                                i32.load offset=8
                                local.tee 0
                                i32.gt_u
                                br_if 0 (;@14;)
                                local.get 0
                                i32.load offset=12
                                local.get 5
                                i32.ne
                                drop
                              end
                              local.get 8
                              local.get 0
                              i32.store offset=8
                              local.get 0
                              local.get 8
                              i32.store offset=12
                              br 11 (;@2;)
                            end
                            block  ;; label = @13
                              local.get 5
                              i32.const 20
                              i32.add
                              local.tee 6
                              i32.load
                              local.tee 0
                              br_if 0 (;@13;)
                              local.get 5
                              i32.load offset=16
                              local.tee 0
                              i32.eqz
                              br_if 3 (;@10;)
                              local.get 5
                              i32.const 16
                              i32.add
                              local.set 6
                            end
                            loop  ;; label = @13
                              local.get 6
                              local.set 11
                              local.get 0
                              local.tee 8
                              i32.const 20
                              i32.add
                              local.tee 6
                              i32.load
                              local.tee 0
                              br_if 0 (;@13;)
                              local.get 8
                              i32.const 16
                              i32.add
                              local.set 6
                              local.get 8
                              i32.load offset=16
                              local.tee 0
                              br_if 0 (;@13;)
                            end
                            local.get 11
                            i32.const 0
                            i32.store
                            br 10 (;@2;)
                          end
                          i32.const -1
                          local.set 3
                          local.get 0
                          i32.const -65
                          i32.gt_u
                          br_if 0 (;@11;)
                          local.get 0
                          i32.const 19
                          i32.add
                          local.tee 0
                          i32.const -16
                          i32.and
                          local.set 3
                          i32.const 0
                          i32.load offset=1058856
                          local.tee 7
                          i32.eqz
                          br_if 0 (;@11;)
                          i32.const 0
                          local.set 11
                          block  ;; label = @12
                            local.get 0
                            i32.const 8
                            i32.shr_u
                            local.tee 0
                            i32.eqz
                            br_if 0 (;@12;)
                            i32.const 31
                            local.set 11
                            local.get 3
                            i32.const 16777215
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 0
                            local.get 0
                            i32.const 1048320
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee 4
                            i32.shl
                            local.tee 0
                            local.get 0
                            i32.const 520192
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.tee 0
                            i32.shl
                            local.tee 6
                            local.get 6
                            i32.const 245760
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee 6
                            i32.shl
                            i32.const 15
                            i32.shr_u
                            local.get 0
                            local.get 4
                            i32.or
                            local.get 6
                            i32.or
                            i32.sub
                            local.tee 0
                            i32.const 1
                            i32.shl
                            local.get 3
                            local.get 0
                            i32.const 21
                            i32.add
                            i32.shr_u
                            i32.const 1
                            i32.and
                            i32.or
                            i32.const 28
                            i32.add
                            local.set 11
                          end
                          i32.const 0
                          local.get 3
                          i32.sub
                          local.set 6
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 11
                                  i32.const 2
                                  i32.shl
                                  i32.const 1059156
                                  i32.add
                                  i32.load
                                  local.tee 4
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.set 0
                                  i32.const 0
                                  local.set 8
                                  br 1 (;@14;)
                                end
                                local.get 3
                                i32.const 0
                                i32.const 25
                                local.get 11
                                i32.const 1
                                i32.shr_u
                                i32.sub
                                local.get 11
                                i32.const 31
                                i32.eq
                                select
                                i32.shl
                                local.set 5
                                i32.const 0
                                local.set 0
                                i32.const 0
                                local.set 8
                                loop  ;; label = @15
                                  block  ;; label = @16
                                    local.get 4
                                    i32.load offset=4
                                    i32.const -8
                                    i32.and
                                    local.get 3
                                    i32.sub
                                    local.tee 2
                                    local.get 6
                                    i32.ge_u
                                    br_if 0 (;@16;)
                                    local.get 2
                                    local.set 6
                                    local.get 4
                                    local.set 8
                                    local.get 2
                                    br_if 0 (;@16;)
                                    i32.const 0
                                    local.set 6
                                    local.get 4
                                    local.set 8
                                    local.get 4
                                    local.set 0
                                    br 3 (;@13;)
                                  end
                                  local.get 0
                                  local.get 4
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee 2
                                  local.get 2
                                  local.get 4
                                  local.get 5
                                  i32.const 29
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  i32.add
                                  i32.const 16
                                  i32.add
                                  i32.load
                                  local.tee 4
                                  i32.eq
                                  select
                                  local.get 0
                                  local.get 2
                                  select
                                  local.set 0
                                  local.get 5
                                  local.get 4
                                  i32.const 0
                                  i32.ne
                                  i32.shl
                                  local.set 5
                                  local.get 4
                                  br_if 0 (;@15;)
                                end
                              end
                              block  ;; label = @14
                                local.get 0
                                local.get 8
                                i32.or
                                br_if 0 (;@14;)
                                i32.const 2
                                local.get 11
                                i32.shl
                                local.tee 0
                                i32.const 0
                                local.get 0
                                i32.sub
                                i32.or
                                local.get 7
                                i32.and
                                local.tee 0
                                i32.eqz
                                br_if 3 (;@11;)
                                local.get 0
                                i32.const 0
                                local.get 0
                                i32.sub
                                i32.and
                                i32.const -1
                                i32.add
                                local.tee 0
                                local.get 0
                                i32.const 12
                                i32.shr_u
                                i32.const 16
                                i32.and
                                local.tee 0
                                i32.shr_u
                                local.tee 4
                                i32.const 5
                                i32.shr_u
                                i32.const 8
                                i32.and
                                local.tee 5
                                local.get 0
                                i32.or
                                local.get 4
                                local.get 5
                                i32.shr_u
                                local.tee 0
                                i32.const 2
                                i32.shr_u
                                i32.const 4
                                i32.and
                                local.tee 4
                                i32.or
                                local.get 0
                                local.get 4
                                i32.shr_u
                                local.tee 0
                                i32.const 1
                                i32.shr_u
                                i32.const 2
                                i32.and
                                local.tee 4
                                i32.or
                                local.get 0
                                local.get 4
                                i32.shr_u
                                local.tee 0
                                i32.const 1
                                i32.shr_u
                                i32.const 1
                                i32.and
                                local.tee 4
                                i32.or
                                local.get 0
                                local.get 4
                                i32.shr_u
                                i32.add
                                i32.const 2
                                i32.shl
                                i32.const 1059156
                                i32.add
                                i32.load
                                local.set 0
                              end
                              local.get 0
                              i32.eqz
                              br_if 1 (;@12;)
                            end
                            loop  ;; label = @13
                              local.get 0
                              i32.load offset=4
                              i32.const -8
                              i32.and
                              local.get 3
                              i32.sub
                              local.tee 2
                              local.get 6
                              i32.lt_u
                              local.set 5
                              block  ;; label = @14
                                local.get 0
                                i32.load offset=16
                                local.tee 4
                                br_if 0 (;@14;)
                                local.get 0
                                i32.const 20
                                i32.add
                                i32.load
                                local.set 4
                              end
                              local.get 2
                              local.get 6
                              local.get 5
                              select
                              local.set 6
                              local.get 0
                              local.get 8
                              local.get 5
                              select
                              local.set 8
                              local.get 4
                              local.set 0
                              local.get 4
                              br_if 0 (;@13;)
                            end
                          end
                          local.get 8
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 6
                          i32.const 0
                          i32.load offset=1058860
                          local.get 3
                          i32.sub
                          i32.ge_u
                          br_if 0 (;@11;)
                          local.get 8
                          i32.load offset=24
                          local.set 11
                          block  ;; label = @12
                            local.get 8
                            i32.load offset=12
                            local.tee 5
                            local.get 8
                            i32.eq
                            br_if 0 (;@12;)
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1058868
                              local.get 8
                              i32.load offset=8
                              local.tee 0
                              i32.gt_u
                              br_if 0 (;@13;)
                              local.get 0
                              i32.load offset=12
                              local.get 8
                              i32.ne
                              drop
                            end
                            local.get 5
                            local.get 0
                            i32.store offset=8
                            local.get 0
                            local.get 5
                            i32.store offset=12
                            br 9 (;@3;)
                          end
                          block  ;; label = @12
                            local.get 8
                            i32.const 20
                            i32.add
                            local.tee 4
                            i32.load
                            local.tee 0
                            br_if 0 (;@12;)
                            local.get 8
                            i32.load offset=16
                            local.tee 0
                            i32.eqz
                            br_if 3 (;@9;)
                            local.get 8
                            i32.const 16
                            i32.add
                            local.set 4
                          end
                          loop  ;; label = @12
                            local.get 4
                            local.set 2
                            local.get 0
                            local.tee 5
                            i32.const 20
                            i32.add
                            local.tee 4
                            i32.load
                            local.tee 0
                            br_if 0 (;@12;)
                            local.get 5
                            i32.const 16
                            i32.add
                            local.set 4
                            local.get 5
                            i32.load offset=16
                            local.tee 0
                            br_if 0 (;@12;)
                          end
                          local.get 2
                          i32.const 0
                          i32.store
                          br 8 (;@3;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1058860
                          local.tee 0
                          local.get 3
                          i32.lt_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.load offset=1058872
                          local.set 4
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 0
                              local.get 3
                              i32.sub
                              local.tee 6
                              i32.const 16
                              i32.lt_u
                              br_if 0 (;@13;)
                              local.get 4
                              local.get 3
                              i32.add
                              local.tee 5
                              local.get 6
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              i32.const 0
                              local.get 6
                              i32.store offset=1058860
                              i32.const 0
                              local.get 5
                              i32.store offset=1058872
                              local.get 4
                              local.get 0
                              i32.add
                              local.get 6
                              i32.store
                              local.get 4
                              local.get 3
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              br 1 (;@12;)
                            end
                            local.get 4
                            local.get 0
                            i32.const 3
                            i32.or
                            i32.store offset=4
                            local.get 4
                            local.get 0
                            i32.add
                            local.tee 0
                            local.get 0
                            i32.load offset=4
                            i32.const 1
                            i32.or
                            i32.store offset=4
                            i32.const 0
                            i32.const 0
                            i32.store offset=1058872
                            i32.const 0
                            i32.const 0
                            i32.store offset=1058860
                          end
                          local.get 4
                          i32.const 8
                          i32.add
                          local.set 0
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1058864
                          local.tee 5
                          local.get 3
                          i32.le_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.load offset=1058876
                          local.tee 0
                          local.get 3
                          i32.add
                          local.tee 4
                          local.get 5
                          local.get 3
                          i32.sub
                          local.tee 6
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          i32.const 0
                          local.get 6
                          i32.store offset=1058864
                          i32.const 0
                          local.get 4
                          i32.store offset=1058876
                          local.get 0
                          local.get 3
                          i32.const 3
                          i32.or
                          i32.store offset=4
                          local.get 0
                          i32.const 8
                          i32.add
                          local.set 0
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1059324
                            i32.eqz
                            br_if 0 (;@12;)
                            i32.const 0
                            i32.load offset=1059332
                            local.set 4
                            br 1 (;@11;)
                          end
                          i32.const 0
                          i64.const -1
                          i64.store offset=1059336 align=4
                          i32.const 0
                          i64.const 281474976776192
                          i64.store offset=1059328 align=4
                          i32.const 0
                          local.get 1
                          i32.const 12
                          i32.add
                          i32.const -16
                          i32.and
                          i32.const 1431655768
                          i32.xor
                          i32.store offset=1059324
                          i32.const 0
                          i32.const 0
                          i32.store offset=1059344
                          i32.const 0
                          i32.const 0
                          i32.store offset=1059296
                          i32.const 65536
                          local.set 4
                        end
                        i32.const 0
                        local.set 0
                        block  ;; label = @11
                          local.get 4
                          local.get 3
                          i32.const 71
                          i32.add
                          local.tee 7
                          i32.add
                          local.tee 2
                          i32.const 0
                          local.get 4
                          i32.sub
                          local.tee 11
                          i32.and
                          local.tee 8
                          local.get 3
                          i32.gt_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059348
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1059292
                          local.tee 0
                          i32.eqz
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1059284
                            local.tee 4
                            local.get 8
                            i32.add
                            local.tee 6
                            local.get 4
                            i32.le_u
                            br_if 0 (;@12;)
                            local.get 6
                            local.get 0
                            i32.le_u
                            br_if 1 (;@11;)
                          end
                          i32.const 0
                          local.set 0
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059348
                          br 10 (;@1;)
                        end
                        i32.const 0
                        i32.load8_u offset=1059296
                        i32.const 4
                        i32.and
                        br_if 4 (;@6;)
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1058876
                              local.tee 4
                              i32.eqz
                              br_if 0 (;@13;)
                              i32.const 1059300
                              local.set 0
                              loop  ;; label = @14
                                block  ;; label = @15
                                  local.get 0
                                  i32.load
                                  local.tee 6
                                  local.get 4
                                  i32.gt_u
                                  br_if 0 (;@15;)
                                  local.get 6
                                  local.get 0
                                  i32.load offset=4
                                  i32.add
                                  local.get 4
                                  i32.gt_u
                                  br_if 3 (;@12;)
                                end
                                local.get 0
                                i32.load offset=8
                                local.tee 0
                                br_if 0 (;@14;)
                              end
                            end
                            i32.const 0
                            call $sbrk
                            local.tee 5
                            i32.const -1
                            i32.eq
                            br_if 5 (;@7;)
                            local.get 8
                            local.set 2
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1059328
                              local.tee 0
                              i32.const -1
                              i32.add
                              local.tee 4
                              local.get 5
                              i32.and
                              i32.eqz
                              br_if 0 (;@13;)
                              local.get 8
                              local.get 5
                              i32.sub
                              local.get 4
                              local.get 5
                              i32.add
                              i32.const 0
                              local.get 0
                              i32.sub
                              i32.and
                              i32.add
                              local.set 2
                            end
                            local.get 2
                            local.get 3
                            i32.le_u
                            br_if 5 (;@7;)
                            local.get 2
                            i32.const 2147483646
                            i32.gt_u
                            br_if 5 (;@7;)
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1059292
                              local.tee 0
                              i32.eqz
                              br_if 0 (;@13;)
                              i32.const 0
                              i32.load offset=1059284
                              local.tee 4
                              local.get 2
                              i32.add
                              local.tee 6
                              local.get 4
                              i32.le_u
                              br_if 6 (;@7;)
                              local.get 6
                              local.get 0
                              i32.gt_u
                              br_if 6 (;@7;)
                            end
                            local.get 2
                            call $sbrk
                            local.tee 0
                            local.get 5
                            i32.ne
                            br_if 1 (;@11;)
                            br 7 (;@5;)
                          end
                          local.get 2
                          local.get 5
                          i32.sub
                          local.get 11
                          i32.and
                          local.tee 2
                          i32.const 2147483646
                          i32.gt_u
                          br_if 4 (;@7;)
                          local.get 2
                          call $sbrk
                          local.tee 5
                          local.get 0
                          i32.load
                          local.get 0
                          i32.load offset=4
                          i32.add
                          i32.eq
                          br_if 3 (;@8;)
                          local.get 5
                          local.set 0
                        end
                        local.get 0
                        local.set 5
                        block  ;; label = @11
                          local.get 3
                          i32.const 72
                          i32.add
                          local.get 2
                          i32.le_u
                          br_if 0 (;@11;)
                          local.get 2
                          i32.const 2147483646
                          i32.gt_u
                          br_if 0 (;@11;)
                          local.get 5
                          i32.const -1
                          i32.eq
                          br_if 0 (;@11;)
                          local.get 7
                          local.get 2
                          i32.sub
                          i32.const 0
                          i32.load offset=1059332
                          local.tee 0
                          i32.add
                          i32.const 0
                          local.get 0
                          i32.sub
                          i32.and
                          local.tee 0
                          i32.const 2147483646
                          i32.gt_u
                          br_if 6 (;@5;)
                          block  ;; label = @12
                            local.get 0
                            call $sbrk
                            i32.const -1
                            i32.eq
                            br_if 0 (;@12;)
                            local.get 0
                            local.get 2
                            i32.add
                            local.set 2
                            br 7 (;@5;)
                          end
                          i32.const 0
                          local.get 2
                          i32.sub
                          call $sbrk
                          drop
                          br 4 (;@7;)
                        end
                        local.get 5
                        i32.const -1
                        i32.ne
                        br_if 5 (;@5;)
                        br 3 (;@7;)
                      end
                      i32.const 0
                      local.set 8
                      br 7 (;@2;)
                    end
                    i32.const 0
                    local.set 5
                    br 5 (;@3;)
                  end
                  local.get 5
                  i32.const -1
                  i32.ne
                  br_if 2 (;@5;)
                end
                i32.const 0
                i32.const 0
                i32.load offset=1059296
                i32.const 4
                i32.or
                i32.store offset=1059296
              end
              local.get 8
              i32.const 2147483646
              i32.gt_u
              br_if 1 (;@4;)
              local.get 8
              call $sbrk
              local.tee 5
              i32.const 0
              call $sbrk
              local.tee 0
              i32.ge_u
              br_if 1 (;@4;)
              local.get 5
              i32.const -1
              i32.eq
              br_if 1 (;@4;)
              local.get 0
              i32.const -1
              i32.eq
              br_if 1 (;@4;)
              local.get 0
              local.get 5
              i32.sub
              local.tee 2
              local.get 3
              i32.const 56
              i32.add
              i32.le_u
              br_if 1 (;@4;)
            end
            i32.const 0
            i32.const 0
            i32.load offset=1059284
            local.get 2
            i32.add
            local.tee 0
            i32.store offset=1059284
            block  ;; label = @5
              local.get 0
              i32.const 0
              i32.load offset=1059288
              i32.le_u
              br_if 0 (;@5;)
              i32.const 0
              local.get 0
              i32.store offset=1059288
            end
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    i32.const 0
                    i32.load offset=1058876
                    local.tee 4
                    i32.eqz
                    br_if 0 (;@8;)
                    i32.const 1059300
                    local.set 0
                    loop  ;; label = @9
                      local.get 5
                      local.get 0
                      i32.load
                      local.tee 6
                      local.get 0
                      i32.load offset=4
                      local.tee 8
                      i32.add
                      i32.eq
                      br_if 2 (;@7;)
                      local.get 0
                      i32.load offset=8
                      local.tee 0
                      br_if 0 (;@9;)
                      br 3 (;@6;)
                    end
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1058868
                      local.tee 0
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 5
                      local.get 0
                      i32.ge_u
                      br_if 1 (;@8;)
                    end
                    i32.const 0
                    local.get 5
                    i32.store offset=1058868
                  end
                  i32.const 0
                  local.set 0
                  i32.const 0
                  local.get 2
                  i32.store offset=1059304
                  i32.const 0
                  local.get 5
                  i32.store offset=1059300
                  i32.const 0
                  i32.const -1
                  i32.store offset=1058884
                  i32.const 0
                  i32.const 0
                  i32.load offset=1059324
                  i32.store offset=1058888
                  i32.const 0
                  i32.const 0
                  i32.store offset=1059312
                  loop  ;; label = @8
                    local.get 0
                    i32.const 1058900
                    i32.add
                    local.get 0
                    i32.const 1058892
                    i32.add
                    local.tee 4
                    i32.store
                    local.get 0
                    i32.const 1058904
                    i32.add
                    local.get 4
                    i32.store
                    local.get 0
                    i32.const 8
                    i32.add
                    local.tee 0
                    i32.const 256
                    i32.ne
                    br_if 0 (;@8;)
                  end
                  local.get 5
                  i32.const -8
                  local.get 5
                  i32.sub
                  i32.const 15
                  i32.and
                  i32.const 0
                  local.get 5
                  i32.const 8
                  i32.add
                  i32.const 15
                  i32.and
                  select
                  local.tee 0
                  i32.add
                  local.tee 4
                  local.get 2
                  i32.const -56
                  i32.add
                  local.tee 6
                  local.get 0
                  i32.sub
                  local.tee 0
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  i32.const 0
                  i32.const 0
                  i32.load offset=1059340
                  i32.store offset=1058880
                  i32.const 0
                  local.get 0
                  i32.store offset=1058864
                  i32.const 0
                  local.get 4
                  i32.store offset=1058876
                  local.get 5
                  local.get 6
                  i32.add
                  i32.const 56
                  i32.store offset=4
                  br 2 (;@5;)
                end
                local.get 0
                i32.load8_u offset=12
                i32.const 8
                i32.and
                br_if 0 (;@6;)
                local.get 5
                local.get 4
                i32.le_u
                br_if 0 (;@6;)
                local.get 6
                local.get 4
                i32.gt_u
                br_if 0 (;@6;)
                local.get 4
                i32.const -8
                local.get 4
                i32.sub
                i32.const 15
                i32.and
                i32.const 0
                local.get 4
                i32.const 8
                i32.add
                i32.const 15
                i32.and
                select
                local.tee 6
                i32.add
                local.tee 5
                i32.const 0
                i32.load offset=1058864
                local.get 2
                i32.add
                local.tee 11
                local.get 6
                i32.sub
                local.tee 6
                i32.const 1
                i32.or
                i32.store offset=4
                local.get 0
                local.get 8
                local.get 2
                i32.add
                i32.store offset=4
                i32.const 0
                i32.const 0
                i32.load offset=1059340
                i32.store offset=1058880
                i32.const 0
                local.get 6
                i32.store offset=1058864
                i32.const 0
                local.get 5
                i32.store offset=1058876
                local.get 4
                local.get 11
                i32.add
                i32.const 56
                i32.store offset=4
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 5
                i32.const 0
                i32.load offset=1058868
                local.tee 8
                i32.ge_u
                br_if 0 (;@6;)
                i32.const 0
                local.get 5
                i32.store offset=1058868
                local.get 5
                local.set 8
              end
              local.get 5
              local.get 2
              i32.add
              local.set 6
              i32.const 1059300
              local.set 0
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            loop  ;; label = @13
                              local.get 0
                              i32.load
                              local.get 6
                              i32.eq
                              br_if 1 (;@12;)
                              local.get 0
                              i32.load offset=8
                              local.tee 0
                              br_if 0 (;@13;)
                              br 2 (;@11;)
                            end
                          end
                          local.get 0
                          i32.load8_u offset=12
                          i32.const 8
                          i32.and
                          i32.eqz
                          br_if 1 (;@10;)
                        end
                        i32.const 1059300
                        local.set 0
                        loop  ;; label = @11
                          block  ;; label = @12
                            local.get 0
                            i32.load
                            local.tee 6
                            local.get 4
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 6
                            local.get 0
                            i32.load offset=4
                            i32.add
                            local.tee 6
                            local.get 4
                            i32.gt_u
                            br_if 3 (;@9;)
                          end
                          local.get 0
                          i32.load offset=8
                          local.set 0
                          br 0 (;@11;)
                        end
                      end
                      local.get 0
                      local.get 5
                      i32.store
                      local.get 0
                      local.get 0
                      i32.load offset=4
                      local.get 2
                      i32.add
                      i32.store offset=4
                      local.get 5
                      i32.const -8
                      local.get 5
                      i32.sub
                      i32.const 15
                      i32.and
                      i32.const 0
                      local.get 5
                      i32.const 8
                      i32.add
                      i32.const 15
                      i32.and
                      select
                      i32.add
                      local.tee 11
                      local.get 3
                      i32.const 3
                      i32.or
                      i32.store offset=4
                      local.get 6
                      i32.const -8
                      local.get 6
                      i32.sub
                      i32.const 15
                      i32.and
                      i32.const 0
                      local.get 6
                      i32.const 8
                      i32.add
                      i32.const 15
                      i32.and
                      select
                      i32.add
                      local.tee 5
                      local.get 11
                      i32.sub
                      local.get 3
                      i32.sub
                      local.set 0
                      local.get 11
                      local.get 3
                      i32.add
                      local.set 6
                      block  ;; label = @10
                        local.get 4
                        local.get 5
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.get 6
                        i32.store offset=1058876
                        i32.const 0
                        i32.const 0
                        i32.load offset=1058864
                        local.get 0
                        i32.add
                        local.tee 0
                        i32.store offset=1058864
                        local.get 6
                        local.get 0
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        br 3 (;@7;)
                      end
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1058872
                        local.get 5
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.get 6
                        i32.store offset=1058872
                        i32.const 0
                        i32.const 0
                        i32.load offset=1058860
                        local.get 0
                        i32.add
                        local.tee 0
                        i32.store offset=1058860
                        local.get 6
                        local.get 0
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get 6
                        local.get 0
                        i32.add
                        local.get 0
                        i32.store
                        br 3 (;@7;)
                      end
                      block  ;; label = @10
                        local.get 5
                        i32.load offset=4
                        local.tee 4
                        i32.const 3
                        i32.and
                        i32.const 1
                        i32.ne
                        br_if 0 (;@10;)
                        local.get 4
                        i32.const -8
                        i32.and
                        local.set 7
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 4
                            i32.const 255
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 5
                            i32.load offset=12
                            local.set 3
                            block  ;; label = @13
                              local.get 5
                              i32.load offset=8
                              local.tee 2
                              local.get 4
                              i32.const 3
                              i32.shr_u
                              local.tee 9
                              i32.const 3
                              i32.shl
                              i32.const 1058892
                              i32.add
                              local.tee 4
                              i32.eq
                              br_if 0 (;@13;)
                              local.get 8
                              local.get 2
                              i32.gt_u
                              drop
                            end
                            block  ;; label = @13
                              local.get 3
                              local.get 2
                              i32.ne
                              br_if 0 (;@13;)
                              i32.const 0
                              i32.const 0
                              i32.load offset=1058852
                              i32.const -2
                              local.get 9
                              i32.rotl
                              i32.and
                              i32.store offset=1058852
                              br 2 (;@11;)
                            end
                            block  ;; label = @13
                              local.get 3
                              local.get 4
                              i32.eq
                              br_if 0 (;@13;)
                              local.get 8
                              local.get 3
                              i32.gt_u
                              drop
                            end
                            local.get 3
                            local.get 2
                            i32.store offset=8
                            local.get 2
                            local.get 3
                            i32.store offset=12
                            br 1 (;@11;)
                          end
                          local.get 5
                          i32.load offset=24
                          local.set 9
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 5
                              i32.load offset=12
                              local.tee 2
                              local.get 5
                              i32.eq
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                local.get 8
                                local.get 5
                                i32.load offset=8
                                local.tee 4
                                i32.gt_u
                                br_if 0 (;@14;)
                                local.get 4
                                i32.load offset=12
                                local.get 5
                                i32.ne
                                drop
                              end
                              local.get 2
                              local.get 4
                              i32.store offset=8
                              local.get 4
                              local.get 2
                              i32.store offset=12
                              br 1 (;@12;)
                            end
                            block  ;; label = @13
                              local.get 5
                              i32.const 20
                              i32.add
                              local.tee 4
                              i32.load
                              local.tee 3
                              br_if 0 (;@13;)
                              local.get 5
                              i32.const 16
                              i32.add
                              local.tee 4
                              i32.load
                              local.tee 3
                              br_if 0 (;@13;)
                              i32.const 0
                              local.set 2
                              br 1 (;@12;)
                            end
                            loop  ;; label = @13
                              local.get 4
                              local.set 8
                              local.get 3
                              local.tee 2
                              i32.const 20
                              i32.add
                              local.tee 4
                              i32.load
                              local.tee 3
                              br_if 0 (;@13;)
                              local.get 2
                              i32.const 16
                              i32.add
                              local.set 4
                              local.get 2
                              i32.load offset=16
                              local.tee 3
                              br_if 0 (;@13;)
                            end
                            local.get 8
                            i32.const 0
                            i32.store
                          end
                          local.get 9
                          i32.eqz
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 5
                              i32.load offset=28
                              local.tee 3
                              i32.const 2
                              i32.shl
                              i32.const 1059156
                              i32.add
                              local.tee 4
                              i32.load
                              local.get 5
                              i32.ne
                              br_if 0 (;@13;)
                              local.get 4
                              local.get 2
                              i32.store
                              local.get 2
                              br_if 1 (;@12;)
                              i32.const 0
                              i32.const 0
                              i32.load offset=1058856
                              i32.const -2
                              local.get 3
                              i32.rotl
                              i32.and
                              i32.store offset=1058856
                              br 2 (;@11;)
                            end
                            local.get 9
                            i32.const 16
                            i32.const 20
                            local.get 9
                            i32.load offset=16
                            local.get 5
                            i32.eq
                            select
                            i32.add
                            local.get 2
                            i32.store
                            local.get 2
                            i32.eqz
                            br_if 1 (;@11;)
                          end
                          local.get 2
                          local.get 9
                          i32.store offset=24
                          block  ;; label = @12
                            local.get 5
                            i32.load offset=16
                            local.tee 4
                            i32.eqz
                            br_if 0 (;@12;)
                            local.get 2
                            local.get 4
                            i32.store offset=16
                            local.get 4
                            local.get 2
                            i32.store offset=24
                          end
                          local.get 5
                          i32.load offset=20
                          local.tee 4
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 2
                          i32.const 20
                          i32.add
                          local.get 4
                          i32.store
                          local.get 4
                          local.get 2
                          i32.store offset=24
                        end
                        local.get 7
                        local.get 0
                        i32.add
                        local.set 0
                        local.get 5
                        local.get 7
                        i32.add
                        local.set 5
                      end
                      local.get 5
                      local.get 5
                      i32.load offset=4
                      i32.const -2
                      i32.and
                      i32.store offset=4
                      local.get 6
                      local.get 0
                      i32.add
                      local.get 0
                      i32.store
                      local.get 6
                      local.get 0
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      block  ;; label = @10
                        local.get 0
                        i32.const 255
                        i32.gt_u
                        br_if 0 (;@10;)
                        local.get 0
                        i32.const 3
                        i32.shr_u
                        local.tee 4
                        i32.const 3
                        i32.shl
                        i32.const 1058892
                        i32.add
                        local.set 0
                        block  ;; label = @11
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1058852
                            local.tee 3
                            i32.const 1
                            local.get 4
                            i32.shl
                            local.tee 4
                            i32.and
                            br_if 0 (;@12;)
                            i32.const 0
                            local.get 3
                            local.get 4
                            i32.or
                            i32.store offset=1058852
                            local.get 0
                            local.set 4
                            br 1 (;@11;)
                          end
                          local.get 0
                          i32.load offset=8
                          local.set 4
                        end
                        local.get 4
                        local.get 6
                        i32.store offset=12
                        local.get 0
                        local.get 6
                        i32.store offset=8
                        local.get 6
                        local.get 0
                        i32.store offset=12
                        local.get 6
                        local.get 4
                        i32.store offset=8
                        br 3 (;@7;)
                      end
                      i32.const 0
                      local.set 4
                      block  ;; label = @10
                        local.get 0
                        i32.const 8
                        i32.shr_u
                        local.tee 3
                        i32.eqz
                        br_if 0 (;@10;)
                        i32.const 31
                        local.set 4
                        local.get 0
                        i32.const 16777215
                        i32.gt_u
                        br_if 0 (;@10;)
                        local.get 3
                        local.get 3
                        i32.const 1048320
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 8
                        i32.and
                        local.tee 4
                        i32.shl
                        local.tee 3
                        local.get 3
                        i32.const 520192
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 4
                        i32.and
                        local.tee 3
                        i32.shl
                        local.tee 5
                        local.get 5
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        local.tee 5
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        local.get 3
                        local.get 4
                        i32.or
                        local.get 5
                        i32.or
                        i32.sub
                        local.tee 4
                        i32.const 1
                        i32.shl
                        local.get 0
                        local.get 4
                        i32.const 21
                        i32.add
                        i32.shr_u
                        i32.const 1
                        i32.and
                        i32.or
                        i32.const 28
                        i32.add
                        local.set 4
                      end
                      local.get 6
                      local.get 4
                      i32.store offset=28
                      local.get 6
                      i64.const 0
                      i64.store offset=16 align=4
                      local.get 4
                      i32.const 2
                      i32.shl
                      i32.const 1059156
                      i32.add
                      local.set 3
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1058856
                        local.tee 5
                        i32.const 1
                        local.get 4
                        i32.shl
                        local.tee 8
                        i32.and
                        br_if 0 (;@10;)
                        local.get 3
                        local.get 6
                        i32.store
                        i32.const 0
                        local.get 5
                        local.get 8
                        i32.or
                        i32.store offset=1058856
                        local.get 6
                        local.get 3
                        i32.store offset=24
                        local.get 6
                        local.get 6
                        i32.store offset=8
                        local.get 6
                        local.get 6
                        i32.store offset=12
                        br 3 (;@7;)
                      end
                      local.get 0
                      i32.const 0
                      i32.const 25
                      local.get 4
                      i32.const 1
                      i32.shr_u
                      i32.sub
                      local.get 4
                      i32.const 31
                      i32.eq
                      select
                      i32.shl
                      local.set 4
                      local.get 3
                      i32.load
                      local.set 5
                      loop  ;; label = @10
                        local.get 5
                        local.tee 3
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get 0
                        i32.eq
                        br_if 2 (;@8;)
                        local.get 4
                        i32.const 29
                        i32.shr_u
                        local.set 5
                        local.get 4
                        i32.const 1
                        i32.shl
                        local.set 4
                        local.get 3
                        local.get 5
                        i32.const 4
                        i32.and
                        i32.add
                        i32.const 16
                        i32.add
                        local.tee 8
                        i32.load
                        local.tee 5
                        br_if 0 (;@10;)
                      end
                      local.get 8
                      local.get 6
                      i32.store
                      local.get 6
                      local.get 3
                      i32.store offset=24
                      local.get 6
                      local.get 6
                      i32.store offset=12
                      local.get 6
                      local.get 6
                      i32.store offset=8
                      br 2 (;@7;)
                    end
                    local.get 5
                    i32.const -8
                    local.get 5
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get 5
                    i32.const 8
                    i32.add
                    i32.const 15
                    i32.and
                    select
                    local.tee 0
                    i32.add
                    local.tee 11
                    local.get 2
                    i32.const -56
                    i32.add
                    local.tee 8
                    local.get 0
                    i32.sub
                    local.tee 0
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get 5
                    local.get 8
                    i32.add
                    i32.const 56
                    i32.store offset=4
                    local.get 4
                    local.get 6
                    i32.const 55
                    local.get 6
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get 6
                    i32.const -55
                    i32.add
                    i32.const 15
                    i32.and
                    select
                    i32.add
                    i32.const -63
                    i32.add
                    local.tee 8
                    local.get 8
                    local.get 4
                    i32.const 16
                    i32.add
                    i32.lt_u
                    select
                    local.tee 8
                    i32.const 35
                    i32.store offset=4
                    i32.const 0
                    i32.const 0
                    i32.load offset=1059340
                    i32.store offset=1058880
                    i32.const 0
                    local.get 0
                    i32.store offset=1058864
                    i32.const 0
                    local.get 11
                    i32.store offset=1058876
                    local.get 8
                    i32.const 16
                    i32.add
                    i32.const 0
                    i64.load offset=1059308 align=4
                    i64.store align=4
                    local.get 8
                    i32.const 0
                    i64.load offset=1059300 align=4
                    i64.store offset=8 align=4
                    i32.const 0
                    local.get 8
                    i32.const 8
                    i32.add
                    i32.store offset=1059308
                    i32.const 0
                    local.get 2
                    i32.store offset=1059304
                    i32.const 0
                    local.get 5
                    i32.store offset=1059300
                    i32.const 0
                    i32.const 0
                    i32.store offset=1059312
                    local.get 8
                    i32.const 36
                    i32.add
                    local.set 0
                    loop  ;; label = @9
                      local.get 0
                      i32.const 7
                      i32.store
                      local.get 6
                      local.get 0
                      i32.const 4
                      i32.add
                      local.tee 0
                      i32.gt_u
                      br_if 0 (;@9;)
                    end
                    local.get 8
                    local.get 4
                    i32.eq
                    br_if 3 (;@5;)
                    local.get 8
                    local.get 8
                    i32.load offset=4
                    i32.const -2
                    i32.and
                    i32.store offset=4
                    local.get 8
                    local.get 8
                    local.get 4
                    i32.sub
                    local.tee 2
                    i32.store
                    local.get 4
                    local.get 2
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    block  ;; label = @9
                      local.get 2
                      i32.const 255
                      i32.gt_u
                      br_if 0 (;@9;)
                      local.get 2
                      i32.const 3
                      i32.shr_u
                      local.tee 6
                      i32.const 3
                      i32.shl
                      i32.const 1058892
                      i32.add
                      local.set 0
                      block  ;; label = @10
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1058852
                          local.tee 5
                          i32.const 1
                          local.get 6
                          i32.shl
                          local.tee 6
                          i32.and
                          br_if 0 (;@11;)
                          i32.const 0
                          local.get 5
                          local.get 6
                          i32.or
                          i32.store offset=1058852
                          local.get 0
                          local.set 6
                          br 1 (;@10;)
                        end
                        local.get 0
                        i32.load offset=8
                        local.set 6
                      end
                      local.get 6
                      local.get 4
                      i32.store offset=12
                      local.get 0
                      local.get 4
                      i32.store offset=8
                      local.get 4
                      local.get 0
                      i32.store offset=12
                      local.get 4
                      local.get 6
                      i32.store offset=8
                      br 4 (;@5;)
                    end
                    i32.const 0
                    local.set 0
                    block  ;; label = @9
                      local.get 2
                      i32.const 8
                      i32.shr_u
                      local.tee 6
                      i32.eqz
                      br_if 0 (;@9;)
                      i32.const 31
                      local.set 0
                      local.get 2
                      i32.const 16777215
                      i32.gt_u
                      br_if 0 (;@9;)
                      local.get 6
                      local.get 6
                      i32.const 1048320
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 8
                      i32.and
                      local.tee 0
                      i32.shl
                      local.tee 6
                      local.get 6
                      i32.const 520192
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 4
                      i32.and
                      local.tee 6
                      i32.shl
                      local.tee 5
                      local.get 5
                      i32.const 245760
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 2
                      i32.and
                      local.tee 5
                      i32.shl
                      i32.const 15
                      i32.shr_u
                      local.get 6
                      local.get 0
                      i32.or
                      local.get 5
                      i32.or
                      i32.sub
                      local.tee 0
                      i32.const 1
                      i32.shl
                      local.get 2
                      local.get 0
                      i32.const 21
                      i32.add
                      i32.shr_u
                      i32.const 1
                      i32.and
                      i32.or
                      i32.const 28
                      i32.add
                      local.set 0
                    end
                    local.get 4
                    i64.const 0
                    i64.store offset=16 align=4
                    local.get 4
                    i32.const 28
                    i32.add
                    local.get 0
                    i32.store
                    local.get 0
                    i32.const 2
                    i32.shl
                    i32.const 1059156
                    i32.add
                    local.set 6
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1058856
                      local.tee 5
                      i32.const 1
                      local.get 0
                      i32.shl
                      local.tee 8
                      i32.and
                      br_if 0 (;@9;)
                      local.get 6
                      local.get 4
                      i32.store
                      i32.const 0
                      local.get 5
                      local.get 8
                      i32.or
                      i32.store offset=1058856
                      local.get 4
                      i32.const 24
                      i32.add
                      local.get 6
                      i32.store
                      local.get 4
                      local.get 4
                      i32.store offset=8
                      local.get 4
                      local.get 4
                      i32.store offset=12
                      br 4 (;@5;)
                    end
                    local.get 2
                    i32.const 0
                    i32.const 25
                    local.get 0
                    i32.const 1
                    i32.shr_u
                    i32.sub
                    local.get 0
                    i32.const 31
                    i32.eq
                    select
                    i32.shl
                    local.set 0
                    local.get 6
                    i32.load
                    local.set 5
                    loop  ;; label = @9
                      local.get 5
                      local.tee 6
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      local.get 2
                      i32.eq
                      br_if 3 (;@6;)
                      local.get 0
                      i32.const 29
                      i32.shr_u
                      local.set 5
                      local.get 0
                      i32.const 1
                      i32.shl
                      local.set 0
                      local.get 6
                      local.get 5
                      i32.const 4
                      i32.and
                      i32.add
                      i32.const 16
                      i32.add
                      local.tee 8
                      i32.load
                      local.tee 5
                      br_if 0 (;@9;)
                    end
                    local.get 8
                    local.get 4
                    i32.store
                    local.get 4
                    i32.const 24
                    i32.add
                    local.get 6
                    i32.store
                    local.get 4
                    local.get 4
                    i32.store offset=12
                    local.get 4
                    local.get 4
                    i32.store offset=8
                    br 3 (;@5;)
                  end
                  local.get 3
                  i32.load offset=8
                  local.set 0
                  local.get 3
                  local.get 6
                  i32.store offset=8
                  local.get 0
                  local.get 6
                  i32.store offset=12
                  local.get 6
                  i32.const 0
                  i32.store offset=24
                  local.get 6
                  local.get 0
                  i32.store offset=8
                  local.get 6
                  local.get 3
                  i32.store offset=12
                end
                local.get 11
                i32.const 8
                i32.add
                local.set 0
                br 5 (;@1;)
              end
              local.get 6
              i32.load offset=8
              local.set 0
              local.get 6
              local.get 4
              i32.store offset=8
              local.get 0
              local.get 4
              i32.store offset=12
              local.get 4
              i32.const 24
              i32.add
              i32.const 0
              i32.store
              local.get 4
              local.get 0
              i32.store offset=8
              local.get 4
              local.get 6
              i32.store offset=12
            end
            i32.const 0
            i32.load offset=1058864
            local.tee 0
            local.get 3
            i32.le_u
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1058876
            local.tee 4
            local.get 3
            i32.add
            local.tee 6
            local.get 0
            local.get 3
            i32.sub
            local.tee 0
            i32.const 1
            i32.or
            i32.store offset=4
            i32.const 0
            local.get 0
            i32.store offset=1058864
            i32.const 0
            local.get 6
            i32.store offset=1058876
            local.get 4
            local.get 3
            i32.const 3
            i32.or
            i32.store offset=4
            local.get 4
            i32.const 8
            i32.add
            local.set 0
            br 3 (;@1;)
          end
          i32.const 0
          local.set 0
          i32.const 0
          i32.const 48
          i32.store offset=1059348
          br 2 (;@1;)
        end
        block  ;; label = @3
          local.get 11
          i32.eqz
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 8
              local.get 8
              i32.load offset=28
              local.tee 4
              i32.const 2
              i32.shl
              i32.const 1059156
              i32.add
              local.tee 0
              i32.load
              i32.ne
              br_if 0 (;@5;)
              local.get 0
              local.get 5
              i32.store
              local.get 5
              br_if 1 (;@4;)
              i32.const 0
              local.get 7
              i32.const -2
              local.get 4
              i32.rotl
              i32.and
              local.tee 7
              i32.store offset=1058856
              br 2 (;@3;)
            end
            local.get 11
            i32.const 16
            i32.const 20
            local.get 11
            i32.load offset=16
            local.get 8
            i32.eq
            select
            i32.add
            local.get 5
            i32.store
            local.get 5
            i32.eqz
            br_if 1 (;@3;)
          end
          local.get 5
          local.get 11
          i32.store offset=24
          block  ;; label = @4
            local.get 8
            i32.load offset=16
            local.tee 0
            i32.eqz
            br_if 0 (;@4;)
            local.get 5
            local.get 0
            i32.store offset=16
            local.get 0
            local.get 5
            i32.store offset=24
          end
          local.get 8
          i32.const 20
          i32.add
          i32.load
          local.tee 0
          i32.eqz
          br_if 0 (;@3;)
          local.get 5
          i32.const 20
          i32.add
          local.get 0
          i32.store
          local.get 0
          local.get 5
          i32.store offset=24
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 6
            i32.const 15
            i32.gt_u
            br_if 0 (;@4;)
            local.get 8
            local.get 6
            local.get 3
            i32.add
            local.tee 0
            i32.const 3
            i32.or
            i32.store offset=4
            local.get 8
            local.get 0
            i32.add
            local.tee 0
            local.get 0
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            br 1 (;@3;)
          end
          local.get 8
          local.get 3
          i32.add
          local.tee 5
          local.get 6
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 8
          local.get 3
          i32.const 3
          i32.or
          i32.store offset=4
          local.get 5
          local.get 6
          i32.add
          local.get 6
          i32.store
          block  ;; label = @4
            local.get 6
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 6
            i32.const 3
            i32.shr_u
            local.tee 4
            i32.const 3
            i32.shl
            i32.const 1058892
            i32.add
            local.set 0
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i32.load offset=1058852
                local.tee 6
                i32.const 1
                local.get 4
                i32.shl
                local.tee 4
                i32.and
                br_if 0 (;@6;)
                i32.const 0
                local.get 6
                local.get 4
                i32.or
                i32.store offset=1058852
                local.get 0
                local.set 4
                br 1 (;@5;)
              end
              local.get 0
              i32.load offset=8
              local.set 4
            end
            local.get 4
            local.get 5
            i32.store offset=12
            local.get 0
            local.get 5
            i32.store offset=8
            local.get 5
            local.get 0
            i32.store offset=12
            local.get 5
            local.get 4
            i32.store offset=8
            br 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 6
              i32.const 8
              i32.shr_u
              local.tee 4
              br_if 0 (;@5;)
              i32.const 0
              local.set 0
              br 1 (;@4;)
            end
            i32.const 31
            local.set 0
            local.get 6
            i32.const 16777215
            i32.gt_u
            br_if 0 (;@4;)
            local.get 4
            local.get 4
            i32.const 1048320
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 8
            i32.and
            local.tee 0
            i32.shl
            local.tee 4
            local.get 4
            i32.const 520192
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 4
            i32.and
            local.tee 4
            i32.shl
            local.tee 3
            local.get 3
            i32.const 245760
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 2
            i32.and
            local.tee 3
            i32.shl
            i32.const 15
            i32.shr_u
            local.get 4
            local.get 0
            i32.or
            local.get 3
            i32.or
            i32.sub
            local.tee 0
            i32.const 1
            i32.shl
            local.get 6
            local.get 0
            i32.const 21
            i32.add
            i32.shr_u
            i32.const 1
            i32.and
            i32.or
            i32.const 28
            i32.add
            local.set 0
          end
          local.get 5
          local.get 0
          i32.store offset=28
          local.get 5
          i64.const 0
          i64.store offset=16 align=4
          local.get 0
          i32.const 2
          i32.shl
          i32.const 1059156
          i32.add
          local.set 4
          block  ;; label = @4
            local.get 7
            i32.const 1
            local.get 0
            i32.shl
            local.tee 3
            i32.and
            br_if 0 (;@4;)
            local.get 4
            local.get 5
            i32.store
            i32.const 0
            local.get 7
            local.get 3
            i32.or
            i32.store offset=1058856
            local.get 5
            local.get 4
            i32.store offset=24
            local.get 5
            local.get 5
            i32.store offset=8
            local.get 5
            local.get 5
            i32.store offset=12
            br 1 (;@3;)
          end
          local.get 6
          i32.const 0
          i32.const 25
          local.get 0
          i32.const 1
          i32.shr_u
          i32.sub
          local.get 0
          i32.const 31
          i32.eq
          select
          i32.shl
          local.set 0
          local.get 4
          i32.load
          local.set 3
          block  ;; label = @4
            loop  ;; label = @5
              local.get 3
              local.tee 4
              i32.load offset=4
              i32.const -8
              i32.and
              local.get 6
              i32.eq
              br_if 1 (;@4;)
              local.get 0
              i32.const 29
              i32.shr_u
              local.set 3
              local.get 0
              i32.const 1
              i32.shl
              local.set 0
              local.get 4
              local.get 3
              i32.const 4
              i32.and
              i32.add
              i32.const 16
              i32.add
              local.tee 2
              i32.load
              local.tee 3
              br_if 0 (;@5;)
            end
            local.get 2
            local.get 5
            i32.store
            local.get 5
            local.get 4
            i32.store offset=24
            local.get 5
            local.get 5
            i32.store offset=12
            local.get 5
            local.get 5
            i32.store offset=8
            br 1 (;@3;)
          end
          local.get 4
          i32.load offset=8
          local.set 0
          local.get 4
          local.get 5
          i32.store offset=8
          local.get 0
          local.get 5
          i32.store offset=12
          local.get 5
          i32.const 0
          i32.store offset=24
          local.get 5
          local.get 0
          i32.store offset=8
          local.get 5
          local.get 4
          i32.store offset=12
        end
        local.get 8
        i32.const 8
        i32.add
        local.set 0
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 10
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            local.get 5
            local.get 5
            i32.load offset=28
            local.tee 6
            i32.const 2
            i32.shl
            i32.const 1059156
            i32.add
            local.tee 0
            i32.load
            i32.ne
            br_if 0 (;@4;)
            local.get 0
            local.get 8
            i32.store
            local.get 8
            br_if 1 (;@3;)
            i32.const 0
            local.get 9
            i32.const -2
            local.get 6
            i32.rotl
            i32.and
            i32.store offset=1058856
            br 2 (;@2;)
          end
          local.get 10
          i32.const 16
          i32.const 20
          local.get 10
          i32.load offset=16
          local.get 5
          i32.eq
          select
          i32.add
          local.get 8
          i32.store
          local.get 8
          i32.eqz
          br_if 1 (;@2;)
        end
        local.get 8
        local.get 10
        i32.store offset=24
        block  ;; label = @3
          local.get 5
          i32.load offset=16
          local.tee 0
          i32.eqz
          br_if 0 (;@3;)
          local.get 8
          local.get 0
          i32.store offset=16
          local.get 0
          local.get 8
          i32.store offset=24
        end
        local.get 5
        i32.const 20
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 8
        i32.const 20
        i32.add
        local.get 0
        i32.store
        local.get 0
        local.get 8
        i32.store offset=24
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.const 15
          i32.gt_u
          br_if 0 (;@3;)
          local.get 5
          local.get 4
          local.get 3
          i32.add
          local.tee 0
          i32.const 3
          i32.or
          i32.store offset=4
          local.get 5
          local.get 0
          i32.add
          local.tee 0
          local.get 0
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          br 1 (;@2;)
        end
        local.get 5
        local.get 3
        i32.add
        local.tee 6
        local.get 4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get 5
        local.get 3
        i32.const 3
        i32.or
        i32.store offset=4
        local.get 6
        local.get 4
        i32.add
        local.get 4
        i32.store
        block  ;; label = @3
          local.get 7
          i32.eqz
          br_if 0 (;@3;)
          local.get 7
          i32.const 3
          i32.shr_u
          local.tee 8
          i32.const 3
          i32.shl
          i32.const 1058892
          i32.add
          local.set 3
          i32.const 0
          i32.load offset=1058872
          local.set 0
          block  ;; label = @4
            block  ;; label = @5
              i32.const 1
              local.get 8
              i32.shl
              local.tee 8
              local.get 2
              i32.and
              br_if 0 (;@5;)
              i32.const 0
              local.get 8
              local.get 2
              i32.or
              i32.store offset=1058852
              local.get 3
              local.set 8
              br 1 (;@4;)
            end
            local.get 3
            i32.load offset=8
            local.set 8
          end
          local.get 8
          local.get 0
          i32.store offset=12
          local.get 3
          local.get 0
          i32.store offset=8
          local.get 0
          local.get 3
          i32.store offset=12
          local.get 0
          local.get 8
          i32.store offset=8
        end
        i32.const 0
        local.get 6
        i32.store offset=1058872
        i32.const 0
        local.get 4
        i32.store offset=1058860
      end
      local.get 5
      i32.const 8
      i32.add
      local.set 0
    end
    local.get 1
    i32.const 16
    i32.add
    global.set 0
    local.get 0)
  (func $free (type 1) (param i32)
    local.get 0
    call $dlfree)
  (func $dlfree (type 1) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      local.get 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const -8
      i32.add
      local.tee 1
      local.get 0
      i32.const -4
      i32.add
      i32.load
      local.tee 2
      i32.const -8
      i32.and
      local.tee 0
      i32.add
      local.set 3
      block  ;; label = @2
        local.get 2
        i32.const 1
        i32.and
        br_if 0 (;@2;)
        local.get 2
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 1
        local.get 1
        i32.load
        local.tee 2
        i32.sub
        local.tee 1
        i32.const 0
        i32.load offset=1058868
        local.tee 4
        i32.lt_u
        br_if 1 (;@1;)
        local.get 2
        local.get 0
        i32.add
        local.set 0
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058872
          local.get 1
          i32.eq
          br_if 0 (;@3;)
          block  ;; label = @4
            local.get 2
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 1
            i32.load offset=12
            local.set 5
            block  ;; label = @5
              local.get 1
              i32.load offset=8
              local.tee 6
              local.get 2
              i32.const 3
              i32.shr_u
              local.tee 7
              i32.const 3
              i32.shl
              i32.const 1058892
              i32.add
              local.tee 2
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              local.get 6
              i32.gt_u
              drop
            end
            block  ;; label = @5
              local.get 5
              local.get 6
              i32.ne
              br_if 0 (;@5;)
              i32.const 0
              i32.const 0
              i32.load offset=1058852
              i32.const -2
              local.get 7
              i32.rotl
              i32.and
              i32.store offset=1058852
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 5
              local.get 2
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              local.get 5
              i32.gt_u
              drop
            end
            local.get 5
            local.get 6
            i32.store offset=8
            local.get 6
            local.get 5
            i32.store offset=12
            br 2 (;@2;)
          end
          local.get 1
          i32.load offset=24
          local.set 7
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load offset=12
              local.tee 5
              local.get 1
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 4
                local.get 1
                i32.load offset=8
                local.tee 2
                i32.gt_u
                br_if 0 (;@6;)
                local.get 2
                i32.load offset=12
                local.get 1
                i32.ne
                drop
              end
              local.get 5
              local.get 2
              i32.store offset=8
              local.get 2
              local.get 5
              i32.store offset=12
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 1
              i32.const 20
              i32.add
              local.tee 2
              i32.load
              local.tee 4
              br_if 0 (;@5;)
              local.get 1
              i32.const 16
              i32.add
              local.tee 2
              i32.load
              local.tee 4
              br_if 0 (;@5;)
              i32.const 0
              local.set 5
              br 1 (;@4;)
            end
            loop  ;; label = @5
              local.get 2
              local.set 6
              local.get 4
              local.tee 5
              i32.const 20
              i32.add
              local.tee 2
              i32.load
              local.tee 4
              br_if 0 (;@5;)
              local.get 5
              i32.const 16
              i32.add
              local.set 2
              local.get 5
              i32.load offset=16
              local.tee 4
              br_if 0 (;@5;)
            end
            local.get 6
            i32.const 0
            i32.store
          end
          local.get 7
          i32.eqz
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load offset=28
              local.tee 4
              i32.const 2
              i32.shl
              i32.const 1059156
              i32.add
              local.tee 2
              i32.load
              local.get 1
              i32.ne
              br_if 0 (;@5;)
              local.get 2
              local.get 5
              i32.store
              local.get 5
              br_if 1 (;@4;)
              i32.const 0
              i32.const 0
              i32.load offset=1058856
              i32.const -2
              local.get 4
              i32.rotl
              i32.and
              i32.store offset=1058856
              br 3 (;@2;)
            end
            local.get 7
            i32.const 16
            i32.const 20
            local.get 7
            i32.load offset=16
            local.get 1
            i32.eq
            select
            i32.add
            local.get 5
            i32.store
            local.get 5
            i32.eqz
            br_if 2 (;@2;)
          end
          local.get 5
          local.get 7
          i32.store offset=24
          block  ;; label = @4
            local.get 1
            i32.load offset=16
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 5
            local.get 2
            i32.store offset=16
            local.get 2
            local.get 5
            i32.store offset=24
          end
          local.get 1
          i32.load offset=20
          local.tee 2
          i32.eqz
          br_if 1 (;@2;)
          local.get 5
          i32.const 20
          i32.add
          local.get 2
          i32.store
          local.get 2
          local.get 5
          i32.store offset=24
          br 1 (;@2;)
        end
        local.get 3
        i32.load offset=4
        local.tee 2
        i32.const 3
        i32.and
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 3
        local.get 2
        i32.const -2
        i32.and
        i32.store offset=4
        i32.const 0
        local.get 0
        i32.store offset=1058860
        local.get 1
        local.get 0
        i32.add
        local.get 0
        i32.store
        local.get 1
        local.get 0
        i32.const 1
        i32.or
        i32.store offset=4
        return
      end
      local.get 3
      local.get 1
      i32.le_u
      br_if 0 (;@1;)
      local.get 3
      i32.load offset=4
      local.tee 2
      i32.const 1
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.const 2
          i32.and
          br_if 0 (;@3;)
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058876
            local.get 3
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 1
            i32.store offset=1058876
            i32.const 0
            i32.const 0
            i32.load offset=1058864
            local.get 0
            i32.add
            local.tee 0
            i32.store offset=1058864
            local.get 1
            local.get 0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 1
            i32.const 0
            i32.load offset=1058872
            i32.ne
            br_if 3 (;@1;)
            i32.const 0
            i32.const 0
            i32.store offset=1058860
            i32.const 0
            i32.const 0
            i32.store offset=1058872
            return
          end
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058872
            local.get 3
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 1
            i32.store offset=1058872
            i32.const 0
            i32.const 0
            i32.load offset=1058860
            local.get 0
            i32.add
            local.tee 0
            i32.store offset=1058860
            local.get 1
            local.get 0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 1
            local.get 0
            i32.add
            local.get 0
            i32.store
            return
          end
          local.get 2
          i32.const -8
          i32.and
          local.get 0
          i32.add
          local.set 0
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              i32.const 255
              i32.gt_u
              br_if 0 (;@5;)
              local.get 3
              i32.load offset=12
              local.set 4
              block  ;; label = @6
                local.get 3
                i32.load offset=8
                local.tee 5
                local.get 2
                i32.const 3
                i32.shr_u
                local.tee 3
                i32.const 3
                i32.shl
                i32.const 1058892
                i32.add
                local.tee 2
                i32.eq
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1058868
                local.get 5
                i32.gt_u
                drop
              end
              block  ;; label = @6
                local.get 4
                local.get 5
                i32.ne
                br_if 0 (;@6;)
                i32.const 0
                i32.const 0
                i32.load offset=1058852
                i32.const -2
                local.get 3
                i32.rotl
                i32.and
                i32.store offset=1058852
                br 2 (;@4;)
              end
              block  ;; label = @6
                local.get 4
                local.get 2
                i32.eq
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1058868
                local.get 4
                i32.gt_u
                drop
              end
              local.get 4
              local.get 5
              i32.store offset=8
              local.get 5
              local.get 4
              i32.store offset=12
              br 1 (;@4;)
            end
            local.get 3
            i32.load offset=24
            local.set 7
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                i32.load offset=12
                local.tee 5
                local.get 3
                i32.eq
                br_if 0 (;@6;)
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1058868
                  local.get 3
                  i32.load offset=8
                  local.tee 2
                  i32.gt_u
                  br_if 0 (;@7;)
                  local.get 2
                  i32.load offset=12
                  local.get 3
                  i32.ne
                  drop
                end
                local.get 5
                local.get 2
                i32.store offset=8
                local.get 2
                local.get 5
                i32.store offset=12
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 3
                i32.const 20
                i32.add
                local.tee 2
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                local.get 3
                i32.const 16
                i32.add
                local.tee 2
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                i32.const 0
                local.set 5
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 2
                local.set 6
                local.get 4
                local.tee 5
                i32.const 20
                i32.add
                local.tee 2
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                local.get 5
                i32.const 16
                i32.add
                local.set 2
                local.get 5
                i32.load offset=16
                local.tee 4
                br_if 0 (;@6;)
              end
              local.get 6
              i32.const 0
              i32.store
            end
            local.get 7
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                i32.load offset=28
                local.tee 4
                i32.const 2
                i32.shl
                i32.const 1059156
                i32.add
                local.tee 2
                i32.load
                local.get 3
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.get 5
                i32.store
                local.get 5
                br_if 1 (;@5;)
                i32.const 0
                i32.const 0
                i32.load offset=1058856
                i32.const -2
                local.get 4
                i32.rotl
                i32.and
                i32.store offset=1058856
                br 2 (;@4;)
              end
              local.get 7
              i32.const 16
              i32.const 20
              local.get 7
              i32.load offset=16
              local.get 3
              i32.eq
              select
              i32.add
              local.get 5
              i32.store
              local.get 5
              i32.eqz
              br_if 1 (;@4;)
            end
            local.get 5
            local.get 7
            i32.store offset=24
            block  ;; label = @5
              local.get 3
              i32.load offset=16
              local.tee 2
              i32.eqz
              br_if 0 (;@5;)
              local.get 5
              local.get 2
              i32.store offset=16
              local.get 2
              local.get 5
              i32.store offset=24
            end
            local.get 3
            i32.load offset=20
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 5
            i32.const 20
            i32.add
            local.get 2
            i32.store
            local.get 2
            local.get 5
            i32.store offset=24
          end
          local.get 1
          local.get 0
          i32.add
          local.get 0
          i32.store
          local.get 1
          local.get 0
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 1
          i32.const 0
          i32.load offset=1058872
          i32.ne
          br_if 1 (;@2;)
          i32.const 0
          local.get 0
          i32.store offset=1058860
          return
        end
        local.get 3
        local.get 2
        i32.const -2
        i32.and
        i32.store offset=4
        local.get 1
        local.get 0
        i32.add
        local.get 0
        i32.store
        local.get 1
        local.get 0
        i32.const 1
        i32.or
        i32.store offset=4
      end
      block  ;; label = @2
        local.get 0
        i32.const 255
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        i32.const 3
        i32.shr_u
        local.tee 2
        i32.const 3
        i32.shl
        i32.const 1058892
        i32.add
        local.set 0
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058852
            local.tee 4
            i32.const 1
            local.get 2
            i32.shl
            local.tee 2
            i32.and
            br_if 0 (;@4;)
            i32.const 0
            local.get 4
            local.get 2
            i32.or
            i32.store offset=1058852
            local.get 0
            local.set 2
            br 1 (;@3;)
          end
          local.get 0
          i32.load offset=8
          local.set 2
        end
        local.get 2
        local.get 1
        i32.store offset=12
        local.get 0
        local.get 1
        i32.store offset=8
        local.get 1
        local.get 0
        i32.store offset=12
        local.get 1
        local.get 2
        i32.store offset=8
        return
      end
      i32.const 0
      local.set 2
      block  ;; label = @2
        local.get 0
        i32.const 8
        i32.shr_u
        local.tee 4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 31
        local.set 2
        local.get 0
        i32.const 16777215
        i32.gt_u
        br_if 0 (;@2;)
        local.get 4
        local.get 4
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee 2
        i32.shl
        local.tee 4
        local.get 4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.tee 4
        i32.shl
        local.tee 5
        local.get 5
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.tee 5
        i32.shl
        i32.const 15
        i32.shr_u
        local.get 4
        local.get 2
        i32.or
        local.get 5
        i32.or
        i32.sub
        local.tee 2
        i32.const 1
        i32.shl
        local.get 0
        local.get 2
        i32.const 21
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        i32.or
        i32.const 28
        i32.add
        local.set 2
      end
      local.get 1
      i64.const 0
      i64.store offset=16 align=4
      local.get 1
      i32.const 28
      i32.add
      local.get 2
      i32.store
      local.get 2
      i32.const 2
      i32.shl
      i32.const 1059156
      i32.add
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058856
          local.tee 5
          i32.const 1
          local.get 2
          i32.shl
          local.tee 3
          i32.and
          br_if 0 (;@3;)
          local.get 4
          local.get 1
          i32.store
          i32.const 0
          local.get 5
          local.get 3
          i32.or
          i32.store offset=1058856
          local.get 1
          i32.const 24
          i32.add
          local.get 4
          i32.store
          local.get 1
          local.get 1
          i32.store offset=8
          local.get 1
          local.get 1
          i32.store offset=12
          br 1 (;@2;)
        end
        local.get 0
        i32.const 0
        i32.const 25
        local.get 2
        i32.const 1
        i32.shr_u
        i32.sub
        local.get 2
        i32.const 31
        i32.eq
        select
        i32.shl
        local.set 2
        local.get 4
        i32.load
        local.set 5
        block  ;; label = @3
          loop  ;; label = @4
            local.get 5
            local.tee 4
            i32.load offset=4
            i32.const -8
            i32.and
            local.get 0
            i32.eq
            br_if 1 (;@3;)
            local.get 2
            i32.const 29
            i32.shr_u
            local.set 5
            local.get 2
            i32.const 1
            i32.shl
            local.set 2
            local.get 4
            local.get 5
            i32.const 4
            i32.and
            i32.add
            i32.const 16
            i32.add
            local.tee 3
            i32.load
            local.tee 5
            br_if 0 (;@4;)
          end
          local.get 3
          local.get 1
          i32.store
          local.get 1
          i32.const 24
          i32.add
          local.get 4
          i32.store
          local.get 1
          local.get 1
          i32.store offset=12
          local.get 1
          local.get 1
          i32.store offset=8
          br 1 (;@2;)
        end
        local.get 4
        i32.load offset=8
        local.set 0
        local.get 4
        local.get 1
        i32.store offset=8
        local.get 0
        local.get 1
        i32.store offset=12
        local.get 1
        i32.const 24
        i32.add
        i32.const 0
        i32.store
        local.get 1
        local.get 0
        i32.store offset=8
        local.get 1
        local.get 4
        i32.store offset=12
      end
      i32.const 0
      i32.const 0
      i32.load offset=1058884
      i32.const -1
      i32.add
      local.tee 1
      i32.store offset=1058884
      local.get 1
      br_if 0 (;@1;)
      i32.const 1059308
      local.set 1
      loop  ;; label = @2
        local.get 1
        i32.load
        local.tee 0
        i32.const 8
        i32.add
        local.set 1
        local.get 0
        br_if 0 (;@2;)
      end
      i32.const 0
      i32.const -1
      i32.store offset=1058884
    end)
  (func $calloc (type 3) (param i32 i32) (result i32)
    (local i32 i64)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        br_if 0 (;@2;)
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      local.get 0
      i64.extend_i32_u
      local.get 1
      i64.extend_i32_u
      i64.mul
      local.tee 3
      i32.wrap_i64
      local.set 2
      local.get 1
      local.get 0
      i32.or
      i32.const 65536
      i32.lt_u
      br_if 0 (;@1;)
      i32.const -1
      local.get 2
      local.get 3
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.const 0
      i32.ne
      select
      local.set 2
    end
    block  ;; label = @1
      local.get 2
      call $dlmalloc
      local.tee 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const -4
      i32.add
      i32.load8_u
      i32.const 3
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 0
      local.get 2
      call $memset
      drop
    end
    local.get 0)
  (func $realloc (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      local.get 1
      call $dlmalloc
      return
    end
    block  ;; label = @1
      local.get 1
      i32.const -64
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 0
      i32.const 48
      i32.store offset=1059348
      i32.const 0
      return
    end
    local.get 1
    i32.const 11
    i32.lt_u
    local.set 2
    local.get 1
    i32.const 19
    i32.add
    i32.const -16
    i32.and
    local.set 3
    local.get 0
    i32.const -8
    i32.add
    local.set 4
    local.get 0
    i32.const -4
    i32.add
    local.tee 5
    i32.load
    local.tee 6
    i32.const 3
    i32.and
    local.set 7
    i32.const 0
    i32.load offset=1058868
    local.set 8
    block  ;; label = @1
      local.get 6
      i32.const -8
      i32.and
      local.tee 9
      i32.const 1
      i32.lt_s
      br_if 0 (;@1;)
      local.get 7
      i32.const 1
      i32.eq
      br_if 0 (;@1;)
      local.get 8
      local.get 4
      i32.gt_u
      drop
    end
    i32.const 16
    local.get 3
    local.get 2
    select
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 7
          br_if 0 (;@3;)
          local.get 2
          i32.const 256
          i32.lt_u
          br_if 1 (;@2;)
          local.get 9
          local.get 2
          i32.const 4
          i32.or
          i32.lt_u
          br_if 1 (;@2;)
          local.get 9
          local.get 2
          i32.sub
          i32.const 0
          i32.load offset=1059332
          i32.const 1
          i32.shl
          i32.le_u
          br_if 2 (;@1;)
          br 1 (;@2;)
        end
        local.get 4
        local.get 9
        i32.add
        local.set 7
        block  ;; label = @3
          local.get 9
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
          local.get 9
          local.get 2
          i32.sub
          local.tee 1
          i32.const 16
          i32.lt_u
          br_if 2 (;@1;)
          local.get 5
          local.get 2
          local.get 6
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get 4
          local.get 2
          i32.add
          local.tee 2
          local.get 1
          i32.const 3
          i32.or
          i32.store offset=4
          local.get 7
          local.get 7
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 2
          local.get 1
          call $dispose_chunk
          local.get 0
          return
        end
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058876
          local.get 7
          i32.ne
          br_if 0 (;@3;)
          i32.const 0
          i32.load offset=1058864
          local.get 9
          i32.add
          local.tee 9
          local.get 2
          i32.le_u
          br_if 1 (;@2;)
          local.get 5
          local.get 2
          local.get 6
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          i32.const 0
          local.get 4
          local.get 2
          i32.add
          local.tee 1
          i32.store offset=1058876
          i32.const 0
          local.get 9
          local.get 2
          i32.sub
          local.tee 2
          i32.store offset=1058864
          local.get 1
          local.get 2
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 0
          return
        end
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058872
          local.get 7
          i32.ne
          br_if 0 (;@3;)
          i32.const 0
          i32.load offset=1058860
          local.get 9
          i32.add
          local.tee 9
          local.get 2
          i32.lt_u
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 9
              local.get 2
              i32.sub
              local.tee 1
              i32.const 16
              i32.lt_u
              br_if 0 (;@5;)
              local.get 5
              local.get 2
              local.get 6
              i32.const 1
              i32.and
              i32.or
              i32.const 2
              i32.or
              i32.store
              local.get 4
              local.get 2
              i32.add
              local.tee 2
              local.get 1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get 4
              local.get 9
              i32.add
              local.tee 9
              local.get 1
              i32.store
              local.get 9
              local.get 9
              i32.load offset=4
              i32.const -2
              i32.and
              i32.store offset=4
              br 1 (;@4;)
            end
            local.get 5
            local.get 6
            i32.const 1
            i32.and
            local.get 9
            i32.or
            i32.const 2
            i32.or
            i32.store
            local.get 4
            local.get 9
            i32.add
            local.tee 1
            local.get 1
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            i32.const 0
            local.set 1
            i32.const 0
            local.set 2
          end
          i32.const 0
          local.get 2
          i32.store offset=1058872
          i32.const 0
          local.get 1
          i32.store offset=1058860
          local.get 0
          return
        end
        local.get 7
        i32.load offset=4
        local.tee 3
        i32.const 2
        i32.and
        br_if 0 (;@2;)
        local.get 3
        i32.const -8
        i32.and
        local.get 9
        i32.add
        local.tee 10
        local.get 2
        i32.lt_u
        br_if 0 (;@2;)
        local.get 10
        local.get 2
        i32.sub
        local.set 11
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 7
            i32.load offset=12
            local.set 1
            block  ;; label = @5
              local.get 7
              i32.load offset=8
              local.tee 9
              local.get 3
              i32.const 3
              i32.shr_u
              local.tee 3
              i32.const 3
              i32.shl
              i32.const 1058892
              i32.add
              local.tee 7
              i32.eq
              br_if 0 (;@5;)
              local.get 8
              local.get 9
              i32.gt_u
              drop
            end
            block  ;; label = @5
              local.get 1
              local.get 9
              i32.ne
              br_if 0 (;@5;)
              i32.const 0
              i32.const 0
              i32.load offset=1058852
              i32.const -2
              local.get 3
              i32.rotl
              i32.and
              i32.store offset=1058852
              br 2 (;@3;)
            end
            block  ;; label = @5
              local.get 1
              local.get 7
              i32.eq
              br_if 0 (;@5;)
              local.get 8
              local.get 1
              i32.gt_u
              drop
            end
            local.get 1
            local.get 9
            i32.store offset=8
            local.get 9
            local.get 1
            i32.store offset=12
            br 1 (;@3;)
          end
          local.get 7
          i32.load offset=24
          local.set 12
          block  ;; label = @4
            block  ;; label = @5
              local.get 7
              i32.load offset=12
              local.tee 3
              local.get 7
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 8
                local.get 7
                i32.load offset=8
                local.tee 1
                i32.gt_u
                br_if 0 (;@6;)
                local.get 1
                i32.load offset=12
                local.get 7
                i32.ne
                drop
              end
              local.get 3
              local.get 1
              i32.store offset=8
              local.get 1
              local.get 3
              i32.store offset=12
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 7
              i32.const 20
              i32.add
              local.tee 1
              i32.load
              local.tee 9
              br_if 0 (;@5;)
              local.get 7
              i32.const 16
              i32.add
              local.tee 1
              i32.load
              local.tee 9
              br_if 0 (;@5;)
              i32.const 0
              local.set 3
              br 1 (;@4;)
            end
            loop  ;; label = @5
              local.get 1
              local.set 8
              local.get 9
              local.tee 3
              i32.const 20
              i32.add
              local.tee 1
              i32.load
              local.tee 9
              br_if 0 (;@5;)
              local.get 3
              i32.const 16
              i32.add
              local.set 1
              local.get 3
              i32.load offset=16
              local.tee 9
              br_if 0 (;@5;)
            end
            local.get 8
            i32.const 0
            i32.store
          end
          local.get 12
          i32.eqz
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 7
              i32.load offset=28
              local.tee 9
              i32.const 2
              i32.shl
              i32.const 1059156
              i32.add
              local.tee 1
              i32.load
              local.get 7
              i32.ne
              br_if 0 (;@5;)
              local.get 1
              local.get 3
              i32.store
              local.get 3
              br_if 1 (;@4;)
              i32.const 0
              i32.const 0
              i32.load offset=1058856
              i32.const -2
              local.get 9
              i32.rotl
              i32.and
              i32.store offset=1058856
              br 2 (;@3;)
            end
            local.get 12
            i32.const 16
            i32.const 20
            local.get 12
            i32.load offset=16
            local.get 7
            i32.eq
            select
            i32.add
            local.get 3
            i32.store
            local.get 3
            i32.eqz
            br_if 1 (;@3;)
          end
          local.get 3
          local.get 12
          i32.store offset=24
          block  ;; label = @4
            local.get 7
            i32.load offset=16
            local.tee 1
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            local.get 1
            i32.store offset=16
            local.get 1
            local.get 3
            i32.store offset=24
          end
          local.get 7
          i32.load offset=20
          local.tee 1
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          i32.const 20
          i32.add
          local.get 1
          i32.store
          local.get 1
          local.get 3
          i32.store offset=24
        end
        block  ;; label = @3
          local.get 11
          i32.const 15
          i32.gt_u
          br_if 0 (;@3;)
          local.get 5
          local.get 6
          i32.const 1
          i32.and
          local.get 10
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get 4
          local.get 10
          i32.add
          local.tee 1
          local.get 1
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 0
          return
        end
        local.get 5
        local.get 2
        local.get 6
        i32.const 1
        i32.and
        i32.or
        i32.const 2
        i32.or
        i32.store
        local.get 4
        local.get 2
        i32.add
        local.tee 1
        local.get 11
        i32.const 3
        i32.or
        i32.store offset=4
        local.get 4
        local.get 10
        i32.add
        local.tee 2
        local.get 2
        i32.load offset=4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get 1
        local.get 11
        call $dispose_chunk
        local.get 0
        return
      end
      block  ;; label = @2
        local.get 1
        call $dlmalloc
        local.tee 2
        br_if 0 (;@2;)
        i32.const 0
        return
      end
      local.get 2
      local.get 0
      local.get 5
      i32.load
      local.tee 9
      i32.const -8
      i32.and
      i32.const 4
      i32.const 8
      local.get 9
      i32.const 3
      i32.and
      select
      i32.sub
      local.tee 9
      local.get 1
      local.get 9
      local.get 1
      i32.lt_u
      select
      call $memcpy
      local.set 1
      local.get 0
      call $dlfree
      local.get 1
      local.set 0
    end
    local.get 0)
  (func $dispose_chunk (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    local.get 0
    local.get 1
    i32.add
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load offset=4
        local.tee 3
        i32.const 1
        i32.and
        br_if 0 (;@2;)
        local.get 3
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.tee 3
        local.get 1
        i32.add
        local.set 1
        block  ;; label = @3
          i32.const 0
          i32.load offset=1058872
          local.get 0
          local.get 3
          i32.sub
          local.tee 0
          i32.eq
          br_if 0 (;@3;)
          i32.const 0
          i32.load offset=1058868
          local.set 4
          block  ;; label = @4
            local.get 3
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 0
            i32.load offset=12
            local.set 5
            block  ;; label = @5
              local.get 0
              i32.load offset=8
              local.tee 6
              local.get 3
              i32.const 3
              i32.shr_u
              local.tee 7
              i32.const 3
              i32.shl
              i32.const 1058892
              i32.add
              local.tee 3
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              local.get 6
              i32.gt_u
              drop
            end
            block  ;; label = @5
              local.get 5
              local.get 6
              i32.ne
              br_if 0 (;@5;)
              i32.const 0
              i32.const 0
              i32.load offset=1058852
              i32.const -2
              local.get 7
              i32.rotl
              i32.and
              i32.store offset=1058852
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 5
              local.get 3
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              local.get 5
              i32.gt_u
              drop
            end
            local.get 5
            local.get 6
            i32.store offset=8
            local.get 6
            local.get 5
            i32.store offset=12
            br 2 (;@2;)
          end
          local.get 0
          i32.load offset=24
          local.set 7
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load offset=12
              local.tee 6
              local.get 0
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 4
                local.get 0
                i32.load offset=8
                local.tee 3
                i32.gt_u
                br_if 0 (;@6;)
                local.get 3
                i32.load offset=12
                local.get 0
                i32.ne
                drop
              end
              local.get 6
              local.get 3
              i32.store offset=8
              local.get 3
              local.get 6
              i32.store offset=12
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 0
              i32.const 20
              i32.add
              local.tee 3
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              local.get 0
              i32.const 16
              i32.add
              local.tee 3
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              i32.const 0
              local.set 6
              br 1 (;@4;)
            end
            loop  ;; label = @5
              local.get 3
              local.set 4
              local.get 5
              local.tee 6
              i32.const 20
              i32.add
              local.tee 3
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              local.get 6
              i32.const 16
              i32.add
              local.set 3
              local.get 6
              i32.load offset=16
              local.tee 5
              br_if 0 (;@5;)
            end
            local.get 4
            i32.const 0
            i32.store
          end
          local.get 7
          i32.eqz
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load offset=28
              local.tee 5
              i32.const 2
              i32.shl
              i32.const 1059156
              i32.add
              local.tee 3
              i32.load
              local.get 0
              i32.ne
              br_if 0 (;@5;)
              local.get 3
              local.get 6
              i32.store
              local.get 6
              br_if 1 (;@4;)
              i32.const 0
              i32.const 0
              i32.load offset=1058856
              i32.const -2
              local.get 5
              i32.rotl
              i32.and
              i32.store offset=1058856
              br 3 (;@2;)
            end
            local.get 7
            i32.const 16
            i32.const 20
            local.get 7
            i32.load offset=16
            local.get 0
            i32.eq
            select
            i32.add
            local.get 6
            i32.store
            local.get 6
            i32.eqz
            br_if 2 (;@2;)
          end
          local.get 6
          local.get 7
          i32.store offset=24
          block  ;; label = @4
            local.get 0
            i32.load offset=16
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            local.get 3
            i32.store offset=16
            local.get 3
            local.get 6
            i32.store offset=24
          end
          local.get 0
          i32.load offset=20
          local.tee 3
          i32.eqz
          br_if 1 (;@2;)
          local.get 6
          i32.const 20
          i32.add
          local.get 3
          i32.store
          local.get 3
          local.get 6
          i32.store offset=24
          br 1 (;@2;)
        end
        local.get 2
        i32.load offset=4
        local.tee 3
        i32.const 3
        i32.and
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        local.get 3
        i32.const -2
        i32.and
        i32.store offset=4
        i32.const 0
        local.get 1
        i32.store offset=1058860
        local.get 2
        local.get 1
        i32.store
        local.get 0
        local.get 1
        i32.const 1
        i32.or
        i32.store offset=4
        return
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.load offset=4
          local.tee 3
          i32.const 2
          i32.and
          br_if 0 (;@3;)
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058876
            local.get 2
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 0
            i32.store offset=1058876
            i32.const 0
            i32.const 0
            i32.load offset=1058864
            local.get 1
            i32.add
            local.tee 1
            i32.store offset=1058864
            local.get 0
            local.get 1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 0
            i32.const 0
            i32.load offset=1058872
            i32.ne
            br_if 3 (;@1;)
            i32.const 0
            i32.const 0
            i32.store offset=1058860
            i32.const 0
            i32.const 0
            i32.store offset=1058872
            return
          end
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058872
            local.get 2
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 0
            i32.store offset=1058872
            i32.const 0
            i32.const 0
            i32.load offset=1058860
            local.get 1
            i32.add
            local.tee 1
            i32.store offset=1058860
            local.get 0
            local.get 1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 0
            local.get 1
            i32.add
            local.get 1
            i32.store
            return
          end
          i32.const 0
          i32.load offset=1058868
          local.set 4
          local.get 3
          i32.const -8
          i32.and
          local.get 1
          i32.add
          local.set 1
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.const 255
              i32.gt_u
              br_if 0 (;@5;)
              local.get 2
              i32.load offset=12
              local.set 5
              block  ;; label = @6
                local.get 2
                i32.load offset=8
                local.tee 6
                local.get 3
                i32.const 3
                i32.shr_u
                local.tee 2
                i32.const 3
                i32.shl
                i32.const 1058892
                i32.add
                local.tee 3
                i32.eq
                br_if 0 (;@6;)
                local.get 4
                local.get 6
                i32.gt_u
                drop
              end
              block  ;; label = @6
                local.get 5
                local.get 6
                i32.ne
                br_if 0 (;@6;)
                i32.const 0
                i32.const 0
                i32.load offset=1058852
                i32.const -2
                local.get 2
                i32.rotl
                i32.and
                i32.store offset=1058852
                br 2 (;@4;)
              end
              block  ;; label = @6
                local.get 5
                local.get 3
                i32.eq
                br_if 0 (;@6;)
                local.get 4
                local.get 5
                i32.gt_u
                drop
              end
              local.get 5
              local.get 6
              i32.store offset=8
              local.get 6
              local.get 5
              i32.store offset=12
              br 1 (;@4;)
            end
            local.get 2
            i32.load offset=24
            local.set 7
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.load offset=12
                local.tee 6
                local.get 2
                i32.eq
                br_if 0 (;@6;)
                block  ;; label = @7
                  local.get 4
                  local.get 2
                  i32.load offset=8
                  local.tee 3
                  i32.gt_u
                  br_if 0 (;@7;)
                  local.get 3
                  i32.load offset=12
                  local.get 2
                  i32.ne
                  drop
                end
                local.get 6
                local.get 3
                i32.store offset=8
                local.get 3
                local.get 6
                i32.store offset=12
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 2
                i32.const 20
                i32.add
                local.tee 3
                i32.load
                local.tee 5
                br_if 0 (;@6;)
                local.get 2
                i32.const 16
                i32.add
                local.tee 3
                i32.load
                local.tee 5
                br_if 0 (;@6;)
                i32.const 0
                local.set 6
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 3
                local.set 4
                local.get 5
                local.tee 6
                i32.const 20
                i32.add
                local.tee 3
                i32.load
                local.tee 5
                br_if 0 (;@6;)
                local.get 6
                i32.const 16
                i32.add
                local.set 3
                local.get 6
                i32.load offset=16
                local.tee 5
                br_if 0 (;@6;)
              end
              local.get 4
              i32.const 0
              i32.store
            end
            local.get 7
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.load offset=28
                local.tee 5
                i32.const 2
                i32.shl
                i32.const 1059156
                i32.add
                local.tee 3
                i32.load
                local.get 2
                i32.ne
                br_if 0 (;@6;)
                local.get 3
                local.get 6
                i32.store
                local.get 6
                br_if 1 (;@5;)
                i32.const 0
                i32.const 0
                i32.load offset=1058856
                i32.const -2
                local.get 5
                i32.rotl
                i32.and
                i32.store offset=1058856
                br 2 (;@4;)
              end
              local.get 7
              i32.const 16
              i32.const 20
              local.get 7
              i32.load offset=16
              local.get 2
              i32.eq
              select
              i32.add
              local.get 6
              i32.store
              local.get 6
              i32.eqz
              br_if 1 (;@4;)
            end
            local.get 6
            local.get 7
            i32.store offset=24
            block  ;; label = @5
              local.get 2
              i32.load offset=16
              local.tee 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 6
              local.get 3
              i32.store offset=16
              local.get 3
              local.get 6
              i32.store offset=24
            end
            local.get 2
            i32.load offset=20
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            i32.const 20
            i32.add
            local.get 3
            i32.store
            local.get 3
            local.get 6
            i32.store offset=24
          end
          local.get 0
          local.get 1
          i32.add
          local.get 1
          i32.store
          local.get 0
          local.get 1
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 0
          i32.const 0
          i32.load offset=1058872
          i32.ne
          br_if 1 (;@2;)
          i32.const 0
          local.get 1
          i32.store offset=1058860
          return
        end
        local.get 2
        local.get 3
        i32.const -2
        i32.and
        i32.store offset=4
        local.get 0
        local.get 1
        i32.add
        local.get 1
        i32.store
        local.get 0
        local.get 1
        i32.const 1
        i32.or
        i32.store offset=4
      end
      block  ;; label = @2
        local.get 1
        i32.const 255
        i32.gt_u
        br_if 0 (;@2;)
        local.get 1
        i32.const 3
        i32.shr_u
        local.tee 3
        i32.const 3
        i32.shl
        i32.const 1058892
        i32.add
        local.set 1
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1058852
            local.tee 5
            i32.const 1
            local.get 3
            i32.shl
            local.tee 3
            i32.and
            br_if 0 (;@4;)
            i32.const 0
            local.get 5
            local.get 3
            i32.or
            i32.store offset=1058852
            local.get 1
            local.set 3
            br 1 (;@3;)
          end
          local.get 1
          i32.load offset=8
          local.set 3
        end
        local.get 3
        local.get 0
        i32.store offset=12
        local.get 1
        local.get 0
        i32.store offset=8
        local.get 0
        local.get 1
        i32.store offset=12
        local.get 0
        local.get 3
        i32.store offset=8
        return
      end
      i32.const 0
      local.set 3
      block  ;; label = @2
        local.get 1
        i32.const 8
        i32.shr_u
        local.tee 5
        i32.eqz
        br_if 0 (;@2;)
        i32.const 31
        local.set 3
        local.get 1
        i32.const 16777215
        i32.gt_u
        br_if 0 (;@2;)
        local.get 5
        local.get 5
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee 3
        i32.shl
        local.tee 5
        local.get 5
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.tee 5
        i32.shl
        local.tee 6
        local.get 6
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.tee 6
        i32.shl
        i32.const 15
        i32.shr_u
        local.get 5
        local.get 3
        i32.or
        local.get 6
        i32.or
        i32.sub
        local.tee 3
        i32.const 1
        i32.shl
        local.get 1
        local.get 3
        i32.const 21
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        i32.or
        i32.const 28
        i32.add
        local.set 3
      end
      local.get 0
      i64.const 0
      i64.store offset=16 align=4
      local.get 0
      i32.const 28
      i32.add
      local.get 3
      i32.store
      local.get 3
      i32.const 2
      i32.shl
      i32.const 1059156
      i32.add
      local.set 5
      block  ;; label = @2
        i32.const 0
        i32.load offset=1058856
        local.tee 6
        i32.const 1
        local.get 3
        i32.shl
        local.tee 2
        i32.and
        br_if 0 (;@2;)
        local.get 5
        local.get 0
        i32.store
        i32.const 0
        local.get 6
        local.get 2
        i32.or
        i32.store offset=1058856
        local.get 0
        i32.const 24
        i32.add
        local.get 5
        i32.store
        local.get 0
        local.get 0
        i32.store offset=8
        local.get 0
        local.get 0
        i32.store offset=12
        return
      end
      local.get 1
      i32.const 0
      i32.const 25
      local.get 3
      i32.const 1
      i32.shr_u
      i32.sub
      local.get 3
      i32.const 31
      i32.eq
      select
      i32.shl
      local.set 3
      local.get 5
      i32.load
      local.set 6
      block  ;; label = @2
        loop  ;; label = @3
          local.get 6
          local.tee 5
          i32.load offset=4
          i32.const -8
          i32.and
          local.get 1
          i32.eq
          br_if 1 (;@2;)
          local.get 3
          i32.const 29
          i32.shr_u
          local.set 6
          local.get 3
          i32.const 1
          i32.shl
          local.set 3
          local.get 5
          local.get 6
          i32.const 4
          i32.and
          i32.add
          i32.const 16
          i32.add
          local.tee 2
          i32.load
          local.tee 6
          br_if 0 (;@3;)
        end
        local.get 2
        local.get 0
        i32.store
        local.get 0
        i32.const 24
        i32.add
        local.get 5
        i32.store
        local.get 0
        local.get 0
        i32.store offset=12
        local.get 0
        local.get 0
        i32.store offset=8
        return
      end
      local.get 5
      i32.load offset=8
      local.set 1
      local.get 5
      local.get 0
      i32.store offset=8
      local.get 1
      local.get 0
      i32.store offset=12
      local.get 0
      i32.const 24
      i32.add
      i32.const 0
      i32.store
      local.get 0
      local.get 1
      i32.store offset=8
      local.get 0
      local.get 5
      i32.store offset=12
    end)
  (func $internal_memalign (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 16
        local.get 0
        i32.const 16
        i32.gt_u
        select
        local.tee 2
        local.get 2
        i32.const -1
        i32.add
        i32.and
        br_if 0 (;@2;)
        local.get 2
        local.set 0
        br 1 (;@1;)
      end
      i32.const 32
      local.set 3
      loop  ;; label = @2
        local.get 3
        local.tee 0
        i32.const 1
        i32.shl
        local.set 3
        local.get 0
        local.get 2
        i32.lt_u
        br_if 0 (;@2;)
      end
    end
    block  ;; label = @1
      i32.const -64
      local.get 0
      i32.sub
      local.get 1
      i32.gt_u
      br_if 0 (;@1;)
      i32.const 0
      i32.const 48
      i32.store offset=1059348
      i32.const 0
      return
    end
    block  ;; label = @1
      i32.const 16
      local.get 1
      i32.const 19
      i32.add
      i32.const -16
      i32.and
      local.get 1
      i32.const 11
      i32.lt_u
      select
      local.tee 1
      i32.const 12
      i32.or
      local.get 0
      i32.add
      call $dlmalloc
      local.tee 3
      br_if 0 (;@1;)
      i32.const 0
      return
    end
    local.get 3
    i32.const -8
    i32.add
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const -1
        i32.add
        local.get 3
        i32.and
        br_if 0 (;@2;)
        local.get 2
        local.set 0
        br 1 (;@1;)
      end
      local.get 3
      i32.const -4
      i32.add
      local.tee 4
      i32.load
      local.tee 5
      i32.const -8
      i32.and
      local.get 3
      local.get 0
      i32.add
      i32.const -1
      i32.add
      i32.const 0
      local.get 0
      i32.sub
      i32.and
      i32.const -8
      i32.add
      local.tee 3
      local.get 3
      local.get 0
      i32.add
      local.get 3
      local.get 2
      i32.sub
      i32.const 15
      i32.gt_u
      select
      local.tee 0
      local.get 2
      i32.sub
      local.tee 3
      i32.sub
      local.set 6
      block  ;; label = @2
        local.get 5
        i32.const 3
        i32.and
        br_if 0 (;@2;)
        local.get 0
        local.get 6
        i32.store offset=4
        local.get 0
        local.get 2
        i32.load
        local.get 3
        i32.add
        i32.store
        br 1 (;@1;)
      end
      local.get 0
      local.get 6
      local.get 0
      i32.load offset=4
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store offset=4
      local.get 0
      local.get 6
      i32.add
      local.tee 6
      local.get 6
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get 4
      local.get 3
      local.get 4
      i32.load
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store
      local.get 0
      local.get 0
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get 2
      local.get 3
      call $dispose_chunk
    end
    block  ;; label = @1
      local.get 0
      i32.load offset=4
      local.tee 3
      i32.const 3
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.const -8
      i32.and
      local.tee 2
      local.get 1
      i32.const 16
      i32.add
      i32.le_u
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      local.get 3
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store offset=4
      local.get 0
      local.get 1
      i32.add
      local.tee 3
      local.get 2
      local.get 1
      i32.sub
      local.tee 1
      i32.const 3
      i32.or
      i32.store offset=4
      local.get 0
      local.get 2
      i32.add
      local.tee 2
      local.get 2
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get 3
      local.get 1
      call $dispose_chunk
    end
    local.get 0
    i32.const 8
    i32.add)
  (func $aligned_alloc (type 3) (param i32 i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.const 16
      i32.gt_u
      br_if 0 (;@1;)
      local.get 1
      call $dlmalloc
      return
    end
    local.get 0
    local.get 1
    call $internal_memalign)
  (func $sbrk (type 5) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      memory.size
      i32.const 16
      i32.shl
      return
    end
    block  ;; label = @1
      local.get 0
      i32.const 65535
      i32.and
      br_if 0 (;@1;)
      local.get 0
      i32.const -1
      i32.le_s
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 0
        i32.const 16
        i32.shr_u
        memory.grow
        local.tee 0
        i32.const -1
        i32.ne
        br_if 0 (;@2;)
        i32.const 0
        i32.const 48
        i32.store offset=1059348
        i32.const -1
        return
      end
      local.get 0
      i32.const 16
      i32.shl
      return
    end
    call $abort
    unreachable)
  (func $_Exit (type 1) (param i32)
    local.get 0
    call $__wasi_proc_exit
    unreachable)
  (func $__wasilibc_ensure_environ (type 0)
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059356
      i32.const -1
      i32.ne
      br_if 0 (;@1;)
      call $__wasilibc_initialize_environ
    end)
  (func $__wasilibc_initialize_environ (type 0)
    (local i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 0
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.const 12
          i32.add
          local.get 0
          i32.const 8
          i32.add
          call $__wasi_environ_sizes_get
          br_if 0 (;@3;)
          block  ;; label = @4
            local.get 0
            i32.load offset=12
            local.tee 1
            br_if 0 (;@4;)
            i32.const 0
            i32.const 1059352
            i32.store offset=1059356
            br 3 (;@1;)
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 1
              i32.add
              local.tee 2
              local.get 1
              i32.lt_u
              br_if 0 (;@5;)
              local.get 0
              i32.load offset=8
              call $malloc
              local.tee 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.const 4
              call $calloc
              local.tee 1
              br_if 1 (;@4;)
              local.get 3
              call $free
            end
            i32.const 70
            call $_Exit
            unreachable
          end
          local.get 1
          local.get 3
          call $__wasi_environ_get
          i32.eqz
          br_if 1 (;@2;)
          local.get 3
          call $free
          local.get 1
          call $free
        end
        i32.const 71
        call $_Exit
        unreachable
      end
      i32.const 0
      local.get 1
      i32.store offset=1059356
    end
    local.get 0
    i32.const 16
    i32.add
    global.set 0)
  (func $__wasilibc_initialize_environ_eagerly (type 0)
    call $__wasilibc_initialize_environ)
  (func $abort (type 0)
    unreachable
    unreachable)
  (func $__wasilibc_populate_preopens (type 0)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 0
    global.set 0
    i32.const 3
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        loop  ;; label = @3
          local.get 1
          local.get 0
          i32.const 8
          i32.add
          call $__wasi_fd_prestat_get
          local.tee 2
          i32.const 8
          i32.gt_u
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              br_table 0 (;@5;) 3 (;@2;) 3 (;@2;) 3 (;@2;) 3 (;@2;) 3 (;@2;) 3 (;@2;) 3 (;@2;) 1 (;@4;) 0 (;@5;)
            end
            block  ;; label = @5
              local.get 0
              i32.load8_u offset=8
              br_if 0 (;@5;)
              local.get 0
              i32.load offset=12
              local.tee 2
              i32.const 1
              i32.add
              call $malloc
              local.tee 3
              i32.eqz
              br_if 4 (;@1;)
              local.get 1
              local.get 3
              local.get 2
              call $__wasi_fd_prestat_dir_name
              br_if 3 (;@2;)
              local.get 3
              local.get 0
              i32.load offset=12
              i32.add
              i32.const 0
              i32.store8
              block  ;; label = @6
                i32.const 0
                i32.load offset=1059360
                local.tee 2
                i32.const 0
                i32.load offset=1059368
                i32.ne
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1059364
                local.set 4
                i32.const 8
                local.get 2
                i32.const 1
                i32.shl
                i32.const 4
                local.get 2
                select
                local.tee 5
                call $calloc
                local.tee 6
                i32.eqz
                br_if 5 (;@1;)
                local.get 6
                local.get 4
                local.get 2
                i32.const 3
                i32.shl
                call $memcpy
                local.set 2
                i32.const 0
                local.get 5
                i32.store offset=1059368
                i32.const 0
                local.get 2
                i32.store offset=1059364
                local.get 4
                call $free
                i32.const 0
                i32.load offset=1059360
                local.set 2
              end
              i32.const 0
              local.get 2
              i32.const 1
              i32.add
              i32.store offset=1059360
              i32.const 0
              i32.load offset=1059364
              local.get 2
              i32.const 3
              i32.shl
              i32.add
              local.tee 2
              local.get 1
              i32.store offset=4
              local.get 2
              local.get 3
              i32.store
            end
            local.get 1
            i32.const 1
            i32.add
            local.set 1
            br 1 (;@3;)
          end
        end
        local.get 0
        i32.const 16
        i32.add
        global.set 0
        return
      end
      i32.const 71
      call $_Exit
      unreachable
    end
    i32.const 70
    call $_Exit
    unreachable)
  (func $dummy (type 0))
  (func $__wasm_call_dtors (type 0)
    call $dummy
    call $dummy)
  (func $getenv (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    call $__wasilibc_ensure_environ
    i32.const 0
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.const 61
      call $__strchrnul
      local.tee 2
      local.get 0
      i32.sub
      local.tee 3
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      i32.load8_u
      br_if 0 (;@1;)
      i32.const 0
      i32.load offset=1059356
      local.tee 4
      i32.eqz
      br_if 0 (;@1;)
      local.get 4
      i32.load
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 4
      i32.const 4
      i32.add
      local.set 4
      block  ;; label = @2
        loop  ;; label = @3
          block  ;; label = @4
            local.get 0
            local.get 2
            local.get 3
            call $strncmp
            br_if 0 (;@4;)
            local.get 2
            local.get 3
            i32.add
            local.tee 2
            i32.load8_u
            i32.const 61
            i32.eq
            br_if 2 (;@2;)
          end
          local.get 4
          i32.load
          local.set 2
          local.get 4
          i32.const 4
          i32.add
          local.set 4
          local.get 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 2
      i32.const 1
      i32.add
      local.set 1
    end
    local.get 1)
  (func $memmove (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      local.get 0
      local.get 1
      i32.eq
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 1
        local.get 0
        i32.sub
        local.get 2
        i32.sub
        i32.const 0
        local.get 2
        i32.const 1
        i32.shl
        i32.sub
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        local.get 2
        call $memcpy
        drop
        br 1 (;@1;)
      end
      local.get 1
      local.get 0
      i32.xor
      i32.const 3
      i32.and
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            local.get 1
            i32.ge_u
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 0
              local.set 3
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 0
              i32.const 3
              i32.and
              br_if 0 (;@5;)
              local.get 0
              local.set 3
              br 2 (;@3;)
            end
            local.get 0
            local.set 3
            loop  ;; label = @5
              local.get 2
              i32.eqz
              br_if 4 (;@1;)
              local.get 3
              local.get 1
              i32.load8_u
              i32.store8
              local.get 1
              i32.const 1
              i32.add
              local.set 1
              local.get 2
              i32.const -1
              i32.add
              local.set 2
              local.get 3
              i32.const 1
              i32.add
              local.tee 3
              i32.const 3
              i32.and
              i32.eqz
              br_if 2 (;@3;)
              br 0 (;@5;)
            end
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              local.set 3
              br 1 (;@4;)
            end
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                local.get 2
                i32.add
                i32.const 3
                i32.and
                br_if 0 (;@6;)
                local.get 2
                local.set 3
                br 1 (;@5;)
              end
              local.get 1
              i32.const -1
              i32.add
              local.set 4
              local.get 0
              i32.const -1
              i32.add
              local.set 5
              loop  ;; label = @6
                local.get 2
                i32.eqz
                br_if 5 (;@1;)
                local.get 5
                local.get 2
                i32.add
                local.tee 6
                local.get 4
                local.get 2
                i32.add
                i32.load8_u
                i32.store8
                local.get 2
                i32.const -1
                i32.add
                local.tee 3
                local.set 2
                local.get 6
                i32.const 3
                i32.and
                br_if 0 (;@6;)
              end
            end
            local.get 3
            i32.const 4
            i32.lt_u
            br_if 0 (;@4;)
            local.get 0
            i32.const -4
            i32.add
            local.set 2
            local.get 1
            i32.const -4
            i32.add
            local.set 6
            loop  ;; label = @5
              local.get 2
              local.get 3
              i32.add
              local.get 6
              local.get 3
              i32.add
              i32.load
              i32.store
              local.get 3
              i32.const -4
              i32.add
              local.tee 3
              i32.const 3
              i32.gt_u
              br_if 0 (;@5;)
            end
          end
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 1
          i32.const -1
          i32.add
          local.set 1
          local.get 0
          i32.const -1
          i32.add
          local.set 2
          loop  ;; label = @4
            local.get 2
            local.get 3
            i32.add
            local.get 1
            local.get 3
            i32.add
            i32.load8_u
            i32.store8
            local.get 3
            i32.const -1
            i32.add
            local.tee 3
            br_if 0 (;@4;)
            br 3 (;@1;)
          end
        end
        local.get 2
        i32.const 4
        i32.lt_u
        br_if 0 (;@2;)
        local.get 2
        local.set 6
        loop  ;; label = @3
          local.get 3
          local.get 1
          i32.load
          i32.store
          local.get 1
          i32.const 4
          i32.add
          local.set 1
          local.get 3
          i32.const 4
          i32.add
          local.set 3
          local.get 6
          i32.const -4
          i32.add
          local.tee 6
          i32.const 3
          i32.gt_u
          br_if 0 (;@3;)
        end
        local.get 2
        i32.const 3
        i32.and
        local.set 2
      end
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 1
        i32.add
        local.set 3
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        local.get 2
        i32.const -1
        i32.add
        local.tee 2
        br_if 0 (;@2;)
      end
    end
    local.get 0)
  (func $strlen (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    local.get 0
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.const 3
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          block  ;; label = @4
            local.get 0
            i32.load8_u
            br_if 0 (;@4;)
            local.get 0
            local.get 0
            i32.sub
            return
          end
          local.get 0
          i32.const 1
          i32.add
          local.set 1
          loop  ;; label = @4
            local.get 1
            i32.const 3
            i32.and
            i32.eqz
            br_if 1 (;@3;)
            local.get 1
            i32.load8_u
            local.set 2
            local.get 1
            i32.const 1
            i32.add
            local.tee 3
            local.set 1
            local.get 2
            i32.eqz
            br_if 2 (;@2;)
            br 0 (;@4;)
          end
        end
        local.get 1
        i32.const -4
        i32.add
        local.set 1
        loop  ;; label = @3
          local.get 1
          i32.const 4
          i32.add
          local.tee 1
          i32.load
          local.tee 2
          i32.const -1
          i32.xor
          local.get 2
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          i32.eqz
          br_if 0 (;@3;)
        end
        block  ;; label = @3
          local.get 2
          i32.const 255
          i32.and
          br_if 0 (;@3;)
          local.get 1
          local.get 0
          i32.sub
          return
        end
        loop  ;; label = @3
          local.get 1
          i32.load8_u offset=1
          local.set 2
          local.get 1
          i32.const 1
          i32.add
          local.tee 3
          local.set 1
          local.get 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 3
      i32.const -1
      i32.add
      local.set 3
    end
    local.get 3
    local.get 0
    i32.sub)
  (func $strerror (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    local.set 1
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059396
      local.tee 2
      br_if 0 (;@1;)
      i32.const 1059372
      local.set 2
      i32.const 0
      i32.const 1059372
      i32.store offset=1059396
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          loop  ;; label = @4
            local.get 1
            i32.const 1052848
            i32.add
            i32.load8_u
            local.get 0
            i32.eq
            br_if 1 (;@3;)
            i32.const 77
            local.set 3
            local.get 1
            i32.const 1
            i32.add
            local.tee 1
            i32.const 77
            i32.ne
            br_if 0 (;@4;)
            br 2 (;@2;)
          end
        end
        local.get 1
        local.set 3
        local.get 1
        br_if 0 (;@2;)
        i32.const 1052928
        local.set 4
        br 1 (;@1;)
      end
      i32.const 1052928
      local.set 1
      loop  ;; label = @2
        local.get 1
        i32.load8_u
        local.set 0
        local.get 1
        i32.const 1
        i32.add
        local.tee 4
        local.set 1
        local.get 0
        br_if 0 (;@2;)
        local.get 4
        local.set 1
        local.get 3
        i32.const -1
        i32.add
        local.tee 3
        br_if 0 (;@2;)
      end
    end
    local.get 4
    local.get 2
    i32.load offset=20
    call $__lctrans)
  (func $strerror_r (type 8) (param i32 i32 i32) (result i32)
    (local i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        call $strerror
        local.tee 0
        call $strlen
        local.tee 3
        local.get 2
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 68
        local.set 3
        local.get 2
        i32.eqz
        br_if 1 (;@1;)
        local.get 1
        local.get 0
        local.get 2
        i32.const -1
        i32.add
        local.tee 2
        call $memcpy
        local.get 2
        i32.add
        i32.const 0
        i32.store8
        i32.const 68
        return
      end
      local.get 1
      local.get 0
      local.get 3
      i32.const 1
      i32.add
      call $memcpy
      drop
      i32.const 0
      local.set 3
    end
    local.get 3)
  (func $memcmp (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32)
    i32.const 0
    local.set 3
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        loop  ;; label = @3
          local.get 0
          i32.load8_u
          local.tee 4
          local.get 1
          i32.load8_u
          local.tee 5
          i32.ne
          br_if 1 (;@2;)
          local.get 1
          i32.const 1
          i32.add
          local.set 1
          local.get 0
          i32.const 1
          i32.add
          local.set 0
          local.get 2
          i32.const -1
          i32.add
          local.tee 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 4
      local.get 5
      i32.sub
      local.set 3
    end
    local.get 3)
  (func $__strchrnul (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 1
      i32.const 255
      i32.and
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.const 3
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          loop  ;; label = @4
            local.get 0
            i32.load8_u
            local.tee 3
            i32.eqz
            br_if 2 (;@2;)
            local.get 3
            local.get 1
            i32.const 255
            i32.and
            i32.eq
            br_if 2 (;@2;)
            local.get 0
            i32.const 1
            i32.add
            local.tee 0
            i32.const 3
            i32.and
            br_if 0 (;@4;)
          end
        end
        block  ;; label = @3
          local.get 0
          i32.load
          local.tee 3
          i32.const -1
          i32.xor
          local.get 3
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          br_if 0 (;@3;)
          local.get 2
          i32.const 16843009
          i32.mul
          local.set 2
          loop  ;; label = @4
            local.get 3
            local.get 2
            i32.xor
            local.tee 3
            i32.const -1
            i32.xor
            local.get 3
            i32.const -16843009
            i32.add
            i32.and
            i32.const -2139062144
            i32.and
            br_if 1 (;@3;)
            local.get 0
            i32.load offset=4
            local.set 3
            local.get 0
            i32.const 4
            i32.add
            local.set 0
            local.get 3
            i32.const -1
            i32.xor
            local.get 3
            i32.const -16843009
            i32.add
            i32.and
            i32.const -2139062144
            i32.and
            i32.eqz
            br_if 0 (;@4;)
          end
        end
        local.get 0
        i32.const -1
        i32.add
        local.set 0
        loop  ;; label = @3
          local.get 0
          i32.const 1
          i32.add
          local.tee 0
          i32.load8_u
          local.tee 3
          i32.eqz
          br_if 1 (;@2;)
          local.get 3
          local.get 1
          i32.const 255
          i32.and
          i32.ne
          br_if 0 (;@3;)
        end
      end
      local.get 0
      return
    end
    local.get 0
    local.get 0
    call $strlen
    i32.add)
  (func $memcpy (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.set 3
        loop  ;; label = @3
          local.get 3
          local.get 1
          i32.load8_u
          i32.store8
          local.get 2
          i32.const -1
          i32.add
          local.set 4
          local.get 3
          i32.const 1
          i32.add
          local.set 3
          local.get 1
          i32.const 1
          i32.add
          local.set 1
          local.get 2
          i32.const 1
          i32.eq
          br_if 2 (;@1;)
          local.get 4
          local.set 2
          local.get 1
          i32.const 3
          i32.and
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 2
      local.set 4
      local.get 0
      local.set 3
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 3
        i32.const 3
        i32.and
        local.tee 2
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            local.get 4
            i32.const 16
            i32.ge_u
            br_if 0 (;@4;)
            local.get 4
            local.set 2
            br 1 (;@3;)
          end
          local.get 4
          i32.const -16
          i32.add
          local.set 2
          loop  ;; label = @4
            local.get 3
            local.get 1
            i32.load
            i32.store
            local.get 3
            i32.const 4
            i32.add
            local.get 1
            i32.const 4
            i32.add
            i32.load
            i32.store
            local.get 3
            i32.const 8
            i32.add
            local.get 1
            i32.const 8
            i32.add
            i32.load
            i32.store
            local.get 3
            i32.const 12
            i32.add
            local.get 1
            i32.const 12
            i32.add
            i32.load
            i32.store
            local.get 3
            i32.const 16
            i32.add
            local.set 3
            local.get 1
            i32.const 16
            i32.add
            local.set 1
            local.get 4
            i32.const -16
            i32.add
            local.tee 4
            i32.const 15
            i32.gt_u
            br_if 0 (;@4;)
          end
        end
        block  ;; label = @3
          local.get 2
          i32.const 8
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          local.get 1
          i64.load align=4
          i64.store align=4
          local.get 1
          i32.const 8
          i32.add
          local.set 1
          local.get 3
          i32.const 8
          i32.add
          local.set 3
        end
        block  ;; label = @3
          local.get 2
          i32.const 4
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          local.get 1
          i32.load
          i32.store
          local.get 1
          i32.const 4
          i32.add
          local.set 1
          local.get 3
          i32.const 4
          i32.add
          local.set 3
        end
        block  ;; label = @3
          local.get 2
          i32.const 2
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          local.get 1
          i32.load8_u
          i32.store8
          local.get 3
          local.get 1
          i32.load8_u offset=1
          i32.store8 offset=1
          local.get 3
          i32.const 2
          i32.add
          local.set 3
          local.get 1
          i32.const 2
          i32.add
          local.set 1
        end
        local.get 2
        i32.const 1
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 0
        return
      end
      block  ;; label = @2
        local.get 4
        i32.const 32
        i32.lt_u
        br_if 0 (;@2;)
        local.get 2
        i32.const -1
        i32.add
        local.tee 2
        i32.const 2
        i32.gt_u
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              br_table 0 (;@5;) 1 (;@4;) 2 (;@3;) 0 (;@5;)
            end
            local.get 3
            local.get 1
            i32.load8_u offset=1
            i32.store8 offset=1
            local.get 3
            local.get 1
            i32.load
            local.tee 5
            i32.store8
            local.get 3
            local.get 1
            i32.load8_u offset=2
            i32.store8 offset=2
            local.get 4
            i32.const -3
            i32.add
            local.set 6
            local.get 3
            i32.const 3
            i32.add
            local.set 7
            local.get 4
            i32.const -20
            i32.add
            i32.const -16
            i32.and
            local.set 8
            i32.const 0
            local.set 2
            loop  ;; label = @5
              local.get 7
              local.get 2
              i32.add
              local.tee 3
              local.get 1
              local.get 2
              i32.add
              local.tee 9
              i32.const 4
              i32.add
              i32.load
              local.tee 10
              i32.const 8
              i32.shl
              local.get 5
              i32.const 24
              i32.shr_u
              i32.or
              i32.store
              local.get 3
              i32.const 4
              i32.add
              local.get 9
              i32.const 8
              i32.add
              i32.load
              local.tee 5
              i32.const 8
              i32.shl
              local.get 10
              i32.const 24
              i32.shr_u
              i32.or
              i32.store
              local.get 3
              i32.const 8
              i32.add
              local.get 9
              i32.const 12
              i32.add
              i32.load
              local.tee 10
              i32.const 8
              i32.shl
              local.get 5
              i32.const 24
              i32.shr_u
              i32.or
              i32.store
              local.get 3
              i32.const 12
              i32.add
              local.get 9
              i32.const 16
              i32.add
              i32.load
              local.tee 5
              i32.const 8
              i32.shl
              local.get 10
              i32.const 24
              i32.shr_u
              i32.or
              i32.store
              local.get 2
              i32.const 16
              i32.add
              local.set 2
              local.get 6
              i32.const -16
              i32.add
              local.tee 6
              i32.const 16
              i32.gt_u
              br_if 0 (;@5;)
            end
            local.get 7
            local.get 2
            i32.add
            local.set 3
            local.get 1
            local.get 2
            i32.add
            i32.const 3
            i32.add
            local.set 1
            local.get 4
            local.get 8
            i32.sub
            i32.const -19
            i32.add
            local.set 4
            br 2 (;@2;)
          end
          local.get 3
          local.get 1
          i32.load
          local.tee 5
          i32.store8
          local.get 3
          local.get 1
          i32.load8_u offset=1
          i32.store8 offset=1
          local.get 4
          i32.const -2
          i32.add
          local.set 6
          local.get 3
          i32.const 2
          i32.add
          local.set 7
          local.get 4
          i32.const -20
          i32.add
          i32.const -16
          i32.and
          local.set 8
          i32.const 0
          local.set 2
          loop  ;; label = @4
            local.get 7
            local.get 2
            i32.add
            local.tee 3
            local.get 1
            local.get 2
            i32.add
            local.tee 9
            i32.const 4
            i32.add
            i32.load
            local.tee 10
            i32.const 16
            i32.shl
            local.get 5
            i32.const 16
            i32.shr_u
            i32.or
            i32.store
            local.get 3
            i32.const 4
            i32.add
            local.get 9
            i32.const 8
            i32.add
            i32.load
            local.tee 5
            i32.const 16
            i32.shl
            local.get 10
            i32.const 16
            i32.shr_u
            i32.or
            i32.store
            local.get 3
            i32.const 8
            i32.add
            local.get 9
            i32.const 12
            i32.add
            i32.load
            local.tee 10
            i32.const 16
            i32.shl
            local.get 5
            i32.const 16
            i32.shr_u
            i32.or
            i32.store
            local.get 3
            i32.const 12
            i32.add
            local.get 9
            i32.const 16
            i32.add
            i32.load
            local.tee 5
            i32.const 16
            i32.shl
            local.get 10
            i32.const 16
            i32.shr_u
            i32.or
            i32.store
            local.get 2
            i32.const 16
            i32.add
            local.set 2
            local.get 6
            i32.const -16
            i32.add
            local.tee 6
            i32.const 17
            i32.gt_u
            br_if 0 (;@4;)
          end
          local.get 7
          local.get 2
          i32.add
          local.set 3
          local.get 1
          local.get 2
          i32.add
          i32.const 2
          i32.add
          local.set 1
          local.get 4
          local.get 8
          i32.sub
          i32.const -18
          i32.add
          local.set 4
          br 1 (;@2;)
        end
        local.get 3
        local.get 1
        i32.load
        local.tee 5
        i32.store8
        local.get 4
        i32.const -1
        i32.add
        local.set 6
        local.get 3
        i32.const 1
        i32.add
        local.set 7
        local.get 4
        i32.const -20
        i32.add
        i32.const -16
        i32.and
        local.set 8
        i32.const 0
        local.set 2
        loop  ;; label = @3
          local.get 7
          local.get 2
          i32.add
          local.tee 3
          local.get 1
          local.get 2
          i32.add
          local.tee 9
          i32.const 4
          i32.add
          i32.load
          local.tee 10
          i32.const 24
          i32.shl
          local.get 5
          i32.const 8
          i32.shr_u
          i32.or
          i32.store
          local.get 3
          i32.const 4
          i32.add
          local.get 9
          i32.const 8
          i32.add
          i32.load
          local.tee 5
          i32.const 24
          i32.shl
          local.get 10
          i32.const 8
          i32.shr_u
          i32.or
          i32.store
          local.get 3
          i32.const 8
          i32.add
          local.get 9
          i32.const 12
          i32.add
          i32.load
          local.tee 10
          i32.const 24
          i32.shl
          local.get 5
          i32.const 8
          i32.shr_u
          i32.or
          i32.store
          local.get 3
          i32.const 12
          i32.add
          local.get 9
          i32.const 16
          i32.add
          i32.load
          local.tee 5
          i32.const 24
          i32.shl
          local.get 10
          i32.const 8
          i32.shr_u
          i32.or
          i32.store
          local.get 2
          i32.const 16
          i32.add
          local.set 2
          local.get 6
          i32.const -16
          i32.add
          local.tee 6
          i32.const 18
          i32.gt_u
          br_if 0 (;@3;)
        end
        local.get 7
        local.get 2
        i32.add
        local.set 3
        local.get 1
        local.get 2
        i32.add
        i32.const 1
        i32.add
        local.set 1
        local.get 4
        local.get 8
        i32.sub
        i32.const -17
        i32.add
        local.set 4
      end
      block  ;; label = @2
        local.get 4
        i32.const 16
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 1
        i32.load16_u align=1
        i32.store16 align=1
        local.get 3
        local.get 1
        i32.load8_u offset=2
        i32.store8 offset=2
        local.get 3
        local.get 1
        i32.load8_u offset=3
        i32.store8 offset=3
        local.get 3
        local.get 1
        i32.load8_u offset=4
        i32.store8 offset=4
        local.get 3
        local.get 1
        i32.load8_u offset=5
        i32.store8 offset=5
        local.get 3
        local.get 1
        i32.load8_u offset=6
        i32.store8 offset=6
        local.get 3
        local.get 1
        i32.load8_u offset=7
        i32.store8 offset=7
        local.get 3
        local.get 1
        i32.load8_u offset=8
        i32.store8 offset=8
        local.get 3
        local.get 1
        i32.load8_u offset=9
        i32.store8 offset=9
        local.get 3
        local.get 1
        i32.load8_u offset=10
        i32.store8 offset=10
        local.get 3
        local.get 1
        i32.load8_u offset=11
        i32.store8 offset=11
        local.get 3
        local.get 1
        i32.load8_u offset=12
        i32.store8 offset=12
        local.get 3
        local.get 1
        i32.load8_u offset=13
        i32.store8 offset=13
        local.get 3
        local.get 1
        i32.load8_u offset=14
        i32.store8 offset=14
        local.get 3
        local.get 1
        i32.load8_u offset=15
        i32.store8 offset=15
        local.get 3
        i32.const 16
        i32.add
        local.set 3
        local.get 1
        i32.const 16
        i32.add
        local.set 1
      end
      block  ;; label = @2
        local.get 4
        i32.const 8
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 3
        local.get 1
        i32.load8_u offset=1
        i32.store8 offset=1
        local.get 3
        local.get 1
        i32.load8_u offset=2
        i32.store8 offset=2
        local.get 3
        local.get 1
        i32.load8_u offset=3
        i32.store8 offset=3
        local.get 3
        local.get 1
        i32.load8_u offset=4
        i32.store8 offset=4
        local.get 3
        local.get 1
        i32.load8_u offset=5
        i32.store8 offset=5
        local.get 3
        local.get 1
        i32.load8_u offset=6
        i32.store8 offset=6
        local.get 3
        local.get 1
        i32.load8_u offset=7
        i32.store8 offset=7
        local.get 3
        i32.const 8
        i32.add
        local.set 3
        local.get 1
        i32.const 8
        i32.add
        local.set 1
      end
      block  ;; label = @2
        local.get 4
        i32.const 4
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 3
        local.get 1
        i32.load8_u offset=1
        i32.store8 offset=1
        local.get 3
        local.get 1
        i32.load8_u offset=2
        i32.store8 offset=2
        local.get 3
        local.get 1
        i32.load8_u offset=3
        i32.store8 offset=3
        local.get 3
        i32.const 4
        i32.add
        local.set 3
        local.get 1
        i32.const 4
        i32.add
        local.set 1
      end
      block  ;; label = @2
        local.get 4
        i32.const 2
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 3
        local.get 1
        i32.load8_u offset=1
        i32.store8 offset=1
        local.get 3
        i32.const 2
        i32.add
        local.set 3
        local.get 1
        i32.const 2
        i32.add
        local.set 1
      end
      local.get 4
      i32.const 1
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      local.get 1
      i32.load8_u
      i32.store8
    end
    local.get 0)
  (func $strncmp (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32)
    block  ;; label = @1
      local.get 2
      br_if 0 (;@1;)
      i32.const 0
      return
    end
    i32.const 0
    local.set 3
    block  ;; label = @1
      local.get 0
      i32.load8_u
      local.tee 4
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 1
      i32.add
      local.set 0
      local.get 2
      i32.const -1
      i32.add
      local.set 2
      loop  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.const 255
          i32.and
          local.get 1
          i32.load8_u
          local.tee 5
          i32.eq
          br_if 0 (;@3;)
          local.get 4
          local.set 3
          br 2 (;@1;)
        end
        block  ;; label = @3
          local.get 2
          br_if 0 (;@3;)
          local.get 4
          local.set 3
          br 2 (;@1;)
        end
        block  ;; label = @3
          local.get 5
          br_if 0 (;@3;)
          local.get 4
          local.set 3
          br 2 (;@1;)
        end
        local.get 2
        i32.const -1
        i32.add
        local.set 2
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        local.get 0
        i32.load8_u
        local.set 4
        local.get 0
        i32.const 1
        i32.add
        local.set 0
        local.get 4
        br_if 0 (;@2;)
      end
    end
    local.get 3
    i32.const 255
    i32.and
    local.get 1
    i32.load8_u
    i32.sub)
  (func $memset (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i64)
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      i32.store8
      local.get 2
      local.get 0
      i32.add
      local.tee 3
      i32.const -1
      i32.add
      local.get 1
      i32.store8
      local.get 2
      i32.const 3
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      i32.store8 offset=2
      local.get 0
      local.get 1
      i32.store8 offset=1
      local.get 3
      i32.const -3
      i32.add
      local.get 1
      i32.store8
      local.get 3
      i32.const -2
      i32.add
      local.get 1
      i32.store8
      local.get 2
      i32.const 7
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      i32.store8 offset=3
      local.get 3
      i32.const -4
      i32.add
      local.get 1
      i32.store8
      local.get 2
      i32.const 9
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 0
      local.get 0
      i32.sub
      i32.const 3
      i32.and
      local.tee 4
      i32.add
      local.tee 3
      local.get 1
      i32.const 255
      i32.and
      i32.const 16843009
      i32.mul
      local.tee 1
      i32.store
      local.get 3
      local.get 2
      local.get 4
      i32.sub
      i32.const -4
      i32.and
      local.tee 4
      i32.add
      local.tee 2
      i32.const -4
      i32.add
      local.get 1
      i32.store
      local.get 4
      i32.const 9
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      local.get 1
      i32.store offset=8
      local.get 3
      local.get 1
      i32.store offset=4
      local.get 2
      i32.const -8
      i32.add
      local.get 1
      i32.store
      local.get 2
      i32.const -12
      i32.add
      local.get 1
      i32.store
      local.get 4
      i32.const 25
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      local.get 1
      i32.store offset=24
      local.get 3
      local.get 1
      i32.store offset=20
      local.get 3
      local.get 1
      i32.store offset=16
      local.get 3
      local.get 1
      i32.store offset=12
      local.get 2
      i32.const -16
      i32.add
      local.get 1
      i32.store
      local.get 2
      i32.const -20
      i32.add
      local.get 1
      i32.store
      local.get 2
      i32.const -24
      i32.add
      local.get 1
      i32.store
      local.get 2
      i32.const -28
      i32.add
      local.get 1
      i32.store
      local.get 4
      local.get 3
      i32.const 4
      i32.and
      i32.const 24
      i32.or
      local.tee 5
      i32.sub
      local.tee 2
      i32.const 32
      i32.lt_u
      br_if 0 (;@1;)
      local.get 1
      i64.extend_i32_u
      local.tee 6
      i64.const 32
      i64.shl
      local.get 6
      i64.or
      local.set 6
      local.get 3
      local.get 5
      i32.add
      local.set 1
      loop  ;; label = @2
        local.get 1
        local.get 6
        i64.store
        local.get 1
        i32.const 24
        i32.add
        local.get 6
        i64.store
        local.get 1
        i32.const 16
        i32.add
        local.get 6
        i64.store
        local.get 1
        i32.const 8
        i32.add
        local.get 6
        i64.store
        local.get 1
        i32.const 32
        i32.add
        local.set 1
        local.get 2
        i32.const -32
        i32.add
        local.tee 2
        i32.const 31
        i32.gt_u
        br_if 0 (;@2;)
      end
    end
    local.get 0)
  (func $dummy.1 (type 3) (param i32 i32) (result i32)
    local.get 0)
  (func $__lctrans (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $dummy.1)
  (func $_ZN5alloc5alloc18handle_alloc_error17hda603f03a0f21835E (type 6) (param i32 i32)
    local.get 0
    local.get 1
    call $rust_oom
    unreachable)
  (func $_ZN5alloc7raw_vec17capacity_overflow17h4cfcf68919b93e61E (type 0)
    i32.const 1054530
    i32.const 17
    i32.const 1054548
    call $_ZN4core9panicking5panic17heeaec3885c636092E
    unreachable)
  (func $_ZN5alloc6string13FromUtf8Error10into_bytes17heafb6e86be24df0cE (type 6) (param i32 i32)
    local.get 0
    local.get 1
    i64.load align=4
    i64.store align=4
    local.get 0
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i32.load
    i32.store)
  (func $_ZN5alloc6string104_$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..vec..Vec$LT$u8$GT$$GT$4from17h4cb8310fd36b5288E (type 6) (param i32 i32)
    local.get 0
    local.get 1
    i64.load align=4
    i64.store align=4
    local.get 0
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i32.load
    i32.store)
  (func $_ZN5alloc3vec12Vec$LT$T$GT$5drain17end_assert_failed17hc6f56e66601fd97cE (type 6) (param i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 1
    i32.store offset=4
    local.get 2
    local.get 0
    i32.store
    local.get 2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get 2
    i32.const 44
    i32.add
    i32.const 17
    i32.store
    local.get 2
    i64.const 3
    i64.store offset=12 align=4
    local.get 2
    i32.const 1054632
    i32.store offset=8
    local.get 2
    i32.const 17
    i32.store offset=36
    local.get 2
    local.get 2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get 2
    local.get 2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get 2
    local.get 2
    i32.store offset=32
    local.get 2
    i32.const 8
    i32.add
    i32.const 1054656
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17hc4bf69ce3fbd1528E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    drop
    loop (result i32)  ;; label = @1
      br 0 (;@1;)
    end)
  (func $_ZN4core3ptr13drop_in_place17h021c8bbd1b3ab19eE (type 1) (param i32))
  (func $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 17
    i32.store
    local.get 3
    i64.const 2
    i64.store offset=12 align=4
    local.get 3
    i32.const 1054876
    i32.store offset=8
    local.get 3
    i32.const 17
    i32.store offset=36
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get 3
    local.get 3
    i32.store offset=40
    local.get 3
    local.get 3
    i32.const 4
    i32.add
    i32.store offset=32
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core9panicking5panic17heeaec3885c636092E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get 3
    i32.const 1054672
    i32.store offset=16
    local.get 3
    i64.const 1
    i64.store offset=4 align=4
    local.get 3
    local.get 1
    i32.store offset=28
    local.get 3
    local.get 0
    i32.store offset=24
    local.get 3
    local.get 3
    i32.const 24
    i32.add
    i32.store
    local.get 3
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 17
    i32.store
    local.get 3
    i64.const 2
    i64.store offset=12 align=4
    local.get 3
    i32.const 1055548
    i32.store offset=8
    local.get 3
    i32.const 17
    i32.store offset=36
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get 3
    local.get 3
    i32.const 4
    i32.add
    i32.store offset=40
    local.get 3
    local.get 3
    i32.store offset=32
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core3fmt9Formatter3pad17h637274232327cb71E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    local.get 0
    i32.load offset=16
    local.set 3
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.load offset=8
            local.tee 4
            i32.const 1
            i32.eq
            br_if 0 (;@4;)
            local.get 3
            i32.const 1
            i32.eq
            br_if 1 (;@3;)
            local.get 0
            i32.load offset=24
            local.get 1
            local.get 2
            local.get 0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect (type 8)
            local.set 3
            br 3 (;@1;)
          end
          local.get 3
          i32.const 1
          i32.ne
          br_if 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            br_if 0 (;@4;)
            i32.const 0
            local.set 2
            br 1 (;@3;)
          end
          local.get 1
          local.get 2
          i32.add
          local.set 5
          local.get 0
          i32.const 20
          i32.add
          i32.load
          i32.const 1
          i32.add
          local.set 6
          i32.const 0
          local.set 7
          local.get 1
          local.set 3
          local.get 1
          local.set 8
          loop  ;; label = @4
            local.get 3
            i32.const 1
            i32.add
            local.set 9
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  i32.load8_s
                  local.tee 10
                  i32.const -1
                  i32.gt_s
                  br_if 0 (;@7;)
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 9
                      local.get 5
                      i32.ne
                      br_if 0 (;@9;)
                      i32.const 0
                      local.set 11
                      local.get 5
                      local.set 3
                      br 1 (;@8;)
                    end
                    local.get 3
                    i32.load8_u offset=1
                    i32.const 63
                    i32.and
                    local.set 11
                    local.get 3
                    i32.const 2
                    i32.add
                    local.tee 9
                    local.set 3
                  end
                  local.get 10
                  i32.const 31
                  i32.and
                  local.set 12
                  block  ;; label = @8
                    local.get 10
                    i32.const 255
                    i32.and
                    local.tee 10
                    i32.const 223
                    i32.gt_u
                    br_if 0 (;@8;)
                    local.get 11
                    local.get 12
                    i32.const 6
                    i32.shl
                    i32.or
                    local.set 10
                    br 2 (;@6;)
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 3
                      local.get 5
                      i32.ne
                      br_if 0 (;@9;)
                      i32.const 0
                      local.set 13
                      local.get 5
                      local.set 14
                      br 1 (;@8;)
                    end
                    local.get 3
                    i32.load8_u
                    i32.const 63
                    i32.and
                    local.set 13
                    local.get 3
                    i32.const 1
                    i32.add
                    local.tee 9
                    local.set 14
                  end
                  local.get 13
                  local.get 11
                  i32.const 6
                  i32.shl
                  i32.or
                  local.set 11
                  block  ;; label = @8
                    local.get 10
                    i32.const 240
                    i32.ge_u
                    br_if 0 (;@8;)
                    local.get 11
                    local.get 12
                    i32.const 12
                    i32.shl
                    i32.or
                    local.set 10
                    br 2 (;@6;)
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 14
                      local.get 5
                      i32.ne
                      br_if 0 (;@9;)
                      i32.const 0
                      local.set 10
                      local.get 9
                      local.set 3
                      br 1 (;@8;)
                    end
                    local.get 14
                    i32.const 1
                    i32.add
                    local.set 3
                    local.get 14
                    i32.load8_u
                    i32.const 63
                    i32.and
                    local.set 10
                  end
                  local.get 11
                  i32.const 6
                  i32.shl
                  local.get 12
                  i32.const 18
                  i32.shl
                  i32.const 1835008
                  i32.and
                  i32.or
                  local.get 10
                  i32.or
                  local.tee 10
                  i32.const 1114112
                  i32.ne
                  br_if 2 (;@5;)
                  br 4 (;@3;)
                end
                local.get 10
                i32.const 255
                i32.and
                local.set 10
              end
              local.get 9
              local.set 3
            end
            block  ;; label = @5
              local.get 6
              i32.const -1
              i32.add
              local.tee 6
              i32.eqz
              br_if 0 (;@5;)
              local.get 7
              local.get 8
              i32.sub
              local.get 3
              i32.add
              local.set 7
              local.get 3
              local.set 8
              local.get 5
              local.get 3
              i32.ne
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
          end
          local.get 10
          i32.const 1114112
          i32.eq
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 7
              i32.eqz
              br_if 0 (;@5;)
              local.get 7
              local.get 2
              i32.eq
              br_if 0 (;@5;)
              i32.const 0
              local.set 3
              local.get 7
              local.get 2
              i32.ge_u
              br_if 1 (;@4;)
              local.get 1
              local.get 7
              i32.add
              i32.load8_s
              i32.const -64
              i32.lt_s
              br_if 1 (;@4;)
            end
            local.get 1
            local.set 3
          end
          local.get 7
          local.get 2
          local.get 3
          select
          local.set 2
          local.get 3
          local.get 1
          local.get 3
          select
          local.set 1
        end
        local.get 4
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        i32.load offset=24
        local.get 1
        local.get 2
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        return
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 9
            local.get 2
            local.set 10
            local.get 1
            local.set 3
            loop  ;; label = @5
              local.get 9
              local.get 3
              i32.load8_u
              i32.const 192
              i32.and
              i32.const 128
              i32.eq
              i32.add
              local.set 9
              local.get 3
              i32.const 1
              i32.add
              local.set 3
              local.get 10
              i32.const -1
              i32.add
              local.tee 10
              br_if 0 (;@5;)
            end
            local.get 2
            local.get 9
            i32.sub
            local.get 0
            i32.load offset=12
            local.tee 6
            i32.ge_u
            br_if 1 (;@3;)
            i32.const 0
            local.set 9
            local.get 2
            local.set 10
            local.get 1
            local.set 3
            loop  ;; label = @5
              local.get 9
              local.get 3
              i32.load8_u
              i32.const 192
              i32.and
              i32.const 128
              i32.eq
              i32.add
              local.set 9
              local.get 3
              i32.const 1
              i32.add
              local.set 3
              local.get 10
              i32.const -1
              i32.add
              local.tee 10
              br_if 0 (;@5;)
              br 3 (;@2;)
            end
          end
          i32.const 0
          local.set 9
          local.get 0
          i32.load offset=12
          local.tee 6
          br_if 1 (;@2;)
        end
        local.get 0
        i32.load offset=24
        local.get 1
        local.get 2
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        return
      end
      i32.const 0
      local.set 3
      local.get 9
      local.get 2
      i32.sub
      local.get 6
      i32.add
      local.tee 6
      local.set 10
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            local.get 0
            i32.load8_u offset=32
            local.tee 9
            local.get 9
            i32.const 3
            i32.eq
            select
            i32.const 3
            i32.and
            br_table 2 (;@2;) 1 (;@3;) 0 (;@4;) 1 (;@3;) 2 (;@2;)
          end
          local.get 6
          i32.const 1
          i32.shr_u
          local.set 3
          local.get 6
          i32.const 1
          i32.add
          i32.const 1
          i32.shr_u
          local.set 10
          br 1 (;@2;)
        end
        i32.const 0
        local.set 10
        local.get 6
        local.set 3
      end
      local.get 3
      i32.const 1
      i32.add
      local.set 3
      block  ;; label = @2
        loop  ;; label = @3
          local.get 3
          i32.const -1
          i32.add
          local.tee 3
          i32.eqz
          br_if 1 (;@2;)
          local.get 0
          i32.load offset=24
          local.get 0
          i32.load offset=4
          local.get 0
          i32.load offset=28
          i32.load offset=16
          call_indirect (type 3)
          i32.eqz
          br_if 0 (;@3;)
        end
        i32.const 1
        return
      end
      local.get 0
      i32.load offset=4
      local.set 9
      i32.const 1
      local.set 3
      local.get 0
      i32.load offset=24
      local.get 1
      local.get 2
      local.get 0
      i32.load offset=28
      i32.load offset=12
      call_indirect (type 8)
      br_if 0 (;@1;)
      local.get 10
      i32.const 1
      i32.add
      local.set 3
      local.get 0
      i32.load offset=28
      local.set 10
      local.get 0
      i32.load offset=24
      local.set 0
      loop  ;; label = @2
        block  ;; label = @3
          local.get 3
          i32.const -1
          i32.add
          local.tee 3
          br_if 0 (;@3;)
          i32.const 0
          return
        end
        local.get 0
        local.get 9
        local.get 10
        i32.load offset=16
        call_indirect (type 3)
        i32.eqz
        br_if 0 (;@2;)
      end
      i32.const 1
      return
    end
    local.get 3)
  (func $_ZN4core3str16slice_error_fail17h457c150a5a135c79E (type 11) (param i32 i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 112
    i32.sub
    local.tee 5
    global.set 0
    local.get 5
    local.get 3
    i32.store offset=12
    local.get 5
    local.get 2
    i32.store offset=8
    i32.const 1
    local.set 6
    local.get 1
    local.set 7
    block  ;; label = @1
      local.get 1
      i32.const 257
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 0
      local.get 1
      i32.sub
      local.set 8
      i32.const 256
      local.set 9
      loop  ;; label = @2
        block  ;; label = @3
          local.get 9
          local.get 1
          i32.ge_u
          br_if 0 (;@3;)
          i32.const 0
          local.set 6
          local.get 0
          local.get 9
          i32.add
          i32.load8_s
          i32.const -65
          i32.le_s
          br_if 0 (;@3;)
          local.get 9
          local.set 7
          br 2 (;@1;)
        end
        local.get 9
        i32.const -1
        i32.add
        local.set 7
        i32.const 0
        local.set 6
        local.get 9
        i32.const 1
        i32.eq
        br_if 1 (;@1;)
        local.get 8
        local.get 9
        i32.add
        local.set 10
        local.get 7
        local.set 9
        local.get 10
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
      end
    end
    local.get 5
    local.get 7
    i32.store offset=20
    local.get 5
    local.get 0
    i32.store offset=16
    local.get 5
    i32.const 0
    i32.const 5
    local.get 6
    select
    i32.store offset=28
    local.get 5
    i32.const 1054672
    i32.const 1056108
    local.get 6
    select
    i32.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            local.get 1
            i32.gt_u
            local.tee 6
            br_if 0 (;@4;)
            local.get 3
            local.get 1
            i32.gt_u
            br_if 0 (;@4;)
            local.get 2
            local.get 3
            i32.gt_u
            br_if 1 (;@3;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.eqz
                br_if 0 (;@6;)
                local.get 1
                local.get 2
                i32.eq
                br_if 0 (;@6;)
                local.get 1
                local.get 2
                i32.le_u
                br_if 1 (;@5;)
                local.get 0
                local.get 2
                i32.add
                i32.load8_s
                i32.const -64
                i32.lt_s
                br_if 1 (;@5;)
              end
              local.get 3
              local.set 2
            end
            local.get 5
            local.get 2
            i32.store offset=32
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.eqz
                br_if 0 (;@6;)
                local.get 2
                local.get 1
                i32.ne
                br_if 1 (;@5;)
              end
              local.get 2
              local.set 6
              br 3 (;@2;)
            end
            local.get 1
            i32.const 1
            i32.add
            local.set 9
            loop  ;; label = @5
              block  ;; label = @6
                local.get 2
                local.get 1
                i32.ge_u
                br_if 0 (;@6;)
                local.get 0
                local.get 2
                i32.add
                i32.load8_s
                i32.const -64
                i32.lt_s
                br_if 0 (;@6;)
                local.get 5
                i32.const 36
                i32.add
                local.set 9
                local.get 2
                local.set 6
                br 5 (;@1;)
              end
              local.get 2
              i32.const -1
              i32.add
              local.set 6
              local.get 2
              i32.const 1
              i32.eq
              br_if 3 (;@2;)
              local.get 9
              local.get 2
              i32.eq
              local.set 3
              local.get 6
              local.set 2
              local.get 3
              br_if 3 (;@2;)
              br 0 (;@5;)
            end
          end
          local.get 5
          local.get 2
          local.get 3
          local.get 6
          select
          i32.store offset=40
          local.get 5
          i32.const 48
          i32.add
          i32.const 20
          i32.add
          i32.const 3
          i32.store
          local.get 5
          i32.const 72
          i32.add
          i32.const 20
          i32.add
          i32.const 81
          i32.store
          local.get 5
          i32.const 84
          i32.add
          i32.const 81
          i32.store
          local.get 5
          i64.const 3
          i64.store offset=52 align=4
          local.get 5
          i32.const 1056148
          i32.store offset=48
          local.get 5
          i32.const 17
          i32.store offset=76
          local.get 5
          local.get 5
          i32.const 72
          i32.add
          i32.store offset=64
          local.get 5
          local.get 5
          i32.const 24
          i32.add
          i32.store offset=88
          local.get 5
          local.get 5
          i32.const 16
          i32.add
          i32.store offset=80
          local.get 5
          local.get 5
          i32.const 40
          i32.add
          i32.store offset=72
          local.get 5
          i32.const 48
          i32.add
          local.get 4
          call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
          unreachable
        end
        local.get 5
        i32.const 100
        i32.add
        i32.const 81
        i32.store
        local.get 5
        i32.const 72
        i32.add
        i32.const 20
        i32.add
        i32.const 81
        i32.store
        local.get 5
        i32.const 84
        i32.add
        i32.const 17
        i32.store
        local.get 5
        i32.const 48
        i32.add
        i32.const 20
        i32.add
        i32.const 4
        i32.store
        local.get 5
        i64.const 4
        i64.store offset=52 align=4
        local.get 5
        i32.const 1056208
        i32.store offset=48
        local.get 5
        i32.const 17
        i32.store offset=76
        local.get 5
        local.get 5
        i32.const 72
        i32.add
        i32.store offset=64
        local.get 5
        local.get 5
        i32.const 24
        i32.add
        i32.store offset=96
        local.get 5
        local.get 5
        i32.const 16
        i32.add
        i32.store offset=88
        local.get 5
        local.get 5
        i32.const 12
        i32.add
        i32.store offset=80
        local.get 5
        local.get 5
        i32.const 8
        i32.add
        i32.store offset=72
        local.get 5
        i32.const 48
        i32.add
        local.get 4
        call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
        unreachable
      end
      local.get 5
      i32.const 36
      i32.add
      local.set 9
    end
    block  ;; label = @1
      local.get 6
      local.get 1
      i32.eq
      br_if 0 (;@1;)
      i32.const 1
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              local.get 6
              i32.add
              local.tee 7
              i32.load8_s
              local.tee 2
              i32.const -1
              i32.gt_s
              br_if 0 (;@5;)
              i32.const 0
              local.set 3
              local.get 0
              local.get 1
              i32.add
              local.tee 1
              local.set 0
              block  ;; label = @6
                local.get 7
                i32.const 1
                i32.add
                local.get 1
                i32.eq
                br_if 0 (;@6;)
                local.get 7
                i32.const 2
                i32.add
                local.set 0
                local.get 7
                i32.load8_u offset=1
                i32.const 63
                i32.and
                local.set 3
              end
              local.get 2
              i32.const 31
              i32.and
              local.set 7
              local.get 2
              i32.const 255
              i32.and
              i32.const 223
              i32.gt_u
              br_if 1 (;@4;)
              local.get 3
              local.get 7
              i32.const 6
              i32.shl
              i32.or
              local.set 2
              br 2 (;@3;)
            end
            local.get 5
            local.get 2
            i32.const 255
            i32.and
            i32.store offset=36
            local.get 5
            i32.const 40
            i32.add
            local.set 1
            br 2 (;@2;)
          end
          i32.const 0
          local.set 8
          local.get 1
          local.set 10
          block  ;; label = @4
            local.get 0
            local.get 1
            i32.eq
            br_if 0 (;@4;)
            local.get 0
            i32.const 1
            i32.add
            local.set 10
            local.get 0
            i32.load8_u
            i32.const 63
            i32.and
            local.set 8
          end
          local.get 8
          local.get 3
          i32.const 6
          i32.shl
          i32.or
          local.set 3
          block  ;; label = @4
            local.get 2
            i32.const 255
            i32.and
            i32.const 240
            i32.ge_u
            br_if 0 (;@4;)
            local.get 3
            local.get 7
            i32.const 12
            i32.shl
            i32.or
            local.set 2
            br 1 (;@3;)
          end
          i32.const 0
          local.set 2
          block  ;; label = @4
            local.get 10
            local.get 1
            i32.eq
            br_if 0 (;@4;)
            local.get 10
            i32.load8_u
            i32.const 63
            i32.and
            local.set 2
          end
          local.get 3
          i32.const 6
          i32.shl
          local.get 7
          i32.const 18
          i32.shl
          i32.const 1835008
          i32.and
          i32.or
          local.get 2
          i32.or
          local.tee 2
          i32.const 1114112
          i32.eq
          br_if 2 (;@1;)
        end
        local.get 5
        local.get 2
        i32.store offset=36
        i32.const 1
        local.set 3
        local.get 5
        i32.const 40
        i32.add
        local.set 1
        local.get 2
        i32.const 128
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 2
        local.set 3
        local.get 2
        i32.const 2048
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 3
        i32.const 4
        local.get 2
        i32.const 65536
        i32.lt_u
        select
        local.set 3
      end
      local.get 5
      local.get 6
      i32.store offset=40
      local.get 5
      local.get 3
      local.get 6
      i32.add
      i32.store offset=44
      local.get 5
      i32.const 48
      i32.add
      i32.const 20
      i32.add
      i32.const 5
      i32.store
      local.get 5
      i32.const 108
      i32.add
      i32.const 81
      i32.store
      local.get 5
      i32.const 100
      i32.add
      i32.const 81
      i32.store
      local.get 5
      i32.const 72
      i32.add
      i32.const 20
      i32.add
      i32.const 82
      i32.store
      local.get 5
      i32.const 84
      i32.add
      i32.const 83
      i32.store
      local.get 5
      i64.const 5
      i64.store offset=52 align=4
      local.get 5
      i32.const 1056292
      i32.store offset=48
      local.get 5
      local.get 1
      i32.store offset=88
      local.get 5
      local.get 9
      i32.store offset=80
      local.get 5
      i32.const 17
      i32.store offset=76
      local.get 5
      local.get 5
      i32.const 72
      i32.add
      i32.store offset=64
      local.get 5
      local.get 5
      i32.const 24
      i32.add
      i32.store offset=104
      local.get 5
      local.get 5
      i32.const 16
      i32.add
      i32.store offset=96
      local.get 5
      local.get 5
      i32.const 32
      i32.add
      i32.store offset=72
      local.get 5
      i32.const 48
      i32.add
      local.get 4
      call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
      unreachable
    end
    i32.const 1054709
    i32.const 43
    local.get 4
    call $_ZN4core9panicking5panic17heeaec3885c636092E
    unreachable)
  (func $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE (type 6) (param i32 i32)
    (local i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 1
    i32.store offset=12
    local.get 2
    local.get 0
    i32.store offset=8
    local.get 2
    i32.const 1054780
    i32.store offset=4
    local.get 2
    i32.const 1054672
    i32.store
    local.get 2
    call $rust_begin_unwind
    unreachable)
  (func $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 17
    i32.store
    local.get 3
    i64.const 2
    i64.store offset=12 align=4
    local.get 3
    i32.const 1055600
    i32.store offset=8
    local.get 3
    i32.const 17
    i32.store offset=36
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get 3
    local.get 3
    i32.const 4
    i32.add
    i32.store offset=40
    local.get 3
    local.get 3
    i32.store offset=32
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 17
    i32.store
    local.get 3
    i64.const 2
    i64.store offset=12 align=4
    local.get 3
    i32.const 1055516
    i32.store offset=8
    local.get 3
    i32.const 17
    i32.store offset=36
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get 3
    local.get 3
    i32.const 4
    i32.add
    i32.store offset=40
    local.get 3
    local.get 3
    i32.store offset=32
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17hc79ab98580260a0dE (type 3) (param i32 i32) (result i32)
    local.get 0
    i64.load32_u
    i32.const 1
    local.get 1
    call $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE)
  (func $_ZN4core3fmt5write17h1626e57fa473d161E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    i32.const 36
    i32.add
    local.get 1
    i32.store
    local.get 3
    i32.const 3
    i32.store8 offset=40
    local.get 3
    i64.const 137438953472
    i64.store offset=8
    local.get 3
    local.get 0
    i32.store offset=32
    local.get 3
    i32.const 0
    i32.store offset=24
    local.get 3
    i32.const 0
    i32.store offset=16
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              i32.load offset=8
              local.tee 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.set 5
              local.get 2
              i32.load offset=4
              local.tee 6
              local.get 2
              i32.const 12
              i32.add
              i32.load
              local.tee 7
              local.get 7
              local.get 6
              i32.gt_u
              select
              local.tee 8
              i32.eqz
              br_if 1 (;@4;)
              local.get 0
              local.get 5
              i32.load
              local.get 5
              i32.load offset=4
              local.get 1
              i32.load offset=12
              call_indirect (type 8)
              br_if 3 (;@2;)
              local.get 5
              i32.const 12
              i32.add
              local.set 0
              local.get 2
              i32.load offset=20
              local.set 9
              local.get 2
              i32.load offset=16
              local.set 10
              local.get 8
              local.set 11
              loop  ;; label = @6
                local.get 3
                local.get 4
                i32.const 28
                i32.add
                i32.load8_u
                i32.store8 offset=40
                local.get 3
                local.get 4
                i32.const 4
                i32.add
                i64.load align=4
                i64.const 32
                i64.rotl
                i64.store offset=8
                local.get 4
                i32.const 24
                i32.add
                i32.load
                local.set 2
                i32.const 0
                local.set 7
                i32.const 0
                local.set 1
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 4
                      i32.const 20
                      i32.add
                      i32.load
                      br_table 1 (;@8;) 0 (;@9;) 2 (;@7;) 1 (;@8;)
                    end
                    block  ;; label = @9
                      local.get 2
                      local.get 9
                      i32.lt_u
                      br_if 0 (;@9;)
                      local.get 2
                      local.get 9
                      i32.const 1055336
                      call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
                      unreachable
                    end
                    local.get 2
                    i32.const 3
                    i32.shl
                    local.set 12
                    i32.const 0
                    local.set 1
                    local.get 10
                    local.get 12
                    i32.add
                    local.tee 12
                    i32.load offset=4
                    i32.const 84
                    i32.ne
                    br_if 1 (;@7;)
                    local.get 12
                    i32.load
                    i32.load
                    local.set 2
                  end
                  i32.const 1
                  local.set 1
                end
                local.get 3
                local.get 2
                i32.store offset=20
                local.get 3
                local.get 1
                i32.store offset=16
                local.get 4
                i32.const 16
                i32.add
                i32.load
                local.set 2
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 4
                      i32.const 12
                      i32.add
                      i32.load
                      br_table 1 (;@8;) 0 (;@9;) 2 (;@7;) 1 (;@8;)
                    end
                    block  ;; label = @9
                      local.get 2
                      local.get 9
                      i32.lt_u
                      br_if 0 (;@9;)
                      local.get 2
                      local.get 9
                      i32.const 1055336
                      call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
                      unreachable
                    end
                    local.get 2
                    i32.const 3
                    i32.shl
                    local.set 1
                    local.get 10
                    local.get 1
                    i32.add
                    local.tee 1
                    i32.load offset=4
                    i32.const 84
                    i32.ne
                    br_if 1 (;@7;)
                    local.get 1
                    i32.load
                    i32.load
                    local.set 2
                  end
                  i32.const 1
                  local.set 7
                end
                local.get 3
                local.get 2
                i32.store offset=28
                local.get 3
                local.get 7
                i32.store offset=24
                block  ;; label = @7
                  local.get 4
                  i32.load
                  local.tee 2
                  local.get 9
                  i32.ge_u
                  br_if 0 (;@7;)
                  local.get 10
                  local.get 2
                  i32.const 3
                  i32.shl
                  i32.add
                  local.tee 2
                  i32.load
                  local.get 3
                  i32.const 8
                  i32.add
                  local.get 2
                  i32.load offset=4
                  call_indirect (type 3)
                  br_if 5 (;@2;)
                  local.get 11
                  i32.const -1
                  i32.add
                  local.tee 11
                  i32.eqz
                  br_if 4 (;@3;)
                  local.get 4
                  i32.const 32
                  i32.add
                  local.set 4
                  local.get 0
                  i32.const -4
                  i32.add
                  local.set 2
                  local.get 0
                  i32.load
                  local.set 1
                  local.get 0
                  i32.const 8
                  i32.add
                  local.set 0
                  local.get 3
                  i32.load offset=32
                  local.get 2
                  i32.load
                  local.get 1
                  local.get 3
                  i32.load offset=36
                  i32.load offset=12
                  call_indirect (type 8)
                  i32.eqz
                  br_if 1 (;@6;)
                  br 5 (;@2;)
                end
              end
              local.get 2
              local.get 9
              i32.const 1055320
              call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
              unreachable
            end
            local.get 2
            i32.load
            local.set 5
            local.get 2
            i32.load offset=4
            local.tee 6
            local.get 2
            i32.const 20
            i32.add
            i32.load
            local.tee 4
            local.get 4
            local.get 6
            i32.gt_u
            select
            local.tee 8
            i32.eqz
            br_if 0 (;@4;)
            local.get 2
            i32.load offset=16
            local.set 4
            local.get 0
            local.get 5
            i32.load
            local.get 5
            i32.load offset=4
            local.get 1
            i32.load offset=12
            call_indirect (type 8)
            br_if 2 (;@2;)
            local.get 5
            i32.const 12
            i32.add
            local.set 0
            local.get 8
            local.set 2
            loop  ;; label = @5
              local.get 4
              i32.load
              local.get 3
              i32.const 8
              i32.add
              local.get 4
              i32.const 4
              i32.add
              i32.load
              call_indirect (type 3)
              br_if 3 (;@2;)
              local.get 2
              i32.const -1
              i32.add
              local.tee 2
              i32.eqz
              br_if 2 (;@3;)
              local.get 4
              i32.const 8
              i32.add
              local.set 4
              local.get 0
              i32.const -4
              i32.add
              local.set 1
              local.get 0
              i32.load
              local.set 7
              local.get 0
              i32.const 8
              i32.add
              local.set 0
              local.get 3
              i32.load offset=32
              local.get 1
              i32.load
              local.get 7
              local.get 3
              i32.load offset=36
              i32.load offset=12
              call_indirect (type 8)
              i32.eqz
              br_if 0 (;@5;)
              br 3 (;@2;)
            end
          end
          i32.const 0
          local.set 8
        end
        block  ;; label = @3
          local.get 6
          local.get 8
          i32.le_u
          br_if 0 (;@3;)
          local.get 3
          i32.load offset=32
          local.get 5
          local.get 8
          i32.const 3
          i32.shl
          i32.add
          local.tee 4
          i32.load
          local.get 4
          i32.load offset=4
          local.get 3
          i32.load offset=36
          i32.load offset=12
          call_indirect (type 8)
          br_if 1 (;@2;)
        end
        i32.const 0
        local.set 4
        br 1 (;@1;)
      end
      i32.const 1
      local.set 4
    end
    local.get 3
    i32.const 48
    i32.add
    global.set 0
    local.get 4)
  (func $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17hc2205f114ba00674E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        local.get 1
        call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h9761667c77b91f77E
        br_if 0 (;@2;)
        local.get 1
        i32.const 28
        i32.add
        i32.load
        local.set 3
        local.get 1
        i32.load offset=24
        local.set 4
        local.get 2
        i32.const 28
        i32.add
        i32.const 0
        i32.store
        local.get 2
        i32.const 1054672
        i32.store offset=24
        local.get 2
        i64.const 1
        i64.store offset=12 align=4
        local.get 2
        i32.const 1054676
        i32.store offset=8
        local.get 4
        local.get 3
        local.get 2
        i32.const 8
        i32.add
        call $_ZN4core3fmt5write17h1626e57fa473d161E
        i32.eqz
        br_if 1 (;@1;)
      end
      local.get 2
      i32.const 32
      i32.add
      global.set 0
      i32.const 1
      return
    end
    local.get 0
    i32.const 4
    i32.add
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h9761667c77b91f77E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h9761667c77b91f77E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load
              local.tee 3
              i32.const 16
              i32.and
              br_if 0 (;@5;)
              local.get 0
              i32.load
              local.set 4
              local.get 3
              i32.const 32
              i32.and
              br_if 1 (;@4;)
              local.get 4
              i64.extend_i32_u
              i32.const 1
              local.get 1
              call $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE
              local.set 0
              br 2 (;@3;)
            end
            local.get 0
            i32.load
            local.set 4
            i32.const 0
            local.set 0
            loop  ;; label = @5
              local.get 2
              local.get 0
              i32.add
              i32.const 127
              i32.add
              local.get 4
              i32.const 15
              i32.and
              local.tee 3
              i32.const 48
              i32.or
              local.get 3
              i32.const 87
              i32.add
              local.get 3
              i32.const 10
              i32.lt_u
              select
              i32.store8
              local.get 0
              i32.const -1
              i32.add
              local.set 0
              local.get 4
              i32.const 4
              i32.shr_u
              local.tee 4
              br_if 0 (;@5;)
            end
            local.get 0
            i32.const 128
            i32.add
            local.tee 4
            i32.const 129
            i32.ge_u
            br_if 2 (;@2;)
            local.get 1
            i32.const 1
            i32.const 1055064
            i32.const 2
            local.get 2
            local.get 0
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get 0
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
            local.set 0
            br 1 (;@3;)
          end
          i32.const 0
          local.set 0
          loop  ;; label = @4
            local.get 2
            local.get 0
            i32.add
            i32.const 127
            i32.add
            local.get 4
            i32.const 15
            i32.and
            local.tee 3
            i32.const 48
            i32.or
            local.get 3
            i32.const 55
            i32.add
            local.get 3
            i32.const 10
            i32.lt_u
            select
            i32.store8
            local.get 0
            i32.const -1
            i32.add
            local.set 0
            local.get 4
            i32.const 4
            i32.shr_u
            local.tee 4
            br_if 0 (;@4;)
          end
          local.get 0
          i32.const 128
          i32.add
          local.tee 4
          i32.const 129
          i32.ge_u
          br_if 2 (;@1;)
          local.get 1
          i32.const 1
          i32.const 1055064
          i32.const 2
          local.get 2
          local.get 0
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get 0
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
          local.set 0
        end
        local.get 2
        i32.const 128
        i32.add
        global.set 0
        local.get 0
        return
      end
      local.get 4
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 4
    i32.const 128
    i32.const 1055048
    call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
    unreachable)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hdd3815e1338426f9E (type 2) (param i32) (result i64)
    i64.const -1895584942197572387)
  (func $_ZN60_$LT$core..cell..BorrowError$u20$as$u20$core..fmt..Debug$GT$3fmt17h6b4af330c9438caaE (type 3) (param i32 i32) (result i32)
    local.get 1
    i32.load offset=24
    i32.const 1054684
    i32.const 11
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8))
  (func $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h79591e9d2985bd22E (type 3) (param i32 i32) (result i32)
    local.get 1
    i32.load offset=24
    i32.const 1054695
    i32.const 14
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8))
  (func $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17he27bc962589af5c1E (type 5) (param i32) (result i32)
    (local i32 i32)
    i32.const 1114112
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.load
            br_table 3 (;@1;) 2 (;@2;) 1 (;@3;) 0 (;@4;) 3 (;@1;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 0
                    i32.const 12
                    i32.add
                    i32.load8_u
                    br_table 7 (;@1;) 4 (;@4;) 3 (;@5;) 2 (;@6;) 1 (;@7;) 0 (;@8;) 7 (;@1;)
                  end
                  local.get 0
                  i32.const 4
                  i32.store8 offset=12
                  i32.const 92
                  return
                end
                local.get 0
                i32.const 3
                i32.store8 offset=12
                i32.const 117
                return
              end
              local.get 0
              i32.const 2
              i32.store8 offset=12
              i32.const 123
              return
            end
            i32.const 48
            i32.const 87
            local.get 0
            i32.load offset=4
            local.get 0
            i32.const 8
            i32.add
            i32.load
            local.tee 1
            i32.const 2
            i32.shl
            i32.const 28
            i32.and
            i32.shr_u
            i32.const 15
            i32.and
            local.tee 2
            i32.const 10
            i32.lt_u
            select
            local.get 2
            i32.add
            local.set 2
            block  ;; label = @5
              local.get 1
              i32.eqz
              br_if 0 (;@5;)
              local.get 0
              local.get 1
              i32.const -1
              i32.add
              i32.store offset=8
              local.get 2
              return
            end
            local.get 0
            i32.const 1
            i32.store8 offset=12
            local.get 2
            return
          end
          local.get 0
          i32.const 0
          i32.store8 offset=12
          i32.const 125
          return
        end
        local.get 0
        i32.const 1
        i32.store
        i32.const 92
        return
      end
      local.get 0
      i32.const 0
      i32.store
      local.get 0
      i32.load offset=4
      local.set 1
    end
    local.get 1)
  (func $_ZN4core3fmt8builders11DebugStruct5field17h74e4a270a55c26e5E (type 12) (param i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i64 i64)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 5
    global.set 0
    i32.const 1
    local.set 6
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=5
      local.set 7
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 8
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 6
        local.get 8
        i32.load offset=24
        i32.const 1054989
        i32.const 1054991
        local.get 7
        i32.const 255
        i32.and
        local.tee 7
        select
        i32.const 2
        i32.const 3
        local.get 7
        select
        local.get 8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        i32.const 1
        local.set 6
        local.get 0
        i32.load
        local.tee 8
        i32.load offset=24
        local.get 1
        local.get 2
        local.get 8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        i32.const 1
        local.set 6
        local.get 0
        i32.load
        local.tee 8
        i32.load offset=24
        i32.const 1054760
        i32.const 2
        local.get 8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 3
        local.get 0
        i32.load
        local.get 4
        i32.load offset=12
        call_indirect (type 3)
        local.set 6
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 7
        i32.const 255
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 6
        local.get 8
        i32.load offset=24
        i32.const 1054984
        i32.const 3
        local.get 8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.set 8
      end
      i32.const 1
      local.set 6
      local.get 5
      i32.const 1
      i32.store8 offset=23
      local.get 5
      i32.const 52
      i32.add
      i32.const 1054924
      i32.store
      local.get 5
      local.get 8
      i64.load offset=24 align=4
      i64.store offset=8
      local.get 5
      local.get 5
      i32.const 23
      i32.add
      i32.store offset=16
      local.get 8
      i64.load offset=8 align=4
      local.set 9
      local.get 8
      i64.load offset=16 align=4
      local.set 10
      local.get 5
      local.get 8
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get 5
      local.get 10
      i64.store offset=40
      local.get 5
      local.get 9
      i64.store offset=32
      local.get 5
      local.get 8
      i64.load align=4
      i64.store offset=24
      local.get 5
      local.get 5
      i32.const 8
      i32.add
      i32.store offset=48
      local.get 5
      i32.const 8
      i32.add
      local.get 1
      local.get 2
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E
      br_if 0 (;@1;)
      local.get 5
      i32.const 8
      i32.add
      i32.const 1054760
      i32.const 2
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E
      br_if 0 (;@1;)
      local.get 3
      local.get 5
      i32.const 24
      i32.add
      local.get 4
      i32.load offset=12
      call_indirect (type 3)
      br_if 0 (;@1;)
      local.get 5
      i32.load offset=48
      i32.const 1054987
      i32.const 2
      local.get 5
      i32.load offset=52
      i32.load offset=12
      call_indirect (type 8)
      local.set 6
    end
    local.get 0
    i32.const 1
    i32.store8 offset=5
    local.get 0
    local.get 6
    i32.store8 offset=4
    local.get 5
    i32.const 64
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core6option13expect_failed17h6275fb248fa0b383E (type 7) (param i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    local.get 3
    local.get 1
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 3
    i32.const 36
    i32.add
    i32.const 1
    i32.store
    local.get 3
    i64.const 1
    i64.store offset=20 align=4
    local.get 3
    i32.const 1054752
    i32.store offset=16
    local.get 3
    i32.const 81
    i32.store offset=44
    local.get 3
    local.get 3
    i32.const 40
    i32.add
    i32.store offset=32
    local.get 3
    local.get 3
    i32.const 8
    i32.add
    i32.store offset=40
    local.get 3
    i32.const 16
    i32.add
    local.get 2
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17he5f79f23f9f461d5E (type 3) (param i32 i32) (result i32)
    local.get 1
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    call $_ZN4core3fmt9Formatter3pad17h637274232327cb71E)
  (func $_ZN4core6option18expect_none_failed17h045d4d1a5bee2ad6E (type 11) (param i32 i32 i32 i32 i32)
    (local i32)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 5
    global.set 0
    local.get 5
    local.get 1
    i32.store offset=12
    local.get 5
    local.get 0
    i32.store offset=8
    local.get 5
    local.get 3
    i32.store offset=20
    local.get 5
    local.get 2
    i32.store offset=16
    local.get 5
    i32.const 44
    i32.add
    i32.const 2
    i32.store
    local.get 5
    i32.const 60
    i32.add
    i32.const 85
    i32.store
    local.get 5
    i64.const 2
    i64.store offset=28 align=4
    local.get 5
    i32.const 1054764
    i32.store offset=24
    local.get 5
    i32.const 81
    i32.store offset=52
    local.get 5
    local.get 5
    i32.const 48
    i32.add
    i32.store offset=40
    local.get 5
    local.get 5
    i32.const 16
    i32.add
    i32.store offset=56
    local.get 5
    local.get 5
    i32.const 8
    i32.add
    i32.store offset=48
    local.get 5
    i32.const 24
    i32.add
    local.get 4
    call $_ZN4core9panicking9panic_fmt17h25a5cdd1181be00cE
    unreachable)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17habd6be6b60c616e8E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 0
    i32.load offset=4
    i32.load offset=12
    call_indirect (type 3))
  (func $_ZN4core5panic9PanicInfo7message17h576f4b436581989eE (type 5) (param i32) (result i32)
    local.get 0
    i32.load offset=8)
  (func $_ZN4core5panic9PanicInfo8location17h289acf84f373c045E (type 5) (param i32) (result i32)
    local.get 0
    i32.load offset=12)
  (func $_ZN4core5panic8Location6caller17h071f67e970dcf83bE (type 5) (param i32) (result i32)
    local.get 0)
  (func $_ZN4core5panic8Location4file17h70cec7a74da50847E (type 6) (param i32 i32)
    local.get 0
    local.get 1
    i64.load align=4
    i64.store)
  (func $_ZN60_$LT$core..panic..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h24f4d50dfe86b3a3E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 20
    i32.add
    i32.const 17
    i32.store
    local.get 2
    i32.const 12
    i32.add
    i32.const 17
    i32.store
    local.get 2
    i32.const 81
    i32.store offset=4
    local.get 2
    local.get 0
    i32.store
    local.get 2
    local.get 0
    i32.const 12
    i32.add
    i32.store offset=16
    local.get 2
    local.get 0
    i32.const 8
    i32.add
    i32.store offset=8
    local.get 1
    i32.const 28
    i32.add
    i32.load
    local.set 0
    local.get 1
    i32.load offset=24
    local.set 1
    local.get 2
    i32.const 24
    i32.add
    i32.const 20
    i32.add
    i32.const 3
    i32.store
    local.get 2
    i64.const 3
    i64.store offset=28 align=4
    local.get 2
    i32.const 1054800
    i32.store offset=24
    local.get 2
    local.get 2
    i32.store offset=40
    local.get 1
    local.get 0
    local.get 2
    i32.const 24
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 0
    local.get 2
    i32.const 48
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        br_if 0 (;@2;)
        i32.const 0
        local.set 4
        br 1 (;@1;)
      end
      local.get 3
      i32.const 40
      i32.add
      local.set 5
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            loop  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.load offset=8
                i32.load8_u
                i32.eqz
                br_if 0 (;@6;)
                local.get 0
                i32.load
                i32.const 1054948
                i32.const 4
                local.get 0
                i32.load offset=4
                i32.load offset=12
                call_indirect (type 8)
                br_if 4 (;@2;)
              end
              local.get 3
              i32.const 10
              i32.store offset=40
              local.get 3
              i64.const 4294967306
              i64.store offset=32
              local.get 3
              local.get 2
              i32.store offset=28
              local.get 3
              i32.const 0
              i32.store offset=24
              local.get 3
              local.get 2
              i32.store offset=20
              local.get 3
              local.get 1
              i32.store offset=16
              local.get 3
              i32.const 8
              i32.add
              i32.const 10
              local.get 1
              local.get 2
              call $_ZN4core5slice6memchr6memchr17h55b5fdbafce1735dE
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 3
                      i32.load offset=8
                      i32.const 1
                      i32.ne
                      br_if 0 (;@9;)
                      local.get 3
                      i32.load offset=12
                      local.set 4
                      loop  ;; label = @10
                        local.get 3
                        local.get 4
                        local.get 3
                        i32.load offset=24
                        i32.add
                        i32.const 1
                        i32.add
                        local.tee 4
                        i32.store offset=24
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 4
                            local.get 3
                            i32.load offset=36
                            local.tee 6
                            i32.ge_u
                            br_if 0 (;@12;)
                            local.get 3
                            i32.load offset=20
                            local.set 7
                            br 1 (;@11;)
                          end
                          local.get 3
                          i32.load offset=20
                          local.tee 7
                          local.get 4
                          i32.lt_u
                          br_if 0 (;@11;)
                          local.get 6
                          i32.const 5
                          i32.ge_u
                          br_if 7 (;@4;)
                          local.get 3
                          i32.load offset=16
                          local.get 4
                          local.get 6
                          i32.sub
                          local.tee 8
                          i32.add
                          local.tee 9
                          local.get 5
                          i32.eq
                          br_if 4 (;@7;)
                          local.get 9
                          local.get 5
                          local.get 6
                          call $memcmp
                          i32.eqz
                          br_if 4 (;@7;)
                        end
                        local.get 3
                        i32.load offset=28
                        local.tee 9
                        local.get 4
                        i32.lt_u
                        br_if 2 (;@8;)
                        local.get 7
                        local.get 9
                        i32.lt_u
                        br_if 2 (;@8;)
                        local.get 3
                        local.get 6
                        local.get 3
                        i32.const 16
                        i32.add
                        i32.add
                        i32.const 23
                        i32.add
                        i32.load8_u
                        local.get 3
                        i32.load offset=16
                        local.get 4
                        i32.add
                        local.get 9
                        local.get 4
                        i32.sub
                        call $_ZN4core5slice6memchr6memchr17h55b5fdbafce1735dE
                        local.get 3
                        i32.load offset=4
                        local.set 4
                        local.get 3
                        i32.load
                        i32.const 1
                        i32.eq
                        br_if 0 (;@10;)
                      end
                    end
                    local.get 3
                    local.get 3
                    i32.load offset=28
                    i32.store offset=24
                  end
                  local.get 0
                  i32.load offset=8
                  i32.const 0
                  i32.store8
                  local.get 2
                  local.set 4
                  br 1 (;@6;)
                end
                local.get 0
                i32.load offset=8
                i32.const 1
                i32.store8
                local.get 8
                i32.const 1
                i32.add
                local.set 4
              end
              local.get 0
              i32.load offset=4
              local.set 9
              local.get 0
              i32.load
              local.set 6
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 4
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 2
                    local.get 4
                    i32.eq
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 2
                      local.get 4
                      i32.le_u
                      br_if 0 (;@9;)
                      local.get 1
                      local.get 4
                      i32.add
                      local.tee 7
                      i32.load8_s
                      i32.const -65
                      i32.gt_s
                      br_if 2 (;@7;)
                    end
                    local.get 1
                    local.get 2
                    i32.const 0
                    local.get 4
                    i32.const 1054952
                    call $_ZN4core3str16slice_error_fail17h457c150a5a135c79E
                    unreachable
                  end
                  local.get 6
                  local.get 1
                  local.get 4
                  local.get 9
                  i32.load offset=12
                  call_indirect (type 8)
                  br_if 5 (;@2;)
                  br 1 (;@6;)
                end
                local.get 6
                local.get 1
                local.get 4
                local.get 9
                i32.load offset=12
                call_indirect (type 8)
                br_if 4 (;@2;)
                local.get 7
                i32.load8_s
                i32.const -65
                i32.le_s
                br_if 3 (;@3;)
              end
              local.get 1
              local.get 4
              i32.add
              local.set 1
              local.get 2
              local.get 4
              i32.sub
              local.tee 2
              br_if 0 (;@5;)
            end
            i32.const 0
            local.set 4
            br 3 (;@1;)
          end
          local.get 6
          i32.const 4
          i32.const 1055692
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        local.get 1
        local.get 2
        local.get 4
        local.get 2
        i32.const 1054968
        call $_ZN4core3str16slice_error_fail17h457c150a5a135c79E
        unreachable
      end
      i32.const 1
      local.set 4
    end
    local.get 3
    i32.const 48
    i32.add
    global.set 0
    local.get 4)
  (func $_ZN4core5slice6memchr6memchr17h55b5fdbafce1735dE (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    i32.const 0
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            local.get 2
            i32.sub
            i32.const 3
            i32.and
            local.tee 5
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            local.get 5
            local.get 5
            local.get 3
            i32.gt_u
            select
            local.tee 6
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 5
            local.get 1
            i32.const 255
            i32.and
            local.set 4
            loop  ;; label = @5
              local.get 2
              local.get 5
              i32.add
              i32.load8_u
              local.get 4
              i32.eq
              br_if 2 (;@3;)
              local.get 6
              local.get 5
              i32.const 1
              i32.add
              local.tee 5
              i32.ne
              br_if 0 (;@5;)
            end
            local.get 6
            local.set 4
          end
          local.get 3
          i32.const 8
          i32.lt_u
          br_if 1 (;@2;)
          local.get 4
          local.get 3
          i32.const -8
          i32.add
          local.tee 7
          i32.gt_u
          br_if 1 (;@2;)
          local.get 1
          i32.const 255
          i32.and
          i32.const 16843009
          i32.mul
          local.set 5
          block  ;; label = @4
            loop  ;; label = @5
              local.get 2
              local.get 4
              i32.add
              local.tee 6
              i32.const 4
              i32.add
              i32.load
              local.get 5
              i32.xor
              local.tee 8
              i32.const -1
              i32.xor
              local.get 8
              i32.const -16843009
              i32.add
              i32.and
              local.get 6
              i32.load
              local.get 5
              i32.xor
              local.tee 6
              i32.const -1
              i32.xor
              local.get 6
              i32.const -16843009
              i32.add
              i32.and
              i32.or
              i32.const -2139062144
              i32.and
              br_if 1 (;@4;)
              local.get 4
              i32.const 8
              i32.add
              local.tee 4
              local.get 7
              i32.le_u
              br_if 0 (;@5;)
            end
          end
          local.get 4
          local.get 3
          i32.le_u
          br_if 1 (;@2;)
          local.get 4
          local.get 3
          i32.const 1055416
          call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
          unreachable
        end
        i32.const 1
        local.set 6
        br 1 (;@1;)
      end
      i32.const 0
      local.set 5
      i32.const 0
      local.set 6
      block  ;; label = @2
        local.get 4
        local.get 3
        i32.eq
        br_if 0 (;@2;)
        local.get 2
        local.get 4
        i32.add
        local.set 2
        local.get 3
        local.get 4
        i32.sub
        local.set 8
        i32.const 0
        local.set 5
        local.get 1
        i32.const 255
        i32.and
        local.set 6
        block  ;; label = @3
          loop  ;; label = @4
            local.get 2
            local.get 5
            i32.add
            i32.load8_u
            local.get 6
            i32.eq
            br_if 1 (;@3;)
            local.get 8
            local.get 5
            i32.const 1
            i32.add
            local.tee 5
            i32.ne
            br_if 0 (;@4;)
          end
          i32.const 0
          local.set 6
          local.get 8
          local.get 4
          i32.add
          local.set 5
          br 2 (;@1;)
        end
        i32.const 1
        local.set 6
        local.get 5
        local.set 5
      end
      local.get 5
      local.get 4
      i32.add
      local.set 5
    end
    local.get 0
    local.get 5
    i32.store offset=4
    local.get 0
    local.get 6
    i32.store)
  (func $_ZN4core3fmt8builders10DebugTuple5field17h0e16581ce3858b5aE (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i64 i64)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 3
    global.set 0
    i32.const 1
    local.set 4
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=8
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=4
      local.set 5
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 6
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 4
        local.get 6
        i32.load offset=24
        i32.const 1054989
        i32.const 1054999
        local.get 5
        select
        i32.const 2
        i32.const 1
        local.get 5
        select
        local.get 6
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 1
        local.get 0
        i32.load
        local.get 2
        i32.load offset=12
        call_indirect (type 3)
        local.set 4
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 5
        br_if 0 (;@2;)
        i32.const 1
        local.set 4
        local.get 6
        i32.load offset=24
        i32.const 1054997
        i32.const 2
        local.get 6
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.set 6
      end
      i32.const 1
      local.set 4
      local.get 3
      i32.const 1
      i32.store8 offset=23
      local.get 3
      i32.const 52
      i32.add
      i32.const 1054924
      i32.store
      local.get 3
      local.get 6
      i64.load offset=24 align=4
      i64.store offset=8
      local.get 3
      local.get 3
      i32.const 23
      i32.add
      i32.store offset=16
      local.get 6
      i64.load offset=8 align=4
      local.set 7
      local.get 6
      i64.load offset=16 align=4
      local.set 8
      local.get 3
      local.get 6
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get 3
      local.get 8
      i64.store offset=40
      local.get 3
      local.get 7
      i64.store offset=32
      local.get 3
      local.get 6
      i64.load align=4
      i64.store offset=24
      local.get 3
      local.get 3
      i32.const 8
      i32.add
      i32.store offset=48
      local.get 1
      local.get 3
      i32.const 24
      i32.add
      local.get 2
      i32.load offset=12
      call_indirect (type 3)
      br_if 0 (;@1;)
      local.get 3
      i32.load offset=48
      i32.const 1054987
      i32.const 2
      local.get 3
      i32.load offset=52
      i32.load offset=12
      call_indirect (type 8)
      local.set 4
    end
    local.get 0
    local.get 4
    i32.store8 offset=8
    local.get 0
    local.get 0
    i32.load offset=4
    i32.const 1
    i32.add
    i32.store offset=4
    local.get 3
    i32.const 64
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3fmt8builders10DebugTuple6finish17hcd73b6e7d638f97cE (type 5) (param i32) (result i32)
    (local i32 i32 i32)
    local.get 0
    i32.load8_u offset=8
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.load offset=4
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.const 255
      i32.and
      local.set 3
      i32.const 1
      local.set 1
      block  ;; label = @2
        local.get 3
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 2
          i32.const 1
          i32.ne
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u offset=9
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load
          local.tee 3
          i32.load8_u
          i32.const 4
          i32.and
          br_if 0 (;@3;)
          i32.const 1
          local.set 1
          local.get 3
          i32.load offset=24
          i32.const 1055000
          i32.const 1
          local.get 3
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 8)
          br_if 1 (;@2;)
        end
        local.get 0
        i32.load
        local.tee 1
        i32.load offset=24
        i32.const 1055001
        i32.const 1
        local.get 1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        local.set 1
      end
      local.get 0
      local.get 1
      i32.store8 offset=8
    end
    local.get 1
    i32.const 255
    i32.and
    i32.const 0
    i32.ne)
  (func $_ZN4core3fmt8builders10DebugInner5entry17h12a2219631ba5f0aE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i64 i64)
    global.get 0
    i32.const 64
    i32.sub
    local.tee 3
    global.set 0
    i32.const 1
    local.set 4
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      local.get 0
      i32.load8_u offset=5
      local.set 4
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 5
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 4
          i32.const 255
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1
          local.set 4
          local.get 5
          i32.load offset=24
          i32.const 1054989
          i32.const 2
          local.get 5
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 8)
          br_if 2 (;@1;)
          local.get 0
          i32.load
          local.set 5
        end
        local.get 1
        local.get 5
        local.get 2
        i32.load offset=12
        call_indirect (type 3)
        local.set 4
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 4
        i32.const 255
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 4
        local.get 5
        i32.load offset=24
        i32.const 1055002
        i32.const 1
        local.get 5
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.set 5
      end
      i32.const 1
      local.set 4
      local.get 3
      i32.const 1
      i32.store8 offset=23
      local.get 3
      i32.const 52
      i32.add
      i32.const 1054924
      i32.store
      local.get 3
      local.get 5
      i64.load offset=24 align=4
      i64.store offset=8
      local.get 3
      local.get 3
      i32.const 23
      i32.add
      i32.store offset=16
      local.get 5
      i64.load offset=8 align=4
      local.set 6
      local.get 5
      i64.load offset=16 align=4
      local.set 7
      local.get 3
      local.get 5
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get 3
      local.get 7
      i64.store offset=40
      local.get 3
      local.get 6
      i64.store offset=32
      local.get 3
      local.get 5
      i64.load align=4
      i64.store offset=24
      local.get 3
      local.get 3
      i32.const 8
      i32.add
      i32.store offset=48
      local.get 1
      local.get 3
      i32.const 24
      i32.add
      local.get 2
      i32.load offset=12
      call_indirect (type 3)
      br_if 0 (;@1;)
      local.get 3
      i32.load offset=48
      i32.const 1054987
      i32.const 2
      local.get 3
      i32.load offset=52
      i32.load offset=12
      call_indirect (type 8)
      local.set 4
    end
    local.get 0
    i32.const 1
    i32.store8 offset=5
    local.get 0
    local.get 4
    i32.store8 offset=4
    local.get 3
    i32.const 64
    i32.add
    global.set 0)
  (func $_ZN4core3fmt8builders8DebugSet5entry17hefdeddab1ca2936fE (type 8) (param i32 i32 i32) (result i32)
    local.get 0
    local.get 1
    local.get 2
    call $_ZN4core3fmt8builders10DebugInner5entry17h12a2219631ba5f0aE
    local.get 0)
  (func $_ZN4core3fmt8builders9DebugList6finish17ha88b4158b5009243E (type 5) (param i32) (result i32)
    (local i32)
    i32.const 1
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      local.get 0
      i32.load
      local.tee 0
      i32.load offset=24
      i32.const 1055020
      i32.const 1
      local.get 0
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type 8)
      local.set 1
    end
    local.get 1)
  (func $_ZN4core3fmt5Write10write_char17h855ab183fb53bf2bE (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    i32.const 0
    i32.store offset=12
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 128
            i32.lt_u
            br_if 0 (;@4;)
            local.get 1
            i32.const 2048
            i32.lt_u
            br_if 1 (;@3;)
            local.get 2
            i32.const 12
            i32.add
            local.set 3
            local.get 1
            i32.const 65536
            i32.ge_u
            br_if 2 (;@2;)
            local.get 2
            local.get 1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get 2
            local.get 1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get 2
            local.get 1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set 1
            br 3 (;@1;)
          end
          local.get 2
          local.get 1
          i32.store8 offset=12
          local.get 2
          i32.const 12
          i32.add
          local.set 3
          i32.const 1
          local.set 1
          br 2 (;@1;)
        end
        local.get 2
        local.get 1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get 2
        local.get 1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        local.get 2
        i32.const 12
        i32.add
        local.set 3
        i32.const 2
        local.set 1
        br 1 (;@1;)
      end
      local.get 2
      local.get 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get 2
      local.get 1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get 2
      local.get 1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get 2
      local.get 1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set 1
    end
    local.get 0
    local.get 3
    local.get 1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN4core3fmt5Write9write_fmt17hf56be3afcac3485dE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1055268
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h76664f8c3d7fdb62E (type 8) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h10e0193b0dcc19e4E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.set 0
    local.get 2
    i32.const 0
    i32.store offset=12
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 128
            i32.lt_u
            br_if 0 (;@4;)
            local.get 1
            i32.const 2048
            i32.lt_u
            br_if 1 (;@3;)
            local.get 2
            i32.const 12
            i32.add
            local.set 3
            local.get 1
            i32.const 65536
            i32.ge_u
            br_if 2 (;@2;)
            local.get 2
            local.get 1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get 2
            local.get 1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get 2
            local.get 1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set 1
            br 3 (;@1;)
          end
          local.get 2
          local.get 1
          i32.store8 offset=12
          local.get 2
          i32.const 12
          i32.add
          local.set 3
          i32.const 1
          local.set 1
          br 2 (;@1;)
        end
        local.get 2
        local.get 1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get 2
        local.get 1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        local.get 2
        i32.const 12
        i32.add
        local.set 3
        i32.const 2
        local.set 1
        br 1 (;@1;)
      end
      local.get 2
      local.get 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get 2
      local.get 1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get 2
      local.get 1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get 2
      local.get 1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set 1
    end
    local.get 0
    local.get 3
    local.get 1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hd8c69e3f7783e4efE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 2
    local.get 0
    i32.load
    i32.store offset=4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 2
    i32.const 4
    i32.add
    i32.const 1055268
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h8c004d7930574d45E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 1
    i32.const 28
    i32.add
    i32.load
    local.set 3
    local.get 1
    i32.load offset=24
    local.set 1
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 0
    i64.load align=4
    i64.store offset=8
    local.get 1
    local.get 3
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 0
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E (type 13) (param i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.eqz
        br_if 0 (;@2;)
        i32.const 43
        i32.const 1114112
        local.get 0
        i32.load
        local.tee 6
        i32.const 1
        i32.and
        local.tee 1
        select
        local.set 7
        local.get 1
        local.get 5
        i32.add
        local.set 8
        br 1 (;@1;)
      end
      local.get 5
      i32.const 1
      i32.add
      local.set 8
      local.get 0
      i32.load
      local.set 6
      i32.const 45
      local.set 7
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 6
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      i32.const 0
      local.set 9
      block  ;; label = @2
        local.get 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.set 10
        local.get 2
        local.set 1
        loop  ;; label = @3
          local.get 9
          local.get 1
          i32.load8_u
          i32.const 192
          i32.and
          i32.const 128
          i32.eq
          i32.add
          local.set 9
          local.get 1
          i32.const 1
          i32.add
          local.set 1
          local.get 10
          i32.const -1
          i32.add
          local.tee 10
          br_if 0 (;@3;)
        end
      end
      local.get 8
      local.get 3
      i32.add
      local.get 9
      i32.sub
      local.set 8
    end
    i32.const 1
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load offset=8
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        local.get 7
        local.get 2
        local.get 3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h24e4a757f662f4aaE
        br_if 1 (;@1;)
        local.get 0
        i32.load offset=24
        local.get 4
        local.get 5
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        local.set 1
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 0
        i32.const 12
        i32.add
        i32.load
        local.tee 9
        local.get 8
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.get 7
        local.get 2
        local.get 3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h24e4a757f662f4aaE
        br_if 1 (;@1;)
        local.get 0
        i32.load offset=24
        local.get 4
        local.get 5
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        return
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 6
                i32.const 8
                i32.and
                i32.eqz
                br_if 0 (;@6;)
                local.get 0
                i32.load offset=4
                local.set 6
                local.get 0
                i32.const 48
                i32.store offset=4
                local.get 0
                i32.load8_u offset=32
                local.set 11
                i32.const 1
                local.set 1
                local.get 0
                i32.const 1
                i32.store8 offset=32
                local.get 0
                local.get 7
                local.get 2
                local.get 3
                call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h24e4a757f662f4aaE
                br_if 5 (;@1;)
                i32.const 0
                local.set 1
                local.get 9
                local.get 8
                i32.sub
                local.tee 10
                local.set 3
                i32.const 1
                local.get 0
                i32.load8_u offset=32
                local.tee 9
                local.get 9
                i32.const 3
                i32.eq
                select
                i32.const 3
                i32.and
                br_table 3 (;@3;) 2 (;@4;) 1 (;@5;) 2 (;@4;) 3 (;@3;)
              end
              i32.const 0
              local.set 1
              local.get 9
              local.get 8
              i32.sub
              local.tee 9
              local.set 8
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    i32.const 1
                    local.get 0
                    i32.load8_u offset=32
                    local.tee 10
                    local.get 10
                    i32.const 3
                    i32.eq
                    select
                    i32.const 3
                    i32.and
                    br_table 2 (;@6;) 1 (;@7;) 0 (;@8;) 1 (;@7;) 2 (;@6;)
                  end
                  local.get 9
                  i32.const 1
                  i32.shr_u
                  local.set 1
                  local.get 9
                  i32.const 1
                  i32.add
                  i32.const 1
                  i32.shr_u
                  local.set 8
                  br 1 (;@6;)
                end
                i32.const 0
                local.set 8
                local.get 9
                local.set 1
              end
              local.get 1
              i32.const 1
              i32.add
              local.set 1
              loop  ;; label = @6
                local.get 1
                i32.const -1
                i32.add
                local.tee 1
                i32.eqz
                br_if 4 (;@2;)
                local.get 0
                i32.load offset=24
                local.get 0
                i32.load offset=4
                local.get 0
                i32.load offset=28
                i32.load offset=16
                call_indirect (type 3)
                i32.eqz
                br_if 0 (;@6;)
              end
              i32.const 1
              return
            end
            local.get 10
            i32.const 1
            i32.shr_u
            local.set 1
            local.get 10
            i32.const 1
            i32.add
            i32.const 1
            i32.shr_u
            local.set 3
            br 1 (;@3;)
          end
          i32.const 0
          local.set 3
          local.get 10
          local.set 1
        end
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        block  ;; label = @3
          loop  ;; label = @4
            local.get 1
            i32.const -1
            i32.add
            local.tee 1
            i32.eqz
            br_if 1 (;@3;)
            local.get 0
            i32.load offset=24
            local.get 0
            i32.load offset=4
            local.get 0
            i32.load offset=28
            i32.load offset=16
            call_indirect (type 3)
            i32.eqz
            br_if 0 (;@4;)
          end
          i32.const 1
          return
        end
        local.get 0
        i32.load offset=4
        local.set 10
        i32.const 1
        local.set 1
        local.get 0
        i32.load offset=24
        local.get 4
        local.get 5
        local.get 0
        i32.load offset=28
        i32.load offset=12
        call_indirect (type 8)
        br_if 1 (;@1;)
        local.get 3
        i32.const 1
        i32.add
        local.set 9
        local.get 0
        i32.load offset=28
        local.set 3
        local.get 0
        i32.load offset=24
        local.set 2
        block  ;; label = @3
          loop  ;; label = @4
            local.get 9
            i32.const -1
            i32.add
            local.tee 9
            i32.eqz
            br_if 1 (;@3;)
            i32.const 1
            local.set 1
            local.get 2
            local.get 10
            local.get 3
            i32.load offset=16
            call_indirect (type 3)
            br_if 3 (;@1;)
            br 0 (;@4;)
          end
        end
        local.get 0
        local.get 11
        i32.store8 offset=32
        local.get 0
        local.get 6
        i32.store offset=4
        i32.const 0
        return
      end
      local.get 0
      i32.load offset=4
      local.set 10
      i32.const 1
      local.set 1
      local.get 0
      local.get 7
      local.get 2
      local.get 3
      call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h24e4a757f662f4aaE
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=24
      local.get 4
      local.get 5
      local.get 0
      i32.load offset=28
      i32.load offset=12
      call_indirect (type 8)
      br_if 0 (;@1;)
      local.get 8
      i32.const 1
      i32.add
      local.set 9
      local.get 0
      i32.load offset=28
      local.set 3
      local.get 0
      i32.load offset=24
      local.set 0
      loop  ;; label = @2
        block  ;; label = @3
          local.get 9
          i32.const -1
          i32.add
          local.tee 9
          br_if 0 (;@3;)
          i32.const 0
          return
        end
        i32.const 1
        local.set 1
        local.get 0
        local.get 10
        local.get 3
        i32.load offset=16
        call_indirect (type 3)
        i32.eqz
        br_if 0 (;@2;)
      end
    end
    local.get 1)
  (func $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h24e4a757f662f4aaE (type 9) (param i32 i32 i32 i32) (result i32)
    (local i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 1114112
        i32.eq
        br_if 0 (;@2;)
        i32.const 1
        local.set 4
        local.get 0
        i32.load offset=24
        local.get 1
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=16
        call_indirect (type 3)
        br_if 1 (;@1;)
      end
      block  ;; label = @2
        local.get 2
        br_if 0 (;@2;)
        i32.const 0
        return
      end
      local.get 0
      i32.load offset=24
      local.get 2
      local.get 3
      local.get 0
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type 8)
      local.set 4
    end
    local.get 4)
  (func $_ZN4core3fmt9Formatter9write_str17he7969cebfa3abe45E (type 8) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load offset=24
    local.get 1
    local.get 2
    local.get 0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8))
  (func $_ZN4core3fmt9Formatter9write_fmt17hc26cc156d7763708E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 32
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.const 28
    i32.add
    i32.load
    local.set 3
    local.get 0
    i32.load offset=24
    local.set 0
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 2
    local.get 1
    i64.load align=4
    i64.store offset=8
    local.get 0
    local.get 3
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17h1626e57fa473d161E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN4core3fmt9Formatter15debug_lower_hex17hfc16dde1881b9db0E (type 5) (param i32) (result i32)
    local.get 0
    i32.load8_u
    i32.const 16
    i32.and
    i32.const 4
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter15debug_upper_hex17hf900d6ea61ff8af8E (type 5) (param i32) (result i32)
    local.get 0
    i32.load8_u
    i32.const 32
    i32.and
    i32.const 5
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter11debug_tuple17h687ff740dc6e8836E (type 4) (param i32 i32 i32 i32)
    local.get 0
    local.get 1
    i32.load offset=24
    local.get 2
    local.get 3
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8)
    i32.store8 offset=8
    local.get 0
    local.get 1
    i32.store
    local.get 0
    local.get 3
    i32.eqz
    i32.store8 offset=9
    local.get 0
    i32.const 0
    i32.store offset=4)
  (func $_ZN4core3fmt9Formatter10debug_list17h030b105a753c1383E (type 6) (param i32 i32)
    (local i32)
    local.get 1
    i32.load offset=24
    i32.const 1055003
    i32.const 1
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8)
    local.set 2
    local.get 0
    i32.const 0
    i32.store8 offset=5
    local.get 0
    local.get 2
    i32.store8 offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h60363d204979c277E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load offset=24
    local.get 1
    local.get 0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=16
    call_indirect (type 3))
  (func $_ZN40_$LT$str$u20$as$u20$core..fmt..Debug$GT$3fmt17h9a17d4d05f37d658E (type 8) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
    i32.const 1
    local.set 3
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.load offset=24
        i32.const 34
        local.get 2
        i32.const 28
        i32.add
        i32.load
        i32.load offset=16
        call_indirect (type 3)
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            br_if 0 (;@4;)
            i32.const 0
            local.set 4
            br 1 (;@3;)
          end
          local.get 0
          local.get 1
          i32.add
          local.set 5
          i32.const 0
          local.set 4
          local.get 0
          local.set 6
          local.get 0
          local.set 7
          i32.const 0
          local.set 8
          block  ;; label = @4
            loop  ;; label = @5
              local.get 6
              i32.const 1
              i32.add
              local.set 9
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 6
                    i32.load8_s
                    local.tee 10
                    i32.const -1
                    i32.gt_s
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 9
                        local.get 5
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.set 11
                        local.get 5
                        local.set 6
                        br 1 (;@9;)
                      end
                      local.get 6
                      i32.load8_u offset=1
                      i32.const 63
                      i32.and
                      local.set 11
                      local.get 6
                      i32.const 2
                      i32.add
                      local.tee 9
                      local.set 6
                    end
                    local.get 10
                    i32.const 31
                    i32.and
                    local.set 3
                    block  ;; label = @9
                      local.get 10
                      i32.const 255
                      i32.and
                      local.tee 10
                      i32.const 223
                      i32.gt_u
                      br_if 0 (;@9;)
                      local.get 11
                      local.get 3
                      i32.const 6
                      i32.shl
                      i32.or
                      local.set 11
                      br 2 (;@7;)
                    end
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 6
                        local.get 5
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.set 12
                        local.get 5
                        local.set 13
                        br 1 (;@9;)
                      end
                      local.get 6
                      i32.load8_u
                      i32.const 63
                      i32.and
                      local.set 12
                      local.get 6
                      i32.const 1
                      i32.add
                      local.tee 9
                      local.set 13
                    end
                    local.get 12
                    local.get 11
                    i32.const 6
                    i32.shl
                    i32.or
                    local.set 11
                    block  ;; label = @9
                      local.get 10
                      i32.const 240
                      i32.ge_u
                      br_if 0 (;@9;)
                      local.get 11
                      local.get 3
                      i32.const 12
                      i32.shl
                      i32.or
                      local.set 11
                      br 2 (;@7;)
                    end
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 13
                        local.get 5
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.set 10
                        local.get 9
                        local.set 6
                        br 1 (;@9;)
                      end
                      local.get 13
                      i32.const 1
                      i32.add
                      local.set 6
                      local.get 13
                      i32.load8_u
                      i32.const 63
                      i32.and
                      local.set 10
                    end
                    local.get 11
                    i32.const 6
                    i32.shl
                    local.get 3
                    i32.const 18
                    i32.shl
                    i32.const 1835008
                    i32.and
                    i32.or
                    local.get 10
                    i32.or
                    local.tee 11
                    i32.const 1114112
                    i32.ne
                    br_if 2 (;@6;)
                    br 4 (;@4;)
                  end
                  local.get 10
                  i32.const 255
                  i32.and
                  local.set 11
                end
                local.get 9
                local.set 6
              end
              i32.const 2
              local.set 9
              i32.const 116
              local.set 13
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 11
                              i32.const -9
                              i32.add
                              br_table 6 (;@7;) 1 (;@12;) 3 (;@10;) 3 (;@10;) 0 (;@13;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 4 (;@9;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 3 (;@10;) 4 (;@9;) 2 (;@11;)
                            end
                            i32.const 114
                            local.set 13
                            br 5 (;@7;)
                          end
                          i32.const 110
                          local.set 13
                          br 4 (;@7;)
                        end
                        local.get 11
                        i32.const 92
                        i32.eq
                        br_if 1 (;@9;)
                      end
                      block  ;; label = @10
                        local.get 11
                        call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h29890100314325f2E
                        br_if 0 (;@10;)
                        local.get 11
                        call $_ZN4core7unicode9printable12is_printable17hbb0b3da4690fa839E
                        br_if 4 (;@6;)
                      end
                      local.get 11
                      i32.const 1
                      i32.or
                      i32.clz
                      i32.const 2
                      i32.shr_u
                      i32.const 7
                      i32.xor
                      i64.extend_i32_u
                      i64.const 21474836480
                      i64.or
                      local.set 14
                      i32.const 3
                      local.set 9
                      br 1 (;@8;)
                    end
                  end
                  local.get 11
                  local.set 13
                end
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 8
                    local.get 4
                    i32.lt_u
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 4
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 4
                      local.get 1
                      i32.eq
                      br_if 0 (;@9;)
                      local.get 4
                      local.get 1
                      i32.ge_u
                      br_if 1 (;@8;)
                      local.get 0
                      local.get 4
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 1 (;@8;)
                    end
                    block  ;; label = @9
                      local.get 8
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 8
                      local.get 1
                      i32.eq
                      br_if 0 (;@9;)
                      local.get 8
                      local.get 1
                      i32.ge_u
                      br_if 1 (;@8;)
                      local.get 0
                      local.get 8
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 1 (;@8;)
                    end
                    local.get 2
                    i32.load offset=24
                    local.get 0
                    local.get 4
                    i32.add
                    local.get 8
                    local.get 4
                    i32.sub
                    local.get 2
                    i32.load offset=28
                    i32.load offset=12
                    call_indirect (type 8)
                    i32.eqz
                    br_if 1 (;@7;)
                    i32.const 1
                    return
                  end
                  local.get 0
                  local.get 1
                  local.get 4
                  local.get 8
                  i32.const 1055352
                  call $_ZN4core3str16slice_error_fail17h457c150a5a135c79E
                  unreachable
                end
                loop  ;; label = @7
                  local.get 9
                  local.set 10
                  i32.const 1
                  local.set 3
                  i32.const 92
                  local.set 4
                  i32.const 1
                  local.set 9
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 10
                              br_table 2 (;@11;) 1 (;@12;) 5 (;@8;) 0 (;@13;) 2 (;@11;)
                            end
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    local.get 14
                                    i64.const 32
                                    i64.shr_u
                                    i32.wrap_i64
                                    i32.const 255
                                    i32.and
                                    br_table 5 (;@11;) 3 (;@13;) 2 (;@14;) 1 (;@15;) 0 (;@16;) 6 (;@10;) 5 (;@11;)
                                  end
                                  local.get 14
                                  i64.const -1095216660481
                                  i64.and
                                  i64.const 12884901888
                                  i64.or
                                  local.set 14
                                  i32.const 3
                                  local.set 9
                                  i32.const 117
                                  local.set 4
                                  br 7 (;@8;)
                                end
                                local.get 14
                                i64.const -1095216660481
                                i64.and
                                i64.const 8589934592
                                i64.or
                                local.set 14
                                i32.const 3
                                local.set 9
                                i32.const 123
                                local.set 4
                                br 6 (;@8;)
                              end
                              i32.const 48
                              i32.const 87
                              local.get 13
                              local.get 14
                              i32.wrap_i64
                              local.tee 9
                              i32.const 2
                              i32.shl
                              i32.const 28
                              i32.and
                              i32.shr_u
                              i32.const 15
                              i32.and
                              local.tee 4
                              i32.const 10
                              i32.lt_u
                              select
                              local.get 4
                              i32.add
                              local.set 4
                              block  ;; label = @14
                                local.get 9
                                i32.eqz
                                br_if 0 (;@14;)
                                local.get 14
                                i64.const -1
                                i64.add
                                i64.const 4294967295
                                i64.and
                                local.get 14
                                i64.const -4294967296
                                i64.and
                                i64.or
                                local.set 14
                                br 5 (;@9;)
                              end
                              local.get 14
                              i64.const -1095216660481
                              i64.and
                              i64.const 4294967296
                              i64.or
                              local.set 14
                              br 4 (;@9;)
                            end
                            local.get 14
                            i64.const -1095216660481
                            i64.and
                            local.set 14
                            i32.const 3
                            local.set 9
                            i32.const 125
                            local.set 4
                            br 4 (;@8;)
                          end
                          i32.const 0
                          local.set 9
                          local.get 13
                          local.set 4
                          br 3 (;@8;)
                        end
                        i32.const 1
                        local.set 9
                        block  ;; label = @11
                          local.get 11
                          i32.const 128
                          i32.lt_u
                          br_if 0 (;@11;)
                          i32.const 2
                          local.set 9
                          local.get 11
                          i32.const 2048
                          i32.lt_u
                          br_if 0 (;@11;)
                          i32.const 3
                          i32.const 4
                          local.get 11
                          i32.const 65536
                          i32.lt_u
                          select
                          local.set 9
                        end
                        local.get 9
                        local.get 8
                        i32.add
                        local.set 4
                        br 4 (;@6;)
                      end
                      local.get 14
                      i64.const -1095216660481
                      i64.and
                      i64.const 17179869184
                      i64.or
                      local.set 14
                    end
                    i32.const 3
                    local.set 9
                  end
                  local.get 2
                  i32.load offset=24
                  local.get 4
                  local.get 2
                  i32.load offset=28
                  i32.load offset=16
                  call_indirect (type 3)
                  i32.eqz
                  br_if 0 (;@7;)
                  br 5 (;@2;)
                end
              end
              local.get 8
              local.get 7
              i32.sub
              local.get 6
              i32.add
              local.set 8
              local.get 6
              local.set 7
              local.get 5
              local.get 6
              i32.ne
              br_if 0 (;@5;)
            end
          end
          local.get 4
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          local.get 1
          i32.eq
          br_if 0 (;@3;)
          local.get 4
          local.get 1
          i32.ge_u
          br_if 2 (;@1;)
          local.get 0
          local.get 4
          i32.add
          i32.load8_s
          i32.const -65
          i32.le_s
          br_if 2 (;@1;)
        end
        i32.const 1
        local.set 3
        local.get 2
        i32.load offset=24
        local.get 0
        local.get 4
        i32.add
        local.get 1
        local.get 4
        i32.sub
        local.get 2
        i32.load offset=28
        i32.load offset=12
        call_indirect (type 8)
        br_if 0 (;@2;)
        local.get 2
        i32.load offset=24
        i32.const 34
        local.get 2
        i32.load offset=28
        i32.load offset=16
        call_indirect (type 3)
        return
      end
      local.get 3
      return
    end
    local.get 0
    local.get 1
    local.get 4
    local.get 1
    i32.const 1055368
    call $_ZN4core3str16slice_error_fail17h457c150a5a135c79E
    unreachable)
  (func $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h29890100314325f2E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.const 15
          local.get 0
          i32.const 68900
          i32.lt_u
          select
          local.tee 1
          local.get 1
          i32.const 8
          i32.add
          local.tee 1
          local.get 1
          i32.const 2
          i32.shl
          i32.const 1057916
          i32.add
          i32.load
          i32.const 11
          i32.shl
          local.get 0
          i32.const 11
          i32.shl
          local.tee 1
          i32.gt_u
          select
          local.tee 2
          local.get 2
          i32.const 4
          i32.add
          local.tee 2
          local.get 2
          i32.const 2
          i32.shl
          i32.const 1057916
          i32.add
          i32.load
          i32.const 11
          i32.shl
          local.get 1
          i32.gt_u
          select
          local.tee 2
          local.get 2
          i32.const 2
          i32.add
          local.tee 2
          local.get 2
          i32.const 2
          i32.shl
          i32.const 1057916
          i32.add
          i32.load
          i32.const 11
          i32.shl
          local.get 1
          i32.gt_u
          select
          local.tee 2
          local.get 2
          i32.const 1
          i32.add
          local.tee 2
          local.get 2
          i32.const 2
          i32.shl
          i32.const 1057916
          i32.add
          i32.load
          i32.const 11
          i32.shl
          local.get 1
          i32.gt_u
          select
          local.tee 2
          i32.const 2
          i32.shl
          i32.const 1057916
          i32.add
          i32.load
          i32.const 11
          i32.shl
          local.tee 3
          local.get 1
          i32.eq
          local.get 3
          local.get 1
          i32.lt_u
          i32.add
          local.get 2
          i32.add
          local.tee 1
          i32.const 30
          i32.gt_u
          br_if 0 (;@3;)
          i32.const 689
          local.set 4
          block  ;; label = @4
            local.get 1
            i32.const 30
            i32.eq
            br_if 0 (;@4;)
            local.get 1
            i32.const 2
            i32.shl
            i32.const 1057920
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.set 4
          end
          i32.const 0
          local.set 2
          block  ;; label = @4
            local.get 1
            i32.const -1
            i32.add
            local.tee 3
            local.get 1
            i32.gt_u
            br_if 0 (;@4;)
            local.get 3
            i32.const 31
            i32.ge_u
            br_if 3 (;@1;)
            local.get 3
            i32.const 2
            i32.shl
            i32.const 1057916
            i32.add
            i32.load
            i32.const 2097151
            i32.and
            local.set 2
          end
          block  ;; label = @4
            local.get 4
            local.get 1
            i32.const 2
            i32.shl
            i32.const 1057916
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.tee 1
            i32.const 1
            i32.add
            i32.eq
            br_if 0 (;@4;)
            local.get 0
            local.get 2
            i32.sub
            local.set 2
            local.get 1
            i32.const 689
            local.get 1
            i32.const 689
            i32.gt_u
            select
            local.set 3
            local.get 4
            i32.const -1
            i32.add
            local.set 4
            i32.const 0
            local.set 0
            loop  ;; label = @5
              local.get 3
              local.get 1
              i32.eq
              br_if 3 (;@2;)
              local.get 0
              local.get 1
              i32.const 1058040
              i32.add
              i32.load8_u
              i32.add
              local.tee 0
              local.get 2
              i32.gt_u
              br_if 1 (;@4;)
              local.get 4
              local.get 1
              i32.const 1
              i32.add
              local.tee 1
              i32.ne
              br_if 0 (;@5;)
            end
            local.get 4
            local.set 1
          end
          local.get 1
          i32.const 1
          i32.and
          return
        end
        local.get 1
        i32.const 31
        i32.const 1057796
        call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
        unreachable
      end
      local.get 3
      i32.const 689
      i32.const 1057812
      call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
      unreachable
    end
    local.get 3
    i32.const 31
    i32.const 1057828
    call $_ZN4core9panicking18panic_bounds_check17h0808cd2679532415E
    unreachable)
  (func $_ZN4core7unicode9printable12is_printable17hbb0b3da4690fa839E (type 5) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 0
                        i32.const 65536
                        i32.lt_u
                        br_if 0 (;@10;)
                        local.get 0
                        i32.const 131072
                        i32.lt_u
                        br_if 1 (;@9;)
                        i32.const 0
                        local.set 1
                        local.get 0
                        i32.const -201547
                        i32.add
                        i32.const 716213
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const -195102
                        i32.add
                        i32.const 1506
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const -191457
                        i32.add
                        i32.const 3103
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const -183970
                        i32.add
                        i32.const 14
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const 2097150
                        i32.and
                        i32.const 178206
                        i32.eq
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const -173790
                        i32.add
                        i32.const 34
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const -177973
                        i32.add
                        i32.const 11
                        i32.lt_u
                        br_if 8 (;@2;)
                        local.get 0
                        i32.const 918000
                        i32.lt_u
                        return
                      end
                      local.get 0
                      i32.const 65280
                      i32.and
                      i32.const 8
                      i32.shr_u
                      local.set 2
                      i32.const 1056404
                      local.set 3
                      i32.const 0
                      local.set 4
                      local.get 0
                      i32.const 255
                      i32.and
                      local.set 5
                      loop  ;; label = @10
                        local.get 3
                        i32.const 2
                        i32.add
                        local.set 6
                        local.get 4
                        local.get 3
                        i32.load8_u offset=1
                        local.tee 1
                        i32.add
                        local.set 7
                        block  ;; label = @11
                          local.get 3
                          i32.load8_u
                          local.tee 3
                          local.get 2
                          i32.eq
                          br_if 0 (;@11;)
                          local.get 3
                          local.get 2
                          i32.gt_u
                          br_if 8 (;@3;)
                          local.get 7
                          local.set 4
                          local.get 6
                          local.set 3
                          local.get 6
                          i32.const 1056486
                          i32.ne
                          br_if 1 (;@10;)
                          br 8 (;@3;)
                        end
                        local.get 7
                        local.get 4
                        i32.lt_u
                        br_if 2 (;@8;)
                        local.get 7
                        i32.const 290
                        i32.gt_u
                        br_if 3 (;@7;)
                        local.get 4
                        i32.const 1056486
                        i32.add
                        local.set 3
                        block  ;; label = @11
                          loop  ;; label = @12
                            local.get 1
                            i32.eqz
                            br_if 1 (;@11;)
                            local.get 1
                            i32.const -1
                            i32.add
                            local.set 1
                            local.get 3
                            i32.load8_u
                            local.set 4
                            local.get 3
                            i32.const 1
                            i32.add
                            local.set 3
                            local.get 4
                            local.get 5
                            i32.ne
                            br_if 0 (;@12;)
                          end
                          i32.const 0
                          local.set 1
                          br 9 (;@2;)
                        end
                        local.get 7
                        local.set 4
                        local.get 6
                        local.set 3
                        local.get 6
                        i32.const 1056486
                        i32.ne
                        br_if 0 (;@10;)
                        br 7 (;@3;)
                      end
                    end
                    local.get 0
                    i32.const 65280
                    i32.and
                    i32.const 8
                    i32.shr_u
                    local.set 2
                    i32.const 1057085
                    local.set 3
                    i32.const 0
                    local.set 4
                    local.get 0
                    i32.const 255
                    i32.and
                    local.set 5
                    loop  ;; label = @9
                      local.get 3
                      i32.const 2
                      i32.add
                      local.set 6
                      local.get 4
                      local.get 3
                      i32.load8_u offset=1
                      local.tee 1
                      i32.add
                      local.set 7
                      block  ;; label = @10
                        local.get 3
                        i32.load8_u
                        local.tee 3
                        local.get 2
                        i32.eq
                        br_if 0 (;@10;)
                        local.get 3
                        local.get 2
                        i32.gt_u
                        br_if 6 (;@4;)
                        local.get 7
                        local.set 4
                        local.get 6
                        local.set 3
                        local.get 6
                        i32.const 1057161
                        i32.ne
                        br_if 1 (;@9;)
                        br 6 (;@4;)
                      end
                      local.get 7
                      local.get 4
                      i32.lt_u
                      br_if 3 (;@6;)
                      local.get 7
                      i32.const 175
                      i32.gt_u
                      br_if 4 (;@5;)
                      local.get 4
                      i32.const 1057161
                      i32.add
                      local.set 3
                      block  ;; label = @10
                        loop  ;; label = @11
                          local.get 1
                          i32.eqz
                          br_if 1 (;@10;)
                          local.get 1
                          i32.const -1
                          i32.add
                          local.set 1
                          local.get 3
                          i32.load8_u
                          local.set 4
                          local.get 3
                          i32.const 1
                          i32.add
                          local.set 3
                          local.get 4
                          local.get 5
                          i32.ne
                          br_if 0 (;@11;)
                        end
                        i32.const 0
                        local.set 1
                        br 8 (;@2;)
                      end
                      local.get 7
                      local.set 4
                      local.get 6
                      local.set 3
                      local.get 6
                      i32.const 1057161
                      i32.ne
                      br_if 0 (;@9;)
                      br 5 (;@4;)
                    end
                  end
                  local.get 4
                  local.get 7
                  i32.const 1056372
                  call $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E
                  unreachable
                end
                local.get 7
                i32.const 290
                i32.const 1056372
                call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                unreachable
              end
              local.get 4
              local.get 7
              i32.const 1056372
              call $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E
              unreachable
            end
            local.get 7
            i32.const 175
            i32.const 1056372
            call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
            unreachable
          end
          local.get 0
          i32.const 65535
          i32.and
          local.set 5
          i32.const 1057336
          local.set 3
          i32.const 1
          local.set 1
          block  ;; label = @4
            loop  ;; label = @5
              local.get 3
              i32.const 1
              i32.add
              local.set 0
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  i32.load8_u
                  local.tee 4
                  i32.const 24
                  i32.shl
                  i32.const 24
                  i32.shr_s
                  local.tee 7
                  i32.const 0
                  i32.lt_s
                  br_if 0 (;@7;)
                  local.get 0
                  local.set 3
                  br 1 (;@6;)
                end
                local.get 0
                i32.const 1057755
                i32.eq
                br_if 2 (;@4;)
                local.get 7
                i32.const 127
                i32.and
                i32.const 8
                i32.shl
                local.get 3
                i32.load8_u offset=1
                i32.or
                local.set 4
                local.get 3
                i32.const 2
                i32.add
                local.set 3
              end
              local.get 5
              local.get 4
              i32.sub
              local.tee 5
              i32.const 0
              i32.lt_s
              br_if 3 (;@2;)
              local.get 1
              i32.const 1
              i32.xor
              local.set 1
              local.get 3
              i32.const 1057755
              i32.ne
              br_if 0 (;@5;)
              br 3 (;@2;)
            end
          end
          i32.const 1054709
          i32.const 43
          i32.const 1056388
          call $_ZN4core9panicking5panic17heeaec3885c636092E
          unreachable
        end
        local.get 0
        i32.const 65535
        i32.and
        local.set 5
        i32.const 1056776
        local.set 3
        i32.const 1
        local.set 1
        loop  ;; label = @3
          local.get 3
          i32.const 1
          i32.add
          local.set 0
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.load8_u
              local.tee 4
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              local.tee 7
              i32.const 0
              i32.lt_s
              br_if 0 (;@5;)
              local.get 0
              local.set 3
              br 1 (;@4;)
            end
            local.get 0
            i32.const 1057085
            i32.eq
            br_if 3 (;@1;)
            local.get 7
            i32.const 127
            i32.and
            i32.const 8
            i32.shl
            local.get 3
            i32.load8_u offset=1
            i32.or
            local.set 4
            local.get 3
            i32.const 2
            i32.add
            local.set 3
          end
          local.get 5
          local.get 4
          i32.sub
          local.tee 5
          i32.const 0
          i32.lt_s
          br_if 1 (;@2;)
          local.get 1
          i32.const 1
          i32.xor
          local.set 1
          local.get 3
          i32.const 1057085
          i32.ne
          br_if 0 (;@3;)
        end
      end
      local.get 1
      i32.const 1
      i32.and
      return
    end
    i32.const 1054709
    i32.const 43
    i32.const 1056388
    call $_ZN4core9panicking5panic17heeaec3885c636092E
    unreachable)
  (func $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hce18dc51dfa14637E (type 8) (param i32 i32 i32) (result i32)
    local.get 2
    local.get 0
    local.get 1
    call $_ZN4core3fmt9Formatter3pad17h637274232327cb71E)
  (func $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hfbd6052127c038a2E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i64 i32)
    i32.const 1
    local.set 2
    block  ;; label = @1
      local.get 1
      i32.load offset=24
      i32.const 39
      local.get 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=16
      call_indirect (type 3)
      br_if 0 (;@1;)
      i32.const 116
      local.set 3
      i32.const 2
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 0
                  i32.load
                  local.tee 0
                  i32.const -9
                  i32.add
                  br_table 5 (;@2;) 1 (;@6;) 3 (;@4;) 3 (;@4;) 0 (;@7;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 4 (;@3;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 3 (;@4;) 4 (;@3;) 2 (;@5;)
                end
                i32.const 114
                local.set 3
                i32.const 2
                local.set 4
                br 4 (;@2;)
              end
              i32.const 110
              local.set 3
              i32.const 2
              local.set 4
              br 3 (;@2;)
            end
            local.get 0
            i32.const 92
            i32.eq
            br_if 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h29890100314325f2E
                i32.eqz
                br_if 0 (;@6;)
                local.get 0
                i32.const 1
                i32.or
                i32.clz
                i32.const 2
                i32.shr_u
                i32.const 7
                i32.xor
                i64.extend_i32_u
                i64.const 21474836480
                i64.or
                local.set 5
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 0
                call $_ZN4core7unicode9printable12is_printable17hbb0b3da4690fa839E
                i32.eqz
                br_if 0 (;@6;)
                i32.const 1
                local.set 4
                br 2 (;@4;)
              end
              local.get 0
              i32.const 1
              i32.or
              i32.clz
              i32.const 2
              i32.shr_u
              i32.const 7
              i32.xor
              i64.extend_i32_u
              i64.const 21474836480
              i64.or
              local.set 5
            end
            i32.const 3
            local.set 4
          end
          local.get 0
          local.set 3
          br 1 (;@2;)
        end
        local.get 0
        local.set 3
        i32.const 2
        local.set 4
      end
      loop  ;; label = @2
        local.get 4
        local.set 6
        i32.const 92
        local.set 0
        i32.const 1
        local.set 2
        i32.const 1
        local.set 4
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 6
                    br_table 2 (;@6;) 1 (;@7;) 5 (;@3;) 0 (;@8;) 2 (;@6;)
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 5
                          i64.const 32
                          i64.shr_u
                          i32.wrap_i64
                          i32.const 255
                          i32.and
                          br_table 5 (;@6;) 3 (;@8;) 2 (;@9;) 1 (;@10;) 0 (;@11;) 6 (;@5;) 5 (;@6;)
                        end
                        local.get 5
                        i64.const -1095216660481
                        i64.and
                        i64.const 12884901888
                        i64.or
                        local.set 5
                        i32.const 117
                        local.set 0
                        br 6 (;@4;)
                      end
                      local.get 5
                      i64.const -1095216660481
                      i64.and
                      i64.const 8589934592
                      i64.or
                      local.set 5
                      i32.const 123
                      local.set 0
                      br 5 (;@4;)
                    end
                    i32.const 48
                    i32.const 87
                    local.get 3
                    local.get 5
                    i32.wrap_i64
                    local.tee 4
                    i32.const 2
                    i32.shl
                    i32.const 28
                    i32.and
                    i32.shr_u
                    i32.const 15
                    i32.and
                    local.tee 0
                    i32.const 10
                    i32.lt_u
                    select
                    local.get 0
                    i32.add
                    local.set 0
                    block  ;; label = @9
                      local.get 4
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 5
                      i64.const -1
                      i64.add
                      i64.const 4294967295
                      i64.and
                      local.get 5
                      i64.const -4294967296
                      i64.and
                      i64.or
                      local.set 5
                      br 5 (;@4;)
                    end
                    local.get 5
                    i64.const -1095216660481
                    i64.and
                    i64.const 4294967296
                    i64.or
                    local.set 5
                    br 4 (;@4;)
                  end
                  local.get 5
                  i64.const -1095216660481
                  i64.and
                  local.set 5
                  i32.const 125
                  local.set 0
                  br 3 (;@4;)
                end
                i32.const 0
                local.set 4
                local.get 3
                local.set 0
                br 3 (;@3;)
              end
              local.get 1
              i32.load offset=24
              i32.const 39
              local.get 1
              i32.load offset=28
              i32.load offset=16
              call_indirect (type 3)
              return
            end
            local.get 5
            i64.const -1095216660481
            i64.and
            i64.const 17179869184
            i64.or
            local.set 5
          end
          i32.const 3
          local.set 4
        end
        local.get 1
        i32.load offset=24
        local.get 0
        local.get 1
        i32.load offset=28
        i32.load offset=16
        call_indirect (type 3)
        i32.eqz
        br_if 0 (;@2;)
      end
    end
    local.get 2)
  (func $_ZN4core5slice6memchr7memrchr17hb78711c2471586dcE (type 4) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    local.get 3
    i32.const 0
    local.get 3
    i32.const 0
    local.get 2
    i32.sub
    i32.const 3
    i32.and
    local.tee 4
    i32.sub
    i32.const 7
    i32.and
    local.get 3
    local.get 4
    i32.lt_u
    local.tee 5
    select
    local.tee 6
    i32.sub
    local.set 7
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            local.get 6
            i32.lt_u
            br_if 0 (;@4;)
            local.get 3
            local.get 4
            local.get 5
            select
            local.set 8
            local.get 2
            local.get 7
            i32.add
            local.get 2
            local.get 3
            i32.add
            local.tee 4
            i32.sub
            local.set 5
            local.get 4
            i32.const -1
            i32.add
            local.set 4
            local.get 1
            i32.const 255
            i32.and
            local.set 9
            block  ;; label = @5
              loop  ;; label = @6
                local.get 6
                i32.eqz
                br_if 1 (;@5;)
                local.get 5
                i32.const 1
                i32.add
                local.set 5
                local.get 6
                i32.const -1
                i32.add
                local.set 6
                local.get 4
                i32.load8_u
                local.set 10
                local.get 4
                i32.const -1
                i32.add
                local.set 4
                local.get 10
                local.get 9
                i32.ne
                br_if 0 (;@6;)
              end
              local.get 7
              local.get 5
              i32.sub
              local.set 6
              br 3 (;@2;)
            end
            local.get 1
            i32.const 255
            i32.and
            i32.const 16843009
            i32.mul
            local.set 4
            block  ;; label = @5
              loop  ;; label = @6
                local.get 7
                local.tee 6
                local.get 8
                i32.le_u
                br_if 1 (;@5;)
                local.get 6
                i32.const -8
                i32.add
                local.set 7
                local.get 2
                local.get 6
                i32.add
                local.tee 5
                i32.const -4
                i32.add
                i32.load
                local.get 4
                i32.xor
                local.tee 10
                i32.const -1
                i32.xor
                local.get 10
                i32.const -16843009
                i32.add
                i32.and
                local.get 5
                i32.const -8
                i32.add
                i32.load
                local.get 4
                i32.xor
                local.tee 5
                i32.const -1
                i32.xor
                local.get 5
                i32.const -16843009
                i32.add
                i32.and
                i32.or
                i32.const -2139062144
                i32.and
                i32.eqz
                br_if 0 (;@6;)
              end
            end
            local.get 6
            local.get 3
            i32.gt_u
            br_if 1 (;@3;)
            local.get 2
            i32.const -1
            i32.add
            local.set 5
            local.get 1
            i32.const 255
            i32.and
            local.set 10
            loop  ;; label = @5
              block  ;; label = @6
                local.get 6
                br_if 0 (;@6;)
                i32.const 0
                local.set 4
                br 5 (;@1;)
              end
              local.get 5
              local.get 6
              i32.add
              local.set 4
              local.get 6
              i32.const -1
              i32.add
              local.set 6
              local.get 4
              i32.load8_u
              local.get 10
              i32.eq
              br_if 3 (;@2;)
              br 0 (;@5;)
            end
          end
          local.get 7
          local.get 3
          i32.const 1055432
          call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
          unreachable
        end
        local.get 6
        local.get 3
        i32.const 1055448
        call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
        unreachable
      end
      i32.const 1
      local.set 4
    end
    local.get 0
    local.get 6
    i32.store offset=4
    local.get 0
    local.get 4
    i32.store)
  (func $_ZN4core5slice25slice_index_overflow_fail17hc610e4c3b7a5b2c6E (type 1) (param i32)
    i32.const 1055616
    i32.const 44
    local.get 0
    call $_ZN4core9panicking5panic17heeaec3885c636092E
    unreachable)
  (func $_ZN4core3str5lossy9Utf8Lossy10from_bytes17hc86396f264469a03E (type 7) (param i32 i32 i32)
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN4core3str5lossy9Utf8Lossy6chunks17hc470c0fce5aec5cfE (type 7) (param i32 i32 i32)
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17ha10c9da4cfe4c60bE (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      local.get 1
      i32.load offset=4
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.load
      local.set 3
      i32.const 0
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        loop  ;; label = @19
                                          local.get 4
                                          i32.const 1
                                          i32.add
                                          local.set 5
                                          block  ;; label = @20
                                            block  ;; label = @21
                                              local.get 3
                                              local.get 4
                                              i32.add
                                              local.tee 6
                                              i32.load8_u
                                              local.tee 7
                                              i32.const 24
                                              i32.shl
                                              i32.const 24
                                              i32.shr_s
                                              local.tee 8
                                              i32.const -1
                                              i32.le_s
                                              br_if 0 (;@21;)
                                              local.get 5
                                              local.set 4
                                              br 1 (;@20;)
                                            end
                                            block  ;; label = @21
                                              block  ;; label = @22
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    local.get 7
                                                    i32.const 1055852
                                                    i32.add
                                                    i32.load8_u
                                                    i32.const -2
                                                    i32.add
                                                    br_table 1 (;@23;) 2 (;@22;) 3 (;@21;) 0 (;@24;)
                                                  end
                                                  local.get 2
                                                  local.get 4
                                                  i32.lt_u
                                                  br_if 7 (;@16;)
                                                  local.get 2
                                                  local.get 4
                                                  i32.le_u
                                                  br_if 8 (;@15;)
                                                  local.get 0
                                                  local.get 4
                                                  i32.store offset=4
                                                  local.get 0
                                                  local.get 3
                                                  i32.store
                                                  local.get 1
                                                  local.get 2
                                                  local.get 5
                                                  i32.sub
                                                  i32.store offset=4
                                                  local.get 1
                                                  local.get 3
                                                  local.get 5
                                                  i32.add
                                                  i32.store
                                                  local.get 0
                                                  i32.const 12
                                                  i32.add
                                                  i32.const 1
                                                  i32.store
                                                  local.get 0
                                                  i32.const 8
                                                  i32.add
                                                  local.get 6
                                                  i32.store
                                                  return
                                                end
                                                block  ;; label = @23
                                                  local.get 3
                                                  local.get 5
                                                  i32.add
                                                  local.tee 8
                                                  i32.const 0
                                                  local.get 2
                                                  local.get 5
                                                  i32.gt_u
                                                  select
                                                  local.tee 7
                                                  i32.const 1054673
                                                  local.get 7
                                                  select
                                                  i32.load8_u
                                                  i32.const 192
                                                  i32.and
                                                  i32.const 128
                                                  i32.ne
                                                  br_if 0 (;@23;)
                                                  local.get 4
                                                  i32.const 2
                                                  i32.add
                                                  local.set 4
                                                  br 3 (;@20;)
                                                end
                                                local.get 2
                                                local.get 4
                                                i32.lt_u
                                                br_if 8 (;@14;)
                                                local.get 2
                                                local.get 4
                                                i32.le_u
                                                br_if 9 (;@13;)
                                                local.get 1
                                                local.get 8
                                                i32.store
                                                local.get 0
                                                local.get 4
                                                i32.store offset=4
                                                local.get 0
                                                local.get 3
                                                i32.store
                                                local.get 1
                                                local.get 2
                                                local.get 5
                                                i32.sub
                                                i32.store offset=4
                                                local.get 0
                                                i32.const 12
                                                i32.add
                                                i32.const 1
                                                i32.store
                                                local.get 0
                                                i32.const 8
                                                i32.add
                                                local.get 6
                                                i32.store
                                                return
                                              end
                                              local.get 3
                                              local.get 5
                                              i32.add
                                              local.tee 9
                                              i32.const 0
                                              local.get 2
                                              local.get 5
                                              i32.gt_u
                                              select
                                              local.tee 10
                                              i32.const 1054673
                                              local.get 10
                                              select
                                              i32.load8_u
                                              local.set 10
                                              block  ;; label = @22
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    block  ;; label = @25
                                                      local.get 7
                                                      i32.const -224
                                                      i32.add
                                                      br_table 0 (;@25;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 2 (;@23;) 1 (;@24;) 2 (;@23;)
                                                    end
                                                    local.get 10
                                                    i32.const 224
                                                    i32.and
                                                    i32.const 160
                                                    i32.eq
                                                    br_if 2 (;@22;)
                                                    br 22 (;@2;)
                                                  end
                                                  local.get 10
                                                  i32.const 24
                                                  i32.shl
                                                  i32.const 24
                                                  i32.shr_s
                                                  i32.const -1
                                                  i32.gt_s
                                                  br_if 21 (;@2;)
                                                  local.get 10
                                                  i32.const 255
                                                  i32.and
                                                  i32.const 160
                                                  i32.ge_u
                                                  br_if 21 (;@2;)
                                                  br 1 (;@22;)
                                                end
                                                block  ;; label = @23
                                                  local.get 8
                                                  i32.const 31
                                                  i32.add
                                                  i32.const 255
                                                  i32.and
                                                  i32.const 11
                                                  i32.gt_u
                                                  br_if 0 (;@23;)
                                                  local.get 10
                                                  i32.const 24
                                                  i32.shl
                                                  i32.const 24
                                                  i32.shr_s
                                                  i32.const -1
                                                  i32.gt_s
                                                  br_if 21 (;@2;)
                                                  local.get 10
                                                  i32.const 255
                                                  i32.and
                                                  i32.const 192
                                                  i32.ge_u
                                                  br_if 21 (;@2;)
                                                  br 1 (;@22;)
                                                end
                                                local.get 10
                                                i32.const 255
                                                i32.and
                                                i32.const 191
                                                i32.gt_u
                                                br_if 20 (;@2;)
                                                local.get 8
                                                i32.const 254
                                                i32.and
                                                i32.const 238
                                                i32.ne
                                                br_if 20 (;@2;)
                                                local.get 10
                                                i32.const 24
                                                i32.shl
                                                i32.const 24
                                                i32.shr_s
                                                i32.const -1
                                                i32.gt_s
                                                br_if 20 (;@2;)
                                              end
                                              block  ;; label = @22
                                                local.get 3
                                                local.get 4
                                                i32.const 2
                                                i32.add
                                                local.tee 5
                                                i32.add
                                                local.tee 8
                                                i32.const 0
                                                local.get 2
                                                local.get 5
                                                i32.gt_u
                                                select
                                                local.tee 7
                                                i32.const 1054673
                                                local.get 7
                                                select
                                                i32.load8_u
                                                i32.const 192
                                                i32.and
                                                i32.const 128
                                                i32.ne
                                                br_if 0 (;@22;)
                                                local.get 4
                                                i32.const 3
                                                i32.add
                                                local.set 4
                                                br 2 (;@20;)
                                              end
                                              local.get 2
                                              local.get 4
                                              i32.lt_u
                                              br_if 9 (;@12;)
                                              local.get 4
                                              i32.const -3
                                              i32.gt_u
                                              br_if 10 (;@11;)
                                              local.get 2
                                              local.get 5
                                              i32.lt_u
                                              br_if 11 (;@10;)
                                              local.get 1
                                              local.get 8
                                              i32.store
                                              local.get 0
                                              local.get 4
                                              i32.store offset=4
                                              local.get 0
                                              local.get 3
                                              i32.store
                                              local.get 1
                                              local.get 2
                                              local.get 5
                                              i32.sub
                                              i32.store offset=4
                                              local.get 0
                                              i32.const 12
                                              i32.add
                                              i32.const 2
                                              i32.store
                                              local.get 0
                                              i32.const 8
                                              i32.add
                                              local.get 6
                                              i32.store
                                              return
                                            end
                                            local.get 3
                                            local.get 5
                                            i32.add
                                            local.tee 9
                                            i32.const 0
                                            local.get 2
                                            local.get 5
                                            i32.gt_u
                                            select
                                            local.tee 10
                                            i32.const 1054673
                                            local.get 10
                                            select
                                            i32.load8_u
                                            local.set 10
                                            block  ;; label = @21
                                              block  ;; label = @22
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    local.get 7
                                                    i32.const -240
                                                    i32.add
                                                    br_table 0 (;@24;) 2 (;@22;) 2 (;@22;) 2 (;@22;) 1 (;@23;) 2 (;@22;)
                                                  end
                                                  local.get 10
                                                  i32.const 112
                                                  i32.add
                                                  i32.const 255
                                                  i32.and
                                                  i32.const 48
                                                  i32.lt_u
                                                  br_if 2 (;@21;)
                                                  br 20 (;@3;)
                                                end
                                                local.get 10
                                                i32.const 24
                                                i32.shl
                                                i32.const 24
                                                i32.shr_s
                                                i32.const -1
                                                i32.gt_s
                                                br_if 19 (;@3;)
                                                local.get 10
                                                i32.const 255
                                                i32.and
                                                i32.const 144
                                                i32.ge_u
                                                br_if 19 (;@3;)
                                                br 1 (;@21;)
                                              end
                                              local.get 10
                                              i32.const 255
                                              i32.and
                                              i32.const 191
                                              i32.gt_u
                                              br_if 18 (;@3;)
                                              local.get 8
                                              i32.const 15
                                              i32.add
                                              i32.const 255
                                              i32.and
                                              i32.const 2
                                              i32.gt_u
                                              br_if 18 (;@3;)
                                              local.get 10
                                              i32.const 24
                                              i32.shl
                                              i32.const 24
                                              i32.shr_s
                                              i32.const -1
                                              i32.gt_s
                                              br_if 18 (;@3;)
                                            end
                                            local.get 3
                                            local.get 4
                                            i32.const 2
                                            i32.add
                                            local.tee 5
                                            i32.add
                                            local.tee 8
                                            i32.const 0
                                            local.get 2
                                            local.get 5
                                            i32.gt_u
                                            select
                                            local.tee 7
                                            i32.const 1054673
                                            local.get 7
                                            select
                                            i32.load8_u
                                            i32.const 192
                                            i32.and
                                            i32.const 128
                                            i32.ne
                                            br_if 2 (;@18;)
                                            local.get 3
                                            local.get 4
                                            i32.const 3
                                            i32.add
                                            local.tee 5
                                            i32.add
                                            local.tee 8
                                            i32.const 0
                                            local.get 2
                                            local.get 5
                                            i32.gt_u
                                            select
                                            local.tee 7
                                            i32.const 1054673
                                            local.get 7
                                            select
                                            i32.load8_u
                                            i32.const 192
                                            i32.and
                                            i32.const 128
                                            i32.ne
                                            br_if 3 (;@17;)
                                            local.get 4
                                            i32.const 4
                                            i32.add
                                            local.set 4
                                          end
                                          local.get 4
                                          local.get 2
                                          i32.lt_u
                                          br_if 0 (;@19;)
                                        end
                                        local.get 1
                                        i32.const 0
                                        i32.store offset=4
                                        local.get 1
                                        i32.const 1054672
                                        i32.store
                                        local.get 0
                                        local.get 2
                                        i32.store offset=4
                                        local.get 0
                                        local.get 3
                                        i32.store
                                        local.get 0
                                        i32.const 12
                                        i32.add
                                        i32.const 0
                                        i32.store
                                        local.get 0
                                        i32.const 8
                                        i32.add
                                        i32.const 1054672
                                        i32.store
                                        return
                                      end
                                      local.get 2
                                      local.get 4
                                      i32.lt_u
                                      br_if 8 (;@9;)
                                      local.get 4
                                      i32.const -3
                                      i32.gt_u
                                      br_if 9 (;@8;)
                                      local.get 2
                                      local.get 5
                                      i32.lt_u
                                      br_if 10 (;@7;)
                                      local.get 1
                                      local.get 8
                                      i32.store
                                      local.get 0
                                      local.get 4
                                      i32.store offset=4
                                      local.get 0
                                      local.get 3
                                      i32.store
                                      local.get 1
                                      local.get 2
                                      local.get 5
                                      i32.sub
                                      i32.store offset=4
                                      local.get 0
                                      i32.const 12
                                      i32.add
                                      i32.const 2
                                      i32.store
                                      local.get 0
                                      i32.const 8
                                      i32.add
                                      local.get 6
                                      i32.store
                                      return
                                    end
                                    local.get 2
                                    local.get 4
                                    i32.lt_u
                                    br_if 10 (;@6;)
                                    local.get 4
                                    i32.const -4
                                    i32.gt_u
                                    br_if 11 (;@5;)
                                    local.get 2
                                    local.get 5
                                    i32.lt_u
                                    br_if 12 (;@4;)
                                    local.get 1
                                    local.get 8
                                    i32.store
                                    local.get 0
                                    local.get 4
                                    i32.store offset=4
                                    local.get 0
                                    local.get 3
                                    i32.store
                                    local.get 1
                                    local.get 2
                                    local.get 5
                                    i32.sub
                                    i32.store offset=4
                                    local.get 0
                                    i32.const 12
                                    i32.add
                                    i32.const 3
                                    i32.store
                                    local.get 0
                                    i32.const 8
                                    i32.add
                                    local.get 6
                                    i32.store
                                    return
                                  end
                                  local.get 4
                                  local.get 2
                                  i32.const 1055740
                                  call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                                  unreachable
                                end
                                local.get 5
                                local.get 2
                                i32.const 1055740
                                call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                                unreachable
                              end
                              local.get 4
                              local.get 2
                              i32.const 1055836
                              call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                              unreachable
                            end
                            local.get 5
                            local.get 2
                            i32.const 1055836
                            call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                            unreachable
                          end
                          local.get 4
                          local.get 2
                          i32.const 1055804
                          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                          unreachable
                        end
                        local.get 4
                        local.get 5
                        i32.const 1055804
                        call $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E
                        unreachable
                      end
                      local.get 5
                      local.get 2
                      i32.const 1055804
                      call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                      unreachable
                    end
                    local.get 4
                    local.get 2
                    i32.const 1055756
                    call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                    unreachable
                  end
                  local.get 4
                  local.get 5
                  i32.const 1055756
                  call $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E
                  unreachable
                end
                local.get 5
                local.get 2
                i32.const 1055756
                call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
                unreachable
              end
              local.get 4
              local.get 2
              i32.const 1055772
              call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
              unreachable
            end
            local.get 4
            local.get 5
            i32.const 1055772
            call $_ZN4core5slice22slice_index_order_fail17h211d7c73488e5e66E
            unreachable
          end
          local.get 5
          local.get 2
          i32.const 1055772
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            local.get 4
            i32.lt_u
            br_if 0 (;@4;)
            local.get 2
            local.get 4
            i32.le_u
            br_if 1 (;@3;)
            local.get 1
            local.get 9
            i32.store
            local.get 0
            local.get 4
            i32.store offset=4
            local.get 0
            local.get 3
            i32.store
            local.get 1
            local.get 2
            local.get 5
            i32.sub
            i32.store offset=4
            local.get 0
            i32.const 12
            i32.add
            i32.const 1
            i32.store
            local.get 0
            i32.const 8
            i32.add
            local.get 6
            i32.store
            return
          end
          local.get 4
          local.get 2
          i32.const 1055788
          call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
          unreachable
        end
        local.get 5
        local.get 2
        i32.const 1055788
        call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
        unreachable
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          local.get 4
          i32.lt_u
          br_if 0 (;@3;)
          local.get 2
          local.get 4
          i32.le_u
          br_if 1 (;@2;)
          local.get 1
          local.get 9
          i32.store
          local.get 0
          local.get 4
          i32.store offset=4
          local.get 0
          local.get 3
          i32.store
          local.get 1
          local.get 2
          local.get 5
          i32.sub
          i32.store offset=4
          local.get 0
          i32.const 12
          i32.add
          i32.const 1
          i32.store
          local.get 0
          i32.const 8
          i32.add
          local.get 6
          i32.store
          return
        end
        local.get 4
        local.get 2
        i32.const 1055820
        call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
        unreachable
      end
      local.get 5
      local.get 2
      i32.const 1055820
      call $_ZN4core5slice24slice_end_index_len_fail17h4c0bfee37c1e1065E
      unreachable
    end
    local.get 0
    i32.const 0
    i32.store)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i8$GT$3fmt17hfd3f164dab1c4039E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load8_u
    local.set 3
    i32.const 0
    local.set 0
    loop  ;; label = @1
      local.get 2
      local.get 0
      i32.add
      i32.const 127
      i32.add
      local.get 3
      i32.const 15
      i32.and
      local.tee 4
      i32.const 48
      i32.or
      local.get 4
      i32.const 87
      i32.add
      local.get 4
      i32.const 10
      i32.lt_u
      select
      i32.store8
      local.get 0
      i32.const -1
      i32.add
      local.set 0
      local.get 3
      i32.const 4
      i32.shr_u
      i32.const 15
      i32.and
      local.tee 3
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 0
      i32.const 128
      i32.add
      local.tee 3
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055064
    i32.const 2
    local.get 2
    local.get 0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3str9from_utf817haf80b3a8845ce8ecE (type 7) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i64 i64)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          local.get 2
          i32.const -7
          i32.add
          local.tee 3
          local.get 3
          local.get 2
          i32.gt_u
          select
          local.set 4
          i32.const 0
          local.set 3
          loop  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 1
                      local.get 3
                      i32.add
                      local.tee 5
                      i32.load8_u
                      local.tee 6
                      i32.const 24
                      i32.shl
                      i32.const 24
                      i32.shr_s
                      local.tee 7
                      i32.const -1
                      i32.gt_s
                      br_if 0 (;@9;)
                      i64.const 1099511627776
                      local.set 8
                      i64.const 4294967296
                      local.set 9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 6
                            i32.const 1055852
                            i32.add
                            i32.load8_u
                            i32.const -2
                            i32.add
                            br_table 0 (;@12;) 1 (;@11;) 2 (;@10;) 11 (;@1;)
                          end
                          local.get 3
                          i32.const 1
                          i32.add
                          local.tee 5
                          local.get 2
                          i32.lt_u
                          br_if 3 (;@8;)
                          i64.const 0
                          local.set 9
                          i64.const 0
                          local.set 8
                          br 10 (;@1;)
                        end
                        i64.const 0
                        local.set 9
                        block  ;; label = @11
                          local.get 3
                          i32.const 1
                          i32.add
                          local.tee 5
                          local.get 2
                          i32.lt_u
                          br_if 0 (;@11;)
                          i64.const 0
                          local.set 8
                          br 10 (;@1;)
                        end
                        local.get 1
                        local.get 5
                        i32.add
                        i32.load8_u
                        local.set 5
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                local.get 6
                                i32.const -224
                                i32.add
                                br_table 0 (;@14;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 2 (;@12;) 1 (;@13;) 2 (;@12;)
                              end
                              local.get 5
                              i32.const 224
                              i32.and
                              i32.const 160
                              i32.eq
                              br_if 2 (;@11;)
                              i64.const 4294967296
                              local.set 9
                              br 12 (;@1;)
                            end
                            block  ;; label = @13
                              local.get 5
                              i32.const 24
                              i32.shl
                              i32.const 24
                              i32.shr_s
                              i32.const -1
                              i32.le_s
                              br_if 0 (;@13;)
                              i64.const 4294967296
                              local.set 9
                              br 12 (;@1;)
                            end
                            local.get 5
                            i32.const 255
                            i32.and
                            i32.const 160
                            i32.lt_u
                            br_if 1 (;@11;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          block  ;; label = @12
                            local.get 7
                            i32.const 31
                            i32.add
                            i32.const 255
                            i32.and
                            i32.const 11
                            i32.gt_u
                            br_if 0 (;@12;)
                            block  ;; label = @13
                              local.get 5
                              i32.const 24
                              i32.shl
                              i32.const 24
                              i32.shr_s
                              i32.const -1
                              i32.le_s
                              br_if 0 (;@13;)
                              i64.const 4294967296
                              local.set 9
                              br 12 (;@1;)
                            end
                            local.get 5
                            i32.const 255
                            i32.and
                            i32.const 192
                            i32.lt_u
                            br_if 1 (;@11;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          block  ;; label = @12
                            local.get 5
                            i32.const 255
                            i32.and
                            i32.const 191
                            i32.le_u
                            br_if 0 (;@12;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          block  ;; label = @12
                            local.get 7
                            i32.const 254
                            i32.and
                            i32.const 238
                            i32.eq
                            br_if 0 (;@12;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          local.get 5
                          i32.const 24
                          i32.shl
                          i32.const 24
                          i32.shr_s
                          i32.const -1
                          i32.le_s
                          br_if 0 (;@11;)
                          i64.const 4294967296
                          local.set 9
                          br 10 (;@1;)
                        end
                        i64.const 0
                        local.set 8
                        local.get 3
                        i32.const 2
                        i32.add
                        local.tee 5
                        local.get 2
                        i32.ge_u
                        br_if 9 (;@1;)
                        local.get 1
                        local.get 5
                        i32.add
                        i32.load8_u
                        i32.const 192
                        i32.and
                        i32.const 128
                        i32.eq
                        br_if 4 (;@6;)
                        br 8 (;@2;)
                      end
                      i64.const 0
                      local.set 9
                      block  ;; label = @10
                        local.get 3
                        i32.const 1
                        i32.add
                        local.tee 5
                        local.get 2
                        i32.lt_u
                        br_if 0 (;@10;)
                        i64.const 0
                        local.set 8
                        br 9 (;@1;)
                      end
                      local.get 1
                      local.get 5
                      i32.add
                      i32.load8_u
                      local.set 5
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 6
                              i32.const -240
                              i32.add
                              br_table 0 (;@13;) 2 (;@11;) 2 (;@11;) 2 (;@11;) 1 (;@12;) 2 (;@11;)
                            end
                            local.get 5
                            i32.const 112
                            i32.add
                            i32.const 255
                            i32.and
                            i32.const 48
                            i32.lt_u
                            br_if 2 (;@10;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          block  ;; label = @12
                            local.get 5
                            i32.const 24
                            i32.shl
                            i32.const 24
                            i32.shr_s
                            i32.const -1
                            i32.le_s
                            br_if 0 (;@12;)
                            i64.const 4294967296
                            local.set 9
                            br 11 (;@1;)
                          end
                          local.get 5
                          i32.const 255
                          i32.and
                          i32.const 144
                          i32.lt_u
                          br_if 1 (;@10;)
                          i64.const 4294967296
                          local.set 9
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          local.get 5
                          i32.const 255
                          i32.and
                          i32.const 191
                          i32.le_u
                          br_if 0 (;@11;)
                          i64.const 4294967296
                          local.set 9
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          local.get 7
                          i32.const 15
                          i32.add
                          i32.const 255
                          i32.and
                          i32.const 2
                          i32.le_u
                          br_if 0 (;@11;)
                          i64.const 4294967296
                          local.set 9
                          br 10 (;@1;)
                        end
                        local.get 5
                        i32.const 24
                        i32.shl
                        i32.const 24
                        i32.shr_s
                        i32.const -1
                        i32.le_s
                        br_if 0 (;@10;)
                        i64.const 4294967296
                        local.set 9
                        br 9 (;@1;)
                      end
                      block  ;; label = @10
                        local.get 3
                        i32.const 2
                        i32.add
                        local.tee 5
                        local.get 2
                        i32.lt_u
                        br_if 0 (;@10;)
                        i64.const 0
                        local.set 8
                        br 9 (;@1;)
                      end
                      local.get 1
                      local.get 5
                      i32.add
                      i32.load8_u
                      i32.const 192
                      i32.and
                      i32.const 128
                      i32.ne
                      br_if 7 (;@2;)
                      i64.const 0
                      local.set 8
                      local.get 3
                      i32.const 3
                      i32.add
                      local.tee 5
                      local.get 2
                      i32.ge_u
                      br_if 8 (;@1;)
                      local.get 1
                      local.get 5
                      i32.add
                      i32.load8_u
                      i32.const 192
                      i32.and
                      i32.const 128
                      i32.eq
                      br_if 3 (;@6;)
                      i64.const 3298534883328
                      local.set 8
                      i64.const 4294967296
                      local.set 9
                      br 8 (;@1;)
                    end
                    i32.const 0
                    local.get 5
                    i32.sub
                    i32.const 3
                    i32.and
                    br_if 1 (;@7;)
                    block  ;; label = @9
                      local.get 3
                      local.get 4
                      i32.ge_u
                      br_if 0 (;@9;)
                      loop  ;; label = @10
                        local.get 1
                        local.get 3
                        i32.add
                        local.tee 5
                        i32.const 4
                        i32.add
                        i32.load
                        local.get 5
                        i32.load
                        i32.or
                        i32.const -2139062144
                        i32.and
                        br_if 1 (;@9;)
                        local.get 3
                        i32.const 8
                        i32.add
                        local.tee 3
                        local.get 4
                        i32.lt_u
                        br_if 0 (;@10;)
                      end
                    end
                    local.get 3
                    local.get 2
                    i32.ge_u
                    br_if 3 (;@5;)
                    loop  ;; label = @9
                      local.get 1
                      local.get 3
                      i32.add
                      i32.load8_s
                      i32.const 0
                      i32.lt_s
                      br_if 4 (;@5;)
                      local.get 2
                      local.get 3
                      i32.const 1
                      i32.add
                      local.tee 3
                      i32.ne
                      br_if 0 (;@9;)
                      br 6 (;@3;)
                    end
                  end
                  i64.const 4294967296
                  local.set 9
                  local.get 1
                  local.get 5
                  i32.add
                  i32.load8_u
                  i32.const 192
                  i32.and
                  i32.const 128
                  i32.eq
                  br_if 1 (;@6;)
                  br 6 (;@1;)
                end
                local.get 3
                i32.const 1
                i32.add
                local.set 3
                br 1 (;@5;)
              end
              local.get 5
              i32.const 1
              i32.add
              local.set 3
            end
            local.get 3
            local.get 2
            i32.lt_u
            br_if 0 (;@4;)
          end
        end
        local.get 0
        local.get 1
        i32.store offset=4
        local.get 0
        i32.const 8
        i32.add
        local.get 2
        i32.store
        local.get 0
        i32.const 0
        i32.store
        return
      end
      i64.const 2199023255552
      local.set 8
      i64.const 4294967296
      local.set 9
    end
    local.get 0
    local.get 9
    local.get 3
    i64.extend_i32_u
    i64.or
    local.get 8
    i64.or
    i64.store offset=4 align=4
    local.get 0
    i32.const 1
    i32.store)
  (func $_ZN4core3fmt3num3imp51_$LT$impl$u20$core..fmt..Display$u20$for$u20$u8$GT$3fmt17h891c553833cf799dE (type 3) (param i32 i32) (result i32)
    local.get 0
    i64.load8_u
    i32.const 1
    local.get 1
    call $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17haf413b6a293768dfE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.set 3
    i32.const 0
    local.set 0
    loop  ;; label = @1
      local.get 2
      local.get 0
      i32.add
      i32.const 127
      i32.add
      local.get 3
      i32.const 15
      i32.and
      local.tee 4
      i32.const 48
      i32.or
      local.get 4
      i32.const 87
      i32.add
      local.get 4
      i32.const 10
      i32.lt_u
      select
      i32.store8
      local.get 0
      i32.const -1
      i32.add
      local.set 0
      local.get 3
      i32.const 4
      i32.shr_u
      local.tee 3
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 0
      i32.const 128
      i32.add
      local.tee 3
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055064
    i32.const 2
    local.get 2
    local.get 0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE (type 14) (param i64 i32 i32) (result i32)
    (local i32 i32 i64 i32 i32 i32)
    global.get 0
    i32.const 48
    i32.sub
    local.tee 3
    global.set 0
    i32.const 39
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i64.const 10000
        i64.ge_u
        br_if 0 (;@2;)
        local.get 0
        local.set 5
        br 1 (;@1;)
      end
      i32.const 39
      local.set 4
      loop  ;; label = @2
        local.get 3
        i32.const 9
        i32.add
        local.get 4
        i32.add
        local.tee 6
        i32.const -4
        i32.add
        local.get 0
        local.get 0
        i64.const 10000
        i64.div_u
        local.tee 5
        i64.const 10000
        i64.mul
        i64.sub
        i32.wrap_i64
        local.tee 7
        i32.const 65535
        i32.and
        i32.const 100
        i32.div_u
        local.tee 8
        i32.const 1
        i32.shl
        i32.const 1055066
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        local.get 6
        i32.const -2
        i32.add
        local.get 7
        local.get 8
        i32.const 100
        i32.mul
        i32.sub
        i32.const 65535
        i32.and
        i32.const 1
        i32.shl
        i32.const 1055066
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        local.get 4
        i32.const -4
        i32.add
        local.set 4
        local.get 0
        i64.const 99999999
        i64.gt_u
        local.set 6
        local.get 5
        local.set 0
        local.get 6
        br_if 0 (;@2;)
      end
    end
    block  ;; label = @1
      local.get 5
      i32.wrap_i64
      local.tee 6
      i32.const 99
      i32.le_s
      br_if 0 (;@1;)
      local.get 3
      i32.const 9
      i32.add
      local.get 4
      i32.const -2
      i32.add
      local.tee 4
      i32.add
      local.get 5
      i32.wrap_i64
      local.tee 6
      local.get 6
      i32.const 65535
      i32.and
      i32.const 100
      i32.div_u
      local.tee 6
      i32.const 100
      i32.mul
      i32.sub
      i32.const 65535
      i32.and
      i32.const 1
      i32.shl
      i32.const 1055066
      i32.add
      i32.load16_u align=1
      i32.store16 align=1
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 6
        i32.const 10
        i32.lt_s
        br_if 0 (;@2;)
        local.get 3
        i32.const 9
        i32.add
        local.get 4
        i32.const -2
        i32.add
        local.tee 4
        i32.add
        local.get 6
        i32.const 1
        i32.shl
        i32.const 1055066
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        br 1 (;@1;)
      end
      local.get 3
      i32.const 9
      i32.add
      local.get 4
      i32.const -1
      i32.add
      local.tee 4
      i32.add
      local.get 6
      i32.const 48
      i32.add
      i32.store8
    end
    local.get 2
    local.get 1
    i32.const 1054672
    i32.const 0
    local.get 3
    i32.const 9
    i32.add
    local.get 4
    i32.add
    i32.const 39
    local.get 4
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
    local.set 4
    local.get 3
    i32.const 48
    i32.add
    global.set 0
    local.get 4)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17h478aac1c493c7938E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load8_u
    local.set 3
    i32.const 0
    local.set 0
    loop  ;; label = @1
      local.get 2
      local.get 0
      i32.add
      i32.const 127
      i32.add
      local.get 3
      i32.const 15
      i32.and
      local.tee 4
      i32.const 48
      i32.or
      local.get 4
      i32.const 55
      i32.add
      local.get 4
      i32.const 10
      i32.lt_u
      select
      i32.store8
      local.get 0
      i32.const -1
      i32.add
      local.set 0
      local.get 3
      i32.const 4
      i32.shr_u
      i32.const 15
      i32.and
      local.tee 3
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 0
      i32.const 128
      i32.add
      local.tee 3
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055064
    i32.const 2
    local.get 2
    local.get 0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17h75226882cdaf372cE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.set 3
    i32.const 0
    local.set 0
    loop  ;; label = @1
      local.get 2
      local.get 0
      i32.add
      i32.const 127
      i32.add
      local.get 3
      i32.const 15
      i32.and
      local.tee 4
      i32.const 48
      i32.or
      local.get 4
      i32.const 55
      i32.add
      local.get 4
      i32.const 10
      i32.lt_u
      select
      i32.store8
      local.get 0
      i32.const -1
      i32.add
      local.set 0
      local.get 3
      i32.const 4
      i32.shr_u
      local.tee 3
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 0
      i32.const 128
      i32.add
      local.tee 3
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 3
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055064
    i32.const 2
    local.get 2
    local.get 0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set 0
    local.get 0)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h12c0d2e5b253da32E (type 3) (param i32 i32) (result i32)
    (local i64)
    local.get 0
    i32.load
    local.tee 0
    i64.extend_i32_s
    local.tee 2
    local.get 2
    i64.const 63
    i64.shr_s
    local.tee 2
    i64.add
    local.get 2
    i64.xor
    local.get 0
    i32.const -1
    i32.xor
    i32.const 31
    i32.shr_u
    local.get 1
    call $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h4a323aa73e9fedd6E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get 0
    i32.const 128
    i32.sub
    local.tee 2
    global.set 0
    local.get 0
    i32.load
    local.set 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load
              local.tee 3
              i32.const 16
              i32.and
              br_if 0 (;@5;)
              local.get 0
              i32.load8_u
              local.set 4
              local.get 3
              i32.const 32
              i32.and
              br_if 1 (;@4;)
              local.get 4
              i64.extend_i32_u
              i64.const 255
              i64.and
              i32.const 1
              local.get 1
              call $_ZN4core3fmt3num3imp7fmt_u6417he4ba77683052e3cdE
              local.set 0
              br 2 (;@3;)
            end
            local.get 0
            i32.load8_u
            local.set 4
            i32.const 0
            local.set 0
            loop  ;; label = @5
              local.get 2
              local.get 0
              i32.add
              i32.const 127
              i32.add
              local.get 4
              i32.const 15
              i32.and
              local.tee 3
              i32.const 48
              i32.or
              local.get 3
              i32.const 87
              i32.add
              local.get 3
              i32.const 10
              i32.lt_u
              select
              i32.store8
              local.get 0
              i32.const -1
              i32.add
              local.set 0
              local.get 4
              i32.const 4
              i32.shr_u
              i32.const 15
              i32.and
              local.tee 4
              br_if 0 (;@5;)
            end
            local.get 0
            i32.const 128
            i32.add
            local.tee 4
            i32.const 129
            i32.ge_u
            br_if 2 (;@2;)
            local.get 1
            i32.const 1
            i32.const 1055064
            i32.const 2
            local.get 2
            local.get 0
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get 0
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
            local.set 0
            br 1 (;@3;)
          end
          i32.const 0
          local.set 0
          loop  ;; label = @4
            local.get 2
            local.get 0
            i32.add
            i32.const 127
            i32.add
            local.get 4
            i32.const 15
            i32.and
            local.tee 3
            i32.const 48
            i32.or
            local.get 3
            i32.const 55
            i32.add
            local.get 3
            i32.const 10
            i32.lt_u
            select
            i32.store8
            local.get 0
            i32.const -1
            i32.add
            local.set 0
            local.get 4
            i32.const 4
            i32.shr_u
            i32.const 15
            i32.and
            local.tee 4
            br_if 0 (;@4;)
          end
          local.get 0
          i32.const 128
          i32.add
          local.tee 4
          i32.const 129
          i32.ge_u
          br_if 2 (;@1;)
          local.get 1
          i32.const 1
          i32.const 1055064
          i32.const 2
          local.get 2
          local.get 0
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get 0
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17hf4c4d5fe4c43c869E
          local.set 0
        end
        local.get 2
        i32.const 128
        i32.add
        global.set 0
        local.get 0
        return
      end
      local.get 4
      i32.const 128
      i32.const 1055048
      call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
      unreachable
    end
    local.get 4
    i32.const 128
    i32.const 1055048
    call $_ZN4core5slice26slice_start_index_len_fail17h40edbc71e8be8fa0E
    unreachable)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h6a9a6269c688a2c1E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 0
        i32.load8_u
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        local.get 1
        i32.load offset=24
        i32.const 1057860
        i32.const 4
        local.get 1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 8)
        i32.store8 offset=8
        local.get 2
        local.get 1
        i32.store
        local.get 2
        i32.const 0
        i32.store8 offset=9
        local.get 2
        i32.const 0
        i32.store offset=4
        local.get 2
        local.get 0
        i32.const 1
        i32.add
        i32.store offset=12
        local.get 2
        local.get 2
        i32.const 12
        i32.add
        i32.const 1055004
        call $_ZN4core3fmt8builders10DebugTuple5field17h0e16581ce3858b5aE
        drop
        local.get 2
        i32.load8_u offset=8
        local.set 1
        block  ;; label = @3
          local.get 2
          i32.load offset=4
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.const 255
          i32.and
          local.set 0
          i32.const 1
          local.set 1
          block  ;; label = @4
            local.get 0
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.const 1
              i32.ne
              br_if 0 (;@5;)
              local.get 2
              i32.load8_u offset=9
              i32.const 255
              i32.and
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.tee 0
              i32.load8_u
              i32.const 4
              i32.and
              br_if 0 (;@5;)
              i32.const 1
              local.set 1
              local.get 0
              i32.load offset=24
              i32.const 1055000
              i32.const 1
              local.get 0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect (type 8)
              br_if 1 (;@4;)
            end
            local.get 2
            i32.load
            local.tee 1
            i32.load offset=24
            i32.const 1055001
            i32.const 1
            local.get 1
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect (type 8)
            local.set 1
          end
          local.get 2
          local.get 1
          i32.store8 offset=8
        end
        local.get 1
        i32.const 255
        i32.and
        i32.const 0
        i32.ne
        local.set 1
        br 1 (;@1;)
      end
      local.get 1
      i32.load offset=24
      i32.const 1057864
      i32.const 4
      local.get 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type 8)
      local.set 1
    end
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hcd52b66f015e2ed4E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..Debug$u20$for$u20$usize$GT$3fmt17h9761667c77b91f77E)
  (func $_ZN57_$LT$core..str..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17hc735c0b415e135fbE (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get 0
    i32.const 16
    i32.sub
    local.tee 2
    global.set 0
    local.get 1
    i32.load offset=24
    i32.const 1057868
    i32.const 9
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 8)
    local.set 3
    local.get 2
    i32.const 0
    i32.store8 offset=5
    local.get 2
    local.get 3
    i32.store8 offset=4
    local.get 2
    local.get 1
    i32.store
    local.get 2
    local.get 0
    i32.store offset=12
    local.get 2
    i32.const 1057877
    i32.const 11
    local.get 2
    i32.const 12
    i32.add
    i32.const 1057844
    call $_ZN4core3fmt8builders11DebugStruct5field17h74e4a270a55c26e5E
    drop
    local.get 2
    local.get 0
    i32.const 4
    i32.add
    i32.store offset=12
    local.get 2
    i32.const 1057888
    i32.const 9
    local.get 2
    i32.const 12
    i32.add
    i32.const 1057900
    call $_ZN4core3fmt8builders11DebugStruct5field17h74e4a270a55c26e5E
    drop
    local.get 2
    i32.load8_u offset=4
    local.set 1
    block  ;; label = @1
      local.get 2
      i32.load8_u offset=5
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.const 255
      i32.and
      local.set 0
      i32.const 1
      local.set 1
      block  ;; label = @2
        local.get 0
        br_if 0 (;@2;)
        local.get 2
        i32.load
        local.tee 1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        local.set 0
        local.get 1
        i32.load offset=24
        local.set 3
        block  ;; label = @3
          local.get 1
          i32.load8_u
          i32.const 4
          i32.and
          br_if 0 (;@3;)
          local.get 3
          i32.const 1054995
          i32.const 2
          local.get 0
          call_indirect (type 8)
          local.set 1
          br 1 (;@2;)
        end
        local.get 3
        i32.const 1054994
        i32.const 1
        local.get 0
        call_indirect (type 8)
        local.set 1
      end
      local.get 2
      local.get 1
      i32.store8 offset=4
    end
    local.get 2
    i32.const 16
    i32.add
    global.set 0
    local.get 1
    i32.const 255
    i32.and
    i32.const 0
    i32.ne)
  (table (;0;) 97 97 funcref)
  (memory (;0;) 17)
  (global (;0;) (mut i32) (i32.const 1048576))
  (global (;1;) i32 (i32.const 1059400))
  (global (;2;) i32 (i32.const 1059400))
  (export "memory" (memory 0))
  (export "_start" (func $_start))
  (export "__original_main" (func $__original_main))
  (export "main" (func $main))
  (export "__data_end" (global 1))
  (export "__heap_base" (global 2))
  (elem (;0;) (i32.const 1) func $_ZN4core3ptr13drop_in_place17hde99e66a4e1519b0E $_ZN3std2rt10lang_start28_$u7b$$u7b$closure$u7d$$u7d$17h6c6012a6d56d2797E $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17haa826d9e64a47c7dE $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17hdd737011a291d185E $_ZN5hello4main17h1bdeb991053409b3E $_ZN4core3ptr13drop_in_place17ha9806990c369dcecE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h33d1e65ef0c901faE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h858d2d50d0fa93e7E $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h228b462170cfd3d4E $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h8c004d7930574d45E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hda98ecdf0621cbe4E $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h3cc362ab1df73571E $_ZN55_$LT$std..path..Display$u20$as$u20$core..fmt..Debug$GT$3fmt17h853c8bfc67334964E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h6a7113d5e1f92fc5E $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h12c0d2e5b253da32E $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17he76d4dd82f8e989eE $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17hc79ab98580260a0dE $_ZN3std5alloc24default_alloc_error_hook17h3c9cdc508dd65ebfE $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h2ffe9d8886f9157bE $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hc70038f3e0b70d11E $_ZN4core3ptr13drop_in_place17h01aa9f0df2a3d41bE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h839b33d7928966c7E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h99f2437e301ed84bE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h07cd168c2ba71c1aE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5a6e30684d6b4715E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h37a7d86d4a86eeb1E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h2383e42b263403e5E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17hfde7181541e0be0aE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h878a07cb96977ed8E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h9ca5c664748f585bE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h31b8a240b20122ecE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hfe13144b583fb519E $_ZN60_$LT$core..cell..BorrowError$u20$as$u20$core..fmt..Debug$GT$3fmt17h6b4af330c9438caaE $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h79591e9d2985bd22E $_ZN4core3ptr13drop_in_place17h722a6486d237df62E $_ZN62_$LT$std..ffi..c_str..NulError$u20$as$u20$core..fmt..Debug$GT$3fmt17he93c4a7fdb5537e7E $_ZN4core3ptr13drop_in_place17h43e1c981bf433273E $_ZN82_$LT$std..sys_common..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h442c1cc490bb7041E $_ZN4core3ptr13drop_in_place17h0efdcecac9224e6cE $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17hd52bd4df9fd4eb9eE $_ZN57_$LT$core..str..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17hc735c0b415e135fbE $_ZN3std5error5Error5cause17hb938bd2ebabb958dE $_ZN3std5error5Error7type_id17he6dcf01a791cbadbE $_ZN3std5error5Error9backtrace17h3b27bb4b1eb1fdc9E $_ZN243_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$std..error..Error$GT$11description17h8d8d7033eb4c2897E $_ZN244_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$core..fmt..Display$GT$3fmt17h19e8603e84100fa4E $_ZN242_$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Sync$u2b$core..marker..Send$GT$$GT$..from..StringError$u20$as$u20$core..fmt..Debug$GT$3fmt17h31e0ae2091b2beacE $_ZN4core3ptr13drop_in_place17h35bf666c94b799f0E $_ZN80_$LT$std..io..Write..write_fmt..Adaptor$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hab706aac4ccd318bE $_ZN4core3fmt5Write10write_char17h524458e201538bfcE $_ZN4core3fmt5Write9write_fmt17h0c8a6cfe66f8ee62E $_ZN80_$LT$std..io..Write..write_fmt..Adaptor$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb4871d4b0022c40fE $_ZN4core3fmt5Write10write_char17hae5a84beb8d1055fE $_ZN4core3fmt5Write9write_fmt17hee3d2ccd7ee9e67cE $_ZN3std4sync4once4Once9call_once28_$u7b$$u7b$closure$u7d$$u7d$17h707f5a9b7e72888fE $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h89bb37873cac3f06E $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17h8f451917953880c1E $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17hb8c1ea81fb56a93eE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17hd5dec2ceaf8b717dE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h3a56ad24f6b6aad4E $_ZN59_$LT$std..process..ChildStdin$u20$as$u20$std..io..Write$GT$5flush17h5817e30417877f3fE $_ZN3std2io5Write9write_all17h95d942a560570f98E $_ZN3std2io5Write18write_all_vectored17h22801b0a1a1e68b9E $_ZN3std2io5Write9write_fmt17h44e9ca74e44479e5E $_ZN4core3ptr13drop_in_place17he59ca02f76ff0db5E $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$5write17h8c2c31ee81abd4daE $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$14write_vectored17h5349500dd10151ecE $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$17is_write_vectored17h068b3aff18149e2dE $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$5flush17h5099216c82f203e7E $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$9write_all17h6c0f53b93c085afcE $_ZN3std2io5Write18write_all_vectored17hb3da9e95bb72fe29E $_ZN3std2io5impls71_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..boxed..Box$LT$W$GT$$GT$9write_fmt17hf7c0a8c6ed5ea434E $_ZN4core3ptr13drop_in_place17h461604a72e03a0f3E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h18a2bce81be58b94E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17he307532799611685E $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h572695cb9d46eea3E $_ZN91_$LT$std..panicking..begin_panic..PanicPayload$LT$A$GT$$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17hf2bd9ca407affc5aE $_ZN91_$LT$std..panicking..begin_panic..PanicPayload$LT$A$GT$$u20$as$u20$core..panic..BoxMeUp$GT$3get17h65f3fed9b8093d68E $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2c23cfb6ed5309fbE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17ha6ef0d9ad75832cdE $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17he5f79f23f9f461d5E $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17hc2205f114ba00674E $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hfbd6052127c038a2E $_ZN4core3ops8function6FnOnce9call_once17hc4bf69ce3fbd1528E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17habd6be6b60c616e8E $_ZN4core3ptr13drop_in_place17h021c8bbd1b3ab19eE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hdd3815e1338426f9E $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h8cc500c6210bd632E $_ZN4core3fmt5Write10write_char17h855ab183fb53bf2bE $_ZN4core3fmt5Write9write_fmt17hf56be3afcac3485dE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h4a323aa73e9fedd6E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h76664f8c3d7fdb62E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h10e0193b0dcc19e4E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hd8c69e3f7783e4efE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hcd52b66f015e2ed4E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h6a9a6269c688a2c1E)
  (data (;0;) (i32.const 1048576) "\01\00\00\00\04\00\00\00\04\00\00\00\02\00\00\00\02\00\00\00\03\00\00\00hello\0a\00\00\18\00\10\00\06\00\00\00 \00\10\00,\00\10\00\0a\00\00\00,\00\10\00\00\00\00\000\00\10\00\01\00\00\004\00\10\00Some\06\00\00\00\04\00\00\00\04\00\00\00\07\00\00\00Noneassertion failed: `(left == right)`\0a  left: ``,\0a right: ``\00\00`\00\10\00-\00\00\00\8d\00\10\00\0c\00\00\00\99\00\10\00\01\00\00\00\9c\00\10\00/rustc/18bf6b4f01a6feaf7259ba7cdae58031af1b7b39/library/core/src/iter/traits/exact_size.rs\00\00\b8\00\10\00Z\00\00\00i\00\00\00\09\00\00\00assertion failed: 0 < pointee_size && pointee_size <= isize::MAX as usize/rustc/18bf6b4f01a6feaf7259ba7cdae58031af1b7b39/library/core/src/ptr/const_ptr.rs\00\00m\01\10\00Q\00\00\00t\01\00\00\09\00\00\00\15\00\00\00\04\00\00\00\04\00\00\00\16\00\00\00\17\00\00\00\18\00\00\00\15\00\00\00\04\00\00\00\04\00\00\00\19\00\00\00\1a\00\00\00\1b\00\00\00\15\00\00\00\04\00\00\00\04\00\00\00\1c\00\00\00\1d\00\00\00\1e\00\00\00\15\00\00\00\04\00\00\00\04\00\00\00\1f\00\00\00already borrowedalready mutably borrowedassertion failed: `(left == right)`\0a  left: ``,\0a right: ``\00\00P\02\10\00-\00\00\00}\02\10\00\0c\00\00\00\89\02\10\00\01\00\00\00\15\00\00\00\00\00\00\00\01\00\00\00 \00\00\00called `Option::unwrap()` on a `None` value\00\15\00\00\00\00\00\00\00\01\00\00\00!\00\00\00\15\00\00\00\00\00\00\00\01\00\00\00\22\00\00\00#\00\00\00\10\00\00\00\04\00\00\00$\00\00\00called `Result::unwrap()` on an `Err` value\00%\00\00\00\08\00\00\00\04\00\00\00&\00\00\00'\00\00\00\0c\00\00\00\04\00\00\00(\00\00\00\15\00\00\00\08\00\00\00\04\00\00\00)\00\00\00\15\00\00\00\04\00\00\00\04\00\00\00\0b\00\00\00library/std/src/thread/mod.rs\00\00\00|\03\10\00\1d\00\00\00k\03\00\00*\00\00\00inconsistent park state\00|\03\10\00\1d\00\00\00y\03\00\00\13\00\00\00\02\00\00\00`: \00P\02\10\00-\00\00\00}\02\10\00\0c\00\00\00\d8\03\10\00\03\00\00\00park state changed unexpectedly\00\f4\03\10\00\1f\00\00\00|\03\10\00\1d\00\00\00v\03\00\00\0d\00\00\00failed to generate unique thread ID: bitspace exhausted\00|\03\10\00\1d\00\00\00\09\04\00\00\11\00\00\00|\03\10\00\1d\00\00\00\0f\04\00\00*\00\00\00thread name may not contain interior null bytes\00|\03\10\00\1d\00\00\00M\04\00\00*\00\00\00inconsistent state in unpark|\03\10\00\1d\00\00\00\83\04\00\00\12\00\00\00|\03\10\00\1d\00\00\00\91\04\00\00%\00\00\00\22RUST_BACKTRACE0library/std/src/env.rsfailed to get environment variable `\00\00&\05\10\00$\00\00\00\d8\03\10\00\03\00\00\00\10\05\10\00\16\00\00\00\f1\00\00\00\1d\00\00\00\10\05\10\00\16\00\00\00\02\03\00\003\00\00\00'\00\00\00\0c\00\00\00\04\00\00\00*\00\00\00+\00\00\00,\00\00\00-\00\00\00*\00\00\00.\00\00\00/\00\00\00library/std/src/ffi/c_str.rsdata provided contains a nul byte\00\00\00\a4\05\10\00\1c\00\00\00*\05\00\00\0a\00\00\00library/std/src/io/buffered.rs\00\00\f4\05\10\00\1e\00\00\00\0e\02\00\00)\00\00\00\f4\05\10\00\1e\00\00\00\0e\02\00\009\00\00\00failed to write the buffered data\00\00\00\f4\05\10\00\1e\00\00\00H\02\00\00\1d\00\00\00\f4\05\10\00\1e\00\00\00\01\04\00\00#\00\00\00unexpected end of fileother os erroroperation interruptedwrite zerotimed outinvalid datainvalid input parameteroperation would blockentity already existsbroken pipeaddress not availableaddress in usenot connectedconnection abortedconnection resetconnection refusedpermission deniedentity not found\00\00\00\a4\02\10\00\00\00\00\00 (os error )\a4\02\10\00\00\00\00\00\ac\07\10\00\0b\00\00\00\b7\07\10\00\01\00\00\00failed to write whole bufferlibrary/std/src/io/stdio.rscannot access stdout during shutdown\00\ec\07\10\00\1b\00\00\00\18\02\00\003\00\00\00\ec\07\10\00\1b\00\00\00q\02\00\00\14\00\00\00\ec\07\10\00\1b\00\00\00@\03\00\00;\00\00\00failed printing to : \00\00\00\5c\08\10\00\13\00\00\00o\08\10\00\02\00\00\00\ec\07\10\00\1b\00\00\00\7f\03\00\00\09\00\00\00\ec\07\10\00\1b\00\00\00t\03\00\00\1a\00\00\00\ec\07\10\00\1b\00\00\00w\03\00\00\14\00\00\00stdoutlibrary/std/src/io/mod.rs\00\ba\08\10\00\19\00\00\00m\04\00\00\19\00\00\00\ba\08\10\00\19\00\00\00h\05\00\00!\00\00\000\00\00\00\0c\00\00\00\04\00\00\001\00\00\002\00\00\003\00\00\00formatter error\000\00\00\00\0c\00\00\00\04\00\00\004\00\00\005\00\00\006\00\00\00attempted to use a condition variable with two mutexeslibrary/std/src/sync/condvar.rs\00\00\00j\09\10\00\1f\00\00\00<\02\00\00\12\00\00\00\15\00\00\00\04\00\00\00\04\00\00\007\00\00\008\00\00\00library/std/src/sync/once.rs\b0\09\10\00\1c\00\00\00\09\01\00\002\00\00\00assertion failed: state_and_queue & STATE_MASK == RUNNING\00\00\00\b0\09\10\00\1c\00\00\00\ac\01\00\00\15\00\00\00Once instance has previously been poisoned\00\00\b0\09\10\00\1c\00\00\00\8c\01\00\00\15\00\00\00\b0\09\10\00\1c\00\00\00\ed\01\00\00\09\00\00\00\b0\09\10\00\1c\00\00\00\f9\01\00\005\00\00\00assertion failed: queue != DONElibrary/std/src/sys_common/at_exit_imp.rs\a3\0a\10\00)\00\00\001\00\00\00\0d\00\00\00stack backtrace:\0a\00\00\00\dc\0a\10\00\11\00\00\00note: Some details are omitted, run with `RUST_BACKTRACE=full` for a verbose backtrace.\0a\f8\0a\10\00X\00\00\00full\5cx\00\00\5c\0b\10\00\02\00\00\00\00\00\00\00 \00\00\00\08\00\00\00\02\00\00\00\00\00\00\00\00\00\00\00\02\00\00\00\03\00\00\00PoisonError { inner: .. }library/std/src/sys_common/thread_info.rs\00\00\a1\0b\10\00)\00\00\00\15\00\00\00\16\00\00\00\a1\0b\10\00)\00\00\00\16\00\00\00\18\00\00\00\a1\0b\10\00)\00\00\00\19\00\00\00\15\00\00\00\a1\0b\10\00)\00\00\00(\00\00\00$\00\00\00assertion failed: c.borrow().is_none()\00\00\a1\0b\10\00)\00\00\00(\00\00\00\1a\00\00\00\a1\0b\10\00)\00\00\00)\00\00\00\22\00\00\00fatal runtime error: \0a\00\00T\0c\10\00\15\00\00\00i\0c\10\00\01\00\00\00\15\00\00\00\04\00\00\00\04\00\00\009\00\00\00memory allocation of  bytes failed\00\00\8c\0c\10\00\15\00\00\00\a1\0c\10\00\0d\00\00\00library/std/src/panicking.rs\c0\0c\10\00\1c\00\00\00\b6\00\00\00$\00\00\00Box<Any><unnamed>\00\00\00\15\00\00\00\00\00\00\00\01\00\00\00:\00\00\00;\00\00\00<\00\00\00=\00\00\00>\00\00\00?\00\00\00@\00\00\00\00\00\00\00A\00\00\00\08\00\00\00\04\00\00\00B\00\00\00C\00\00\00D\00\00\00E\00\00\00F\00\00\00G\00\00\00H\00\00\00\00\00\00\00thread '' panicked at '', \00\00X\0d\10\00\08\00\00\00`\0d\10\00\0f\00\00\00o\0d\10\00\03\00\00\00i\0c\10\00\01\00\00\00note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\0a\00\00\94\0d\10\00N\00\00\00\c0\0c\10\00\1c\00\00\00\d9\01\00\00\1f\00\00\00\c0\0c\10\00\1c\00\00\00\da\01\00\00\1e\00\00\00I\00\00\00\10\00\00\00\04\00\00\00J\00\00\00K\00\00\00'\00\00\00\0c\00\00\00\04\00\00\00L\00\00\00\15\00\00\00\08\00\00\00\04\00\00\00M\00\00\00N\00\00\00\15\00\00\00\08\00\00\00\04\00\00\00O\00\00\00thread panicked while processing panic. aborting.\0a\00\00T\0e\10\002\00\00\00thread panicked while panicking. aborting.\0a\00\90\0e\10\00+\00\00\00failed to initiate panic, error \c4\0e\10\00 \00\00\00NulError\15\00\00\00\04\00\00\00\04\00\00\00P\00\00\00condvar wait not supportedlibrary/std/src/sys/wasi/../unsupported/condvar.rs\1e\0f\10\002\00\00\00\15\00\00\00\09\00\00\00advancing IoSlice beyond its lengthlibrary/std/src/sys/wasi/io.rs\00\00\00\83\0f\10\00\1e\00\00\00\14\00\00\00\0d\00\00\00cannot recursively acquire mutexlibrary/std/src/sys/wasi/../unsupported/mutex.rs\d4\0f\10\000\00\00\00\16\00\00\00\09\00\00\00strerror_r failurelibrary/std/src/sys/wasi/os.rs&\10\10\00\1e\00\00\00#\00\00\00\0d\00\00\00&\10\10\00\1e\00\00\00%\00\00\006\00\00\00rwlock locked for writing\00\00\00d\10\10\00\19\00\00\00operation not supported on this platform\00\19\12D;\02?,G\14=30\0a\1b\06FKE7\0fI\0e\17\03@\1d<+6\1fJ-\1c\01 %)!\08\0c\15\16\22.\108>\0b41\18/A\099\11#C2B:\05\04&('\0d*\1e5\07\1aH\13$L\ff\00\00Success\00Illegal byte sequence\00Domain error\00Result not representable\00Not a tty\00Permission denied\00Operation not permitted\00No such file or directory\00No such process\00File exists\00Value too large for data type\00No space left on device\00Out of memory\00Resource busy\00Interrupted system call\00Resource temporarily unavailable\00Invalid seek\00Cross-device link\00Read-only file system\00Directory not empty\00Connection reset by peer\00Operation timed out\00Connection refused\00Host is unreachable\00Address in use\00Broken pipe\00I/O error\00No such device or address\00No such device\00Not a directory\00Is a directory\00Text file busy\00Exec format error\00Invalid argument\00Argument list too long\00Symbolic link loop\00Filename too long\00Too many open files in system\00No file descriptors available\00Bad file descriptor\00No child process\00Bad address\00File too large\00Too many links\00No locks available\00Resource deadlock would occur\00State not recoverable\00Previous owner died\00Operation canceled\00Function not implemented\00No message of desired type\00Identifier removed\00Link has been severed\00Protocol error\00Bad message\00Not a socket\00Destination address required\00Message too large\00Protocol wrong type for socket\00Protocol not available\00Protocol not supported\00Not supported\00Address family not supported by protocol\00Address not available\00Network is down\00Network unreachable\00Connection reset by network\00Connection aborted\00No buffer space available\00Socket is connected\00Socket not connected\00Operation already in progress\00Operation in progress\00Stale file handle\00Quota exceeded\00Multihop attempted\00Capabilities insufficient\00No error information\00\00library/alloc/src/raw_vec.rscapacity overflow\00&\17\10\00\1c\00\00\00\19\02\00\00\05\00\00\00)library/alloc/src/vec.rs) should be <= len (is end drain index (is \94\17\10\00\14\00\00\00}\17\10\00\17\00\00\00d\17\10\00\01\00\00\00e\17\10\00\18\00\00\000\05\00\00\0d\00\00\00`\00..\d2\17\10\00\02\00\00\00BorrowErrorBorrowMutErrorcalled `Option::unwrap()` on a `None` value\d0\17\10\00\00\00\00\00: \00\00\d0\17\10\00\00\00\00\00(\18\10\00\02\00\00\00V\00\00\00\00\00\00\00\01\00\00\00W\00\00\00:\00\00\00\d0\17\10\00\00\00\00\00L\18\10\00\01\00\00\00L\18\10\00\01\00\00\00index out of bounds: the len is  but the index is \00\00h\18\10\00 \00\00\00\88\18\10\00\12\00\00\00library/core/src/fmt/builders.rsV\00\00\00\0c\00\00\00\04\00\00\00X\00\00\00Y\00\00\00Z\00\00\00    \ac\18\10\00 \00\00\000\00\00\00!\00\00\00\ac\18\10\00 \00\00\001\00\00\00\12\00\00\00 {\0a,\0a,  { } }(\0a(,)\0a[V\00\00\00\04\00\00\00\04\00\00\00[\00\00\00]library/core/src/fmt/num.rs-\19\10\00\1b\00\00\00T\00\00\00\14\00\00\000x00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899\00\00V\00\00\00\04\00\00\00\04\00\00\00\5c\00\00\00]\00\00\00^\00\00\00library/core/src/fmt/mod.rs\00<\1a\10\00\1b\00\00\00Y\04\00\00\11\00\00\00<\1a\10\00\1b\00\00\00c\04\00\00$\00\00\00<\1a\10\00\1b\00\00\00\f4\07\00\00\1e\00\00\00<\1a\10\00\1b\00\00\00\fb\07\00\00\16\00\00\00library/core/src/slice/memchr.rs\98\1a\10\00 \00\00\00R\00\00\00\05\00\00\00\98\1a\10\00 \00\00\00i\00\00\00\1a\00\00\00\98\1a\10\00 \00\00\00\83\00\00\00\05\00\00\00range start index  out of range for slice of length \e8\1a\10\00\12\00\00\00\fa\1a\10\00\22\00\00\00range end index ,\1b\10\00\10\00\00\00\fa\1a\10\00\22\00\00\00slice index starts at  but ends at \00L\1b\10\00\16\00\00\00b\1b\10\00\0d\00\00\00attempted to index slice up to maximum usizelibrary/core/src/str/pattern.rs\00\ac\1b\10\00\1f\00\00\00\b0\01\00\00&\00\00\00library/core/src/str/lossy.rs\00\00\00\dc\1b\10\00\1d\00\00\00\80\00\00\00\19\00\00\00\dc\1b\10\00\1d\00\00\00w\00\00\00\1d\00\00\00\dc\1b\10\00\1d\00\00\00{\00\00\00\1d\00\00\00\dc\1b\10\00\1d\00\00\00r\00\00\00!\00\00\00\dc\1b\10\00\1d\00\00\00h\00\00\00\1d\00\00\00\dc\1b\10\00\1d\00\00\00c\00\00\00!\00\00\00\dc\1b\10\00\1d\00\00\00X\00\00\00\1d\00\00\00\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\04\04\04\04\04\00\00\00\00\00\00\00\00\00\00\00[...]byte index  is out of bounds of `\00\00q\1d\10\00\0b\00\00\00|\1d\10\00\16\00\00\00\d0\17\10\00\01\00\00\00begin <= end ( <= ) when slicing `\00\00\ac\1d\10\00\0e\00\00\00\ba\1d\10\00\04\00\00\00\be\1d\10\00\10\00\00\00\d0\17\10\00\01\00\00\00 is not a char boundary; it is inside  (bytes ) of `q\1d\10\00\0b\00\00\00\f0\1d\10\00&\00\00\00\16\1e\10\00\08\00\00\00\1e\1e\10\00\06\00\00\00\d0\17\10\00\01\00\00\00library/core/src/unicode/printable.rs\00\00\00L\1e\10\00%\00\00\00\0a\00\00\00\1c\00\00\00L\1e\10\00%\00\00\00\1a\00\00\006\00\00\00\00\01\03\05\05\06\06\03\07\06\08\08\09\11\0a\1c\0b\19\0c\14\0d\10\0e\0d\0f\04\10\03\12\12\13\09\16\01\17\05\18\02\19\03\1a\07\1c\02\1d\01\1f\16 \03+\03,\02-\0b.\010\031\022\01\a7\02\a9\02\aa\04\ab\08\fa\02\fb\05\fd\04\fe\03\ff\09\adxy\8b\8d\a20WX\8b\8c\90\1c\1d\dd\0e\0fKL\fb\fc./?\5c]_\b5\e2\84\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\ff\00\04\11\12)147:;=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91\9b\9d\c9\ce\cf\0d\11)EIWde\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\0d\11EIde\80\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\8b\a4\a6\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfINOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7\11\16\17[\5c\f6\f7\fe\ff\80\0dmq\de\df\0e\0f\1fno\1c\1d_}~\ae\af\bb\bc\fa\16\17\1e\1fFGNOXZ\5c^~\7f\b5\c5\d4\d5\dc\f0\f1\f5rs\8ftu\96/_&./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\980\8f\1f\c0\c1\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91\fe\ffSgu\c8\c9\d0\d1\d8\d9\e7\fe\ff\00 _\22\82\df\04\82D\08\1b\04\06\11\81\ac\0e\80\ab5(\0b\80\e0\03\19\08\01\04/\044\04\07\03\01\07\06\07\11\0aP\0f\12\07U\07\03\04\1c\0a\09\03\08\03\07\03\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05:\03\11\07\06\05\10\07W\07\02\07\15\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%_ m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\15\0b\17\09\14\0c\14\0cj\06\0a\06\1a\06Y\07+\05F\0a,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\0a\06!?L\04-\03t\08<\03\0f\03<\078\08+\05\82\ff\11\18\08/\11-\03 \10!\0f\80\8c\04\82\97\19\0b\15\88\94\05/\05;\07\02\0e\18\09\80\b3-t\0c\80\d6\1a\0c\05\80\ff\05\80\df\0c\ee\0d\03\84\8d\037\09\81\5c\14\80\b8\08\80\cb*8\03\0a\068\08F\08\0c\06t\0b\1e\03Z\04Y\09\80\83\18\1c\0a\16\09L\04\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a5\11\81m\10x(*\06L\04\80\8d\04\80\be\03\1b\03\0f\0d\00\06\01\01\03\01\04\02\08\08\09\02\0a\05\0b\02\0e\04\10\01\11\02\12\05\13\11\14\01\15\02\17\02\19\0d\1c\05\1d\08$\01j\03k\02\bc\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e1\02\e8\02\ee \f0\04\f8\02\f9\02\fa\02\fb\01\0c';>NO\8f\9e\9e\9f\06\07\096=>V\f3\d0\d1\04\14\1867VW\7f\aa\ae\af\bd5\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:EFIJNOde\5c\b6\b7\1b\1c\07\08\0a\0b\14\1769:\a8\a9\d8\d9\097\90\91\a8\07\0a;>fi\8f\92o_\ee\efZb\9a\9b'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\22%>?\c5\c6\04 #%&(38:HJLPSUVXZ\5c^`cefksx}\7f\8a\a4\aa\af\b0\c0\d0\ae\afy\ccno\93^\22{\05\03\04-\03f\03\01/.\80\82\1d\031\0f\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0b\01\80\90\817\09\16\0a\08\80\989\03c\08\090\16\05!\03\1b\05\01@8\04K\05/\04\0a\07\09\07@ '\04\0c\096\03:\05\1a\07\04\0c\07PI73\0d3\07.\08\0a\81&RN(\08*V\1c\14\17\09N\04\1e\0fC\0e\19\07\0a\06H\08'\09u\0b?A*\06;\05\0a\06Q\06\01\05\10\03\05\80\8bb\1eH\08\0a\80\a6^\22E\0b\0a\06\0d\139\07\0a6,\04\10\80\c0<dS\0cH\09\0aFE\1bH\08S\1d9\81\07F\0a\1d\03GI7\03\0e\08\0a\069\07\0a\816\19\80\b7\01\0f2\0d\83\9bfu\0b\80\c4\8a\bc\84/\8f\d1\82G\a1\b9\829\07*\04\02`&\0aF\0a(\05\13\82\b0[eK\049\07\11@\05\0b\02\0e\97\f8\08\84\d6*\09\a2\f7\81\1f1\03\11\04\08\81\8c\89\04k\05\0d\03\09\07\10\93`\80\f6\0as\08n\17F\80\9a\14\0cW\09\19\80\87\81G\03\85B\0f\15\85P+\80\d5-\03\1a\04\02\81p:\05\01\85\00\80\d7)L\04\0a\04\02\83\11DL=\80\c2<\06\01\04U\05\1b4\02\81\0e,\04d\0cV\0a\80\ae8\1d\0d,\04\09\07\02\0e\06\80\9a\83\d8\08\0d\03\0d\03t\0cY\07\0c\14\0c\048\08\0a\06(\08\22N\81T\0c\15\03\03\05\07\09\19\07\07\09\03\0d\07)\80\cb%\0a\84\06library/core/src/unicode/unicode_data.rs\00\db#\10\00(\00\00\00K\00\00\00(\00\00\00\db#\10\00(\00\00\00W\00\00\00\16\00\00\00\db#\10\00(\00\00\00R\00\00\00>\00\00\00V\00\00\00\04\00\00\00\04\00\00\00_\00\00\00SomeNoneUtf8Errorvalid_up_toerror_len\00\00\00V\00\00\00\04\00\00\00\04\00\00\00`\00\00\00\00\03\00\00\83\04 \00\91\05`\00]\13\a0\00\12\17\a0\1e\0c \e0\1e\ef, +*0\a0+o\a6`,\02\a8\e0,\1e\fb\e0-\00\fe\a05\9e\ff\e05\fd\01a6\01\0a\a16$\0da7\ab\0e\e18/\18!90\1caF\f3\1e\a1J\f0jaNOo\a1N\9d\bc!Oe\d1\e1O\00\da!P\00\e0\e1Q0\e1aS\ec\e2\a1T\d0\e8\e1T \00.U\f0\01\bfU\00p\00\07\00-\01\01\01\02\01\02\01\01H\0b0\15\10\01e\07\02\06\02\02\01\04#\01\1e\1b[\0b:\09\09\01\18\04\01\09\01\03\01\05+\03w\0f\01 7\01\01\01\04\08\04\01\03\07\0a\02\1d\01:\01\01\01\02\04\08\01\09\01\0a\02\1a\01\02\029\01\04\02\04\02\02\03\03\01\1e\02\03\01\0b\029\01\04\05\01\02\04\01\14\02\16\06\01\01:\01\01\02\01\04\08\01\07\03\0a\02\1e\01;\01\01\01\0c\01\09\01(\01\03\019\03\05\03\01\04\07\02\0b\02\1d\01:\01\02\01\02\01\03\01\05\02\07\02\0b\02\1c\029\02\01\01\02\04\08\01\09\01\0a\02\1d\01H\01\04\01\02\03\01\01\08\01Q\01\02\07\0c\08b\01\02\09\0b\06J\02\1b\01\01\01\01\017\0e\01\05\01\02\05\0b\01$\09\01f\04\01\06\01\02\02\02\19\02\04\03\10\04\0d\01\02\02\06\01\0f\01\00\03\00\03\1d\03\1d\02\1e\02@\02\01\07\08\01\02\0b\09\01-\03w\02\22\01v\03\04\02\09\01\06\03\db\02\02\01:\01\01\07\01\01\01\01\02\08\06\0a\02\010\11?\040\07\01\01\05\01(\09\0c\02 \04\02\02\01\038\01\01\02\03\01\01\03:\08\02\02\98\03\01\0d\01\07\04\01\06\01\03\02\c6:\01\05\00\01\c3!\00\03\8d\01` \00\06i\02\00\04\01\0a \02P\02\00\01\03\01\04\01\19\02\05\01\97\02\1a\12\0d\01&\08\19\0b.\030\01\02\04\02\02'\01C\06\02\02\02\02\0c\01\08\01/\013\01\01\03\02\02\05\02\01\01*\02\08\01\ee\01\02\01\04\01\00\01\00\10\10\10\00\02\00\01\e2\01\95\05\00\03\01\02\05\04(\03\04\01\a5\02\00\04\00\02\99\0b\b0\016\0f8\031\04\02\02E\03$\05\01\08>\01\0c\024\09\0a\04\02\01_\03\02\01\01\02\06\01\a0\01\03\08\15\029\02\01\01\01\01\16\01\0e\07\03\05\c3\08\02\03\01\01\17\01Q\01\02\06\01\01\02\01\01\02\01\02\eb\01\02\04\06\02\01\02\1b\02U\08\02\01\01\02j\01\01\01\02\06\01\01e\03\02\04\01\05\00\09\01\02\f5\01\0a\02\01\01\04\01\90\04\02\02\04\01 \0a(\06\02\04\08\01\09\06\02\03.\0d\01\02\00\07\01\06\01\01R\16\02\07\01\02\01\02z\06\03\01\01\02\01\07\01\01H\02\03\01\01\01\00\02\00\05;\07\00\01?\04Q\01\00\02\00\01\01\03\04\05\08\08\02\07\1e\04\94\03\007\042\08\01\0e\01\16\05\01\0f\00\07\01\11\02\07\01\02\01\05\00\07\00\04\00\07m\07\00`\80\f0\00")
  (data (;1;) (i32.const 1058736) "\01\00\00\00\00\00\00\00\01"))
