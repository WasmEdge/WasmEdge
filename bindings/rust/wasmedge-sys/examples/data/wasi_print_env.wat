(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func (param i32) (result i64)))
  (type (;2;) (func (param i32 i32)))
  (type (;3;) (func (param i32 i32) (result i32)))
  (type (;4;) (func (param i32 i32 i32)))
  (type (;5;) (func (param i32 i32 i32) (result i32)))
  (type (;6;) (func (param i32 i32 i32 i32) (result i32)))
  (type (;7;) (func))
  (type (;8;) (func (param i32 i32 i32 i32)))
  (type (;9;) (func (result i32)))
  (type (;10;) (func (param i32) (result i32)))
  (type (;11;) (func (param i32 i32 i32 i32 i32)))
  (type (;12;) (func (param i32 i32 i32 i32 i32) (result i32)))
  (type (;13;) (func (param i32 i32 i32 i32 i32 i32 i32)))
  (type (;14;) (func (param i32 i32 i32 i32 i32 i32) (result i32)))
  (type (;15;) (func (param i32 i32 i32 i32 i32 i32 i32) (result i32)))
  (type (;16;) (func (param i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "args_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hfe12d9463b2a7790E (type 3)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h19746ec286063fe2E (type 3)))
  (import "wasi_snapshot_preview1" "fd_write" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h2fb5d28ef433e1b2E (type 6)))
  (import "wasi_snapshot_preview1" "environ_get" (func $__imported_wasi_snapshot_preview1_environ_get (type 3)))
  (import "wasi_snapshot_preview1" "environ_sizes_get" (func $__imported_wasi_snapshot_preview1_environ_sizes_get (type 3)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $__imported_wasi_snapshot_preview1_proc_exit (type 0)))
  (func $__wasm_call_ctors (type 7)
    call $__wasilibc_initialize_environ_eagerly)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.const 8
    i32.add
    i32.load
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $print_env (type 7)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 0
    global.set $__stack_pointer
    local.get 0
    i32.const 124
    i32.add
    i32.const 0
    i32.store
    local.get 0
    i32.const 1048576
    i32.store offset=120
    local.get 0
    i64.const 1
    i64.store offset=108 align=4
    local.get 0
    i32.const 1048608
    i32.store offset=104
    local.get 0
    i32.const 104
    i32.add
    call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
    local.get 0
    call $_ZN3std3env4vars17h6ec7390381e46b70E
    local.get 0
    i32.const 16
    i32.add
    i32.const 8
    i32.add
    local.get 0
    i32.const 8
    i32.add
    i64.load
    i64.store
    local.get 0
    local.get 0
    i64.load
    i64.store offset=16
    local.get 0
    i32.const 32
    i32.add
    local.get 0
    i32.const 16
    i32.add
    call $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE
    block  ;; label = @1
      local.get 0
      i32.load offset=32
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.const 44
      i32.add
      local.set 1
      loop  ;; label = @2
        local.get 0
        i32.const 56
        i32.add
        i32.const 8
        i32.add
        local.get 0
        i32.const 32
        i32.add
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get 0
        local.get 0
        i64.load offset=32
        i64.store offset=56
        local.get 0
        i32.const 72
        i32.add
        i32.const 8
        i32.add
        local.get 1
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get 0
        local.get 1
        i64.load align=4
        i64.store offset=72
        local.get 0
        i32.const 2
        i32.store offset=124
        local.get 0
        i64.const 3
        i64.store offset=108 align=4
        local.get 0
        i32.const 1048620
        i32.store offset=104
        local.get 0
        i32.const 1
        i32.store offset=100
        local.get 0
        i32.const 1
        i32.store offset=92
        local.get 0
        local.get 0
        i32.const 88
        i32.add
        i32.store offset=120
        local.get 0
        local.get 0
        i32.const 72
        i32.add
        i32.store offset=96
        local.get 0
        local.get 0
        i32.const 56
        i32.add
        i32.store offset=88
        local.get 0
        i32.const 104
        i32.add
        call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
        block  ;; label = @3
          local.get 0
          i32.load offset=76
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load offset=72
          local.get 2
          i32.const 1
          call $__rust_dealloc
        end
        block  ;; label = @3
          local.get 0
          i32.load offset=60
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load offset=56
          local.get 2
          i32.const 1
          call $__rust_dealloc
        end
        local.get 0
        i32.const 32
        i32.add
        local.get 0
        i32.const 16
        i32.add
        call $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE
        local.get 0
        i32.load offset=32
        br_if 0 (;@2;)
      end
    end
    local.get 0
    i32.load offset=28
    local.get 0
    i32.load offset=24
    local.tee 1
    i32.sub
    local.tee 2
    i32.const 24
    i32.div_u
    local.set 3
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.const 24
      i32.mul
      local.set 2
      loop  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.const 4
          i32.add
          i32.load
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 3
          i32.const 1
          call $__rust_dealloc
        end
        block  ;; label = @3
          local.get 1
          i32.const 16
          i32.add
          i32.load
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.const 12
          i32.add
          i32.load
          local.get 3
          i32.const 1
          call $__rust_dealloc
        end
        local.get 1
        i32.const 24
        i32.add
        local.set 1
        local.get 2
        i32.const -24
        i32.add
        local.tee 2
        br_if 0 (;@2;)
      end
    end
    block  ;; label = @1
      local.get 0
      i32.load offset=20
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=16
      local.get 1
      i64.extend_i32_u
      i64.const 24
      i64.mul
      i32.wrap_i64
      i32.const 4
      call $__rust_dealloc
    end
    local.get 0
    i32.const 124
    i32.add
    i32.const 0
    i32.store
    local.get 0
    i32.const 1048576
    i32.store offset=120
    local.get 0
    i64.const 1
    i64.store offset=108 align=4
    local.get 0
    i32.const 1048672
    i32.store offset=104
    local.get 0
    i32.const 104
    i32.add
    call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
    local.get 0
    i32.const 88
    i32.add
    call $_ZN3std3env4args17h02d9a592c6ab5652E
    local.get 0
    i32.const 32
    i32.add
    i32.const 8
    i32.add
    local.get 0
    i32.const 88
    i32.add
    i32.const 8
    i32.add
    i64.load
    i64.store
    local.get 0
    local.get 0
    i64.load offset=88
    i64.store offset=32
    local.get 0
    local.get 0
    i32.const 32
    i32.add
    call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E
    block  ;; label = @1
      local.get 0
      i32.load
      i32.eqz
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 0
        i32.const 16
        i32.add
        i32.const 8
        i32.add
        local.get 0
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get 0
        local.get 0
        i64.load
        i64.store offset=16
        local.get 0
        i32.const 1
        i32.store offset=124
        local.get 0
        i64.const 2
        i64.store offset=108 align=4
        local.get 0
        i32.const 1048680
        i32.store offset=104
        local.get 0
        i32.const 1
        i32.store offset=76
        local.get 0
        local.get 0
        i32.const 72
        i32.add
        i32.store offset=120
        local.get 0
        local.get 0
        i32.const 16
        i32.add
        i32.store offset=72
        local.get 0
        i32.const 104
        i32.add
        call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
        block  ;; label = @3
          local.get 0
          i32.load offset=20
          local.tee 1
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load offset=16
          local.get 1
          i32.const 1
          call $__rust_dealloc
        end
        local.get 0
        local.get 0
        i32.const 32
        i32.add
        call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E
        local.get 0
        i32.load
        br_if 0 (;@2;)
      end
    end
    local.get 0
    i32.load offset=44
    local.get 0
    i32.load offset=40
    local.tee 1
    i32.sub
    local.tee 2
    i32.const 12
    i32.div_u
    local.set 3
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.const 12
      i32.mul
      local.set 2
      loop  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.const 4
          i32.add
          i32.load
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 1
          i32.load
          local.get 3
          i32.const 1
          call $__rust_dealloc
        end
        local.get 1
        i32.const 12
        i32.add
        local.set 1
        local.get 2
        i32.const -12
        i32.add
        local.tee 2
        br_if 0 (;@2;)
      end
    end
    block  ;; label = @1
      local.get 0
      i32.load offset=36
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=32
      local.get 1
      i64.extend_i32_u
      i64.const 12
      i64.mul
      i32.wrap_i64
      i32.const 4
      call $__rust_dealloc
    end
    local.get 0
    i32.const 128
    i32.add
    global.set $__stack_pointer)
  (func $__rust_alloc (type 3) (param i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    call $__rdl_alloc
    local.set 2
    local.get 2
    return)
  (func $__rust_dealloc (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    call $__rdl_dealloc
    return)
  (func $__rust_realloc (type 6) (param i32 i32 i32 i32) (result i32)
    (local i32)
    local.get 0
    local.get 1
    local.get 2
    local.get 3
    call $__rdl_realloc
    local.set 4
    local.get 4
    return)
  (func $__rust_alloc_error_handler (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $__rg_oom
    return)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2d0c1ef8155b3364E (type 1) (param i32) (result i64)
    i64.const 7628363314155272196)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h5d3c77c4d604c6d6E (type 1) (param i32) (result i64)
    i64.const -887290134024487584)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h83af4092997449d4E (type 1) (param i32) (result i64)
    i64.const -5139102199292759541)
  (func $_ZN66_$LT$std..sys..wasi..os_str..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hb769d13a4a71030dE (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 96
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    i32.const 1
    local.set 4
    block  ;; label = @1
      local.get 2
      i32.const 1049436
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE
      br_if 0 (;@1;)
      local.get 3
      i32.const 8
      i32.add
      local.get 0
      local.get 1
      call $_ZN4core3str5lossy9Utf8Lossy10from_bytes17h9d8ade7d6641c0d4E
      local.get 3
      local.get 3
      i32.load offset=8
      local.get 3
      i32.load offset=12
      call $_ZN4core3str5lossy9Utf8Lossy6chunks17hdeccf1496c5995d4E
      local.get 3
      local.get 3
      i64.load
      i64.store offset=16
      local.get 3
      i32.const 24
      i32.add
      local.get 3
      i32.const 16
      i32.add
      call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E
      block  ;; label = @2
        local.get 3
        i32.load offset=24
        local.tee 4
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        i32.const 48
        i32.add
        local.set 5
        local.get 3
        i32.const 40
        i32.add
        i32.const 24
        i32.add
        local.set 6
        loop  ;; label = @3
          local.get 3
          i32.load offset=36
          local.set 7
          local.get 3
          i32.load offset=32
          local.set 1
          local.get 3
          i32.load offset=28
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
            loop  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 4
                          i32.const 4
                          i32.eq
                          br_if 0 (;@11;)
                          local.get 5
                          call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E
                          local.tee 4
                          i32.const 1114112
                          i32.ne
                          br_if 1 (;@10;)
                          local.get 3
                          i32.const 4
                          i32.store offset=48
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        local.get 3
                                        i32.load offset=40
                                        local.tee 4
                                        i32.eqz
                                        br_if 0 (;@18;)
                                        local.get 3
                                        i32.load offset=44
                                        local.get 4
                                        i32.eq
                                        br_if 0 (;@18;)
                                        local.get 3
                                        local.get 4
                                        i32.const 1
                                        i32.add
                                        i32.store offset=40
                                        block  ;; label = @19
                                          local.get 4
                                          i32.load8_u
                                          local.tee 0
                                          i32.const 24
                                          i32.shl
                                          i32.const 24
                                          i32.shr_s
                                          i32.const -1
                                          i32.gt_s
                                          br_if 0 (;@19;)
                                          local.get 3
                                          local.get 4
                                          i32.const 2
                                          i32.add
                                          i32.store offset=40
                                          local.get 4
                                          i32.load8_u offset=1
                                          i32.const 63
                                          i32.and
                                          local.set 8
                                          local.get 0
                                          i32.const 31
                                          i32.and
                                          local.set 9
                                          block  ;; label = @20
                                            local.get 0
                                            i32.const 223
                                            i32.gt_u
                                            br_if 0 (;@20;)
                                            local.get 9
                                            i32.const 6
                                            i32.shl
                                            local.get 8
                                            i32.or
                                            local.set 0
                                            br 1 (;@19;)
                                          end
                                          local.get 3
                                          local.get 4
                                          i32.const 3
                                          i32.add
                                          i32.store offset=40
                                          local.get 8
                                          i32.const 6
                                          i32.shl
                                          local.get 4
                                          i32.load8_u offset=2
                                          i32.const 63
                                          i32.and
                                          i32.or
                                          local.set 8
                                          block  ;; label = @20
                                            local.get 0
                                            i32.const 240
                                            i32.ge_u
                                            br_if 0 (;@20;)
                                            local.get 8
                                            local.get 9
                                            i32.const 12
                                            i32.shl
                                            i32.or
                                            local.set 0
                                            br 1 (;@19;)
                                          end
                                          local.get 3
                                          local.get 4
                                          i32.const 4
                                          i32.add
                                          i32.store offset=40
                                          local.get 8
                                          i32.const 6
                                          i32.shl
                                          local.get 4
                                          i32.load8_u offset=3
                                          i32.const 63
                                          i32.and
                                          i32.or
                                          local.get 9
                                          i32.const 18
                                          i32.shl
                                          i32.const 1835008
                                          i32.and
                                          i32.or
                                          local.set 0
                                        end
                                        i32.const 48
                                        local.set 8
                                        i32.const 2
                                        local.set 4
                                        block  ;; label = @19
                                          local.get 0
                                          br_table 13 (;@6;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 7 (;@12;) 3 (;@16;) 4 (;@15;) 4 (;@15;) 2 (;@17;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 5 (;@14;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 4 (;@15;) 5 (;@14;) 0 (;@19;)
                                        end
                                        local.get 0
                                        i32.const 92
                                        i32.eq
                                        br_if 4 (;@14;)
                                        local.get 0
                                        i32.const 1114112
                                        i32.ne
                                        br_if 3 (;@15;)
                                      end
                                      block  ;; label = @18
                                        local.get 3
                                        i32.load offset=64
                                        i32.const 4
                                        i32.eq
                                        br_if 0 (;@18;)
                                        local.get 6
                                        call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E
                                        local.tee 4
                                        i32.const 1114112
                                        i32.ne
                                        br_if 8 (;@10;)
                                      end
                                      local.get 7
                                      br_if 8 (;@9;)
                                      br 13 (;@4;)
                                    end
                                    i32.const 114
                                    local.set 8
                                    br 5 (;@11;)
                                  end
                                  i32.const 110
                                  local.set 8
                                  br 4 (;@11;)
                                end
                                block  ;; label = @15
                                  local.get 0
                                  call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E
                                  i32.eqz
                                  br_if 0 (;@15;)
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
                                  local.set 10
                                  br 8 (;@7;)
                                end
                                i32.const 1
                                local.set 4
                                local.get 0
                                call $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE
                                i32.eqz
                                br_if 1 (;@13;)
                              end
                              local.get 0
                              local.set 8
                              br 2 (;@11;)
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
                            local.set 10
                            br 5 (;@7;)
                          end
                          i32.const 116
                          local.set 8
                        end
                        br 4 (;@6;)
                      end
                      local.get 2
                      local.get 4
                      call $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h968628441d2ac37cE
                      br_if 1 (;@8;)
                      local.get 3
                      i32.load offset=48
                      local.set 4
                      br 4 (;@5;)
                    end
                    loop  ;; label = @9
                      local.get 3
                      local.get 1
                      i32.store offset=84
                      local.get 3
                      i32.const 1
                      i32.store offset=60
                      local.get 3
                      i32.const 1
                      i32.store offset=52
                      local.get 3
                      i32.const 1052560
                      i32.store offset=48
                      local.get 3
                      i32.const 1
                      i32.store offset=44
                      local.get 3
                      i32.const 1052552
                      i32.store offset=40
                      local.get 3
                      i32.const 2
                      i32.store offset=92
                      local.get 3
                      local.get 3
                      i32.const 88
                      i32.add
                      i32.store offset=56
                      local.get 3
                      local.get 3
                      i32.const 84
                      i32.add
                      i32.store offset=88
                      local.get 2
                      local.get 3
                      i32.const 40
                      i32.add
                      call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
                      br_if 1 (;@8;)
                      local.get 1
                      i32.const 1
                      i32.add
                      local.set 1
                      local.get 7
                      i32.const -1
                      i32.add
                      local.tee 7
                      i32.eqz
                      br_if 5 (;@4;)
                      br 0 (;@9;)
                    end
                  end
                  i32.const 1
                  local.set 4
                  br 6 (;@1;)
                end
                i32.const 3
                local.set 4
                local.get 0
                local.set 8
              end
              local.get 3
              local.get 10
              i64.store offset=56
              local.get 3
              local.get 8
              i32.store offset=52
              local.get 3
              local.get 4
              i32.store offset=48
              br 0 (;@5;)
            end
          end
          local.get 3
          i32.const 24
          i32.add
          local.get 3
          i32.const 16
          i32.add
          call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E
          local.get 3
          i32.load offset=24
          local.tee 4
          br_if 0 (;@3;)
        end
      end
      local.get 2
      i32.const 1049436
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE
      local.set 4
    end
    local.get 3
    i32.const 96
    i32.add
    global.set $__stack_pointer
    local.get 4)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h51406901289b8250E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN43_$LT$bool$u20$as$u20$core..fmt..Display$GT$3fmt17h107f9e8c546bfafcE)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h72c85046dd08428aE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.set 0
    block  ;; label = @1
      local.get 1
      call $_ZN4core3fmt9Formatter15debug_lower_hex17h633f97836530945dE
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 1
        call $_ZN4core3fmt9Formatter15debug_upper_hex17h565dd489bc473806E
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        call $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E
        return
      end
      local.get 0
      local.get 1
      call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17hfe79af84422ce847E
      return
    end
    local.get 0
    local.get 1
    call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17hb5f1486a0d6bf05cE)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h32abcc04c39e85a4E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN70_$LT$core..panic..location..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h679c135f41a338baE)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17hf8285e92300923c0E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h547981a8fa47a7d3E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17hb78061cd9b3bd312E)
  (func $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 3
      i32.const 4
      i32.add
      i32.load
      local.get 3
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 0
      i32.sub
      local.get 1
      i32.ge_u
      br_if 0 (;@1;)
      local.get 3
      local.get 0
      local.get 1
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 0
    end
    local.get 3
    i32.load
    local.get 0
    i32.add
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $memcpy
    drop
    local.get 4
    local.get 0
    local.get 1
    i32.add
    i32.store
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    i32.const 0)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE (type 4) (param i32 i32 i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    block  ;; label = @1
      local.get 1
      local.get 2
      i32.add
      local.tee 2
      local.get 1
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 4
      i32.const 1
      i32.shl
      local.tee 1
      local.get 2
      local.get 1
      local.get 2
      i32.gt_u
      select
      local.tee 1
      i32.const 8
      local.get 1
      i32.const 8
      i32.gt_u
      select
      local.set 1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          br_if 0 (;@3;)
          i32.const 0
          local.set 2
          br 1 (;@2;)
        end
        local.get 3
        local.get 4
        i32.store offset=20
        local.get 3
        local.get 0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set 2
      end
      local.get 3
      local.get 2
      i32.store offset=24
      local.get 3
      local.get 1
      i32.const 1
      local.get 3
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block  ;; label = @2
        local.get 3
        i32.load
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        i32.const 8
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        i32.load offset=4
        local.get 0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get 3
      i32.load offset=4
      local.set 2
      local.get 0
      i32.const 4
      i32.add
      local.get 1
      i32.store
      local.get 0
      local.get 2
      i32.store
      local.get 3
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E (type 3) (param i32 i32) (result i32)
    (local i32 i64 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    local.get 2
    i32.const 4
    i32.add
    local.get 1
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block  ;; label = @1
      local.get 2
      i32.load8_u offset=8
      local.tee 1
      i32.const 4
      i32.eq
      br_if 0 (;@1;)
      local.get 2
      i64.load offset=8
      local.set 3
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 4
        i32.load
        local.get 4
        i32.load offset=4
        i32.load
        call_indirect (type 0)
        block  ;; label = @3
          local.get 4
          i32.load offset=4
          local.tee 5
          i32.load offset=4
          local.tee 6
          i32.eqz
          br_if 0 (;@3;)
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
      local.get 0
      local.get 3
      i64.store offset=4 align=4
    end
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1
    i32.const 4
    i32.ne)
  (func $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i64 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load
              local.tee 1
              i32.load offset=8
              br_if 0 (;@5;)
              local.get 1
              i32.const -1
              i32.store offset=8
              local.get 4
              i32.const 10
              local.get 2
              local.get 3
              call $_ZN4core5slice6memchr7memrchr17heb9113f3074b2e71E
              local.get 1
              i32.const 12
              i32.add
              local.set 5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 4
                  i32.load
                  br_if 0 (;@7;)
                  local.get 1
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee 6
                  i32.eqz
                  br_if 1 (;@6;)
                  local.get 1
                  i32.load offset=12
                  local.tee 7
                  i32.eqz
                  br_if 1 (;@6;)
                  local.get 6
                  local.get 7
                  i32.add
                  i32.const -1
                  i32.add
                  i32.load8_u
                  i32.const 10
                  i32.ne
                  br_if 1 (;@6;)
                  local.get 4
                  i32.const 8
                  i32.add
                  local.get 5
                  call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
                  local.get 4
                  i32.load8_u offset=8
                  i32.const 4
                  i32.eq
                  br_if 1 (;@6;)
                  local.get 4
                  i64.load offset=8
                  local.tee 8
                  i32.wrap_i64
                  i32.const 255
                  i32.and
                  i32.const 4
                  i32.eq
                  br_if 1 (;@6;)
                  local.get 0
                  local.get 8
                  i64.store align=4
                  br 6 (;@1;)
                end
                local.get 4
                i32.load offset=4
                i32.const 1
                i32.add
                local.tee 6
                local.get 3
                i32.gt_u
                br_if 2 (;@4;)
                block  ;; label = @7
                  local.get 1
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee 7
                  i32.eqz
                  br_if 0 (;@7;)
                  block  ;; label = @8
                    local.get 1
                    i32.const 16
                    i32.add
                    i32.load
                    local.get 7
                    i32.sub
                    local.get 6
                    i32.le_u
                    br_if 0 (;@8;)
                    local.get 1
                    i32.load offset=12
                    local.get 7
                    i32.add
                    local.get 2
                    local.get 6
                    call $memcpy
                    drop
                    local.get 1
                    i32.const 20
                    i32.add
                    local.get 7
                    local.get 6
                    i32.add
                    i32.store
                    br 5 (;@3;)
                  end
                  local.get 4
                  i32.const 8
                  i32.add
                  local.get 5
                  local.get 2
                  local.get 6
                  call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
                  local.get 4
                  i32.load8_u offset=8
                  i32.const 4
                  i32.eq
                  br_if 4 (;@3;)
                  local.get 4
                  i64.load offset=8
                  local.tee 8
                  i32.wrap_i64
                  i32.const 255
                  i32.and
                  i32.const 4
                  i32.eq
                  br_if 4 (;@3;)
                  local.get 0
                  local.get 8
                  i64.store align=4
                  br 6 (;@1;)
                end
                local.get 4
                i32.const 8
                i32.add
                local.get 5
                local.get 2
                local.get 6
                call $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E
                local.get 4
                i32.load8_u offset=8
                i32.const 4
                i32.eq
                br_if 4 (;@2;)
                local.get 4
                i64.load offset=8
                local.tee 8
                i32.wrap_i64
                i32.const 255
                i32.and
                i32.const 4
                i32.eq
                br_if 4 (;@2;)
                local.get 0
                local.get 8
                i64.store align=4
                br 5 (;@1;)
              end
              block  ;; label = @6
                local.get 1
                i32.const 16
                i32.add
                i32.load
                local.get 1
                i32.const 20
                i32.add
                local.tee 7
                i32.load
                local.tee 6
                i32.sub
                local.get 3
                i32.gt_u
                br_if 0 (;@6;)
                local.get 0
                local.get 5
                local.get 2
                local.get 3
                call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
                br 5 (;@1;)
              end
              local.get 1
              i32.load offset=12
              local.get 6
              i32.add
              local.get 2
              local.get 3
              call $memcpy
              drop
              local.get 0
              i32.const 4
              i32.store8
              local.get 7
              local.get 6
              local.get 3
              i32.add
              i32.store
              br 4 (;@1;)
            end
            i32.const 1048792
            i32.const 16
            local.get 4
            i32.const 8
            i32.add
            i32.const 1048904
            i32.const 1050576
            call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
            unreachable
          end
          i32.const 1048824
          i32.const 35
          i32.const 1049704
          call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
          unreachable
        end
        local.get 4
        i32.const 8
        i32.add
        local.get 5
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
        local.get 4
        i32.load8_u offset=8
        i32.const 4
        i32.eq
        br_if 0 (;@2;)
        local.get 4
        i64.load offset=8
        local.tee 8
        i32.wrap_i64
        i32.const 255
        i32.and
        i32.const 4
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        local.get 8
        i64.store align=4
        br 1 (;@1;)
      end
      local.get 2
      local.get 6
      i32.add
      local.set 7
      block  ;; label = @2
        local.get 1
        i32.const 16
        i32.add
        i32.load
        local.get 1
        i32.const 20
        i32.add
        local.tee 9
        i32.load
        local.tee 2
        i32.sub
        local.get 3
        local.get 6
        i32.sub
        local.tee 3
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.get 5
        local.get 7
        local.get 3
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
        br 1 (;@1;)
      end
      local.get 1
      i32.load offset=12
      local.get 2
      i32.add
      local.get 7
      local.get 3
      call $memcpy
      drop
      local.get 0
      i32.const 4
      i32.store8
      local.get 9
      local.get 2
      local.get 3
      i32.add
      i32.store
    end
    local.get 1
    local.get 1
    i32.load offset=8
    i32.const 1
    i32.add
    i32.store offset=8
    local.get 4
    i32.const 16
    i32.add
    global.set $__stack_pointer)
  (func $_ZN4core3fmt5Write10write_char17hbe9f53f72de9fff4E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    i32.const 0
    local.set 4
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          loop  ;; label = @4
            local.get 3
            local.get 2
            i32.store offset=12
            local.get 3
            local.get 1
            i32.store offset=8
            local.get 3
            i32.const 16
            i32.add
            i32.const 2
            local.get 3
            i32.const 8
            i32.add
            i32.const 1
            call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  i32.load16_u offset=16
                  br_if 0 (;@7;)
                  local.get 3
                  i32.load offset=20
                  local.tee 5
                  br_if 1 (;@6;)
                  i32.const 1050536
                  local.set 5
                  i64.const 2
                  local.set 6
                  br 5 (;@2;)
                end
                local.get 3
                local.get 3
                i32.load16_u offset=18
                i32.store16 offset=30
                local.get 3
                i32.const 30
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee 5
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if 1 (;@5;)
                i64.const 0
                local.set 6
                br 4 (;@2;)
              end
              local.get 2
              local.get 5
              i32.lt_u
              br_if 2 (;@3;)
              local.get 1
              local.get 5
              i32.add
              local.set 1
              local.get 2
              local.get 5
              i32.sub
              local.set 2
            end
            local.get 2
            br_if 0 (;@4;)
            br 3 (;@1;)
          end
        end
        local.get 5
        local.get 2
        i32.const 1050692
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get 5
      i64.extend_i32_u
      i64.const 32
      i64.shl
      local.get 6
      i64.or
      local.set 6
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 2
        i32.load
        local.get 2
        i32.load offset=4
        i32.load
        call_indirect (type 0)
        block  ;; label = @3
          local.get 2
          i32.load offset=4
          local.tee 1
          i32.load offset=4
          local.tee 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          i32.load
          local.get 5
          local.get 1
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 2
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 0
      local.get 6
      i64.store offset=4 align=4
      i32.const 1
      local.set 4
    end
    local.get 3
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 4)
  (func $_ZN4core3fmt5Write9write_fmt17h8a28623dee2384e2E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048696
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN4core3fmt5Write9write_fmt17h9501b0d1bd73714aE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048720
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN4core3fmt5Write9write_fmt17hcb1cb3e9d099f402E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048744
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN3std9panicking12default_hook17h9e7a62be9c47f791E (type 0) (param i32)
    (local i32 i32 i64 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 96
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    i32.const 1
    local.set 2
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059304
      i32.const 1
      i32.gt_u
      br_if 0 (;@1;)
      call $_ZN3std5panic19get_backtrace_style17h5dc7902a5c3a80f3E
      i32.const 255
      i32.and
      local.set 2
    end
    local.get 1
    local.get 2
    i32.store8 offset=27
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                call $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE
                local.tee 2
                i32.eqz
                br_if 0 (;@6;)
                local.get 1
                local.get 2
                i32.store offset=28
                local.get 1
                i32.const 16
                i32.add
                local.get 0
                call $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E
                local.get 1
                i32.load offset=16
                local.tee 2
                local.get 1
                i32.load offset=20
                i32.load offset=12
                call_indirect (type 1)
                local.set 3
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 2
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 3
                      i64.const -5139102199292759541
                      i64.eq
                      br_if 1 (;@8;)
                    end
                    local.get 1
                    i32.const 8
                    i32.add
                    local.get 0
                    call $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E
                    i32.const 1051500
                    local.set 4
                    i32.const 12
                    local.set 0
                    local.get 1
                    i32.load offset=8
                    local.tee 2
                    local.get 1
                    i32.load offset=12
                    i32.load offset=12
                    call_indirect (type 1)
                    local.set 3
                    block  ;; label = @9
                      local.get 2
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 3
                      i64.const 7628363314155272196
                      i64.ne
                      br_if 0 (;@9;)
                      local.get 2
                      i32.const 8
                      i32.add
                      i32.load
                      local.set 0
                      local.get 2
                      i32.load
                      local.set 4
                    end
                    local.get 1
                    local.get 4
                    i32.store offset=32
                    br 1 (;@7;)
                  end
                  local.get 1
                  local.get 2
                  i32.load
                  i32.store offset=32
                  local.get 2
                  i32.load offset=4
                  local.set 0
                end
                local.get 1
                local.get 0
                i32.store offset=36
                i32.const 0
                i32.load offset=1059296
                br_if 1 (;@5;)
                i32.const 0
                i32.const -1
                i32.store offset=1059296
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1059300
                  local.tee 0
                  br_if 0 (;@7;)
                  i32.const 0
                  i32.const 0
                  local.get 1
                  call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                  local.tee 0
                  i32.store offset=1059300
                end
                local.get 0
                local.get 0
                i32.load
                local.tee 2
                i32.const 1
                i32.add
                i32.store
                local.get 2
                i32.const -1
                i32.le_s
                br_if 2 (;@4;)
                i32.const 0
                i32.const 0
                i32.load offset=1059296
                i32.const 1
                i32.add
                i32.store offset=1059296
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 0
                    br_if 0 (;@8;)
                    i32.const 0
                    local.set 2
                    br 1 (;@7;)
                  end
                  local.get 0
                  i32.const 20
                  i32.add
                  i32.load
                  i32.const -1
                  i32.add
                  local.set 4
                  local.get 0
                  i32.load offset=16
                  local.set 2
                end
                local.get 1
                local.get 4
                i32.const 9
                local.get 2
                select
                i32.store offset=44
                local.get 1
                local.get 2
                i32.const 1051512
                local.get 2
                select
                i32.store offset=40
                local.get 1
                local.get 1
                i32.const 27
                i32.add
                i32.store offset=60
                local.get 1
                local.get 1
                i32.const 28
                i32.add
                i32.store offset=56
                local.get 1
                local.get 1
                i32.const 32
                i32.add
                i32.store offset=52
                local.get 1
                local.get 1
                i32.const 40
                i32.add
                i32.store offset=48
                block  ;; label = @7
                  i32.const 0
                  i32.load8_u offset=1059218
                  i32.eqz
                  br_if 0 (;@7;)
                  i32.const 0
                  i32.const 1
                  i32.store8 offset=1059218
                  block  ;; label = @8
                    i32.const 0
                    i32.load offset=1059284
                    br_if 0 (;@8;)
                    i32.const 0
                    i64.const 1
                    i64.store offset=1059284 align=4
                    br 1 (;@7;)
                  end
                  i32.const 0
                  i32.load offset=1059288
                  local.set 2
                  i32.const 0
                  i32.const 0
                  i32.store offset=1059288
                  local.get 2
                  br_if 4 (;@3;)
                end
                local.get 1
                i32.const 48
                i32.add
                local.get 1
                i32.const 72
                i32.add
                i32.const 1051524
                call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
                i32.const 0
                local.set 4
                i32.const 0
                local.set 2
                br 4 (;@2;)
              end
              i32.const 1048859
              i32.const 43
              i32.const 1051484
              call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
              unreachable
            end
            i32.const 1048792
            i32.const 16
            local.get 1
            i32.const 72
            i32.add
            i32.const 1048904
            i32.const 1051348
            call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
            unreachable
          end
          unreachable
          unreachable
        end
        local.get 2
        i32.load8_u offset=8
        local.set 4
        local.get 2
        i32.const 1
        i32.store8 offset=8
        local.get 1
        local.get 4
        i32.const 1
        i32.and
        local.tee 4
        i32.store8 offset=71
        local.get 4
        br_if 1 (;@1;)
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              i32.load offset=1059280
              i32.const 2147483647
              i32.and
              br_if 0 (;@5;)
              local.get 1
              i32.const 48
              i32.add
              local.get 2
              i32.const 12
              i32.add
              i32.const 1051564
              call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
              br 1 (;@4;)
            end
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            local.set 4
            local.get 1
            i32.const 48
            i32.add
            local.get 2
            i32.const 12
            i32.add
            i32.const 1051564
            call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
            local.get 4
            i32.eqz
            br_if 1 (;@3;)
          end
          i32.const 0
          i32.load offset=1059280
          i32.const 2147483647
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
          br_if 0 (;@3;)
          local.get 2
          i32.const 1
          i32.store8 offset=9
        end
        i32.const 1
        local.set 4
        i32.const 0
        i32.const 1
        i32.store8 offset=1059218
        local.get 2
        i32.const 0
        i32.store8 offset=8
        block  ;; label = @3
          i32.const 0
          i32.load offset=1059284
          br_if 0 (;@3;)
          i32.const 0
          local.get 2
          i32.store offset=1059288
          i32.const 1
          local.set 4
          i32.const 0
          i32.const 1
          i32.store offset=1059284
          br 1 (;@2;)
        end
        i32.const 0
        i32.load offset=1059288
        local.set 5
        i32.const 0
        local.get 2
        i32.store offset=1059288
        local.get 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 5
        local.get 5
        i32.load
        local.tee 6
        i32.const -1
        i32.add
        i32.store
        i32.const 1
        local.set 4
        local.get 6
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 5
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
      end
      block  ;; label = @2
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 0
        i32.load
        local.tee 5
        i32.const -1
        i32.add
        i32.store
        local.get 5
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
      end
      block  ;; label = @2
        local.get 4
        i32.const -1
        i32.xor
        local.get 2
        i32.const 0
        i32.ne
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        local.get 2
        i32.load
        local.tee 0
        i32.const -1
        i32.add
        i32.store
        local.get 0
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
      end
      local.get 1
      i32.const 96
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 1
    i32.const 92
    i32.add
    i32.const 0
    i32.store
    local.get 1
    i32.const 88
    i32.add
    i32.const 1048792
    i32.store
    local.get 1
    i64.const 1
    i64.store offset=76 align=4
    local.get 1
    i32.const 1052308
    i32.store offset=72
    local.get 1
    i32.const 71
    i32.add
    local.get 1
    i32.const 72
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 2
    i32.const 1049552
    i32.store offset=4
    local.get 2
    local.get 0
    i32.store
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
    i32.const 0
    local.get 2
    i32.const 1048996
    local.get 2
    i32.const 4
    i32.add
    i32.const 1048996
    local.get 2
    i32.const 8
    i32.add
    i32.const 1052372
    call $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E
    unreachable)
  (func $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17hd87567c1af08f732E (type 2) (param i32 i32)
    (local i32)
    local.get 0
    i32.load
    local.tee 2
    i32.load
    local.set 0
    local.get 2
    i32.const 0
    i32.store
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1024
        i32.const 1
        call $__rust_alloc
        local.tee 2
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 0
        i32.store8 offset=28
        local.get 0
        i32.const 0
        i32.store8 offset=24
        local.get 0
        i64.const 1024
        i64.store offset=16 align=4
        local.get 0
        local.get 2
        i32.store offset=12
        local.get 0
        i32.const 0
        i32.store offset=8
        local.get 0
        i64.const 0
        i64.store align=4
        return
      end
      i32.const 1048859
      i32.const 43
      i32.const 1050912
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    i32.const 1024
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN4core3ptr100drop_in_place$LT$$RF$mut$u20$std..io..Write..write_fmt..Adapter$LT$alloc..vec..Vec$LT$u8$GT$$GT$$GT$17hf1b3c7767b0b7765E (type 0) (param i32))
  (func $_ZN4core3ptr103drop_in_place$LT$std..sync..poison..PoisonError$LT$std..sync..mutex..MutexGuard$LT$$LP$$RP$$GT$$GT$$GT$17h342b8354fe7ce8f5E (type 0) (param i32)
    (local i32)
    local.get 0
    i32.load
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      i32.const 0
      i32.load offset=1059280
      i32.const 2147483647
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
      br_if 0 (;@1;)
      local.get 1
      i32.const 1
      i32.store8 offset=1
    end
    local.get 1
    i32.const 0
    i32.store8)
  (func $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE (type 9) (result i32)
    i32.const 0
    i32.load offset=1059304
    i32.eqz)
  (func $_ZN4core3ptr226drop_in_place$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Send$u2b$core..marker..Sync$GT$$GT$..from..StringError$GT$17h8f80eb1a61eb416eE (type 0) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load
      local.get 1
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE (type 0) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.load offset=16
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.const 0
      i32.store8
      local.get 0
      i32.const 20
      i32.add
      i32.load
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=16
      local.get 1
      i32.const 1
      call $__rust_dealloc
    end
    block  ;; label = @1
      local.get 0
      i32.const -1
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      local.get 0
      i32.load offset=4
      local.tee 1
      i32.const -1
      i32.add
      i32.store offset=4
      local.get 1
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      local.get 0
      i32.const 32
      i32.const 8
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr70drop_in_place$LT$std..panicking..begin_panic_handler..PanicPayload$GT$17ha86c38a5fb14f016E (type 0) (param i32)
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
  (func $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE (type 0) (param i32)
    (local i32 i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load8_u
      i32.const 3
      i32.ne
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 1
      i32.load
      local.get 1
      i32.load offset=4
      i32.load
      call_indirect (type 0)
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
      i32.load offset=4
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr87drop_in_place$LT$std..io..Write..write_fmt..Adapter$LT$$RF$mut$u20$$u5b$u8$u5d$$GT$$GT$17h9e512fc24cfce3e7E (type 0) (param i32)
    (local i32 i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      i32.const 3
      i32.ne
      br_if 0 (;@1;)
      local.get 0
      i32.const 8
      i32.add
      i32.load
      local.tee 1
      i32.load
      local.get 1
      i32.load offset=4
      i32.load
      call_indirect (type 0)
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
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17h38e7befd79b1f826E (type 3) (param i32 i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      i32.const 1048859
      i32.const 43
      local.get 1
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get 0)
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17h88efffa862cbe9d0E (type 10) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      br_if 0 (;@1;)
      i32.const 1048859
      i32.const 43
      i32.const 1051768
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get 0)
  (func $_ZN4core9panicking13assert_failed17he78e5fdda9a074e5E (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 1051044
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 3
    local.get 1
    i64.load align=4
    i64.store offset=8
    i32.const 0
    local.get 3
    i32.const 1049012
    local.get 3
    i32.const 4
    i32.add
    i32.const 1049012
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E
    unreachable)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h2ffc04b3c6716e0dE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5a3b0091f8192a8fE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN58_$LT$alloc..string..String$u20$as$u20$core..fmt..Write$GT$10write_char17h5e45316a4ac7182cE
    drop
    i32.const 0)
  (func $_ZN58_$LT$alloc..string..String$u20$as$u20$core..fmt..Write$GT$10write_char17h5e45316a4ac7182cE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 128
              i32.lt_u
              br_if 0 (;@5;)
              local.get 2
              i32.const 0
              i32.store offset=12
              local.get 1
              i32.const 2048
              i32.lt_u
              br_if 1 (;@4;)
              local.get 1
              i32.const 65536
              i32.ge_u
              br_if 2 (;@3;)
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
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 0
              i32.load offset=8
              local.tee 3
              local.get 0
              i32.const 4
              i32.add
              i32.load
              i32.ne
              br_if 0 (;@5;)
              local.get 0
              local.get 3
              call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h886fbd751427a748E
              local.get 0
              i32.load offset=8
              local.set 3
            end
            local.get 0
            local.get 3
            i32.const 1
            i32.add
            i32.store offset=8
            local.get 0
            i32.load
            local.get 3
            i32.add
            local.get 1
            i32.store8
            br 3 (;@1;)
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
          i32.const 2
          local.set 1
          br 1 (;@2;)
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
      block  ;; label = @2
        local.get 0
        i32.const 4
        i32.add
        i32.load
        local.get 0
        i32.const 8
        i32.add
        local.tee 4
        i32.load
        local.tee 3
        i32.sub
        local.get 1
        i32.ge_u
        br_if 0 (;@2;)
        local.get 0
        local.get 3
        local.get 1
        call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
        local.get 4
        i32.load
        local.set 3
      end
      local.get 0
      i32.load
      local.get 3
      i32.add
      local.get 2
      i32.const 12
      i32.add
      local.get 1
      call $memcpy
      drop
      local.get 4
      local.get 3
      local.get 1
      i32.add
      i32.store
    end
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    i32.const 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h9896f020280c35deE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17hbc3d3931408e41d6E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h47c6b3c5d318955aE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048720
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h8027bf4984b5aaecE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048768
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17he4a59f9604beea94E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048744
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hf0a5b3c4ac0d0496E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1048696
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h35232905e30669aaE (type 5) (param i32 i32 i32) (result i32)
    (local i32 i64 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.tee 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      local.tee 1
      i32.const 4
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 2
        i32.load
        local.get 2
        i32.load offset=4
        i32.load
        call_indirect (type 0)
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
      local.get 4
      i64.store offset=4 align=4
    end
    local.get 3
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1
    i32.const 4
    i32.ne)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5e136cc95a131902E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load
      i32.load
      local.tee 3
      i32.const 4
      i32.add
      i32.load
      local.get 3
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 0
      i32.sub
      local.get 2
      i32.ge_u
      br_if 0 (;@1;)
      local.get 3
      local.get 0
      local.get 2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 0
    end
    local.get 3
    i32.load
    local.get 0
    i32.add
    local.get 1
    local.get 2
    call $memcpy
    drop
    local.get 4
    local.get 0
    local.get 2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h7f904ebe63e601aaE (type 5) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17he56d425d213d6944E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 3
      i32.const 4
      i32.add
      i32.load
      local.get 3
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 0
      i32.sub
      local.get 2
      i32.ge_u
      br_if 0 (;@1;)
      local.get 3
      local.get 0
      local.get 2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 0
    end
    local.get 3
    i32.load
    local.get 0
    i32.add
    local.get 1
    local.get 2
    call $memcpy
    drop
    local.get 4
    local.get 0
    local.get 2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h886fbd751427a748E (type 2) (param i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      local.get 1
      i32.const 1
      i32.add
      local.tee 3
      local.get 1
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 4
      i32.const 1
      i32.shl
      local.tee 1
      local.get 3
      local.get 1
      local.get 3
      i32.gt_u
      select
      local.tee 1
      i32.const 8
      local.get 1
      i32.const 8
      i32.gt_u
      select
      local.set 1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          br_if 0 (;@3;)
          i32.const 0
          local.set 3
          br 1 (;@2;)
        end
        local.get 2
        local.get 4
        i32.store offset=20
        local.get 2
        local.get 0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set 3
      end
      local.get 2
      local.get 3
      i32.store offset=24
      local.get 2
      local.get 1
      i32.const 1
      local.get 2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block  ;; label = @2
        local.get 2
        i32.load
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.const 8
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.load offset=4
        local.get 0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get 2
      i32.load offset=4
      local.set 3
      local.get 0
      i32.const 4
      i32.add
      local.get 1
      i32.store
      local.get 0
      local.get 3
      i32.store
      local.get 2
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E (type 0) (param i32)
    (local i32)
    block  ;; label = @1
      local.get 0
      i32.const 16
      i32.add
      i32.load
      local.tee 1
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=12
      local.get 1
      i32.const 1
      call $__rust_dealloc
    end
    block  ;; label = @1
      local.get 0
      i32.const -1
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      local.get 0
      i32.load offset=4
      local.tee 1
      i32.const -1
      i32.add
      i32.store offset=4
      local.get 1
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      local.get 0
      i32.const 24
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E (type 8) (param i32 i32 i32 i32)
    (local i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 2
                  i32.eqz
                  br_if 0 (;@7;)
                  i32.const 1
                  local.set 4
                  local.get 1
                  i32.const 0
                  i32.lt_s
                  br_if 1 (;@6;)
                  local.get 3
                  i32.load offset=8
                  i32.eqz
                  br_if 3 (;@4;)
                  local.get 3
                  i32.load offset=4
                  local.tee 5
                  br_if 2 (;@5;)
                  local.get 1
                  br_if 4 (;@3;)
                  local.get 2
                  local.set 3
                  br 5 (;@2;)
                end
                local.get 0
                local.get 1
                i32.store offset=4
                i32.const 1
                local.set 4
              end
              i32.const 0
              local.set 1
              br 4 (;@1;)
            end
            local.get 3
            i32.load
            local.get 5
            local.get 2
            local.get 1
            call $__rust_realloc
            local.set 3
            br 2 (;@2;)
          end
          local.get 1
          br_if 0 (;@3;)
          local.get 2
          local.set 3
          br 1 (;@2;)
        end
        local.get 1
        local.get 2
        call $__rust_alloc
        local.set 3
      end
      block  ;; label = @2
        local.get 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 3
        i32.store offset=4
        i32.const 0
        local.set 4
        br 1 (;@1;)
      end
      local.get 0
      local.get 1
      i32.store offset=4
      local.get 2
      local.set 1
    end
    local.get 0
    local.get 4
    i32.store
    local.get 0
    i32.const 8
    i32.add
    local.get 1
    i32.store)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h4285ebb9278a5d57E (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      local.get 1
      i32.const 1
      i32.add
      local.tee 3
      local.get 1
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 4
      local.set 4
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 1
      i32.const 1
      i32.shl
      local.tee 5
      local.get 3
      local.get 5
      local.get 3
      i32.gt_u
      select
      local.tee 3
      i32.const 4
      local.get 3
      i32.const 4
      i32.gt_u
      select
      local.tee 6
      i64.extend_i32_u
      i64.const 24
      i64.mul
      local.tee 7
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.eqz
      i32.const 2
      i32.shl
      local.set 3
      local.get 7
      i32.wrap_i64
      local.set 5
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          br_if 0 (;@3;)
          i32.const 0
          local.set 4
          br 1 (;@2;)
        end
        local.get 2
        local.get 0
        i32.load
        i32.store offset=16
        local.get 2
        local.get 1
        i64.extend_i32_u
        i64.const 24
        i64.mul
        i64.store32 offset=20
      end
      local.get 2
      local.get 4
      i32.store offset=24
      local.get 2
      local.get 5
      local.get 3
      local.get 2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block  ;; label = @2
        local.get 2
        i32.load
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.const 8
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.load offset=4
        local.get 0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get 2
      i32.load offset=4
      local.set 1
      local.get 0
      i32.const 4
      i32.add
      local.get 6
      i32.store
      local.get 0
      local.get 1
      i32.store
      local.get 2
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h49fd6398be4a4a9aE (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      local.get 1
      i32.const 1
      i32.add
      local.tee 3
      local.get 1
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 4
      local.set 4
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 1
      i32.const 1
      i32.shl
      local.tee 5
      local.get 3
      local.get 5
      local.get 3
      i32.gt_u
      select
      local.tee 3
      i32.const 4
      local.get 3
      i32.const 4
      i32.gt_u
      select
      local.tee 6
      i64.extend_i32_u
      i64.const 12
      i64.mul
      local.tee 7
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.eqz
      i32.const 2
      i32.shl
      local.set 3
      local.get 7
      i32.wrap_i64
      local.set 5
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          br_if 0 (;@3;)
          i32.const 0
          local.set 4
          br 1 (;@2;)
        end
        local.get 2
        local.get 0
        i32.load
        i32.store offset=16
        local.get 2
        local.get 1
        i64.extend_i32_u
        i64.const 12
        i64.mul
        i64.store32 offset=20
      end
      local.get 2
      local.get 4
      i32.store offset=24
      local.get 2
      local.get 5
      local.get 3
      local.get 2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block  ;; label = @2
        local.get 2
        i32.load
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.const 8
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.load offset=4
        local.get 0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get 2
      i32.load offset=4
      local.set 1
      local.get 0
      i32.const 4
      i32.add
      local.get 6
      i32.store
      local.get 0
      local.get 1
      i32.store
      local.get 2
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E.1 (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.const 8
    i32.add
    i32.load
    local.get 1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $_ZN3std4sync4once4Once10call_inner17h90795951c1d06984E (type 11) (param i32 i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 5
    global.set $__stack_pointer
    local.get 5
    i32.const 8
    i32.add
    i32.const 2
    i32.or
    local.set 6
    local.get 0
    i32.load
    local.set 7
    loop  ;; label = @1
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
                                  local.get 7
                                  br_table 1 (;@14;) 0 (;@15;) 2 (;@13;) 5 (;@10;) 2 (;@13;)
                                end
                                local.get 1
                                i32.eqz
                                br_if 2 (;@12;)
                              end
                              local.get 0
                              i32.const 2
                              local.get 0
                              i32.load
                              local.tee 8
                              local.get 8
                              local.get 7
                              i32.eq
                              local.tee 9
                              select
                              i32.store
                              local.get 9
                              br_if 2 (;@11;)
                              local.get 8
                              local.set 7
                              br 12 (;@1;)
                            end
                            block  ;; label = @13
                              local.get 7
                              i32.const 3
                              i32.and
                              i32.const 2
                              i32.ne
                              br_if 0 (;@13;)
                              loop  ;; label = @14
                                local.get 7
                                local.set 9
                                i32.const 0
                                i32.load offset=1059296
                                br_if 5 (;@9;)
                                i32.const 0
                                i32.const -1
                                i32.store offset=1059296
                                block  ;; label = @15
                                  i32.const 0
                                  i32.load offset=1059300
                                  local.tee 8
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  i32.const 0
                                  local.get 7
                                  call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                                  local.tee 8
                                  i32.store offset=1059300
                                end
                                local.get 8
                                local.get 8
                                i32.load
                                local.tee 7
                                i32.const 1
                                i32.add
                                i32.store
                                local.get 7
                                i32.const -1
                                i32.le_s
                                br_if 6 (;@8;)
                                i32.const 0
                                i32.const 0
                                i32.load offset=1059296
                                i32.const 1
                                i32.add
                                i32.store offset=1059296
                                local.get 8
                                i32.eqz
                                br_if 7 (;@7;)
                                local.get 0
                                local.get 6
                                local.get 0
                                i32.load
                                local.tee 7
                                local.get 7
                                local.get 9
                                i32.eq
                                select
                                i32.store
                                local.get 5
                                i32.const 0
                                i32.store8 offset=16
                                local.get 5
                                local.get 8
                                i32.store offset=8
                                local.get 5
                                local.get 9
                                i32.const -4
                                i32.and
                                i32.store offset=12
                                block  ;; label = @15
                                  local.get 7
                                  local.get 9
                                  i32.ne
                                  br_if 0 (;@15;)
                                  local.get 5
                                  i32.load8_u offset=16
                                  i32.eqz
                                  br_if 9 (;@6;)
                                  br 12 (;@3;)
                                end
                                block  ;; label = @15
                                  local.get 5
                                  i32.load offset=8
                                  local.tee 8
                                  i32.eqz
                                  br_if 0 (;@15;)
                                  local.get 8
                                  local.get 8
                                  i32.load
                                  local.tee 9
                                  i32.const -1
                                  i32.add
                                  i32.store
                                  local.get 9
                                  i32.const 1
                                  i32.ne
                                  br_if 0 (;@15;)
                                  local.get 5
                                  i32.load offset=8
                                  call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                                end
                                local.get 7
                                i32.const 3
                                i32.and
                                i32.const 2
                                i32.eq
                                br_if 0 (;@14;)
                                br 12 (;@2;)
                              end
                            end
                            i32.const 1050928
                            i32.const 64
                            local.get 4
                            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
                            unreachable
                          end
                          local.get 5
                          i32.const 28
                          i32.add
                          i32.const 0
                          i32.store
                          local.get 5
                          i32.const 1048792
                          i32.store offset=24
                          local.get 5
                          i64.const 1
                          i64.store offset=12 align=4
                          local.get 5
                          i32.const 1051036
                          i32.store offset=8
                          local.get 5
                          i32.const 8
                          i32.add
                          local.get 4
                          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
                          unreachable
                        end
                        local.get 5
                        local.get 7
                        i32.const 1
                        i32.eq
                        i32.store8 offset=12
                        local.get 5
                        i32.const 3
                        i32.store offset=8
                        local.get 2
                        local.get 5
                        i32.const 8
                        i32.add
                        local.get 3
                        i32.load offset=16
                        call_indirect (type 2)
                        local.get 0
                        i32.load
                        local.set 7
                        local.get 0
                        local.get 5
                        i32.load offset=8
                        i32.store
                        local.get 5
                        local.get 7
                        i32.const 3
                        i32.and
                        local.tee 8
                        i32.store
                        local.get 8
                        i32.const 2
                        i32.ne
                        br_if 5 (;@5;)
                        local.get 7
                        i32.const -2
                        i32.add
                        local.tee 8
                        i32.eqz
                        br_if 0 (;@10;)
                        loop  ;; label = @11
                          local.get 8
                          i32.load
                          local.set 7
                          local.get 8
                          i32.const 0
                          i32.store
                          local.get 7
                          i32.eqz
                          br_if 7 (;@4;)
                          local.get 8
                          i32.load offset=4
                          local.set 9
                          local.get 8
                          i32.const 1
                          i32.store8 offset=8
                          local.get 7
                          i32.const 24
                          i32.add
                          call $_ZN3std10sys_common13thread_parker7generic6Parker6unpark17h41bf5bed244e8e6fE
                          local.get 7
                          local.get 7
                          i32.load
                          local.tee 8
                          i32.const -1
                          i32.add
                          i32.store
                          block  ;; label = @12
                            local.get 8
                            i32.const 1
                            i32.ne
                            br_if 0 (;@12;)
                            local.get 7
                            call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                          end
                          local.get 9
                          local.set 8
                          local.get 9
                          br_if 0 (;@11;)
                        end
                      end
                      local.get 5
                      i32.const 32
                      i32.add
                      global.set $__stack_pointer
                      return
                    end
                    i32.const 1048792
                    i32.const 16
                    local.get 5
                    i32.const 1048904
                    i32.const 1051348
                    call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
                    unreachable
                  end
                  unreachable
                  unreachable
                end
                i32.const 1049200
                i32.const 94
                i32.const 1049324
                call $_ZN4core6option13expect_failed17h8d17c0af9e73185dE
                unreachable
              end
              loop  ;; label = @6
                call $_ZN3std6thread4park17h401b5f43fa6ef902E
                local.get 5
                i32.load8_u offset=16
                i32.eqz
                br_if 0 (;@6;)
                br 3 (;@3;)
              end
            end
            local.get 5
            i32.const 0
            i32.store offset=8
            local.get 5
            local.get 5
            i32.const 8
            i32.add
            i32.const 1051048
            call $_ZN4core9panicking13assert_failed17he78e5fdda9a074e5E
            unreachable
          end
          i32.const 1048859
          i32.const 43
          i32.const 1051064
          call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
          unreachable
        end
        local.get 5
        i32.load offset=8
        local.tee 7
        i32.eqz
        br_if 0 (;@2;)
        local.get 7
        local.get 7
        i32.load
        local.tee 8
        i32.const -1
        i32.add
        i32.store
        local.get 8
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        local.get 5
        i32.load offset=8
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
        local.get 0
        i32.load
        local.set 7
        br 1 (;@1;)
      end
      local.get 0
      i32.load
      local.set 7
      br 0 (;@1;)
    end)
  (func $_ZN3std6thread6Thread3new17hd168535b42058e7aE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i64)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        i32.const 32
        i32.const 8
        call $__rust_alloc
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 0
        i32.store offset=16
        local.get 3
        i64.const 4294967297
        i64.store
        local.get 3
        i32.const 20
        i32.add
        local.get 1
        i32.store
        i32.const 0
        i32.load8_u offset=1059217
        local.set 0
        i32.const 0
        i32.const 1
        i32.store8 offset=1059217
        local.get 2
        local.get 0
        i32.store8 offset=7
        local.get 0
        br_if 1 (;@1;)
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i64.load offset=1059200
            local.tee 4
            i64.const -1
            i64.eq
            br_if 0 (;@4;)
            i32.const 0
            local.get 4
            i64.const 1
            i64.add
            i64.store offset=1059200
            local.get 4
            i64.const 0
            i64.ne
            br_if 1 (;@3;)
            i32.const 1048859
            i32.const 43
            i32.const 1049420
            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
            unreachable
          end
          i32.const 0
          i32.const 0
          i32.store8 offset=1059217
          local.get 2
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get 2
          i32.const 1048792
          i32.store offset=24
          local.get 2
          i64.const 1
          i64.store offset=12 align=4
          local.get 2
          i32.const 1049396
          i32.store offset=8
          local.get 2
          i32.const 8
          i32.add
          i32.const 1049404
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 3
        i64.const 0
        i64.store offset=24
        local.get 3
        local.get 4
        i64.store offset=8
        i32.const 0
        i32.const 0
        i32.store8 offset=1059217
        local.get 2
        i32.const 32
        i32.add
        global.set $__stack_pointer
        local.get 3
        return
      end
      i32.const 32
      i32.const 8
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get 2
    i32.const 8
    i32.add
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get 2
    i32.const 24
    i32.add
    i32.const 1048792
    i32.store
    local.get 2
    i64.const 1
    i64.store offset=12 align=4
    local.get 2
    i32.const 1052308
    i32.store offset=8
    local.get 2
    i32.const 7
    i32.add
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 4
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
        local.get 3
        i32.const 8
        i32.add
        i32.const 1050736
        local.get 3
        i32.const 24
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 3
          i32.load8_u offset=12
          i32.const 4
          i32.ne
          br_if 0 (;@3;)
          local.get 0
          i32.const 1050724
          i64.extend_i32_u
          i64.const 32
          i64.shl
          i64.const 2
          i64.or
          i64.store align=4
          br 2 (;@1;)
        end
        local.get 0
        local.get 3
        i64.load offset=12 align=4
        i64.store align=4
        br 1 (;@1;)
      end
      local.get 0
      i32.const 4
      i32.store8
      local.get 3
      i32.load8_u offset=12
      i32.const 3
      i32.ne
      br_if 0 (;@1;)
      local.get 3
      i32.const 16
      i32.add
      i32.load
      local.tee 2
      i32.load
      local.get 2
      i32.load offset=4
      i32.load
      call_indirect (type 0)
      block  ;; label = @2
        local.get 2
        i32.load offset=4
        local.tee 1
        i32.load offset=4
        local.tee 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.load
        local.get 0
        local.get 1
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
    global.set $__stack_pointer)
  (func $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE (type 7)
    call $abort
    unreachable)
  (func $_ZN3std10sys_common13thread_parker7generic6Parker6unpark17h41bf5bed244e8e6fE (type 0) (param i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    local.get 0
    i32.load
    local.set 2
    local.get 0
    i32.const 2
    i32.store
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            br_table 2 (;@2;) 1 (;@3;) 2 (;@2;) 0 (;@4;)
          end
          local.get 1
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get 1
          i32.const 1048792
          i32.store offset=24
          local.get 1
          i64.const 1
          i64.store offset=12 align=4
          local.get 1
          i32.const 1052828
          i32.store offset=8
          local.get 1
          i32.const 8
          i32.add
          i32.const 1052836
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 0
        i32.load8_u offset=4
        local.set 2
        local.get 0
        i32.const 1
        i32.store8 offset=4
        local.get 1
        local.get 2
        i32.const 1
        i32.and
        local.tee 2
        i32.store8 offset=7
        local.get 2
        br_if 1 (;@1;)
        local.get 0
        i32.const 4
        i32.add
        local.set 0
        i32.const 0
        local.set 2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1059280
                  i32.const 2147483647
                  i32.and
                  i32.eqz
                  br_if 0 (;@7;)
                  call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                  local.set 2
                  local.get 0
                  i32.load8_u offset=1
                  i32.eqz
                  br_if 2 (;@5;)
                  local.get 2
                  i32.const 1
                  i32.xor
                  local.set 2
                  br 1 (;@6;)
                end
                local.get 0
                i32.load8_u offset=1
                i32.eqz
                br_if 2 (;@4;)
              end
              local.get 1
              local.get 2
              i32.store8 offset=12
              local.get 1
              local.get 0
              i32.store offset=8
              i32.const 1048920
              i32.const 43
              local.get 1
              i32.const 8
              i32.add
              i32.const 1048964
              i32.const 1052852
              call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
              unreachable
            end
            local.get 2
            i32.eqz
            br_if 1 (;@3;)
          end
          i32.const 0
          i32.load offset=1059280
          i32.const 2147483647
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
          br_if 0 (;@3;)
          local.get 0
          i32.const 1
          i32.store8 offset=1
        end
        local.get 0
        i32.const 0
        i32.store8
      end
      local.get 1
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 1
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get 1
    i32.const 24
    i32.add
    i32.const 1048792
    i32.store
    local.get 1
    i64.const 1
    i64.store offset=12 align=4
    local.get 1
    i32.const 1052308
    i32.store offset=8
    local.get 1
    i32.const 7
    i32.add
    local.get 1
    i32.const 8
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN3std6thread4park17h401b5f43fa6ef902E (type 7)
    (local i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 0
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    i32.const 0
                    i32.load offset=1059296
                    br_if 0 (;@8;)
                    i32.const 0
                    i32.const -1
                    i32.store offset=1059296
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1059300
                      local.tee 1
                      br_if 0 (;@9;)
                      i32.const 0
                      i32.const 0
                      local.get 1
                      call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                      local.tee 1
                      i32.store offset=1059300
                    end
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
                    br_if 1 (;@7;)
                    i32.const 0
                    i32.const 0
                    i32.load offset=1059296
                    i32.const 1
                    i32.add
                    i32.store offset=1059296
                    local.get 1
                    i32.eqz
                    br_if 2 (;@6;)
                    local.get 1
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
                    block  ;; label = @9
                      local.get 2
                      br_if 0 (;@9;)
                      local.get 1
                      i32.const 24
                      i32.add
                      local.tee 2
                      i32.load8_u offset=4
                      local.set 3
                      local.get 2
                      i32.const 1
                      i32.store8 offset=4
                      local.get 0
                      local.get 3
                      i32.const 1
                      i32.and
                      local.tee 3
                      i32.store8 offset=4
                      local.get 3
                      br_if 4 (;@5;)
                      local.get 2
                      i32.const 4
                      i32.add
                      local.set 4
                      i32.const 0
                      local.set 5
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1059280
                        i32.const 2147483647
                        i32.and
                        i32.eqz
                        br_if 0 (;@10;)
                        call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                        i32.const 1
                        i32.xor
                        local.set 5
                      end
                      local.get 4
                      i32.load8_u offset=1
                      br_if 5 (;@4;)
                      local.get 2
                      local.get 2
                      i32.load
                      local.tee 3
                      i32.const 1
                      local.get 3
                      select
                      i32.store
                      local.get 3
                      i32.eqz
                      br_if 8 (;@1;)
                      local.get 3
                      i32.const 2
                      i32.ne
                      br_if 6 (;@3;)
                      local.get 2
                      i32.load
                      local.set 3
                      local.get 2
                      i32.const 0
                      i32.store
                      local.get 0
                      local.get 3
                      i32.store offset=4
                      local.get 3
                      i32.const 2
                      i32.ne
                      br_if 7 (;@2;)
                      block  ;; label = @10
                        local.get 5
                        br_if 0 (;@10;)
                        i32.const 0
                        i32.load offset=1059280
                        i32.const 2147483647
                        i32.and
                        i32.eqz
                        br_if 0 (;@10;)
                        call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                        br_if 0 (;@10;)
                        local.get 4
                        i32.const 1
                        i32.store8 offset=1
                      end
                      local.get 4
                      i32.const 0
                      i32.store8
                    end
                    local.get 1
                    local.get 1
                    i32.load
                    local.tee 2
                    i32.const -1
                    i32.add
                    i32.store
                    block  ;; label = @9
                      local.get 2
                      i32.const 1
                      i32.ne
                      br_if 0 (;@9;)
                      local.get 1
                      call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                    end
                    local.get 0
                    i32.const 32
                    i32.add
                    global.set $__stack_pointer
                    return
                  end
                  i32.const 1048792
                  i32.const 16
                  local.get 0
                  i32.const 8
                  i32.add
                  i32.const 1048904
                  i32.const 1051348
                  call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
                  unreachable
                end
                unreachable
                unreachable
              end
              i32.const 1049200
              i32.const 94
              i32.const 1049324
              call $_ZN4core6option13expect_failed17h8d17c0af9e73185dE
              unreachable
            end
            local.get 0
            i32.const 28
            i32.add
            i32.const 0
            i32.store
            local.get 0
            i32.const 24
            i32.add
            i32.const 1048792
            i32.store
            local.get 0
            i64.const 1
            i64.store offset=12 align=4
            local.get 0
            i32.const 1052308
            i32.store offset=8
            local.get 0
            i32.const 4
            i32.add
            local.get 0
            i32.const 8
            i32.add
            call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
            unreachable
          end
          local.get 0
          local.get 5
          i32.store8 offset=12
          local.get 0
          local.get 4
          i32.store offset=8
          i32.const 1048920
          i32.const 43
          local.get 0
          i32.const 8
          i32.add
          i32.const 1048964
          i32.const 1052680
          call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
          unreachable
        end
        local.get 0
        i32.const 28
        i32.add
        i32.const 0
        i32.store
        local.get 0
        i32.const 1048792
        i32.store offset=24
        local.get 0
        i64.const 1
        i64.store offset=12 align=4
        local.get 0
        i32.const 1052720
        i32.store offset=8
        local.get 0
        i32.const 8
        i32.add
        i32.const 1052728
        call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
        unreachable
      end
      local.get 0
      i32.const 28
      i32.add
      i32.const 0
      i32.store
      local.get 0
      i32.const 24
      i32.add
      i32.const 1048792
      i32.store
      local.get 0
      i64.const 1
      i64.store offset=12 align=4
      local.get 0
      i32.const 1052776
      i32.store offset=8
      local.get 0
      i32.const 4
      i32.add
      local.get 0
      i32.const 8
      i32.add
      i32.const 1052784
      call $_ZN4core9panicking13assert_failed17he78e5fdda9a074e5E
      unreachable
    end
    local.get 0
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get 0
    i32.const 1048792
    i32.store offset=24
    local.get 0
    i64.const 1
    i64.store offset=12 align=4
    local.get 0
    i32.const 1052196
    i32.store offset=8
    local.get 0
    i32.const 8
    i32.add
    i32.const 1052260
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN3std3env11current_dir17he9cc85f9a6781258E (type 0) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    i32.const 512
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 512
            i32.const 1
            call $__rust_alloc
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            i32.const 512
            i32.store offset=4
            local.get 1
            local.get 3
            i32.store
            local.get 3
            i32.const 512
            call $getcwd
            br_if 1 (;@3;)
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  i32.const 0
                  i32.load offset=1059804
                  local.tee 2
                  i32.const 68
                  i32.ne
                  br_if 0 (;@7;)
                  i32.const 512
                  local.set 2
                  br 1 (;@6;)
                end
                local.get 0
                i64.const 1
                i64.store align=4
                local.get 0
                i32.const 8
                i32.add
                local.get 2
                i32.store
                i32.const 512
                local.set 2
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 1
                local.get 2
                i32.store offset=8
                local.get 1
                local.get 2
                i32.const 1
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                local.get 1
                i32.load
                local.tee 3
                local.get 1
                i32.load offset=4
                local.tee 2
                call $getcwd
                br_if 3 (;@3;)
                i32.const 0
                i32.load offset=1059804
                local.tee 4
                i32.const 68
                i32.eq
                br_if 0 (;@6;)
              end
              local.get 0
              i64.const 1
              i64.store align=4
              local.get 0
              i32.const 8
              i32.add
              local.get 4
              i32.store
              local.get 2
              i32.eqz
              br_if 3 (;@2;)
            end
            local.get 3
            local.get 2
            i32.const 1
            call $__rust_dealloc
            br 2 (;@2;)
          end
          i32.const 512
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        local.get 1
        local.get 3
        call $strlen
        local.tee 4
        i32.store offset=8
        block  ;; label = @3
          local.get 2
          local.get 4
          i32.le_u
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 4
              br_if 0 (;@5;)
              i32.const 1
              local.set 5
              local.get 3
              local.get 2
              i32.const 1
              call $__rust_dealloc
              br 1 (;@4;)
            end
            local.get 3
            local.get 2
            i32.const 1
            local.get 4
            call $__rust_realloc
            local.tee 5
            i32.eqz
            br_if 3 (;@1;)
          end
          local.get 1
          local.get 4
          i32.store offset=4
          local.get 1
          local.get 5
          i32.store
        end
        local.get 0
        local.get 1
        i64.load
        i64.store offset=4 align=4
        local.get 0
        i32.const 0
        i32.store
        local.get 0
        i32.const 12
        i32.add
        local.get 1
        i32.const 8
        i32.add
        i32.load
        i32.store
      end
      local.get 1
      i32.const 16
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std3env7_var_os17hac8a3e6bb094e6d3E (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 8
    i32.add
    local.get 1
    local.get 2
    call $_ZN72_$LT$$RF$str$u20$as$u20$alloc..ffi..c_str..CString..new..SpecNewImpl$GT$13spec_new_impl17h8200e939a8ca496bE
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.load offset=8
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.const 20
              i32.add
              i32.load
              local.tee 1
              i32.eqz
              br_if 0 (;@5;)
              local.get 3
              i32.const 16
              i32.add
              i32.load
              local.get 1
              i32.const 1
              call $__rust_dealloc
            end
            local.get 0
            i32.const 0
            i32.store
            br 1 (;@3;)
          end
          local.get 3
          i32.const 16
          i32.add
          i32.load
          local.set 4
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.load offset=12
              local.tee 2
              call $getenv
              local.tee 5
              i32.eqz
              br_if 0 (;@5;)
              block  ;; label = @6
                block  ;; label = @7
                  local.get 5
                  call $strlen
                  local.tee 1
                  br_if 0 (;@7;)
                  i32.const 1
                  local.set 6
                  br 1 (;@6;)
                end
                local.get 1
                i32.const 0
                i32.lt_s
                br_if 4 (;@2;)
                local.get 1
                i32.const 1
                call $__rust_alloc
                local.tee 6
                i32.eqz
                br_if 5 (;@1;)
              end
              local.get 6
              local.get 5
              local.get 1
              call $memcpy
              local.set 5
              local.get 0
              i32.const 8
              i32.add
              local.get 1
              i32.store
              local.get 0
              local.get 1
              i32.store offset=4
              local.get 0
              local.get 5
              i32.store
              br 1 (;@4;)
            end
            local.get 0
            i32.const 0
            i32.store
          end
          local.get 2
          i32.const 0
          i32.store8
          local.get 4
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          local.get 4
          i32.const 1
          call $__rust_dealloc
        end
        local.get 3
        i32.const 32
        i32.add
        global.set $__stack_pointer
        return
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get 1
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std3env4vars17h6ec7390381e46b70E (type 0) (param i32)
    local.get 0
    call $_ZN3std3env7vars_os17h4335c88f098b90b5E)
  (func $_ZN3std3env7vars_os17h4335c88f098b90b5E (type 0) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    i32.const 0
    local.set 2
    i32.const 0
    i32.load offset=1059812
    local.set 3
    local.get 1
    i32.const 0
    i32.store offset=24
    local.get 1
    i64.const 4
    i64.store offset=16
    i32.const 4
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 3
                  br_if 0 (;@7;)
                  i32.const 0
                  local.set 5
                  br 1 (;@6;)
                end
                i32.const 0
                local.set 5
                local.get 3
                i32.load
                local.tee 6
                i32.eqz
                br_if 0 (;@6;)
                i32.const 0
                local.set 2
                i32.const 4
                local.set 7
                loop  ;; label = @7
                  local.get 3
                  local.set 8
                  block  ;; label = @8
                    local.get 6
                    call $strlen
                    local.tee 9
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 6
                    i32.const 1
                    i32.add
                    local.set 5
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 9
                        i32.const -1
                        i32.add
                        local.tee 4
                        i32.const 8
                        i32.lt_u
                        br_if 0 (;@10;)
                        local.get 1
                        i32.const 8
                        i32.add
                        i32.const 61
                        local.get 5
                        local.get 4
                        call $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E
                        local.get 1
                        i32.load offset=12
                        local.set 3
                        local.get 1
                        i32.load offset=8
                        local.set 5
                        br 1 (;@9;)
                      end
                      i32.const 0
                      local.set 3
                      block  ;; label = @10
                        local.get 4
                        br_if 0 (;@10;)
                        i32.const 0
                        local.set 5
                        br 1 (;@9;)
                      end
                      loop  ;; label = @10
                        block  ;; label = @11
                          local.get 5
                          local.get 3
                          i32.add
                          i32.load8_u
                          i32.const 61
                          i32.ne
                          br_if 0 (;@11;)
                          i32.const 1
                          local.set 5
                          br 2 (;@9;)
                        end
                        local.get 4
                        local.get 3
                        i32.const 1
                        i32.add
                        local.tee 3
                        i32.ne
                        br_if 0 (;@10;)
                      end
                      i32.const 0
                      local.set 5
                      local.get 4
                      local.set 3
                    end
                    local.get 5
                    i32.eqz
                    br_if 0 (;@8;)
                    local.get 3
                    i32.const 1
                    i32.add
                    local.tee 3
                    local.get 9
                    i32.gt_u
                    br_if 3 (;@5;)
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 3
                        br_if 0 (;@10;)
                        i32.const 1
                        local.set 4
                        br 1 (;@9;)
                      end
                      local.get 3
                      i32.const 0
                      i32.lt_s
                      br_if 7 (;@2;)
                      local.get 3
                      i32.const 1
                      call $__rust_alloc
                      local.tee 4
                      i32.eqz
                      br_if 5 (;@4;)
                    end
                    local.get 4
                    local.get 6
                    local.get 3
                    call $memcpy
                    local.set 10
                    local.get 3
                    i32.const 1
                    i32.add
                    local.set 5
                    local.get 3
                    local.get 9
                    i32.ge_u
                    br_if 5 (;@3;)
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 9
                        local.get 5
                        i32.sub
                        local.tee 4
                        br_if 0 (;@10;)
                        i32.const 1
                        local.set 9
                        br 1 (;@9;)
                      end
                      local.get 4
                      i32.const 0
                      i32.lt_s
                      br_if 7 (;@2;)
                      local.get 4
                      i32.const 1
                      call $__rust_alloc
                      local.tee 9
                      i32.eqz
                      br_if 8 (;@1;)
                    end
                    local.get 9
                    local.get 6
                    local.get 5
                    i32.add
                    local.get 4
                    call $memcpy
                    local.set 6
                    local.get 10
                    i32.eqz
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 2
                      local.get 1
                      i32.load offset=20
                      i32.ne
                      br_if 0 (;@9;)
                      local.get 1
                      i32.const 16
                      i32.add
                      local.get 2
                      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h4285ebb9278a5d57E
                      local.get 1
                      i32.load offset=16
                      local.set 7
                      local.get 1
                      i32.load offset=24
                      local.set 2
                    end
                    local.get 7
                    local.get 2
                    i32.const 24
                    i32.mul
                    i32.add
                    local.tee 5
                    local.get 6
                    i32.store offset=12
                    local.get 5
                    local.get 3
                    i32.store offset=8
                    local.get 5
                    local.get 3
                    i32.store offset=4
                    local.get 5
                    local.get 10
                    i32.store
                    local.get 5
                    i32.const 20
                    i32.add
                    local.get 4
                    i32.store
                    local.get 5
                    i32.const 16
                    i32.add
                    local.get 4
                    i32.store
                    local.get 1
                    local.get 2
                    i32.const 1
                    i32.add
                    local.tee 2
                    i32.store offset=24
                  end
                  local.get 8
                  i32.const 4
                  i32.add
                  local.set 3
                  local.get 8
                  i32.load offset=4
                  local.tee 6
                  br_if 0 (;@7;)
                end
                local.get 1
                i32.load offset=20
                local.set 5
                local.get 1
                i32.load offset=16
                local.set 4
              end
              local.get 0
              local.get 4
              i32.store offset=8
              local.get 0
              local.get 5
              i32.store offset=4
              local.get 0
              local.get 4
              i32.store
              local.get 0
              local.get 4
              local.get 2
              i32.const 24
              i32.mul
              i32.add
              i32.store offset=12
              local.get 1
              i32.const 32
              i32.add
              global.set $__stack_pointer
              return
            end
            local.get 3
            local.get 9
            i32.const 1052516
            call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
            unreachable
          end
          local.get 3
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        local.get 5
        local.get 9
        i32.const 1052532
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get 4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 96
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.load offset=8
              local.tee 3
              local.get 1
              i32.load offset=12
              i32.eq
              br_if 0 (;@5;)
              local.get 1
              local.get 3
              i32.const 24
              i32.add
              i32.store offset=8
              local.get 3
              i32.load
              local.tee 1
              br_if 1 (;@4;)
            end
            local.get 0
            i32.const 0
            i32.store
            br 1 (;@3;)
          end
          local.get 3
          i32.const 20
          i32.add
          i32.load
          local.set 4
          local.get 3
          i32.const 16
          i32.add
          i32.load
          local.set 5
          local.get 3
          i32.load offset=12
          local.set 6
          local.get 3
          i32.load offset=4
          local.set 7
          local.get 2
          i32.const 8
          i32.add
          local.get 1
          local.get 3
          i32.load offset=8
          local.tee 3
          call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
          local.get 2
          i32.load offset=8
          br_if 1 (;@2;)
          local.get 2
          i32.const 64
          i32.add
          i32.const 8
          i32.add
          local.tee 8
          local.get 7
          i32.store
          local.get 2
          i32.const 32
          i32.add
          i32.const 8
          i32.add
          local.tee 7
          local.get 3
          i32.store
          local.get 2
          local.get 1
          i32.store offset=68
          local.get 2
          local.get 2
          i64.load offset=68 align=4
          i64.store offset=32
          local.get 2
          i32.const 80
          i32.add
          local.get 6
          local.get 4
          call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
          local.get 2
          i32.load offset=80
          br_if 2 (;@1;)
          local.get 8
          local.get 5
          i32.store
          local.get 2
          i32.const 28
          i32.add
          local.get 4
          i32.store
          local.get 2
          i32.const 8
          i32.add
          i32.const 8
          i32.add
          local.tee 3
          local.get 7
          i32.load
          i32.store
          local.get 2
          local.get 6
          i32.store offset=68
          local.get 2
          local.get 2
          i64.load offset=32
          i64.store offset=8
          local.get 2
          local.get 2
          i64.load offset=68 align=4
          i64.store offset=20 align=4
          local.get 0
          i32.const 16
          i32.add
          local.get 2
          i32.const 8
          i32.add
          i32.const 16
          i32.add
          i64.load
          i64.store align=4
          local.get 0
          i32.const 8
          i32.add
          local.get 3
          i64.load
          i64.store align=4
          local.get 0
          local.get 2
          i64.load offset=8
          i64.store align=4
        end
        local.get 2
        i32.const 96
        i32.add
        global.set $__stack_pointer
        return
      end
      local.get 2
      local.get 2
      i64.load offset=12 align=4
      i64.store offset=20 align=4
      local.get 2
      local.get 3
      i32.store offset=16
      local.get 2
      local.get 7
      i32.store offset=12
      local.get 2
      local.get 1
      i32.store offset=8
      local.get 2
      i32.const 64
      i32.add
      i32.const 4
      i32.or
      local.get 2
      i32.const 8
      i32.add
      call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
      local.get 2
      i32.const 80
      i32.add
      i32.const 8
      i32.add
      local.get 2
      i32.const 76
      i32.add
      i32.load
      local.tee 3
      i32.store
      local.get 2
      local.get 2
      i64.load offset=68 align=4
      local.tee 9
      i64.store offset=80
      local.get 2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.get 3
      i32.store
      local.get 2
      local.get 9
      i64.store offset=8
      i32.const 1048920
      i32.const 43
      local.get 2
      i32.const 8
      i32.add
      i32.const 1049536
      i32.const 1049476
      call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
      unreachable
    end
    local.get 2
    local.get 2
    i64.load offset=84 align=4
    i64.store offset=20 align=4
    local.get 2
    local.get 4
    i32.store offset=16
    local.get 2
    local.get 5
    i32.store offset=12
    local.get 2
    local.get 6
    i32.store offset=8
    local.get 2
    i32.const 64
    i32.add
    i32.const 4
    i32.or
    local.get 2
    i32.const 8
    i32.add
    call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
    local.get 2
    i32.const 80
    i32.add
    i32.const 8
    i32.add
    local.get 2
    i32.const 76
    i32.add
    i32.load
    local.tee 3
    i32.store
    local.get 2
    local.get 2
    i64.load offset=68 align=4
    local.tee 9
    i64.store offset=80
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 3
    i32.store
    local.get 2
    local.get 9
    i64.store offset=8
    i32.const 1048920
    i32.const 43
    local.get 2
    i32.const 8
    i32.add
    i32.const 1049536
    i32.const 1049492
    call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
    unreachable)
  (func $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h30941f502999929dE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load8_u
              br_table 0 (;@5;) 1 (;@4;) 2 (;@3;) 3 (;@2;) 0 (;@5;)
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
            call $_ZN3std3sys4wasi2os12error_string17h0414d6055eae23aaE
            local.get 2
            i32.const 60
            i32.add
            i32.const 2
            i32.store
            local.get 2
            i32.const 36
            i32.add
            i32.const 3
            i32.store
            local.get 2
            i64.const 3
            i64.store offset=44 align=4
            local.get 2
            i32.const 1050484
            i32.store offset=40
            local.get 2
            i32.const 4
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
            call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
            local.set 0
            local.get 2
            i32.load offset=12
            local.tee 1
            i32.eqz
            br_if 3 (;@1;)
            local.get 2
            i32.load offset=8
            local.get 1
            i32.const 1
            call $__rust_dealloc
            br 3 (;@1;)
          end
          local.get 0
          i32.load8_u offset=1
          local.set 0
          local.get 2
          i32.const 60
          i32.add
          i32.const 1
          i32.store
          local.get 2
          i64.const 1
          i64.store offset=44 align=4
          local.get 2
          i32.const 1049524
          i32.store offset=40
          local.get 2
          i32.const 5
          i32.store offset=12
          local.get 2
          local.get 0
          i32.const 32
          i32.xor
          i32.const 63
          i32.and
          i32.const 2
          i32.shl
          local.tee 0
          i32.const 1052868
          i32.add
          i32.load
          i32.store offset=28
          local.get 2
          local.get 0
          i32.const 1053124
          i32.add
          i32.load
          i32.store offset=24
          local.get 2
          local.get 2
          i32.const 8
          i32.add
          i32.store offset=56
          local.get 2
          local.get 2
          i32.const 24
          i32.add
          i32.store offset=8
          local.get 1
          local.get 2
          i32.const 40
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
          local.set 0
          br 2 (;@1;)
        end
        local.get 0
        i32.const 4
        i32.add
        i32.load
        local.tee 0
        i32.load
        local.get 0
        i32.load offset=4
        local.get 1
        call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE
        local.set 0
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
      i32.load offset=16
      call_indirect (type 3)
      local.set 0
    end
    local.get 2
    i32.const 64
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN3std3env4args17h02d9a592c6ab5652E (type 0) (param i32)
    local.get 0
    call $_ZN3std3env7args_os17hd21bfff4539d3ffdE)
  (func $_ZN3std3env7args_os17hd21bfff4539d3ffdE (type 0) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    local.get 1
    i32.const 16
    i32.add
    call $_ZN4wasi13lib_generated14args_sizes_get17h4999a6465b98090cE
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 1
                      i32.load16_u offset=16
                      br_if 0 (;@9;)
                      local.get 1
                      i32.const 24
                      i32.add
                      i32.load
                      local.set 2
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 1
                          i32.load offset=20
                          local.tee 3
                          br_if 0 (;@11;)
                          i32.const 4
                          local.set 4
                          br 1 (;@10;)
                        end
                        local.get 3
                        i32.const 1073741823
                        i32.and
                        local.tee 5
                        local.get 3
                        i32.ne
                        br_if 4 (;@6;)
                        local.get 3
                        i32.const 2
                        i32.shl
                        local.tee 6
                        i32.const -1
                        i32.le_s
                        br_if 4 (;@6;)
                        local.get 5
                        local.get 3
                        i32.eq
                        i32.const 2
                        i32.shl
                        local.set 5
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 6
                            br_if 0 (;@12;)
                            local.get 5
                            local.set 4
                            br 1 (;@11;)
                          end
                          local.get 6
                          local.get 5
                          call $__rust_alloc
                          local.set 4
                        end
                        local.get 4
                        i32.eqz
                        br_if 2 (;@8;)
                      end
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 2
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 2
                          i32.const 0
                          i32.lt_s
                          br_if 5 (;@6;)
                          local.get 2
                          i32.const 1
                          call $__rust_alloc
                          local.tee 7
                          i32.eqz
                          br_if 7 (;@4;)
                          local.get 1
                          i32.const 8
                          i32.add
                          local.get 4
                          local.get 7
                          call $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E
                          local.get 1
                          i32.load16_u offset=8
                          i32.eqz
                          br_if 1 (;@10;)
                          local.get 7
                          local.get 2
                          i32.const 1
                          call $__rust_dealloc
                          br 8 (;@3;)
                        end
                        i32.const 1
                        local.set 7
                        local.get 1
                        local.get 4
                        i32.const 1
                        call $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E
                        local.get 1
                        i32.load16_u
                        br_if 7 (;@3;)
                      end
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 3
                          br_if 0 (;@11;)
                          local.get 1
                          i64.const 4
                          i64.store offset=16
                          i32.const 0
                          local.set 6
                          br 1 (;@10;)
                        end
                        local.get 3
                        i64.extend_i32_u
                        i64.const 12
                        i64.mul
                        local.tee 8
                        i64.const 32
                        i64.shr_u
                        i32.wrap_i64
                        local.tee 5
                        br_if 4 (;@6;)
                        local.get 8
                        i32.wrap_i64
                        local.tee 6
                        i32.const -1
                        i32.le_s
                        br_if 4 (;@6;)
                        local.get 5
                        i32.eqz
                        i32.const 2
                        i32.shl
                        local.set 5
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 6
                            br_if 0 (;@12;)
                            local.get 5
                            local.set 9
                            br 1 (;@11;)
                          end
                          local.get 6
                          local.get 5
                          call $__rust_alloc
                          local.set 9
                        end
                        local.get 9
                        i32.eqz
                        br_if 3 (;@7;)
                        local.get 1
                        i32.const 0
                        i32.store offset=24
                        local.get 1
                        local.get 9
                        i32.store offset=16
                        local.get 1
                        local.get 3
                        i32.store offset=20
                        local.get 3
                        i32.const 2
                        i32.shl
                        local.set 10
                        i32.const 0
                        local.set 6
                        local.get 4
                        local.set 11
                        loop  ;; label = @11
                          i32.const 1
                          local.set 12
                          block  ;; label = @12
                            local.get 11
                            i32.load
                            local.tee 13
                            call $strlen
                            local.tee 5
                            i32.eqz
                            br_if 0 (;@12;)
                            local.get 5
                            i32.const 0
                            i32.lt_s
                            br_if 6 (;@6;)
                            local.get 5
                            i32.const 1
                            call $__rust_alloc
                            local.tee 12
                            i32.eqz
                            br_if 7 (;@5;)
                          end
                          local.get 12
                          local.get 13
                          local.get 5
                          call $memcpy
                          local.set 13
                          block  ;; label = @12
                            local.get 6
                            local.get 1
                            i32.load offset=20
                            i32.ne
                            br_if 0 (;@12;)
                            local.get 1
                            i32.const 16
                            i32.add
                            local.get 6
                            call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h49fd6398be4a4a9aE
                            local.get 1
                            i32.load offset=16
                            local.set 9
                            local.get 1
                            i32.load offset=24
                            local.set 6
                          end
                          local.get 11
                          i32.const 4
                          i32.add
                          local.set 11
                          local.get 9
                          local.get 6
                          i32.const 12
                          i32.mul
                          i32.add
                          local.tee 12
                          local.get 5
                          i32.store offset=8
                          local.get 12
                          local.get 5
                          i32.store offset=4
                          local.get 12
                          local.get 13
                          i32.store
                          local.get 1
                          local.get 6
                          i32.const 1
                          i32.add
                          local.tee 6
                          i32.store offset=24
                          local.get 10
                          i32.const -4
                          i32.add
                          local.tee 10
                          br_if 0 (;@11;)
                        end
                        local.get 3
                        i32.const 2
                        i32.shl
                        local.tee 5
                        i32.eqz
                        br_if 0 (;@10;)
                        local.get 4
                        local.get 5
                        i32.const 4
                        call $__rust_dealloc
                      end
                      local.get 1
                      i32.load offset=20
                      local.set 11
                      local.get 1
                      i32.load offset=16
                      local.set 5
                      block  ;; label = @10
                        local.get 2
                        i32.eqz
                        br_if 0 (;@10;)
                        local.get 7
                        local.get 2
                        i32.const 1
                        call $__rust_dealloc
                      end
                      local.get 5
                      br_if 8 (;@1;)
                    end
                    i32.const 4
                    local.set 5
                    i32.const 0
                    local.set 11
                    br 6 (;@2;)
                  end
                  local.get 6
                  local.get 5
                  call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
                  unreachable
                end
                local.get 6
                local.get 5
                call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
                unreachable
              end
              call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
              unreachable
            end
            local.get 5
            i32.const 1
            call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
            unreachable
          end
          local.get 2
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        i32.const 0
        local.set 11
        block  ;; label = @3
          local.get 3
          br_if 0 (;@3;)
          i32.const 4
          local.set 5
          br 1 (;@2;)
        end
        i32.const 4
        local.set 5
        local.get 4
        local.get 3
        i32.const 2
        i32.shl
        i32.const 4
        call $__rust_dealloc
      end
      i32.const 0
      local.set 6
    end
    local.get 0
    local.get 5
    i32.store offset=8
    local.get 0
    local.get 11
    i32.store offset=4
    local.get 0
    local.get 5
    i32.store
    local.get 0
    local.get 5
    local.get 6
    i32.const 12
    i32.mul
    i32.add
    i32.store offset=12
    local.get 1
    i32.const 32
    i32.add
    global.set $__stack_pointer)
  (func $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E (type 2) (param i32 i32)
    (local i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
        i32.const 40
        i32.add
        local.get 1
        local.get 3
        i32.load offset=8
        local.tee 3
        call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
        local.get 2
        i32.load offset=40
        br_if 1 (;@1;)
        local.get 2
        i32.const 24
        i32.add
        i32.const 8
        i32.add
        local.get 4
        i32.store
        local.get 2
        i32.const 8
        i32.add
        i32.const 8
        i32.add
        local.get 3
        i32.store
        local.get 2
        local.get 1
        i32.store offset=28
        local.get 2
        local.get 2
        i64.load offset=28 align=4
        local.tee 5
        i64.store offset=8
        local.get 0
        i32.const 8
        i32.add
        local.get 3
        i32.store
        local.get 0
        local.get 5
        i64.store align=4
      end
      local.get 2
      i32.const 64
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 2
    local.get 2
    i64.load offset=44 align=4
    i64.store offset=52 align=4
    local.get 2
    local.get 3
    i32.store offset=48
    local.get 2
    local.get 4
    i32.store offset=44
    local.get 2
    local.get 1
    i32.store offset=40
    local.get 2
    i32.const 24
    i32.add
    i32.const 4
    i32.or
    local.get 2
    i32.const 40
    i32.add
    call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
    local.get 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get 2
    i32.const 36
    i32.add
    i32.load
    local.tee 1
    i32.store
    local.get 2
    local.get 2
    i64.load offset=28 align=4
    local.tee 5
    i64.store offset=8
    local.get 2
    i32.const 40
    i32.add
    i32.const 8
    i32.add
    local.get 1
    i32.store
    local.get 2
    local.get 5
    i64.store offset=40
    i32.const 1048920
    i32.const 43
    local.get 2
    i32.const 40
    i32.add
    i32.const 1049536
    i32.const 1049508
    call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
    unreachable)
  (func $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E (type 10) (param i32) (result i32)
    (local i32)
    i32.const 40
    local.set 1
    block  ;; label = @1
      local.get 0
      i32.const 65535
      i32.gt_u
      br_if 0 (;@1;)
      i32.const 2
      local.set 1
      i32.const 1052594
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 3
      local.set 1
      i32.const 1052596
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 1
      local.set 1
      i32.const 1052598
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 1052600
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 11
      local.set 1
      i32.const 1052602
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 7
      local.set 1
      i32.const 1052604
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 6
      local.set 1
      i32.const 1052606
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 9
      local.set 1
      i32.const 1052608
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 8
      local.set 1
      i32.const 1052610
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 0
      local.set 1
      i32.const 1052612
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 35
      local.set 1
      i32.const 1052614
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 20
      local.set 1
      i32.const 1052616
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 22
      local.set 1
      i32.const 1052618
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 12
      local.set 1
      i32.const 1052620
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 13
      local.set 1
      i32.const 1052622
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 36
      local.set 1
      i32.const 1052624
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      br_if 0 (;@1;)
      i32.const 38
      i32.const 40
      i32.const 1052626
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get 0
      i32.eq
      select
      local.set 1
    end
    local.get 1)
  (func $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.const 8
          i32.add
          i32.load
          local.tee 3
          br_if 0 (;@3;)
          local.get 0
          i32.const 4
          i32.store8
          br 1 (;@2;)
        end
        local.get 1
        i32.load
        local.set 4
        i32.const 0
        local.set 5
        loop  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                local.get 5
                i32.lt_u
                br_if 0 (;@6;)
                local.get 2
                local.get 3
                local.get 5
                i32.sub
                local.tee 6
                i32.store offset=12
                local.get 2
                local.get 4
                local.get 5
                i32.add
                local.tee 7
                i32.store offset=8
                local.get 2
                i32.const 16
                i32.add
                i32.const 1
                local.get 2
                i32.const 8
                i32.add
                i32.const 1
                call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 2
                        i32.load16_u offset=16
                        br_if 0 (;@10;)
                        local.get 2
                        i32.load offset=20
                        local.set 8
                        br 1 (;@9;)
                      end
                      local.get 2
                      local.get 2
                      i32.load16_u offset=18
                      i32.store16 offset=30
                      local.get 6
                      local.set 8
                      local.get 2
                      i32.const 30
                      i32.add
                      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                      i32.const 65535
                      i32.and
                      local.tee 9
                      i32.const 1052592
                      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                      i32.const 65535
                      i32.and
                      i32.ne
                      br_if 1 (;@8;)
                    end
                    local.get 1
                    i32.const 0
                    i32.store8 offset=12
                    local.get 8
                    i32.eqz
                    br_if 1 (;@7;)
                    local.get 8
                    local.get 5
                    i32.add
                    local.set 5
                    br 4 (;@4;)
                  end
                  local.get 1
                  i32.const 0
                  i32.store8 offset=12
                  local.get 9
                  call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                  i32.const 255
                  i32.and
                  i32.const 35
                  i32.eq
                  br_if 3 (;@4;)
                  local.get 0
                  i32.const 0
                  i32.store
                  local.get 0
                  i32.const 4
                  i32.add
                  local.get 9
                  i32.store
                  br 2 (;@5;)
                end
                local.get 0
                i32.const 1049588
                i64.extend_i32_u
                i64.const 32
                i64.shl
                i64.const 2
                i64.or
                i64.store align=4
                br 1 (;@5;)
              end
              local.get 5
              local.get 3
              i32.const 1049640
              call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
              unreachable
            end
            local.get 5
            i32.eqz
            br_if 2 (;@2;)
            local.get 1
            i32.const 8
            i32.add
            local.tee 5
            i32.const 0
            i32.store
            local.get 6
            i32.eqz
            br_if 2 (;@2;)
            local.get 4
            local.get 7
            local.get 6
            call $memmove
            drop
            local.get 5
            local.get 6
            i32.store
            br 2 (;@2;)
          end
          local.get 3
          local.get 5
          i32.gt_u
          br_if 0 (;@3;)
        end
        local.get 0
        i32.const 4
        i32.store8
        local.get 5
        i32.eqz
        br_if 0 (;@2;)
        local.get 3
        local.get 5
        i32.lt_u
        br_if 1 (;@1;)
        local.get 1
        i32.const 8
        i32.add
        local.tee 8
        i32.const 0
        i32.store
        local.get 3
        local.get 5
        i32.sub
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load
        local.tee 6
        local.get 6
        local.get 5
        i32.add
        local.get 3
        call $memmove
        drop
        local.get 8
        local.get 3
        i32.store
      end
      local.get 2
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 5
    local.get 3
    i32.const 1049144
    call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
    unreachable)
  (func $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i64)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 4
        i32.add
        local.tee 5
        i32.load
        local.get 1
        i32.const 8
        i32.add
        i32.load
        i32.sub
        local.get 3
        i32.ge_u
        br_if 0 (;@2;)
        local.get 4
        i32.const 8
        i32.add
        local.get 1
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
        local.get 4
        i32.load8_u offset=8
        i32.const 4
        i32.eq
        br_if 0 (;@2;)
        local.get 4
        i64.load offset=8
        local.tee 6
        i32.wrap_i64
        i32.const 255
        i32.and
        i32.const 4
        i32.eq
        br_if 0 (;@2;)
        local.get 0
        local.get 6
        i64.store align=4
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 5
        i32.load
        local.get 3
        i32.le_u
        br_if 0 (;@2;)
        local.get 1
        i32.load
        local.get 1
        i32.const 8
        i32.add
        local.tee 1
        i32.load
        local.tee 5
        i32.add
        local.get 2
        local.get 3
        call $memcpy
        drop
        local.get 0
        i32.const 4
        i32.store8
        local.get 1
        local.get 5
        local.get 3
        i32.add
        i32.store
        br 1 (;@1;)
      end
      local.get 1
      i32.const 1
      i32.store8 offset=12
      local.get 4
      i32.const 8
      i32.add
      local.get 1
      local.get 2
      local.get 3
      call $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E
      local.get 1
      i32.const 0
      i32.store8 offset=12
      local.get 0
      local.get 4
      i64.load offset=8
      i64.store align=4
    end
    local.get 4
    i32.const 16
    i32.add
    global.set $__stack_pointer)
  (func $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E (type 8) (param i32 i32 i32 i32)
    (local i32 i64 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    i64.const 4
    local.set 5
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
                i32.const 1
                local.get 4
                i32.const 8
                i32.add
                i32.const 1
                call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 4
                      i32.load16_u offset=16
                      local.tee 6
                      br_if 0 (;@9;)
                      local.get 4
                      i32.load offset=20
                      local.tee 7
                      br_if 1 (;@8;)
                      i32.const 1050536
                      local.set 7
                      i64.const 2
                      local.set 5
                      br 6 (;@3;)
                    end
                    local.get 4
                    local.get 4
                    i32.load16_u offset=18
                    i32.store16 offset=30
                    local.get 4
                    i32.const 30
                    i32.add
                    call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                    i32.const 65535
                    i32.and
                    local.tee 7
                    call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                    i32.const 255
                    i32.and
                    i32.const 35
                    i32.eq
                    br_if 1 (;@7;)
                    i64.const 0
                    local.set 5
                    br 5 (;@3;)
                  end
                  local.get 3
                  local.get 7
                  i32.lt_u
                  br_if 3 (;@4;)
                  local.get 2
                  local.get 7
                  i32.add
                  local.set 2
                  local.get 3
                  local.get 7
                  i32.sub
                  local.set 3
                end
                local.get 3
                br_if 0 (;@6;)
              end
            end
            br 2 (;@2;)
          end
          local.get 7
          local.get 3
          i32.const 1050692
          call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
          unreachable
        end
        i32.const 1052592
        call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
        local.set 3
        local.get 6
        i32.eqz
        br_if 0 (;@2;)
        local.get 7
        local.get 3
        i32.const 65535
        i32.and
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 4
        i32.store8
        br 1 (;@1;)
      end
      local.get 0
      local.get 7
      i64.extend_i32_u
      i64.const 32
      i64.shl
      local.get 5
      i64.or
      i64.store align=4
    end
    local.get 4
    i32.const 32
    i32.add
    global.set $__stack_pointer)
  (func $_ZN3std3sys4wasi2os12error_string17h0414d6055eae23aaE (type 2) (param i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 1056
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            local.get 2
            i32.const 0
            i32.const 1024
            call $memset
            local.tee 2
            i32.const 1024
            call $strerror_r
            i32.const 0
            i32.lt_s
            br_if 0 (;@4;)
            local.get 2
            i32.const 1024
            i32.add
            local.get 2
            local.get 2
            call $strlen
            call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
            local.get 2
            i32.load offset=1024
            br_if 1 (;@3;)
            local.get 2
            i32.load offset=1028
            local.set 3
            block  ;; label = @5
              block  ;; label = @6
                local.get 2
                i32.const 1032
                i32.add
                i32.load
                local.tee 1
                br_if 0 (;@6;)
                i32.const 1
                local.set 4
                br 1 (;@5;)
              end
              local.get 1
              i32.const 0
              i32.lt_s
              br_if 3 (;@2;)
              local.get 1
              i32.const 1
              call $__rust_alloc
              local.tee 4
              i32.eqz
              br_if 4 (;@1;)
            end
            local.get 4
            local.get 3
            local.get 1
            call $memcpy
            local.set 3
            local.get 0
            local.get 1
            i32.store offset=8
            local.get 0
            local.get 1
            i32.store offset=4
            local.get 0
            local.get 3
            i32.store
            local.get 2
            i32.const 1056
            i32.add
            global.set $__stack_pointer
            return
          end
          local.get 2
          i32.const 1044
          i32.add
          i32.const 0
          i32.store
          local.get 2
          i32.const 1048792
          i32.store offset=1040
          local.get 2
          i64.const 1
          i64.store offset=1028 align=4
          local.get 2
          i32.const 1052444
          i32.store offset=1024
          local.get 2
          i32.const 1024
          i32.add
          i32.const 1052484
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 2
        local.get 2
        i64.load offset=1028 align=4
        i64.store offset=1048
        i32.const 1048920
        i32.const 43
        local.get 2
        i32.const 1048
        i32.add
        i32.const 1048980
        i32.const 1052500
        call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get 1
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5write17hfeddc07cce766a8dE (type 8) (param i32 i32 i32 i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 1
      i32.const 4
      i32.add
      i32.load
      local.get 1
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 5
      i32.sub
      local.get 3
      i32.ge_u
      br_if 0 (;@1;)
      local.get 1
      local.get 5
      local.get 3
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 5
    end
    local.get 1
    i32.load
    local.get 5
    i32.add
    local.get 2
    local.get 3
    call $memcpy
    drop
    local.get 0
    local.get 3
    i32.store offset=4
    local.get 4
    local.get 5
    local.get 3
    i32.add
    i32.store
    local.get 0
    i32.const 0
    i32.store)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$14write_vectored17h5e2f18a5d90c2318E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const 3
            i32.shl
            local.tee 4
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            local.tee 5
            i32.const 1
            i32.add
            local.tee 6
            i32.const 7
            i32.and
            local.set 7
            local.get 5
            i32.const 7
            i32.ge_u
            br_if 1 (;@3;)
            i32.const 0
            local.set 6
            local.get 2
            local.set 5
            br 2 (;@2;)
          end
          local.get 1
          i32.const 4
          i32.add
          local.set 8
          local.get 1
          i32.const 8
          i32.add
          local.set 5
          i32.const 0
          local.set 6
          br 2 (;@1;)
        end
        local.get 2
        i32.const 60
        i32.add
        local.set 5
        local.get 6
        i32.const 1073741816
        i32.and
        local.set 9
        i32.const 0
        local.set 6
        loop  ;; label = @3
          local.get 5
          i32.load
          local.get 5
          i32.const -8
          i32.add
          i32.load
          local.get 5
          i32.const -16
          i32.add
          i32.load
          local.get 5
          i32.const -24
          i32.add
          i32.load
          local.get 5
          i32.const -32
          i32.add
          i32.load
          local.get 5
          i32.const -40
          i32.add
          i32.load
          local.get 5
          i32.const -48
          i32.add
          i32.load
          local.get 5
          i32.const -56
          i32.add
          i32.load
          local.get 6
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          local.set 6
          local.get 5
          i32.const 64
          i32.add
          local.set 5
          local.get 9
          i32.const -8
          i32.add
          local.tee 9
          br_if 0 (;@3;)
        end
        local.get 5
        i32.const -60
        i32.add
        local.set 5
      end
      block  ;; label = @2
        local.get 7
        i32.eqz
        br_if 0 (;@2;)
        local.get 5
        i32.const 4
        i32.add
        local.set 5
        loop  ;; label = @3
          local.get 5
          i32.load
          local.get 6
          i32.add
          local.set 6
          local.get 5
          i32.const 8
          i32.add
          local.set 5
          local.get 7
          i32.const -1
          i32.add
          local.tee 7
          br_if 0 (;@3;)
        end
      end
      local.get 1
      i32.const 8
      i32.add
      local.set 5
      local.get 1
      i32.const 4
      i32.add
      local.tee 8
      i32.load
      local.get 1
      i32.load offset=8
      local.tee 7
      i32.sub
      local.get 6
      i32.ge_u
      br_if 0 (;@1;)
      local.get 1
      local.get 7
      local.get 6
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
    end
    block  ;; label = @1
      local.get 3
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      local.get 4
      i32.add
      local.set 3
      local.get 5
      i32.load
      local.set 5
      loop  ;; label = @2
        local.get 2
        i32.load
        local.set 9
        block  ;; label = @3
          local.get 8
          i32.load
          local.get 5
          i32.sub
          local.get 2
          i32.const 4
          i32.add
          i32.load
          local.tee 7
          i32.ge_u
          br_if 0 (;@3;)
          local.get 1
          local.get 5
          local.get 7
          call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
          local.get 1
          i32.load offset=8
          local.set 5
        end
        local.get 1
        i32.load
        local.get 5
        i32.add
        local.get 9
        local.get 7
        call $memcpy
        drop
        local.get 1
        local.get 5
        local.get 7
        i32.add
        local.tee 5
        i32.store offset=8
        local.get 3
        local.get 2
        i32.const 8
        i32.add
        local.tee 2
        i32.ne
        br_if 0 (;@2;)
      end
    end
    local.get 0
    i32.const 0
    i32.store
    local.get 0
    local.get 6
    i32.store offset=4)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$17is_write_vectored17hbcc64e0de2a7bd6eE (type 10) (param i32) (result i32)
    i32.const 1)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$9write_all17hacb9a4b06548536cE (type 8) (param i32 i32 i32 i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 1
      i32.const 4
      i32.add
      i32.load
      local.get 1
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 5
      i32.sub
      local.get 3
      i32.ge_u
      br_if 0 (;@1;)
      local.get 1
      local.get 5
      local.get 3
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 5
    end
    local.get 1
    i32.load
    local.get 5
    i32.add
    local.get 2
    local.get 3
    call $memcpy
    drop
    local.get 0
    i32.const 4
    i32.store8
    local.get 4
    local.get 5
    local.get 3
    i32.add
    i32.store)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5flush17h3e171420e6883b71E (type 2) (param i32 i32)
    local.get 0
    i32.const 4
    i32.store8)
  (func $_ZN3std2io5Write18write_all_vectored17h3007a4983f10eddeE (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              br_if 0 (;@5;)
              i32.const 0
              local.set 5
              br 1 (;@4;)
            end
            local.get 2
            i32.const 4
            i32.add
            local.set 6
            local.get 3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.set 7
            i32.const 0
            local.set 5
            block  ;; label = @5
              loop  ;; label = @6
                local.get 6
                i32.load
                br_if 1 (;@5;)
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
                br_if 0 (;@6;)
              end
              local.get 7
              local.set 5
            end
            local.get 5
            local.get 3
            i32.gt_u
            br_if 1 (;@3;)
          end
          local.get 3
          local.get 5
          i32.sub
          local.tee 7
          i32.eqz
          br_if 1 (;@2;)
          local.get 2
          local.get 5
          i32.const 3
          i32.shl
          i32.add
          local.set 5
          block  ;; label = @4
            loop  ;; label = @5
              local.get 4
              i32.const 8
              i32.add
              i32.const 2
              local.get 5
              local.get 7
              call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
              block  ;; label = @6
                block  ;; label = @7
                  local.get 4
                  i32.load16_u offset=8
                  br_if 0 (;@7;)
                  local.get 4
                  i32.load offset=12
                  local.tee 8
                  br_if 1 (;@6;)
                  local.get 0
                  i32.const 1050536
                  i64.extend_i32_u
                  i64.const 32
                  i64.shl
                  i64.const 2
                  i64.or
                  i64.store align=4
                  br 6 (;@1;)
                end
                local.get 4
                local.get 4
                i32.load16_u offset=10
                i32.store16 offset=6
                local.get 4
                i32.const 6
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee 6
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if 1 (;@5;)
                local.get 0
                i32.const 0
                i32.store
                local.get 0
                i32.const 4
                i32.add
                local.get 6
                i32.store
                br 5 (;@1;)
              end
              local.get 5
              i32.const 4
              i32.add
              local.set 6
              local.get 7
              i32.const -1
              i32.add
              i32.const 536870911
              i32.and
              i32.const 1
              i32.add
              local.set 9
              i32.const 0
              local.set 3
              i32.const 0
              local.set 2
              block  ;; label = @6
                loop  ;; label = @7
                  local.get 6
                  i32.load
                  local.get 2
                  i32.add
                  local.tee 10
                  local.get 8
                  i32.gt_u
                  br_if 1 (;@6;)
                  local.get 6
                  i32.const 8
                  i32.add
                  local.set 6
                  local.get 10
                  local.set 2
                  local.get 9
                  local.get 3
                  i32.const 1
                  i32.add
                  local.tee 3
                  i32.ne
                  br_if 0 (;@7;)
                end
                local.get 10
                local.set 2
                local.get 9
                local.set 3
              end
              block  ;; label = @6
                local.get 7
                local.get 3
                i32.lt_u
                br_if 0 (;@6;)
                local.get 7
                local.get 3
                i32.sub
                local.tee 7
                i32.eqz
                br_if 4 (;@2;)
                local.get 5
                local.get 3
                i32.const 3
                i32.shl
                i32.add
                local.tee 5
                i32.load offset=4
                local.tee 3
                local.get 8
                local.get 2
                i32.sub
                local.tee 6
                i32.lt_u
                br_if 2 (;@4;)
                local.get 5
                i32.const 4
                i32.add
                local.get 3
                local.get 6
                i32.sub
                i32.store
                local.get 5
                local.get 5
                i32.load
                local.get 6
                i32.add
                i32.store
                br 1 (;@5;)
              end
            end
            local.get 3
            local.get 7
            i32.const 1050676
            call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
            unreachable
          end
          local.get 4
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get 4
          i32.const 1048792
          i32.store offset=24
          local.get 4
          i64.const 1
          i64.store offset=12 align=4
          local.get 4
          i32.const 1052112
          i32.store offset=8
          local.get 4
          i32.const 8
          i32.add
          i32.const 1052152
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 5
        local.get 3
        i32.const 1050676
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get 0
      i32.const 4
      i32.store8
    end
    local.get 4
    i32.const 32
    i32.add
    global.set $__stack_pointer)
  (func $_ZN61_$LT$$RF$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17hc123b657c412bbeeE (type 4) (param i32 i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.load
            i32.load
            local.tee 1
            i32.load
            i32.const 1059292
            i32.eq
            br_if 0 (;@4;)
            local.get 1
            i32.load8_u offset=28
            local.set 4
            local.get 1
            i32.const 1
            i32.store8 offset=28
            local.get 3
            local.get 4
            i32.const 1
            i32.and
            local.tee 4
            i32.store8 offset=8
            local.get 4
            br_if 2 (;@2;)
            local.get 1
            i32.const 1
            i32.store offset=4
            local.get 1
            i32.const 1059292
            i32.store
            br 1 (;@3;)
          end
          local.get 1
          i32.load offset=4
          local.tee 4
          i32.const 1
          i32.add
          local.tee 5
          local.get 4
          i32.lt_u
          br_if 2 (;@1;)
          local.get 1
          local.get 5
          i32.store offset=4
        end
        local.get 3
        local.get 1
        i32.store offset=4
        local.get 3
        i32.const 4
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
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const 8
            i32.add
            i32.const 1050760
            local.get 3
            i32.const 24
            i32.add
            call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.load8_u offset=12
              i32.const 4
              i32.ne
              br_if 0 (;@5;)
              local.get 0
              i32.const 1050724
              i64.extend_i32_u
              i64.const 32
              i64.shl
              i64.const 2
              i64.or
              i64.store align=4
              br 2 (;@3;)
            end
            local.get 0
            local.get 3
            i64.load offset=12 align=4
            i64.store align=4
            br 1 (;@3;)
          end
          local.get 0
          i32.const 4
          i32.store8
          local.get 3
          i32.load8_u offset=12
          i32.const 3
          i32.ne
          br_if 0 (;@3;)
          local.get 3
          i32.const 16
          i32.add
          i32.load
          local.tee 1
          i32.load
          local.get 1
          i32.load offset=4
          i32.load
          call_indirect (type 0)
          block  ;; label = @4
            local.get 1
            i32.load offset=4
            local.tee 2
            i32.load offset=4
            local.tee 0
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            i32.load
            local.get 0
            local.get 2
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
        i32.load offset=4
        local.tee 1
        local.get 1
        i32.load offset=4
        i32.const -1
        i32.add
        local.tee 2
        i32.store offset=4
        block  ;; label = @3
          local.get 2
          br_if 0 (;@3;)
          local.get 1
          i32.const 0
          i32.store8 offset=28
          local.get 1
          i32.const 0
          i32.store
        end
        local.get 3
        i32.const 48
        i32.add
        global.set $__stack_pointer
        return
      end
      local.get 3
      i32.const 44
      i32.add
      i32.const 0
      i32.store
      local.get 3
      i32.const 40
      i32.add
      i32.const 1048792
      i32.store
      local.get 3
      i64.const 1
      i64.store offset=28 align=4
      local.get 3
      i32.const 1052308
      i32.store offset=24
      local.get 3
      i32.const 8
      i32.add
      local.get 3
      i32.const 24
      i32.add
      call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
      unreachable
    end
    i32.const 1051212
    i32.const 38
    i32.const 1051288
    call $_ZN4core6option13expect_failed17h8d17c0af9e73185dE
    unreachable)
  (func $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE (type 0) (param i32)
    (local i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 96
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    local.get 1
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
    local.get 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 1
    local.get 0
    i64.load align=4
    i64.store
    local.get 1
    i32.const 6
    i32.store offset=28
    local.get 1
    i32.const 1050644
    i32.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load8_u offset=1059218
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          i32.const 0
          i32.load offset=1059284
          br_if 0 (;@3;)
          i32.const 0
          i64.const 1
          i64.store offset=1059284 align=4
          br 1 (;@2;)
        end
        i32.const 0
        i32.load offset=1059288
        local.set 0
        i32.const 0
        i32.const 0
        i32.store offset=1059288
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.load8_u offset=8
        local.set 2
        i32.const 1
        local.set 3
        local.get 0
        i32.const 1
        i32.store8 offset=8
        local.get 1
        local.get 2
        i32.const 1
        i32.and
        local.tee 2
        i32.store8 offset=56
        block  ;; label = @3
          local.get 2
          br_if 0 (;@3;)
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059280
            i32.const 2147483647
            i32.and
            i32.eqz
            br_if 0 (;@4;)
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            local.set 3
          end
          local.get 1
          i32.const 4
          i32.store8 offset=60
          local.get 1
          local.get 0
          i32.const 12
          i32.add
          i32.store offset=56
          local.get 1
          i32.const 72
          i32.add
          i32.const 16
          i32.add
          local.get 1
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
          i64.load
          i64.store
          local.get 1
          local.get 1
          i64.load
          i64.store offset=72
          block  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 56
              i32.add
              i32.const 1050784
              local.get 1
              i32.const 72
              i32.add
              call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
              i32.eqz
              br_if 0 (;@5;)
              local.get 1
              i32.load8_u offset=60
              i32.const 4
              i32.eq
              br_if 1 (;@4;)
              local.get 1
              i32.load8_u offset=60
              i32.const 3
              i32.ne
              br_if 1 (;@4;)
              local.get 1
              i32.const 64
              i32.add
              i32.load
              local.tee 2
              i32.load
              local.get 2
              i32.load offset=4
              i32.load
              call_indirect (type 0)
              block  ;; label = @6
                local.get 2
                i32.load offset=4
                local.tee 4
                i32.load offset=4
                local.tee 5
                i32.eqz
                br_if 0 (;@6;)
                local.get 2
                i32.load
                local.get 5
                local.get 4
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get 2
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              br 1 (;@4;)
            end
            local.get 1
            i32.load8_u offset=60
            i32.const 3
            i32.ne
            br_if 0 (;@4;)
            local.get 1
            i32.const 64
            i32.add
            i32.load
            local.tee 2
            i32.load
            local.get 2
            i32.load offset=4
            i32.load
            call_indirect (type 0)
            block  ;; label = @5
              local.get 2
              i32.load offset=4
              local.tee 4
              i32.load offset=4
              local.tee 5
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.load
              local.get 5
              local.get 4
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get 1
            i32.load offset=64
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          block  ;; label = @4
            local.get 3
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1059280
            i32.const 2147483647
            i32.and
            i32.eqz
            br_if 0 (;@4;)
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            br_if 0 (;@4;)
            local.get 0
            i32.const 1
            i32.store8 offset=9
          end
          local.get 0
          i32.const 0
          i32.store8 offset=8
          i32.const 0
          i32.load offset=1059288
          local.set 3
          i32.const 0
          local.get 0
          i32.store offset=1059288
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 3
          local.get 3
          i32.load
          local.tee 0
          i32.const -1
          i32.add
          i32.store
          local.get 0
          i32.const 1
          i32.ne
          br_if 2 (;@1;)
          local.get 3
          call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
          br 2 (;@1;)
        end
        local.get 1
        i32.const 92
        i32.add
        i32.const 0
        i32.store
        local.get 1
        i32.const 88
        i32.add
        i32.const 1048792
        i32.store
        local.get 1
        i64.const 1
        i64.store offset=76 align=4
        local.get 1
        i32.const 1052308
        i32.store offset=72
        local.get 1
        i32.const 56
        i32.add
        local.get 1
        i32.const 72
        i32.add
        call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
        unreachable
      end
      block  ;; label = @2
        i32.const 0
        i32.load offset=1059220
        i32.const 3
        i32.eq
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=1059220
        i32.const 3
        i32.eq
        br_if 0 (;@2;)
        local.get 1
        i32.const 1059224
        i32.store offset=56
        local.get 1
        local.get 1
        i32.const 56
        i32.add
        i32.store offset=72
        i32.const 1059220
        i32.const 1
        local.get 1
        i32.const 72
        i32.add
        i32.const 1050892
        i32.const 1050876
        call $_ZN3std4sync4once4Once10call_inner17h90795951c1d06984E
      end
      local.get 1
      i32.const 1059224
      i32.store offset=44
      local.get 1
      local.get 1
      i32.const 44
      i32.add
      i32.store offset=56
      local.get 1
      i32.const 72
      i32.add
      i32.const 16
      i32.add
      local.get 1
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
      i64.load
      i64.store
      local.get 1
      local.get 1
      i64.load
      i64.store offset=72
      local.get 1
      i32.const 32
      i32.add
      local.get 1
      i32.const 56
      i32.add
      local.get 1
      i32.const 72
      i32.add
      call $_ZN61_$LT$$RF$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17hc123b657c412bbeeE
      local.get 1
      i32.load8_u offset=32
      i32.const 4
      i32.eq
      br_if 0 (;@1;)
      local.get 1
      local.get 1
      i64.load offset=32
      i64.store offset=48
      local.get 1
      i32.const 92
      i32.add
      i32.const 2
      i32.store
      local.get 1
      i32.const 68
      i32.add
      i32.const 6
      i32.store
      local.get 1
      i64.const 2
      i64.store offset=76 align=4
      local.get 1
      i32.const 1050612
      i32.store offset=72
      local.get 1
      i32.const 5
      i32.store offset=60
      local.get 1
      local.get 1
      i32.const 56
      i32.add
      i32.store offset=88
      local.get 1
      local.get 1
      i32.const 48
      i32.add
      i32.store offset=64
      local.get 1
      local.get 1
      i32.const 24
      i32.add
      i32.store offset=56
      local.get 1
      i32.const 72
      i32.add
      i32.const 1050628
      call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
      unreachable
    end
    local.get 1
    i32.const 96
    i32.add
    global.set $__stack_pointer)
  (func $_ZN3std2io5Write9write_all17hf2d1f3fd20cf6d1dE (type 8) (param i32 i32 i32 i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          i32.eqz
          br_if 0 (;@3;)
          loop  ;; label = @4
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
            call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 4
                  i32.load16_u offset=16
                  br_if 0 (;@7;)
                  local.get 4
                  i32.load offset=20
                  local.tee 5
                  br_if 1 (;@6;)
                  local.get 0
                  i32.const 1050536
                  i64.extend_i32_u
                  i64.const 32
                  i64.shl
                  i64.const 2
                  i64.or
                  i64.store align=4
                  br 5 (;@2;)
                end
                local.get 4
                local.get 4
                i32.load16_u offset=18
                i32.store16 offset=30
                local.get 4
                i32.const 30
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee 5
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if 1 (;@5;)
                local.get 0
                i32.const 0
                i32.store
                local.get 0
                i32.const 4
                i32.add
                local.get 5
                i32.store
                br 4 (;@2;)
              end
              local.get 3
              local.get 5
              i32.lt_u
              br_if 4 (;@1;)
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
            br_if 0 (;@4;)
          end
        end
        local.get 0
        i32.const 4
        i32.store8
      end
      local.get 4
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 5
    local.get 3
    i32.const 1050692
    call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
    unreachable)
  (func $_ZN3std2io5Write18write_all_vectored17h73403c20721d838fE (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              br_if 0 (;@5;)
              i32.const 0
              local.set 5
              br 1 (;@4;)
            end
            local.get 2
            i32.const 4
            i32.add
            local.set 6
            local.get 3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.set 7
            i32.const 0
            local.set 5
            block  ;; label = @5
              loop  ;; label = @6
                local.get 6
                i32.load
                br_if 1 (;@5;)
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
                br_if 0 (;@6;)
              end
              local.get 7
              local.set 5
            end
            local.get 5
            local.get 3
            i32.gt_u
            br_if 1 (;@3;)
          end
          local.get 3
          local.get 5
          i32.sub
          local.tee 8
          i32.eqz
          br_if 1 (;@2;)
          local.get 2
          local.get 5
          i32.const 3
          i32.shl
          i32.add
          local.set 9
          local.get 1
          i32.const 4
          i32.add
          local.set 10
          block  ;; label = @4
            loop  ;; label = @5
              local.get 8
              i32.const -1
              i32.add
              i32.const 536870911
              i32.and
              local.tee 6
              i32.const 1
              i32.add
              local.tee 11
              i32.const 7
              i32.and
              local.set 5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 6
                  i32.const 7
                  i32.ge_u
                  br_if 0 (;@7;)
                  i32.const 0
                  local.set 3
                  local.get 9
                  local.set 6
                  br 1 (;@6;)
                end
                local.get 9
                i32.const 60
                i32.add
                local.set 6
                local.get 11
                i32.const 1073741816
                i32.and
                local.set 7
                i32.const 0
                local.set 3
                loop  ;; label = @7
                  local.get 6
                  i32.load
                  local.get 6
                  i32.const -8
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -16
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -24
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -32
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -40
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -48
                  i32.add
                  i32.load
                  local.get 6
                  i32.const -56
                  i32.add
                  i32.load
                  local.get 3
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  local.set 3
                  local.get 6
                  i32.const 64
                  i32.add
                  local.set 6
                  local.get 7
                  i32.const -8
                  i32.add
                  local.tee 7
                  br_if 0 (;@7;)
                end
                local.get 6
                i32.const -60
                i32.add
                local.set 6
              end
              block  ;; label = @6
                local.get 5
                i32.eqz
                br_if 0 (;@6;)
                local.get 6
                i32.const 4
                i32.add
                local.set 6
                loop  ;; label = @7
                  local.get 6
                  i32.load
                  local.get 3
                  i32.add
                  local.set 3
                  local.get 6
                  i32.const 8
                  i32.add
                  local.set 6
                  local.get 5
                  i32.const -1
                  i32.add
                  local.tee 5
                  br_if 0 (;@7;)
                end
              end
              local.get 8
              i32.const 3
              i32.shl
              local.set 5
              block  ;; label = @6
                local.get 10
                i32.load
                local.get 1
                i32.load offset=8
                local.tee 6
                i32.sub
                local.get 3
                i32.ge_u
                br_if 0 (;@6;)
                local.get 1
                local.get 6
                local.get 3
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                local.get 1
                i32.load offset=8
                local.set 6
              end
              local.get 9
              local.get 5
              i32.add
              local.set 12
              local.get 9
              local.set 5
              loop  ;; label = @6
                local.get 5
                i32.load
                local.set 2
                block  ;; label = @7
                  local.get 10
                  i32.load
                  local.get 6
                  i32.sub
                  local.get 5
                  i32.const 4
                  i32.add
                  i32.load
                  local.tee 7
                  i32.ge_u
                  br_if 0 (;@7;)
                  local.get 1
                  local.get 6
                  local.get 7
                  call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                  local.get 1
                  i32.load offset=8
                  local.set 6
                end
                local.get 1
                i32.load
                local.get 6
                i32.add
                local.get 2
                local.get 7
                call $memcpy
                drop
                local.get 1
                local.get 6
                local.get 7
                i32.add
                local.tee 6
                i32.store offset=8
                local.get 12
                local.get 5
                i32.const 8
                i32.add
                local.tee 5
                i32.ne
                br_if 0 (;@6;)
              end
              block  ;; label = @6
                local.get 3
                br_if 0 (;@6;)
                local.get 0
                i32.const 1050536
                i64.extend_i32_u
                i64.const 32
                i64.shl
                i64.const 2
                i64.or
                i64.store align=4
                br 5 (;@1;)
              end
              local.get 9
              i32.const 4
              i32.add
              local.set 6
              i32.const 0
              local.set 5
              i32.const 0
              local.set 7
              block  ;; label = @6
                loop  ;; label = @7
                  local.get 6
                  i32.load
                  local.get 7
                  i32.add
                  local.tee 2
                  local.get 3
                  i32.gt_u
                  br_if 1 (;@6;)
                  local.get 6
                  i32.const 8
                  i32.add
                  local.set 6
                  local.get 2
                  local.set 7
                  local.get 11
                  local.get 5
                  i32.const 1
                  i32.add
                  local.tee 5
                  i32.ne
                  br_if 0 (;@7;)
                end
                local.get 2
                local.set 7
                local.get 11
                local.set 5
              end
              block  ;; label = @6
                local.get 8
                local.get 5
                i32.lt_u
                br_if 0 (;@6;)
                local.get 8
                local.get 5
                i32.sub
                local.tee 8
                i32.eqz
                br_if 4 (;@2;)
                local.get 9
                local.get 5
                i32.const 3
                i32.shl
                local.tee 5
                i32.add
                local.tee 2
                i32.load offset=4
                local.tee 12
                local.get 3
                local.get 7
                i32.sub
                local.tee 6
                i32.lt_u
                br_if 2 (;@4;)
                local.get 2
                i32.const 4
                i32.add
                local.get 12
                local.get 6
                i32.sub
                i32.store
                local.get 9
                local.get 5
                i32.add
                local.tee 9
                local.get 9
                i32.load
                local.get 6
                i32.add
                i32.store
                br 1 (;@5;)
              end
            end
            local.get 5
            local.get 8
            i32.const 1050676
            call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
            unreachable
          end
          local.get 4
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get 4
          i32.const 1048792
          i32.store offset=24
          local.get 4
          i64.const 1
          i64.store offset=12 align=4
          local.get 4
          i32.const 1052112
          i32.store offset=8
          local.get 4
          i32.const 8
          i32.add
          i32.const 1052152
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 5
        local.get 3
        i32.const 1050676
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get 0
      i32.const 4
      i32.store8
    end
    local.get 4
    i32.const 32
    i32.add
    global.set $__stack_pointer)
  (func $_ZN3std2io5Write9write_fmt17hf06e34d13533586bE (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 4
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
        local.get 3
        i32.const 8
        i32.add
        i32.const 1050784
        local.get 3
        i32.const 24
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 3
          i32.load8_u offset=12
          i32.const 4
          i32.ne
          br_if 0 (;@3;)
          local.get 0
          i32.const 1050724
          i64.extend_i32_u
          i64.const 32
          i64.shl
          i64.const 2
          i64.or
          i64.store align=4
          br 2 (;@1;)
        end
        local.get 0
        local.get 3
        i64.load offset=12 align=4
        i64.store align=4
        br 1 (;@1;)
      end
      local.get 0
      i32.const 4
      i32.store8
      local.get 3
      i32.load8_u offset=12
      i32.const 3
      i32.ne
      br_if 0 (;@1;)
      local.get 3
      i32.const 16
      i32.add
      i32.load
      local.tee 2
      i32.load
      local.get 2
      i32.load offset=4
      i32.load
      call_indirect (type 0)
      block  ;; label = @2
        local.get 2
        i32.load offset=4
        local.tee 1
        i32.load offset=4
        local.tee 0
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.load
        local.get 0
        local.get 1
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
    global.set $__stack_pointer)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha099fde24b4a4dcaE (type 5) (param i32 i32 i32) (result i32)
    (local i32 i64 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 8
    i32.add
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=8
      local.tee 1
      i32.const 4
      i32.eq
      br_if 0 (;@1;)
      local.get 3
      i64.load offset=8
      local.set 4
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.add
        i32.load
        local.tee 2
        i32.load
        local.get 2
        i32.load offset=4
        i32.load
        call_indirect (type 0)
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
      local.get 4
      i64.store offset=4 align=4
    end
    local.get 3
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1
    i32.const 4
    i32.ne)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb12703cce11fe208E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      i32.load
      local.tee 3
      i32.const 4
      i32.add
      i32.load
      local.get 3
      i32.const 8
      i32.add
      local.tee 4
      i32.load
      local.tee 0
      i32.sub
      local.get 2
      i32.ge_u
      br_if 0 (;@1;)
      local.get 3
      local.get 0
      local.get 2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get 4
      i32.load
      local.set 0
    end
    local.get 3
    i32.load
    local.get 0
    i32.add
    local.get 1
    local.get 2
    call $memcpy
    drop
    local.get 4
    local.get 0
    local.get 2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN3std5panic19get_backtrace_style17h5dc7902a5c3a80f3E (type 9) (result i32)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 0
    global.set $__stack_pointer
    i32.const 0
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              i32.load offset=1059256
              br_table 3 (;@2;) 4 (;@1;) 1 (;@4;) 2 (;@3;) 0 (;@5;)
            end
            i32.const 1049028
            i32.const 40
            i32.const 1050832
            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
            unreachable
          end
          i32.const 1
          local.set 1
          br 2 (;@1;)
        end
        i32.const 2
        local.set 1
        br 1 (;@1;)
      end
      local.get 0
      i32.const 1049437
      i32.const 14
      call $_ZN3std3env7_var_os17hac8a3e6bb094e6d3E
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.load
          local.tee 1
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          local.set 2
          local.get 0
          i32.load offset=4
          local.set 3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.const 8
                i32.add
                i32.load
                i32.const -1
                i32.add
                br_table 0 (;@6;) 2 (;@4;) 2 (;@4;) 1 (;@5;) 2 (;@4;)
              end
              i32.const -2
              i32.const 0
              local.get 1
              i32.load8_u
              i32.const 48
              i32.eq
              select
              local.set 2
              br 1 (;@4;)
            end
            local.get 1
            i32.load align=1
            i32.const 1819047270
            i32.eq
            local.set 2
          end
          block  ;; label = @4
            local.get 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            local.get 3
            i32.const 1
            call $__rust_dealloc
          end
          i32.const 1
          local.set 3
          i32.const 0
          local.set 1
          block  ;; label = @4
            local.get 2
            i32.const 3
            i32.and
            br_table 2 (;@2;) 0 (;@4;) 1 (;@3;) 2 (;@2;)
          end
          i32.const 2
          local.set 3
          i32.const 1
          local.set 1
          br 1 (;@2;)
        end
        i32.const 3
        local.set 3
        i32.const 2
        local.set 1
      end
      i32.const 0
      local.get 3
      i32.store offset=1059256
    end
    local.get 0
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17h3d2bbcbe4fd788bbE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 0
    i32.const 8
    i32.add
    i32.load
    local.get 1
    call $_ZN66_$LT$std..sys..wasi..os_str..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hb769d13a4a71030dE)
  (func $_ZN3std7process5abort17h4bdc2697b17f0111E (type 7)
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $_ZN3std4sync4once4Once15call_once_force28_$u7b$$u7b$closure$u7d$$u7d$17h5da9e8acebb38efeE (type 2) (param i32 i32)
    (local i32)
    local.get 0
    i32.load
    local.tee 2
    i32.load
    local.set 0
    local.get 2
    i32.const 0
    i32.store
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1024
        i32.const 1
        call $__rust_alloc
        local.tee 2
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 0
        i32.store8 offset=28
        local.get 0
        i32.const 0
        i32.store8 offset=24
        local.get 0
        i64.const 1024
        i64.store offset=16 align=4
        local.get 0
        local.get 2
        i32.store offset=12
        local.get 0
        i32.const 0
        i32.store offset=8
        local.get 0
        i64.const 0
        i64.store align=4
        return
      end
      i32.const 1048859
      i32.const 43
      i32.const 1050912
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    i32.const 1024
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN76_$LT$std..sync..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h45ee2ecae3ad0859E (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 2
    i32.const 8
    i32.add
    local.get 1
    i32.const 1051080
    i32.const 11
    call $_ZN4core3fmt9Formatter12debug_struct17h678028cd4a965ad7E
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt8builders11DebugStruct21finish_non_exhaustive17h9dcede22c3d51d75E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hafba394e37912a5aE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 0
    i32.load8_u
    local.set 3
    local.get 2
    i32.const 8
    i32.add
    call $_ZN3std3env11current_dir17he9cc85f9a6781258E
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.load offset=8
        br_if 0 (;@2;)
        local.get 2
        i32.const 16
        i32.add
        i32.load
        local.set 4
        local.get 2
        i32.load offset=12
        local.set 0
        br 1 (;@1;)
      end
      i32.const 0
      local.set 0
      block  ;; label = @2
        local.get 2
        i32.load8_u offset=12
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        i32.const 16
        i32.add
        i32.load
        local.tee 4
        i32.load
        local.get 4
        i32.load offset=4
        i32.load
        call_indirect (type 0)
        block  ;; label = @3
          local.get 4
          i32.load offset=4
          local.tee 5
          i32.load offset=4
          local.tee 6
          i32.eqz
          br_if 0 (;@3;)
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
    end
    local.get 2
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get 2
    i32.const 1048792
    i32.store offset=24
    local.get 2
    i64.const 1
    i64.store offset=12 align=4
    local.get 2
    i32.const 1051108
    i32.store offset=8
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          local.get 2
          i32.const 8
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
          br_if 0 (;@3;)
          block  ;; label = @4
            local.get 3
            i32.const 255
            i32.and
            br_if 0 (;@4;)
            local.get 2
            i32.const 28
            i32.add
            i32.const 0
            i32.store
            local.get 2
            i32.const 1048792
            i32.store offset=24
            local.get 2
            i64.const 1
            i64.store offset=12 align=4
            local.get 2
            i32.const 1051204
            i32.store offset=8
            local.get 1
            local.get 2
            i32.const 8
            i32.add
            call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
            br_if 1 (;@3;)
          end
          i32.const 0
          local.set 1
          local.get 0
          i32.eqz
          br_if 2 (;@1;)
          local.get 4
          i32.eqz
          br_if 2 (;@1;)
          br 1 (;@2;)
        end
        i32.const 1
        local.set 1
        local.get 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        i32.eqz
        br_if 1 (;@1;)
      end
      local.get 0
      local.get 4
      i32.const 1
      call $__rust_dealloc
    end
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17ha133246273fcb7e9E (type 0) (param i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    local.get 0
    i32.load offset=8
    call $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h20406b5ff3712528E
    unreachable)
  (func $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h20406b5ff3712528E (type 4) (param i32 i32 i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 0
    i32.const 20
    i32.add
    i32.load
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.const 4
            i32.add
            i32.load
            br_table 0 (;@4;) 1 (;@3;) 3 (;@1;)
          end
          local.get 4
          br_if 2 (;@1;)
          i32.const 1048792
          local.set 0
          i32.const 0
          local.set 4
          br 1 (;@2;)
        end
        local.get 4
        br_if 1 (;@1;)
        local.get 0
        i32.load
        local.tee 0
        i32.load offset=4
        local.set 4
        local.get 0
        i32.load
        local.set 0
      end
      local.get 3
      local.get 4
      i32.store offset=4
      local.get 3
      local.get 0
      i32.store
      local.get 3
      i32.const 1051836
      local.get 1
      call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
      local.get 2
      local.get 1
      call $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE
      call $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E
      unreachable
    end
    local.get 3
    i32.const 0
    i32.store offset=4
    local.get 3
    local.get 0
    i32.store
    local.get 3
    i32.const 1051816
    local.get 1
    call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
    local.get 2
    local.get 1
    call $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE
    call $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E
    unreachable)
  (func $_ZN3std5alloc24default_alloc_error_hook17h5a1a1d982f9bd818E (type 2) (param i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      i32.const 0
      i32.load8_u offset=1059216
      br_if 0 (;@1;)
      local.get 2
      i32.const 7
      i32.store offset=4
      local.get 2
      local.get 0
      i32.store offset=12
      local.get 2
      local.get 2
      i32.const 12
      i32.add
      i32.store
      local.get 2
      i32.const 4
      i32.store8 offset=20
      local.get 2
      local.get 2
      i32.const 56
      i32.add
      i32.store offset=16
      local.get 2
      i32.const 52
      i32.add
      i32.const 1
      i32.store
      local.get 2
      i64.const 2
      i64.store offset=36 align=4
      local.get 2
      i32.const 1051400
      i32.store offset=32
      local.get 2
      local.get 2
      i32.store offset=48
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.const 16
          i32.add
          i32.const 1050736
          local.get 2
          i32.const 32
          i32.add
          call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          i32.load8_u offset=20
          i32.const 4
          i32.eq
          br_if 1 (;@2;)
          local.get 2
          i32.load8_u offset=20
          i32.const 3
          i32.ne
          br_if 1 (;@2;)
          local.get 2
          i32.const 24
          i32.add
          i32.load
          local.tee 0
          i32.load
          local.get 0
          i32.load offset=4
          i32.load
          call_indirect (type 0)
          block  ;; label = @4
            local.get 0
            i32.load offset=4
            local.tee 3
            i32.load offset=4
            local.tee 4
            i32.eqz
            br_if 0 (;@4;)
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
          br 1 (;@2;)
        end
        local.get 2
        i32.load8_u offset=20
        i32.const 3
        i32.ne
        br_if 0 (;@2;)
        local.get 2
        i32.const 24
        i32.add
        i32.load
        local.tee 0
        i32.load
        local.get 0
        i32.load offset=4
        i32.load
        call_indirect (type 0)
        block  ;; label = @3
          local.get 0
          i32.load offset=4
          local.tee 3
          i32.load offset=4
          local.tee 4
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load
          local.get 4
          local.get 3
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get 2
        i32.load offset=24
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get 2
      i32.const 64
      i32.add
      global.set $__stack_pointer
      return
    end
    local.get 2
    i32.const 52
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=36 align=4
    local.get 2
    i32.const 1051400
    i32.store offset=32
    local.get 2
    i32.const 7
    i32.store offset=20
    local.get 2
    local.get 0
    i32.store
    local.get 2
    local.get 2
    i32.const 16
    i32.add
    i32.store offset=48
    local.get 2
    local.get 2
    i32.store offset=16
    local.get 2
    i32.const 32
    i32.add
    i32.const 1051440
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $rust_oom (type 2) (param i32 i32)
    (local i32)
    local.get 0
    local.get 1
    i32.const 0
    i32.load offset=1059264
    local.tee 2
    i32.const 8
    local.get 2
    select
    call_indirect (type 2)
    call $_ZN3std7process5abort17h4bdc2697b17f0111E
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
  (func $__rdl_dealloc (type 4) (param i32 i32 i32)
    local.get 0
    call $free)
  (func $__rdl_realloc (type 6) (param i32 i32 i32 i32) (result i32)
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
  (func $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 3
    global.set $__stack_pointer
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
    i32.const 9
    i32.store
    local.get 3
    i32.const 44
    i32.add
    i32.const 5
    i32.store
    local.get 3
    i64.const 4
    i64.store offset=4 align=4
    local.get 3
    i32.const 1051632
    i32.store
    local.get 3
    i32.const 5
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
    local.tee 4
    call_indirect (type 4)
    block  ;; label = @1
      local.get 3
      i32.load8_u offset=24
      i32.const 3
      i32.ne
      br_if 0 (;@1;)
      local.get 3
      i32.load offset=28
      local.tee 2
      i32.load
      local.get 2
      i32.load offset=4
      i32.load
      call_indirect (type 0)
      block  ;; label = @2
        local.get 2
        i32.load offset=4
        local.tee 5
        i32.load offset=4
        local.tee 6
        i32.eqz
        br_if 0 (;@2;)
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
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.load offset=12
          i32.load8_u
          local.tee 0
          i32.const 3
          i32.eq
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                br_table 0 (;@6;) 1 (;@5;) 2 (;@4;) 0 (;@6;)
              end
              i32.const 0
              i32.load8_u offset=1059260
              local.set 0
              i32.const 0
              i32.const 1
              i32.store8 offset=1059260
              local.get 3
              local.get 0
              i32.store8
              local.get 0
              br_if 3 (;@2;)
              local.get 3
              i32.const 52
              i32.add
              i32.const 1
              i32.store
              local.get 3
              i64.const 1
              i64.store offset=36 align=4
              local.get 3
              i32.const 1049524
              i32.store offset=32
              local.get 3
              i32.const 10
              i32.store offset=4
              local.get 3
              i32.const 0
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
              local.get 4
              call_indirect (type 4)
              i32.const 0
              i32.const 0
              i32.store8 offset=1059260
              local.get 3
              i32.load8_u offset=24
              i32.const 3
              i32.ne
              br_if 2 (;@3;)
              local.get 3
              i32.load offset=28
              local.tee 0
              i32.load
              local.get 0
              i32.load offset=4
              i32.load
              call_indirect (type 0)
              block  ;; label = @6
                local.get 0
                i32.load offset=4
                local.tee 1
                i32.load offset=4
                local.tee 2
                i32.eqz
                br_if 0 (;@6;)
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
              br 2 (;@3;)
            end
            i32.const 0
            i32.load8_u offset=1059260
            local.set 0
            i32.const 0
            i32.const 1
            i32.store8 offset=1059260
            local.get 3
            local.get 0
            i32.store8
            local.get 0
            br_if 3 (;@1;)
            local.get 3
            i32.const 52
            i32.add
            i32.const 1
            i32.store
            local.get 3
            i64.const 1
            i64.store offset=36 align=4
            local.get 3
            i32.const 1049524
            i32.store offset=32
            local.get 3
            i32.const 10
            i32.store offset=4
            local.get 3
            i32.const 1
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
            local.get 4
            call_indirect (type 4)
            i32.const 0
            i32.const 0
            i32.store8 offset=1059260
            local.get 3
            i32.load8_u offset=24
            i32.const 3
            i32.ne
            br_if 1 (;@3;)
            local.get 3
            i32.load offset=28
            local.tee 0
            i32.load
            local.get 0
            i32.load offset=4
            i32.load
            call_indirect (type 0)
            block  ;; label = @5
              local.get 0
              i32.load offset=4
              local.tee 1
              i32.load offset=4
              local.tee 2
              i32.eqz
              br_if 0 (;@5;)
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
            br 1 (;@3;)
          end
          i32.const 0
          i32.load8_u offset=1059208
          local.set 0
          i32.const 0
          i32.const 0
          i32.store8 offset=1059208
          local.get 0
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          i32.const 52
          i32.add
          i32.const 0
          i32.store
          local.get 3
          i32.const 1048792
          i32.store offset=48
          local.get 3
          i64.const 1
          i64.store offset=36 align=4
          local.get 3
          i32.const 1051744
          i32.store offset=32
          local.get 3
          local.get 1
          local.get 3
          i32.const 32
          i32.add
          local.get 4
          call_indirect (type 4)
          local.get 3
          i32.load8_u
          i32.const 3
          i32.ne
          br_if 0 (;@3;)
          local.get 3
          i32.load offset=4
          local.tee 0
          i32.load
          local.get 0
          i32.load offset=4
          i32.load
          call_indirect (type 0)
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
        end
        local.get 3
        i32.const 64
        i32.add
        global.set $__stack_pointer
        return
      end
      local.get 3
      i32.const 52
      i32.add
      i32.const 0
      i32.store
      local.get 3
      i32.const 48
      i32.add
      i32.const 1048792
      i32.store
      local.get 3
      i64.const 1
      i64.store offset=36 align=4
      local.get 3
      i32.const 1052308
      i32.store offset=32
      local.get 3
      local.get 3
      i32.const 32
      i32.add
      call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
      unreachable
    end
    local.get 3
    i32.const 52
    i32.add
    i32.const 0
    i32.store
    local.get 3
    i32.const 48
    i32.add
    i32.const 1048792
    i32.store
    local.get 3
    i64.const 1
    i64.store offset=36 align=4
    local.get 3
    i32.const 1052308
    i32.store offset=32
    local.get 3
    local.get 3
    i32.const 32
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $rust_begin_unwind (type 0) (param i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    local.get 0
    call $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE
    i32.const 1051752
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17h38e7befd79b1f826E
    local.set 2
    local.get 0
    call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17h88efffa862cbe9d0E
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
    call $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17ha133246273fcb7e9E
    unreachable)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h34d0cd2380313f86E (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i64)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee 5
      i32.const 0
      i32.store
      local.get 2
      i64.const 1
      i64.store offset=8
      local.get 2
      local.get 2
      i32.const 8
      i32.add
      i32.store offset=20
      local.get 2
      i32.const 24
      i32.add
      i32.const 16
      i32.add
      local.get 4
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      i32.const 24
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
      i64.store offset=24
      local.get 2
      i32.const 20
      i32.add
      i32.const 1048768
      local.get 2
      i32.const 24
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      drop
      local.get 3
      i32.const 8
      i32.add
      local.get 5
      i32.load
      i32.store
      local.get 3
      local.get 2
      i64.load offset=8
      i64.store align=4
    end
    local.get 2
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.tee 4
    local.get 3
    i32.const 8
    i32.add
    i32.load
    i32.store
    local.get 1
    i32.const 12
    i32.add
    i32.const 0
    i32.store
    local.get 3
    i64.load align=4
    local.set 6
    local.get 1
    i64.const 1
    i64.store offset=4 align=4
    local.get 2
    local.get 6
    i64.store offset=24
    block  ;; label = @1
      i32.const 12
      i32.const 4
      call $__rust_alloc
      local.tee 1
      br_if 0 (;@1;)
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get 1
    local.get 2
    i64.load offset=24
    i64.store align=4
    local.get 1
    i32.const 8
    i32.add
    local.get 4
    i32.load
    i32.store
    local.get 0
    i32.const 1051784
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store
    local.get 2
    i32.const 48
    i32.add
    global.set $__stack_pointer)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17ha161f6684539a1beE (type 2) (param i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
      local.set 1
      local.get 2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee 4
      i32.const 0
      i32.store
      local.get 2
      i64.const 1
      i64.store offset=8
      local.get 2
      local.get 2
      i32.const 8
      i32.add
      i32.store offset=20
      local.get 2
      i32.const 24
      i32.add
      i32.const 16
      i32.add
      local.get 1
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get 2
      i32.const 24
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
      i64.store offset=24
      local.get 2
      i32.const 20
      i32.add
      i32.const 1048768
      local.get 2
      i32.const 24
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      drop
      local.get 3
      i32.const 8
      i32.add
      local.get 4
      i32.load
      i32.store
      local.get 3
      local.get 2
      i64.load offset=8
      i64.store align=4
    end
    local.get 0
    i32.const 1051784
    i32.store offset=4
    local.get 0
    local.get 3
    i32.store
    local.get 2
    i32.const 48
    i32.add
    global.set $__stack_pointer)
  (func $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h652349175b493901E (type 2) (param i32 i32)
    (local i32 i32)
    local.get 1
    i32.load offset=4
    local.set 2
    local.get 1
    i32.load
    local.set 3
    block  ;; label = @1
      i32.const 8
      i32.const 4
      call $__rust_alloc
      local.tee 1
      br_if 0 (;@1;)
      i32.const 8
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get 1
    local.get 2
    i32.store offset=4
    local.get 1
    local.get 3
    i32.store
    local.get 0
    i32.const 1051800
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17hdfcd929547320219E (type 2) (param i32 i32)
    local.get 0
    i32.const 1051800
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E (type 11) (param i32 i32 i32 i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 112
    i32.sub
    local.tee 5
    global.set $__stack_pointer
    i32.const 0
    i32.const 0
    i32.load offset=1059280
    local.tee 6
    i32.const 1
    i32.add
    i32.store offset=1059280
    i32.const 0
    i32.const 0
    i32.load offset=1059304
    i32.const 1
    i32.add
    local.tee 7
    i32.store offset=1059304
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 6
            i32.const 0
            i32.lt_s
            br_if 0 (;@4;)
            local.get 7
            i32.const 2
            i32.gt_u
            br_if 0 (;@4;)
            local.get 5
            local.get 4
            i32.store8 offset=32
            local.get 5
            local.get 3
            i32.store offset=28
            local.get 5
            local.get 2
            i32.store offset=24
            i32.const 0
            i32.load offset=1059268
            local.tee 6
            i32.const -1
            i32.le_s
            br_if 2 (;@2;)
            i32.const 0
            local.get 6
            i32.const 1
            i32.add
            i32.store offset=1059268
            i32.const 0
            i32.load offset=1059276
            local.tee 6
            i32.eqz
            br_if 1 (;@3;)
            i32.const 0
            i32.load offset=1059272
            local.set 2
            local.get 5
            i32.const 8
            i32.add
            local.get 0
            local.get 1
            i32.load offset=16
            call_indirect (type 2)
            local.get 5
            local.get 5
            i64.load offset=8
            i64.store offset=16
            local.get 2
            local.get 5
            i32.const 16
            i32.add
            local.get 6
            i32.load offset=20
            call_indirect (type 2)
            br 3 (;@1;)
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 7
              i32.const 2
              i32.gt_u
              br_if 0 (;@5;)
              local.get 5
              local.get 4
              i32.store8 offset=64
              local.get 5
              local.get 3
              i32.store offset=60
              local.get 5
              local.get 2
              i32.store offset=56
              local.get 5
              i32.const 1048808
              i32.store offset=52
              local.get 5
              i32.const 1048792
              i32.store offset=48
              local.get 5
              i32.const 11
              i32.store offset=76
              local.get 5
              local.get 5
              i32.const 48
              i32.add
              i32.store offset=72
              local.get 5
              i32.const 4
              i32.store8 offset=20
              local.get 5
              local.get 5
              i32.const 104
              i32.add
              i32.store offset=16
              local.get 5
              i32.const 100
              i32.add
              i32.const 1
              i32.store
              local.get 5
              i64.const 2
              i64.store offset=84 align=4
              local.get 5
              i32.const 1051968
              i32.store offset=80
              local.get 5
              local.get 5
              i32.const 72
              i32.add
              i32.store offset=96
              block  ;; label = @6
                local.get 5
                i32.const 16
                i32.add
                i32.const 1050736
                local.get 5
                i32.const 80
                i32.add
                call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
                i32.eqz
                br_if 0 (;@6;)
                local.get 5
                i32.load8_u offset=20
                i32.const 4
                i32.eq
                br_if 2 (;@4;)
                local.get 5
                i32.load8_u offset=20
                i32.const 3
                i32.ne
                br_if 2 (;@4;)
                local.get 5
                i32.const 24
                i32.add
                i32.load
                local.tee 5
                i32.load
                local.get 5
                i32.load offset=4
                i32.load
                call_indirect (type 0)
                block  ;; label = @7
                  local.get 5
                  i32.load offset=4
                  local.tee 7
                  i32.load offset=4
                  local.tee 6
                  i32.eqz
                  br_if 0 (;@7;)
                  local.get 5
                  i32.load
                  local.get 6
                  local.get 7
                  i32.load offset=8
                  call $__rust_dealloc
                end
                local.get 5
                i32.const 12
                i32.const 4
                call $__rust_dealloc
                call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
                unreachable
              end
              local.get 5
              i32.load8_u offset=20
              i32.const 3
              i32.ne
              br_if 1 (;@4;)
              local.get 5
              i32.const 24
              i32.add
              i32.load
              local.tee 7
              i32.load
              local.get 7
              i32.load offset=4
              i32.load
              call_indirect (type 0)
              block  ;; label = @6
                local.get 7
                i32.load offset=4
                local.tee 6
                i32.load offset=4
                local.tee 4
                i32.eqz
                br_if 0 (;@6;)
                local.get 7
                i32.load
                local.get 4
                local.get 6
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get 5
              i32.load offset=24
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
              unreachable
            end
            local.get 5
            i32.const 4
            i32.store8 offset=52
            local.get 5
            local.get 5
            i32.const 104
            i32.add
            i32.store offset=48
            local.get 5
            i32.const 100
            i32.add
            i32.const 0
            i32.store
            local.get 5
            i32.const 1048792
            i32.store offset=96
            local.get 5
            i64.const 1
            i64.store offset=84 align=4
            local.get 5
            i32.const 1051908
            i32.store offset=80
            block  ;; label = @5
              local.get 5
              i32.const 48
              i32.add
              i32.const 1050736
              local.get 5
              i32.const 80
              i32.add
              call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
              i32.eqz
              br_if 0 (;@5;)
              local.get 5
              i32.load8_u offset=52
              i32.const 4
              i32.eq
              br_if 1 (;@4;)
              local.get 5
              i32.load8_u offset=52
              i32.const 3
              i32.ne
              br_if 1 (;@4;)
              local.get 5
              i32.const 56
              i32.add
              i32.load
              local.tee 5
              i32.load
              local.get 5
              i32.load offset=4
              i32.load
              call_indirect (type 0)
              block  ;; label = @6
                local.get 5
                i32.load offset=4
                local.tee 7
                i32.load offset=4
                local.tee 6
                i32.eqz
                br_if 0 (;@6;)
                local.get 5
                i32.load
                local.get 6
                local.get 7
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get 5
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
              unreachable
            end
            local.get 5
            i32.load8_u offset=52
            i32.const 3
            i32.ne
            br_if 0 (;@4;)
            local.get 5
            i32.const 56
            i32.add
            i32.load
            local.tee 7
            i32.load
            local.get 7
            i32.load offset=4
            i32.load
            call_indirect (type 0)
            block  ;; label = @5
              local.get 7
              i32.load offset=4
              local.tee 6
              i32.load offset=4
              local.tee 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 7
              i32.load
              local.get 4
              local.get 6
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get 5
            i32.load offset=56
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
          unreachable
        end
        local.get 5
        local.get 0
        local.get 1
        i32.load offset=16
        call_indirect (type 2)
        local.get 5
        local.get 5
        i64.load
        i64.store offset=16
        local.get 5
        i32.const 16
        i32.add
        call $_ZN3std9panicking12default_hook17h9e7a62be9c47f791E
        br 1 (;@1;)
      end
      local.get 5
      i32.const 48
      i32.add
      i32.const 20
      i32.add
      i32.const 1
      i32.store
      local.get 5
      i32.const 80
      i32.add
      i32.const 20
      i32.add
      i32.const 0
      i32.store
      local.get 5
      i64.const 2
      i64.store offset=52 align=4
      local.get 5
      i32.const 1049184
      i32.store offset=48
      local.get 5
      i32.const 12
      i32.store offset=76
      local.get 5
      i32.const 1048792
      i32.store offset=96
      local.get 5
      i64.const 1
      i64.store offset=84 align=4
      local.get 5
      i32.const 1052416
      i32.store offset=80
      local.get 5
      local.get 5
      i32.const 72
      i32.add
      i32.store offset=64
      local.get 5
      local.get 5
      i32.const 80
      i32.add
      i32.store offset=72
      local.get 5
      i32.const 40
      i32.add
      local.get 5
      i32.const 104
      i32.add
      local.get 5
      i32.const 48
      i32.add
      call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
      local.get 5
      i32.const 40
      i32.add
      call $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE
      call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
      unreachable
    end
    i32.const 0
    i32.const 0
    i32.load offset=1059268
    i32.const -1
    i32.add
    i32.store offset=1059268
    block  ;; label = @1
      local.get 7
      i32.const 1
      i32.gt_u
      br_if 0 (;@1;)
      local.get 4
      i32.eqz
      br_if 0 (;@1;)
      local.get 0
      local.get 1
      call $rust_panic
      unreachable
    end
    local.get 5
    i32.const 100
    i32.add
    i32.const 0
    i32.store
    local.get 5
    i32.const 1048792
    i32.store offset=96
    local.get 5
    i64.const 1
    i64.store offset=84 align=4
    local.get 5
    i32.const 1052028
    i32.store offset=80
    local.get 5
    i32.const 48
    i32.add
    local.get 5
    i32.const 104
    i32.add
    local.get 5
    i32.const 80
    i32.add
    call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
    local.get 5
    i32.const 48
    i32.add
    call $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $rust_panic (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 96
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 24
    i32.add
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i32.const 56
    i32.add
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=28 align=4
    local.get 2
    i32.const 1049184
    i32.store offset=24
    local.get 2
    i32.const 12
    i32.store offset=52
    local.get 2
    i64.const 1
    i64.store offset=60 align=4
    local.get 2
    i32.const 1052068
    i32.store offset=56
    local.get 2
    i32.const 7
    i32.store offset=84
    local.get 2
    local.get 2
    i32.const 48
    i32.add
    i32.store offset=40
    local.get 2
    local.get 2
    i32.const 56
    i32.add
    i32.store offset=48
    local.get 2
    local.get 2
    i32.const 80
    i32.add
    i32.store offset=72
    local.get 2
    local.get 2
    i32.const 12
    i32.add
    i32.store offset=80
    local.get 2
    i32.const 16
    i32.add
    local.get 2
    i32.const 88
    i32.add
    local.get 2
    i32.const 24
    i32.add
    call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
    local.get 2
    i32.const 16
    i32.add
    call $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17h7fa424a882c725e9E (type 8) (param i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 4
    global.set $__stack_pointer
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
    call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
    block  ;; label = @1
      block  ;; label = @2
        local.get 4
        i32.load16_u offset=16
        br_if 0 (;@2;)
        local.get 0
        local.get 4
        i32.load offset=20
        i32.store offset=4
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      local.get 4
      local.get 4
      i32.load16_u offset=18
      i32.store16 offset=30
      local.get 0
      local.get 4
      i32.const 30
      i32.add
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i64.extend_i32_u
      i64.const 65535
      i64.and
      i64.const 32
      i64.shl
      i64.store offset=4 align=4
    end
    local.get 0
    local.get 2
    i32.store
    local.get 4
    i32.const 32
    i32.add
    global.set $__stack_pointer)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17he7f7854621726461E (type 8) (param i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    local.get 4
    i32.const 2
    local.get 2
    local.get 3
    call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
    block  ;; label = @1
      block  ;; label = @2
        local.get 4
        i32.load16_u
        br_if 0 (;@2;)
        local.get 0
        local.get 4
        i32.load offset=4
        i32.store offset=4
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      local.get 4
      local.get 4
      i32.load16_u offset=2
      i32.store16 offset=14
      local.get 0
      local.get 4
      i32.const 14
      i32.add
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i64.extend_i32_u
      i64.const 65535
      i64.and
      i64.const 32
      i64.shl
      i64.store offset=4 align=4
      i32.const 1
      local.set 2
    end
    local.get 0
    local.get 2
    i32.store
    local.get 4
    i32.const 16
    i32.add
    global.set $__stack_pointer)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h1cc0993ef9fea8dfE (type 10) (param i32) (result i32)
    i32.const 1)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5flush17h5d2f24e96e822d71E (type 2) (param i32 i32)
    local.get 0
    i32.const 4
    i32.store8)
  (func $__rust_start_panic (type 10) (param i32) (result i32)
    unreachable
    unreachable)
  (func $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E (type 10) (param i32) (result i32)
    local.get 0
    i32.load16_u)
  (func $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    call $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hfe12d9463b2a7790E
    local.tee 1
    i32.store16 offset=2
    local.get 0
    local.get 1
    i32.const 0
    i32.ne
    i32.store16)
  (func $_ZN4wasi13lib_generated14args_sizes_get17h4999a6465b98090cE (type 0) (param i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 8
        i32.add
        local.get 1
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h19746ec286063fe2E
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
    global.set $__stack_pointer)
  (func $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE (type 8) (param i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        local.get 2
        local.get 3
        local.get 4
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h2fb5d28ef433e1b2E
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
    global.set $__stack_pointer)
  (func $malloc (type 10) (param i32) (result i32)
    local.get 0
    call $dlmalloc)
  (func $dlmalloc (type 10) (param i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059332
      br_if 0 (;@1;)
      i32.const 0
      call $sbrk
      i32.const 1059856
      i32.sub
      local.tee 2
      i32.const 89
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 0
      local.set 3
      block  ;; label = @2
        i32.const 0
        i32.load offset=1059780
        local.tee 4
        br_if 0 (;@2;)
        i32.const 0
        i64.const -1
        i64.store offset=1059792 align=4
        i32.const 0
        i64.const 281474976776192
        i64.store offset=1059784 align=4
        i32.const 0
        local.get 1
        i32.const 8
        i32.add
        i32.const -16
        i32.and
        i32.const 1431655768
        i32.xor
        local.tee 4
        i32.store offset=1059780
        i32.const 0
        i32.const 0
        i32.store offset=1059800
        i32.const 0
        i32.const 0
        i32.store offset=1059752
      end
      i32.const 0
      local.get 2
      i32.store offset=1059760
      i32.const 0
      i32.const 1059856
      i32.store offset=1059756
      i32.const 0
      i32.const 1059856
      i32.store offset=1059324
      i32.const 0
      local.get 4
      i32.store offset=1059344
      i32.const 0
      i32.const -1
      i32.store offset=1059340
      loop  ;; label = @2
        local.get 3
        i32.const 1059368
        i32.add
        local.get 3
        i32.const 1059356
        i32.add
        local.tee 4
        i32.store
        local.get 4
        local.get 3
        i32.const 1059348
        i32.add
        local.tee 5
        i32.store
        local.get 3
        i32.const 1059360
        i32.add
        local.get 5
        i32.store
        local.get 3
        i32.const 1059376
        i32.add
        local.get 3
        i32.const 1059364
        i32.add
        local.tee 5
        i32.store
        local.get 5
        local.get 4
        i32.store
        local.get 3
        i32.const 1059384
        i32.add
        local.get 3
        i32.const 1059372
        i32.add
        local.tee 4
        i32.store
        local.get 4
        local.get 5
        i32.store
        local.get 3
        i32.const 1059380
        i32.add
        local.get 4
        i32.store
        local.get 3
        i32.const 32
        i32.add
        local.tee 3
        i32.const 256
        i32.ne
        br_if 0 (;@2;)
      end
      i32.const 1059856
      i32.const -8
      i32.const 1059856
      i32.sub
      i32.const 15
      i32.and
      i32.const 0
      i32.const 1059856
      i32.const 8
      i32.add
      i32.const 15
      i32.and
      select
      local.tee 3
      i32.add
      local.tee 4
      i32.const 4
      i32.add
      local.get 2
      i32.const -56
      i32.add
      local.tee 5
      local.get 3
      i32.sub
      local.tee 3
      i32.const 1
      i32.or
      i32.store
      i32.const 0
      i32.const 0
      i32.load offset=1059796
      i32.store offset=1059336
      i32.const 0
      local.get 3
      i32.store offset=1059320
      i32.const 0
      local.get 4
      i32.store offset=1059332
      i32.const 1059856
      local.get 5
      i32.add
      i32.const 56
      i32.store offset=4
    end
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
                              i32.load offset=1059308
                              local.tee 6
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
                              local.tee 2
                              i32.const 3
                              i32.shr_u
                              local.tee 4
                              i32.shr_u
                              local.tee 3
                              i32.const 3
                              i32.and
                              i32.eqz
                              br_if 0 (;@13;)
                              local.get 3
                              i32.const 1
                              i32.and
                              local.get 4
                              i32.or
                              i32.const 1
                              i32.xor
                              local.tee 5
                              i32.const 3
                              i32.shl
                              local.tee 0
                              i32.const 1059356
                              i32.add
                              i32.load
                              local.tee 4
                              i32.const 8
                              i32.add
                              local.set 3
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 4
                                  i32.load offset=8
                                  local.tee 2
                                  local.get 0
                                  i32.const 1059348
                                  i32.add
                                  local.tee 0
                                  i32.ne
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.get 6
                                  i32.const -2
                                  local.get 5
                                  i32.rotl
                                  i32.and
                                  i32.store offset=1059308
                                  br 1 (;@14;)
                                end
                                local.get 0
                                local.get 2
                                i32.store offset=8
                                local.get 2
                                local.get 0
                                i32.store offset=12
                              end
                              local.get 4
                              local.get 5
                              i32.const 3
                              i32.shl
                              local.tee 5
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get 4
                              local.get 5
                              i32.add
                              local.tee 4
                              local.get 4
                              i32.load offset=4
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              br 12 (;@1;)
                            end
                            local.get 2
                            i32.const 0
                            i32.load offset=1059316
                            local.tee 7
                            i32.le_u
                            br_if 1 (;@11;)
                            block  ;; label = @13
                              local.get 3
                              i32.eqz
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 3
                                  local.get 4
                                  i32.shl
                                  i32.const 2
                                  local.get 4
                                  i32.shl
                                  local.tee 3
                                  i32.const 0
                                  local.get 3
                                  i32.sub
                                  i32.or
                                  i32.and
                                  local.tee 3
                                  i32.const 0
                                  local.get 3
                                  i32.sub
                                  i32.and
                                  i32.const -1
                                  i32.add
                                  local.tee 3
                                  local.get 3
                                  i32.const 12
                                  i32.shr_u
                                  i32.const 16
                                  i32.and
                                  local.tee 3
                                  i32.shr_u
                                  local.tee 4
                                  i32.const 5
                                  i32.shr_u
                                  i32.const 8
                                  i32.and
                                  local.tee 5
                                  local.get 3
                                  i32.or
                                  local.get 4
                                  local.get 5
                                  i32.shr_u
                                  local.tee 3
                                  i32.const 2
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 3
                                  local.get 4
                                  i32.shr_u
                                  local.tee 3
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 2
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 3
                                  local.get 4
                                  i32.shr_u
                                  local.tee 3
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 1
                                  i32.and
                                  local.tee 4
                                  i32.or
                                  local.get 3
                                  local.get 4
                                  i32.shr_u
                                  i32.add
                                  local.tee 5
                                  i32.const 3
                                  i32.shl
                                  local.tee 0
                                  i32.const 1059356
                                  i32.add
                                  i32.load
                                  local.tee 4
                                  i32.load offset=8
                                  local.tee 3
                                  local.get 0
                                  i32.const 1059348
                                  i32.add
                                  local.tee 0
                                  i32.ne
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.get 6
                                  i32.const -2
                                  local.get 5
                                  i32.rotl
                                  i32.and
                                  local.tee 6
                                  i32.store offset=1059308
                                  br 1 (;@14;)
                                end
                                local.get 0
                                local.get 3
                                i32.store offset=8
                                local.get 3
                                local.get 0
                                i32.store offset=12
                              end
                              local.get 4
                              i32.const 8
                              i32.add
                              local.set 3
                              local.get 4
                              local.get 2
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get 4
                              local.get 5
                              i32.const 3
                              i32.shl
                              local.tee 5
                              i32.add
                              local.get 5
                              local.get 2
                              i32.sub
                              local.tee 5
                              i32.store
                              local.get 4
                              local.get 2
                              i32.add
                              local.tee 0
                              local.get 5
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
                                i32.const 1059348
                                i32.add
                                local.set 2
                                i32.const 0
                                i32.load offset=1059328
                                local.set 4
                                block  ;; label = @15
                                  block  ;; label = @16
                                    local.get 6
                                    i32.const 1
                                    local.get 8
                                    i32.shl
                                    local.tee 8
                                    i32.and
                                    br_if 0 (;@16;)
                                    i32.const 0
                                    local.get 6
                                    local.get 8
                                    i32.or
                                    i32.store offset=1059308
                                    local.get 2
                                    local.set 8
                                    br 1 (;@15;)
                                  end
                                  local.get 2
                                  i32.load offset=8
                                  local.set 8
                                end
                                local.get 8
                                local.get 4
                                i32.store offset=12
                                local.get 2
                                local.get 4
                                i32.store offset=8
                                local.get 4
                                local.get 2
                                i32.store offset=12
                                local.get 4
                                local.get 8
                                i32.store offset=8
                              end
                              i32.const 0
                              local.get 0
                              i32.store offset=1059328
                              i32.const 0
                              local.get 5
                              i32.store offset=1059316
                              br 12 (;@1;)
                            end
                            i32.const 0
                            i32.load offset=1059312
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
                            local.tee 3
                            local.get 3
                            i32.const 12
                            i32.shr_u
                            i32.const 16
                            i32.and
                            local.tee 3
                            i32.shr_u
                            local.tee 4
                            i32.const 5
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee 5
                            local.get 3
                            i32.or
                            local.get 4
                            local.get 5
                            i32.shr_u
                            local.tee 3
                            i32.const 2
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 3
                            local.get 4
                            i32.shr_u
                            local.tee 3
                            i32.const 1
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 3
                            local.get 4
                            i32.shr_u
                            local.tee 3
                            i32.const 1
                            i32.shr_u
                            i32.const 1
                            i32.and
                            local.tee 4
                            i32.or
                            local.get 3
                            local.get 4
                            i32.shr_u
                            i32.add
                            i32.const 2
                            i32.shl
                            i32.const 1059612
                            i32.add
                            i32.load
                            local.tee 0
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            local.get 2
                            i32.sub
                            local.set 4
                            local.get 0
                            local.set 5
                            block  ;; label = @13
                              loop  ;; label = @14
                                block  ;; label = @15
                                  local.get 5
                                  i32.load offset=16
                                  local.tee 3
                                  br_if 0 (;@15;)
                                  local.get 5
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee 3
                                  i32.eqz
                                  br_if 2 (;@13;)
                                end
                                local.get 3
                                i32.load offset=4
                                i32.const -8
                                i32.and
                                local.get 2
                                i32.sub
                                local.tee 5
                                local.get 4
                                local.get 5
                                local.get 4
                                i32.lt_u
                                local.tee 5
                                select
                                local.set 4
                                local.get 3
                                local.get 0
                                local.get 5
                                select
                                local.set 0
                                local.get 3
                                local.set 5
                                br 0 (;@14;)
                              end
                            end
                            local.get 0
                            i32.load offset=24
                            local.set 10
                            block  ;; label = @13
                              local.get 0
                              i32.load offset=12
                              local.tee 8
                              local.get 0
                              i32.eq
                              br_if 0 (;@13;)
                              i32.const 0
                              i32.load offset=1059324
                              local.get 0
                              i32.load offset=8
                              local.tee 3
                              i32.gt_u
                              drop
                              local.get 8
                              local.get 3
                              i32.store offset=8
                              local.get 3
                              local.get 8
                              i32.store offset=12
                              br 11 (;@2;)
                            end
                            block  ;; label = @13
                              local.get 0
                              i32.const 20
                              i32.add
                              local.tee 5
                              i32.load
                              local.tee 3
                              br_if 0 (;@13;)
                              local.get 0
                              i32.load offset=16
                              local.tee 3
                              i32.eqz
                              br_if 3 (;@10;)
                              local.get 0
                              i32.const 16
                              i32.add
                              local.set 5
                            end
                            loop  ;; label = @13
                              local.get 5
                              local.set 11
                              local.get 3
                              local.tee 8
                              i32.const 20
                              i32.add
                              local.tee 5
                              i32.load
                              local.tee 3
                              br_if 0 (;@13;)
                              local.get 8
                              i32.const 16
                              i32.add
                              local.set 5
                              local.get 8
                              i32.load offset=16
                              local.tee 3
                              br_if 0 (;@13;)
                            end
                            local.get 11
                            i32.const 0
                            i32.store
                            br 10 (;@2;)
                          end
                          i32.const -1
                          local.set 2
                          local.get 0
                          i32.const -65
                          i32.gt_u
                          br_if 0 (;@11;)
                          local.get 0
                          i32.const 19
                          i32.add
                          local.tee 3
                          i32.const -16
                          i32.and
                          local.set 2
                          i32.const 0
                          i32.load offset=1059312
                          local.tee 7
                          i32.eqz
                          br_if 0 (;@11;)
                          i32.const 0
                          local.set 11
                          block  ;; label = @12
                            local.get 2
                            i32.const 256
                            i32.lt_u
                            br_if 0 (;@12;)
                            i32.const 31
                            local.set 11
                            local.get 2
                            i32.const 16777215
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 3
                            i32.const 8
                            i32.shr_u
                            local.tee 3
                            local.get 3
                            i32.const 1048320
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee 3
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
                            local.get 3
                            local.get 4
                            i32.or
                            local.get 5
                            i32.or
                            i32.sub
                            local.tee 3
                            i32.const 1
                            i32.shl
                            local.get 2
                            local.get 3
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
                          local.get 2
                          i32.sub
                          local.set 4
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  local.get 11
                                  i32.const 2
                                  i32.shl
                                  i32.const 1059612
                                  i32.add
                                  i32.load
                                  local.tee 5
                                  br_if 0 (;@15;)
                                  i32.const 0
                                  local.set 3
                                  i32.const 0
                                  local.set 8
                                  br 1 (;@14;)
                                end
                                i32.const 0
                                local.set 3
                                local.get 2
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
                                local.set 0
                                i32.const 0
                                local.set 8
                                loop  ;; label = @15
                                  block  ;; label = @16
                                    local.get 5
                                    i32.load offset=4
                                    i32.const -8
                                    i32.and
                                    local.get 2
                                    i32.sub
                                    local.tee 6
                                    local.get 4
                                    i32.ge_u
                                    br_if 0 (;@16;)
                                    local.get 6
                                    local.set 4
                                    local.get 5
                                    local.set 8
                                    local.get 6
                                    br_if 0 (;@16;)
                                    i32.const 0
                                    local.set 4
                                    local.get 5
                                    local.set 8
                                    local.get 5
                                    local.set 3
                                    br 3 (;@13;)
                                  end
                                  local.get 3
                                  local.get 5
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee 6
                                  local.get 6
                                  local.get 5
                                  local.get 0
                                  i32.const 29
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  i32.add
                                  i32.const 16
                                  i32.add
                                  i32.load
                                  local.tee 5
                                  i32.eq
                                  select
                                  local.get 3
                                  local.get 6
                                  select
                                  local.set 3
                                  local.get 0
                                  i32.const 1
                                  i32.shl
                                  local.set 0
                                  local.get 5
                                  br_if 0 (;@15;)
                                end
                              end
                              block  ;; label = @14
                                local.get 3
                                local.get 8
                                i32.or
                                br_if 0 (;@14;)
                                i32.const 0
                                local.set 8
                                i32.const 2
                                local.get 11
                                i32.shl
                                local.tee 3
                                i32.const 0
                                local.get 3
                                i32.sub
                                i32.or
                                local.get 7
                                i32.and
                                local.tee 3
                                i32.eqz
                                br_if 3 (;@11;)
                                local.get 3
                                i32.const 0
                                local.get 3
                                i32.sub
                                i32.and
                                i32.const -1
                                i32.add
                                local.tee 3
                                local.get 3
                                i32.const 12
                                i32.shr_u
                                i32.const 16
                                i32.and
                                local.tee 3
                                i32.shr_u
                                local.tee 5
                                i32.const 5
                                i32.shr_u
                                i32.const 8
                                i32.and
                                local.tee 0
                                local.get 3
                                i32.or
                                local.get 5
                                local.get 0
                                i32.shr_u
                                local.tee 3
                                i32.const 2
                                i32.shr_u
                                i32.const 4
                                i32.and
                                local.tee 5
                                i32.or
                                local.get 3
                                local.get 5
                                i32.shr_u
                                local.tee 3
                                i32.const 1
                                i32.shr_u
                                i32.const 2
                                i32.and
                                local.tee 5
                                i32.or
                                local.get 3
                                local.get 5
                                i32.shr_u
                                local.tee 3
                                i32.const 1
                                i32.shr_u
                                i32.const 1
                                i32.and
                                local.tee 5
                                i32.or
                                local.get 3
                                local.get 5
                                i32.shr_u
                                i32.add
                                i32.const 2
                                i32.shl
                                i32.const 1059612
                                i32.add
                                i32.load
                                local.set 3
                              end
                              local.get 3
                              i32.eqz
                              br_if 1 (;@12;)
                            end
                            loop  ;; label = @13
                              local.get 3
                              i32.load offset=4
                              i32.const -8
                              i32.and
                              local.get 2
                              i32.sub
                              local.tee 6
                              local.get 4
                              i32.lt_u
                              local.set 0
                              block  ;; label = @14
                                local.get 3
                                i32.load offset=16
                                local.tee 5
                                br_if 0 (;@14;)
                                local.get 3
                                i32.const 20
                                i32.add
                                i32.load
                                local.set 5
                              end
                              local.get 6
                              local.get 4
                              local.get 0
                              select
                              local.set 4
                              local.get 3
                              local.get 8
                              local.get 0
                              select
                              local.set 8
                              local.get 5
                              local.set 3
                              local.get 5
                              br_if 0 (;@13;)
                            end
                          end
                          local.get 8
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 4
                          i32.const 0
                          i32.load offset=1059316
                          local.get 2
                          i32.sub
                          i32.ge_u
                          br_if 0 (;@11;)
                          local.get 8
                          i32.load offset=24
                          local.set 11
                          block  ;; label = @12
                            local.get 8
                            i32.load offset=12
                            local.tee 0
                            local.get 8
                            i32.eq
                            br_if 0 (;@12;)
                            i32.const 0
                            i32.load offset=1059324
                            local.get 8
                            i32.load offset=8
                            local.tee 3
                            i32.gt_u
                            drop
                            local.get 0
                            local.get 3
                            i32.store offset=8
                            local.get 3
                            local.get 0
                            i32.store offset=12
                            br 9 (;@3;)
                          end
                          block  ;; label = @12
                            local.get 8
                            i32.const 20
                            i32.add
                            local.tee 5
                            i32.load
                            local.tee 3
                            br_if 0 (;@12;)
                            local.get 8
                            i32.load offset=16
                            local.tee 3
                            i32.eqz
                            br_if 3 (;@9;)
                            local.get 8
                            i32.const 16
                            i32.add
                            local.set 5
                          end
                          loop  ;; label = @12
                            local.get 5
                            local.set 6
                            local.get 3
                            local.tee 0
                            i32.const 20
                            i32.add
                            local.tee 5
                            i32.load
                            local.tee 3
                            br_if 0 (;@12;)
                            local.get 0
                            i32.const 16
                            i32.add
                            local.set 5
                            local.get 0
                            i32.load offset=16
                            local.tee 3
                            br_if 0 (;@12;)
                          end
                          local.get 6
                          i32.const 0
                          i32.store
                          br 8 (;@3;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1059316
                          local.tee 3
                          local.get 2
                          i32.lt_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.load offset=1059328
                          local.set 4
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 3
                              local.get 2
                              i32.sub
                              local.tee 5
                              i32.const 16
                              i32.lt_u
                              br_if 0 (;@13;)
                              local.get 4
                              local.get 2
                              i32.add
                              local.tee 0
                              local.get 5
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              i32.const 0
                              local.get 5
                              i32.store offset=1059316
                              i32.const 0
                              local.get 0
                              i32.store offset=1059328
                              local.get 4
                              local.get 3
                              i32.add
                              local.get 5
                              i32.store
                              local.get 4
                              local.get 2
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              br 1 (;@12;)
                            end
                            local.get 4
                            local.get 3
                            i32.const 3
                            i32.or
                            i32.store offset=4
                            local.get 4
                            local.get 3
                            i32.add
                            local.tee 3
                            local.get 3
                            i32.load offset=4
                            i32.const 1
                            i32.or
                            i32.store offset=4
                            i32.const 0
                            i32.const 0
                            i32.store offset=1059328
                            i32.const 0
                            i32.const 0
                            i32.store offset=1059316
                          end
                          local.get 4
                          i32.const 8
                          i32.add
                          local.set 3
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1059320
                          local.tee 0
                          local.get 2
                          i32.le_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.load offset=1059332
                          local.tee 3
                          local.get 2
                          i32.add
                          local.tee 4
                          local.get 0
                          local.get 2
                          i32.sub
                          local.tee 5
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          i32.const 0
                          local.get 5
                          i32.store offset=1059320
                          i32.const 0
                          local.get 4
                          i32.store offset=1059332
                          local.get 3
                          local.get 2
                          i32.const 3
                          i32.or
                          i32.store offset=4
                          local.get 3
                          i32.const 8
                          i32.add
                          local.set 3
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1059780
                            i32.eqz
                            br_if 0 (;@12;)
                            i32.const 0
                            i32.load offset=1059788
                            local.set 4
                            br 1 (;@11;)
                          end
                          i32.const 0
                          i64.const -1
                          i64.store offset=1059792 align=4
                          i32.const 0
                          i64.const 281474976776192
                          i64.store offset=1059784 align=4
                          i32.const 0
                          local.get 1
                          i32.const 12
                          i32.add
                          i32.const -16
                          i32.and
                          i32.const 1431655768
                          i32.xor
                          i32.store offset=1059780
                          i32.const 0
                          i32.const 0
                          i32.store offset=1059800
                          i32.const 0
                          i32.const 0
                          i32.store offset=1059752
                          i32.const 65536
                          local.set 4
                        end
                        i32.const 0
                        local.set 3
                        block  ;; label = @11
                          local.get 4
                          local.get 2
                          i32.const 71
                          i32.add
                          local.tee 7
                          i32.add
                          local.tee 6
                          i32.const 0
                          local.get 4
                          i32.sub
                          local.tee 11
                          i32.and
                          local.tee 8
                          local.get 2
                          i32.gt_u
                          br_if 0 (;@11;)
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059804
                          br 10 (;@1;)
                        end
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1059748
                          local.tee 3
                          i32.eqz
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1059740
                            local.tee 4
                            local.get 8
                            i32.add
                            local.tee 5
                            local.get 4
                            i32.le_u
                            br_if 0 (;@12;)
                            local.get 5
                            local.get 3
                            i32.le_u
                            br_if 1 (;@11;)
                          end
                          i32.const 0
                          local.set 3
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059804
                          br 10 (;@1;)
                        end
                        i32.const 0
                        i32.load8_u offset=1059752
                        i32.const 4
                        i32.and
                        br_if 4 (;@6;)
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1059332
                              local.tee 4
                              i32.eqz
                              br_if 0 (;@13;)
                              i32.const 1059756
                              local.set 3
                              loop  ;; label = @14
                                block  ;; label = @15
                                  local.get 3
                                  i32.load
                                  local.tee 5
                                  local.get 4
                                  i32.gt_u
                                  br_if 0 (;@15;)
                                  local.get 5
                                  local.get 3
                                  i32.load offset=4
                                  i32.add
                                  local.get 4
                                  i32.gt_u
                                  br_if 3 (;@12;)
                                end
                                local.get 3
                                i32.load offset=8
                                local.tee 3
                                br_if 0 (;@14;)
                              end
                            end
                            i32.const 0
                            call $sbrk
                            local.tee 0
                            i32.const -1
                            i32.eq
                            br_if 5 (;@7;)
                            local.get 8
                            local.set 6
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1059784
                              local.tee 3
                              i32.const -1
                              i32.add
                              local.tee 4
                              local.get 0
                              i32.and
                              i32.eqz
                              br_if 0 (;@13;)
                              local.get 8
                              local.get 0
                              i32.sub
                              local.get 4
                              local.get 0
                              i32.add
                              i32.const 0
                              local.get 3
                              i32.sub
                              i32.and
                              i32.add
                              local.set 6
                            end
                            local.get 6
                            local.get 2
                            i32.le_u
                            br_if 5 (;@7;)
                            local.get 6
                            i32.const 2147483646
                            i32.gt_u
                            br_if 5 (;@7;)
                            block  ;; label = @13
                              i32.const 0
                              i32.load offset=1059748
                              local.tee 3
                              i32.eqz
                              br_if 0 (;@13;)
                              i32.const 0
                              i32.load offset=1059740
                              local.tee 4
                              local.get 6
                              i32.add
                              local.tee 5
                              local.get 4
                              i32.le_u
                              br_if 6 (;@7;)
                              local.get 5
                              local.get 3
                              i32.gt_u
                              br_if 6 (;@7;)
                            end
                            local.get 6
                            call $sbrk
                            local.tee 3
                            local.get 0
                            i32.ne
                            br_if 1 (;@11;)
                            br 7 (;@5;)
                          end
                          local.get 6
                          local.get 0
                          i32.sub
                          local.get 11
                          i32.and
                          local.tee 6
                          i32.const 2147483646
                          i32.gt_u
                          br_if 4 (;@7;)
                          local.get 6
                          call $sbrk
                          local.tee 0
                          local.get 3
                          i32.load
                          local.get 3
                          i32.load offset=4
                          i32.add
                          i32.eq
                          br_if 3 (;@8;)
                          local.get 0
                          local.set 3
                        end
                        block  ;; label = @11
                          local.get 3
                          i32.const -1
                          i32.eq
                          br_if 0 (;@11;)
                          local.get 2
                          i32.const 72
                          i32.add
                          local.get 6
                          i32.le_u
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            local.get 7
                            local.get 6
                            i32.sub
                            i32.const 0
                            i32.load offset=1059788
                            local.tee 4
                            i32.add
                            i32.const 0
                            local.get 4
                            i32.sub
                            i32.and
                            local.tee 4
                            i32.const 2147483646
                            i32.le_u
                            br_if 0 (;@12;)
                            local.get 3
                            local.set 0
                            br 7 (;@5;)
                          end
                          block  ;; label = @12
                            local.get 4
                            call $sbrk
                            i32.const -1
                            i32.eq
                            br_if 0 (;@12;)
                            local.get 4
                            local.get 6
                            i32.add
                            local.set 6
                            local.get 3
                            local.set 0
                            br 7 (;@5;)
                          end
                          i32.const 0
                          local.get 6
                          i32.sub
                          call $sbrk
                          drop
                          br 4 (;@7;)
                        end
                        local.get 3
                        local.set 0
                        local.get 3
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
                    local.set 0
                    br 5 (;@3;)
                  end
                  local.get 0
                  i32.const -1
                  i32.ne
                  br_if 2 (;@5;)
                end
                i32.const 0
                i32.const 0
                i32.load offset=1059752
                i32.const 4
                i32.or
                i32.store offset=1059752
              end
              local.get 8
              i32.const 2147483646
              i32.gt_u
              br_if 1 (;@4;)
              local.get 8
              call $sbrk
              local.set 0
              i32.const 0
              call $sbrk
              local.set 3
              local.get 0
              i32.const -1
              i32.eq
              br_if 1 (;@4;)
              local.get 3
              i32.const -1
              i32.eq
              br_if 1 (;@4;)
              local.get 0
              local.get 3
              i32.ge_u
              br_if 1 (;@4;)
              local.get 3
              local.get 0
              i32.sub
              local.tee 6
              local.get 2
              i32.const 56
              i32.add
              i32.le_u
              br_if 1 (;@4;)
            end
            i32.const 0
            i32.const 0
            i32.load offset=1059740
            local.get 6
            i32.add
            local.tee 3
            i32.store offset=1059740
            block  ;; label = @5
              local.get 3
              i32.const 0
              i32.load offset=1059744
              i32.le_u
              br_if 0 (;@5;)
              i32.const 0
              local.get 3
              i32.store offset=1059744
            end
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    i32.const 0
                    i32.load offset=1059332
                    local.tee 4
                    i32.eqz
                    br_if 0 (;@8;)
                    i32.const 1059756
                    local.set 3
                    loop  ;; label = @9
                      local.get 0
                      local.get 3
                      i32.load
                      local.tee 5
                      local.get 3
                      i32.load offset=4
                      local.tee 8
                      i32.add
                      i32.eq
                      br_if 2 (;@7;)
                      local.get 3
                      i32.load offset=8
                      local.tee 3
                      br_if 0 (;@9;)
                      br 3 (;@6;)
                    end
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1059324
                      local.tee 3
                      i32.eqz
                      br_if 0 (;@9;)
                      local.get 0
                      local.get 3
                      i32.ge_u
                      br_if 1 (;@8;)
                    end
                    i32.const 0
                    local.get 0
                    i32.store offset=1059324
                  end
                  i32.const 0
                  local.set 3
                  i32.const 0
                  local.get 6
                  i32.store offset=1059760
                  i32.const 0
                  local.get 0
                  i32.store offset=1059756
                  i32.const 0
                  i32.const -1
                  i32.store offset=1059340
                  i32.const 0
                  i32.const 0
                  i32.load offset=1059780
                  i32.store offset=1059344
                  i32.const 0
                  i32.const 0
                  i32.store offset=1059768
                  loop  ;; label = @8
                    local.get 3
                    i32.const 1059368
                    i32.add
                    local.get 3
                    i32.const 1059356
                    i32.add
                    local.tee 4
                    i32.store
                    local.get 4
                    local.get 3
                    i32.const 1059348
                    i32.add
                    local.tee 5
                    i32.store
                    local.get 3
                    i32.const 1059360
                    i32.add
                    local.get 5
                    i32.store
                    local.get 3
                    i32.const 1059376
                    i32.add
                    local.get 3
                    i32.const 1059364
                    i32.add
                    local.tee 5
                    i32.store
                    local.get 5
                    local.get 4
                    i32.store
                    local.get 3
                    i32.const 1059384
                    i32.add
                    local.get 3
                    i32.const 1059372
                    i32.add
                    local.tee 4
                    i32.store
                    local.get 4
                    local.get 5
                    i32.store
                    local.get 3
                    i32.const 1059380
                    i32.add
                    local.get 4
                    i32.store
                    local.get 3
                    i32.const 32
                    i32.add
                    local.tee 3
                    i32.const 256
                    i32.ne
                    br_if 0 (;@8;)
                  end
                  local.get 0
                  i32.const -8
                  local.get 0
                  i32.sub
                  i32.const 15
                  i32.and
                  i32.const 0
                  local.get 0
                  i32.const 8
                  i32.add
                  i32.const 15
                  i32.and
                  select
                  local.tee 3
                  i32.add
                  local.tee 4
                  local.get 6
                  i32.const -56
                  i32.add
                  local.tee 5
                  local.get 3
                  i32.sub
                  local.tee 3
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  i32.const 0
                  i32.const 0
                  i32.load offset=1059796
                  i32.store offset=1059336
                  i32.const 0
                  local.get 3
                  i32.store offset=1059320
                  i32.const 0
                  local.get 4
                  i32.store offset=1059332
                  local.get 0
                  local.get 5
                  i32.add
                  i32.const 56
                  i32.store offset=4
                  br 2 (;@5;)
                end
                local.get 3
                i32.load8_u offset=12
                i32.const 8
                i32.and
                br_if 0 (;@6;)
                local.get 5
                local.get 4
                i32.gt_u
                br_if 0 (;@6;)
                local.get 0
                local.get 4
                i32.le_u
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
                local.tee 5
                i32.add
                local.tee 0
                i32.const 0
                i32.load offset=1059320
                local.get 6
                i32.add
                local.tee 11
                local.get 5
                i32.sub
                local.tee 5
                i32.const 1
                i32.or
                i32.store offset=4
                local.get 3
                local.get 8
                local.get 6
                i32.add
                i32.store offset=4
                i32.const 0
                i32.const 0
                i32.load offset=1059796
                i32.store offset=1059336
                i32.const 0
                local.get 5
                i32.store offset=1059320
                i32.const 0
                local.get 0
                i32.store offset=1059332
                local.get 4
                local.get 11
                i32.add
                i32.const 56
                i32.store offset=4
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 0
                i32.const 0
                i32.load offset=1059324
                local.tee 8
                i32.ge_u
                br_if 0 (;@6;)
                i32.const 0
                local.get 0
                i32.store offset=1059324
                local.get 0
                local.set 8
              end
              local.get 0
              local.get 6
              i32.add
              local.set 5
              i32.const 1059756
              local.set 3
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            loop  ;; label = @13
                              local.get 3
                              i32.load
                              local.get 5
                              i32.eq
                              br_if 1 (;@12;)
                              local.get 3
                              i32.load offset=8
                              local.tee 3
                              br_if 0 (;@13;)
                              br 2 (;@11;)
                            end
                          end
                          local.get 3
                          i32.load8_u offset=12
                          i32.const 8
                          i32.and
                          i32.eqz
                          br_if 1 (;@10;)
                        end
                        i32.const 1059756
                        local.set 3
                        loop  ;; label = @11
                          block  ;; label = @12
                            local.get 3
                            i32.load
                            local.tee 5
                            local.get 4
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 5
                            local.get 3
                            i32.load offset=4
                            i32.add
                            local.tee 5
                            local.get 4
                            i32.gt_u
                            br_if 3 (;@9;)
                          end
                          local.get 3
                          i32.load offset=8
                          local.set 3
                          br 0 (;@11;)
                        end
                      end
                      local.get 3
                      local.get 0
                      i32.store
                      local.get 3
                      local.get 3
                      i32.load offset=4
                      local.get 6
                      i32.add
                      i32.store offset=4
                      local.get 0
                      i32.const -8
                      local.get 0
                      i32.sub
                      i32.const 15
                      i32.and
                      i32.const 0
                      local.get 0
                      i32.const 8
                      i32.add
                      i32.const 15
                      i32.and
                      select
                      i32.add
                      local.tee 11
                      local.get 2
                      i32.const 3
                      i32.or
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
                      local.tee 6
                      local.get 11
                      local.get 2
                      i32.add
                      local.tee 2
                      i32.sub
                      local.set 5
                      block  ;; label = @10
                        local.get 4
                        local.get 6
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.get 2
                        i32.store offset=1059332
                        i32.const 0
                        i32.const 0
                        i32.load offset=1059320
                        local.get 5
                        i32.add
                        local.tee 3
                        i32.store offset=1059320
                        local.get 2
                        local.get 3
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        br 3 (;@7;)
                      end
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1059328
                        local.get 6
                        i32.ne
                        br_if 0 (;@10;)
                        i32.const 0
                        local.get 2
                        i32.store offset=1059328
                        i32.const 0
                        i32.const 0
                        i32.load offset=1059316
                        local.get 5
                        i32.add
                        local.tee 3
                        i32.store offset=1059316
                        local.get 2
                        local.get 3
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get 2
                        local.get 3
                        i32.add
                        local.get 3
                        i32.store
                        br 3 (;@7;)
                      end
                      block  ;; label = @10
                        local.get 6
                        i32.load offset=4
                        local.tee 3
                        i32.const 3
                        i32.and
                        i32.const 1
                        i32.ne
                        br_if 0 (;@10;)
                        local.get 3
                        i32.const -8
                        i32.and
                        local.set 7
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 3
                            i32.const 255
                            i32.gt_u
                            br_if 0 (;@12;)
                            local.get 6
                            i32.load offset=8
                            local.tee 4
                            local.get 3
                            i32.const 3
                            i32.shr_u
                            local.tee 8
                            i32.const 3
                            i32.shl
                            i32.const 1059348
                            i32.add
                            local.tee 0
                            i32.eq
                            drop
                            block  ;; label = @13
                              local.get 6
                              i32.load offset=12
                              local.tee 3
                              local.get 4
                              i32.ne
                              br_if 0 (;@13;)
                              i32.const 0
                              i32.const 0
                              i32.load offset=1059308
                              i32.const -2
                              local.get 8
                              i32.rotl
                              i32.and
                              i32.store offset=1059308
                              br 2 (;@11;)
                            end
                            local.get 3
                            local.get 0
                            i32.eq
                            drop
                            local.get 3
                            local.get 4
                            i32.store offset=8
                            local.get 4
                            local.get 3
                            i32.store offset=12
                            br 1 (;@11;)
                          end
                          local.get 6
                          i32.load offset=24
                          local.set 9
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 6
                              i32.load offset=12
                              local.tee 0
                              local.get 6
                              i32.eq
                              br_if 0 (;@13;)
                              local.get 8
                              local.get 6
                              i32.load offset=8
                              local.tee 3
                              i32.gt_u
                              drop
                              local.get 0
                              local.get 3
                              i32.store offset=8
                              local.get 3
                              local.get 0
                              i32.store offset=12
                              br 1 (;@12;)
                            end
                            block  ;; label = @13
                              local.get 6
                              i32.const 20
                              i32.add
                              local.tee 3
                              i32.load
                              local.tee 4
                              br_if 0 (;@13;)
                              local.get 6
                              i32.const 16
                              i32.add
                              local.tee 3
                              i32.load
                              local.tee 4
                              br_if 0 (;@13;)
                              i32.const 0
                              local.set 0
                              br 1 (;@12;)
                            end
                            loop  ;; label = @13
                              local.get 3
                              local.set 8
                              local.get 4
                              local.tee 0
                              i32.const 20
                              i32.add
                              local.tee 3
                              i32.load
                              local.tee 4
                              br_if 0 (;@13;)
                              local.get 0
                              i32.const 16
                              i32.add
                              local.set 3
                              local.get 0
                              i32.load offset=16
                              local.tee 4
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
                              local.get 6
                              i32.load offset=28
                              local.tee 4
                              i32.const 2
                              i32.shl
                              i32.const 1059612
                              i32.add
                              local.tee 3
                              i32.load
                              local.get 6
                              i32.ne
                              br_if 0 (;@13;)
                              local.get 3
                              local.get 0
                              i32.store
                              local.get 0
                              br_if 1 (;@12;)
                              i32.const 0
                              i32.const 0
                              i32.load offset=1059312
                              i32.const -2
                              local.get 4
                              i32.rotl
                              i32.and
                              i32.store offset=1059312
                              br 2 (;@11;)
                            end
                            local.get 9
                            i32.const 16
                            i32.const 20
                            local.get 9
                            i32.load offset=16
                            local.get 6
                            i32.eq
                            select
                            i32.add
                            local.get 0
                            i32.store
                            local.get 0
                            i32.eqz
                            br_if 1 (;@11;)
                          end
                          local.get 0
                          local.get 9
                          i32.store offset=24
                          block  ;; label = @12
                            local.get 6
                            i32.load offset=16
                            local.tee 3
                            i32.eqz
                            br_if 0 (;@12;)
                            local.get 0
                            local.get 3
                            i32.store offset=16
                            local.get 3
                            local.get 0
                            i32.store offset=24
                          end
                          local.get 6
                          i32.load offset=20
                          local.tee 3
                          i32.eqz
                          br_if 0 (;@11;)
                          local.get 0
                          i32.const 20
                          i32.add
                          local.get 3
                          i32.store
                          local.get 3
                          local.get 0
                          i32.store offset=24
                        end
                        local.get 7
                        local.get 5
                        i32.add
                        local.set 5
                        local.get 6
                        local.get 7
                        i32.add
                        local.set 6
                      end
                      local.get 6
                      local.get 6
                      i32.load offset=4
                      i32.const -2
                      i32.and
                      i32.store offset=4
                      local.get 2
                      local.get 5
                      i32.add
                      local.get 5
                      i32.store
                      local.get 2
                      local.get 5
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      block  ;; label = @10
                        local.get 5
                        i32.const 255
                        i32.gt_u
                        br_if 0 (;@10;)
                        local.get 5
                        i32.const 3
                        i32.shr_u
                        local.tee 4
                        i32.const 3
                        i32.shl
                        i32.const 1059348
                        i32.add
                        local.set 3
                        block  ;; label = @11
                          block  ;; label = @12
                            i32.const 0
                            i32.load offset=1059308
                            local.tee 5
                            i32.const 1
                            local.get 4
                            i32.shl
                            local.tee 4
                            i32.and
                            br_if 0 (;@12;)
                            i32.const 0
                            local.get 5
                            local.get 4
                            i32.or
                            i32.store offset=1059308
                            local.get 3
                            local.set 4
                            br 1 (;@11;)
                          end
                          local.get 3
                          i32.load offset=8
                          local.set 4
                        end
                        local.get 4
                        local.get 2
                        i32.store offset=12
                        local.get 3
                        local.get 2
                        i32.store offset=8
                        local.get 2
                        local.get 3
                        i32.store offset=12
                        local.get 2
                        local.get 4
                        i32.store offset=8
                        br 3 (;@7;)
                      end
                      i32.const 31
                      local.set 3
                      block  ;; label = @10
                        local.get 5
                        i32.const 16777215
                        i32.gt_u
                        br_if 0 (;@10;)
                        local.get 5
                        i32.const 8
                        i32.shr_u
                        local.tee 3
                        local.get 3
                        i32.const 1048320
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 8
                        i32.and
                        local.tee 3
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
                        local.tee 0
                        local.get 0
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        local.tee 0
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        local.get 3
                        local.get 4
                        i32.or
                        local.get 0
                        i32.or
                        i32.sub
                        local.tee 3
                        i32.const 1
                        i32.shl
                        local.get 5
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
                      local.get 2
                      local.get 3
                      i32.store offset=28
                      local.get 2
                      i64.const 0
                      i64.store offset=16 align=4
                      local.get 3
                      i32.const 2
                      i32.shl
                      i32.const 1059612
                      i32.add
                      local.set 4
                      block  ;; label = @10
                        i32.const 0
                        i32.load offset=1059312
                        local.tee 0
                        i32.const 1
                        local.get 3
                        i32.shl
                        local.tee 8
                        i32.and
                        br_if 0 (;@10;)
                        local.get 4
                        local.get 2
                        i32.store
                        i32.const 0
                        local.get 0
                        local.get 8
                        i32.or
                        i32.store offset=1059312
                        local.get 2
                        local.get 4
                        i32.store offset=24
                        local.get 2
                        local.get 2
                        i32.store offset=8
                        local.get 2
                        local.get 2
                        i32.store offset=12
                        br 3 (;@7;)
                      end
                      local.get 5
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
                      local.get 4
                      i32.load
                      local.set 0
                      loop  ;; label = @10
                        local.get 0
                        local.tee 4
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get 5
                        i32.eq
                        br_if 2 (;@8;)
                        local.get 3
                        i32.const 29
                        i32.shr_u
                        local.set 0
                        local.get 3
                        i32.const 1
                        i32.shl
                        local.set 3
                        local.get 4
                        local.get 0
                        i32.const 4
                        i32.and
                        i32.add
                        i32.const 16
                        i32.add
                        local.tee 8
                        i32.load
                        local.tee 0
                        br_if 0 (;@10;)
                      end
                      local.get 8
                      local.get 2
                      i32.store
                      local.get 2
                      local.get 4
                      i32.store offset=24
                      local.get 2
                      local.get 2
                      i32.store offset=12
                      local.get 2
                      local.get 2
                      i32.store offset=8
                      br 2 (;@7;)
                    end
                    local.get 0
                    i32.const -8
                    local.get 0
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get 0
                    i32.const 8
                    i32.add
                    i32.const 15
                    i32.and
                    select
                    local.tee 3
                    i32.add
                    local.tee 11
                    local.get 6
                    i32.const -56
                    i32.add
                    local.tee 8
                    local.get 3
                    i32.sub
                    local.tee 3
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get 0
                    local.get 8
                    i32.add
                    i32.const 56
                    i32.store offset=4
                    local.get 4
                    local.get 5
                    i32.const 55
                    local.get 5
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get 5
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
                    i32.load offset=1059796
                    i32.store offset=1059336
                    i32.const 0
                    local.get 3
                    i32.store offset=1059320
                    i32.const 0
                    local.get 11
                    i32.store offset=1059332
                    local.get 8
                    i32.const 16
                    i32.add
                    i32.const 0
                    i64.load offset=1059764 align=4
                    i64.store align=4
                    local.get 8
                    i32.const 0
                    i64.load offset=1059756 align=4
                    i64.store offset=8 align=4
                    i32.const 0
                    local.get 8
                    i32.const 8
                    i32.add
                    i32.store offset=1059764
                    i32.const 0
                    local.get 6
                    i32.store offset=1059760
                    i32.const 0
                    local.get 0
                    i32.store offset=1059756
                    i32.const 0
                    i32.const 0
                    i32.store offset=1059768
                    local.get 8
                    i32.const 36
                    i32.add
                    local.set 3
                    loop  ;; label = @9
                      local.get 3
                      i32.const 7
                      i32.store
                      local.get 5
                      local.get 3
                      i32.const 4
                      i32.add
                      local.tee 3
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
                    local.tee 6
                    i32.store
                    local.get 4
                    local.get 6
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    block  ;; label = @9
                      local.get 6
                      i32.const 255
                      i32.gt_u
                      br_if 0 (;@9;)
                      local.get 6
                      i32.const 3
                      i32.shr_u
                      local.tee 5
                      i32.const 3
                      i32.shl
                      i32.const 1059348
                      i32.add
                      local.set 3
                      block  ;; label = @10
                        block  ;; label = @11
                          i32.const 0
                          i32.load offset=1059308
                          local.tee 0
                          i32.const 1
                          local.get 5
                          i32.shl
                          local.tee 5
                          i32.and
                          br_if 0 (;@11;)
                          i32.const 0
                          local.get 0
                          local.get 5
                          i32.or
                          i32.store offset=1059308
                          local.get 3
                          local.set 5
                          br 1 (;@10;)
                        end
                        local.get 3
                        i32.load offset=8
                        local.set 5
                      end
                      local.get 5
                      local.get 4
                      i32.store offset=12
                      local.get 3
                      local.get 4
                      i32.store offset=8
                      local.get 4
                      local.get 3
                      i32.store offset=12
                      local.get 4
                      local.get 5
                      i32.store offset=8
                      br 4 (;@5;)
                    end
                    i32.const 31
                    local.set 3
                    block  ;; label = @9
                      local.get 6
                      i32.const 16777215
                      i32.gt_u
                      br_if 0 (;@9;)
                      local.get 6
                      i32.const 8
                      i32.shr_u
                      local.tee 3
                      local.get 3
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
                      local.tee 0
                      local.get 0
                      i32.const 245760
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 2
                      i32.and
                      local.tee 0
                      i32.shl
                      i32.const 15
                      i32.shr_u
                      local.get 3
                      local.get 5
                      i32.or
                      local.get 0
                      i32.or
                      i32.sub
                      local.tee 3
                      i32.const 1
                      i32.shl
                      local.get 6
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
                    local.get 4
                    i64.const 0
                    i64.store offset=16 align=4
                    local.get 4
                    i32.const 28
                    i32.add
                    local.get 3
                    i32.store
                    local.get 3
                    i32.const 2
                    i32.shl
                    i32.const 1059612
                    i32.add
                    local.set 5
                    block  ;; label = @9
                      i32.const 0
                      i32.load offset=1059312
                      local.tee 0
                      i32.const 1
                      local.get 3
                      i32.shl
                      local.tee 8
                      i32.and
                      br_if 0 (;@9;)
                      local.get 5
                      local.get 4
                      i32.store
                      i32.const 0
                      local.get 0
                      local.get 8
                      i32.or
                      i32.store offset=1059312
                      local.get 4
                      i32.const 24
                      i32.add
                      local.get 5
                      i32.store
                      local.get 4
                      local.get 4
                      i32.store offset=8
                      local.get 4
                      local.get 4
                      i32.store offset=12
                      br 4 (;@5;)
                    end
                    local.get 6
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
                    local.set 0
                    loop  ;; label = @9
                      local.get 0
                      local.tee 5
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      local.get 6
                      i32.eq
                      br_if 3 (;@6;)
                      local.get 3
                      i32.const 29
                      i32.shr_u
                      local.set 0
                      local.get 3
                      i32.const 1
                      i32.shl
                      local.set 3
                      local.get 5
                      local.get 0
                      i32.const 4
                      i32.and
                      i32.add
                      i32.const 16
                      i32.add
                      local.tee 8
                      i32.load
                      local.tee 0
                      br_if 0 (;@9;)
                    end
                    local.get 8
                    local.get 4
                    i32.store
                    local.get 4
                    i32.const 24
                    i32.add
                    local.get 5
                    i32.store
                    local.get 4
                    local.get 4
                    i32.store offset=12
                    local.get 4
                    local.get 4
                    i32.store offset=8
                    br 3 (;@5;)
                  end
                  local.get 4
                  i32.load offset=8
                  local.tee 3
                  local.get 2
                  i32.store offset=12
                  local.get 4
                  local.get 2
                  i32.store offset=8
                  local.get 2
                  i32.const 0
                  i32.store offset=24
                  local.get 2
                  local.get 4
                  i32.store offset=12
                  local.get 2
                  local.get 3
                  i32.store offset=8
                end
                local.get 11
                i32.const 8
                i32.add
                local.set 3
                br 5 (;@1;)
              end
              local.get 5
              i32.load offset=8
              local.tee 3
              local.get 4
              i32.store offset=12
              local.get 5
              local.get 4
              i32.store offset=8
              local.get 4
              i32.const 24
              i32.add
              i32.const 0
              i32.store
              local.get 4
              local.get 5
              i32.store offset=12
              local.get 4
              local.get 3
              i32.store offset=8
            end
            i32.const 0
            i32.load offset=1059320
            local.tee 3
            local.get 2
            i32.le_u
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1059332
            local.tee 4
            local.get 2
            i32.add
            local.tee 5
            local.get 3
            local.get 2
            i32.sub
            local.tee 3
            i32.const 1
            i32.or
            i32.store offset=4
            i32.const 0
            local.get 3
            i32.store offset=1059320
            i32.const 0
            local.get 5
            i32.store offset=1059332
            local.get 4
            local.get 2
            i32.const 3
            i32.or
            i32.store offset=4
            local.get 4
            i32.const 8
            i32.add
            local.set 3
            br 3 (;@1;)
          end
          i32.const 0
          local.set 3
          i32.const 0
          i32.const 48
          i32.store offset=1059804
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
              local.tee 5
              i32.const 2
              i32.shl
              i32.const 1059612
              i32.add
              local.tee 3
              i32.load
              i32.ne
              br_if 0 (;@5;)
              local.get 3
              local.get 0
              i32.store
              local.get 0
              br_if 1 (;@4;)
              i32.const 0
              local.get 7
              i32.const -2
              local.get 5
              i32.rotl
              i32.and
              local.tee 7
              i32.store offset=1059312
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
            local.get 0
            i32.store
            local.get 0
            i32.eqz
            br_if 1 (;@3;)
          end
          local.get 0
          local.get 11
          i32.store offset=24
          block  ;; label = @4
            local.get 8
            i32.load offset=16
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            local.get 3
            i32.store offset=16
            local.get 3
            local.get 0
            i32.store offset=24
          end
          local.get 8
          i32.const 20
          i32.add
          i32.load
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.const 20
          i32.add
          local.get 3
          i32.store
          local.get 3
          local.get 0
          i32.store offset=24
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 4
            i32.const 15
            i32.gt_u
            br_if 0 (;@4;)
            local.get 8
            local.get 4
            local.get 2
            i32.add
            local.tee 3
            i32.const 3
            i32.or
            i32.store offset=4
            local.get 8
            local.get 3
            i32.add
            local.tee 3
            local.get 3
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            br 1 (;@3;)
          end
          local.get 8
          local.get 2
          i32.add
          local.tee 0
          local.get 4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get 8
          local.get 2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get 0
          local.get 4
          i32.add
          local.get 4
          i32.store
          block  ;; label = @4
            local.get 4
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 4
            i32.const 3
            i32.shr_u
            local.tee 4
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.set 3
            block  ;; label = @5
              block  ;; label = @6
                i32.const 0
                i32.load offset=1059308
                local.tee 5
                i32.const 1
                local.get 4
                i32.shl
                local.tee 4
                i32.and
                br_if 0 (;@6;)
                i32.const 0
                local.get 5
                local.get 4
                i32.or
                i32.store offset=1059308
                local.get 3
                local.set 4
                br 1 (;@5;)
              end
              local.get 3
              i32.load offset=8
              local.set 4
            end
            local.get 4
            local.get 0
            i32.store offset=12
            local.get 3
            local.get 0
            i32.store offset=8
            local.get 0
            local.get 3
            i32.store offset=12
            local.get 0
            local.get 4
            i32.store offset=8
            br 1 (;@3;)
          end
          i32.const 31
          local.set 3
          block  ;; label = @4
            local.get 4
            i32.const 16777215
            i32.gt_u
            br_if 0 (;@4;)
            local.get 4
            i32.const 8
            i32.shr_u
            local.tee 3
            local.get 3
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
            local.tee 2
            local.get 2
            i32.const 245760
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 2
            i32.and
            local.tee 2
            i32.shl
            i32.const 15
            i32.shr_u
            local.get 3
            local.get 5
            i32.or
            local.get 2
            i32.or
            i32.sub
            local.tee 3
            i32.const 1
            i32.shl
            local.get 4
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
          local.get 3
          i32.store offset=28
          local.get 0
          i64.const 0
          i64.store offset=16 align=4
          local.get 3
          i32.const 2
          i32.shl
          i32.const 1059612
          i32.add
          local.set 5
          block  ;; label = @4
            local.get 7
            i32.const 1
            local.get 3
            i32.shl
            local.tee 2
            i32.and
            br_if 0 (;@4;)
            local.get 5
            local.get 0
            i32.store
            i32.const 0
            local.get 7
            local.get 2
            i32.or
            i32.store offset=1059312
            local.get 0
            local.get 5
            i32.store offset=24
            local.get 0
            local.get 0
            i32.store offset=8
            local.get 0
            local.get 0
            i32.store offset=12
            br 1 (;@3;)
          end
          local.get 4
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
          local.set 2
          block  ;; label = @4
            loop  ;; label = @5
              local.get 2
              local.tee 5
              i32.load offset=4
              i32.const -8
              i32.and
              local.get 4
              i32.eq
              br_if 1 (;@4;)
              local.get 3
              i32.const 29
              i32.shr_u
              local.set 2
              local.get 3
              i32.const 1
              i32.shl
              local.set 3
              local.get 5
              local.get 2
              i32.const 4
              i32.and
              i32.add
              i32.const 16
              i32.add
              local.tee 6
              i32.load
              local.tee 2
              br_if 0 (;@5;)
            end
            local.get 6
            local.get 0
            i32.store
            local.get 0
            local.get 5
            i32.store offset=24
            local.get 0
            local.get 0
            i32.store offset=12
            local.get 0
            local.get 0
            i32.store offset=8
            br 1 (;@3;)
          end
          local.get 5
          i32.load offset=8
          local.tee 3
          local.get 0
          i32.store offset=12
          local.get 5
          local.get 0
          i32.store offset=8
          local.get 0
          i32.const 0
          i32.store offset=24
          local.get 0
          local.get 5
          i32.store offset=12
          local.get 0
          local.get 3
          i32.store offset=8
        end
        local.get 8
        i32.const 8
        i32.add
        local.set 3
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 10
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            local.get 0
            i32.load offset=28
            local.tee 5
            i32.const 2
            i32.shl
            i32.const 1059612
            i32.add
            local.tee 3
            i32.load
            i32.ne
            br_if 0 (;@4;)
            local.get 3
            local.get 8
            i32.store
            local.get 8
            br_if 1 (;@3;)
            i32.const 0
            local.get 9
            i32.const -2
            local.get 5
            i32.rotl
            i32.and
            i32.store offset=1059312
            br 2 (;@2;)
          end
          local.get 10
          i32.const 16
          i32.const 20
          local.get 10
          i32.load offset=16
          local.get 0
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
          local.get 0
          i32.load offset=16
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 8
          local.get 3
          i32.store offset=16
          local.get 3
          local.get 8
          i32.store offset=24
        end
        local.get 0
        i32.const 20
        i32.add
        i32.load
        local.tee 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 8
        i32.const 20
        i32.add
        local.get 3
        i32.store
        local.get 3
        local.get 8
        i32.store offset=24
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.const 15
          i32.gt_u
          br_if 0 (;@3;)
          local.get 0
          local.get 4
          local.get 2
          i32.add
          local.tee 3
          i32.const 3
          i32.or
          i32.store offset=4
          local.get 0
          local.get 3
          i32.add
          local.tee 3
          local.get 3
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          br 1 (;@2;)
        end
        local.get 0
        local.get 2
        i32.add
        local.tee 5
        local.get 4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get 0
        local.get 2
        i32.const 3
        i32.or
        i32.store offset=4
        local.get 5
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
          i32.const 1059348
          i32.add
          local.set 2
          i32.const 0
          i32.load offset=1059328
          local.set 3
          block  ;; label = @4
            block  ;; label = @5
              i32.const 1
              local.get 8
              i32.shl
              local.tee 8
              local.get 6
              i32.and
              br_if 0 (;@5;)
              i32.const 0
              local.get 8
              local.get 6
              i32.or
              i32.store offset=1059308
              local.get 2
              local.set 8
              br 1 (;@4;)
            end
            local.get 2
            i32.load offset=8
            local.set 8
          end
          local.get 8
          local.get 3
          i32.store offset=12
          local.get 2
          local.get 3
          i32.store offset=8
          local.get 3
          local.get 2
          i32.store offset=12
          local.get 3
          local.get 8
          i32.store offset=8
        end
        i32.const 0
        local.get 5
        i32.store offset=1059328
        i32.const 0
        local.get 4
        i32.store offset=1059316
      end
      local.get 0
      i32.const 8
      i32.add
      local.set 3
    end
    local.get 1
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 3)
  (func $free (type 0) (param i32)
    local.get 0
    call $dlfree)
  (func $dlfree (type 0) (param i32)
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
        i32.load offset=1059324
        local.tee 4
        i32.lt_u
        br_if 1 (;@1;)
        local.get 2
        local.get 0
        i32.add
        local.set 0
        block  ;; label = @3
          i32.const 0
          i32.load offset=1059328
          local.get 1
          i32.eq
          br_if 0 (;@3;)
          block  ;; label = @4
            local.get 2
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 1
            i32.load offset=8
            local.tee 4
            local.get 2
            i32.const 3
            i32.shr_u
            local.tee 5
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.tee 6
            i32.eq
            drop
            block  ;; label = @5
              local.get 1
              i32.load offset=12
              local.tee 2
              local.get 4
              i32.ne
              br_if 0 (;@5;)
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get 5
              i32.rotl
              i32.and
              i32.store offset=1059308
              br 3 (;@2;)
            end
            local.get 2
            local.get 6
            i32.eq
            drop
            local.get 2
            local.get 4
            i32.store offset=8
            local.get 4
            local.get 2
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
              local.tee 6
              local.get 1
              i32.eq
              br_if 0 (;@5;)
              local.get 4
              local.get 1
              i32.load offset=8
              local.tee 2
              i32.gt_u
              drop
              local.get 6
              local.get 2
              i32.store offset=8
              local.get 2
              local.get 6
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
              local.set 6
              br 1 (;@4;)
            end
            loop  ;; label = @5
              local.get 2
              local.set 5
              local.get 4
              local.tee 6
              i32.const 20
              i32.add
              local.tee 2
              i32.load
              local.tee 4
              br_if 0 (;@5;)
              local.get 6
              i32.const 16
              i32.add
              local.set 2
              local.get 6
              i32.load offset=16
              local.tee 4
              br_if 0 (;@5;)
            end
            local.get 5
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
              i32.const 1059612
              i32.add
              local.tee 2
              i32.load
              local.get 1
              i32.ne
              br_if 0 (;@5;)
              local.get 2
              local.get 6
              i32.store
              local.get 6
              br_if 1 (;@4;)
              i32.const 0
              i32.const 0
              i32.load offset=1059312
              i32.const -2
              local.get 4
              i32.rotl
              i32.and
              i32.store offset=1059312
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
            local.get 1
            i32.load offset=16
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            local.get 2
            i32.store offset=16
            local.get 2
            local.get 6
            i32.store offset=24
          end
          local.get 1
          i32.load offset=20
          local.tee 2
          i32.eqz
          br_if 1 (;@2;)
          local.get 6
          i32.const 20
          i32.add
          local.get 2
          i32.store
          local.get 2
          local.get 6
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
        i32.store offset=1059316
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
            i32.load offset=1059332
            local.get 3
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 1
            i32.store offset=1059332
            i32.const 0
            i32.const 0
            i32.load offset=1059320
            local.get 0
            i32.add
            local.tee 0
            i32.store offset=1059320
            local.get 1
            local.get 0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 1
            i32.const 0
            i32.load offset=1059328
            i32.ne
            br_if 3 (;@1;)
            i32.const 0
            i32.const 0
            i32.store offset=1059316
            i32.const 0
            i32.const 0
            i32.store offset=1059328
            return
          end
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059328
            local.get 3
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 1
            i32.store offset=1059328
            i32.const 0
            i32.const 0
            i32.load offset=1059316
            local.get 0
            i32.add
            local.tee 0
            i32.store offset=1059316
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
              i32.load offset=8
              local.tee 4
              local.get 2
              i32.const 3
              i32.shr_u
              local.tee 5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee 6
              i32.eq
              drop
              block  ;; label = @6
                local.get 3
                i32.load offset=12
                local.tee 2
                local.get 4
                i32.ne
                br_if 0 (;@6;)
                i32.const 0
                i32.const 0
                i32.load offset=1059308
                i32.const -2
                local.get 5
                i32.rotl
                i32.and
                i32.store offset=1059308
                br 2 (;@4;)
              end
              local.get 2
              local.get 6
              i32.eq
              drop
              local.get 2
              local.get 4
              i32.store offset=8
              local.get 4
              local.get 2
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
                local.tee 6
                local.get 3
                i32.eq
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1059324
                local.get 3
                i32.load offset=8
                local.tee 2
                i32.gt_u
                drop
                local.get 6
                local.get 2
                i32.store offset=8
                local.get 2
                local.get 6
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
                local.set 6
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 2
                local.set 5
                local.get 4
                local.tee 6
                i32.const 20
                i32.add
                local.tee 2
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                local.get 6
                i32.const 16
                i32.add
                local.set 2
                local.get 6
                i32.load offset=16
                local.tee 4
                br_if 0 (;@6;)
              end
              local.get 5
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
                i32.const 1059612
                i32.add
                local.tee 2
                i32.load
                local.get 3
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.get 6
                i32.store
                local.get 6
                br_if 1 (;@5;)
                i32.const 0
                i32.const 0
                i32.load offset=1059312
                i32.const -2
                local.get 4
                i32.rotl
                i32.and
                i32.store offset=1059312
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
              local.get 3
              i32.load offset=16
              local.tee 2
              i32.eqz
              br_if 0 (;@5;)
              local.get 6
              local.get 2
              i32.store offset=16
              local.get 2
              local.get 6
              i32.store offset=24
            end
            local.get 3
            i32.load offset=20
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 6
            i32.const 20
            i32.add
            local.get 2
            i32.store
            local.get 2
            local.get 6
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
          i32.load offset=1059328
          i32.ne
          br_if 1 (;@2;)
          i32.const 0
          local.get 0
          i32.store offset=1059316
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
        i32.const 1059348
        i32.add
        local.set 0
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059308
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
            i32.store offset=1059308
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
      i32.const 31
      local.set 2
      block  ;; label = @2
        local.get 0
        i32.const 16777215
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        i32.const 8
        i32.shr_u
        local.tee 2
        local.get 2
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
        local.get 2
        local.get 4
        i32.or
        local.get 6
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
      i32.const 1059612
      i32.add
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          i32.load offset=1059312
          local.tee 6
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
          local.get 6
          local.get 3
          i32.or
          i32.store offset=1059312
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
        local.set 6
        block  ;; label = @3
          loop  ;; label = @4
            local.get 6
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
            local.set 6
            local.get 2
            i32.const 1
            i32.shl
            local.set 2
            local.get 4
            local.get 6
            i32.const 4
            i32.and
            i32.add
            i32.const 16
            i32.add
            local.tee 3
            i32.load
            local.tee 6
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
        local.tee 0
        local.get 1
        i32.store offset=12
        local.get 4
        local.get 1
        i32.store offset=8
        local.get 1
        i32.const 24
        i32.add
        i32.const 0
        i32.store
        local.get 1
        local.get 4
        i32.store offset=12
        local.get 1
        local.get 0
        i32.store offset=8
      end
      i32.const 0
      i32.const 0
      i32.load offset=1059340
      i32.const -1
      i32.add
      local.tee 1
      i32.const -1
      local.get 1
      select
      i32.store offset=1059340
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
      i32.store offset=1059804
      i32.const 0
      return
    end
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
    local.set 2
    local.get 0
    i32.const -4
    i32.add
    local.tee 3
    i32.load
    local.tee 4
    i32.const -8
    i32.and
    local.set 5
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.const 3
          i32.and
          br_if 0 (;@3;)
          local.get 2
          i32.const 256
          i32.lt_u
          br_if 1 (;@2;)
          local.get 5
          local.get 2
          i32.const 4
          i32.or
          i32.lt_u
          br_if 1 (;@2;)
          local.get 5
          local.get 2
          i32.sub
          i32.const 0
          i32.load offset=1059788
          i32.const 1
          i32.shl
          i32.le_u
          br_if 2 (;@1;)
          br 1 (;@2;)
        end
        local.get 0
        i32.const -8
        i32.add
        local.tee 6
        local.get 5
        i32.add
        local.set 7
        block  ;; label = @3
          local.get 5
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
          local.get 5
          local.get 2
          i32.sub
          local.tee 1
          i32.const 16
          i32.lt_u
          br_if 2 (;@1;)
          local.get 3
          local.get 2
          local.get 4
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get 6
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
          i32.load offset=1059332
          local.get 7
          i32.ne
          br_if 0 (;@3;)
          i32.const 0
          i32.load offset=1059320
          local.get 5
          i32.add
          local.tee 5
          local.get 2
          i32.le_u
          br_if 1 (;@2;)
          local.get 3
          local.get 2
          local.get 4
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          i32.const 0
          local.get 6
          local.get 2
          i32.add
          local.tee 1
          i32.store offset=1059332
          i32.const 0
          local.get 5
          local.get 2
          i32.sub
          local.tee 2
          i32.store offset=1059320
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
          i32.load offset=1059328
          local.get 7
          i32.ne
          br_if 0 (;@3;)
          i32.const 0
          i32.load offset=1059316
          local.get 5
          i32.add
          local.tee 5
          local.get 2
          i32.lt_u
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 5
              local.get 2
              i32.sub
              local.tee 1
              i32.const 16
              i32.lt_u
              br_if 0 (;@5;)
              local.get 3
              local.get 2
              local.get 4
              i32.const 1
              i32.and
              i32.or
              i32.const 2
              i32.or
              i32.store
              local.get 6
              local.get 2
              i32.add
              local.tee 2
              local.get 1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get 6
              local.get 5
              i32.add
              local.tee 5
              local.get 1
              i32.store
              local.get 5
              local.get 5
              i32.load offset=4
              i32.const -2
              i32.and
              i32.store offset=4
              br 1 (;@4;)
            end
            local.get 3
            local.get 4
            i32.const 1
            i32.and
            local.get 5
            i32.or
            i32.const 2
            i32.or
            i32.store
            local.get 6
            local.get 5
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
          i32.store offset=1059328
          i32.const 0
          local.get 1
          i32.store offset=1059316
          local.get 0
          return
        end
        local.get 7
        i32.load offset=4
        local.tee 8
        i32.const 2
        i32.and
        br_if 0 (;@2;)
        local.get 8
        i32.const -8
        i32.and
        local.get 5
        i32.add
        local.tee 9
        local.get 2
        i32.lt_u
        br_if 0 (;@2;)
        local.get 9
        local.get 2
        i32.sub
        local.set 10
        block  ;; label = @3
          block  ;; label = @4
            local.get 8
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 7
            i32.load offset=8
            local.tee 1
            local.get 8
            i32.const 3
            i32.shr_u
            local.tee 11
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.tee 8
            i32.eq
            drop
            block  ;; label = @5
              local.get 7
              i32.load offset=12
              local.tee 5
              local.get 1
              i32.ne
              br_if 0 (;@5;)
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get 11
              i32.rotl
              i32.and
              i32.store offset=1059308
              br 2 (;@3;)
            end
            local.get 5
            local.get 8
            i32.eq
            drop
            local.get 5
            local.get 1
            i32.store offset=8
            local.get 1
            local.get 5
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
              local.tee 8
              local.get 7
              i32.eq
              br_if 0 (;@5;)
              i32.const 0
              i32.load offset=1059324
              local.get 7
              i32.load offset=8
              local.tee 1
              i32.gt_u
              drop
              local.get 8
              local.get 1
              i32.store offset=8
              local.get 1
              local.get 8
              i32.store offset=12
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 7
              i32.const 20
              i32.add
              local.tee 1
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              local.get 7
              i32.const 16
              i32.add
              local.tee 1
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              i32.const 0
              local.set 8
              br 1 (;@4;)
            end
            loop  ;; label = @5
              local.get 1
              local.set 11
              local.get 5
              local.tee 8
              i32.const 20
              i32.add
              local.tee 1
              i32.load
              local.tee 5
              br_if 0 (;@5;)
              local.get 8
              i32.const 16
              i32.add
              local.set 1
              local.get 8
              i32.load offset=16
              local.tee 5
              br_if 0 (;@5;)
            end
            local.get 11
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
              local.tee 5
              i32.const 2
              i32.shl
              i32.const 1059612
              i32.add
              local.tee 1
              i32.load
              local.get 7
              i32.ne
              br_if 0 (;@5;)
              local.get 1
              local.get 8
              i32.store
              local.get 8
              br_if 1 (;@4;)
              i32.const 0
              i32.const 0
              i32.load offset=1059312
              i32.const -2
              local.get 5
              i32.rotl
              i32.and
              i32.store offset=1059312
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
            local.get 8
            i32.store
            local.get 8
            i32.eqz
            br_if 1 (;@3;)
          end
          local.get 8
          local.get 12
          i32.store offset=24
          block  ;; label = @4
            local.get 7
            i32.load offset=16
            local.tee 1
            i32.eqz
            br_if 0 (;@4;)
            local.get 8
            local.get 1
            i32.store offset=16
            local.get 1
            local.get 8
            i32.store offset=24
          end
          local.get 7
          i32.load offset=20
          local.tee 1
          i32.eqz
          br_if 0 (;@3;)
          local.get 8
          i32.const 20
          i32.add
          local.get 1
          i32.store
          local.get 1
          local.get 8
          i32.store offset=24
        end
        block  ;; label = @3
          local.get 10
          i32.const 15
          i32.gt_u
          br_if 0 (;@3;)
          local.get 3
          local.get 4
          i32.const 1
          i32.and
          local.get 9
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get 6
          local.get 9
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
        local.get 3
        local.get 2
        local.get 4
        i32.const 1
        i32.and
        i32.or
        i32.const 2
        i32.or
        i32.store
        local.get 6
        local.get 2
        i32.add
        local.tee 1
        local.get 10
        i32.const 3
        i32.or
        i32.store offset=4
        local.get 6
        local.get 9
        i32.add
        local.tee 2
        local.get 2
        i32.load offset=4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get 1
        local.get 10
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
      i32.const -4
      i32.const -8
      local.get 3
      i32.load
      local.tee 5
      i32.const 3
      i32.and
      select
      local.get 5
      i32.const -8
      i32.and
      i32.add
      local.tee 5
      local.get 1
      local.get 5
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
  (func $dispose_chunk (type 2) (param i32 i32)
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
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059328
            local.get 0
            local.get 3
            i32.sub
            local.tee 0
            i32.eq
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.const 255
              i32.gt_u
              br_if 0 (;@5;)
              local.get 0
              i32.load offset=8
              local.tee 4
              local.get 3
              i32.const 3
              i32.shr_u
              local.tee 5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee 6
              i32.eq
              drop
              local.get 0
              i32.load offset=12
              local.tee 3
              local.get 4
              i32.ne
              br_if 2 (;@3;)
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get 5
              i32.rotl
              i32.and
              i32.store offset=1059308
              br 3 (;@2;)
            end
            local.get 0
            i32.load offset=24
            local.set 7
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.load offset=12
                local.tee 6
                local.get 0
                i32.eq
                br_if 0 (;@6;)
                i32.const 0
                i32.load offset=1059324
                local.get 0
                i32.load offset=8
                local.tee 3
                i32.gt_u
                drop
                local.get 6
                local.get 3
                i32.store offset=8
                local.get 3
                local.get 6
                i32.store offset=12
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 0
                i32.const 20
                i32.add
                local.tee 3
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                local.get 0
                i32.const 16
                i32.add
                local.tee 3
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                i32.const 0
                local.set 6
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 3
                local.set 5
                local.get 4
                local.tee 6
                i32.const 20
                i32.add
                local.tee 3
                i32.load
                local.tee 4
                br_if 0 (;@6;)
                local.get 6
                i32.const 16
                i32.add
                local.set 3
                local.get 6
                i32.load offset=16
                local.tee 4
                br_if 0 (;@6;)
              end
              local.get 5
              i32.const 0
              i32.store
            end
            local.get 7
            i32.eqz
            br_if 2 (;@2;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.load offset=28
                local.tee 4
                i32.const 2
                i32.shl
                i32.const 1059612
                i32.add
                local.tee 3
                i32.load
                local.get 0
                i32.ne
                br_if 0 (;@6;)
                local.get 3
                local.get 6
                i32.store
                local.get 6
                br_if 1 (;@5;)
                i32.const 0
                i32.const 0
                i32.load offset=1059312
                i32.const -2
                local.get 4
                i32.rotl
                i32.and
                i32.store offset=1059312
                br 4 (;@2;)
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
              br_if 3 (;@2;)
            end
            local.get 6
            local.get 7
            i32.store offset=24
            block  ;; label = @5
              local.get 0
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
            local.get 0
            i32.load offset=20
            local.tee 3
            i32.eqz
            br_if 2 (;@2;)
            local.get 6
            i32.const 20
            i32.add
            local.get 3
            i32.store
            local.get 3
            local.get 6
            i32.store offset=24
            br 2 (;@2;)
          end
          local.get 2
          i32.load offset=4
          local.tee 3
          i32.const 3
          i32.and
          i32.const 3
          i32.ne
          br_if 1 (;@2;)
          local.get 2
          local.get 3
          i32.const -2
          i32.and
          i32.store offset=4
          i32.const 0
          local.get 1
          i32.store offset=1059316
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
        local.get 3
        local.get 6
        i32.eq
        drop
        local.get 3
        local.get 4
        i32.store offset=8
        local.get 4
        local.get 3
        i32.store offset=12
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
            i32.load offset=1059332
            local.get 2
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 0
            i32.store offset=1059332
            i32.const 0
            i32.const 0
            i32.load offset=1059320
            local.get 1
            i32.add
            local.tee 1
            i32.store offset=1059320
            local.get 0
            local.get 1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get 0
            i32.const 0
            i32.load offset=1059328
            i32.ne
            br_if 3 (;@1;)
            i32.const 0
            i32.const 0
            i32.store offset=1059316
            i32.const 0
            i32.const 0
            i32.store offset=1059328
            return
          end
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059328
            local.get 2
            i32.ne
            br_if 0 (;@4;)
            i32.const 0
            local.get 0
            i32.store offset=1059328
            i32.const 0
            i32.const 0
            i32.load offset=1059316
            local.get 1
            i32.add
            local.tee 1
            i32.store offset=1059316
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
              i32.load offset=8
              local.tee 4
              local.get 3
              i32.const 3
              i32.shr_u
              local.tee 5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee 6
              i32.eq
              drop
              block  ;; label = @6
                local.get 2
                i32.load offset=12
                local.tee 3
                local.get 4
                i32.ne
                br_if 0 (;@6;)
                i32.const 0
                i32.const 0
                i32.load offset=1059308
                i32.const -2
                local.get 5
                i32.rotl
                i32.and
                i32.store offset=1059308
                br 2 (;@4;)
              end
              local.get 3
              local.get 6
              i32.eq
              drop
              local.get 3
              local.get 4
              i32.store offset=8
              local.get 4
              local.get 3
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
                i32.const 0
                i32.load offset=1059324
                local.get 2
                i32.load offset=8
                local.tee 3
                i32.gt_u
                drop
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
                local.tee 4
                i32.load
                local.tee 3
                br_if 0 (;@6;)
                local.get 2
                i32.const 16
                i32.add
                local.tee 4
                i32.load
                local.tee 3
                br_if 0 (;@6;)
                i32.const 0
                local.set 6
                br 1 (;@5;)
              end
              loop  ;; label = @6
                local.get 4
                local.set 5
                local.get 3
                local.tee 6
                i32.const 20
                i32.add
                local.tee 4
                i32.load
                local.tee 3
                br_if 0 (;@6;)
                local.get 6
                i32.const 16
                i32.add
                local.set 4
                local.get 6
                i32.load offset=16
                local.tee 3
                br_if 0 (;@6;)
              end
              local.get 5
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
                local.tee 4
                i32.const 2
                i32.shl
                i32.const 1059612
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
                i32.load offset=1059312
                i32.const -2
                local.get 4
                i32.rotl
                i32.and
                i32.store offset=1059312
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
          i32.load offset=1059328
          i32.ne
          br_if 1 (;@2;)
          i32.const 0
          local.get 1
          i32.store offset=1059316
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
        i32.const 1059348
        i32.add
        local.set 1
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=1059308
            local.tee 4
            i32.const 1
            local.get 3
            i32.shl
            local.tee 3
            i32.and
            br_if 0 (;@4;)
            i32.const 0
            local.get 4
            local.get 3
            i32.or
            i32.store offset=1059308
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
      i32.const 31
      local.set 3
      block  ;; label = @2
        local.get 1
        i32.const 16777215
        i32.gt_u
        br_if 0 (;@2;)
        local.get 1
        i32.const 8
        i32.shr_u
        local.tee 3
        local.get 3
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee 3
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
        local.get 3
        local.get 4
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
      i32.const 1059612
      i32.add
      local.set 4
      block  ;; label = @2
        i32.const 0
        i32.load offset=1059312
        local.tee 6
        i32.const 1
        local.get 3
        i32.shl
        local.tee 2
        i32.and
        br_if 0 (;@2;)
        local.get 4
        local.get 0
        i32.store
        i32.const 0
        local.get 6
        local.get 2
        i32.or
        i32.store offset=1059312
        local.get 0
        i32.const 24
        i32.add
        local.get 4
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
      local.get 4
      i32.load
      local.set 6
      block  ;; label = @2
        loop  ;; label = @3
          local.get 6
          local.tee 4
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
          local.get 4
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
        local.get 4
        i32.store
        local.get 0
        local.get 0
        i32.store offset=12
        local.get 0
        local.get 0
        i32.store offset=8
        return
      end
      local.get 4
      i32.load offset=8
      local.tee 1
      local.get 0
      i32.store offset=12
      local.get 4
      local.get 0
      i32.store offset=8
      local.get 0
      i32.const 24
      i32.add
      i32.const 0
      i32.store
      local.get 0
      local.get 4
      i32.store offset=12
      local.get 0
      local.get 1
      i32.store offset=8
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
      i32.store offset=1059804
      i32.const 0
      return
    end
    block  ;; label = @1
      local.get 0
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
      i32.add
      i32.const 12
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
      i32.const 0
      local.get 0
      local.get 3
      local.get 2
      i32.sub
      i32.const 15
      i32.gt_u
      select
      i32.add
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
      local.get 2
      local.get 3
      i32.add
      local.tee 6
      local.get 6
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
  (func $_Exit (type 0) (param i32)
    local.get 0
    call $__wasi_proc_exit
    unreachable)
  (func $sbrk (type 10) (param i32) (result i32)
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
        i32.store offset=1059804
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
  (func $__wasilibc_ensure_environ (type 7)
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059812
      i32.const -1
      i32.ne
      br_if 0 (;@1;)
      call $__wasilibc_initialize_environ
    end)
  (func $__wasilibc_initialize_environ (type 7)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 0
    global.set $__stack_pointer
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
            i32.const 1059808
            i32.store offset=1059812
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
      i32.store offset=1059812
    end
    local.get 0
    i32.const 16
    i32.add
    global.set $__stack_pointer)
  (func $__wasilibc_initialize_environ_eagerly (type 7)
    call $__wasilibc_initialize_environ)
  (func $abort (type 7)
    unreachable
    unreachable)
  (func $getcwd (type 3) (param i32 i32) (result i32)
    (local i32)
    i32.const 0
    i32.load offset=1059212
    local.set 2
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        br_if 0 (;@2;)
        local.get 2
        call $strdup
        local.tee 0
        br_if 1 (;@1;)
        i32.const 0
        i32.const 48
        i32.store offset=1059804
        i32.const 0
        return
      end
      block  ;; label = @2
        local.get 2
        call $strlen
        i32.const 1
        i32.add
        local.get 1
        i32.gt_u
        br_if 0 (;@2;)
        local.get 0
        local.get 2
        call $strcpy
        return
      end
      i32.const 0
      local.set 0
      i32.const 0
      i32.const 68
      i32.store offset=1059804
    end
    local.get 0)
  (func $__wasi_environ_get (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $__imported_wasi_snapshot_preview1_environ_get
    i32.const 65535
    i32.and)
  (func $__wasi_environ_sizes_get (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $__imported_wasi_snapshot_preview1_environ_sizes_get
    i32.const 65535
    i32.and)
  (func $__wasi_proc_exit (type 0) (param i32)
    local.get 0
    call $__imported_wasi_snapshot_preview1_proc_exit
    unreachable)
  (func $dummy (type 7))
  (func $__wasm_call_dtors (type 7)
    call $dummy
    call $dummy)
  (func $getenv (type 10) (param i32) (result i32)
    (local i32 i32 i32 i32)
    call $__wasilibc_ensure_environ
    block  ;; label = @1
      local.get 0
      i32.const 61
      call $__strchrnul
      local.get 0
      i32.sub
      local.tee 1
      br_if 0 (;@1;)
      i32.const 0
      return
    end
    i32.const 0
    local.set 2
    block  ;; label = @1
      local.get 0
      local.get 1
      i32.add
      i32.load8_u
      br_if 0 (;@1;)
      i32.const 0
      i32.load offset=1059812
      local.tee 3
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.load
      local.tee 4
      i32.eqz
      br_if 0 (;@1;)
      local.get 3
      i32.const 4
      i32.add
      local.set 3
      block  ;; label = @2
        loop  ;; label = @3
          block  ;; label = @4
            local.get 0
            local.get 4
            local.get 1
            call $strncmp
            br_if 0 (;@4;)
            local.get 4
            local.get 1
            i32.add
            local.tee 4
            i32.load8_u
            i32.const 61
            i32.eq
            br_if 2 (;@2;)
          end
          local.get 3
          i32.load
          local.set 4
          local.get 3
          i32.const 4
          i32.add
          local.set 3
          local.get 4
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 4
      i32.const 1
      i32.add
      local.set 2
    end
    local.get 2)
  (func $strdup (type 10) (param i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      local.get 0
      call $strlen
      i32.const 1
      i32.add
      local.tee 1
      call $malloc
      local.tee 2
      i32.eqz
      br_if 0 (;@1;)
      local.get 2
      local.get 0
      local.get 1
      call $memcpy
      drop
    end
    local.get 2)
  (func $memmove (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      local.get 0
      local.get 1
      i32.eq
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 1
        local.get 0
        local.get 2
        i32.add
        local.tee 3
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
      local.set 4
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            local.get 1
            i32.ge_u
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 4
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              local.set 4
              local.get 0
              local.set 3
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 0
              i32.const 3
              i32.and
              br_if 0 (;@5;)
              local.get 2
              local.set 4
              local.get 0
              local.set 3
              br 2 (;@3;)
            end
            local.get 2
            i32.eqz
            br_if 3 (;@1;)
            local.get 0
            local.get 1
            i32.load8_u
            i32.store8
            local.get 2
            i32.const -1
            i32.add
            local.set 4
            block  ;; label = @5
              local.get 0
              i32.const 1
              i32.add
              local.tee 3
              i32.const 3
              i32.and
              br_if 0 (;@5;)
              local.get 1
              i32.const 1
              i32.add
              local.set 1
              br 2 (;@3;)
            end
            local.get 4
            i32.eqz
            br_if 3 (;@1;)
            local.get 0
            local.get 1
            i32.load8_u offset=1
            i32.store8 offset=1
            local.get 2
            i32.const -2
            i32.add
            local.set 4
            block  ;; label = @5
              local.get 0
              i32.const 2
              i32.add
              local.tee 3
              i32.const 3
              i32.and
              br_if 0 (;@5;)
              local.get 1
              i32.const 2
              i32.add
              local.set 1
              br 2 (;@3;)
            end
            local.get 4
            i32.eqz
            br_if 3 (;@1;)
            local.get 0
            local.get 1
            i32.load8_u offset=2
            i32.store8 offset=2
            local.get 2
            i32.const -3
            i32.add
            local.set 4
            block  ;; label = @5
              local.get 0
              i32.const 3
              i32.add
              local.tee 3
              i32.const 3
              i32.and
              br_if 0 (;@5;)
              local.get 1
              i32.const 3
              i32.add
              local.set 1
              br 2 (;@3;)
            end
            local.get 4
            i32.eqz
            br_if 3 (;@1;)
            local.get 0
            local.get 1
            i32.load8_u offset=3
            i32.store8 offset=3
            local.get 0
            i32.const 4
            i32.add
            local.set 3
            local.get 1
            i32.const 4
            i32.add
            local.set 1
            local.get 2
            i32.const -4
            i32.add
            local.set 4
            br 1 (;@3;)
          end
          block  ;; label = @4
            local.get 4
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 3
              i32.const 3
              i32.and
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              i32.eqz
              br_if 4 (;@1;)
              local.get 0
              local.get 2
              i32.const -1
              i32.add
              local.tee 3
              i32.add
              local.tee 4
              local.get 1
              local.get 3
              i32.add
              i32.load8_u
              i32.store8
              block  ;; label = @6
                local.get 4
                i32.const 3
                i32.and
                br_if 0 (;@6;)
                local.get 3
                local.set 2
                br 1 (;@5;)
              end
              local.get 3
              i32.eqz
              br_if 4 (;@1;)
              local.get 0
              local.get 2
              i32.const -2
              i32.add
              local.tee 3
              i32.add
              local.tee 4
              local.get 1
              local.get 3
              i32.add
              i32.load8_u
              i32.store8
              block  ;; label = @6
                local.get 4
                i32.const 3
                i32.and
                br_if 0 (;@6;)
                local.get 3
                local.set 2
                br 1 (;@5;)
              end
              local.get 3
              i32.eqz
              br_if 4 (;@1;)
              local.get 0
              local.get 2
              i32.const -3
              i32.add
              local.tee 3
              i32.add
              local.tee 4
              local.get 1
              local.get 3
              i32.add
              i32.load8_u
              i32.store8
              block  ;; label = @6
                local.get 4
                i32.const 3
                i32.and
                br_if 0 (;@6;)
                local.get 3
                local.set 2
                br 1 (;@5;)
              end
              local.get 3
              i32.eqz
              br_if 4 (;@1;)
              local.get 0
              local.get 2
              i32.const -4
              i32.add
              local.tee 2
              i32.add
              local.get 1
              local.get 2
              i32.add
              i32.load8_u
              i32.store8
            end
            local.get 2
            i32.const 4
            i32.lt_u
            br_if 0 (;@4;)
            block  ;; label = @5
              local.get 2
              i32.const -4
              i32.add
              local.tee 5
              i32.const 2
              i32.shr_u
              i32.const 1
              i32.add
              i32.const 3
              i32.and
              local.tee 3
              i32.eqz
              br_if 0 (;@5;)
              local.get 1
              i32.const -4
              i32.add
              local.set 4
              local.get 0
              i32.const -4
              i32.add
              local.set 6
              loop  ;; label = @6
                local.get 6
                local.get 2
                i32.add
                local.get 4
                local.get 2
                i32.add
                i32.load
                i32.store
                local.get 2
                i32.const -4
                i32.add
                local.set 2
                local.get 3
                i32.const -1
                i32.add
                local.tee 3
                br_if 0 (;@6;)
              end
            end
            local.get 5
            i32.const 12
            i32.lt_u
            br_if 0 (;@4;)
            local.get 1
            i32.const -16
            i32.add
            local.set 6
            local.get 0
            i32.const -16
            i32.add
            local.set 5
            loop  ;; label = @5
              local.get 5
              local.get 2
              i32.add
              local.tee 3
              i32.const 12
              i32.add
              local.get 6
              local.get 2
              i32.add
              local.tee 4
              i32.const 12
              i32.add
              i32.load
              i32.store
              local.get 3
              i32.const 8
              i32.add
              local.get 4
              i32.const 8
              i32.add
              i32.load
              i32.store
              local.get 3
              i32.const 4
              i32.add
              local.get 4
              i32.const 4
              i32.add
              i32.load
              i32.store
              local.get 3
              local.get 4
              i32.load
              i32.store
              local.get 2
              i32.const -16
              i32.add
              local.tee 2
              i32.const 3
              i32.gt_u
              br_if 0 (;@5;)
            end
          end
          local.get 2
          i32.eqz
          br_if 2 (;@1;)
          local.get 2
          i32.const -1
          i32.add
          local.set 5
          block  ;; label = @4
            local.get 2
            i32.const 3
            i32.and
            local.tee 3
            i32.eqz
            br_if 0 (;@4;)
            local.get 1
            i32.const -1
            i32.add
            local.set 4
            local.get 0
            i32.const -1
            i32.add
            local.set 6
            loop  ;; label = @5
              local.get 6
              local.get 2
              i32.add
              local.get 4
              local.get 2
              i32.add
              i32.load8_u
              i32.store8
              local.get 2
              i32.const -1
              i32.add
              local.set 2
              local.get 3
              i32.const -1
              i32.add
              local.tee 3
              br_if 0 (;@5;)
            end
          end
          local.get 5
          i32.const 3
          i32.lt_u
          br_if 2 (;@1;)
          local.get 1
          i32.const -4
          i32.add
          local.set 4
          local.get 0
          i32.const -4
          i32.add
          local.set 6
          loop  ;; label = @4
            local.get 6
            local.get 2
            i32.add
            local.tee 1
            i32.const 3
            i32.add
            local.get 4
            local.get 2
            i32.add
            local.tee 3
            i32.const 3
            i32.add
            i32.load8_u
            i32.store8
            local.get 1
            i32.const 2
            i32.add
            local.get 3
            i32.const 2
            i32.add
            i32.load8_u
            i32.store8
            local.get 1
            i32.const 1
            i32.add
            local.get 3
            i32.const 1
            i32.add
            i32.load8_u
            i32.store8
            local.get 1
            local.get 3
            i32.load8_u
            i32.store8
            local.get 2
            i32.const -4
            i32.add
            local.tee 2
            br_if 0 (;@4;)
            br 3 (;@1;)
          end
        end
        local.get 4
        i32.const 4
        i32.lt_u
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 4
          i32.const -4
          i32.add
          local.tee 6
          i32.const 2
          i32.shr_u
          i32.const 1
          i32.add
          i32.const 7
          i32.and
          local.tee 2
          i32.eqz
          br_if 0 (;@3;)
          loop  ;; label = @4
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
            local.get 4
            i32.const -4
            i32.add
            local.set 4
            local.get 2
            i32.const -1
            i32.add
            local.tee 2
            br_if 0 (;@4;)
          end
        end
        local.get 6
        i32.const 28
        i32.lt_u
        br_if 0 (;@2;)
        loop  ;; label = @3
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
          local.get 1
          i32.const 16
          i32.add
          i32.load
          i32.store
          local.get 3
          i32.const 20
          i32.add
          local.get 1
          i32.const 20
          i32.add
          i32.load
          i32.store
          local.get 3
          i32.const 24
          i32.add
          local.get 1
          i32.const 24
          i32.add
          i32.load
          i32.store
          local.get 3
          i32.const 28
          i32.add
          local.get 1
          i32.const 28
          i32.add
          i32.load
          i32.store
          local.get 3
          i32.const 32
          i32.add
          local.set 3
          local.get 1
          i32.const 32
          i32.add
          local.set 1
          local.get 4
          i32.const -32
          i32.add
          local.tee 4
          i32.const 3
          i32.gt_u
          br_if 0 (;@3;)
        end
      end
      local.get 4
      i32.eqz
      br_if 0 (;@1;)
      local.get 4
      i32.const -1
      i32.add
      local.set 6
      block  ;; label = @2
        local.get 4
        i32.const 7
        i32.and
        local.tee 2
        i32.eqz
        br_if 0 (;@2;)
        loop  ;; label = @3
          local.get 3
          local.get 1
          i32.load8_u
          i32.store8
          local.get 4
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
          i32.const -1
          i32.add
          local.tee 2
          br_if 0 (;@3;)
        end
      end
      local.get 6
      i32.const 7
      i32.lt_u
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 3
        local.get 1
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 1
        i32.add
        local.get 1
        i32.const 1
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 2
        i32.add
        local.get 1
        i32.const 2
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 3
        i32.add
        local.get 1
        i32.const 3
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 4
        i32.add
        local.get 1
        i32.const 4
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 5
        i32.add
        local.get 1
        i32.const 5
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 6
        i32.add
        local.get 1
        i32.const 6
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 7
        i32.add
        local.get 1
        i32.const 7
        i32.add
        i32.load8_u
        i32.store8
        local.get 3
        i32.const 8
        i32.add
        local.set 3
        local.get 1
        i32.const 8
        i32.add
        local.set 1
        local.get 4
        i32.const -8
        i32.add
        local.tee 4
        br_if 0 (;@2;)
      end
    end
    local.get 0)
  (func $__strchrnul (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 255
            i32.and
            local.tee 2
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            i32.const 3
            i32.and
            i32.eqz
            br_if 2 (;@2;)
            block  ;; label = @5
              local.get 0
              i32.load8_u
              local.tee 3
              br_if 0 (;@5;)
              local.get 0
              return
            end
            local.get 3
            local.get 1
            i32.const 255
            i32.and
            i32.ne
            br_if 1 (;@3;)
            local.get 0
            return
          end
          local.get 0
          local.get 0
          call $strlen
          i32.add
          return
        end
        block  ;; label = @3
          local.get 0
          i32.const 1
          i32.add
          local.tee 3
          i32.const 3
          i32.and
          br_if 0 (;@3;)
          local.get 3
          local.set 0
          br 1 (;@2;)
        end
        local.get 3
        i32.load8_u
        local.tee 4
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        local.get 1
        i32.const 255
        i32.and
        i32.eq
        br_if 1 (;@1;)
        block  ;; label = @3
          local.get 0
          i32.const 2
          i32.add
          local.tee 3
          i32.const 3
          i32.and
          br_if 0 (;@3;)
          local.get 3
          local.set 0
          br 1 (;@2;)
        end
        local.get 3
        i32.load8_u
        local.tee 4
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        local.get 1
        i32.const 255
        i32.and
        i32.eq
        br_if 1 (;@1;)
        block  ;; label = @3
          local.get 0
          i32.const 3
          i32.add
          local.tee 3
          i32.const 3
          i32.and
          br_if 0 (;@3;)
          local.get 3
          local.set 0
          br 1 (;@2;)
        end
        local.get 3
        i32.load8_u
        local.tee 4
        i32.eqz
        br_if 1 (;@1;)
        local.get 4
        local.get 1
        i32.const 255
        i32.and
        i32.eq
        br_if 1 (;@1;)
        local.get 0
        i32.const 4
        i32.add
        local.set 0
      end
      block  ;; label = @2
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
        br_if 0 (;@2;)
        local.get 2
        i32.const 16843009
        i32.mul
        local.set 2
        loop  ;; label = @3
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
          br_if 1 (;@2;)
          local.get 0
          i32.const 4
          i32.add
          local.tee 0
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
          i32.eqz
          br_if 0 (;@3;)
        end
      end
      local.get 0
      i32.const -1
      i32.add
      local.set 3
      loop  ;; label = @2
        local.get 3
        i32.const 1
        i32.add
        local.tee 3
        i32.load8_u
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        i32.const 255
        i32.and
        i32.ne
        br_if 0 (;@2;)
      end
    end
    local.get 3)
  (func $memcpy (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 1
        i32.load8_u
        i32.store8
        local.get 2
        i32.const -1
        i32.add
        local.set 3
        local.get 0
        i32.const 1
        i32.add
        local.set 4
        local.get 1
        i32.const 1
        i32.add
        local.tee 5
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        i32.load8_u offset=1
        i32.store8 offset=1
        local.get 2
        i32.const -2
        i32.add
        local.set 3
        local.get 0
        i32.const 2
        i32.add
        local.set 4
        local.get 1
        i32.const 2
        i32.add
        local.tee 5
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        i32.load8_u offset=2
        i32.store8 offset=2
        local.get 2
        i32.const -3
        i32.add
        local.set 3
        local.get 0
        i32.const 3
        i32.add
        local.set 4
        local.get 1
        i32.const 3
        i32.add
        local.tee 5
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        i32.load8_u offset=3
        i32.store8 offset=3
        local.get 2
        i32.const -4
        i32.add
        local.set 3
        local.get 0
        i32.const 4
        i32.add
        local.set 4
        local.get 1
        i32.const 4
        i32.add
        local.set 5
        br 1 (;@1;)
      end
      local.get 2
      local.set 3
      local.get 0
      local.set 4
      local.get 1
      local.set 5
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          i32.const 3
          i32.and
          local.tee 1
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.const 16
              i32.lt_u
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 3
                i32.const -16
                i32.add
                local.tee 1
                i32.const 16
                i32.and
                br_if 0 (;@6;)
                local.get 4
                local.get 5
                i64.load align=4
                i64.store align=4
                local.get 4
                local.get 5
                i64.load offset=8 align=4
                i64.store offset=8 align=4
                local.get 4
                i32.const 16
                i32.add
                local.set 4
                local.get 5
                i32.const 16
                i32.add
                local.set 5
                local.get 1
                local.set 3
              end
              local.get 1
              i32.const 16
              i32.lt_u
              br_if 1 (;@4;)
              loop  ;; label = @6
                local.get 4
                local.get 5
                i64.load align=4
                i64.store align=4
                local.get 4
                i32.const 8
                i32.add
                local.get 5
                i32.const 8
                i32.add
                i64.load align=4
                i64.store align=4
                local.get 4
                i32.const 16
                i32.add
                local.get 5
                i32.const 16
                i32.add
                i64.load align=4
                i64.store align=4
                local.get 4
                i32.const 24
                i32.add
                local.get 5
                i32.const 24
                i32.add
                i64.load align=4
                i64.store align=4
                local.get 4
                i32.const 32
                i32.add
                local.set 4
                local.get 5
                i32.const 32
                i32.add
                local.set 5
                local.get 3
                i32.const -32
                i32.add
                local.tee 3
                i32.const 15
                i32.gt_u
                br_if 0 (;@6;)
              end
            end
            local.get 3
            local.set 1
          end
          block  ;; label = @4
            local.get 1
            i32.const 8
            i32.and
            i32.eqz
            br_if 0 (;@4;)
            local.get 4
            local.get 5
            i64.load align=4
            i64.store align=4
            local.get 5
            i32.const 8
            i32.add
            local.set 5
            local.get 4
            i32.const 8
            i32.add
            local.set 4
          end
          block  ;; label = @4
            local.get 1
            i32.const 4
            i32.and
            i32.eqz
            br_if 0 (;@4;)
            local.get 4
            local.get 5
            i32.load
            i32.store
            local.get 5
            i32.const 4
            i32.add
            local.set 5
            local.get 4
            i32.const 4
            i32.add
            local.set 4
          end
          block  ;; label = @4
            local.get 1
            i32.const 2
            i32.and
            i32.eqz
            br_if 0 (;@4;)
            local.get 4
            local.get 5
            i32.load16_u align=1
            i32.store16 align=1
            local.get 4
            i32.const 2
            i32.add
            local.set 4
            local.get 5
            i32.const 2
            i32.add
            local.set 5
          end
          local.get 1
          i32.const 1
          i32.and
          br_if 1 (;@2;)
          br 2 (;@1;)
        end
        block  ;; label = @3
          local.get 3
          i32.const 32
          i32.lt_u
          br_if 0 (;@3;)
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 1
                i32.const -1
                i32.add
                br_table 0 (;@6;) 1 (;@5;) 2 (;@4;) 3 (;@3;)
              end
              local.get 4
              local.get 5
              i32.load
              local.tee 6
              i32.store8
              local.get 4
              local.get 6
              i32.const 16
              i32.shr_u
              i32.store8 offset=2
              local.get 4
              local.get 6
              i32.const 8
              i32.shr_u
              i32.store8 offset=1
              local.get 3
              i32.const -3
              i32.add
              local.set 3
              local.get 4
              i32.const 3
              i32.add
              local.set 7
              i32.const 0
              local.set 1
              loop  ;; label = @6
                local.get 7
                local.get 1
                i32.add
                local.tee 4
                local.get 5
                local.get 1
                i32.add
                local.tee 2
                i32.const 4
                i32.add
                i32.load
                local.tee 8
                i32.const 8
                i32.shl
                local.get 6
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get 4
                i32.const 4
                i32.add
                local.get 2
                i32.const 8
                i32.add
                i32.load
                local.tee 6
                i32.const 8
                i32.shl
                local.get 8
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get 4
                i32.const 8
                i32.add
                local.get 2
                i32.const 12
                i32.add
                i32.load
                local.tee 8
                i32.const 8
                i32.shl
                local.get 6
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get 4
                i32.const 12
                i32.add
                local.get 2
                i32.const 16
                i32.add
                i32.load
                local.tee 6
                i32.const 8
                i32.shl
                local.get 8
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get 1
                i32.const 16
                i32.add
                local.set 1
                local.get 3
                i32.const -16
                i32.add
                local.tee 3
                i32.const 16
                i32.gt_u
                br_if 0 (;@6;)
              end
              local.get 7
              local.get 1
              i32.add
              local.set 4
              local.get 5
              local.get 1
              i32.add
              i32.const 3
              i32.add
              local.set 5
              br 2 (;@3;)
            end
            local.get 4
            local.get 5
            i32.load
            local.tee 6
            i32.store16 align=1
            local.get 3
            i32.const -2
            i32.add
            local.set 3
            local.get 4
            i32.const 2
            i32.add
            local.set 7
            i32.const 0
            local.set 1
            loop  ;; label = @5
              local.get 7
              local.get 1
              i32.add
              local.tee 4
              local.get 5
              local.get 1
              i32.add
              local.tee 2
              i32.const 4
              i32.add
              i32.load
              local.tee 8
              i32.const 16
              i32.shl
              local.get 6
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get 4
              i32.const 4
              i32.add
              local.get 2
              i32.const 8
              i32.add
              i32.load
              local.tee 6
              i32.const 16
              i32.shl
              local.get 8
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get 4
              i32.const 8
              i32.add
              local.get 2
              i32.const 12
              i32.add
              i32.load
              local.tee 8
              i32.const 16
              i32.shl
              local.get 6
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get 4
              i32.const 12
              i32.add
              local.get 2
              i32.const 16
              i32.add
              i32.load
              local.tee 6
              i32.const 16
              i32.shl
              local.get 8
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get 1
              i32.const 16
              i32.add
              local.set 1
              local.get 3
              i32.const -16
              i32.add
              local.tee 3
              i32.const 17
              i32.gt_u
              br_if 0 (;@5;)
            end
            local.get 7
            local.get 1
            i32.add
            local.set 4
            local.get 5
            local.get 1
            i32.add
            i32.const 2
            i32.add
            local.set 5
            br 1 (;@3;)
          end
          local.get 4
          local.get 5
          i32.load
          local.tee 6
          i32.store8
          local.get 3
          i32.const -1
          i32.add
          local.set 3
          local.get 4
          i32.const 1
          i32.add
          local.set 7
          i32.const 0
          local.set 1
          loop  ;; label = @4
            local.get 7
            local.get 1
            i32.add
            local.tee 4
            local.get 5
            local.get 1
            i32.add
            local.tee 2
            i32.const 4
            i32.add
            i32.load
            local.tee 8
            i32.const 24
            i32.shl
            local.get 6
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get 4
            i32.const 4
            i32.add
            local.get 2
            i32.const 8
            i32.add
            i32.load
            local.tee 6
            i32.const 24
            i32.shl
            local.get 8
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get 4
            i32.const 8
            i32.add
            local.get 2
            i32.const 12
            i32.add
            i32.load
            local.tee 8
            i32.const 24
            i32.shl
            local.get 6
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get 4
            i32.const 12
            i32.add
            local.get 2
            i32.const 16
            i32.add
            i32.load
            local.tee 6
            i32.const 24
            i32.shl
            local.get 8
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get 1
            i32.const 16
            i32.add
            local.set 1
            local.get 3
            i32.const -16
            i32.add
            local.tee 3
            i32.const 18
            i32.gt_u
            br_if 0 (;@4;)
          end
          local.get 7
          local.get 1
          i32.add
          local.set 4
          local.get 5
          local.get 1
          i32.add
          i32.const 1
          i32.add
          local.set 5
        end
        block  ;; label = @3
          local.get 3
          i32.const 16
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          local.get 5
          i32.load8_u
          i32.store8
          local.get 4
          local.get 5
          i32.load offset=1 align=1
          i32.store offset=1 align=1
          local.get 4
          local.get 5
          i64.load offset=5 align=1
          i64.store offset=5 align=1
          local.get 4
          local.get 5
          i32.load16_u offset=13 align=1
          i32.store16 offset=13 align=1
          local.get 4
          local.get 5
          i32.load8_u offset=15
          i32.store8 offset=15
          local.get 4
          i32.const 16
          i32.add
          local.set 4
          local.get 5
          i32.const 16
          i32.add
          local.set 5
        end
        block  ;; label = @3
          local.get 3
          i32.const 8
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          local.get 5
          i64.load align=1
          i64.store align=1
          local.get 4
          i32.const 8
          i32.add
          local.set 4
          local.get 5
          i32.const 8
          i32.add
          local.set 5
        end
        block  ;; label = @3
          local.get 3
          i32.const 4
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          local.get 5
          i32.load align=1
          i32.store align=1
          local.get 4
          i32.const 4
          i32.add
          local.set 4
          local.get 5
          i32.const 4
          i32.add
          local.set 5
        end
        block  ;; label = @3
          local.get 3
          i32.const 2
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          local.get 5
          i32.load16_u align=1
          i32.store16 align=1
          local.get 4
          i32.const 2
          i32.add
          local.set 4
          local.get 5
          i32.const 2
          i32.add
          local.set 5
        end
        local.get 3
        i32.const 1
        i32.and
        i32.eqz
        br_if 1 (;@1;)
      end
      local.get 4
      local.get 5
      i32.load8_u
      i32.store8
    end
    local.get 0)
  (func $memset (type 5) (param i32 i32 i32) (result i32)
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
      i64.const 4294967297
      i64.mul
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
  (func $strncmp (type 5) (param i32 i32 i32) (result i32)
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
          local.get 1
          i32.load8_u
          local.tee 5
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
          local.get 4
          i32.const 255
          i32.and
          local.get 5
          i32.eq
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
  (func $strerror (type 10) (param i32) (result i32)
    (local i32)
    block  ;; label = @1
      i32.const 0
      i32.load offset=1059840
      local.tee 1
      br_if 0 (;@1;)
      i32.const 1059816
      local.set 1
      i32.const 0
      i32.const 1059816
      i32.store offset=1059840
    end
    i32.const 0
    local.get 0
    local.get 0
    i32.const 76
    i32.gt_u
    select
    i32.const 1
    i32.shl
    i32.const 1054944
    i32.add
    i32.load16_u
    i32.const 1053382
    i32.add
    local.get 1
    i32.load offset=20
    call $__lctrans)
  (func $strerror_r (type 5) (param i32 i32 i32) (result i32)
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
  (func $__stpcpy (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          local.get 0
          i32.xor
          i32.const 3
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          local.set 2
          br 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            local.get 1
            i32.const 3
            i32.and
            br_if 0 (;@4;)
            local.get 0
            local.set 2
            br 1 (;@3;)
          end
          local.get 0
          local.get 1
          i32.load8_u
          local.tee 2
          i32.store8
          block  ;; label = @4
            local.get 2
            br_if 0 (;@4;)
            local.get 0
            return
          end
          local.get 0
          i32.const 1
          i32.add
          local.set 2
          block  ;; label = @4
            local.get 1
            i32.const 1
            i32.add
            local.tee 3
            i32.const 3
            i32.and
            br_if 0 (;@4;)
            local.get 3
            local.set 1
            br 1 (;@3;)
          end
          local.get 2
          local.get 3
          i32.load8_u
          local.tee 3
          i32.store8
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 0
          i32.const 2
          i32.add
          local.set 2
          block  ;; label = @4
            local.get 1
            i32.const 2
            i32.add
            local.tee 3
            i32.const 3
            i32.and
            br_if 0 (;@4;)
            local.get 3
            local.set 1
            br 1 (;@3;)
          end
          local.get 2
          local.get 3
          i32.load8_u
          local.tee 3
          i32.store8
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 0
          i32.const 3
          i32.add
          local.set 2
          block  ;; label = @4
            local.get 1
            i32.const 3
            i32.add
            local.tee 3
            i32.const 3
            i32.and
            br_if 0 (;@4;)
            local.get 3
            local.set 1
            br 1 (;@3;)
          end
          local.get 2
          local.get 3
          i32.load8_u
          local.tee 3
          i32.store8
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 0
          i32.const 4
          i32.add
          local.set 2
          local.get 1
          i32.const 4
          i32.add
          local.set 1
        end
        local.get 1
        i32.load
        local.tee 0
        i32.const -1
        i32.xor
        local.get 0
        i32.const -16843009
        i32.add
        i32.and
        i32.const -2139062144
        i32.and
        br_if 0 (;@2;)
        loop  ;; label = @3
          local.get 2
          local.get 0
          i32.store
          local.get 2
          i32.const 4
          i32.add
          local.set 2
          local.get 1
          i32.const 4
          i32.add
          local.tee 1
          i32.load
          local.tee 0
          i32.const -1
          i32.xor
          local.get 0
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          i32.eqz
          br_if 0 (;@3;)
        end
      end
      local.get 2
      local.get 1
      i32.load8_u
      local.tee 0
      i32.store8
      local.get 0
      i32.eqz
      br_if 0 (;@1;)
      local.get 1
      i32.const 1
      i32.add
      local.set 1
      loop  ;; label = @2
        local.get 2
        i32.const 1
        i32.add
        local.tee 2
        local.get 1
        i32.load8_u
        local.tee 0
        i32.store8
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        local.get 0
        br_if 0 (;@2;)
      end
    end
    local.get 2)
  (func $strcpy (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $__stpcpy
    drop
    local.get 0)
  (func $strlen (type 10) (param i32) (result i32)
    (local i32 i32)
    local.get 0
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.set 1
        local.get 0
        i32.load8_u
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 1
        i32.add
        local.tee 1
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load8_u
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 2
        i32.add
        local.tee 1
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load8_u
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 3
        i32.add
        local.tee 1
        i32.const 3
        i32.and
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        i32.load8_u
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        i32.const 4
        i32.add
        local.set 1
      end
      local.get 1
      i32.const -4
      i32.add
      local.set 1
      loop  ;; label = @2
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
        br_if 0 (;@2;)
      end
      local.get 2
      i32.const 255
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 1
        i32.const 1
        i32.add
        local.tee 1
        i32.load8_u
        br_if 0 (;@2;)
      end
    end
    local.get 1
    local.get 0
    i32.sub)
  (func $dummy.1 (type 3) (param i32 i32) (result i32)
    local.get 0)
  (func $__lctrans (type 3) (param i32 i32) (result i32)
    local.get 0
    local.get 1
    call $dummy.1)
  (func $_ZN4core10intrinsics17const_eval_select17hbd6213c80b3cb031E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core3ops8function6FnOnce9call_once17hb51a445455a75a4cE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17hb51a445455a75a4cE (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN5alloc5alloc18handle_alloc_error8rt_error17haea38d28aaaeb3e5E
    unreachable)
  (func $_ZN5alloc5alloc18handle_alloc_error8rt_error17haea38d28aaaeb3e5E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $__rust_alloc_error_handler
    unreachable)
  (func $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E (type 8) (param i32 i32 i32 i32)
    (local i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 2
                  i32.eqz
                  br_if 0 (;@7;)
                  i32.const 1
                  local.set 4
                  local.get 1
                  i32.const 0
                  i32.lt_s
                  br_if 1 (;@6;)
                  local.get 3
                  i32.load offset=8
                  i32.eqz
                  br_if 3 (;@4;)
                  local.get 3
                  i32.load offset=4
                  local.tee 5
                  br_if 2 (;@5;)
                  local.get 1
                  br_if 4 (;@3;)
                  local.get 2
                  local.set 3
                  br 5 (;@2;)
                end
                local.get 0
                local.get 1
                i32.store offset=4
                i32.const 1
                local.set 4
              end
              i32.const 0
              local.set 1
              br 4 (;@1;)
            end
            local.get 3
            i32.load
            local.get 5
            local.get 2
            local.get 1
            call $__rust_realloc
            local.set 3
            br 2 (;@2;)
          end
          local.get 1
          br_if 0 (;@3;)
          local.get 2
          local.set 3
          br 1 (;@2;)
        end
        local.get 1
        local.get 2
        call $__rust_alloc
        local.set 3
      end
      block  ;; label = @2
        local.get 3
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        local.get 3
        i32.store offset=4
        i32.const 0
        local.set 4
        br 1 (;@1;)
      end
      local.get 0
      local.get 1
      i32.store offset=4
      local.get 2
      local.set 1
    end
    local.get 0
    local.get 4
    i32.store
    local.get 0
    i32.const 8
    i32.add
    local.get 1
    i32.store)
  (func $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core10intrinsics17const_eval_select17hbd6213c80b3cb031E
    unreachable)
  (func $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE (type 7)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 0
    global.set $__stack_pointer
    local.get 0
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get 0
    i32.const 1055100
    i32.store offset=24
    local.get 0
    i64.const 1
    i64.store offset=12 align=4
    local.get 0
    i32.const 1055188
    i32.store offset=8
    local.get 0
    i32.const 8
    i32.add
    i32.const 1055196
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17hf80c878abd8e9e00E (type 2) (param i32 i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      local.get 1
      i32.const 1
      i32.add
      local.tee 3
      local.get 1
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      i32.load
      local.tee 4
      i32.const 1
      i32.shl
      local.tee 1
      local.get 3
      local.get 1
      local.get 3
      i32.gt_u
      select
      local.tee 1
      i32.const 8
      local.get 1
      i32.const 8
      i32.gt_u
      select
      local.set 1
      block  ;; label = @2
        block  ;; label = @3
          local.get 4
          br_if 0 (;@3;)
          i32.const 0
          local.set 3
          br 1 (;@2;)
        end
        local.get 2
        local.get 4
        i32.store offset=20
        local.get 2
        local.get 0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set 3
      end
      local.get 2
      local.get 3
      i32.store offset=24
      local.get 2
      local.get 1
      i32.const 1
      local.get 2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E
      block  ;; label = @2
        local.get 2
        i32.load
        i32.eqz
        br_if 0 (;@2;)
        local.get 2
        i32.const 8
        i32.add
        i32.load
        local.tee 0
        i32.eqz
        br_if 1 (;@1;)
        local.get 2
        i32.load offset=4
        local.get 0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get 2
      i32.load offset=4
      local.set 3
      local.get 0
      i32.const 4
      i32.add
      local.get 1
      i32.store
      local.get 0
      local.get 3
      i32.store
      local.get 2
      i32.const 32
      i32.add
      global.set $__stack_pointer
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $__rg_oom (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $rust_oom
    unreachable)
  (func $_ZN72_$LT$$RF$str$u20$as$u20$alloc..ffi..c_str..CString..new..SpecNewImpl$GT$13spec_new_impl17h8200e939a8ca496bE (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.const 1
          i32.add
          local.tee 4
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
          local.get 4
          i32.const -1
          i32.le_s
          br_if 1 (;@2;)
          local.get 4
          i32.const 1
          call $__rust_alloc
          local.tee 5
          i32.eqz
          br_if 2 (;@1;)
          local.get 5
          local.get 1
          local.get 2
          call $memcpy
          local.set 6
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              i32.const 8
              i32.lt_u
              br_if 0 (;@5;)
              local.get 3
              i32.const 8
              i32.add
              i32.const 0
              local.get 1
              local.get 2
              call $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E
              local.get 3
              i32.load offset=12
              local.set 7
              local.get 3
              i32.load offset=8
              local.set 5
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 2
              br_if 0 (;@5;)
              i32.const 0
              local.set 7
              i32.const 0
              local.set 5
              br 1 (;@4;)
            end
            block  ;; label = @5
              block  ;; label = @6
                local.get 1
                i32.load8_u
                br_if 0 (;@6;)
                i32.const 0
                local.set 8
                br 1 (;@5;)
              end
              i32.const 1
              local.set 8
              i32.const 0
              local.set 5
              block  ;; label = @6
                local.get 2
                i32.const 1
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 1
              i32.load8_u offset=1
              i32.eqz
              br_if 0 (;@5;)
              i32.const 2
              local.set 8
              block  ;; label = @6
                local.get 2
                i32.const 2
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 1
              i32.load8_u offset=2
              i32.eqz
              br_if 0 (;@5;)
              i32.const 3
              local.set 8
              block  ;; label = @6
                local.get 2
                i32.const 3
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 1
              i32.load8_u offset=3
              i32.eqz
              br_if 0 (;@5;)
              i32.const 4
              local.set 8
              block  ;; label = @6
                local.get 2
                i32.const 4
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 1
              i32.load8_u offset=4
              i32.eqz
              br_if 0 (;@5;)
              i32.const 5
              local.set 8
              block  ;; label = @6
                local.get 2
                i32.const 5
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 1
              i32.load8_u offset=5
              i32.eqz
              br_if 0 (;@5;)
              i32.const 6
              local.set 8
              block  ;; label = @6
                local.get 2
                i32.const 6
                i32.ne
                br_if 0 (;@6;)
                local.get 2
                local.set 7
                br 2 (;@4;)
              end
              local.get 2
              local.set 7
              local.get 1
              i32.load8_u offset=6
              br_if 1 (;@4;)
            end
            i32.const 1
            local.set 5
            local.get 8
            local.set 7
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 5
              br_if 0 (;@5;)
              local.get 3
              local.get 2
              i32.store offset=24
              local.get 3
              local.get 4
              i32.store offset=20
              local.get 3
              local.get 6
              i32.store offset=16
              local.get 3
              local.get 3
              i32.const 16
              i32.add
              call $_ZN5alloc3ffi5c_str7CString19_from_vec_unchecked17ha2258490c84ae819E
              local.get 0
              local.get 3
              i64.load
              i64.store offset=4 align=4
              i32.const 0
              local.set 2
              br 1 (;@4;)
            end
            local.get 0
            i32.const 16
            i32.add
            local.get 2
            i32.store
            local.get 0
            i32.const 12
            i32.add
            local.get 4
            i32.store
            local.get 0
            i32.const 8
            i32.add
            local.get 6
            i32.store
            local.get 0
            local.get 7
            i32.store offset=4
            i32.const 1
            local.set 2
          end
          local.get 0
          local.get 2
          i32.store
          local.get 3
          i32.const 32
          i32.add
          global.set $__stack_pointer
          return
        end
        i32.const 1055100
        i32.const 43
        i32.const 1055244
        call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get 4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN5alloc3ffi5c_str7CString19_from_vec_unchecked17ha2258490c84ae819E (type 2) (param i32 i32)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 1
                i32.const 4
                i32.add
                i32.load
                local.tee 3
                local.get 1
                i32.load offset=8
                local.tee 4
                i32.ne
                br_if 0 (;@6;)
                local.get 4
                i32.const 1
                i32.add
                local.tee 3
                local.get 4
                i32.lt_u
                br_if 4 (;@2;)
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 4
                    br_if 0 (;@8;)
                    i32.const 0
                    local.set 5
                    br 1 (;@7;)
                  end
                  local.get 2
                  local.get 4
                  i32.store offset=20
                  local.get 2
                  local.get 1
                  i32.load
                  i32.store offset=16
                  i32.const 1
                  local.set 5
                end
                local.get 2
                local.get 5
                i32.store offset=24
                local.get 2
                local.get 3
                i32.const 1
                local.get 2
                i32.const 16
                i32.add
                call $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E
                local.get 2
                i32.load
                br_if 1 (;@5;)
                local.get 2
                i32.load offset=4
                local.set 5
                local.get 1
                i32.const 4
                i32.add
                local.get 3
                i32.store
                local.get 1
                local.get 5
                i32.store
              end
              block  ;; label = @6
                local.get 4
                local.get 3
                i32.ne
                br_if 0 (;@6;)
                local.get 1
                local.get 4
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17hf80c878abd8e9e00E
                local.get 1
                i32.const 4
                i32.add
                i32.load
                local.set 3
                local.get 1
                i32.load offset=8
                local.set 4
              end
              local.get 1
              local.get 4
              i32.const 1
              i32.add
              local.tee 5
              i32.store offset=8
              local.get 1
              i32.load
              local.tee 1
              local.get 4
              i32.add
              i32.const 0
              i32.store8
              local.get 3
              local.get 5
              i32.gt_u
              br_if 1 (;@4;)
              local.get 1
              local.set 4
              br 2 (;@3;)
            end
            local.get 2
            i32.const 8
            i32.add
            i32.load
            local.tee 1
            i32.eqz
            br_if 2 (;@2;)
            local.get 2
            i32.load offset=4
            local.get 1
            call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
            unreachable
          end
          block  ;; label = @4
            local.get 5
            br_if 0 (;@4;)
            i32.const 1
            local.set 4
            local.get 1
            local.get 3
            i32.const 1
            call $__rust_dealloc
            br 1 (;@3;)
          end
          local.get 1
          local.get 3
          i32.const 1
          local.get 5
          call $__rust_realloc
          local.tee 4
          i32.eqz
          br_if 2 (;@1;)
        end
        local.get 0
        local.get 5
        i32.store offset=4
        local.get 0
        local.get 4
        i32.store
        local.get 2
        i32.const 32
        i32.add
        global.set $__stack_pointer
        return
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get 5
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E (type 2) (param i32 i32)
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
  (func $_ZN4core3ops8function6FnOnce9call_once17h32ae774a248ad9dfE (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core5slice5index27slice_end_index_len_fail_rt17h9667e64b60d286b0E
    unreachable)
  (func $_ZN4core5slice5index27slice_end_index_len_fail_rt17h9667e64b60d286b0E (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 7
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=12 align=4
    local.get 2
    i32.const 1056088
    i32.store offset=8
    local.get 2
    i32.const 7
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
    i32.const 1056104
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17h3754f24dd73f738cE (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core5slice5index25slice_index_order_fail_rt17h328217a869652e55E
    unreachable)
  (func $_ZN4core5slice5index25slice_index_order_fail_rt17h328217a869652e55E (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 7
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=12 align=4
    local.get 2
    i32.const 1056156
    i32.store offset=8
    local.get 2
    i32.const 7
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
    i32.const 1056172
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17ha3f2b2a4e66a4793E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core5slice5index29slice_start_index_len_fail_rt17hd8d1c16b5bc67070E
    unreachable)
  (func $_ZN4core5slice5index29slice_start_index_len_fail_rt17hd8d1c16b5bc67070E (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 7
    i32.store
    local.get 2
    i64.const 2
    i64.store offset=12 align=4
    local.get 2
    i32.const 1056008
    i32.store offset=8
    local.get 2
    i32.const 7
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
    i32.const 1056056
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17hcb988a1f0067a302E (type 8) (param i32 i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    local.get 3
    call $_ZN4core3str19slice_error_fail_rt17h3d6d7e29ae750f51E
    unreachable)
  (func $_ZN4core3str19slice_error_fail_rt17h3d6d7e29ae750f51E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 112
    i32.sub
    local.tee 4
    global.set $__stack_pointer
    local.get 4
    local.get 3
    i32.store offset=12
    local.get 4
    local.get 2
    i32.store offset=8
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 1
                    i32.const 257
                    i32.lt_u
                    br_if 0 (;@8;)
                    i32.const 256
                    local.set 5
                    block  ;; label = @9
                      local.get 0
                      i32.load8_s offset=256
                      i32.const -65
                      i32.gt_s
                      br_if 0 (;@9;)
                      i32.const 255
                      local.set 5
                      local.get 0
                      i32.load8_s offset=255
                      i32.const -65
                      i32.gt_s
                      br_if 0 (;@9;)
                      i32.const 254
                      local.set 5
                      local.get 0
                      i32.load8_s offset=254
                      i32.const -65
                      i32.gt_s
                      br_if 0 (;@9;)
                      i32.const 253
                      local.set 5
                    end
                    local.get 5
                    local.get 1
                    i32.lt_u
                    br_if 1 (;@7;)
                    local.get 5
                    local.get 1
                    i32.ne
                    br_if 3 (;@5;)
                  end
                  local.get 4
                  local.get 1
                  i32.store offset=20
                  local.get 4
                  local.get 0
                  i32.store offset=16
                  i32.const 0
                  local.set 5
                  i32.const 1055260
                  local.set 6
                  br 1 (;@6;)
                end
                local.get 4
                local.get 5
                i32.store offset=20
                local.get 4
                local.get 0
                i32.store offset=16
                i32.const 5
                local.set 5
                i32.const 1056471
                local.set 6
              end
              local.get 4
              local.get 5
              i32.store offset=28
              local.get 4
              local.get 6
              i32.store offset=24
              local.get 2
              local.get 1
              i32.gt_u
              local.tee 5
              br_if 1 (;@4;)
              local.get 3
              local.get 1
              i32.gt_u
              br_if 1 (;@4;)
              block  ;; label = @6
                local.get 2
                local.get 3
                i32.gt_u
                br_if 0 (;@6;)
                block  ;; label = @7
                  block  ;; label = @8
                    local.get 2
                    i32.eqz
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 2
                      local.get 1
                      i32.lt_u
                      br_if 0 (;@9;)
                      local.get 1
                      local.get 2
                      i32.eq
                      br_if 1 (;@8;)
                      br 2 (;@7;)
                    end
                    local.get 0
                    local.get 2
                    i32.add
                    i32.load8_s
                    i32.const -64
                    i32.lt_s
                    br_if 1 (;@7;)
                  end
                  local.get 3
                  local.set 2
                end
                local.get 4
                local.get 2
                i32.store offset=32
                local.get 1
                local.set 3
                block  ;; label = @7
                  local.get 2
                  local.get 1
                  i32.ge_u
                  br_if 0 (;@7;)
                  local.get 2
                  i32.const 1
                  i32.add
                  local.tee 5
                  i32.const 0
                  local.get 2
                  i32.const -3
                  i32.add
                  local.tee 3
                  local.get 3
                  local.get 2
                  i32.gt_u
                  select
                  local.tee 3
                  i32.lt_u
                  br_if 4 (;@3;)
                  block  ;; label = @8
                    local.get 3
                    local.get 5
                    i32.eq
                    br_if 0 (;@8;)
                    local.get 0
                    local.get 5
                    i32.add
                    local.get 0
                    local.get 3
                    i32.add
                    local.tee 7
                    i32.sub
                    local.set 5
                    block  ;; label = @9
                      local.get 0
                      local.get 2
                      i32.add
                      local.tee 8
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const -1
                      i32.add
                      local.set 6
                      br 1 (;@8;)
                    end
                    local.get 3
                    local.get 2
                    i32.eq
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 8
                      i32.const -1
                      i32.add
                      local.tee 2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const -2
                      i32.add
                      local.set 6
                      br 1 (;@8;)
                    end
                    local.get 7
                    local.get 2
                    i32.eq
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 8
                      i32.const -2
                      i32.add
                      local.tee 2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const -3
                      i32.add
                      local.set 6
                      br 1 (;@8;)
                    end
                    local.get 7
                    local.get 2
                    i32.eq
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      local.get 8
                      i32.const -3
                      i32.add
                      local.tee 2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const -4
                      i32.add
                      local.set 6
                      br 1 (;@8;)
                    end
                    local.get 7
                    local.get 2
                    i32.eq
                    br_if 0 (;@8;)
                    local.get 5
                    i32.const -5
                    i32.add
                    local.set 6
                  end
                  local.get 6
                  local.get 3
                  i32.add
                  local.set 3
                end
                block  ;; label = @7
                  local.get 3
                  i32.eqz
                  br_if 0 (;@7;)
                  block  ;; label = @8
                    local.get 3
                    local.get 1
                    i32.lt_u
                    br_if 0 (;@8;)
                    local.get 3
                    local.get 1
                    i32.eq
                    br_if 1 (;@7;)
                    br 7 (;@1;)
                  end
                  local.get 0
                  local.get 3
                  i32.add
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if 6 (;@1;)
                end
                local.get 3
                local.get 1
                i32.eq
                br_if 4 (;@2;)
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        local.get 0
                        local.get 3
                        i32.add
                        local.tee 2
                        i32.load8_s
                        local.tee 1
                        i32.const -1
                        i32.gt_s
                        br_if 0 (;@10;)
                        local.get 2
                        i32.load8_u offset=1
                        i32.const 63
                        i32.and
                        local.set 0
                        local.get 1
                        i32.const 31
                        i32.and
                        local.set 5
                        local.get 1
                        i32.const -33
                        i32.gt_u
                        br_if 1 (;@9;)
                        local.get 5
                        i32.const 6
                        i32.shl
                        local.get 0
                        i32.or
                        local.set 2
                        br 2 (;@8;)
                      end
                      local.get 4
                      local.get 1
                      i32.const 255
                      i32.and
                      i32.store offset=36
                      i32.const 1
                      local.set 1
                      br 2 (;@7;)
                    end
                    local.get 0
                    i32.const 6
                    i32.shl
                    local.get 2
                    i32.load8_u offset=2
                    i32.const 63
                    i32.and
                    i32.or
                    local.set 0
                    block  ;; label = @9
                      local.get 1
                      i32.const -16
                      i32.ge_u
                      br_if 0 (;@9;)
                      local.get 0
                      local.get 5
                      i32.const 12
                      i32.shl
                      i32.or
                      local.set 2
                      br 1 (;@8;)
                    end
                    local.get 0
                    i32.const 6
                    i32.shl
                    local.get 2
                    i32.load8_u offset=3
                    i32.const 63
                    i32.and
                    i32.or
                    local.get 5
                    i32.const 18
                    i32.shl
                    i32.const 1835008
                    i32.and
                    i32.or
                    local.tee 2
                    i32.const 1114112
                    i32.eq
                    br_if 6 (;@2;)
                  end
                  local.get 4
                  local.get 2
                  i32.store offset=36
                  i32.const 1
                  local.set 1
                  local.get 2
                  i32.const 128
                  i32.lt_u
                  br_if 0 (;@7;)
                  i32.const 2
                  local.set 1
                  local.get 2
                  i32.const 2048
                  i32.lt_u
                  br_if 0 (;@7;)
                  i32.const 3
                  i32.const 4
                  local.get 2
                  i32.const 65536
                  i32.lt_u
                  select
                  local.set 1
                end
                local.get 4
                local.get 3
                i32.store offset=40
                local.get 4
                local.get 1
                local.get 3
                i32.add
                i32.store offset=44
                local.get 4
                i32.const 48
                i32.add
                i32.const 20
                i32.add
                i32.const 5
                i32.store
                local.get 4
                i32.const 108
                i32.add
                i32.const 68
                i32.store
                local.get 4
                i32.const 100
                i32.add
                i32.const 68
                i32.store
                local.get 4
                i32.const 72
                i32.add
                i32.const 20
                i32.add
                i32.const 69
                i32.store
                local.get 4
                i32.const 84
                i32.add
                i32.const 70
                i32.store
                local.get 4
                i64.const 5
                i64.store offset=52 align=4
                local.get 4
                i32.const 1056704
                i32.store offset=48
                local.get 4
                i32.const 7
                i32.store offset=76
                local.get 4
                local.get 4
                i32.const 72
                i32.add
                i32.store offset=64
                local.get 4
                local.get 4
                i32.const 24
                i32.add
                i32.store offset=104
                local.get 4
                local.get 4
                i32.const 16
                i32.add
                i32.store offset=96
                local.get 4
                local.get 4
                i32.const 40
                i32.add
                i32.store offset=88
                local.get 4
                local.get 4
                i32.const 36
                i32.add
                i32.store offset=80
                local.get 4
                local.get 4
                i32.const 32
                i32.add
                i32.store offset=72
                local.get 4
                i32.const 48
                i32.add
                i32.const 1056744
                call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
                unreachable
              end
              local.get 4
              i32.const 100
              i32.add
              i32.const 68
              i32.store
              local.get 4
              i32.const 72
              i32.add
              i32.const 20
              i32.add
              i32.const 68
              i32.store
              local.get 4
              i32.const 84
              i32.add
              i32.const 7
              i32.store
              local.get 4
              i32.const 48
              i32.add
              i32.const 20
              i32.add
              i32.const 4
              i32.store
              local.get 4
              i64.const 4
              i64.store offset=52 align=4
              local.get 4
              i32.const 1056588
              i32.store offset=48
              local.get 4
              i32.const 7
              i32.store offset=76
              local.get 4
              local.get 4
              i32.const 72
              i32.add
              i32.store offset=64
              local.get 4
              local.get 4
              i32.const 24
              i32.add
              i32.store offset=96
              local.get 4
              local.get 4
              i32.const 16
              i32.add
              i32.store offset=88
              local.get 4
              local.get 4
              i32.const 12
              i32.add
              i32.store offset=80
              local.get 4
              local.get 4
              i32.const 8
              i32.add
              i32.store offset=72
              local.get 4
              i32.const 48
              i32.add
              i32.const 1056620
              call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
              unreachable
            end
            local.get 0
            local.get 1
            i32.const 0
            local.get 5
            local.get 4
            call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
            unreachable
          end
          local.get 4
          local.get 2
          local.get 3
          local.get 5
          select
          i32.store offset=40
          local.get 4
          i32.const 48
          i32.add
          i32.const 20
          i32.add
          i32.const 3
          i32.store
          local.get 4
          i32.const 72
          i32.add
          i32.const 20
          i32.add
          i32.const 68
          i32.store
          local.get 4
          i32.const 84
          i32.add
          i32.const 68
          i32.store
          local.get 4
          i64.const 3
          i64.store offset=52 align=4
          local.get 4
          i32.const 1056512
          i32.store offset=48
          local.get 4
          i32.const 7
          i32.store offset=76
          local.get 4
          local.get 4
          i32.const 72
          i32.add
          i32.store offset=64
          local.get 4
          local.get 4
          i32.const 24
          i32.add
          i32.store offset=88
          local.get 4
          local.get 4
          i32.const 16
          i32.add
          i32.store offset=80
          local.get 4
          local.get 4
          i32.const 40
          i32.add
          i32.store offset=72
          local.get 4
          i32.const 48
          i32.add
          i32.const 1056536
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get 3
        local.get 5
        local.get 4
        call $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E
        unreachable
      end
      i32.const 1055352
      i32.const 43
      i32.const 1056636
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get 0
    local.get 1
    local.get 3
    local.get 1
    local.get 4
    call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17he625032f56c99b4fE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    drop
    loop (result i32)  ;; label = @1
      br 0 (;@1;)
    end)
  (func $_ZN4core3ptr102drop_in_place$LT$$RF$core..iter..adapters..copied..Copied$LT$core..slice..iter..Iter$LT$u8$GT$$GT$$GT$17h0ae777612708bf9bE (type 0) (param i32))
  (func $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 2
    i32.const 1
    i32.store8 offset=24
    local.get 2
    local.get 1
    i32.store offset=20
    local.get 2
    local.get 0
    i32.store offset=16
    local.get 2
    i32.const 1055420
    i32.store offset=12
    local.get 2
    i32.const 1055260
    i32.store offset=8
    local.get 2
    i32.const 8
    i32.add
    call $rust_begin_unwind
    unreachable)
  (func $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
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
    i32.const 7
    i32.store
    local.get 3
    i64.const 2
    i64.store offset=12 align=4
    local.get 3
    i32.const 1055336
    i32.store offset=8
    local.get 3
    i32.const 7
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
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core10intrinsics17const_eval_select17hd6a7319a41be58fcE
    unreachable)
  (func $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core10intrinsics17const_eval_select17h045db9052655b575E
    unreachable)
  (func $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    local.get 0
    i32.load offset=16
    local.set 3
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.load offset=8
                local.tee 4
                i32.const 1
                i32.eq
                br_if 0 (;@6;)
                local.get 3
                i32.const 1
                i32.ne
                br_if 1 (;@5;)
              end
              local.get 3
              i32.const 1
              i32.ne
              br_if 3 (;@2;)
              local.get 1
              local.get 2
              i32.add
              local.set 5
              local.get 0
              i32.const 20
              i32.add
              i32.load
              local.tee 6
              br_if 1 (;@4;)
              i32.const 0
              local.set 7
              local.get 1
              local.set 8
              br 2 (;@3;)
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
            call_indirect (type 5)
            local.set 3
            br 3 (;@1;)
          end
          i32.const 0
          local.set 7
          local.get 1
          local.set 8
          loop  ;; label = @4
            local.get 8
            local.tee 3
            local.get 5
            i32.eq
            br_if 2 (;@2;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 3
                i32.load8_s
                local.tee 8
                i32.const -1
                i32.le_s
                br_if 0 (;@6;)
                local.get 3
                i32.const 1
                i32.add
                local.set 8
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 8
                i32.const -32
                i32.ge_u
                br_if 0 (;@6;)
                local.get 3
                i32.const 2
                i32.add
                local.set 8
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 8
                i32.const -16
                i32.ge_u
                br_if 0 (;@6;)
                local.get 3
                i32.const 3
                i32.add
                local.set 8
                br 1 (;@5;)
              end
              local.get 3
              i32.load8_u offset=2
              i32.const 63
              i32.and
              i32.const 6
              i32.shl
              local.get 3
              i32.load8_u offset=1
              i32.const 63
              i32.and
              i32.const 12
              i32.shl
              i32.or
              local.get 3
              i32.load8_u offset=3
              i32.const 63
              i32.and
              i32.or
              local.get 8
              i32.const 255
              i32.and
              i32.const 18
              i32.shl
              i32.const 1835008
              i32.and
              i32.or
              i32.const 1114112
              i32.eq
              br_if 3 (;@2;)
              local.get 3
              i32.const 4
              i32.add
              local.set 8
            end
            local.get 7
            local.get 3
            i32.sub
            local.get 8
            i32.add
            local.set 7
            local.get 6
            i32.const -1
            i32.add
            local.tee 6
            br_if 0 (;@4;)
          end
        end
        local.get 8
        local.get 5
        i32.eq
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 8
          i32.load8_s
          local.tee 3
          i32.const -1
          i32.gt_s
          br_if 0 (;@3;)
          local.get 3
          i32.const -32
          i32.lt_u
          br_if 0 (;@3;)
          local.get 3
          i32.const -16
          i32.lt_u
          br_if 0 (;@3;)
          local.get 8
          i32.load8_u offset=2
          i32.const 63
          i32.and
          i32.const 6
          i32.shl
          local.get 8
          i32.load8_u offset=1
          i32.const 63
          i32.and
          i32.const 12
          i32.shl
          i32.or
          local.get 8
          i32.load8_u offset=3
          i32.const 63
          i32.and
          i32.or
          local.get 3
          i32.const 255
          i32.and
          i32.const 18
          i32.shl
          i32.const 1835008
          i32.and
          i32.or
          i32.const 1114112
          i32.eq
          br_if 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 7
              br_if 0 (;@5;)
              i32.const 0
              local.set 8
              br 1 (;@4;)
            end
            block  ;; label = @5
              local.get 7
              local.get 2
              i32.lt_u
              br_if 0 (;@5;)
              i32.const 0
              local.set 3
              local.get 2
              local.set 8
              local.get 7
              local.get 2
              i32.eq
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            i32.const 0
            local.set 3
            local.get 7
            local.set 8
            local.get 1
            local.get 7
            i32.add
            i32.load8_s
            i32.const -64
            i32.lt_s
            br_if 1 (;@3;)
          end
          local.get 8
          local.set 7
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
      block  ;; label = @2
        local.get 4
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
        call_indirect (type 5)
        return
      end
      local.get 0
      i32.const 12
      i32.add
      i32.load
      local.set 5
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.const 16
          i32.lt_u
          br_if 0 (;@3;)
          local.get 1
          local.get 2
          call $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E
          local.set 8
          br 1 (;@2;)
        end
        block  ;; label = @3
          local.get 2
          br_if 0 (;@3;)
          i32.const 0
          local.set 8
          br 1 (;@2;)
        end
        local.get 2
        i32.const 3
        i32.and
        local.set 7
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.const -1
            i32.add
            i32.const 3
            i32.ge_u
            br_if 0 (;@4;)
            i32.const 0
            local.set 8
            local.get 1
            local.set 3
            br 1 (;@3;)
          end
          local.get 2
          i32.const -4
          i32.and
          local.set 6
          i32.const 0
          local.set 8
          local.get 1
          local.set 3
          loop  ;; label = @4
            local.get 8
            local.get 3
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 3
            i32.const 1
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 3
            i32.const 2
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 3
            i32.const 3
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set 8
            local.get 3
            i32.const 4
            i32.add
            local.set 3
            local.get 6
            i32.const -4
            i32.add
            local.tee 6
            br_if 0 (;@4;)
          end
        end
        local.get 7
        i32.eqz
        br_if 0 (;@2;)
        loop  ;; label = @3
          local.get 8
          local.get 3
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set 8
          local.get 3
          i32.const 1
          i32.add
          local.set 3
          local.get 7
          i32.const -1
          i32.add
          local.tee 7
          br_if 0 (;@3;)
        end
      end
      block  ;; label = @2
        local.get 5
        local.get 8
        i32.le_u
        br_if 0 (;@2;)
        i32.const 0
        local.set 3
        local.get 5
        local.get 8
        i32.sub
        local.tee 7
        local.set 6
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              i32.const 0
              local.get 0
              i32.load8_u offset=32
              local.tee 8
              local.get 8
              i32.const 3
              i32.eq
              select
              i32.const 3
              i32.and
              br_table 2 (;@3;) 0 (;@5;) 1 (;@4;) 2 (;@3;)
            end
            i32.const 0
            local.set 6
            local.get 7
            local.set 3
            br 1 (;@3;)
          end
          local.get 7
          i32.const 1
          i32.shr_u
          local.set 3
          local.get 7
          i32.const 1
          i32.add
          i32.const 1
          i32.shr_u
          local.set 6
        end
        local.get 3
        i32.const 1
        i32.add
        local.set 3
        local.get 0
        i32.const 28
        i32.add
        i32.load
        local.set 7
        local.get 0
        i32.load offset=4
        local.set 8
        local.get 0
        i32.load offset=24
        local.set 0
        block  ;; label = @3
          loop  ;; label = @4
            local.get 3
            i32.const -1
            i32.add
            local.tee 3
            i32.eqz
            br_if 1 (;@3;)
            local.get 0
            local.get 8
            local.get 7
            i32.load offset=16
            call_indirect (type 3)
            i32.eqz
            br_if 0 (;@4;)
          end
          i32.const 1
          return
        end
        i32.const 1
        local.set 3
        local.get 8
        i32.const 1114112
        i32.eq
        br_if 1 (;@1;)
        local.get 0
        local.get 1
        local.get 2
        local.get 7
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        i32.const 0
        local.set 3
        loop  ;; label = @3
          block  ;; label = @4
            local.get 6
            local.get 3
            i32.ne
            br_if 0 (;@4;)
            local.get 6
            local.get 6
            i32.lt_u
            return
          end
          local.get 3
          i32.const 1
          i32.add
          local.set 3
          local.get 0
          local.get 8
          local.get 7
          i32.load offset=16
          call_indirect (type 3)
          i32.eqz
          br_if 0 (;@3;)
        end
        local.get 3
        i32.const -1
        i32.add
        local.get 6
        i32.lt_u
        return
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
      call_indirect (type 5)
      return
    end
    local.get 3)
  (func $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get 3
    i32.const 1055260
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
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core10intrinsics17const_eval_select17h61c8e8fbcbf6b317E
    unreachable)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E (type 3) (param i32 i32) (result i32)
    local.get 0
    i64.load32_u
    i32.const 1
    local.get 1
    call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E)
  (func $_ZN4core10intrinsics17const_eval_select17h045db9052655b575E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core3ops8function6FnOnce9call_once17h32ae774a248ad9dfE
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17h5024e92cfa62bd24E (type 0) (param i32)
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    local.get 0
    i32.load offset=8
    local.get 0
    i32.load offset=12
    call $_ZN4core3ops8function6FnOnce9call_once17hcb988a1f0067a302E
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17h61c8e8fbcbf6b317E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core3ops8function6FnOnce9call_once17h3754f24dd73f738cE
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17hd6a7319a41be58fcE (type 2) (param i32 i32)
    local.get 0
    local.get 1
    call $_ZN4core3ops8function6FnOnce9call_once17ha3f2b2a4e66a4793E
    unreachable)
  (func $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
              local.get 3
              i32.const 32
              i32.and
              br_if 1 (;@4;)
              local.get 0
              i64.load32_u
              i32.const 1
              local.get 1
              call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E
              local.set 0
              br 4 (;@1;)
            end
            local.get 0
            i32.load
            local.set 0
            i32.const 0
            local.set 3
            loop  ;; label = @5
              local.get 2
              local.get 3
              i32.add
              i32.const 127
              i32.add
              i32.const 48
              i32.const 87
              local.get 0
              i32.const 15
              i32.and
              local.tee 4
              i32.const 10
              i32.lt_u
              select
              local.get 4
              i32.add
              i32.store8
              local.get 3
              i32.const -1
              i32.add
              local.set 3
              local.get 0
              i32.const 15
              i32.gt_u
              local.set 4
              local.get 0
              i32.const 4
              i32.shr_u
              local.set 0
              local.get 4
              br_if 0 (;@5;)
            end
            local.get 3
            i32.const 128
            i32.add
            local.tee 0
            i32.const 129
            i32.ge_u
            br_if 1 (;@3;)
            local.get 1
            i32.const 1
            i32.const 1055716
            i32.const 2
            local.get 2
            local.get 3
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get 3
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
            local.set 0
            br 3 (;@1;)
          end
          local.get 0
          i32.load
          local.set 0
          i32.const 0
          local.set 3
          loop  ;; label = @4
            local.get 2
            local.get 3
            i32.add
            i32.const 127
            i32.add
            i32.const 48
            i32.const 55
            local.get 0
            i32.const 15
            i32.and
            local.tee 4
            i32.const 10
            i32.lt_u
            select
            local.get 4
            i32.add
            i32.store8
            local.get 3
            i32.const -1
            i32.add
            local.set 3
            local.get 0
            i32.const 15
            i32.gt_u
            local.set 4
            local.get 0
            i32.const 4
            i32.shr_u
            local.set 0
            local.get 4
            br_if 0 (;@4;)
          end
          local.get 3
          i32.const 128
          i32.add
          local.tee 0
          i32.const 129
          i32.ge_u
          br_if 1 (;@2;)
          local.get 1
          i32.const 1
          i32.const 1055716
          i32.const 2
          local.get 2
          local.get 3
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get 3
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
          local.set 0
          br 2 (;@1;)
        end
        local.get 0
        i32.const 128
        local.get 0
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get 0
      i32.const 128
      local.get 0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 2
    i32.const 128
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt5write17ha48fcb73fabd1d51E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
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
    i32.const 0
    local.set 4
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
            local.get 2
            i32.load offset=8
            local.tee 5
            br_if 0 (;@4;)
            local.get 2
            i32.const 20
            i32.add
            i32.load
            local.tee 6
            i32.eqz
            br_if 1 (;@3;)
            local.get 2
            i32.load
            local.set 1
            local.get 2
            i32.load offset=16
            local.set 0
            local.get 6
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.tee 4
            local.set 6
            loop  ;; label = @5
              block  ;; label = @6
                local.get 1
                i32.const 4
                i32.add
                i32.load
                local.tee 7
                i32.eqz
                br_if 0 (;@6;)
                local.get 3
                i32.load offset=32
                local.get 1
                i32.load
                local.get 7
                local.get 3
                i32.load offset=36
                i32.load offset=12
                call_indirect (type 5)
                br_if 4 (;@2;)
              end
              local.get 0
              i32.load
              local.get 3
              i32.const 8
              i32.add
              local.get 0
              i32.const 4
              i32.add
              i32.load
              call_indirect (type 3)
              br_if 3 (;@2;)
              local.get 0
              i32.const 8
              i32.add
              local.set 0
              local.get 1
              i32.const 8
              i32.add
              local.set 1
              local.get 6
              i32.const -1
              i32.add
              local.tee 6
              br_if 0 (;@5;)
              br 2 (;@3;)
            end
          end
          local.get 2
          i32.const 12
          i32.add
          i32.load
          local.tee 0
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.const 5
          i32.shl
          local.set 8
          local.get 0
          i32.const -1
          i32.add
          i32.const 134217727
          i32.and
          i32.const 1
          i32.add
          local.set 4
          local.get 2
          i32.load
          local.set 1
          i32.const 0
          local.set 6
          loop  ;; label = @4
            block  ;; label = @5
              local.get 1
              i32.const 4
              i32.add
              i32.load
              local.tee 0
              i32.eqz
              br_if 0 (;@5;)
              local.get 3
              i32.load offset=32
              local.get 1
              i32.load
              local.get 0
              local.get 3
              i32.load offset=36
              i32.load offset=12
              call_indirect (type 5)
              br_if 3 (;@2;)
            end
            local.get 3
            local.get 5
            local.get 6
            i32.add
            local.tee 0
            i32.const 28
            i32.add
            i32.load8_u
            i32.store8 offset=40
            local.get 3
            local.get 0
            i32.const 4
            i32.add
            i64.load align=4
            i64.const 32
            i64.rotl
            i64.store offset=8
            local.get 0
            i32.const 24
            i32.add
            i32.load
            local.set 9
            local.get 2
            i32.load offset=16
            local.set 10
            i32.const 0
            local.set 11
            i32.const 0
            local.set 7
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 0
                  i32.const 20
                  i32.add
                  i32.load
                  br_table 1 (;@6;) 0 (;@7;) 2 (;@5;) 1 (;@6;)
                end
                local.get 9
                i32.const 3
                i32.shl
                local.set 12
                i32.const 0
                local.set 7
                local.get 10
                local.get 12
                i32.add
                local.tee 12
                i32.load offset=4
                i32.const 71
                i32.ne
                br_if 1 (;@5;)
                local.get 12
                i32.load
                i32.load
                local.set 9
              end
              i32.const 1
              local.set 7
            end
            local.get 3
            local.get 9
            i32.store offset=20
            local.get 3
            local.get 7
            i32.store offset=16
            local.get 0
            i32.const 16
            i32.add
            i32.load
            local.set 7
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 0
                  i32.const 12
                  i32.add
                  i32.load
                  br_table 1 (;@6;) 0 (;@7;) 2 (;@5;) 1 (;@6;)
                end
                local.get 7
                i32.const 3
                i32.shl
                local.set 9
                local.get 10
                local.get 9
                i32.add
                local.tee 9
                i32.load offset=4
                i32.const 71
                i32.ne
                br_if 1 (;@5;)
                local.get 9
                i32.load
                i32.load
                local.set 7
              end
              i32.const 1
              local.set 11
            end
            local.get 3
            local.get 7
            i32.store offset=28
            local.get 3
            local.get 11
            i32.store offset=24
            local.get 10
            local.get 0
            i32.load
            i32.const 3
            i32.shl
            i32.add
            local.tee 0
            i32.load
            local.get 3
            i32.const 8
            i32.add
            local.get 0
            i32.load offset=4
            call_indirect (type 3)
            br_if 2 (;@2;)
            local.get 1
            i32.const 8
            i32.add
            local.set 1
            local.get 8
            local.get 6
            i32.const 32
            i32.add
            local.tee 6
            i32.ne
            br_if 0 (;@4;)
          end
        end
        i32.const 0
        local.set 0
        local.get 4
        local.get 2
        i32.load offset=4
        i32.lt_u
        local.tee 1
        i32.eqz
        br_if 1 (;@1;)
        local.get 3
        i32.load offset=32
        local.get 2
        i32.load
        local.get 4
        i32.const 3
        i32.shl
        i32.add
        i32.const 0
        local.get 1
        select
        local.tee 1
        i32.load
        local.get 1
        i32.load offset=4
        local.get 3
        i32.load offset=36
        i32.load offset=12
        call_indirect (type 5)
        i32.eqz
        br_if 1 (;@1;)
      end
      i32.const 1
      local.set 0
    end
    local.get 3
    i32.const 48
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17ha826dd87512c4c00E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    i32.const 1
    local.set 3
    block  ;; label = @1
      local.get 0
      local.get 1
      call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E
      br_if 0 (;@1;)
      local.get 1
      i32.const 28
      i32.add
      i32.load
      local.set 4
      local.get 1
      i32.load offset=24
      local.set 5
      local.get 2
      i32.const 28
      i32.add
      i32.const 0
      i32.store
      local.get 2
      i32.const 1055260
      i32.store offset=24
      local.get 2
      i64.const 1
      i64.store offset=12 align=4
      local.get 2
      i32.const 1055264
      i32.store offset=8
      local.get 5
      local.get 4
      local.get 2
      i32.const 8
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      br_if 0 (;@1;)
      local.get 0
      i32.const 4
      i32.add
      local.get 1
      call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E
      local.set 3
    end
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 3)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hd78f961f99573a2cE (type 1) (param i32) (result i64)
    i64.const -887290134024487584)
  (func $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h651b426602cda903E (type 3) (param i32 i32) (result i32)
    local.get 1
    i32.load offset=24
    i32.const 1055272
    i32.const 14
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 5))
  (func $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E (type 10) (param i32) (result i32)
    (local i32 i32)
    i32.const 1114112
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 0
              i32.load
              br_table 3 (;@2;) 0 (;@5;) 1 (;@4;) 2 (;@3;) 3 (;@2;)
            end
            local.get 0
            i32.const 0
            i32.store
            local.get 0
            i32.load offset=4
            return
          end
          local.get 0
          i32.const 1
          i32.store
          i32.const 92
          return
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 0
                  i32.const 12
                  i32.add
                  i32.load8_u
                  br_table 5 (;@2;) 0 (;@7;) 4 (;@3;) 1 (;@6;) 2 (;@5;) 3 (;@4;) 5 (;@2;)
                end
                local.get 0
                i32.const 0
                i32.store8 offset=12
                i32.const 125
                return
              end
              local.get 0
              i32.const 2
              i32.store8 offset=12
              i32.const 123
              return
            end
            local.get 0
            i32.const 3
            i32.store8 offset=12
            i32.const 117
            return
          end
          local.get 0
          i32.const 4
          i32.store8 offset=12
          i32.const 92
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
        local.tee 2
        i32.const 2
        i32.shl
        i32.shr_u
        i32.const 15
        i32.and
        local.tee 1
        i32.const 10
        i32.lt_u
        select
        local.get 1
        i32.add
        local.set 1
        local.get 2
        i32.eqz
        br_if 1 (;@1;)
        local.get 0
        local.get 2
        i32.const -1
        i32.add
        i32.store offset=8
      end
      local.get 1
      return
    end
    local.get 0
    i32.const 1
    i32.store8 offset=12
    local.get 1)
  (func $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.const 3
            i32.add
            i32.const -4
            i32.and
            local.get 2
            i32.sub
            local.tee 4
            i32.eqz
            br_if 0 (;@4;)
            local.get 3
            local.get 4
            local.get 4
            local.get 3
            i32.gt_u
            select
            local.tee 4
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 5
            local.get 1
            i32.const 255
            i32.and
            local.set 6
            i32.const 1
            local.set 7
            loop  ;; label = @5
              local.get 2
              local.get 5
              i32.add
              i32.load8_u
              local.get 6
              i32.eq
              br_if 4 (;@1;)
              local.get 4
              local.get 5
              i32.const 1
              i32.add
              local.tee 5
              i32.ne
              br_if 0 (;@5;)
            end
            local.get 4
            local.get 3
            i32.const -8
            i32.add
            local.tee 8
            i32.gt_u
            br_if 2 (;@2;)
            br 1 (;@3;)
          end
          local.get 3
          i32.const -8
          i32.add
          local.set 8
          i32.const 0
          local.set 4
        end
        local.get 1
        i32.const 255
        i32.and
        i32.const 16843009
        i32.mul
        local.set 5
        block  ;; label = @3
          loop  ;; label = @4
            local.get 2
            local.get 4
            i32.add
            local.tee 6
            i32.load
            local.get 5
            i32.xor
            local.tee 7
            i32.const -1
            i32.xor
            local.get 7
            i32.const -16843009
            i32.add
            i32.and
            local.get 6
            i32.const 4
            i32.add
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
            br_if 1 (;@3;)
            local.get 4
            i32.const 8
            i32.add
            local.tee 4
            local.get 8
            i32.le_u
            br_if 0 (;@4;)
          end
        end
        local.get 4
        local.get 3
        i32.le_u
        br_if 0 (;@2;)
        local.get 4
        local.get 3
        local.get 4
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      block  ;; label = @2
        local.get 4
        local.get 3
        i32.eq
        br_if 0 (;@2;)
        local.get 4
        local.get 3
        i32.sub
        local.set 8
        local.get 2
        local.get 4
        i32.add
        local.set 6
        i32.const 0
        local.set 5
        local.get 1
        i32.const 255
        i32.and
        local.set 7
        block  ;; label = @3
          loop  ;; label = @4
            local.get 6
            local.get 5
            i32.add
            i32.load8_u
            local.get 7
            i32.eq
            br_if 1 (;@3;)
            local.get 8
            local.get 5
            i32.const 1
            i32.add
            local.tee 5
            i32.add
            i32.eqz
            br_if 2 (;@2;)
            br 0 (;@4;)
          end
        end
        local.get 4
        local.get 5
        i32.add
        local.set 5
        i32.const 1
        local.set 7
        br 1 (;@1;)
      end
      i32.const 0
      local.set 7
    end
    local.get 0
    local.get 5
    i32.store offset=4
    local.get 0
    local.get 7
    i32.store)
  (func $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i64 i64 i32)
    block  ;; label = @1
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
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
      local.get 1
      i32.const 3
      i32.add
      i32.const -4
      i32.and
      local.get 1
      i32.sub
      local.set 5
      i32.const 0
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              loop  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      local.get 1
                      local.get 3
                      i32.add
                      i32.load8_u
                      local.tee 6
                      i32.const 24
                      i32.shl
                      i32.const 24
                      i32.shr_s
                      local.tee 7
                      i32.const 0
                      i32.lt_s
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const -1
                      i32.eq
                      br_if 1 (;@8;)
                      local.get 5
                      local.get 3
                      i32.sub
                      i32.const 3
                      i32.and
                      br_if 1 (;@8;)
                      block  ;; label = @10
                        local.get 3
                        local.get 4
                        i32.ge_u
                        br_if 0 (;@10;)
                        loop  ;; label = @11
                          local.get 1
                          local.get 3
                          i32.add
                          local.tee 6
                          i32.load
                          local.get 6
                          i32.const 4
                          i32.add
                          i32.load
                          i32.or
                          i32.const -2139062144
                          i32.and
                          br_if 1 (;@10;)
                          local.get 3
                          i32.const 8
                          i32.add
                          local.tee 3
                          local.get 4
                          i32.lt_u
                          br_if 0 (;@11;)
                        end
                      end
                      local.get 3
                      local.get 2
                      i32.ge_u
                      br_if 2 (;@7;)
                      loop  ;; label = @10
                        local.get 1
                        local.get 3
                        i32.add
                        i32.load8_s
                        i32.const 0
                        i32.lt_s
                        br_if 3 (;@7;)
                        local.get 2
                        local.get 3
                        i32.const 1
                        i32.add
                        local.tee 3
                        i32.ne
                        br_if 0 (;@10;)
                        br 9 (;@1;)
                      end
                    end
                    i64.const 1099511627776
                    local.set 8
                    i64.const 4294967296
                    local.set 9
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      local.get 6
                                      i32.const 1056188
                                      i32.add
                                      i32.load8_u
                                      i32.const -2
                                      i32.add
                                      br_table 0 (;@17;) 1 (;@16;) 2 (;@15;) 15 (;@2;)
                                    end
                                    local.get 3
                                    i32.const 1
                                    i32.add
                                    local.tee 6
                                    local.get 2
                                    i32.lt_u
                                    br_if 6 (;@10;)
                                    i64.const 0
                                    local.set 8
                                    br 13 (;@3;)
                                  end
                                  i64.const 0
                                  local.set 8
                                  local.get 3
                                  i32.const 1
                                  i32.add
                                  local.tee 10
                                  local.get 2
                                  i32.ge_u
                                  br_if 12 (;@3;)
                                  local.get 1
                                  local.get 10
                                  i32.add
                                  i32.load8_s
                                  local.set 10
                                  local.get 6
                                  i32.const -224
                                  i32.add
                                  br_table 1 (;@14;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 3 (;@12;) 2 (;@13;) 3 (;@12;)
                                end
                                i64.const 0
                                local.set 8
                                local.get 3
                                i32.const 1
                                i32.add
                                local.tee 10
                                local.get 2
                                i32.ge_u
                                br_if 11 (;@3;)
                                local.get 1
                                local.get 10
                                i32.add
                                i32.load8_s
                                local.set 10
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        local.get 6
                                        i32.const -240
                                        i32.add
                                        br_table 1 (;@17;) 0 (;@18;) 0 (;@18;) 0 (;@18;) 2 (;@16;) 0 (;@18;)
                                      end
                                      local.get 7
                                      i32.const 15
                                      i32.add
                                      i32.const 255
                                      i32.and
                                      i32.const 2
                                      i32.gt_u
                                      br_if 13 (;@4;)
                                      local.get 10
                                      i32.const -1
                                      i32.gt_s
                                      br_if 13 (;@4;)
                                      local.get 10
                                      i32.const -64
                                      i32.ge_u
                                      br_if 13 (;@4;)
                                      br 2 (;@15;)
                                    end
                                    local.get 10
                                    i32.const 112
                                    i32.add
                                    i32.const 255
                                    i32.and
                                    i32.const 48
                                    i32.ge_u
                                    br_if 12 (;@4;)
                                    br 1 (;@15;)
                                  end
                                  local.get 10
                                  i32.const -113
                                  i32.gt_s
                                  br_if 11 (;@4;)
                                end
                                local.get 3
                                i32.const 2
                                i32.add
                                local.tee 6
                                local.get 2
                                i32.ge_u
                                br_if 11 (;@3;)
                                local.get 1
                                local.get 6
                                i32.add
                                i32.load8_s
                                i32.const -65
                                i32.gt_s
                                br_if 9 (;@5;)
                                i64.const 0
                                local.set 9
                                local.get 3
                                i32.const 3
                                i32.add
                                local.tee 6
                                local.get 2
                                i32.ge_u
                                br_if 12 (;@2;)
                                local.get 1
                                local.get 6
                                i32.add
                                i32.load8_s
                                i32.const -65
                                i32.le_s
                                br_if 5 (;@9;)
                                i64.const 3298534883328
                                local.set 8
                                i64.const 4294967296
                                local.set 9
                                br 12 (;@2;)
                              end
                              local.get 10
                              i32.const -32
                              i32.and
                              i32.const -96
                              i32.ne
                              br_if 9 (;@4;)
                              br 2 (;@11;)
                            end
                            local.get 10
                            i32.const -96
                            i32.ge_s
                            br_if 8 (;@4;)
                            br 1 (;@11;)
                          end
                          block  ;; label = @12
                            local.get 7
                            i32.const 31
                            i32.add
                            i32.const 255
                            i32.and
                            i32.const 12
                            i32.lt_u
                            br_if 0 (;@12;)
                            local.get 7
                            i32.const -2
                            i32.and
                            i32.const -18
                            i32.ne
                            br_if 8 (;@4;)
                            local.get 10
                            i32.const -1
                            i32.gt_s
                            br_if 8 (;@4;)
                            local.get 10
                            i32.const -64
                            i32.ge_u
                            br_if 8 (;@4;)
                            br 1 (;@11;)
                          end
                          local.get 10
                          i32.const -65
                          i32.gt_s
                          br_if 7 (;@4;)
                        end
                        i64.const 0
                        local.set 9
                        local.get 3
                        i32.const 2
                        i32.add
                        local.tee 6
                        local.get 2
                        i32.ge_u
                        br_if 8 (;@2;)
                        local.get 1
                        local.get 6
                        i32.add
                        i32.load8_s
                        i32.const -65
                        i32.gt_s
                        br_if 5 (;@5;)
                        br 1 (;@9;)
                      end
                      i64.const 1099511627776
                      local.set 8
                      i64.const 4294967296
                      local.set 9
                      local.get 1
                      local.get 6
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.gt_s
                      br_if 7 (;@2;)
                    end
                    local.get 6
                    i32.const 1
                    i32.add
                    local.set 3
                    br 1 (;@7;)
                  end
                  local.get 3
                  i32.const 1
                  i32.add
                  local.set 3
                end
                local.get 3
                local.get 2
                i32.lt_u
                br_if 0 (;@6;)
                br 5 (;@1;)
              end
            end
            i64.const 2199023255552
            local.set 8
            i64.const 4294967296
            local.set 9
            br 2 (;@2;)
          end
          i64.const 1099511627776
          local.set 8
          i64.const 4294967296
          local.set 9
          br 1 (;@2;)
        end
        i64.const 0
        local.set 9
      end
      local.get 0
      local.get 8
      local.get 3
      i64.extend_i32_u
      i64.or
      local.get 9
      i64.or
      i64.store offset=4 align=4
      local.get 0
      i32.const 1
      i32.store
      return
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
    i32.store)
  (func $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE (type 12) (param i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i64 i64)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 5
    global.set $__stack_pointer
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
        i32.load
        local.tee 9
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 6
        local.get 8
        i32.load offset=24
        i32.const 1055669
        i32.const 1055671
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
        call_indirect (type 5)
        br_if 1 (;@1;)
        i32.const 1
        local.set 6
        local.get 8
        i32.load offset=24
        local.get 1
        local.get 2
        local.get 8
        i32.load offset=28
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        i32.const 1
        local.set 6
        local.get 8
        i32.load offset=24
        i32.const 1055616
        i32.const 2
        local.get 8
        i32.load offset=28
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        local.get 3
        local.get 8
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
        i32.const 1055664
        i32.const 3
        local.get 8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        local.get 8
        i32.load
        local.set 9
      end
      i32.const 1
      local.set 6
      local.get 5
      i32.const 1
      i32.store8 offset=23
      local.get 5
      i32.const 52
      i32.add
      i32.const 1055636
      i32.store
      local.get 5
      i32.const 16
      i32.add
      local.get 5
      i32.const 23
      i32.add
      i32.store
      local.get 5
      local.get 9
      i32.store offset=24
      local.get 5
      local.get 8
      i64.load offset=24 align=4
      i64.store offset=8
      local.get 8
      i64.load offset=8 align=4
      local.set 10
      local.get 8
      i64.load offset=16 align=4
      local.set 11
      local.get 5
      local.get 8
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get 5
      local.get 8
      i32.load offset=4
      i32.store offset=28
      local.get 5
      local.get 11
      i64.store offset=40
      local.get 5
      local.get 10
      i64.store offset=32
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
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
      br_if 0 (;@1;)
      local.get 5
      i32.const 8
      i32.add
      i32.const 1055616
      i32.const 2
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
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
      i32.const 1055667
      i32.const 2
      local.get 5
      i32.load offset=52
      i32.load offset=12
      call_indirect (type 5)
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
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core6option13expect_failed17h8d17c0af9e73185dE (type 4) (param i32 i32 i32)
    local.get 0
    local.get 1
    local.get 2
    call $_ZN4core9panicking9panic_str17h62305f3b2d2b36d0E
    unreachable)
  (func $_ZN4core9panicking9panic_str17h62305f3b2d2b36d0E (type 4) (param i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    local.get 3
    local.get 1
    i32.store offset=12
    local.get 3
    local.get 0
    i32.store offset=8
    local.get 3
    i32.const 8
    i32.add
    local.get 2
    call $_ZN4core9panicking13panic_display17hba1c2a50fab5b809E
    unreachable)
  (func $_ZN70_$LT$core..panic..location..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h679c135f41a338baE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 2
    i32.const 20
    i32.add
    i32.const 7
    i32.store
    local.get 2
    i32.const 12
    i32.add
    i32.const 7
    i32.store
    local.get 2
    i32.const 68
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
    i32.const 1055396
    i32.store offset=24
    local.get 2
    local.get 2
    i32.store offset=40
    local.get 1
    local.get 0
    local.get 2
    i32.const 24
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 0
    local.get 2
    i32.const 48
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h8790479cff165904E (type 3) (param i32 i32) (result i32)
    local.get 1
    local.get 0
    i32.load
    local.get 0
    i32.load offset=4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E (type 2) (param i32 i32)
    local.get 0
    local.get 1
    i64.load align=4
    i64.store)
  (func $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E (type 10) (param i32) (result i32)
    local.get 0
    i32.load offset=8)
  (func $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE (type 10) (param i32) (result i32)
    local.get 0
    i32.load offset=12)
  (func $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE (type 10) (param i32) (result i32)
    local.get 0
    i32.load8_u offset=16)
  (func $_ZN73_$LT$core..panic..panic_info..PanicInfo$u20$as$u20$core..fmt..Display$GT$3fmt17h92a87928d361b08eE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    i32.const 1
    local.set 3
    block  ;; label = @1
      local.get 1
      i32.load offset=24
      local.tee 4
      i32.const 1055436
      i32.const 12
      local.get 1
      i32.const 28
      i32.add
      i32.load
      local.tee 1
      i32.load offset=12
      call_indirect (type 5)
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          local.get 0
          i32.load offset=8
          local.tee 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 2
          local.get 3
          i32.store offset=12
          local.get 2
          i32.const 72
          i32.store offset=20
          local.get 2
          local.get 2
          i32.const 12
          i32.add
          i32.store offset=16
          i32.const 1
          local.set 3
          local.get 2
          i32.const 60
          i32.add
          i32.const 1
          i32.store
          local.get 2
          i64.const 2
          i64.store offset=44 align=4
          local.get 2
          i32.const 1055452
          i32.store offset=40
          local.get 2
          local.get 2
          i32.const 16
          i32.add
          i32.store offset=56
          local.get 4
          local.get 1
          local.get 2
          i32.const 40
          i32.add
          call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
          i32.eqz
          br_if 1 (;@2;)
          br 2 (;@1;)
        end
        local.get 0
        i32.load
        local.tee 3
        local.get 0
        i32.load offset=4
        i32.load offset=12
        call_indirect (type 1)
        i64.const -5139102199292759541
        i64.ne
        br_if 0 (;@2;)
        local.get 2
        local.get 3
        i32.store offset=12
        local.get 2
        i32.const 73
        i32.store offset=20
        local.get 2
        local.get 2
        i32.const 12
        i32.add
        i32.store offset=16
        i32.const 1
        local.set 3
        local.get 2
        i32.const 60
        i32.add
        i32.const 1
        i32.store
        local.get 2
        i64.const 2
        i64.store offset=44 align=4
        local.get 2
        i32.const 1055452
        i32.store offset=40
        local.get 2
        local.get 2
        i32.const 16
        i32.add
        i32.store offset=56
        local.get 4
        local.get 1
        local.get 2
        i32.const 40
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        br_if 1 (;@1;)
      end
      local.get 0
      i32.load offset=12
      local.set 3
      local.get 2
      i32.const 16
      i32.add
      i32.const 20
      i32.add
      i32.const 7
      i32.store
      local.get 2
      i32.const 16
      i32.add
      i32.const 12
      i32.add
      i32.const 7
      i32.store
      local.get 2
      local.get 3
      i32.const 12
      i32.add
      i32.store offset=32
      local.get 2
      local.get 3
      i32.const 8
      i32.add
      i32.store offset=24
      local.get 2
      i32.const 68
      i32.store offset=20
      local.get 2
      local.get 3
      i32.store offset=16
      local.get 2
      i32.const 40
      i32.add
      i32.const 20
      i32.add
      i32.const 3
      i32.store
      local.get 2
      i64.const 3
      i64.store offset=44 align=4
      local.get 2
      i32.const 1055396
      i32.store offset=40
      local.get 2
      local.get 2
      i32.const 16
      i32.add
      i32.store offset=56
      local.get 4
      local.get 1
      local.get 2
      i32.const 40
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      local.set 3
    end
    local.get 2
    i32.const 64
    i32.add
    global.set $__stack_pointer
    local.get 3)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h866d2163a1275370E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 1
    i32.const 28
    i32.add
    i32.load
    local.set 3
    local.get 1
    i32.load offset=24
    local.set 4
    local.get 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get 0
    i32.load
    local.tee 1
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
    local.get 4
    local.get 3
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h0d0f7379d9aa8697E (type 3) (param i32 i32) (result i32)
    local.get 1
    local.get 0
    i32.load
    local.tee 0
    i32.load
    local.get 0
    i32.load offset=4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core9panicking13panic_display17hba1c2a50fab5b809E (type 2) (param i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 2
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get 2
    i64.const 1
    i64.store offset=4 align=4
    local.get 2
    i32.const 1055468
    i32.store
    local.get 2
    i32.const 68
    i32.store offset=28
    local.get 2
    local.get 0
    i32.store offset=24
    local.get 2
    local.get 2
    i32.const 24
    i32.add
    i32.store offset=16
    local.get 2
    local.get 1
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E (type 13) (param i32 i32 i32 i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 112
    i32.sub
    local.tee 7
    global.set $__stack_pointer
    local.get 7
    local.get 2
    i32.store offset=12
    local.get 7
    local.get 1
    i32.store offset=8
    local.get 7
    local.get 4
    i32.store offset=20
    local.get 7
    local.get 3
    i32.store offset=16
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            local.get 0
            i32.const 255
            i32.and
            br_table 0 (;@4;) 1 (;@3;) 2 (;@2;) 0 (;@4;)
          end
          local.get 7
          i32.const 1055485
          i32.store offset=24
          i32.const 2
          local.set 0
          br 2 (;@1;)
        end
        local.get 7
        i32.const 1055483
        i32.store offset=24
        i32.const 2
        local.set 0
        br 1 (;@1;)
      end
      local.get 7
      i32.const 1055476
      i32.store offset=24
      i32.const 7
      local.set 0
    end
    local.get 7
    local.get 0
    i32.store offset=28
    block  ;; label = @1
      local.get 5
      i32.load
      br_if 0 (;@1;)
      local.get 7
      i32.const 56
      i32.add
      i32.const 20
      i32.add
      i32.const 74
      i32.store
      local.get 7
      i32.const 68
      i32.add
      i32.const 74
      i32.store
      local.get 7
      i32.const 88
      i32.add
      i32.const 20
      i32.add
      i32.const 3
      i32.store
      local.get 7
      i64.const 4
      i64.store offset=92 align=4
      local.get 7
      i32.const 1055584
      i32.store offset=88
      local.get 7
      i32.const 68
      i32.store offset=60
      local.get 7
      local.get 7
      i32.const 56
      i32.add
      i32.store offset=104
      local.get 7
      local.get 7
      i32.const 16
      i32.add
      i32.store offset=72
      local.get 7
      local.get 7
      i32.const 8
      i32.add
      i32.store offset=64
      local.get 7
      local.get 7
      i32.const 24
      i32.add
      i32.store offset=56
      local.get 7
      i32.const 88
      i32.add
      local.get 6
      call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
      unreachable
    end
    local.get 7
    i32.const 32
    i32.add
    i32.const 16
    i32.add
    local.get 5
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get 7
    i32.const 32
    i32.add
    i32.const 8
    i32.add
    local.get 5
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get 7
    local.get 5
    i64.load align=4
    i64.store offset=32
    local.get 7
    i32.const 88
    i32.add
    i32.const 20
    i32.add
    i32.const 4
    i32.store
    local.get 7
    i32.const 84
    i32.add
    i32.const 12
    i32.store
    local.get 7
    i32.const 56
    i32.add
    i32.const 20
    i32.add
    i32.const 74
    i32.store
    local.get 7
    i32.const 68
    i32.add
    i32.const 74
    i32.store
    local.get 7
    i64.const 4
    i64.store offset=92 align=4
    local.get 7
    i32.const 1055548
    i32.store offset=88
    local.get 7
    i32.const 68
    i32.store offset=60
    local.get 7
    local.get 7
    i32.const 56
    i32.add
    i32.store offset=104
    local.get 7
    local.get 7
    i32.const 32
    i32.add
    i32.store offset=80
    local.get 7
    local.get 7
    i32.const 16
    i32.add
    i32.store offset=72
    local.get 7
    local.get 7
    i32.const 8
    i32.add
    i32.store offset=64
    local.get 7
    local.get 7
    i32.const 24
    i32.add
    i32.store offset=56
    local.get 7
    i32.const 88
    i32.add
    local.get 6
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hade274a01ebef0eeE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 0
    i32.load offset=4
    i32.load offset=12
    call_indirect (type 3))
  (func $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h5e2128c63f339bb4E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 0
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE (type 11) (param i32 i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 5
    global.set $__stack_pointer
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
    i32.const 74
    i32.store
    local.get 5
    i64.const 2
    i64.store offset=28 align=4
    local.get 5
    i32.const 1055620
    i32.store offset=24
    local.get 5
    i32.const 68
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
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.load offset=4
        local.set 3
        local.get 0
        i32.load
        local.set 4
        local.get 0
        i32.load offset=8
        local.set 5
        loop  ;; label = @3
          block  ;; label = @4
            local.get 5
            i32.load8_u
            i32.eqz
            br_if 0 (;@4;)
            local.get 4
            i32.const 1055660
            i32.const 4
            local.get 3
            i32.load offset=12
            call_indirect (type 5)
            i32.eqz
            br_if 0 (;@4;)
            i32.const 1
            return
          end
          i32.const 0
          local.set 6
          local.get 2
          local.set 7
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  loop  ;; label = @8
                    local.get 1
                    local.get 6
                    i32.add
                    local.set 8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              local.get 7
                              i32.const 8
                              i32.lt_u
                              br_if 0 (;@13;)
                              block  ;; label = @14
                                local.get 8
                                i32.const 3
                                i32.add
                                i32.const -4
                                i32.and
                                local.get 8
                                i32.sub
                                local.tee 0
                                br_if 0 (;@14;)
                                local.get 7
                                i32.const -8
                                i32.add
                                local.set 9
                                i32.const 0
                                local.set 0
                                br 3 (;@11;)
                              end
                              local.get 7
                              local.get 0
                              local.get 0
                              local.get 7
                              i32.gt_u
                              select
                              local.set 0
                              i32.const 0
                              local.set 10
                              loop  ;; label = @14
                                local.get 8
                                local.get 10
                                i32.add
                                i32.load8_u
                                i32.const 10
                                i32.eq
                                br_if 5 (;@9;)
                                local.get 0
                                local.get 10
                                i32.const 1
                                i32.add
                                local.tee 10
                                i32.eq
                                br_if 2 (;@12;)
                                br 0 (;@14;)
                              end
                            end
                            local.get 7
                            i32.eqz
                            br_if 5 (;@7;)
                            i32.const 0
                            local.set 10
                            local.get 8
                            i32.load8_u
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 1
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 1
                            local.set 10
                            local.get 8
                            i32.load8_u offset=1
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 2
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 2
                            local.set 10
                            local.get 8
                            i32.load8_u offset=2
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 3
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 3
                            local.set 10
                            local.get 8
                            i32.load8_u offset=3
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 4
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 4
                            local.set 10
                            local.get 8
                            i32.load8_u offset=4
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 5
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 5
                            local.set 10
                            local.get 8
                            i32.load8_u offset=5
                            i32.const 10
                            i32.eq
                            br_if 3 (;@9;)
                            local.get 7
                            i32.const 6
                            i32.eq
                            br_if 5 (;@7;)
                            i32.const 6
                            local.set 10
                            local.get 8
                            i32.load8_u offset=6
                            i32.const 10
                            i32.ne
                            br_if 5 (;@7;)
                            br 3 (;@9;)
                          end
                          local.get 0
                          local.get 7
                          i32.const -8
                          i32.add
                          local.tee 9
                          i32.gt_u
                          br_if 1 (;@10;)
                        end
                        block  ;; label = @11
                          loop  ;; label = @12
                            local.get 8
                            local.get 0
                            i32.add
                            local.tee 10
                            i32.load
                            local.tee 11
                            i32.const -1
                            i32.xor
                            local.get 11
                            i32.const 168430090
                            i32.xor
                            i32.const -16843009
                            i32.add
                            i32.and
                            local.get 10
                            i32.const 4
                            i32.add
                            i32.load
                            local.tee 10
                            i32.const -1
                            i32.xor
                            local.get 10
                            i32.const 168430090
                            i32.xor
                            i32.const -16843009
                            i32.add
                            i32.and
                            i32.or
                            i32.const -2139062144
                            i32.and
                            br_if 1 (;@11;)
                            local.get 0
                            i32.const 8
                            i32.add
                            local.tee 0
                            local.get 9
                            i32.le_u
                            br_if 0 (;@12;)
                          end
                        end
                        local.get 0
                        local.get 7
                        i32.le_u
                        br_if 0 (;@10;)
                        local.get 0
                        local.get 7
                        local.get 0
                        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
                        unreachable
                      end
                      local.get 0
                      local.get 7
                      i32.eq
                      br_if 2 (;@7;)
                      local.get 0
                      local.get 7
                      i32.sub
                      local.set 11
                      local.get 8
                      local.get 0
                      i32.add
                      local.set 8
                      i32.const 0
                      local.set 10
                      block  ;; label = @10
                        loop  ;; label = @11
                          local.get 8
                          local.get 10
                          i32.add
                          i32.load8_u
                          i32.const 10
                          i32.eq
                          br_if 1 (;@10;)
                          local.get 11
                          local.get 10
                          i32.const 1
                          i32.add
                          local.tee 10
                          i32.add
                          br_if 0 (;@11;)
                          br 4 (;@7;)
                        end
                      end
                      local.get 0
                      local.get 10
                      i32.add
                      local.set 10
                    end
                    block  ;; label = @9
                      local.get 10
                      local.get 6
                      i32.add
                      local.tee 0
                      i32.const 1
                      i32.add
                      local.tee 6
                      local.get 0
                      i32.lt_u
                      br_if 0 (;@9;)
                      local.get 2
                      local.get 6
                      i32.lt_u
                      br_if 0 (;@9;)
                      local.get 1
                      local.get 0
                      i32.add
                      i32.load8_u
                      i32.const 10
                      i32.ne
                      br_if 0 (;@9;)
                      local.get 5
                      i32.const 1
                      i32.store8
                      local.get 2
                      local.get 6
                      i32.le_u
                      br_if 3 (;@6;)
                      local.get 6
                      local.set 0
                      local.get 1
                      local.get 6
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if 4 (;@5;)
                      br 5 (;@4;)
                    end
                    local.get 2
                    local.get 6
                    i32.sub
                    local.set 7
                    local.get 2
                    local.get 6
                    i32.ge_u
                    br_if 0 (;@8;)
                  end
                end
                local.get 5
                i32.const 0
                i32.store8
                local.get 2
                local.set 6
              end
              local.get 2
              local.set 0
              local.get 2
              local.get 6
              i32.eq
              br_if 1 (;@4;)
            end
            local.get 1
            local.get 2
            i32.const 0
            local.get 6
            local.get 0
            call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
            unreachable
          end
          block  ;; label = @4
            local.get 4
            local.get 1
            local.get 0
            local.get 3
            i32.load offset=12
            call_indirect (type 5)
            i32.eqz
            br_if 0 (;@4;)
            i32.const 1
            return
          end
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              local.get 0
              i32.gt_u
              br_if 0 (;@5;)
              local.get 2
              local.get 0
              i32.eq
              br_if 1 (;@4;)
              br 4 (;@1;)
            end
            local.get 1
            local.get 0
            i32.add
            i32.load8_s
            i32.const -65
            i32.le_s
            br_if 3 (;@1;)
          end
          local.get 1
          local.get 0
          i32.add
          local.set 1
          local.get 2
          local.get 0
          i32.sub
          local.tee 2
          br_if 0 (;@3;)
        end
      end
      i32.const 0
      return
    end
    local.get 1
    local.get 2
    local.get 0
    local.get 2
    local.get 0
    call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
    unreachable)
  (func $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE (type 11) (param i32 i32 i32 i32 i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 5
    global.set $__stack_pointer
    local.get 5
    local.get 3
    i32.store offset=12
    local.get 5
    local.get 2
    i32.store offset=8
    local.get 5
    local.get 1
    i32.store offset=4
    local.get 5
    local.get 0
    i32.store
    local.get 5
    call $_ZN4core10intrinsics17const_eval_select17h5024e92cfa62bd24E
    unreachable)
  (func $_ZN4core3fmt8builders11DebugStruct21finish_non_exhaustive17h9dcede22c3d51d75E (type 10) (param i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 1
    global.set $__stack_pointer
    i32.const 1
    local.set 2
    block  ;; label = @1
      local.get 0
      i32.load8_u offset=4
      br_if 0 (;@1;)
      local.get 0
      i32.load
      local.set 3
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=5
        br_if 0 (;@2;)
        local.get 3
        i32.load offset=24
        i32.const 1055684
        i32.const 7
        local.get 3
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        local.set 2
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 3
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        local.get 3
        i32.load offset=24
        i32.const 1055678
        i32.const 6
        local.get 3
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        local.set 2
        br 1 (;@1;)
      end
      i32.const 1
      local.set 2
      local.get 1
      i32.const 1
      i32.store8 offset=15
      local.get 1
      i32.const 8
      i32.add
      local.get 1
      i32.const 15
      i32.add
      i32.store
      local.get 1
      local.get 3
      i64.load offset=24 align=4
      i64.store
      local.get 1
      i32.const 1055674
      i32.const 3
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
      br_if 0 (;@1;)
      local.get 3
      i32.load offset=24
      i32.const 1055677
      i32.const 1
      local.get 3
      i32.load offset=28
      i32.load offset=12
      call_indirect (type 5)
      local.set 2
    end
    local.get 0
    local.get 2
    i32.store8 offset=4
    local.get 1
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 2)
  (func $_ZN4core3fmt8builders10DebugTuple5field17h963fe1aaa21a4b0fE (type 5) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i64 i64)
    global.get $__stack_pointer
    i32.const 64
    i32.sub
    local.tee 3
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load8_u offset=8
        i32.eqz
        br_if 0 (;@2;)
        local.get 0
        i32.load offset=4
        local.set 4
        i32.const 1
        local.set 5
        br 1 (;@1;)
      end
      local.get 0
      i32.load offset=4
      local.set 4
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 6
        i32.load
        local.tee 7
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 1
        local.set 5
        local.get 6
        i32.load offset=24
        i32.const 1055669
        i32.const 1055695
        local.get 4
        select
        i32.const 2
        i32.const 1
        local.get 4
        select
        local.get 6
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        local.get 1
        local.get 6
        local.get 2
        i32.load offset=12
        call_indirect (type 3)
        local.set 5
        br 1 (;@1;)
      end
      block  ;; label = @2
        local.get 4
        br_if 0 (;@2;)
        block  ;; label = @3
          local.get 6
          i32.load offset=24
          i32.const 1055693
          i32.const 2
          local.get 6
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 5)
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1
          local.set 5
          i32.const 0
          local.set 4
          br 2 (;@1;)
        end
        local.get 6
        i32.load
        local.set 7
      end
      i32.const 1
      local.set 5
      local.get 3
      i32.const 1
      i32.store8 offset=23
      local.get 3
      i32.const 52
      i32.add
      i32.const 1055636
      i32.store
      local.get 3
      i32.const 16
      i32.add
      local.get 3
      i32.const 23
      i32.add
      i32.store
      local.get 3
      local.get 7
      i32.store offset=24
      local.get 3
      local.get 6
      i64.load offset=24 align=4
      i64.store offset=8
      local.get 6
      i64.load offset=8 align=4
      local.set 8
      local.get 6
      i64.load offset=16 align=4
      local.set 9
      local.get 3
      local.get 6
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get 3
      local.get 6
      i32.load offset=4
      i32.store offset=28
      local.get 3
      local.get 9
      i64.store offset=40
      local.get 3
      local.get 8
      i64.store offset=32
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
      i32.const 1055667
      i32.const 2
      local.get 3
      i32.load offset=52
      i32.load offset=12
      call_indirect (type 5)
      local.set 5
    end
    local.get 0
    local.get 5
    i32.store8 offset=8
    local.get 0
    local.get 4
    i32.const 1
    i32.add
    i32.store offset=4
    local.get 3
    i32.const 64
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E (type 14) (param i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.eqz
        br_if 0 (;@2;)
        i32.const 43
        i32.const 1114112
        local.get 0
        i32.load
        local.tee 1
        i32.const 1
        i32.and
        local.tee 6
        select
        local.set 7
        local.get 6
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
      local.set 1
      i32.const 45
      local.set 7
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 1
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        i32.const 0
        local.set 2
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          local.get 3
          i32.const 16
          i32.lt_u
          br_if 0 (;@3;)
          local.get 2
          local.get 3
          call $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E
          local.set 6
          br 1 (;@2;)
        end
        block  ;; label = @3
          local.get 3
          br_if 0 (;@3;)
          i32.const 0
          local.set 6
          br 1 (;@2;)
        end
        local.get 3
        i32.const 3
        i32.and
        local.set 9
        block  ;; label = @3
          block  ;; label = @4
            local.get 3
            i32.const -1
            i32.add
            i32.const 3
            i32.ge_u
            br_if 0 (;@4;)
            i32.const 0
            local.set 6
            local.get 2
            local.set 1
            br 1 (;@3;)
          end
          local.get 3
          i32.const -4
          i32.and
          local.set 10
          i32.const 0
          local.set 6
          local.get 2
          local.set 1
          loop  ;; label = @4
            local.get 6
            local.get 1
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 1
            i32.const 1
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 1
            i32.const 2
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get 1
            i32.const 3
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set 6
            local.get 1
            i32.const 4
            i32.add
            local.set 1
            local.get 10
            i32.const -4
            i32.add
            local.tee 10
            br_if 0 (;@4;)
          end
        end
        local.get 9
        i32.eqz
        br_if 0 (;@2;)
        loop  ;; label = @3
          local.get 6
          local.get 1
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set 6
          local.get 1
          i32.const 1
          i32.add
          local.set 1
          local.get 9
          i32.const -1
          i32.add
          local.tee 9
          br_if 0 (;@3;)
        end
      end
      local.get 6
      local.get 8
      i32.add
      local.set 8
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load offset=8
        br_if 0 (;@2;)
        i32.const 1
        local.set 1
        local.get 0
        local.get 7
        local.get 2
        local.get 3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
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
        call_indirect (type 5)
        return
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                local.get 0
                i32.const 12
                i32.add
                i32.load
                local.tee 6
                local.get 8
                i32.le_u
                br_if 0 (;@6;)
                local.get 0
                i32.load8_u
                i32.const 8
                i32.and
                br_if 4 (;@2;)
                i32.const 0
                local.set 1
                local.get 6
                local.get 8
                i32.sub
                local.tee 9
                local.set 8
                i32.const 1
                local.get 0
                i32.load8_u offset=32
                local.tee 6
                local.get 6
                i32.const 3
                i32.eq
                select
                i32.const 3
                i32.and
                br_table 3 (;@3;) 1 (;@5;) 2 (;@4;) 3 (;@3;)
              end
              i32.const 1
              local.set 1
              local.get 0
              local.get 7
              local.get 2
              local.get 3
              call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
              br_if 4 (;@1;)
              local.get 0
              i32.load offset=24
              local.get 4
              local.get 5
              local.get 0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect (type 5)
              return
            end
            i32.const 0
            local.set 8
            local.get 9
            local.set 1
            br 1 (;@3;)
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
        end
        local.get 1
        i32.const 1
        i32.add
        local.set 1
        local.get 0
        i32.const 28
        i32.add
        i32.load
        local.set 9
        local.get 0
        i32.load offset=4
        local.set 6
        local.get 0
        i32.load offset=24
        local.set 10
        block  ;; label = @3
          loop  ;; label = @4
            local.get 1
            i32.const -1
            i32.add
            local.tee 1
            i32.eqz
            br_if 1 (;@3;)
            local.get 10
            local.get 6
            local.get 9
            i32.load offset=16
            call_indirect (type 3)
            i32.eqz
            br_if 0 (;@4;)
          end
          i32.const 1
          return
        end
        i32.const 1
        local.set 1
        local.get 6
        i32.const 1114112
        i32.eq
        br_if 1 (;@1;)
        local.get 0
        local.get 7
        local.get 2
        local.get 3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
        br_if 1 (;@1;)
        local.get 0
        i32.load offset=24
        local.get 4
        local.get 5
        local.get 0
        i32.load offset=28
        i32.load offset=12
        call_indirect (type 5)
        br_if 1 (;@1;)
        local.get 0
        i32.load offset=28
        local.set 9
        local.get 0
        i32.load offset=24
        local.set 0
        i32.const 0
        local.set 1
        block  ;; label = @3
          loop  ;; label = @4
            block  ;; label = @5
              local.get 8
              local.get 1
              i32.ne
              br_if 0 (;@5;)
              local.get 8
              local.set 1
              br 2 (;@3;)
            end
            local.get 1
            i32.const 1
            i32.add
            local.set 1
            local.get 0
            local.get 6
            local.get 9
            i32.load offset=16
            call_indirect (type 3)
            i32.eqz
            br_if 0 (;@4;)
          end
          local.get 1
          i32.const -1
          i32.add
          local.set 1
        end
        local.get 1
        local.get 8
        i32.lt_u
        local.set 1
        br 1 (;@1;)
      end
      local.get 0
      i32.load offset=4
      local.set 11
      local.get 0
      i32.const 48
      i32.store offset=4
      local.get 0
      i32.load8_u offset=32
      local.set 12
      i32.const 1
      local.set 1
      local.get 0
      i32.const 1
      i32.store8 offset=32
      local.get 0
      local.get 7
      local.get 2
      local.get 3
      call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
      br_if 0 (;@1;)
      i32.const 0
      local.set 1
      local.get 6
      local.get 8
      i32.sub
      local.tee 9
      local.set 3
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 1
            local.get 0
            i32.load8_u offset=32
            local.tee 6
            local.get 6
            i32.const 3
            i32.eq
            select
            i32.const 3
            i32.and
            br_table 2 (;@2;) 0 (;@4;) 1 (;@3;) 2 (;@2;)
          end
          i32.const 0
          local.set 3
          local.get 9
          local.set 1
          br 1 (;@2;)
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
        local.set 3
      end
      local.get 1
      i32.const 1
      i32.add
      local.set 1
      local.get 0
      i32.const 28
      i32.add
      i32.load
      local.set 9
      local.get 0
      i32.load offset=4
      local.set 6
      local.get 0
      i32.load offset=24
      local.set 10
      block  ;; label = @2
        loop  ;; label = @3
          local.get 1
          i32.const -1
          i32.add
          local.tee 1
          i32.eqz
          br_if 1 (;@2;)
          local.get 10
          local.get 6
          local.get 9
          i32.load offset=16
          call_indirect (type 3)
          i32.eqz
          br_if 0 (;@3;)
        end
        i32.const 1
        return
      end
      i32.const 1
      local.set 1
      local.get 6
      i32.const 1114112
      i32.eq
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=24
      local.get 4
      local.get 5
      local.get 0
      i32.load offset=28
      i32.load offset=12
      call_indirect (type 5)
      br_if 0 (;@1;)
      local.get 0
      i32.load offset=28
      local.set 1
      local.get 0
      i32.load offset=24
      local.set 10
      i32.const 0
      local.set 9
      block  ;; label = @2
        loop  ;; label = @3
          local.get 3
          local.get 9
          i32.eq
          br_if 1 (;@2;)
          local.get 9
          i32.const 1
          i32.add
          local.set 9
          local.get 10
          local.get 6
          local.get 1
          i32.load offset=16
          call_indirect (type 3)
          i32.eqz
          br_if 0 (;@3;)
        end
        i32.const 1
        local.set 1
        local.get 9
        i32.const -1
        i32.add
        local.get 3
        i32.lt_u
        br_if 1 (;@1;)
      end
      local.get 0
      local.get 12
      i32.store8 offset=32
      local.get 0
      local.get 11
      i32.store offset=4
      i32.const 0
      return
    end
    local.get 1)
  (func $_ZN4core3fmt5Write10write_char17h13cfc975967354caE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN4core3fmt5Write9write_fmt17hdb67d064950d9d5bE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1055920
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h84bb0430ffe0ae70E (type 5) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    local.get 2
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5b8cfb95cae022fbE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    local.get 2
    i32.const 12
    i32.add
    local.get 1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
    local.set 1
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h532266dfb8016bcfE (type 3) (param i32 i32) (result i32)
    (local i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    i32.const 1055920
    local.get 2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 3
        i32.add
        i32.const -4
        i32.and
        local.tee 2
        local.get 0
        i32.sub
        local.tee 3
        local.get 1
        i32.gt_u
        br_if 0 (;@2;)
        local.get 3
        i32.const 4
        i32.gt_u
        br_if 0 (;@2;)
        local.get 1
        local.get 3
        i32.sub
        local.tee 4
        i32.const 4
        i32.lt_u
        br_if 0 (;@2;)
        local.get 4
        i32.const 3
        i32.and
        local.set 5
        i32.const 0
        local.set 6
        i32.const 0
        local.set 1
        block  ;; label = @3
          local.get 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 3
          i32.const 3
          i32.and
          local.set 7
          block  ;; label = @4
            block  ;; label = @5
              local.get 2
              local.get 0
              i32.const -1
              i32.xor
              i32.add
              i32.const 3
              i32.ge_u
              br_if 0 (;@5;)
              i32.const 0
              local.set 1
              local.get 0
              local.set 2
              br 1 (;@4;)
            end
            local.get 3
            i32.const -4
            i32.and
            local.set 8
            i32.const 0
            local.set 1
            local.get 0
            local.set 2
            loop  ;; label = @5
              local.get 1
              local.get 2
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get 2
              i32.const 1
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get 2
              i32.const 2
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get 2
              i32.const 3
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.set 1
              local.get 2
              i32.const 4
              i32.add
              local.set 2
              local.get 8
              i32.const -4
              i32.add
              local.tee 8
              br_if 0 (;@5;)
            end
          end
          local.get 7
          i32.eqz
          br_if 0 (;@3;)
          loop  ;; label = @4
            local.get 1
            local.get 2
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set 1
            local.get 2
            i32.const 1
            i32.add
            local.set 2
            local.get 7
            i32.const -1
            i32.add
            local.tee 7
            br_if 0 (;@4;)
          end
        end
        local.get 0
        local.get 3
        i32.add
        local.set 0
        block  ;; label = @3
          local.get 5
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          local.get 4
          i32.const -4
          i32.and
          i32.add
          local.tee 2
          i32.load8_s
          i32.const -65
          i32.gt_s
          local.set 6
          local.get 5
          i32.const 1
          i32.eq
          br_if 0 (;@3;)
          local.get 6
          local.get 2
          i32.load8_s offset=1
          i32.const -65
          i32.gt_s
          i32.add
          local.set 6
          local.get 5
          i32.const 2
          i32.eq
          br_if 0 (;@3;)
          local.get 6
          local.get 2
          i32.load8_s offset=2
          i32.const -65
          i32.gt_s
          i32.add
          local.set 6
        end
        local.get 4
        i32.const 2
        i32.shr_u
        local.set 3
        local.get 6
        local.get 1
        i32.add
        local.set 8
        loop  ;; label = @3
          local.get 0
          local.set 6
          local.get 3
          i32.eqz
          br_if 2 (;@1;)
          local.get 3
          i32.const 192
          local.get 3
          i32.const 192
          i32.lt_u
          select
          local.tee 4
          i32.const 3
          i32.and
          local.set 5
          local.get 4
          i32.const 2
          i32.shl
          local.set 9
          block  ;; label = @4
            block  ;; label = @5
              local.get 4
              i32.const 252
              i32.and
              local.tee 10
              i32.const 2
              i32.shl
              local.tee 0
              br_if 0 (;@5;)
              i32.const 0
              local.set 2
              br 1 (;@4;)
            end
            local.get 6
            local.get 0
            i32.add
            local.set 7
            i32.const 0
            local.set 2
            local.get 6
            local.set 0
            loop  ;; label = @5
              local.get 0
              i32.const 12
              i32.add
              i32.load
              local.tee 1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get 1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get 0
              i32.const 8
              i32.add
              i32.load
              local.tee 1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get 1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get 0
              i32.const 4
              i32.add
              i32.load
              local.tee 1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get 1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get 0
              i32.load
              local.tee 1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get 1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get 2
              i32.add
              i32.add
              i32.add
              i32.add
              local.set 2
              local.get 0
              i32.const 16
              i32.add
              local.tee 0
              local.get 7
              i32.ne
              br_if 0 (;@5;)
            end
          end
          local.get 6
          local.get 9
          i32.add
          local.set 0
          local.get 3
          local.get 4
          i32.sub
          local.set 3
          local.get 2
          i32.const 8
          i32.shr_u
          i32.const 16711935
          i32.and
          local.get 2
          i32.const 16711935
          i32.and
          i32.add
          i32.const 65537
          i32.mul
          i32.const 16
          i32.shr_u
          local.get 8
          i32.add
          local.set 8
          local.get 5
          i32.eqz
          br_if 0 (;@3;)
        end
        local.get 6
        local.get 10
        i32.const 2
        i32.shl
        i32.add
        local.set 0
        local.get 5
        i32.const 1073741823
        i32.add
        local.tee 4
        i32.const 1073741823
        i32.and
        local.tee 2
        i32.const 1
        i32.add
        local.tee 1
        i32.const 3
        i32.and
        local.set 3
        block  ;; label = @3
          block  ;; label = @4
            local.get 2
            i32.const 3
            i32.ge_u
            br_if 0 (;@4;)
            i32.const 0
            local.set 2
            br 1 (;@3;)
          end
          local.get 1
          i32.const 2147483644
          i32.and
          local.set 1
          i32.const 0
          local.set 2
          loop  ;; label = @4
            local.get 0
            i32.const 12
            i32.add
            i32.load
            local.tee 7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get 7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get 0
            i32.const 8
            i32.add
            i32.load
            local.tee 7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get 7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get 0
            i32.const 4
            i32.add
            i32.load
            local.tee 7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get 7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get 0
            i32.load
            local.tee 7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get 7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get 2
            i32.add
            i32.add
            i32.add
            i32.add
            local.set 2
            local.get 0
            i32.const 16
            i32.add
            local.set 0
            local.get 1
            i32.const -4
            i32.add
            local.tee 1
            br_if 0 (;@4;)
          end
        end
        block  ;; label = @3
          local.get 3
          i32.eqz
          br_if 0 (;@3;)
          local.get 4
          i32.const -1073741823
          i32.add
          local.set 1
          loop  ;; label = @4
            local.get 0
            i32.load
            local.tee 7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get 7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get 2
            i32.add
            local.set 2
            local.get 0
            i32.const 4
            i32.add
            local.set 0
            local.get 1
            i32.const -1
            i32.add
            local.tee 1
            br_if 0 (;@4;)
          end
        end
        local.get 2
        i32.const 8
        i32.shr_u
        i32.const 16711935
        i32.and
        local.get 2
        i32.const 16711935
        i32.and
        i32.add
        i32.const 65537
        i32.mul
        i32.const 16
        i32.shr_u
        local.get 8
        i32.add
        return
      end
      block  ;; label = @2
        local.get 1
        br_if 0 (;@2;)
        i32.const 0
        return
      end
      local.get 1
      i32.const 3
      i32.and
      local.set 2
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.const -1
          i32.add
          i32.const 3
          i32.ge_u
          br_if 0 (;@3;)
          i32.const 0
          local.set 8
          br 1 (;@2;)
        end
        local.get 1
        i32.const -4
        i32.and
        local.set 1
        i32.const 0
        local.set 8
        loop  ;; label = @3
          local.get 8
          local.get 0
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get 0
          i32.const 1
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get 0
          i32.const 2
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get 0
          i32.const 3
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set 8
          local.get 0
          i32.const 4
          i32.add
          local.set 0
          local.get 1
          i32.const -4
          i32.add
          local.tee 1
          br_if 0 (;@3;)
        end
      end
      local.get 2
      i32.eqz
      br_if 0 (;@1;)
      loop  ;; label = @2
        local.get 8
        local.get 0
        i32.load8_s
        i32.const -65
        i32.gt_s
        i32.add
        local.set 8
        local.get 0
        i32.const 1
        i32.add
        local.set 0
        local.get 2
        i32.const -1
        i32.add
        local.tee 2
        br_if 0 (;@2;)
      end
    end
    local.get 8)
  (func $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E (type 6) (param i32 i32 i32 i32) (result i32)
    (local i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 1
          i32.const 1114112
          i32.eq
          br_if 0 (;@3;)
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
          br_if 1 (;@2;)
        end
        local.get 2
        br_if 1 (;@1;)
        i32.const 0
        local.set 4
      end
      local.get 4
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
    call_indirect (type 5))
  (func $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE (type 5) (param i32 i32 i32) (result i32)
    local.get 0
    i32.load offset=24
    local.get 1
    local.get 2
    local.get 0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 5))
  (func $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 32
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set 1
    local.get 2
    i32.const 32
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN4core3fmt9Formatter15debug_lower_hex17h633f97836530945dE (type 10) (param i32) (result i32)
    local.get 0
    i32.load8_u
    i32.const 16
    i32.and
    i32.const 4
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter15debug_upper_hex17h565dd489bc473806E (type 10) (param i32) (result i32)
    local.get 0
    i32.load8_u
    i32.const 32
    i32.and
    i32.const 5
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter12debug_struct17h678028cd4a965ad7E (type 8) (param i32 i32 i32 i32)
    local.get 1
    i32.load offset=24
    local.get 2
    local.get 3
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 5)
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
  (func $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h968628441d2ac37cE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load offset=24
    local.get 1
    local.get 0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=16
    call_indirect (type 3))
  (func $_ZN43_$LT$bool$u20$as$u20$core..fmt..Display$GT$3fmt17h107f9e8c546bfafcE (type 3) (param i32 i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.load8_u
      br_if 0 (;@1;)
      local.get 1
      i32.const 1055948
      i32.const 5
      call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E
      return
    end
    local.get 1
    i32.const 1055944
    i32.const 4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E (type 10) (param i32) (result i32)
    (local i32 i32 i32 i32 i32)
    local.get 0
    i32.const 11
    i32.shl
    local.set 1
    i32.const 0
    local.set 2
    i32.const 32
    local.set 3
    i32.const 32
    local.set 4
    block  ;; label = @1
      block  ;; label = @2
        loop  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              i32.const 1
              i32.shr_u
              local.get 2
              i32.add
              local.tee 3
              i32.const 2
              i32.shl
              i32.const 1058364
              i32.add
              i32.load
              i32.const 11
              i32.shl
              local.tee 5
              local.get 1
              i32.lt_u
              br_if 0 (;@5;)
              local.get 5
              local.get 1
              i32.eq
              br_if 3 (;@2;)
              local.get 3
              local.set 4
              br 1 (;@4;)
            end
            local.get 3
            i32.const 1
            i32.add
            local.set 2
          end
          local.get 4
          local.get 2
          i32.sub
          local.set 3
          local.get 4
          local.get 2
          i32.gt_u
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      local.get 3
      i32.const 1
      i32.add
      local.set 2
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.const 31
          i32.gt_u
          br_if 0 (;@3;)
          local.get 2
          i32.const 2
          i32.shl
          local.set 3
          i32.const 707
          local.set 4
          block  ;; label = @4
            local.get 2
            i32.const 31
            i32.eq
            br_if 0 (;@4;)
            local.get 3
            i32.const 1058368
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.set 4
          end
          i32.const 0
          local.set 1
          block  ;; label = @4
            local.get 2
            i32.const -1
            i32.add
            local.tee 5
            local.get 2
            i32.gt_u
            br_if 0 (;@4;)
            local.get 5
            i32.const 32
            i32.ge_u
            br_if 2 (;@2;)
            local.get 5
            i32.const 2
            i32.shl
            i32.const 1058364
            i32.add
            i32.load
            i32.const 2097151
            i32.and
            local.set 1
          end
          block  ;; label = @4
            local.get 4
            local.get 3
            i32.const 1058364
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.tee 2
            i32.const -1
            i32.xor
            i32.add
            i32.eqz
            br_if 0 (;@4;)
            local.get 0
            local.get 1
            i32.sub
            local.set 1
            local.get 2
            i32.const 707
            local.get 2
            i32.const 707
            i32.gt_u
            select
            local.set 3
            local.get 4
            i32.const -1
            i32.add
            local.set 5
            i32.const 0
            local.set 4
            loop  ;; label = @5
              local.get 3
              local.get 2
              i32.eq
              br_if 4 (;@1;)
              local.get 4
              local.get 2
              i32.const 1058492
              i32.add
              i32.load8_u
              i32.add
              local.tee 4
              local.get 1
              i32.gt_u
              br_if 1 (;@4;)
              local.get 5
              local.get 2
              i32.const 1
              i32.add
              local.tee 2
              i32.ne
              br_if 0 (;@5;)
            end
            local.get 5
            local.set 2
          end
          local.get 2
          i32.const 1
          i32.and
          return
        end
        local.get 2
        i32.const 32
        i32.const 1058244
        call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
        unreachable
      end
      local.get 5
      i32.const 32
      i32.const 1058276
      call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
      unreachable
    end
    local.get 3
    i32.const 707
    i32.const 1058260
    call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
    unreachable)
  (func $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE (type 10) (param i32) (result i32)
    (local i32)
    i32.const 0
    local.set 1
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.const 32
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 1
        local.set 1
        local.get 0
        i32.const 127
        i32.lt_u
        br_if 0 (;@2;)
        local.get 0
        i32.const 65536
        i32.lt_u
        br_if 1 (;@1;)
        block  ;; label = @3
          local.get 0
          i32.const 131072
          i32.lt_u
          br_if 0 (;@3;)
          local.get 0
          i32.const 2097150
          i32.and
          i32.const 178206
          i32.ne
          local.get 0
          i32.const 2097120
          i32.and
          i32.const 173792
          i32.ne
          local.get 0
          i32.const -177977
          i32.add
          i32.const 6
          i32.gt_u
          i32.and
          i32.and
          local.get 0
          i32.const -183984
          i32.add
          i32.const -14
          i32.lt_u
          i32.and
          local.get 0
          i32.const -194560
          i32.add
          i32.const -3103
          i32.lt_u
          i32.and
          local.get 0
          i32.const -196608
          i32.add
          i32.const -1506
          i32.lt_u
          i32.and
          local.get 0
          i32.const -917760
          i32.add
          i32.const -716213
          i32.lt_u
          i32.and
          local.get 0
          i32.const 918000
          i32.lt_u
          i32.and
          return
        end
        local.get 0
        i32.const 1057487
        i32.const 42
        i32.const 1057571
        i32.const 192
        i32.const 1057763
        i32.const 438
        call $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE
        local.set 1
      end
      local.get 1
      return
    end
    local.get 0
    i32.const 1056816
    i32.const 40
    i32.const 1056896
    i32.const 288
    i32.const 1057184
    i32.const 303
    call $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE)
  (func $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE (type 5) (param i32 i32 i32) (result i32)
    local.get 2
    local.get 0
    local.get 1
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hf9cabbcc6a6076ccE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i64 i32)
    i32.const 1
    local.set 2
    block  ;; label = @1
      local.get 1
      i32.load offset=24
      local.tee 3
      i32.const 39
      local.get 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=16
      local.tee 4
      call_indirect (type 3)
      br_if 0 (;@1;)
      i32.const 2
      local.set 1
      i32.const 48
      local.set 5
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
                        i32.load
                        local.tee 0
                        br_table 8 (;@2;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 2 (;@8;) 4 (;@6;) 1 (;@9;) 1 (;@9;) 3 (;@7;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 1 (;@9;) 5 (;@5;) 0 (;@10;)
                      end
                      local.get 0
                      i32.const 92
                      i32.eq
                      br_if 4 (;@5;)
                    end
                    local.get 0
                    call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E
                    i32.eqz
                    br_if 4 (;@4;)
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
                    local.set 6
                    br 5 (;@3;)
                  end
                  i32.const 116
                  local.set 5
                  i32.const 2
                  local.set 1
                  br 5 (;@2;)
                end
                i32.const 114
                local.set 5
                i32.const 2
                local.set 1
                br 4 (;@2;)
              end
              i32.const 110
              local.set 5
              i32.const 2
              local.set 1
              br 3 (;@2;)
            end
            i32.const 2
            local.set 1
            local.get 0
            local.set 5
            br 2 (;@2;)
          end
          block  ;; label = @4
            local.get 0
            call $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE
            i32.eqz
            br_if 0 (;@4;)
            i32.const 1
            local.set 1
            local.get 0
            local.set 5
            br 2 (;@2;)
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
          local.set 6
        end
        i32.const 3
        local.set 1
        local.get 0
        local.set 5
      end
      loop  ;; label = @2
        local.get 1
        local.set 7
        i32.const 0
        local.set 1
        local.get 5
        local.set 0
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  local.get 7
                  br_table 1 (;@6;) 4 (;@3;) 2 (;@5;) 0 (;@7;) 1 (;@6;)
                end
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 6
                          i64.const 32
                          i64.shr_u
                          i32.wrap_i64
                          i32.const 255
                          i32.and
                          br_table 5 (;@6;) 0 (;@11;) 4 (;@7;) 1 (;@10;) 2 (;@9;) 3 (;@8;) 5 (;@6;)
                        end
                        local.get 6
                        i64.const -1095216660481
                        i64.and
                        local.set 6
                        i32.const 125
                        local.set 0
                        i32.const 3
                        local.set 1
                        br 7 (;@3;)
                      end
                      local.get 6
                      i64.const -1095216660481
                      i64.and
                      i64.const 8589934592
                      i64.or
                      local.set 6
                      i32.const 123
                      local.set 0
                      i32.const 3
                      local.set 1
                      br 6 (;@3;)
                    end
                    local.get 6
                    i64.const -1095216660481
                    i64.and
                    i64.const 12884901888
                    i64.or
                    local.set 6
                    i32.const 117
                    local.set 0
                    i32.const 3
                    local.set 1
                    br 5 (;@3;)
                  end
                  local.get 6
                  i64.const -1095216660481
                  i64.and
                  i64.const 17179869184
                  i64.or
                  local.set 6
                  i32.const 92
                  local.set 0
                  i32.const 3
                  local.set 1
                  br 4 (;@3;)
                end
                i32.const 48
                i32.const 87
                local.get 5
                local.get 6
                i32.wrap_i64
                local.tee 1
                i32.const 2
                i32.shl
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
                local.get 1
                i32.eqz
                br_if 2 (;@4;)
                local.get 6
                i64.const -1
                i64.add
                i64.const 4294967295
                i64.and
                local.get 6
                i64.const -4294967296
                i64.and
                i64.or
                local.set 6
                i32.const 3
                local.set 1
                br 3 (;@3;)
              end
              local.get 3
              i32.const 39
              local.get 4
              call_indirect (type 3)
              local.set 2
              br 4 (;@1;)
            end
            i32.const 92
            local.set 0
            i32.const 1
            local.set 1
            br 1 (;@3;)
          end
          local.get 6
          i64.const -1095216660481
          i64.and
          i64.const 4294967296
          i64.or
          local.set 6
          i32.const 3
          local.set 1
        end
        local.get 3
        local.get 0
        local.get 4
        call_indirect (type 3)
        i32.eqz
        br_if 0 (;@2;)
      end
    end
    local.get 2)
  (func $_ZN4core5slice6memchr7memrchr17heb9113f3074b2e71E (type 8) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    local.get 3
    i32.const 0
    local.get 3
    local.get 2
    i32.const 3
    i32.add
    i32.const -4
    i32.and
    local.get 2
    i32.sub
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
        local.get 3
        local.get 6
        i32.lt_u
        br_if 0 (;@2;)
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              local.get 6
              i32.eqz
              br_if 0 (;@5;)
              local.get 2
              local.get 3
              i32.add
              local.tee 6
              local.get 2
              local.get 7
              i32.add
              local.tee 8
              i32.sub
              local.set 9
              block  ;; label = @6
                local.get 6
                i32.const -1
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -1
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -2
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -2
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -3
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -3
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -4
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -4
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -5
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -5
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -6
                i32.add
                local.tee 10
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -6
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 10
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                local.get 6
                i32.const -7
                i32.add
                local.tee 6
                i32.load8_u
                local.get 1
                i32.const 255
                i32.and
                i32.ne
                br_if 0 (;@6;)
                local.get 9
                i32.const -7
                i32.add
                local.get 7
                i32.add
                local.set 6
                br 2 (;@4;)
              end
              local.get 8
              local.get 6
              i32.eq
              br_if 0 (;@5;)
              local.get 9
              i32.const -8
              i32.add
              local.get 7
              i32.add
              local.set 6
              br 1 (;@4;)
            end
            local.get 3
            local.get 4
            local.get 5
            select
            local.set 8
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
                i32.const -8
                i32.add
                i32.load
                local.get 4
                i32.xor
                local.tee 9
                i32.const -1
                i32.xor
                local.get 9
                i32.const -16843009
                i32.add
                i32.and
                local.get 5
                i32.const -4
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
            br_if 3 (;@1;)
            local.get 2
            i32.const -1
            i32.add
            local.set 4
            local.get 1
            i32.const 255
            i32.and
            local.set 5
            loop  ;; label = @5
              block  ;; label = @6
                local.get 6
                br_if 0 (;@6;)
                i32.const 0
                local.set 7
                br 3 (;@3;)
              end
              local.get 4
              local.get 6
              i32.add
              local.set 7
              local.get 6
              i32.const -1
              i32.add
              local.set 6
              local.get 7
              i32.load8_u
              local.get 5
              i32.ne
              br_if 0 (;@5;)
            end
          end
          i32.const 1
          local.set 7
        end
        local.get 0
        local.get 6
        i32.store offset=4
        local.get 0
        local.get 7
        i32.store
        return
      end
      local.get 7
      local.get 3
      local.get 6
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 6
    local.get 3
    local.get 6
    call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
    unreachable)
  (func $_ZN4core3str5lossy9Utf8Lossy10from_bytes17h9d8ade7d6641c0d4E (type 4) (param i32 i32 i32)
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN4core3str5lossy9Utf8Lossy6chunks17hdeccf1496c5995d4E (type 4) (param i32 i32 i32)
    local.get 0
    local.get 2
    i32.store offset=4
    local.get 0
    local.get 1
    i32.store)
  (func $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E (type 2) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32)
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
        loop  ;; label = @3
          local.get 4
          i32.const 1
          i32.add
          local.set 5
          block  ;; label = @4
            block  ;; label = @5
              local.get 3
              local.get 4
              i32.add
              i32.load8_u
              local.tee 6
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              local.tee 7
              i32.const -1
              i32.le_s
              br_if 0 (;@5;)
              local.get 5
              local.set 4
              br 1 (;@4;)
            end
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          local.get 6
                          i32.const 1056188
                          i32.add
                          i32.load8_u
                          i32.const -2
                          i32.add
                          br_table 0 (;@11;) 1 (;@10;) 2 (;@9;) 9 (;@2;)
                        end
                        local.get 3
                        local.get 5
                        i32.add
                        i32.const 1055260
                        local.get 5
                        local.get 2
                        i32.lt_u
                        select
                        i32.load8_u
                        i32.const 192
                        i32.and
                        i32.const 128
                        i32.ne
                        br_if 8 (;@2;)
                        local.get 4
                        i32.const 2
                        i32.add
                        local.set 4
                        br 6 (;@4;)
                      end
                      local.get 3
                      local.get 5
                      i32.add
                      i32.const 1055260
                      local.get 5
                      local.get 2
                      i32.lt_u
                      select
                      i32.load8_s
                      local.set 8
                      local.get 6
                      i32.const -224
                      i32.add
                      br_table 1 (;@8;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 3 (;@6;) 2 (;@7;) 3 (;@6;)
                    end
                    local.get 3
                    local.get 5
                    i32.add
                    i32.const 1055260
                    local.get 5
                    local.get 2
                    i32.lt_u
                    select
                    i32.load8_s
                    local.set 8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            local.get 6
                            i32.const -240
                            i32.add
                            br_table 1 (;@11;) 0 (;@12;) 0 (;@12;) 0 (;@12;) 2 (;@10;) 0 (;@12;)
                          end
                          local.get 7
                          i32.const 15
                          i32.add
                          i32.const 255
                          i32.and
                          i32.const 2
                          i32.gt_u
                          br_if 9 (;@2;)
                          local.get 8
                          i32.const -1
                          i32.gt_s
                          br_if 9 (;@2;)
                          local.get 8
                          i32.const -64
                          i32.lt_u
                          br_if 2 (;@9;)
                          br 9 (;@2;)
                        end
                        local.get 8
                        i32.const 112
                        i32.add
                        i32.const 255
                        i32.and
                        i32.const 48
                        i32.lt_u
                        br_if 1 (;@9;)
                        br 8 (;@2;)
                      end
                      local.get 8
                      i32.const -113
                      i32.gt_s
                      br_if 7 (;@2;)
                    end
                    local.get 3
                    local.get 4
                    i32.const 2
                    i32.add
                    local.tee 5
                    i32.add
                    i32.const 1055260
                    local.get 5
                    local.get 2
                    i32.lt_u
                    select
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.ne
                    br_if 6 (;@2;)
                    local.get 3
                    local.get 4
                    i32.const 3
                    i32.add
                    local.tee 5
                    i32.add
                    i32.const 1055260
                    local.get 5
                    local.get 2
                    i32.lt_u
                    select
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.ne
                    br_if 6 (;@2;)
                    local.get 4
                    i32.const 4
                    i32.add
                    local.set 4
                    br 4 (;@4;)
                  end
                  local.get 8
                  i32.const -32
                  i32.and
                  i32.const -96
                  i32.ne
                  br_if 5 (;@2;)
                  br 2 (;@5;)
                end
                local.get 8
                i32.const -96
                i32.ge_s
                br_if 4 (;@2;)
                br 1 (;@5;)
              end
              block  ;; label = @6
                local.get 7
                i32.const 31
                i32.add
                i32.const 255
                i32.and
                i32.const 12
                i32.lt_u
                br_if 0 (;@6;)
                local.get 7
                i32.const -2
                i32.and
                i32.const -18
                i32.ne
                br_if 4 (;@2;)
                local.get 8
                i32.const -1
                i32.gt_s
                br_if 4 (;@2;)
                local.get 8
                i32.const -64
                i32.ge_u
                br_if 4 (;@2;)
                br 1 (;@5;)
              end
              local.get 8
              i32.const -65
              i32.gt_s
              br_if 3 (;@2;)
            end
            local.get 3
            local.get 4
            i32.const 2
            i32.add
            local.tee 5
            i32.add
            i32.const 1055260
            local.get 5
            local.get 2
            i32.lt_u
            select
            i32.load8_u
            i32.const 192
            i32.and
            i32.const 128
            i32.ne
            br_if 2 (;@2;)
            local.get 4
            i32.const 3
            i32.add
            local.set 4
          end
          local.get 4
          local.set 5
          local.get 4
          local.get 2
          i32.lt_u
          br_if 0 (;@3;)
        end
      end
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
      local.get 5
      local.get 4
      i32.sub
      i32.store
      local.get 0
      i32.const 8
      i32.add
      local.get 3
      local.get 4
      i32.add
      i32.store
      return
    end
    local.get 0
    i32.const 0
    i32.store)
  (func $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE (type 15) (param i32 i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    i32.const 1
    local.set 7
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        i32.eqz
        br_if 0 (;@2;)
        local.get 1
        local.get 2
        i32.const 1
        i32.shl
        i32.add
        local.set 8
        local.get 0
        i32.const 65280
        i32.and
        i32.const 8
        i32.shr_u
        local.set 9
        i32.const 0
        local.set 10
        local.get 0
        i32.const 255
        i32.and
        local.set 11
        block  ;; label = @3
          loop  ;; label = @4
            local.get 1
            i32.const 2
            i32.add
            local.set 12
            local.get 10
            local.get 1
            i32.load8_u offset=1
            local.tee 2
            i32.add
            local.set 13
            block  ;; label = @5
              local.get 1
              i32.load8_u
              local.tee 1
              local.get 9
              i32.eq
              br_if 0 (;@5;)
              local.get 1
              local.get 9
              i32.gt_u
              br_if 3 (;@2;)
              local.get 13
              local.set 10
              local.get 12
              local.set 1
              local.get 12
              local.get 8
              i32.ne
              br_if 1 (;@4;)
              br 3 (;@2;)
            end
            block  ;; label = @5
              local.get 13
              local.get 10
              i32.lt_u
              br_if 0 (;@5;)
              local.get 13
              local.get 4
              i32.gt_u
              br_if 2 (;@3;)
              local.get 3
              local.get 10
              i32.add
              local.set 1
              block  ;; label = @6
                loop  ;; label = @7
                  local.get 2
                  i32.eqz
                  br_if 1 (;@6;)
                  local.get 2
                  i32.const -1
                  i32.add
                  local.set 2
                  local.get 1
                  i32.load8_u
                  local.set 10
                  local.get 1
                  i32.const 1
                  i32.add
                  local.set 1
                  local.get 10
                  local.get 11
                  i32.ne
                  br_if 0 (;@7;)
                end
                i32.const 0
                local.set 7
                br 5 (;@1;)
              end
              local.get 13
              local.set 10
              local.get 12
              local.set 1
              local.get 12
              local.get 8
              i32.ne
              br_if 1 (;@4;)
              br 3 (;@2;)
            end
          end
          local.get 10
          local.get 13
          local.get 2
          call $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E
          unreachable
        end
        local.get 13
        local.get 4
        local.get 2
        call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
        unreachable
      end
      local.get 6
      i32.eqz
      br_if 0 (;@1;)
      local.get 5
      local.get 6
      i32.add
      local.set 11
      local.get 0
      i32.const 65535
      i32.and
      local.set 1
      i32.const 1
      local.set 7
      block  ;; label = @2
        loop  ;; label = @3
          local.get 5
          i32.const 1
          i32.add
          local.set 10
          block  ;; label = @4
            block  ;; label = @5
              local.get 5
              i32.load8_u
              local.tee 2
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              local.tee 13
              i32.const 0
              i32.lt_s
              br_if 0 (;@5;)
              local.get 10
              local.set 5
              br 1 (;@4;)
            end
            local.get 10
            local.get 11
            i32.eq
            br_if 2 (;@2;)
            local.get 13
            i32.const 127
            i32.and
            i32.const 8
            i32.shl
            local.get 5
            i32.load8_u offset=1
            i32.or
            local.set 2
            local.get 5
            i32.const 2
            i32.add
            local.set 5
          end
          local.get 1
          local.get 2
          i32.sub
          local.tee 1
          i32.const 0
          i32.lt_s
          br_if 2 (;@1;)
          local.get 7
          i32.const 1
          i32.xor
          local.set 7
          local.get 5
          local.get 11
          i32.ne
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
      end
      i32.const 1055352
      i32.const 43
      i32.const 1056800
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get 7
    i32.const 1
    i32.and)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17hb5f1486a0d6bf05cE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 0
    i32.load
    local.set 0
    i32.const 0
    local.set 3
    loop  ;; label = @1
      local.get 2
      local.get 3
      i32.add
      i32.const 127
      i32.add
      i32.const 48
      i32.const 87
      local.get 0
      i32.const 15
      i32.and
      local.tee 4
      i32.const 10
      i32.lt_u
      select
      local.get 4
      i32.add
      i32.store8
      local.get 3
      i32.const -1
      i32.add
      local.set 3
      local.get 0
      i32.const 15
      i32.gt_u
      local.set 4
      local.get 0
      i32.const 4
      i32.shr_u
      local.set 0
      local.get 4
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 3
      i32.const 128
      i32.add
      local.tee 0
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 128
      local.get 0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get 2
    local.get 3
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 3
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt3num49_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u8$GT$3fmt17h2f6ce406ff5467efE (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
              local.get 3
              i32.const 32
              i32.and
              br_if 1 (;@4;)
              local.get 0
              i64.load8_u
              i32.const 1
              local.get 1
              call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E
              local.set 0
              br 4 (;@1;)
            end
            local.get 0
            i32.load8_u
            local.set 3
            i32.const 0
            local.set 0
            loop  ;; label = @5
              local.get 2
              local.get 0
              i32.add
              i32.const 127
              i32.add
              i32.const 48
              i32.const 87
              local.get 3
              i32.const 15
              i32.and
              local.tee 4
              i32.const 10
              i32.lt_u
              select
              local.get 4
              i32.add
              i32.store8
              local.get 0
              i32.const -1
              i32.add
              local.set 0
              local.get 3
              i32.const 255
              i32.and
              local.tee 4
              i32.const 4
              i32.shr_u
              local.set 3
              local.get 4
              i32.const 15
              i32.gt_u
              br_if 0 (;@5;)
            end
            local.get 0
            i32.const 128
            i32.add
            local.tee 3
            i32.const 129
            i32.ge_u
            br_if 1 (;@3;)
            local.get 1
            i32.const 1
            i32.const 1055716
            i32.const 2
            local.get 2
            local.get 0
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get 0
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
            local.set 0
            br 3 (;@1;)
          end
          local.get 0
          i32.load8_u
          local.set 3
          i32.const 0
          local.set 0
          loop  ;; label = @4
            local.get 2
            local.get 0
            i32.add
            i32.const 127
            i32.add
            i32.const 48
            i32.const 55
            local.get 3
            i32.const 15
            i32.and
            local.tee 4
            i32.const 10
            i32.lt_u
            select
            local.get 4
            i32.add
            i32.store8
            local.get 0
            i32.const -1
            i32.add
            local.set 0
            local.get 3
            i32.const 255
            i32.and
            local.tee 4
            i32.const 4
            i32.shr_u
            local.set 3
            local.get 4
            i32.const 15
            i32.gt_u
            br_if 0 (;@4;)
          end
          local.get 0
          i32.const 128
          i32.add
          local.tee 3
          i32.const 129
          i32.ge_u
          br_if 1 (;@2;)
          local.get 1
          i32.const 1
          i32.const 1055716
          i32.const 2
          local.get 2
          local.get 0
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get 0
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
          local.set 0
          br 2 (;@1;)
        end
        local.get 3
        i32.const 128
        local.get 0
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get 3
      i32.const 128
      local.get 0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 2
    i32.const 128
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E (type 16) (param i64 i32 i32) (result i32)
    (local i32 i32 i64 i32 i32 i32)
    global.get $__stack_pointer
    i32.const 48
    i32.sub
    local.tee 3
    global.set $__stack_pointer
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
        i32.const 1055718
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
        i32.const 1055718
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
      i32.le_u
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
      i32.const 1055718
      i32.add
      i32.load16_u align=1
      i32.store16 align=1
    end
    block  ;; label = @1
      block  ;; label = @2
        local.get 6
        i32.const 10
        i32.lt_u
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
        i32.const 1055718
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
    i32.const 1055260
    i32.const 0
    local.get 3
    i32.const 9
    i32.add
    local.get 4
    i32.add
    i32.const 39
    local.get 4
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set 4
    local.get 3
    i32.const 48
    i32.add
    global.set $__stack_pointer
    local.get 4)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17hb78061cd9b3bd312E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 2
    global.set $__stack_pointer
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
      i32.const 48
      i32.const 55
      local.get 3
      i32.const 15
      i32.and
      local.tee 4
      i32.const 10
      i32.lt_u
      select
      local.get 4
      i32.add
      i32.store8
      local.get 0
      i32.const -1
      i32.add
      local.set 0
      local.get 3
      i32.const 255
      i32.and
      local.tee 4
      i32.const 4
      i32.shr_u
      local.set 3
      local.get 4
      i32.const 15
      i32.gt_u
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
      local.get 0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get 2
    local.get 0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17hfe79af84422ce847E (type 3) (param i32 i32) (result i32)
    (local i32 i32 i32)
    global.get $__stack_pointer
    i32.const 128
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 0
    i32.load
    local.set 0
    i32.const 0
    local.set 3
    loop  ;; label = @1
      local.get 2
      local.get 3
      i32.add
      i32.const 127
      i32.add
      i32.const 48
      i32.const 55
      local.get 0
      i32.const 15
      i32.and
      local.tee 4
      i32.const 10
      i32.lt_u
      select
      local.get 4
      i32.add
      i32.store8
      local.get 3
      i32.const -1
      i32.add
      local.set 3
      local.get 0
      i32.const 15
      i32.gt_u
      local.set 4
      local.get 0
      i32.const 4
      i32.shr_u
      local.set 0
      local.get 4
      br_if 0 (;@1;)
    end
    block  ;; label = @1
      local.get 3
      i32.const 128
      i32.add
      local.tee 0
      i32.const 129
      i32.lt_u
      br_if 0 (;@1;)
      local.get 0
      i32.const 128
      local.get 0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get 1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get 2
    local.get 3
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get 3
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set 0
    local.get 2
    i32.const 128
    i32.add
    global.set $__stack_pointer
    local.get 0)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h4887f6c2cac827aeE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.tee 0
    i64.extend_i32_u
    local.get 0
    i32.const -1
    i32.xor
    i64.extend_i32_s
    i64.const 1
    i64.add
    local.get 0
    i32.const -1
    i32.gt_s
    local.tee 0
    select
    local.get 0
    local.get 1
    call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h015fd4bc7d5a98ffE (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hd1cef3d00f68bba3E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    block  ;; label = @1
      block  ;; label = @2
        local.get 0
        i32.load
        local.tee 0
        i32.load8_u
        br_if 0 (;@2;)
        local.get 1
        i32.load offset=24
        i32.const 1058296
        i32.const 4
        local.get 1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        local.set 1
        br 1 (;@1;)
      end
      local.get 2
      local.get 1
      i32.load offset=24
      i32.const 1058292
      i32.const 4
      local.get 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type 5)
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
      i32.const 1
      local.set 1
      local.get 2
      local.get 0
      i32.const 1
      i32.add
      i32.store offset=12
      local.get 2
      local.get 2
      i32.const 12
      i32.add
      i32.const 1055700
      call $_ZN4core3fmt8builders10DebugTuple5field17h963fe1aaa21a4b0fE
      drop
      local.get 2
      i32.load8_u offset=8
      local.set 0
      block  ;; label = @2
        block  ;; label = @3
          local.get 2
          i32.load offset=4
          local.tee 3
          br_if 0 (;@3;)
          local.get 0
          local.set 1
          br 1 (;@2;)
        end
        local.get 0
        i32.const 255
        i32.and
        br_if 0 (;@2;)
        local.get 2
        i32.load
        local.set 0
        block  ;; label = @3
          local.get 3
          i32.const 1
          i32.ne
          br_if 0 (;@3;)
          local.get 2
          i32.load8_u offset=9
          i32.const 255
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          local.get 0
          i32.load8_u
          i32.const 4
          i32.and
          br_if 0 (;@3;)
          i32.const 1
          local.set 1
          local.get 0
          i32.load offset=24
          i32.const 1055696
          i32.const 1
          local.get 0
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 5)
          br_if 1 (;@2;)
        end
        local.get 0
        i32.load offset=24
        i32.const 1055261
        i32.const 1
        local.get 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        local.set 1
      end
      local.get 1
      i32.const 255
      i32.and
      i32.const 0
      i32.ne
      local.set 1
    end
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17he00dc59ef87d3ec0E (type 3) (param i32 i32) (result i32)
    local.get 0
    i32.load
    local.get 1
    call $_ZN4core3fmt3num49_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u8$GT$3fmt17h2f6ce406ff5467efE)
  (func $_ZN64_$LT$core..str..error..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17h6965576c5be6dee1E (type 3) (param i32 i32) (result i32)
    (local i32 i32)
    global.get $__stack_pointer
    i32.const 16
    i32.sub
    local.tee 2
    global.set $__stack_pointer
    local.get 1
    i32.load offset=24
    i32.const 1058316
    i32.const 9
    local.get 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 5)
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
    i32.const 1058325
    i32.const 11
    local.get 2
    i32.const 12
    i32.add
    i32.const 1058300
    call $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE
    local.set 1
    local.get 2
    local.get 0
    i32.const 4
    i32.add
    i32.store offset=12
    local.get 1
    i32.const 1058336
    i32.const 9
    local.get 2
    i32.const 12
    i32.add
    i32.const 1058348
    call $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE
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
      local.get 0
      br_if 0 (;@1;)
      block  ;; label = @2
        local.get 2
        i32.load
        local.tee 1
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        local.get 1
        i32.load offset=24
        i32.const 1055691
        i32.const 2
        local.get 1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 5)
        local.set 1
        br 1 (;@1;)
      end
      local.get 1
      i32.load offset=24
      i32.const 1055677
      i32.const 1
      local.get 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type 5)
      local.set 1
    end
    local.get 2
    i32.const 16
    i32.add
    global.set $__stack_pointer
    local.get 1
    i32.const 255
    i32.and
    i32.const 0
    i32.ne)
  (func $print_env.command_export (type 7)
    call $__wasm_call_ctors
    call $print_env
    call $__wasm_call_dtors)
  (table (;0;) 86 86 funcref)
  (memory (;0;) 17)
  (global $__stack_pointer (mut i32) (i32.const 1048576))
  (global (;1;) i32 (i32.const 1059856))
  (global (;2;) i32 (i32.const 1059844))
  (export "memory" (memory 0))
  (export "__heap_base" (global 1))
  (export "__data_end" (global 2))
  (export "print_env" (func $print_env.command_export))
  (elem (;0;) (i32.const 1) func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h547981a8fa47a7d3E $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h4887f6c2cac827aeE $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E.1 $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17hf8285e92300923c0E $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h30941f502999929dE $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E $_ZN3std5alloc24default_alloc_error_hook17h5a1a1d982f9bd818E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h32abcc04c39e85a4E $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hafba394e37912a5aE $_ZN73_$LT$core..panic..panic_info..PanicInfo$u20$as$u20$core..fmt..Display$GT$3fmt17h92a87928d361b08eE $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h5e2128c63f339bb4E $_ZN4core3ptr100drop_in_place$LT$$RF$mut$u20$std..io..Write..write_fmt..Adapter$LT$alloc..vec..Vec$LT$u8$GT$$GT$$GT$17hf1b3c7767b0b7765E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h35232905e30669aaE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17hbc3d3931408e41d6E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hf0a5b3c4ac0d0496E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h7f904ebe63e601aaE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h9896f020280c35deE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h47c6b3c5d318955aE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5e136cc95a131902E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h2ffc04b3c6716e0dE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17he4a59f9604beea94E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17he56d425d213d6944E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5a3b0091f8192a8fE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h8027bf4984b5aaecE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h5d3c77c4d604c6d6E $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h651b426602cda903E $_ZN4core3ptr103drop_in_place$LT$std..sync..poison..PoisonError$LT$std..sync..mutex..MutexGuard$LT$$LP$$RP$$GT$$GT$$GT$17h342b8354fe7ce8f5E $_ZN76_$LT$std..sync..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h45ee2ecae3ad0859E $_ZN64_$LT$core..str..error..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17h6965576c5be6dee1E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h51406901289b8250E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h72c85046dd08428aE $_ZN4core3ptr226drop_in_place$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Send$u2b$core..marker..Sync$GT$$GT$..from..StringError$GT$17h8f80eb1a61eb416eE $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17h3d2bbcbe4fd788bbE $_ZN4core3ptr87drop_in_place$LT$std..io..Write..write_fmt..Adapter$LT$$RF$mut$u20$$u5b$u8$u5d$$GT$$GT$17h9e512fc24cfce3e7E $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E $_ZN4core3fmt5Write10write_char17hbe9f53f72de9fff4E $_ZN4core3fmt5Write9write_fmt17h9501b0d1bd73714aE $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha099fde24b4a4dcaE $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E $_ZN4core3fmt5Write9write_fmt17h8a28623dee2384e2E $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb12703cce11fe208E $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E $_ZN4core3fmt5Write9write_fmt17hcb1cb3e9d099f402E $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17hd87567c1af08f732E $_ZN3std4sync4once4Once15call_once_force28_$u7b$$u7b$closure$u7d$$u7d$17h5da9e8acebb38efeE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17h7fa424a882c725e9E $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17he7f7854621726461E $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h1cc0993ef9fea8dfE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5flush17h5d2f24e96e822d71E $_ZN3std2io5Write9write_all17hf2d1f3fd20cf6d1dE $_ZN3std2io5Write18write_all_vectored17h3007a4983f10eddeE $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5write17hfeddc07cce766a8dE $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$14write_vectored17h5e2f18a5d90c2318E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$17is_write_vectored17hbcc64e0de2a7bd6eE $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5flush17h3e171420e6883b71E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$9write_all17hacb9a4b06548536cE $_ZN3std2io5Write18write_all_vectored17h73403c20721d838fE $_ZN3std2io5Write9write_fmt17hf06e34d13533586bE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2d0c1ef8155b3364E $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h83af4092997449d4E $_ZN4core3ptr70drop_in_place$LT$std..panicking..begin_panic_handler..PanicPayload$GT$17ha86c38a5fb14f016E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h34d0cd2380313f86E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17ha161f6684539a1beE $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h652349175b493901E $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17hdfcd929547320219E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h8790479cff165904E $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17ha826dd87512c4c00E $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hf9cabbcc6a6076ccE $_ZN4core3ops8function6FnOnce9call_once17he625032f56c99b4fE $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h866d2163a1275370E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h0d0f7379d9aa8697E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hade274a01ebef0eeE $_ZN4core3ptr102drop_in_place$LT$$RF$core..iter..adapters..copied..Copied$LT$core..slice..iter..Iter$LT$u8$GT$$GT$$GT$17h0ae777612708bf9bE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hd78f961f99573a2cE $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E $_ZN4core3fmt5Write10write_char17h13cfc975967354caE $_ZN4core3fmt5Write9write_fmt17hdb67d064950d9d5bE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17he00dc59ef87d3ec0E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h84bb0430ffe0ae70E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5b8cfb95cae022fbE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h532266dfb8016bcfE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h015fd4bc7d5a98ffE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hd1cef3d00f68bba3E)
  (data $.rodata (i32.const 1048576) "The env vars are as follows.\0a\00\00\00\00\00\10\00\1d\00\00\00: \0a\00\00\00\10\00\00\00\00\00(\00\10\00\02\00\00\00*\00\10\00\01\00\00\00The args are as follows.\0a\00\00\00D\00\10\00\19\00\00\00\00\00\10\00\00\00\00\00*\00\10\00\01\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\0e\00\00\00\0f\00\00\00\10\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\11\00\00\00\12\00\00\00\13\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\14\00\00\00\15\00\00\00\16\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\17\00\00\00\18\00\00\00\19\00\00\00already borrowed\0d\00\00\00\00\00\00\00\01\00\00\00\1a\00\00\00assertion failed: mid <= self.len()called `Option::unwrap()` on a `None` value\00\00\0d\00\00\00\00\00\00\00\01\00\00\00\1b\00\00\00called `Result::unwrap()` on an `Err` value\00\1c\00\00\00\08\00\00\00\04\00\00\00\1d\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00\1e\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\1f\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00 \00\00\00internal error: entered unreachable code/rustc/546c826f0ccaab36e897860205281f490db274e6/library/alloc/src/vec/mod.rs\ec\01\10\00L\00\00\00>\07\00\00$\00\00\00fatal runtime error: \0a\00\00H\02\10\00\15\00\00\00]\02\10\00\01\00\00\00use of std::thread::current() is not possible after the thread's local data has been destroyedlibrary/std/src/thread/mod.rs\00\ce\02\10\00\1d\00\00\00\a5\02\00\00#\00\00\00failed to generate unique thread ID: bitspace exhausted\00\fc\02\10\007\00\00\00\ce\02\10\00\1d\00\00\00\13\04\00\00\11\00\00\00\ce\02\10\00\1d\00\00\00\19\04\00\00*\00\00\00\22RUST_BACKTRACElibrary/std/src/env.rs\00\00\00k\03\10\00\16\00\00\00\ab\00\00\009\00\00\00k\03\10\00\16\00\00\00\ab\00\00\00S\00\00\00k\03\10\00\16\00\00\00%\03\00\003\00\00\00\d8\00\10\00\00\00\00\00: \00\00!\00\00\00\0c\00\00\00\04\00\00\00\22\00\00\00\00failed to write the buffered data\00\00\d1\03\10\00!\00\00\00\17\00\00\00library/std/src/io/buffered/bufwriter.rs\00\04\10\00(\00\00\00\8d\00\00\00\12\00\00\00library/std/src/io/buffered/linewritershim.rs\00\00\008\04\10\00-\00\00\00\01\01\00\00)\00\00\00uncategorized errorother errorout of memoryunexpected end of fileunsupportedoperation interruptedargument list too longinvalid filenametoo many linkscross-device link or renamedeadlockexecutable file busyresource busyfile too largefilesystem quota exceededseek on unseekable fileno storage spacewrite zerotimed outinvalid datainvalid input parameterstale network file handlefilesystem loop or indirection limit (e.g. symlink loop)read-only filesystem or storage mediumdirectory not emptyis a directorynot a directoryoperation would blockentity already existsbroken pipenetwork downaddress not availableaddress in usenot connectedconnection abortednetwork unreachablehost unreachableconnection resetconnection refusedpermission deniedentity not found (os error )\00\00\00\d8\00\10\00\00\00\00\00e\07\10\00\0b\00\00\00p\07\10\00\01\00\00\00failed to write whole buffer\8c\07\10\00\1c\00\00\00\17\00\00\00library/std/src/io/stdio.rs\00\b4\07\10\00\1b\00\00\00\dc\02\00\00\14\00\00\00failed printing to \00\e0\07\10\00\13\00\00\00\bc\03\10\00\02\00\00\00\b4\07\10\00\1b\00\00\00\f8\03\00\00\09\00\00\00stdoutlibrary/std/src/io/mod.rs\00\1a\08\10\00\19\00\00\00\0a\05\00\00\16\00\00\00\1a\08\10\00\19\00\00\00\f1\05\00\00!\00\00\00formatter error\00T\08\10\00\0f\00\00\00(\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00$\00\00\00%\00\00\00&\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00'\00\00\00(\00\00\00)\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00*\00\00\00+\00\00\00,\00\00\00library/std/src/panic.rs\b8\08\10\00\18\00\00\00\f0\00\00\00\12\00\00\00library/std/src/sync/once.rs\e0\08\10\00\1c\00\00\00N\01\00\00\0e\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00-\00\00\00.\00\00\00\e0\08\10\00\1c\00\00\00N\01\00\001\00\00\00assertion failed: state_and_queue.addr() & STATE_MASK == RUNNINGOnce instance has previously been poisoned\00\00p\09\10\00*\00\00\00\02\00\00\00\e0\08\10\00\1c\00\00\00\ff\01\00\00\09\00\00\00\e0\08\10\00\1c\00\00\00\0c\02\00\005\00\00\00PoisonErrorstack backtrace:\0a\d3\09\10\00\11\00\00\00note: Some details are omitted, run with `RUST_BACKTRACE=full` for a verbose backtrace.\0a\ec\09\10\00X\00\00\00lock count overflow in reentrant mutexlibrary/std/src/sys_common/remutex.rs\00r\0a\10\00%\00\00\00\a7\00\00\00\0e\00\00\00library/std/src/sys_common/thread_info.rs\00\00\00\a8\0a\10\00)\00\00\00\16\00\00\003\00\00\00memory allocation of  bytes failed\0a\00\e4\0a\10\00\15\00\00\00\f9\0a\10\00\0e\00\00\00library/std/src/alloc.rs\18\0b\10\00\18\00\00\00R\01\00\00\09\00\00\00library/std/src/panicking.rs@\0b\10\00\1c\00\00\00\11\01\00\00$\00\00\00Box<dyn Any><unnamed>\00\00\00\0d\00\00\00\00\00\00\00\01\00\00\00/\00\00\000\00\00\001\00\00\002\00\00\003\00\00\004\00\00\005\00\00\00!\00\00\00\0c\00\00\00\04\00\00\006\00\00\007\00\00\008\00\00\009\00\00\00:\00\00\00;\00\00\00<\00\00\00thread '' panicked at '', \00\00\d4\0b\10\00\08\00\00\00\dc\0b\10\00\0f\00\00\00\eb\0b\10\00\03\00\00\00]\02\10\00\01\00\00\00note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\0a\00\00\10\0c\10\00N\00\00\00@\0b\10\00\1c\00\00\00F\02\00\00\1f\00\00\00@\0b\10\00\1c\00\00\00G\02\00\00\1e\00\00\00!\00\00\00\0c\00\00\00\04\00\00\00=\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00>\00\00\00?\00\00\00\10\00\00\00\04\00\00\00@\00\00\00A\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00B\00\00\00C\00\00\00thread panicked while processing panic. aborting.\0a\00\00\d0\0c\10\002\00\00\00\0apanicked after panic::always_abort(), aborting.\0a\00\00\00\d8\00\10\00\00\00\00\00\0c\0d\10\001\00\00\00thread panicked while panicking. aborting.\0a\00P\0d\10\00+\00\00\00failed to initiate panic, error \84\0d\10\00 \00\00\00advancing IoSlice beyond its length\00\ac\0d\10\00#\00\00\00library/std/src/sys/wasi/io.rs\00\00\d8\0d\10\00\1e\00\00\00\16\00\00\00\0d\00\00\00condvar wait not supported\00\00\08\0e\10\00\1a\00\00\00library/std/src/sys/wasi/../unsupported/locks/condvar.rs,\0e\10\008\00\00\00\14\00\00\00\09\00\00\00cannot recursively acquire mutext\0e\10\00 \00\00\00library/std/src/sys/wasi/../unsupported/locks/mutex.rs\00\00\9c\0e\10\006\00\00\00\17\00\00\00\09\00\00\00rwlock locked for writing\00\00\00\e4\0e\10\00\19\00\00\00strerror_r failure\00\00\08\0f\10\00\12\00\00\00library/std/src/sys/wasi/os.rs\00\00$\0f\10\00\1e\00\00\00/\00\00\00\0d\00\00\00$\0f\10\00\1e\00\00\001\00\00\006\00\00\00$\0f\10\00\1e\00\00\00\ab\00\00\00'\00\00\00$\0f\10\00\1e\00\00\00\ac\00\00\00'\00\00\00\5cx\00\00\84\0f\10\00\02\00\00\00\00\00\00\00 \00\00\00\08\00\00\00\02\00\00\00\00\00\00\00\00\00\00\00\02\00\00\00\03\00\00\00\08\00\0e\00\0f\00?\00\02\00@\005\00\0d\00\04\00\03\00,\00\1b\00\1c\00I\00\14\00\06\004\000\00library/std/src/sys_common/thread_parker/generic.rs\00\d4\0f\10\003\00\00\00'\00\00\00&\00\00\00inconsistent park state\00\18\10\10\00\17\00\00\00\d4\0f\10\003\00\00\005\00\00\00\17\00\00\00park state changed unexpectedly\00H\10\10\00\1f\00\00\00\d4\0f\10\003\00\00\002\00\00\00\11\00\00\00inconsistent state in unpark\80\10\10\00\1c\00\00\00\d4\0f\10\003\00\00\00l\00\00\00\12\00\00\00\d4\0f\10\003\00\00\00z\00\00\00\1f\00\00\00\0e\00\00\00\10\00\00\00\16\00\00\00\15\00\00\00\0b\00\00\00\16\00\00\00\0d\00\00\00\0b\00\00\00\13\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\11\00\00\00\12\00\00\00\10\00\00\00\10\00\00\00\13\00\00\00\12\00\00\00\0d\00\00\00\0e\00\00\00\15\00\00\00\0c\00\00\00\0b\00\00\00\15\00\00\00\15\00\00\00\0f\00\00\00\0e\00\00\00\13\00\00\00&\00\00\008\00\00\00\19\00\00\00\17\00\00\00\0c\00\00\00\09\00\00\00\0a\00\00\00\10\00\00\00\17\00\00\00\19\00\00\00\0e\00\00\00\0d\00\00\00\14\00\00\00\08\00\00\00\1b\00\00\00\ff\04\10\00\ef\04\10\00\d9\04\10\00\c4\04\10\00\b9\04\10\00\a3\04\10\00\96\04\10\00\8b\04\10\00x\04\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00D\07\10\002\07\10\00\22\07\10\00\12\07\10\00\ff\06\10\00\ed\06\10\00\e0\06\10\00\d2\06\10\00\bd\06\10\00\b1\06\10\00\a6\06\10\00\91\06\10\00|\06\10\00m\06\10\00_\06\10\00L\06\10\00&\06\10\00\ee\05\10\00\d5\05\10\00\be\05\10\00\b2\05\10\00\a9\05\10\00\9f\05\10\00\8f\05\10\00x\05\10\00_\05\10\00Q\05\10\00D\05\10\000\05\10\00(\05\10\00\0d\05\10\00/\00Success\00Illegal byte sequence\00Domain error\00Result not representable\00Not a tty\00Permission denied\00Operation not permitted\00No such file or directory\00No such process\00File exists\00Value too large for data type\00No space left on device\00Out of memory\00Resource busy\00Interrupted system call\00Resource temporarily unavailable\00Invalid seek\00Cross-device link\00Read-only file system\00Directory not empty\00Connection reset by peer\00Operation timed out\00Connection refused\00Host is unreachable\00Address in use\00Broken pipe\00I/O error\00No such device or address\00No such device\00Not a directory\00Is a directory\00Text file busy\00Exec format error\00Invalid argument\00Argument list too long\00Symbolic link loop\00Filename too long\00Too many open files in system\00No file descriptors available\00Bad file descriptor\00No child process\00Bad address\00File too large\00Too many links\00No locks available\00Resource deadlock would occur\00State not recoverable\00Previous owner died\00Operation canceled\00Function not implemented\00No message of desired type\00Identifier removed\00Link has been severed\00Protocol error\00Bad message\00Not a socket\00Destination address required\00Message too large\00Protocol wrong type for socket\00Protocol not available\00Protocol not supported\00Not supported\00Address family not supported by protocol\00Address not available\00Network is down\00Network unreachable\00Connection reset by network\00Connection aborted\00No buffer space available\00Socket is connected\00Socket not connected\00Operation already in progress\00Operation in progress\00Stale file handle\00Quota exceeded\00Multihop attempted\00Capabilities insufficient\00\00\00\00\00\00\00\00\00\00\00\00\00u\02N\00\d6\01\e2\04\b9\04\18\01\8e\05\ed\02\16\04\f2\00\97\03\01\038\05\af\01\82\01O\03/\04\1e\00\d4\05\a2\00\12\03\1e\03\c2\01\de\03\08\00\ac\05\00\01d\02\f1\01e\054\02\8c\02\cf\02-\03L\04\e3\05\9f\02\f8\04\1c\05\08\05\b1\02K\05\15\02x\00R\02<\03\f1\03\e4\00\c3\03}\04\cc\00\aa\03y\05$\02n\01m\03\22\04\ab\04D\00\fb\01\ae\00\83\03`\00\e5\01\07\04\94\04^\04+\00X\019\01\92\00\c2\05\9b\01C\02F\01\f6\05\00\00called `Option::unwrap()` on a `None` valuelibrary/alloc/src/raw_vec.rscapacity overflow\c3\19\10\00\11\00\00\00\a7\19\10\00\1c\00\00\00\05\02\00\00\05\00\00\00library/alloc/src/ffi/c_str.rs\00\00\ec\19\10\00\1e\00\00\00\1b\01\00\007\00\00\00\00)..\1e\1a\10\00\02\00\00\00BorrowMutErrorindex out of bounds: the len is  but the index is 6\1a\10\00 \00\00\00V\1a\10\00\12\00\00\00called `Option::unwrap()` on a `None` value:\1c\1a\10\00\00\00\00\00\a3\1a\10\00\01\00\00\00\a3\1a\10\00\01\00\00\00K\00\00\00\00\00\00\00\01\00\00\00L\00\00\00panicked at '', \d8\1a\10\00\01\00\00\00\d9\1a\10\00\03\00\00\00\1c\1a\10\00\00\00\00\00matches!===assertion failed: `(left  right)`\0a  left: ``,\0a right: ``: \00\00\00\ff\1a\10\00\19\00\00\00\18\1b\10\00\12\00\00\00*\1b\10\00\0c\00\00\006\1b\10\00\03\00\00\00`\00\00\00\ff\1a\10\00\19\00\00\00\18\1b\10\00\12\00\00\00*\1b\10\00\0c\00\00\00\5c\1b\10\00\01\00\00\00: \00\00\1c\1a\10\00\00\00\00\00\80\1b\10\00\02\00\00\00K\00\00\00\0c\00\00\00\04\00\00\00M\00\00\00N\00\00\00O\00\00\00     {\0a,\0a,  { ..\0a}, .. } { .. } }(\0a(,\00\00\00K\00\00\00\04\00\00\00\04\00\00\00P\00\00\000x00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899\00\00K\00\00\00\04\00\00\00\04\00\00\00Q\00\00\00R\00\00\00S\00\00\00truefalserange start index  out of range for slice of length \00\00\00\d1\1c\10\00\12\00\00\00\e3\1c\10\00\22\00\00\00library/core/src/slice/index.rs\00\18\1d\10\00\1f\00\00\004\00\00\00\05\00\00\00range end index H\1d\10\00\10\00\00\00\e3\1c\10\00\22\00\00\00\18\1d\10\00\1f\00\00\00I\00\00\00\05\00\00\00slice index starts at  but ends at \00x\1d\10\00\16\00\00\00\8e\1d\10\00\0d\00\00\00\18\1d\10\00\1f\00\00\00\5c\00\00\00\05\00\00\00\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\04\04\04\04\04\00\00\00\00\00\00\00\00\00\00\00library/core/src/str/mod.rs[...]byte index  is out of bounds of `\00\00\00\dc\1e\10\00\0b\00\00\00\e7\1e\10\00\16\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00k\00\00\00\09\00\00\00begin <= end ( <= ) when slicing `\00\00(\1f\10\00\0e\00\00\006\1f\10\00\04\00\00\00:\1f\10\00\10\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00o\00\00\00\05\00\00\00\bc\1e\10\00\1b\00\00\00}\00\00\00-\00\00\00 is not a char boundary; it is inside  (bytes ) of `\dc\1e\10\00\0b\00\00\00\8c\1f\10\00&\00\00\00\b2\1f\10\00\08\00\00\00\ba\1f\10\00\06\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00\7f\00\00\00\05\00\00\00library/core/src/unicode/printable.rs\00\00\00\f8\1f\10\00%\00\00\00\1a\00\00\006\00\00\00\00\01\03\05\05\06\06\02\07\06\08\07\09\11\0a\1c\0b\19\0c\1a\0d\10\0e\0d\0f\04\10\03\12\12\13\09\16\01\17\04\18\01\19\03\1a\07\1b\01\1c\02\1f\16 \03+\03-\0b.\010\031\022\01\a7\02\a9\02\aa\04\ab\08\fa\02\fb\05\fd\02\fe\03\ff\09\adxy\8b\8d\a20WX\8b\8c\90\1c\dd\0e\0fKL\fb\fc./?\5c]_\e2\84\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\ff\00\04\11\12)147:;=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91\9b\9d\c9\ce\cf\0d\11):;EIW[\5c^_de\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\0d\11EIde\80\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\8b\a4\a6\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfINOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7\11\16\17[\5c\f6\f7\fe\ff\80mq\de\df\0e\1fno\1c\1d_}~\ae\af\7f\bb\bc\16\17\1e\1fFGNOXZ\5c^~\7f\b5\c5\d4\d5\dc\f0\f1\f5rs\8ftu\96&./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\980\8f\1f\d2\d4\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91Sgu\c8\c9\d0\d1\d8\d9\e7\fe\ff\00 _\22\82\df\04\82D\08\1b\04\06\11\81\ac\0e\80\ab\05\1f\09\81\1b\03\19\08\01\04/\044\04\07\03\01\07\06\07\11\0aP\0f\12\07U\07\03\04\1c\0a\09\03\08\03\07\03\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05N\07\1b\07W\07\02\06\16\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%_ m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\16\09\18\09\14\0c\14\0cj\06\0a\06\1a\06Y\07+\05F\0a,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\0a\06/1M\03\80\a4\08<\03\0f\03<\078\08+\05\82\ff\11\18\08/\11-\03!\0f!\0f\80\8c\04\82\97\19\0b\15\88\94\05/\05;\07\02\0e\18\09\80\be\22t\0c\80\d6\1a\0c\05\80\ff\05\80\df\0c\f2\9d\037\09\81\5c\14\80\b8\08\80\cb\05\0a\18;\03\0a\068\08F\08\0c\06t\0b\1e\03Z\04Y\09\80\83\18\1c\0a\16\09L\04\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a6\10\81\f5\07\01 *\06L\04\80\8d\04\80\be\03\1b\03\0f\0d\00\06\01\01\03\01\04\02\05\07\07\02\08\08\09\02\0a\05\0b\02\0e\04\10\01\11\02\12\05\13\11\14\01\15\02\17\02\19\0d\1c\05\1d\08$\01j\04k\02\af\03\bc\02\cf\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e1\02\e7\04\e8\02\ee \f0\04\f8\02\fa\02\fb\01\0c';>NO\8f\9e\9e\9f{\8b\93\96\a2\b2\ba\86\b1\06\07\096=>V\f3\d0\d1\04\14\1867VW\7f\aa\ae\af\bd5\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:EFIJNOde\5c\b6\b7\1b\1c\07\08\0a\0b\14\1769:\a8\a9\d8\d9\097\90\91\a8\07\0a;>fi\8f\92o_\bf\ee\efZb\f4\fc\ff\9a\9b./'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\22%>?\e7\ec\ef\ff\c5\c6\04 #%&(38:HJLPSUVXZ\5c^`cefksx}\7f\8a\a4\aa\af\b0\c0\d0\ae\afno\93^\22{\05\03\04-\03f\03\01/.\80\82\1d\031\0f\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0bNC\817\09\16\0a\08\18;E9\03c\08\090\16\05!\03\1b\05\01@8\04K\05/\04\0a\07\09\07@ '\04\0c\096\03:\05\1a\07\04\0c\07PI73\0d3\07.\08\0a\81&RN(\08*\16\1a&\1c\14\17\09N\04$\09D\0d\19\07\0a\06H\08'\09u\0b?A*\06;\05\0a\06Q\06\01\05\10\03\05\80\8bb\1eH\08\0a\80\a6^\22E\0b\0a\06\0d\13:\06\0a6,\04\17\80\b9<dS\0cH\09\0aFE\1bH\08S\0dI\81\07F\0a\1d\03GI7\03\0e\08\0a\069\07\0a\816\19\80\b7\01\0f2\0d\83\9bfu\0b\80\c4\8aLc\0d\84/\8f\d1\82G\a1\b9\829\07*\04\5c\06&\0aF\0a(\05\13\82\b0[eK\049\07\11@\05\0b\02\0e\97\f8\08\84\d6*\09\a2\e7\813-\03\11\04\08\81\8c\89\04k\05\0d\03\09\07\10\92`G\09t<\80\f6\0as\08p\15F\80\9a\14\0cW\09\19\80\87\81G\03\85B\0f\15\84P\1f\80\e1+\80\d5-\03\1a\04\02\81@\1f\11:\05\01\84\e0\80\f7)L\04\0a\04\02\83\11DL=\80\c2<\06\01\04U\05\1b4\02\81\0e,\04d\0cV\0a\80\ae8\1d\0d,\04\09\07\02\0e\06\80\9a\83\d8\05\10\03\0d\03t\0cY\07\0c\04\01\0f\0c\048\08\0a\06(\08\22N\81T\0c\15\03\05\03\07\09\1d\03\0b\05\06\0a\0a\06\08\08\07\09\80\cb%\0a\84\06library/core/src/unicode/unicode_data.rs\00\00\00\99%\10\00(\00\00\00K\00\00\00(\00\00\00\99%\10\00(\00\00\00W\00\00\00\16\00\00\00\99%\10\00(\00\00\00R\00\00\00>\00\00\00SomeNoneK\00\00\00\04\00\00\00\04\00\00\00T\00\00\00Utf8Errorvalid_up_toerror_len\00\00\00K\00\00\00\04\00\00\00\04\00\00\00U\00\00\00\00\03\00\00\83\04 \00\91\05`\00]\13\a0\00\12\17 \1f\0c `\1f\ef,\a0+*0 ,o\a6\e0,\02\a8`-\1e\fb`.\00\fe 6\9e\ff`6\fd\01\e16\01\0a!7$\0d\e17\ab\0ea9/\18\a190\1c\e1G\f3\1e!L\f0j\e1OOo!P\9d\bc\a1P\00\cfaQe\d1\a1Q\00\da!R\00\e0\e1S0\e1aU\ae\e2\a1V\d0\e8\e1V \00nW\f0\01\ffW\00p\00\07\00-\01\01\01\02\01\02\01\01H\0b0\15\10\01e\07\02\06\02\02\01\04#\01\1e\1b[\0b:\09\09\01\18\04\01\09\01\03\01\05+\03<\08*\18\01 7\01\01\01\04\08\04\01\03\07\0a\02\1d\01:\01\01\01\02\04\08\01\09\01\0a\02\1a\01\02\029\01\04\02\04\02\02\03\03\01\1e\02\03\01\0b\029\01\04\05\01\02\04\01\14\02\16\06\01\01:\01\01\02\01\04\08\01\07\03\0a\02\1e\01;\01\01\01\0c\01\09\01(\01\03\017\01\01\03\05\03\01\04\07\02\0b\02\1d\01:\01\02\01\02\01\03\01\05\02\07\02\0b\02\1c\029\02\01\01\02\04\08\01\09\01\0a\02\1d\01H\01\04\01\02\03\01\01\08\01Q\01\02\07\0c\08b\01\02\09\0b\06J\02\1b\01\01\01\01\017\0e\01\05\01\02\05\0b\01$\09\01f\04\01\06\01\02\02\02\19\02\04\03\10\04\0d\01\02\02\06\01\0f\01\00\03\00\03\1d\02\1e\02\1e\02@\02\01\07\08\01\02\0b\09\01-\03\01\01u\02\22\01v\03\04\02\09\01\06\03\db\02\02\01:\01\01\07\01\01\01\01\02\08\06\0a\02\010\1f1\040\07\01\01\05\01(\09\0c\02 \04\02\02\01\038\01\01\02\03\01\01\03:\08\02\02\98\03\01\0d\01\07\04\01\06\01\03\02\c6@\00\01\c3!\00\03\8d\01` \00\06i\02\00\04\01\0a \02P\02\00\01\03\01\04\01\19\02\05\01\97\02\1a\12\0d\01&\08\19\0b.\030\01\02\04\02\02'\01C\06\02\02\02\02\0c\01\08\01/\013\01\01\03\02\02\05\02\01\01*\02\08\01\ee\01\02\01\04\01\00\01\00\10\10\10\00\02\00\01\e2\01\95\05\00\03\01\02\05\04(\03\04\01\a5\02\00\04\00\02\99\0b1\04{\016\0f)\01\02\02\0a\031\04\02\02\07\01=\03$\05\01\08>\01\0c\024\09\0a\04\02\01_\03\02\01\01\02\06\01\a0\01\03\08\15\029\02\01\01\01\01\16\01\0e\07\03\05\c3\08\02\03\01\01\17\01Q\01\02\06\01\01\02\01\01\02\01\02\eb\01\02\04\06\02\01\02\1b\02U\08\02\01\01\02j\01\01\01\02\06\01\01e\03\02\04\01\05\00\09\01\02\f5\01\0a\02\01\01\04\01\90\04\02\02\04\01 \0a(\06\02\04\08\01\09\06\02\03.\0d\01\02\00\07\01\06\01\01R\16\02\07\01\02\01\02z\06\03\01\01\02\01\07\01\01H\02\03\01\01\01\00\02\00\05;\07\00\01?\04Q\01\00\02\00.\02\17\00\01\01\03\04\05\08\08\02\07\1e\04\94\03\007\042\08\01\0e\01\16\05\01\0f\00\07\01\11\02\07\01\02\01\05\00\07\00\01=\04\00\07m\07\00`\80\f0\00")
  (data $.data (i32.const 1059200) "\01\00\00\00\00\00\00\00\01\00\00\00\c4\12\10\00"))
