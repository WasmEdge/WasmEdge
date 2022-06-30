(module
  (type $t0 (func (param i32)))
  (type $t1 (func (param i32) (result i64)))
  (type $t2 (func (param i32 i32)))
  (type $t3 (func (param i32 i32) (result i32)))
  (type $t4 (func (param i32 i32 i32)))
  (type $t5 (func (param i32 i32 i32) (result i32)))
  (type $t6 (func (param i32 i32 i32 i32) (result i32)))
  (type $t7 (func))
  (type $t8 (func (param i32 i32 i32 i32)))
  (type $t9 (func (result i32)))
  (type $t10 (func (param i32) (result i32)))
  (type $t11 (func (param i32 i32 i32 i32 i32)))
  (type $t12 (func (param i32 i32 i32 i32 i32) (result i32)))
  (type $t13 (func (param i32 i32 i32 i32 i32 i32 i32)))
  (type $t14 (func (param i32 i32 i32 i32 i32 i32) (result i32)))
  (type $t15 (func (param i32 i32 i32 i32 i32 i32 i32) (result i32)))
  (type $t16 (func (param i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "args_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hfe12d9463b2a7790E (type $t3)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h19746ec286063fe2E (type $t3)))
  (import "wasi_snapshot_preview1" "fd_write" (func $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h2fb5d28ef433e1b2E (type $t6)))
  (import "wasi_snapshot_preview1" "environ_get" (func $__imported_wasi_snapshot_preview1_environ_get (type $t3)))
  (import "wasi_snapshot_preview1" "environ_sizes_get" (func $__imported_wasi_snapshot_preview1_environ_sizes_get (type $t3)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $__imported_wasi_snapshot_preview1_proc_exit (type $t0)))
  (func $__wasm_call_ctors (type $t7)
    call $__wasilibc_initialize_environ_eagerly)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.const 8
    i32.add
    i32.load
    local.get $p1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $print_env (type $t7)
    (local $l0 i32) (local $l1 i32) (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l0
    global.set $g0
    local.get $l0
    i32.const 124
    i32.add
    i32.const 0
    i32.store
    local.get $l0
    i32.const 1048576
    i32.store offset=120
    local.get $l0
    i64.const 1
    i64.store offset=108 align=4
    local.get $l0
    i32.const 1048608
    i32.store offset=104
    local.get $l0
    i32.const 104
    i32.add
    call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
    local.get $l0
    call $_ZN3std3env4vars17h6ec7390381e46b70E
    local.get $l0
    i32.const 16
    i32.add
    i32.const 8
    i32.add
    local.get $l0
    i32.const 8
    i32.add
    i64.load
    i64.store
    local.get $l0
    local.get $l0
    i64.load
    i64.store offset=16
    local.get $l0
    i32.const 32
    i32.add
    local.get $l0
    i32.const 16
    i32.add
    call $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE
    block $B0
      local.get $l0
      i32.load offset=32
      i32.eqz
      br_if $B0
      local.get $l0
      i32.const 44
      i32.add
      local.set $l1
      loop $L1
        local.get $l0
        i32.const 56
        i32.add
        i32.const 8
        i32.add
        local.get $l0
        i32.const 32
        i32.add
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get $l0
        local.get $l0
        i64.load offset=32
        i64.store offset=56
        local.get $l0
        i32.const 72
        i32.add
        i32.const 8
        i32.add
        local.get $l1
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get $l0
        local.get $l1
        i64.load align=4
        i64.store offset=72
        local.get $l0
        i32.const 2
        i32.store offset=124
        local.get $l0
        i64.const 3
        i64.store offset=108 align=4
        local.get $l0
        i32.const 1048620
        i32.store offset=104
        local.get $l0
        i32.const 1
        i32.store offset=100
        local.get $l0
        i32.const 1
        i32.store offset=92
        local.get $l0
        local.get $l0
        i32.const 88
        i32.add
        i32.store offset=120
        local.get $l0
        local.get $l0
        i32.const 72
        i32.add
        i32.store offset=96
        local.get $l0
        local.get $l0
        i32.const 56
        i32.add
        i32.store offset=88
        local.get $l0
        i32.const 104
        i32.add
        call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
        block $B2
          local.get $l0
          i32.load offset=76
          local.tee $l2
          i32.eqz
          br_if $B2
          local.get $l0
          i32.load offset=72
          local.get $l2
          i32.const 1
          call $__rust_dealloc
        end
        block $B3
          local.get $l0
          i32.load offset=60
          local.tee $l2
          i32.eqz
          br_if $B3
          local.get $l0
          i32.load offset=56
          local.get $l2
          i32.const 1
          call $__rust_dealloc
        end
        local.get $l0
        i32.const 32
        i32.add
        local.get $l0
        i32.const 16
        i32.add
        call $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE
        local.get $l0
        i32.load offset=32
        br_if $L1
      end
    end
    local.get $l0
    i32.load offset=28
    local.get $l0
    i32.load offset=24
    local.tee $l1
    i32.sub
    local.tee $l2
    i32.const 24
    i32.div_u
    local.set $l3
    block $B4
      local.get $l2
      i32.eqz
      br_if $B4
      local.get $l3
      i32.const 24
      i32.mul
      local.set $l2
      loop $L5
        block $B6
          local.get $l1
          i32.const 4
          i32.add
          i32.load
          local.tee $l3
          i32.eqz
          br_if $B6
          local.get $l1
          i32.load
          local.get $l3
          i32.const 1
          call $__rust_dealloc
        end
        block $B7
          local.get $l1
          i32.const 16
          i32.add
          i32.load
          local.tee $l3
          i32.eqz
          br_if $B7
          local.get $l1
          i32.const 12
          i32.add
          i32.load
          local.get $l3
          i32.const 1
          call $__rust_dealloc
        end
        local.get $l1
        i32.const 24
        i32.add
        local.set $l1
        local.get $l2
        i32.const -24
        i32.add
        local.tee $l2
        br_if $L5
      end
    end
    block $B8
      local.get $l0
      i32.load offset=20
      local.tee $l1
      i32.eqz
      br_if $B8
      local.get $l0
      i32.load offset=16
      local.get $l1
      i64.extend_i32_u
      i64.const 24
      i64.mul
      i32.wrap_i64
      i32.const 4
      call $__rust_dealloc
    end
    local.get $l0
    i32.const 124
    i32.add
    i32.const 0
    i32.store
    local.get $l0
    i32.const 1048576
    i32.store offset=120
    local.get $l0
    i64.const 1
    i64.store offset=108 align=4
    local.get $l0
    i32.const 1048672
    i32.store offset=104
    local.get $l0
    i32.const 104
    i32.add
    call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
    local.get $l0
    i32.const 88
    i32.add
    call $_ZN3std3env4args17h02d9a592c6ab5652E
    local.get $l0
    i32.const 32
    i32.add
    i32.const 8
    i32.add
    local.get $l0
    i32.const 88
    i32.add
    i32.const 8
    i32.add
    i64.load
    i64.store
    local.get $l0
    local.get $l0
    i64.load offset=88
    i64.store offset=32
    local.get $l0
    local.get $l0
    i32.const 32
    i32.add
    call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E
    block $B9
      local.get $l0
      i32.load
      i32.eqz
      br_if $B9
      loop $L10
        local.get $l0
        i32.const 16
        i32.add
        i32.const 8
        i32.add
        local.get $l0
        i32.const 8
        i32.add
        i32.load
        i32.store
        local.get $l0
        local.get $l0
        i64.load
        i64.store offset=16
        local.get $l0
        i32.const 1
        i32.store offset=124
        local.get $l0
        i64.const 2
        i64.store offset=108 align=4
        local.get $l0
        i32.const 1048680
        i32.store offset=104
        local.get $l0
        i32.const 1
        i32.store offset=76
        local.get $l0
        local.get $l0
        i32.const 72
        i32.add
        i32.store offset=120
        local.get $l0
        local.get $l0
        i32.const 16
        i32.add
        i32.store offset=72
        local.get $l0
        i32.const 104
        i32.add
        call $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE
        block $B11
          local.get $l0
          i32.load offset=20
          local.tee $l1
          i32.eqz
          br_if $B11
          local.get $l0
          i32.load offset=16
          local.get $l1
          i32.const 1
          call $__rust_dealloc
        end
        local.get $l0
        local.get $l0
        i32.const 32
        i32.add
        call $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E
        local.get $l0
        i32.load
        br_if $L10
      end
    end
    local.get $l0
    i32.load offset=44
    local.get $l0
    i32.load offset=40
    local.tee $l1
    i32.sub
    local.tee $l2
    i32.const 12
    i32.div_u
    local.set $l3
    block $B12
      local.get $l2
      i32.eqz
      br_if $B12
      local.get $l3
      i32.const 12
      i32.mul
      local.set $l2
      loop $L13
        block $B14
          local.get $l1
          i32.const 4
          i32.add
          i32.load
          local.tee $l3
          i32.eqz
          br_if $B14
          local.get $l1
          i32.load
          local.get $l3
          i32.const 1
          call $__rust_dealloc
        end
        local.get $l1
        i32.const 12
        i32.add
        local.set $l1
        local.get $l2
        i32.const -12
        i32.add
        local.tee $l2
        br_if $L13
      end
    end
    block $B15
      local.get $l0
      i32.load offset=36
      local.tee $l1
      i32.eqz
      br_if $B15
      local.get $l0
      i32.load offset=32
      local.get $l1
      i64.extend_i32_u
      i64.const 12
      i64.mul
      i32.wrap_i64
      i32.const 4
      call $__rust_dealloc
    end
    local.get $l0
    i32.const 128
    i32.add
    global.set $g0)
  (func $__rust_alloc (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    local.get $p0
    local.get $p1
    call $__rdl_alloc
    local.set $l2
    local.get $l2
    return)
  (func $__rust_dealloc (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    local.get $p2
    call $__rdl_dealloc
    return)
  (func $__rust_realloc (type $t6) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (result i32)
    (local $l4 i32)
    local.get $p0
    local.get $p1
    local.get $p2
    local.get $p3
    call $__rdl_realloc
    local.set $l4
    local.get $l4
    return)
  (func $__rust_alloc_error_handler (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $__rg_oom
    return)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2d0c1ef8155b3364E (type $t1) (param $p0 i32) (result i64)
    i64.const 7628363314155272196)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h5d3c77c4d604c6d6E (type $t1) (param $p0 i32) (result i64)
    i64.const -887290134024487584)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h83af4092997449d4E (type $t1) (param $p0 i32) (result i64)
    i64.const -5139102199292759541)
  (func $_ZN66_$LT$std..sys..wasi..os_str..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hb769d13a4a71030dE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i64)
    global.get $g0
    i32.const 96
    i32.sub
    local.tee $l3
    global.set $g0
    i32.const 1
    local.set $l4
    block $B0
      local.get $p2
      i32.const 1049436
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE
      br_if $B0
      local.get $l3
      i32.const 8
      i32.add
      local.get $p0
      local.get $p1
      call $_ZN4core3str5lossy9Utf8Lossy10from_bytes17h9d8ade7d6641c0d4E
      local.get $l3
      local.get $l3
      i32.load offset=8
      local.get $l3
      i32.load offset=12
      call $_ZN4core3str5lossy9Utf8Lossy6chunks17hdeccf1496c5995d4E
      local.get $l3
      local.get $l3
      i64.load
      i64.store offset=16
      local.get $l3
      i32.const 24
      i32.add
      local.get $l3
      i32.const 16
      i32.add
      call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E
      block $B1
        local.get $l3
        i32.load offset=24
        local.tee $l4
        i32.eqz
        br_if $B1
        local.get $l3
        i32.const 48
        i32.add
        local.set $l5
        local.get $l3
        i32.const 40
        i32.add
        i32.const 24
        i32.add
        local.set $l6
        loop $L2
          local.get $l3
          i32.load offset=36
          local.set $l7
          local.get $l3
          i32.load offset=32
          local.set $p1
          local.get $l3
          i32.load offset=28
          local.set $p0
          local.get $l3
          i32.const 4
          i32.store offset=64
          local.get $l3
          i32.const 4
          i32.store offset=48
          local.get $l3
          local.get $l4
          i32.store offset=40
          local.get $l3
          local.get $l4
          local.get $p0
          i32.add
          i32.store offset=44
          i32.const 4
          local.set $l4
          block $B3
            loop $L4
              block $B5
                block $B6
                  block $B7
                    block $B8
                      block $B9
                        block $B10
                          local.get $l4
                          i32.const 4
                          i32.eq
                          br_if $B10
                          local.get $l5
                          call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E
                          local.tee $l4
                          i32.const 1114112
                          i32.ne
                          br_if $B9
                          local.get $l3
                          i32.const 4
                          i32.store offset=48
                        end
                        block $B11
                          block $B12
                            block $B13
                              block $B14
                                block $B15
                                  block $B16
                                    block $B17
                                      block $B18
                                        local.get $l3
                                        i32.load offset=40
                                        local.tee $l4
                                        i32.eqz
                                        br_if $B18
                                        local.get $l3
                                        i32.load offset=44
                                        local.get $l4
                                        i32.eq
                                        br_if $B18
                                        local.get $l3
                                        local.get $l4
                                        i32.const 1
                                        i32.add
                                        i32.store offset=40
                                        block $B19
                                          local.get $l4
                                          i32.load8_u
                                          local.tee $p0
                                          i32.const 24
                                          i32.shl
                                          i32.const 24
                                          i32.shr_s
                                          i32.const -1
                                          i32.gt_s
                                          br_if $B19
                                          local.get $l3
                                          local.get $l4
                                          i32.const 2
                                          i32.add
                                          i32.store offset=40
                                          local.get $l4
                                          i32.load8_u offset=1
                                          i32.const 63
                                          i32.and
                                          local.set $l8
                                          local.get $p0
                                          i32.const 31
                                          i32.and
                                          local.set $l9
                                          block $B20
                                            local.get $p0
                                            i32.const 223
                                            i32.gt_u
                                            br_if $B20
                                            local.get $l9
                                            i32.const 6
                                            i32.shl
                                            local.get $l8
                                            i32.or
                                            local.set $p0
                                            br $B19
                                          end
                                          local.get $l3
                                          local.get $l4
                                          i32.const 3
                                          i32.add
                                          i32.store offset=40
                                          local.get $l8
                                          i32.const 6
                                          i32.shl
                                          local.get $l4
                                          i32.load8_u offset=2
                                          i32.const 63
                                          i32.and
                                          i32.or
                                          local.set $l8
                                          block $B21
                                            local.get $p0
                                            i32.const 240
                                            i32.ge_u
                                            br_if $B21
                                            local.get $l8
                                            local.get $l9
                                            i32.const 12
                                            i32.shl
                                            i32.or
                                            local.set $p0
                                            br $B19
                                          end
                                          local.get $l3
                                          local.get $l4
                                          i32.const 4
                                          i32.add
                                          i32.store offset=40
                                          local.get $l8
                                          i32.const 6
                                          i32.shl
                                          local.get $l4
                                          i32.load8_u offset=3
                                          i32.const 63
                                          i32.and
                                          i32.or
                                          local.get $l9
                                          i32.const 18
                                          i32.shl
                                          i32.const 1835008
                                          i32.and
                                          i32.or
                                          local.set $p0
                                        end
                                        i32.const 48
                                        local.set $l8
                                        i32.const 2
                                        local.set $l4
                                        block $B22
                                          local.get $p0
                                          br_table $B5 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B12 $B16 $B15 $B15 $B17 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B14 $B15 $B15 $B15 $B15 $B14 $B22
                                        end
                                        local.get $p0
                                        i32.const 92
                                        i32.eq
                                        br_if $B14
                                        local.get $p0
                                        i32.const 1114112
                                        i32.ne
                                        br_if $B15
                                      end
                                      block $B23
                                        local.get $l3
                                        i32.load offset=64
                                        i32.const 4
                                        i32.eq
                                        br_if $B23
                                        local.get $l6
                                        call $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E
                                        local.tee $l4
                                        i32.const 1114112
                                        i32.ne
                                        br_if $B9
                                      end
                                      local.get $l7
                                      br_if $B8
                                      br $B3
                                    end
                                    i32.const 114
                                    local.set $l8
                                    br $B11
                                  end
                                  i32.const 110
                                  local.set $l8
                                  br $B11
                                end
                                block $B24
                                  local.get $p0
                                  call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E
                                  i32.eqz
                                  br_if $B24
                                  local.get $p0
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
                                  local.set $l10
                                  br $B6
                                end
                                i32.const 1
                                local.set $l4
                                local.get $p0
                                call $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE
                                i32.eqz
                                br_if $B13
                              end
                              local.get $p0
                              local.set $l8
                              br $B11
                            end
                            local.get $p0
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
                            local.set $l10
                            br $B6
                          end
                          i32.const 116
                          local.set $l8
                        end
                        br $B5
                      end
                      local.get $p2
                      local.get $l4
                      call $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h968628441d2ac37cE
                      br_if $B7
                      local.get $l3
                      i32.load offset=48
                      local.set $l4
                      br $L4
                    end
                    loop $L25
                      local.get $l3
                      local.get $p1
                      i32.store offset=84
                      local.get $l3
                      i32.const 1
                      i32.store offset=60
                      local.get $l3
                      i32.const 1
                      i32.store offset=52
                      local.get $l3
                      i32.const 1052560
                      i32.store offset=48
                      local.get $l3
                      i32.const 1
                      i32.store offset=44
                      local.get $l3
                      i32.const 1052552
                      i32.store offset=40
                      local.get $l3
                      i32.const 2
                      i32.store offset=92
                      local.get $l3
                      local.get $l3
                      i32.const 88
                      i32.add
                      i32.store offset=56
                      local.get $l3
                      local.get $l3
                      i32.const 84
                      i32.add
                      i32.store offset=88
                      local.get $p2
                      local.get $l3
                      i32.const 40
                      i32.add
                      call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
                      br_if $B7
                      local.get $p1
                      i32.const 1
                      i32.add
                      local.set $p1
                      local.get $l7
                      i32.const -1
                      i32.add
                      local.tee $l7
                      i32.eqz
                      br_if $B3
                      br $L25
                    end
                  end
                  i32.const 1
                  local.set $l4
                  br $B0
                end
                i32.const 3
                local.set $l4
                local.get $p0
                local.set $l8
              end
              local.get $l3
              local.get $l10
              i64.store offset=56
              local.get $l3
              local.get $l8
              i32.store offset=52
              local.get $l3
              local.get $l4
              i32.store offset=48
              br $L4
            end
          end
          local.get $l3
          i32.const 24
          i32.add
          local.get $l3
          i32.const 16
          i32.add
          call $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E
          local.get $l3
          i32.load offset=24
          local.tee $l4
          br_if $L2
        end
      end
      local.get $p2
      i32.const 1049436
      i32.const 1
      call $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE
      local.set $l4
    end
    local.get $l3
    i32.const 96
    i32.add
    global.set $g0
    local.get $l4)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h51406901289b8250E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN43_$LT$bool$u20$as$u20$core..fmt..Display$GT$3fmt17h107f9e8c546bfafcE)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h72c85046dd08428aE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.set $p0
    block $B0
      local.get $p1
      call $_ZN4core3fmt9Formatter15debug_lower_hex17h633f97836530945dE
      br_if $B0
      block $B1
        local.get $p1
        call $_ZN4core3fmt9Formatter15debug_upper_hex17h565dd489bc473806E
        br_if $B1
        local.get $p0
        local.get $p1
        call $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E
        return
      end
      local.get $p0
      local.get $p1
      call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17hfe79af84422ce847E
      return
    end
    local.get $p0
    local.get $p1
    call $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17hb5f1486a0d6bf05cE)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h32abcc04c39e85a4E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN70_$LT$core..panic..location..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h679c135f41a338baE)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17hf8285e92300923c0E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.load offset=4
    local.get $p1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h547981a8fa47a7d3E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17hb78061cd9b3bd312E)
  (func $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 0
    i32.store offset=12
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=12
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set $p1
    end
    block $B4
      local.get $p0
      i32.load
      local.tee $l3
      i32.const 4
      i32.add
      i32.load
      local.get $l3
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $p0
      i32.sub
      local.get $p1
      i32.ge_u
      br_if $B4
      local.get $l3
      local.get $p0
      local.get $p1
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $p0
    end
    local.get $l3
    i32.load
    local.get $p0
    i32.add
    local.get $l2
    i32.const 12
    i32.add
    local.get $p1
    call $memcpy
    drop
    local.get $l4
    local.get $p0
    local.get $p1
    i32.add
    i32.store
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    i32.const 0)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    block $B0
      local.get $p1
      local.get $p2
      i32.add
      local.tee $p2
      local.get $p1
      i32.lt_u
      br_if $B0
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $l4
      i32.const 1
      i32.shl
      local.tee $p1
      local.get $p2
      local.get $p1
      local.get $p2
      i32.gt_u
      select
      local.tee $p1
      i32.const 8
      local.get $p1
      i32.const 8
      i32.gt_u
      select
      local.set $p1
      block $B1
        block $B2
          local.get $l4
          br_if $B2
          i32.const 0
          local.set $p2
          br $B1
        end
        local.get $l3
        local.get $l4
        i32.store offset=20
        local.get $l3
        local.get $p0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set $p2
      end
      local.get $l3
      local.get $p2
      i32.store offset=24
      local.get $l3
      local.get $p1
      i32.const 1
      local.get $l3
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block $B3
        local.get $l3
        i32.load
        i32.eqz
        br_if $B3
        local.get $l3
        i32.const 8
        i32.add
        i32.load
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $l3
        i32.load offset=4
        local.get $p0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get $l3
      i32.load offset=4
      local.set $p2
      local.get $p0
      i32.const 4
      i32.add
      local.get $p1
      i32.store
      local.get $p0
      local.get $p2
      i32.store
      local.get $l3
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i64) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 0
    i32.store offset=4
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=6
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=4
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=5
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=4
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=5
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=4
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=7
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=4
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=6
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=5
      i32.const 4
      local.set $p1
    end
    local.get $l2
    i32.const 8
    i32.add
    local.get $p0
    i32.load
    local.get $l2
    i32.const 4
    i32.add
    local.get $p1
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block $B4
      local.get $l2
      i32.load8_u offset=8
      local.tee $p1
      i32.const 4
      i32.eq
      br_if $B4
      local.get $l2
      i64.load offset=8
      local.set $l3
      block $B5
        local.get $p0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if $B5
        local.get $p0
        i32.const 8
        i32.add
        i32.load
        local.tee $l4
        i32.load
        local.get $l4
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B6
          local.get $l4
          i32.load offset=4
          local.tee $l5
          i32.load offset=4
          local.tee $l6
          i32.eqz
          br_if $B6
          local.get $l4
          i32.load
          local.get $l6
          local.get $l5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $l4
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get $p0
      local.get $l3
      i64.store offset=4 align=4
    end
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1
    i32.const 4
    i32.ne)
  (func $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i64) (local $l9 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p1
              i32.load
              local.tee $p1
              i32.load offset=8
              br_if $B4
              local.get $p1
              i32.const -1
              i32.store offset=8
              local.get $l4
              i32.const 10
              local.get $p2
              local.get $p3
              call $_ZN4core5slice6memchr7memrchr17heb9113f3074b2e71E
              local.get $p1
              i32.const 12
              i32.add
              local.set $l5
              block $B5
                block $B6
                  local.get $l4
                  i32.load
                  br_if $B6
                  local.get $p1
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee $l6
                  i32.eqz
                  br_if $B5
                  local.get $p1
                  i32.load offset=12
                  local.tee $l7
                  i32.eqz
                  br_if $B5
                  local.get $l6
                  local.get $l7
                  i32.add
                  i32.const -1
                  i32.add
                  i32.load8_u
                  i32.const 10
                  i32.ne
                  br_if $B5
                  local.get $l4
                  i32.const 8
                  i32.add
                  local.get $l5
                  call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
                  local.get $l4
                  i32.load8_u offset=8
                  i32.const 4
                  i32.eq
                  br_if $B5
                  local.get $l4
                  i64.load offset=8
                  local.tee $l8
                  i32.wrap_i64
                  i32.const 255
                  i32.and
                  i32.const 4
                  i32.eq
                  br_if $B5
                  local.get $p0
                  local.get $l8
                  i64.store align=4
                  br $B0
                end
                local.get $l4
                i32.load offset=4
                i32.const 1
                i32.add
                local.tee $l6
                local.get $p3
                i32.gt_u
                br_if $B3
                block $B7
                  local.get $p1
                  i32.const 20
                  i32.add
                  i32.load
                  local.tee $l7
                  i32.eqz
                  br_if $B7
                  block $B8
                    local.get $p1
                    i32.const 16
                    i32.add
                    i32.load
                    local.get $l7
                    i32.sub
                    local.get $l6
                    i32.le_u
                    br_if $B8
                    local.get $p1
                    i32.load offset=12
                    local.get $l7
                    i32.add
                    local.get $p2
                    local.get $l6
                    call $memcpy
                    drop
                    local.get $p1
                    i32.const 20
                    i32.add
                    local.get $l7
                    local.get $l6
                    i32.add
                    i32.store
                    br $B2
                  end
                  local.get $l4
                  i32.const 8
                  i32.add
                  local.get $l5
                  local.get $p2
                  local.get $l6
                  call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
                  local.get $l4
                  i32.load8_u offset=8
                  i32.const 4
                  i32.eq
                  br_if $B2
                  local.get $l4
                  i64.load offset=8
                  local.tee $l8
                  i32.wrap_i64
                  i32.const 255
                  i32.and
                  i32.const 4
                  i32.eq
                  br_if $B2
                  local.get $p0
                  local.get $l8
                  i64.store align=4
                  br $B0
                end
                local.get $l4
                i32.const 8
                i32.add
                local.get $l5
                local.get $p2
                local.get $l6
                call $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E
                local.get $l4
                i32.load8_u offset=8
                i32.const 4
                i32.eq
                br_if $B1
                local.get $l4
                i64.load offset=8
                local.tee $l8
                i32.wrap_i64
                i32.const 255
                i32.and
                i32.const 4
                i32.eq
                br_if $B1
                local.get $p0
                local.get $l8
                i64.store align=4
                br $B0
              end
              block $B9
                local.get $p1
                i32.const 16
                i32.add
                i32.load
                local.get $p1
                i32.const 20
                i32.add
                local.tee $l7
                i32.load
                local.tee $l6
                i32.sub
                local.get $p3
                i32.gt_u
                br_if $B9
                local.get $p0
                local.get $l5
                local.get $p2
                local.get $p3
                call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
                br $B0
              end
              local.get $p1
              i32.load offset=12
              local.get $l6
              i32.add
              local.get $p2
              local.get $p3
              call $memcpy
              drop
              local.get $p0
              i32.const 4
              i32.store8
              local.get $l7
              local.get $l6
              local.get $p3
              i32.add
              i32.store
              br $B0
            end
            i32.const 1048792
            i32.const 16
            local.get $l4
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
        local.get $l4
        i32.const 8
        i32.add
        local.get $l5
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
        local.get $l4
        i32.load8_u offset=8
        i32.const 4
        i32.eq
        br_if $B1
        local.get $l4
        i64.load offset=8
        local.tee $l8
        i32.wrap_i64
        i32.const 255
        i32.and
        i32.const 4
        i32.eq
        br_if $B1
        local.get $p0
        local.get $l8
        i64.store align=4
        br $B0
      end
      local.get $p2
      local.get $l6
      i32.add
      local.set $l7
      block $B10
        local.get $p1
        i32.const 16
        i32.add
        i32.load
        local.get $p1
        i32.const 20
        i32.add
        local.tee $l9
        i32.load
        local.tee $p2
        i32.sub
        local.get $p3
        local.get $l6
        i32.sub
        local.tee $p3
        i32.gt_u
        br_if $B10
        local.get $p0
        local.get $l5
        local.get $l7
        local.get $p3
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E
        br $B0
      end
      local.get $p1
      i32.load offset=12
      local.get $p2
      i32.add
      local.get $l7
      local.get $p3
      call $memcpy
      drop
      local.get $p0
      i32.const 4
      i32.store8
      local.get $l9
      local.get $p2
      local.get $p3
      i32.add
      i32.store
    end
    local.get $p1
    local.get $p1
    i32.load offset=8
    i32.const 1
    i32.add
    i32.store offset=8
    local.get $l4
    i32.const 16
    i32.add
    global.set $g0)
  (func $_ZN4core3fmt5Write10write_char17hbe9f53f72de9fff4E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 0
    i32.store offset=12
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=12
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set $p1
    end
    local.get $p0
    local.get $l2
    i32.const 12
    i32.add
    local.get $p1
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E
    local.set $p1
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i64)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    i32.const 0
    local.set $l4
    block $B0
      local.get $p2
      i32.eqz
      br_if $B0
      block $B1
        block $B2
          loop $L3
            local.get $l3
            local.get $p2
            i32.store offset=12
            local.get $l3
            local.get $p1
            i32.store offset=8
            local.get $l3
            i32.const 16
            i32.add
            i32.const 2
            local.get $l3
            i32.const 8
            i32.add
            i32.const 1
            call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
            block $B4
              block $B5
                block $B6
                  local.get $l3
                  i32.load16_u offset=16
                  br_if $B6
                  local.get $l3
                  i32.load offset=20
                  local.tee $l5
                  br_if $B5
                  i32.const 1050536
                  local.set $l5
                  i64.const 2
                  local.set $l6
                  br $B1
                end
                local.get $l3
                local.get $l3
                i32.load16_u offset=18
                i32.store16 offset=30
                local.get $l3
                i32.const 30
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee $l5
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if $B4
                i64.const 0
                local.set $l6
                br $B1
              end
              local.get $p2
              local.get $l5
              i32.lt_u
              br_if $B2
              local.get $p1
              local.get $l5
              i32.add
              local.set $p1
              local.get $p2
              local.get $l5
              i32.sub
              local.set $p2
            end
            local.get $p2
            br_if $L3
            br $B0
          end
        end
        local.get $l5
        local.get $p2
        i32.const 1050692
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get $l5
      i64.extend_i32_u
      i64.const 32
      i64.shl
      local.get $l6
      i64.or
      local.set $l6
      block $B7
        local.get $p0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if $B7
        local.get $p0
        i32.const 8
        i32.add
        i32.load
        local.tee $p2
        i32.load
        local.get $p2
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B8
          local.get $p2
          i32.load offset=4
          local.tee $p1
          i32.load offset=4
          local.tee $l5
          i32.eqz
          br_if $B8
          local.get $p2
          i32.load
          local.get $l5
          local.get $p1
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $p2
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get $p0
      local.get $l6
      i64.store offset=4 align=4
      i32.const 1
      local.set $l4
    end
    local.get $l3
    i32.const 32
    i32.add
    global.set $g0
    local.get $l4)
  (func $_ZN4core3fmt5Write9write_fmt17h8a28623dee2384e2E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048696
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN4core3fmt5Write9write_fmt17h9501b0d1bd73714aE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048720
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN4core3fmt5Write9write_fmt17hcb1cb3e9d099f402E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048744
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN3std9panicking12default_hook17h9e7a62be9c47f791E (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i64) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 96
    i32.sub
    local.tee $l1
    global.set $g0
    i32.const 1
    local.set $l2
    block $B0
      i32.const 0
      i32.load offset=1059304
      i32.const 1
      i32.gt_u
      br_if $B0
      call $_ZN3std5panic19get_backtrace_style17h5dc7902a5c3a80f3E
      i32.const 255
      i32.and
      local.set $l2
    end
    local.get $l1
    local.get $l2
    i32.store8 offset=27
    block $B1
      block $B2
        block $B3
          block $B4
            block $B5
              block $B6
                local.get $p0
                call $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE
                local.tee $l2
                i32.eqz
                br_if $B6
                local.get $l1
                local.get $l2
                i32.store offset=28
                local.get $l1
                i32.const 16
                i32.add
                local.get $p0
                call $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E
                local.get $l1
                i32.load offset=16
                local.tee $l2
                local.get $l1
                i32.load offset=20
                i32.load offset=12
                call_indirect (type $t1) $T0
                local.set $l3
                block $B7
                  block $B8
                    block $B9
                      local.get $l2
                      i32.eqz
                      br_if $B9
                      local.get $l3
                      i64.const -5139102199292759541
                      i64.eq
                      br_if $B8
                    end
                    local.get $l1
                    i32.const 8
                    i32.add
                    local.get $p0
                    call $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E
                    i32.const 1051500
                    local.set $l4
                    i32.const 12
                    local.set $p0
                    local.get $l1
                    i32.load offset=8
                    local.tee $l2
                    local.get $l1
                    i32.load offset=12
                    i32.load offset=12
                    call_indirect (type $t1) $T0
                    local.set $l3
                    block $B10
                      local.get $l2
                      i32.eqz
                      br_if $B10
                      local.get $l3
                      i64.const 7628363314155272196
                      i64.ne
                      br_if $B10
                      local.get $l2
                      i32.const 8
                      i32.add
                      i32.load
                      local.set $p0
                      local.get $l2
                      i32.load
                      local.set $l4
                    end
                    local.get $l1
                    local.get $l4
                    i32.store offset=32
                    br $B7
                  end
                  local.get $l1
                  local.get $l2
                  i32.load
                  i32.store offset=32
                  local.get $l2
                  i32.load offset=4
                  local.set $p0
                end
                local.get $l1
                local.get $p0
                i32.store offset=36
                i32.const 0
                i32.load offset=1059296
                br_if $B5
                i32.const 0
                i32.const -1
                i32.store offset=1059296
                block $B11
                  i32.const 0
                  i32.load offset=1059300
                  local.tee $p0
                  br_if $B11
                  i32.const 0
                  i32.const 0
                  local.get $l1
                  call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                  local.tee $p0
                  i32.store offset=1059300
                end
                local.get $p0
                local.get $p0
                i32.load
                local.tee $l2
                i32.const 1
                i32.add
                i32.store
                local.get $l2
                i32.const -1
                i32.le_s
                br_if $B4
                i32.const 0
                i32.const 0
                i32.load offset=1059296
                i32.const 1
                i32.add
                i32.store offset=1059296
                block $B12
                  block $B13
                    local.get $p0
                    br_if $B13
                    i32.const 0
                    local.set $l2
                    br $B12
                  end
                  local.get $p0
                  i32.const 20
                  i32.add
                  i32.load
                  i32.const -1
                  i32.add
                  local.set $l4
                  local.get $p0
                  i32.load offset=16
                  local.set $l2
                end
                local.get $l1
                local.get $l4
                i32.const 9
                local.get $l2
                select
                i32.store offset=44
                local.get $l1
                local.get $l2
                i32.const 1051512
                local.get $l2
                select
                i32.store offset=40
                local.get $l1
                local.get $l1
                i32.const 27
                i32.add
                i32.store offset=60
                local.get $l1
                local.get $l1
                i32.const 28
                i32.add
                i32.store offset=56
                local.get $l1
                local.get $l1
                i32.const 32
                i32.add
                i32.store offset=52
                local.get $l1
                local.get $l1
                i32.const 40
                i32.add
                i32.store offset=48
                block $B14
                  i32.const 0
                  i32.load8_u offset=1059218
                  i32.eqz
                  br_if $B14
                  i32.const 0
                  i32.const 1
                  i32.store8 offset=1059218
                  block $B15
                    i32.const 0
                    i32.load offset=1059284
                    br_if $B15
                    i32.const 0
                    i64.const 1
                    i64.store offset=1059284 align=4
                    br $B14
                  end
                  i32.const 0
                  i32.load offset=1059288
                  local.set $l2
                  i32.const 0
                  i32.const 0
                  i32.store offset=1059288
                  local.get $l2
                  br_if $B3
                end
                local.get $l1
                i32.const 48
                i32.add
                local.get $l1
                i32.const 72
                i32.add
                i32.const 1051524
                call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
                i32.const 0
                local.set $l4
                i32.const 0
                local.set $l2
                br $B2
              end
              i32.const 1048859
              i32.const 43
              i32.const 1051484
              call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
              unreachable
            end
            i32.const 1048792
            i32.const 16
            local.get $l1
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
        local.get $l2
        i32.load8_u offset=8
        local.set $l4
        local.get $l2
        i32.const 1
        i32.store8 offset=8
        local.get $l1
        local.get $l4
        i32.const 1
        i32.and
        local.tee $l4
        i32.store8 offset=71
        local.get $l4
        br_if $B1
        block $B16
          block $B17
            block $B18
              i32.const 0
              i32.load offset=1059280
              i32.const 2147483647
              i32.and
              br_if $B18
              local.get $l1
              i32.const 48
              i32.add
              local.get $l2
              i32.const 12
              i32.add
              i32.const 1051564
              call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
              br $B17
            end
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            local.set $l4
            local.get $l1
            i32.const 48
            i32.add
            local.get $l2
            i32.const 12
            i32.add
            i32.const 1051564
            call $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE
            local.get $l4
            i32.eqz
            br_if $B16
          end
          i32.const 0
          i32.load offset=1059280
          i32.const 2147483647
          i32.and
          i32.eqz
          br_if $B16
          call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
          br_if $B16
          local.get $l2
          i32.const 1
          i32.store8 offset=9
        end
        i32.const 1
        local.set $l4
        i32.const 0
        i32.const 1
        i32.store8 offset=1059218
        local.get $l2
        i32.const 0
        i32.store8 offset=8
        block $B19
          i32.const 0
          i32.load offset=1059284
          br_if $B19
          i32.const 0
          local.get $l2
          i32.store offset=1059288
          i32.const 1
          local.set $l4
          i32.const 0
          i32.const 1
          i32.store offset=1059284
          br $B2
        end
        i32.const 0
        i32.load offset=1059288
        local.set $l5
        i32.const 0
        local.get $l2
        i32.store offset=1059288
        local.get $l5
        i32.eqz
        br_if $B2
        local.get $l5
        local.get $l5
        i32.load
        local.tee $l6
        i32.const -1
        i32.add
        i32.store
        i32.const 1
        local.set $l4
        local.get $l6
        i32.const 1
        i32.ne
        br_if $B2
        local.get $l5
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
      end
      block $B20
        local.get $p0
        i32.eqz
        br_if $B20
        local.get $p0
        local.get $p0
        i32.load
        local.tee $l5
        i32.const -1
        i32.add
        i32.store
        local.get $l5
        i32.const 1
        i32.ne
        br_if $B20
        local.get $p0
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
      end
      block $B21
        local.get $l4
        i32.const -1
        i32.xor
        local.get $l2
        i32.const 0
        i32.ne
        i32.and
        i32.eqz
        br_if $B21
        local.get $l2
        local.get $l2
        i32.load
        local.tee $p0
        i32.const -1
        i32.add
        i32.store
        local.get $p0
        i32.const 1
        i32.ne
        br_if $B21
        local.get $l2
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
      end
      local.get $l1
      i32.const 96
      i32.add
      global.set $g0
      return
    end
    local.get $l1
    i32.const 92
    i32.add
    i32.const 0
    i32.store
    local.get $l1
    i32.const 88
    i32.add
    i32.const 1048792
    i32.store
    local.get $l1
    i64.const 1
    i64.store offset=76 align=4
    local.get $l1
    i32.const 1052308
    i32.store offset=72
    local.get $l1
    i32.const 71
    i32.add
    local.get $l1
    i32.const 72
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 1049552
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    i32.const 0
    local.get $l2
    i32.const 1048996
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048996
    local.get $l2
    i32.const 8
    i32.add
    i32.const 1052372
    call $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E
    unreachable)
  (func $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17hd87567c1af08f732E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    local.get $p0
    i32.load
    local.tee $l2
    i32.load
    local.set $p0
    local.get $l2
    i32.const 0
    i32.store
    block $B0
      block $B1
        local.get $p0
        i32.eqz
        br_if $B1
        i32.const 1024
        i32.const 1
        call $__rust_alloc
        local.tee $l2
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 0
        i32.store8 offset=28
        local.get $p0
        i32.const 0
        i32.store8 offset=24
        local.get $p0
        i64.const 1024
        i64.store offset=16 align=4
        local.get $p0
        local.get $l2
        i32.store offset=12
        local.get $p0
        i32.const 0
        i32.store offset=8
        local.get $p0
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
  (func $_ZN4core3ptr100drop_in_place$LT$$RF$mut$u20$std..io..Write..write_fmt..Adapter$LT$alloc..vec..Vec$LT$u8$GT$$GT$$GT$17hf1b3c7767b0b7765E (type $t0) (param $p0 i32))
  (func $_ZN4core3ptr103drop_in_place$LT$std..sync..poison..PoisonError$LT$std..sync..mutex..MutexGuard$LT$$LP$$RP$$GT$$GT$$GT$17h342b8354fe7ce8f5E (type $t0) (param $p0 i32)
    (local $l1 i32)
    local.get $p0
    i32.load
    local.set $l1
    block $B0
      local.get $p0
      i32.load8_u offset=4
      br_if $B0
      i32.const 0
      i32.load offset=1059280
      i32.const 2147483647
      i32.and
      i32.eqz
      br_if $B0
      call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
      br_if $B0
      local.get $l1
      i32.const 1
      i32.store8 offset=1
    end
    local.get $l1
    i32.const 0
    i32.store8)
  (func $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE (type $t9) (result i32)
    i32.const 0
    i32.load offset=1059304
    i32.eqz)
  (func $_ZN4core3ptr226drop_in_place$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Send$u2b$core..marker..Sync$GT$$GT$..from..StringError$GT$17h8f80eb1a61eb416eE (type $t0) (param $p0 i32)
    (local $l1 i32)
    block $B0
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $l1
      i32.eqz
      br_if $B0
      local.get $p0
      i32.load
      local.get $l1
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE (type $t0) (param $p0 i32)
    (local $l1 i32)
    block $B0
      local.get $p0
      i32.load offset=16
      local.tee $l1
      i32.eqz
      br_if $B0
      local.get $l1
      i32.const 0
      i32.store8
      local.get $p0
      i32.const 20
      i32.add
      i32.load
      local.tee $l1
      i32.eqz
      br_if $B0
      local.get $p0
      i32.load offset=16
      local.get $l1
      i32.const 1
      call $__rust_dealloc
    end
    block $B1
      local.get $p0
      i32.const -1
      i32.eq
      br_if $B1
      local.get $p0
      local.get $p0
      i32.load offset=4
      local.tee $l1
      i32.const -1
      i32.add
      i32.store offset=4
      local.get $l1
      i32.const 1
      i32.ne
      br_if $B1
      local.get $p0
      i32.const 32
      i32.const 8
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr70drop_in_place$LT$std..panicking..begin_panic_handler..PanicPayload$GT$17ha86c38a5fb14f016E (type $t0) (param $p0 i32)
    (local $l1 i32)
    block $B0
      local.get $p0
      i32.load offset=4
      local.tee $l1
      i32.eqz
      br_if $B0
      local.get $p0
      i32.const 8
      i32.add
      i32.load
      local.tee $p0
      i32.eqz
      br_if $B0
      local.get $l1
      local.get $p0
      i32.const 1
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32)
    block $B0
      local.get $p0
      i32.load8_u
      i32.const 3
      i32.ne
      br_if $B0
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $l1
      i32.load
      local.get $l1
      i32.load offset=4
      i32.load
      call_indirect (type $t0) $T0
      block $B1
        local.get $l1
        i32.load offset=4
        local.tee $l2
        i32.load offset=4
        local.tee $l3
        i32.eqz
        br_if $B1
        local.get $l1
        i32.load
        local.get $l3
        local.get $l2
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get $p0
      i32.load offset=4
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN4core3ptr87drop_in_place$LT$std..io..Write..write_fmt..Adapter$LT$$RF$mut$u20$$u5b$u8$u5d$$GT$$GT$17h9e512fc24cfce3e7E (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32)
    block $B0
      local.get $p0
      i32.load8_u offset=4
      i32.const 3
      i32.ne
      br_if $B0
      local.get $p0
      i32.const 8
      i32.add
      i32.load
      local.tee $l1
      i32.load
      local.get $l1
      i32.load offset=4
      i32.load
      call_indirect (type $t0) $T0
      block $B1
        local.get $l1
        i32.load offset=4
        local.tee $l2
        i32.load offset=4
        local.tee $l3
        i32.eqz
        br_if $B1
        local.get $l1
        i32.load
        local.get $l3
        local.get $l2
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get $p0
      i32.load offset=8
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17h38e7befd79b1f826E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    block $B0
      local.get $p0
      br_if $B0
      i32.const 1048859
      i32.const 43
      local.get $p1
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get $p0)
  (func $_ZN4core6option15Option$LT$T$GT$6unwrap17h88efffa862cbe9d0E (type $t10) (param $p0 i32) (result i32)
    block $B0
      local.get $p0
      br_if $B0
      i32.const 1048859
      i32.const 43
      i32.const 1051768
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get $p0)
  (func $_ZN4core9panicking13assert_failed17he78e5fdda9a074e5E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 1051044
    i32.store offset=4
    local.get $l3
    local.get $p0
    i32.store
    local.get $l3
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    local.get $p1
    i64.load align=4
    i64.store offset=8
    i32.const 0
    local.get $l3
    i32.const 1049012
    local.get $l3
    i32.const 4
    i32.add
    i32.const 1049012
    local.get $l3
    i32.const 8
    i32.add
    local.get $p2
    call $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E
    unreachable)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h2ffc04b3c6716e0dE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5a3b0091f8192a8fE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN58_$LT$alloc..string..String$u20$as$u20$core..fmt..Write$GT$10write_char17h5e45316a4ac7182cE
    drop
    i32.const 0)
  (func $_ZN58_$LT$alloc..string..String$u20$as$u20$core..fmt..Write$GT$10write_char17h5e45316a4ac7182cE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p1
              i32.const 128
              i32.lt_u
              br_if $B4
              local.get $l2
              i32.const 0
              i32.store offset=12
              local.get $p1
              i32.const 2048
              i32.lt_u
              br_if $B3
              local.get $p1
              i32.const 65536
              i32.ge_u
              br_if $B2
              local.get $l2
              local.get $p1
              i32.const 63
              i32.and
              i32.const 128
              i32.or
              i32.store8 offset=14
              local.get $l2
              local.get $p1
              i32.const 12
              i32.shr_u
              i32.const 224
              i32.or
              i32.store8 offset=12
              local.get $l2
              local.get $p1
              i32.const 6
              i32.shr_u
              i32.const 63
              i32.and
              i32.const 128
              i32.or
              i32.store8 offset=13
              i32.const 3
              local.set $p1
              br $B1
            end
            block $B5
              local.get $p0
              i32.load offset=8
              local.tee $l3
              local.get $p0
              i32.const 4
              i32.add
              i32.load
              i32.ne
              br_if $B5
              local.get $p0
              local.get $l3
              call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h886fbd751427a748E
              local.get $p0
              i32.load offset=8
              local.set $l3
            end
            local.get $p0
            local.get $l3
            i32.const 1
            i32.add
            i32.store offset=8
            local.get $p0
            i32.load
            local.get $l3
            i32.add
            local.get $p1
            i32.store8
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8 offset=13
          local.get $l2
          local.get $p1
          i32.const 6
          i32.shr_u
          i32.const 192
          i32.or
          i32.store8 offset=12
          i32.const 2
          local.set $p1
          br $B1
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=15
        local.get $l2
        local.get $p1
        i32.const 18
        i32.shr_u
        i32.const 240
        i32.or
        i32.store8 offset=12
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=14
        local.get $l2
        local.get $p1
        i32.const 12
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        i32.const 4
        local.set $p1
      end
      block $B6
        local.get $p0
        i32.const 4
        i32.add
        i32.load
        local.get $p0
        i32.const 8
        i32.add
        local.tee $l4
        i32.load
        local.tee $l3
        i32.sub
        local.get $p1
        i32.ge_u
        br_if $B6
        local.get $p0
        local.get $l3
        local.get $p1
        call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
        local.get $l4
        i32.load
        local.set $l3
      end
      local.get $p0
      i32.load
      local.get $l3
      i32.add
      local.get $l2
      i32.const 12
      i32.add
      local.get $p1
      call $memcpy
      drop
      local.get $l4
      local.get $l3
      local.get $p1
      i32.add
      i32.store
    end
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    i32.const 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h9896f020280c35deE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load
    local.set $p0
    local.get $l2
    i32.const 0
    i32.store offset=12
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=12
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set $p1
    end
    local.get $p0
    local.get $l2
    i32.const 12
    i32.add
    local.get $p1
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E
    local.set $p1
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17hbc3d3931408e41d6E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h47c6b3c5d318955aE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.load
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048720
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h8027bf4984b5aaecE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.load
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048768
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17he4a59f9604beea94E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.load
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048744
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hf0a5b3c4ac0d0496E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.load
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1048696
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h35232905e30669aaE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i64) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 8
    i32.add
    local.get $p0
    i32.load
    local.tee $p0
    i32.load
    local.get $p1
    local.get $p2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block $B0
      local.get $l3
      i32.load8_u offset=8
      local.tee $p1
      i32.const 4
      i32.eq
      br_if $B0
      local.get $l3
      i64.load offset=8
      local.set $l4
      block $B1
        local.get $p0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if $B1
        local.get $p0
        i32.const 8
        i32.add
        i32.load
        local.tee $p2
        i32.load
        local.get $p2
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B2
          local.get $p2
          i32.load offset=4
          local.tee $l5
          i32.load offset=4
          local.tee $l6
          i32.eqz
          br_if $B2
          local.get $p2
          i32.load
          local.get $l6
          local.get $l5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $p2
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get $p0
      local.get $l4
      i64.store offset=4 align=4
    end
    local.get $l3
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1
    i32.const 4
    i32.ne)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5e136cc95a131902E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32)
    block $B0
      local.get $p0
      i32.load
      i32.load
      local.tee $l3
      i32.const 4
      i32.add
      i32.load
      local.get $l3
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $p0
      i32.sub
      local.get $p2
      i32.ge_u
      br_if $B0
      local.get $l3
      local.get $p0
      local.get $p2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $p0
    end
    local.get $l3
    i32.load
    local.get $p0
    i32.add
    local.get $p1
    local.get $p2
    call $memcpy
    drop
    local.get $l4
    local.get $p0
    local.get $p2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h7f904ebe63e601aaE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    local.get $p2
    call $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17he56d425d213d6944E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32)
    block $B0
      local.get $p0
      i32.load
      local.tee $l3
      i32.const 4
      i32.add
      i32.load
      local.get $l3
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $p0
      i32.sub
      local.get $p2
      i32.ge_u
      br_if $B0
      local.get $l3
      local.get $p0
      local.get $p2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $p0
    end
    local.get $l3
    i32.load
    local.get $p0
    i32.add
    local.get $p1
    local.get $p2
    call $memcpy
    drop
    local.get $l4
    local.get $p0
    local.get $p2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h886fbd751427a748E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      local.get $p1
      i32.const 1
      i32.add
      local.tee $l3
      local.get $p1
      i32.lt_u
      br_if $B0
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $l4
      i32.const 1
      i32.shl
      local.tee $p1
      local.get $l3
      local.get $p1
      local.get $l3
      i32.gt_u
      select
      local.tee $p1
      i32.const 8
      local.get $p1
      i32.const 8
      i32.gt_u
      select
      local.set $p1
      block $B1
        block $B2
          local.get $l4
          br_if $B2
          i32.const 0
          local.set $l3
          br $B1
        end
        local.get $l2
        local.get $l4
        i32.store offset=20
        local.get $l2
        local.get $p0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set $l3
      end
      local.get $l2
      local.get $l3
      i32.store offset=24
      local.get $l2
      local.get $p1
      i32.const 1
      local.get $l2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block $B3
        local.get $l2
        i32.load
        i32.eqz
        br_if $B3
        local.get $l2
        i32.const 8
        i32.add
        i32.load
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $l2
        i32.load offset=4
        local.get $p0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get $l2
      i32.load offset=4
      local.set $l3
      local.get $p0
      i32.const 4
      i32.add
      local.get $p1
      i32.store
      local.get $p0
      local.get $l3
      i32.store
      local.get $l2
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E (type $t0) (param $p0 i32)
    (local $l1 i32)
    block $B0
      local.get $p0
      i32.const 16
      i32.add
      i32.load
      local.tee $l1
      i32.eqz
      br_if $B0
      local.get $p0
      i32.load offset=12
      local.get $l1
      i32.const 1
      call $__rust_dealloc
    end
    block $B1
      local.get $p0
      i32.const -1
      i32.eq
      br_if $B1
      local.get $p0
      local.get $p0
      i32.load offset=4
      local.tee $l1
      i32.const -1
      i32.add
      i32.store offset=4
      local.get $l1
      i32.const 1
      i32.ne
      br_if $B1
      local.get $p0
      i32.const 24
      i32.const 4
      call $__rust_dealloc
    end)
  (func $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32)
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  local.get $p2
                  i32.eqz
                  br_if $B6
                  i32.const 1
                  local.set $l4
                  local.get $p1
                  i32.const 0
                  i32.lt_s
                  br_if $B5
                  local.get $p3
                  i32.load offset=8
                  i32.eqz
                  br_if $B3
                  local.get $p3
                  i32.load offset=4
                  local.tee $l5
                  br_if $B4
                  local.get $p1
                  br_if $B2
                  local.get $p2
                  local.set $p3
                  br $B1
                end
                local.get $p0
                local.get $p1
                i32.store offset=4
                i32.const 1
                local.set $l4
              end
              i32.const 0
              local.set $p1
              br $B0
            end
            local.get $p3
            i32.load
            local.get $l5
            local.get $p2
            local.get $p1
            call $__rust_realloc
            local.set $p3
            br $B1
          end
          local.get $p1
          br_if $B2
          local.get $p2
          local.set $p3
          br $B1
        end
        local.get $p1
        local.get $p2
        call $__rust_alloc
        local.set $p3
      end
      block $B7
        local.get $p3
        i32.eqz
        br_if $B7
        local.get $p0
        local.get $p3
        i32.store offset=4
        i32.const 0
        local.set $l4
        br $B0
      end
      local.get $p0
      local.get $p1
      i32.store offset=4
      local.get $p2
      local.set $p1
    end
    local.get $p0
    local.get $l4
    i32.store
    local.get $p0
    i32.const 8
    i32.add
    local.get $p1
    i32.store)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h4285ebb9278a5d57E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i64)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      local.get $p1
      i32.const 1
      i32.add
      local.tee $l3
      local.get $p1
      i32.lt_u
      br_if $B0
      i32.const 4
      local.set $l4
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $p1
      i32.const 1
      i32.shl
      local.tee $l5
      local.get $l3
      local.get $l5
      local.get $l3
      i32.gt_u
      select
      local.tee $l3
      i32.const 4
      local.get $l3
      i32.const 4
      i32.gt_u
      select
      local.tee $l6
      i64.extend_i32_u
      i64.const 24
      i64.mul
      local.tee $l7
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.eqz
      i32.const 2
      i32.shl
      local.set $l3
      local.get $l7
      i32.wrap_i64
      local.set $l5
      block $B1
        block $B2
          local.get $p1
          br_if $B2
          i32.const 0
          local.set $l4
          br $B1
        end
        local.get $l2
        local.get $p0
        i32.load
        i32.store offset=16
        local.get $l2
        local.get $p1
        i64.extend_i32_u
        i64.const 24
        i64.mul
        i64.store32 offset=20
      end
      local.get $l2
      local.get $l4
      i32.store offset=24
      local.get $l2
      local.get $l5
      local.get $l3
      local.get $l2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block $B3
        local.get $l2
        i32.load
        i32.eqz
        br_if $B3
        local.get $l2
        i32.const 8
        i32.add
        i32.load
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $l2
        i32.load offset=4
        local.get $p0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get $l2
      i32.load offset=4
      local.set $p1
      local.get $p0
      i32.const 4
      i32.add
      local.get $l6
      i32.store
      local.get $p0
      local.get $p1
      i32.store
      local.get $l2
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h49fd6398be4a4a9aE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i64)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      local.get $p1
      i32.const 1
      i32.add
      local.tee $l3
      local.get $p1
      i32.lt_u
      br_if $B0
      i32.const 4
      local.set $l4
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $p1
      i32.const 1
      i32.shl
      local.tee $l5
      local.get $l3
      local.get $l5
      local.get $l3
      i32.gt_u
      select
      local.tee $l3
      i32.const 4
      local.get $l3
      i32.const 4
      i32.gt_u
      select
      local.tee $l6
      i64.extend_i32_u
      i64.const 12
      i64.mul
      local.tee $l7
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.eqz
      i32.const 2
      i32.shl
      local.set $l3
      local.get $l7
      i32.wrap_i64
      local.set $l5
      block $B1
        block $B2
          local.get $p1
          br_if $B2
          i32.const 0
          local.set $l4
          br $B1
        end
        local.get $l2
        local.get $p0
        i32.load
        i32.store offset=16
        local.get $l2
        local.get $p1
        i64.extend_i32_u
        i64.const 12
        i64.mul
        i64.store32 offset=20
      end
      local.get $l2
      local.get $l4
      i32.store offset=24
      local.get $l2
      local.get $l5
      local.get $l3
      local.get $l2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17h85fdcb8b7f987133E
      block $B3
        local.get $l2
        i32.load
        i32.eqz
        br_if $B3
        local.get $l2
        i32.const 8
        i32.add
        i32.load
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $l2
        i32.load offset=4
        local.get $p0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get $l2
      i32.load offset=4
      local.set $p1
      local.get $p0
      i32.const 4
      i32.add
      local.get $l6
      i32.store
      local.get $p0
      local.get $p1
      i32.store
      local.get $l2
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E.1 (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.const 8
    i32.add
    i32.load
    local.get $p1
    call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE)
  (func $_ZN3std4sync4once4Once10call_inner17h90795951c1d06984E (type $t11) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32)
    (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l5
    global.set $g0
    local.get $l5
    i32.const 8
    i32.add
    i32.const 2
    i32.or
    local.set $l6
    local.get $p0
    i32.load
    local.set $l7
    loop $L0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  block $B7
                    block $B8
                      block $B9
                        block $B10
                          block $B11
                            block $B12
                              block $B13
                                block $B14
                                  local.get $l7
                                  br_table $B13 $B14 $B12 $B9 $B12
                                end
                                local.get $p1
                                i32.eqz
                                br_if $B11
                              end
                              local.get $p0
                              i32.const 2
                              local.get $p0
                              i32.load
                              local.tee $l8
                              local.get $l8
                              local.get $l7
                              i32.eq
                              local.tee $l9
                              select
                              i32.store
                              local.get $l9
                              br_if $B10
                              local.get $l8
                              local.set $l7
                              br $L0
                            end
                            block $B15
                              local.get $l7
                              i32.const 3
                              i32.and
                              i32.const 2
                              i32.ne
                              br_if $B15
                              loop $L16
                                local.get $l7
                                local.set $l9
                                i32.const 0
                                i32.load offset=1059296
                                br_if $B8
                                i32.const 0
                                i32.const -1
                                i32.store offset=1059296
                                block $B17
                                  i32.const 0
                                  i32.load offset=1059300
                                  local.tee $l8
                                  br_if $B17
                                  i32.const 0
                                  i32.const 0
                                  local.get $l7
                                  call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                                  local.tee $l8
                                  i32.store offset=1059300
                                end
                                local.get $l8
                                local.get $l8
                                i32.load
                                local.tee $l7
                                i32.const 1
                                i32.add
                                i32.store
                                local.get $l7
                                i32.const -1
                                i32.le_s
                                br_if $B7
                                i32.const 0
                                i32.const 0
                                i32.load offset=1059296
                                i32.const 1
                                i32.add
                                i32.store offset=1059296
                                local.get $l8
                                i32.eqz
                                br_if $B6
                                local.get $p0
                                local.get $l6
                                local.get $p0
                                i32.load
                                local.tee $l7
                                local.get $l7
                                local.get $l9
                                i32.eq
                                select
                                i32.store
                                local.get $l5
                                i32.const 0
                                i32.store8 offset=16
                                local.get $l5
                                local.get $l8
                                i32.store offset=8
                                local.get $l5
                                local.get $l9
                                i32.const -4
                                i32.and
                                i32.store offset=12
                                block $B18
                                  local.get $l7
                                  local.get $l9
                                  i32.ne
                                  br_if $B18
                                  local.get $l5
                                  i32.load8_u offset=16
                                  i32.eqz
                                  br_if $B5
                                  br $B2
                                end
                                block $B19
                                  local.get $l5
                                  i32.load offset=8
                                  local.tee $l8
                                  i32.eqz
                                  br_if $B19
                                  local.get $l8
                                  local.get $l8
                                  i32.load
                                  local.tee $l9
                                  i32.const -1
                                  i32.add
                                  i32.store
                                  local.get $l9
                                  i32.const 1
                                  i32.ne
                                  br_if $B19
                                  local.get $l5
                                  i32.load offset=8
                                  call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                                end
                                local.get $l7
                                i32.const 3
                                i32.and
                                i32.const 2
                                i32.eq
                                br_if $L16
                                br $B1
                              end
                            end
                            i32.const 1050928
                            i32.const 64
                            local.get $p4
                            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
                            unreachable
                          end
                          local.get $l5
                          i32.const 28
                          i32.add
                          i32.const 0
                          i32.store
                          local.get $l5
                          i32.const 1048792
                          i32.store offset=24
                          local.get $l5
                          i64.const 1
                          i64.store offset=12 align=4
                          local.get $l5
                          i32.const 1051036
                          i32.store offset=8
                          local.get $l5
                          i32.const 8
                          i32.add
                          local.get $p4
                          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
                          unreachable
                        end
                        local.get $l5
                        local.get $l7
                        i32.const 1
                        i32.eq
                        i32.store8 offset=12
                        local.get $l5
                        i32.const 3
                        i32.store offset=8
                        local.get $p2
                        local.get $l5
                        i32.const 8
                        i32.add
                        local.get $p3
                        i32.load offset=16
                        call_indirect (type $t2) $T0
                        local.get $p0
                        i32.load
                        local.set $l7
                        local.get $p0
                        local.get $l5
                        i32.load offset=8
                        i32.store
                        local.get $l5
                        local.get $l7
                        i32.const 3
                        i32.and
                        local.tee $l8
                        i32.store
                        local.get $l8
                        i32.const 2
                        i32.ne
                        br_if $B4
                        local.get $l7
                        i32.const -2
                        i32.add
                        local.tee $l8
                        i32.eqz
                        br_if $B9
                        loop $L20
                          local.get $l8
                          i32.load
                          local.set $l7
                          local.get $l8
                          i32.const 0
                          i32.store
                          local.get $l7
                          i32.eqz
                          br_if $B3
                          local.get $l8
                          i32.load offset=4
                          local.set $l9
                          local.get $l8
                          i32.const 1
                          i32.store8 offset=8
                          local.get $l7
                          i32.const 24
                          i32.add
                          call $_ZN3std10sys_common13thread_parker7generic6Parker6unpark17h41bf5bed244e8e6fE
                          local.get $l7
                          local.get $l7
                          i32.load
                          local.tee $l8
                          i32.const -1
                          i32.add
                          i32.store
                          block $B21
                            local.get $l8
                            i32.const 1
                            i32.ne
                            br_if $B21
                            local.get $l7
                            call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                          end
                          local.get $l9
                          local.set $l8
                          local.get $l9
                          br_if $L20
                        end
                      end
                      local.get $l5
                      i32.const 32
                      i32.add
                      global.set $g0
                      return
                    end
                    i32.const 1048792
                    i32.const 16
                    local.get $l5
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
              loop $L22
                call $_ZN3std6thread4park17h401b5f43fa6ef902E
                local.get $l5
                i32.load8_u offset=16
                i32.eqz
                br_if $L22
                br $B2
              end
            end
            local.get $l5
            i32.const 0
            i32.store offset=8
            local.get $l5
            local.get $l5
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
        local.get $l5
        i32.load offset=8
        local.tee $l7
        i32.eqz
        br_if $B1
        local.get $l7
        local.get $l7
        i32.load
        local.tee $l8
        i32.const -1
        i32.add
        i32.store
        local.get $l8
        i32.const 1
        i32.ne
        br_if $B1
        local.get $l5
        i32.load offset=8
        call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
        local.get $p0
        i32.load
        local.set $l7
        br $L0
      end
      local.get $p0
      i32.load
      local.set $l7
      br $L0
    end)
  (func $_ZN3std6thread6Thread3new17hd168535b42058e7aE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i64)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        i32.const 32
        i32.const 8
        call $__rust_alloc
        local.tee $l3
        i32.eqz
        br_if $B1
        local.get $l3
        local.get $p0
        i32.store offset=16
        local.get $l3
        i64.const 4294967297
        i64.store
        local.get $l3
        i32.const 20
        i32.add
        local.get $p1
        i32.store
        i32.const 0
        i32.load8_u offset=1059217
        local.set $p0
        i32.const 0
        i32.const 1
        i32.store8 offset=1059217
        local.get $l2
        local.get $p0
        i32.store8 offset=7
        local.get $p0
        br_if $B0
        block $B2
          block $B3
            i32.const 0
            i64.load offset=1059200
            local.tee $l4
            i64.const -1
            i64.eq
            br_if $B3
            i32.const 0
            local.get $l4
            i64.const 1
            i64.add
            i64.store offset=1059200
            local.get $l4
            i64.const 0
            i64.ne
            br_if $B2
            i32.const 1048859
            i32.const 43
            i32.const 1049420
            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
            unreachable
          end
          i32.const 0
          i32.const 0
          i32.store8 offset=1059217
          local.get $l2
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get $l2
          i32.const 1048792
          i32.store offset=24
          local.get $l2
          i64.const 1
          i64.store offset=12 align=4
          local.get $l2
          i32.const 1049396
          i32.store offset=8
          local.get $l2
          i32.const 8
          i32.add
          i32.const 1049404
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $l3
        i64.const 0
        i64.store offset=24
        local.get $l3
        local.get $l4
        i64.store offset=8
        i32.const 0
        i32.const 0
        i32.store8 offset=1059217
        local.get $l2
        i32.const 32
        i32.add
        global.set $g0
        local.get $l3
        return
      end
      i32.const 32
      i32.const 8
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get $l2
    i32.const 8
    i32.add
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get $l2
    i32.const 24
    i32.add
    i32.const 1048792
    i32.store
    local.get $l2
    i64.const 1
    i64.store offset=12 align=4
    local.get $l2
    i32.const 1052308
    i32.store offset=8
    local.get $l2
    i32.const 7
    i32.add
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 4
    i32.store8 offset=12
    local.get $l3
    local.get $p1
    i32.store offset=8
    local.get $l3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    local.get $p2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.get $p2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    local.get $p2
    i64.load align=4
    i64.store offset=24
    block $B0
      block $B1
        local.get $l3
        i32.const 8
        i32.add
        i32.const 1050736
        local.get $l3
        i32.const 24
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        i32.eqz
        br_if $B1
        block $B2
          local.get $l3
          i32.load8_u offset=12
          i32.const 4
          i32.ne
          br_if $B2
          local.get $p0
          i32.const 1050724
          i64.extend_i32_u
          i64.const 32
          i64.shl
          i64.const 2
          i64.or
          i64.store align=4
          br $B0
        end
        local.get $p0
        local.get $l3
        i64.load offset=12 align=4
        i64.store align=4
        br $B0
      end
      local.get $p0
      i32.const 4
      i32.store8
      local.get $l3
      i32.load8_u offset=12
      i32.const 3
      i32.ne
      br_if $B0
      local.get $l3
      i32.const 16
      i32.add
      i32.load
      local.tee $p2
      i32.load
      local.get $p2
      i32.load offset=4
      i32.load
      call_indirect (type $t0) $T0
      block $B3
        local.get $p2
        i32.load offset=4
        local.tee $p1
        i32.load offset=4
        local.tee $p0
        i32.eqz
        br_if $B3
        local.get $p2
        i32.load
        local.get $p0
        local.get $p1
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get $l3
      i32.load offset=16
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    local.get $l3
    i32.const 48
    i32.add
    global.set $g0)
  (func $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE (type $t7)
    call $abort
    unreachable)
  (func $_ZN3std10sys_common13thread_parker7generic6Parker6unpark17h41bf5bed244e8e6fE (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l1
    global.set $g0
    local.get $p0
    i32.load
    local.set $l2
    local.get $p0
    i32.const 2
    i32.store
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $l2
            br_table $B1 $B2 $B1 $B3
          end
          local.get $l1
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get $l1
          i32.const 1048792
          i32.store offset=24
          local.get $l1
          i64.const 1
          i64.store offset=12 align=4
          local.get $l1
          i32.const 1052828
          i32.store offset=8
          local.get $l1
          i32.const 8
          i32.add
          i32.const 1052836
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $p0
        i32.load8_u offset=4
        local.set $l2
        local.get $p0
        i32.const 1
        i32.store8 offset=4
        local.get $l1
        local.get $l2
        i32.const 1
        i32.and
        local.tee $l2
        i32.store8 offset=7
        local.get $l2
        br_if $B0
        local.get $p0
        i32.const 4
        i32.add
        local.set $p0
        i32.const 0
        local.set $l2
        block $B4
          block $B5
            block $B6
              block $B7
                block $B8
                  i32.const 0
                  i32.load offset=1059280
                  i32.const 2147483647
                  i32.and
                  i32.eqz
                  br_if $B8
                  call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                  local.set $l2
                  local.get $p0
                  i32.load8_u offset=1
                  i32.eqz
                  br_if $B6
                  local.get $l2
                  i32.const 1
                  i32.xor
                  local.set $l2
                  br $B7
                end
                local.get $p0
                i32.load8_u offset=1
                i32.eqz
                br_if $B5
              end
              local.get $l1
              local.get $l2
              i32.store8 offset=12
              local.get $l1
              local.get $p0
              i32.store offset=8
              i32.const 1048920
              i32.const 43
              local.get $l1
              i32.const 8
              i32.add
              i32.const 1048964
              i32.const 1052852
              call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
              unreachable
            end
            local.get $l2
            i32.eqz
            br_if $B4
          end
          i32.const 0
          i32.load offset=1059280
          i32.const 2147483647
          i32.and
          i32.eqz
          br_if $B4
          call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
          br_if $B4
          local.get $p0
          i32.const 1
          i32.store8 offset=1
        end
        local.get $p0
        i32.const 0
        i32.store8
      end
      local.get $l1
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    local.get $l1
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get $l1
    i32.const 24
    i32.add
    i32.const 1048792
    i32.store
    local.get $l1
    i64.const 1
    i64.store offset=12 align=4
    local.get $l1
    i32.const 1052308
    i32.store offset=8
    local.get $l1
    i32.const 7
    i32.add
    local.get $l1
    i32.const 8
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $_ZN3std6thread4park17h401b5f43fa6ef902E (type $t7)
    (local $l0 i32) (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l0
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  block $B7
                    i32.const 0
                    i32.load offset=1059296
                    br_if $B7
                    i32.const 0
                    i32.const -1
                    i32.store offset=1059296
                    block $B8
                      i32.const 0
                      i32.load offset=1059300
                      local.tee $l1
                      br_if $B8
                      i32.const 0
                      i32.const 0
                      local.get $l1
                      call $_ZN3std6thread6Thread3new17hd168535b42058e7aE
                      local.tee $l1
                      i32.store offset=1059300
                    end
                    local.get $l1
                    local.get $l1
                    i32.load
                    local.tee $l2
                    i32.const 1
                    i32.add
                    i32.store
                    local.get $l2
                    i32.const -1
                    i32.le_s
                    br_if $B6
                    i32.const 0
                    i32.const 0
                    i32.load offset=1059296
                    i32.const 1
                    i32.add
                    i32.store offset=1059296
                    local.get $l1
                    i32.eqz
                    br_if $B5
                    local.get $l1
                    i32.const 0
                    local.get $l1
                    i32.load offset=24
                    local.tee $l2
                    local.get $l2
                    i32.const 2
                    i32.eq
                    local.tee $l2
                    select
                    i32.store offset=24
                    block $B9
                      local.get $l2
                      br_if $B9
                      local.get $l1
                      i32.const 24
                      i32.add
                      local.tee $l2
                      i32.load8_u offset=4
                      local.set $l3
                      local.get $l2
                      i32.const 1
                      i32.store8 offset=4
                      local.get $l0
                      local.get $l3
                      i32.const 1
                      i32.and
                      local.tee $l3
                      i32.store8 offset=4
                      local.get $l3
                      br_if $B4
                      local.get $l2
                      i32.const 4
                      i32.add
                      local.set $l4
                      i32.const 0
                      local.set $l5
                      block $B10
                        i32.const 0
                        i32.load offset=1059280
                        i32.const 2147483647
                        i32.and
                        i32.eqz
                        br_if $B10
                        call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                        i32.const 1
                        i32.xor
                        local.set $l5
                      end
                      local.get $l4
                      i32.load8_u offset=1
                      br_if $B3
                      local.get $l2
                      local.get $l2
                      i32.load
                      local.tee $l3
                      i32.const 1
                      local.get $l3
                      select
                      i32.store
                      local.get $l3
                      i32.eqz
                      br_if $B0
                      local.get $l3
                      i32.const 2
                      i32.ne
                      br_if $B2
                      local.get $l2
                      i32.load
                      local.set $l3
                      local.get $l2
                      i32.const 0
                      i32.store
                      local.get $l0
                      local.get $l3
                      i32.store offset=4
                      local.get $l3
                      i32.const 2
                      i32.ne
                      br_if $B1
                      block $B11
                        local.get $l5
                        br_if $B11
                        i32.const 0
                        i32.load offset=1059280
                        i32.const 2147483647
                        i32.and
                        i32.eqz
                        br_if $B11
                        call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
                        br_if $B11
                        local.get $l4
                        i32.const 1
                        i32.store8 offset=1
                      end
                      local.get $l4
                      i32.const 0
                      i32.store8
                    end
                    local.get $l1
                    local.get $l1
                    i32.load
                    local.tee $l2
                    i32.const -1
                    i32.add
                    i32.store
                    block $B12
                      local.get $l2
                      i32.const 1
                      i32.ne
                      br_if $B12
                      local.get $l1
                      call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17h586e8553822f3f8dE
                    end
                    local.get $l0
                    i32.const 32
                    i32.add
                    global.set $g0
                    return
                  end
                  i32.const 1048792
                  i32.const 16
                  local.get $l0
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
            local.get $l0
            i32.const 28
            i32.add
            i32.const 0
            i32.store
            local.get $l0
            i32.const 24
            i32.add
            i32.const 1048792
            i32.store
            local.get $l0
            i64.const 1
            i64.store offset=12 align=4
            local.get $l0
            i32.const 1052308
            i32.store offset=8
            local.get $l0
            i32.const 4
            i32.add
            local.get $l0
            i32.const 8
            i32.add
            call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
            unreachable
          end
          local.get $l0
          local.get $l5
          i32.store8 offset=12
          local.get $l0
          local.get $l4
          i32.store offset=8
          i32.const 1048920
          i32.const 43
          local.get $l0
          i32.const 8
          i32.add
          i32.const 1048964
          i32.const 1052680
          call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
          unreachable
        end
        local.get $l0
        i32.const 28
        i32.add
        i32.const 0
        i32.store
        local.get $l0
        i32.const 1048792
        i32.store offset=24
        local.get $l0
        i64.const 1
        i64.store offset=12 align=4
        local.get $l0
        i32.const 1052720
        i32.store offset=8
        local.get $l0
        i32.const 8
        i32.add
        i32.const 1052728
        call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
        unreachable
      end
      local.get $l0
      i32.const 28
      i32.add
      i32.const 0
      i32.store
      local.get $l0
      i32.const 24
      i32.add
      i32.const 1048792
      i32.store
      local.get $l0
      i64.const 1
      i64.store offset=12 align=4
      local.get $l0
      i32.const 1052776
      i32.store offset=8
      local.get $l0
      i32.const 4
      i32.add
      local.get $l0
      i32.const 8
      i32.add
      i32.const 1052784
      call $_ZN4core9panicking13assert_failed17he78e5fdda9a074e5E
      unreachable
    end
    local.get $l0
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get $l0
    i32.const 1048792
    i32.store offset=24
    local.get $l0
    i64.const 1
    i64.store offset=12 align=4
    local.get $l0
    i32.const 1052196
    i32.store offset=8
    local.get $l0
    i32.const 8
    i32.add
    i32.const 1052260
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN3std3env11current_dir17he9cc85f9a6781258E (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l1
    global.set $g0
    i32.const 512
    local.set $l2
    block $B0
      block $B1
        block $B2
          block $B3
            i32.const 512
            i32.const 1
            call $__rust_alloc
            local.tee $l3
            i32.eqz
            br_if $B3
            local.get $l1
            i32.const 512
            i32.store offset=4
            local.get $l1
            local.get $l3
            i32.store
            local.get $l3
            i32.const 512
            call $getcwd
            br_if $B2
            block $B4
              block $B5
                block $B6
                  i32.const 0
                  i32.load offset=1059804
                  local.tee $l2
                  i32.const 68
                  i32.ne
                  br_if $B6
                  i32.const 512
                  local.set $l2
                  br $B5
                end
                local.get $p0
                i64.const 1
                i64.store align=4
                local.get $p0
                i32.const 8
                i32.add
                local.get $l2
                i32.store
                i32.const 512
                local.set $l2
                br $B4
              end
              loop $L7
                local.get $l1
                local.get $l2
                i32.store offset=8
                local.get $l1
                local.get $l2
                i32.const 1
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                local.get $l1
                i32.load
                local.tee $l3
                local.get $l1
                i32.load offset=4
                local.tee $l2
                call $getcwd
                br_if $B2
                i32.const 0
                i32.load offset=1059804
                local.tee $l4
                i32.const 68
                i32.eq
                br_if $L7
              end
              local.get $p0
              i64.const 1
              i64.store align=4
              local.get $p0
              i32.const 8
              i32.add
              local.get $l4
              i32.store
              local.get $l2
              i32.eqz
              br_if $B1
            end
            local.get $l3
            local.get $l2
            i32.const 1
            call $__rust_dealloc
            br $B1
          end
          i32.const 512
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        local.get $l1
        local.get $l3
        call $strlen
        local.tee $l4
        i32.store offset=8
        block $B8
          local.get $l2
          local.get $l4
          i32.le_u
          br_if $B8
          block $B9
            block $B10
              local.get $l4
              br_if $B10
              i32.const 1
              local.set $l5
              local.get $l3
              local.get $l2
              i32.const 1
              call $__rust_dealloc
              br $B9
            end
            local.get $l3
            local.get $l2
            i32.const 1
            local.get $l4
            call $__rust_realloc
            local.tee $l5
            i32.eqz
            br_if $B0
          end
          local.get $l1
          local.get $l4
          i32.store offset=4
          local.get $l1
          local.get $l5
          i32.store
        end
        local.get $p0
        local.get $l1
        i64.load
        i64.store offset=4 align=4
        local.get $p0
        i32.const 0
        i32.store
        local.get $p0
        i32.const 12
        i32.add
        local.get $l1
        i32.const 8
        i32.add
        i32.load
        i32.store
      end
      local.get $l1
      i32.const 16
      i32.add
      global.set $g0
      return
    end
    local.get $l4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std3env7_var_os17hac8a3e6bb094e6d3E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 8
    i32.add
    local.get $p1
    local.get $p2
    call $_ZN72_$LT$$RF$str$u20$as$u20$alloc..ffi..c_str..CString..new..SpecNewImpl$GT$13spec_new_impl17h8200e939a8ca496bE
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $l3
            i32.load offset=8
            i32.eqz
            br_if $B3
            block $B4
              local.get $l3
              i32.const 20
              i32.add
              i32.load
              local.tee $p1
              i32.eqz
              br_if $B4
              local.get $l3
              i32.const 16
              i32.add
              i32.load
              local.get $p1
              i32.const 1
              call $__rust_dealloc
            end
            local.get $p0
            i32.const 0
            i32.store
            br $B2
          end
          local.get $l3
          i32.const 16
          i32.add
          i32.load
          local.set $l4
          block $B5
            block $B6
              local.get $l3
              i32.load offset=12
              local.tee $p2
              call $getenv
              local.tee $l5
              i32.eqz
              br_if $B6
              block $B7
                block $B8
                  local.get $l5
                  call $strlen
                  local.tee $p1
                  br_if $B8
                  i32.const 1
                  local.set $l6
                  br $B7
                end
                local.get $p1
                i32.const 0
                i32.lt_s
                br_if $B1
                local.get $p1
                i32.const 1
                call $__rust_alloc
                local.tee $l6
                i32.eqz
                br_if $B0
              end
              local.get $l6
              local.get $l5
              local.get $p1
              call $memcpy
              local.set $l5
              local.get $p0
              i32.const 8
              i32.add
              local.get $p1
              i32.store
              local.get $p0
              local.get $p1
              i32.store offset=4
              local.get $p0
              local.get $l5
              i32.store
              br $B5
            end
            local.get $p0
            i32.const 0
            i32.store
          end
          local.get $p2
          i32.const 0
          i32.store8
          local.get $l4
          i32.eqz
          br_if $B2
          local.get $p2
          local.get $l4
          i32.const 1
          call $__rust_dealloc
        end
        local.get $l3
        i32.const 32
        i32.add
        global.set $g0
        return
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get $p1
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std3env4vars17h6ec7390381e46b70E (type $t0) (param $p0 i32)
    local.get $p0
    call $_ZN3std3env7vars_os17h4335c88f098b90b5E)
  (func $_ZN3std3env7vars_os17h4335c88f098b90b5E (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l1
    global.set $g0
    i32.const 0
    local.set $l2
    i32.const 0
    i32.load offset=1059812
    local.set $l3
    local.get $l1
    i32.const 0
    i32.store offset=24
    local.get $l1
    i64.const 4
    i64.store offset=16
    i32.const 4
    local.set $l4
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  local.get $l3
                  br_if $B6
                  i32.const 0
                  local.set $l5
                  br $B5
                end
                i32.const 0
                local.set $l5
                local.get $l3
                i32.load
                local.tee $l6
                i32.eqz
                br_if $B5
                i32.const 0
                local.set $l2
                i32.const 4
                local.set $l7
                loop $L7
                  local.get $l3
                  local.set $l8
                  block $B8
                    local.get $l6
                    call $strlen
                    local.tee $l9
                    i32.eqz
                    br_if $B8
                    local.get $l6
                    i32.const 1
                    i32.add
                    local.set $l5
                    block $B9
                      block $B10
                        local.get $l9
                        i32.const -1
                        i32.add
                        local.tee $l4
                        i32.const 8
                        i32.lt_u
                        br_if $B10
                        local.get $l1
                        i32.const 8
                        i32.add
                        i32.const 61
                        local.get $l5
                        local.get $l4
                        call $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E
                        local.get $l1
                        i32.load offset=12
                        local.set $l3
                        local.get $l1
                        i32.load offset=8
                        local.set $l5
                        br $B9
                      end
                      i32.const 0
                      local.set $l3
                      block $B11
                        local.get $l4
                        br_if $B11
                        i32.const 0
                        local.set $l5
                        br $B9
                      end
                      loop $L12
                        block $B13
                          local.get $l5
                          local.get $l3
                          i32.add
                          i32.load8_u
                          i32.const 61
                          i32.ne
                          br_if $B13
                          i32.const 1
                          local.set $l5
                          br $B9
                        end
                        local.get $l4
                        local.get $l3
                        i32.const 1
                        i32.add
                        local.tee $l3
                        i32.ne
                        br_if $L12
                      end
                      i32.const 0
                      local.set $l5
                      local.get $l4
                      local.set $l3
                    end
                    local.get $l5
                    i32.eqz
                    br_if $B8
                    local.get $l3
                    i32.const 1
                    i32.add
                    local.tee $l3
                    local.get $l9
                    i32.gt_u
                    br_if $B4
                    block $B14
                      block $B15
                        local.get $l3
                        br_if $B15
                        i32.const 1
                        local.set $l4
                        br $B14
                      end
                      local.get $l3
                      i32.const 0
                      i32.lt_s
                      br_if $B1
                      local.get $l3
                      i32.const 1
                      call $__rust_alloc
                      local.tee $l4
                      i32.eqz
                      br_if $B3
                    end
                    local.get $l4
                    local.get $l6
                    local.get $l3
                    call $memcpy
                    local.set $l10
                    local.get $l3
                    i32.const 1
                    i32.add
                    local.set $l5
                    local.get $l3
                    local.get $l9
                    i32.ge_u
                    br_if $B2
                    block $B16
                      block $B17
                        local.get $l9
                        local.get $l5
                        i32.sub
                        local.tee $l4
                        br_if $B17
                        i32.const 1
                        local.set $l9
                        br $B16
                      end
                      local.get $l4
                      i32.const 0
                      i32.lt_s
                      br_if $B1
                      local.get $l4
                      i32.const 1
                      call $__rust_alloc
                      local.tee $l9
                      i32.eqz
                      br_if $B0
                    end
                    local.get $l9
                    local.get $l6
                    local.get $l5
                    i32.add
                    local.get $l4
                    call $memcpy
                    local.set $l6
                    local.get $l10
                    i32.eqz
                    br_if $B8
                    block $B18
                      local.get $l2
                      local.get $l1
                      i32.load offset=20
                      i32.ne
                      br_if $B18
                      local.get $l1
                      i32.const 16
                      i32.add
                      local.get $l2
                      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h4285ebb9278a5d57E
                      local.get $l1
                      i32.load offset=16
                      local.set $l7
                      local.get $l1
                      i32.load offset=24
                      local.set $l2
                    end
                    local.get $l7
                    local.get $l2
                    i32.const 24
                    i32.mul
                    i32.add
                    local.tee $l5
                    local.get $l6
                    i32.store offset=12
                    local.get $l5
                    local.get $l3
                    i32.store offset=8
                    local.get $l5
                    local.get $l3
                    i32.store offset=4
                    local.get $l5
                    local.get $l10
                    i32.store
                    local.get $l5
                    i32.const 20
                    i32.add
                    local.get $l4
                    i32.store
                    local.get $l5
                    i32.const 16
                    i32.add
                    local.get $l4
                    i32.store
                    local.get $l1
                    local.get $l2
                    i32.const 1
                    i32.add
                    local.tee $l2
                    i32.store offset=24
                  end
                  local.get $l8
                  i32.const 4
                  i32.add
                  local.set $l3
                  local.get $l8
                  i32.load offset=4
                  local.tee $l6
                  br_if $L7
                end
                local.get $l1
                i32.load offset=20
                local.set $l5
                local.get $l1
                i32.load offset=16
                local.set $l4
              end
              local.get $p0
              local.get $l4
              i32.store offset=8
              local.get $p0
              local.get $l5
              i32.store offset=4
              local.get $p0
              local.get $l4
              i32.store
              local.get $p0
              local.get $l4
              local.get $l2
              i32.const 24
              i32.mul
              i32.add
              i32.store offset=12
              local.get $l1
              i32.const 32
              i32.add
              global.set $g0
              return
            end
            local.get $l3
            local.get $l9
            i32.const 1052516
            call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
            unreachable
          end
          local.get $l3
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        local.get $l5
        local.get $l9
        i32.const 1052532
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get $l4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN73_$LT$std..env..Vars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16af7521e3223f9cE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i64)
    global.get $g0
    i32.const 96
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p1
              i32.load offset=8
              local.tee $l3
              local.get $p1
              i32.load offset=12
              i32.eq
              br_if $B4
              local.get $p1
              local.get $l3
              i32.const 24
              i32.add
              i32.store offset=8
              local.get $l3
              i32.load
              local.tee $p1
              br_if $B3
            end
            local.get $p0
            i32.const 0
            i32.store
            br $B2
          end
          local.get $l3
          i32.const 20
          i32.add
          i32.load
          local.set $l4
          local.get $l3
          i32.const 16
          i32.add
          i32.load
          local.set $l5
          local.get $l3
          i32.load offset=12
          local.set $l6
          local.get $l3
          i32.load offset=4
          local.set $l7
          local.get $l2
          i32.const 8
          i32.add
          local.get $p1
          local.get $l3
          i32.load offset=8
          local.tee $l3
          call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
          local.get $l2
          i32.load offset=8
          br_if $B1
          local.get $l2
          i32.const 64
          i32.add
          i32.const 8
          i32.add
          local.tee $l8
          local.get $l7
          i32.store
          local.get $l2
          i32.const 32
          i32.add
          i32.const 8
          i32.add
          local.tee $l7
          local.get $l3
          i32.store
          local.get $l2
          local.get $p1
          i32.store offset=68
          local.get $l2
          local.get $l2
          i64.load offset=68 align=4
          i64.store offset=32
          local.get $l2
          i32.const 80
          i32.add
          local.get $l6
          local.get $l4
          call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
          local.get $l2
          i32.load offset=80
          br_if $B0
          local.get $l8
          local.get $l5
          i32.store
          local.get $l2
          i32.const 28
          i32.add
          local.get $l4
          i32.store
          local.get $l2
          i32.const 8
          i32.add
          i32.const 8
          i32.add
          local.tee $l3
          local.get $l7
          i32.load
          i32.store
          local.get $l2
          local.get $l6
          i32.store offset=68
          local.get $l2
          local.get $l2
          i64.load offset=32
          i64.store offset=8
          local.get $l2
          local.get $l2
          i64.load offset=68 align=4
          i64.store offset=20 align=4
          local.get $p0
          i32.const 16
          i32.add
          local.get $l2
          i32.const 8
          i32.add
          i32.const 16
          i32.add
          i64.load
          i64.store align=4
          local.get $p0
          i32.const 8
          i32.add
          local.get $l3
          i64.load
          i64.store align=4
          local.get $p0
          local.get $l2
          i64.load offset=8
          i64.store align=4
        end
        local.get $l2
        i32.const 96
        i32.add
        global.set $g0
        return
      end
      local.get $l2
      local.get $l2
      i64.load offset=12 align=4
      i64.store offset=20 align=4
      local.get $l2
      local.get $l3
      i32.store offset=16
      local.get $l2
      local.get $l7
      i32.store offset=12
      local.get $l2
      local.get $p1
      i32.store offset=8
      local.get $l2
      i32.const 64
      i32.add
      i32.const 4
      i32.or
      local.get $l2
      i32.const 8
      i32.add
      call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
      local.get $l2
      i32.const 80
      i32.add
      i32.const 8
      i32.add
      local.get $l2
      i32.const 76
      i32.add
      i32.load
      local.tee $l3
      i32.store
      local.get $l2
      local.get $l2
      i64.load offset=68 align=4
      local.tee $l9
      i64.store offset=80
      local.get $l2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.get $l3
      i32.store
      local.get $l2
      local.get $l9
      i64.store offset=8
      i32.const 1048920
      i32.const 43
      local.get $l2
      i32.const 8
      i32.add
      i32.const 1049536
      i32.const 1049476
      call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
      unreachable
    end
    local.get $l2
    local.get $l2
    i64.load offset=84 align=4
    i64.store offset=20 align=4
    local.get $l2
    local.get $l4
    i32.store offset=16
    local.get $l2
    local.get $l5
    i32.store offset=12
    local.get $l2
    local.get $l6
    i32.store offset=8
    local.get $l2
    i32.const 64
    i32.add
    i32.const 4
    i32.or
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
    local.get $l2
    i32.const 80
    i32.add
    i32.const 8
    i32.add
    local.get $l2
    i32.const 76
    i32.add
    i32.load
    local.tee $l3
    i32.store
    local.get $l2
    local.get $l2
    i64.load offset=68 align=4
    local.tee $l9
    i64.store offset=80
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $l3
    i32.store
    local.get $l2
    local.get $l9
    i64.store offset=8
    i32.const 1048920
    i32.const 43
    local.get $l2
    i32.const 8
    i32.add
    i32.const 1049536
    i32.const 1049492
    call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
    unreachable)
  (func $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h30941f502999929dE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p0
              i32.load8_u
              br_table $B4 $B3 $B2 $B1 $B4
            end
            local.get $l2
            local.get $p0
            i32.const 4
            i32.add
            i32.load
            local.tee $p0
            i32.store offset=4
            local.get $l2
            i32.const 8
            i32.add
            local.get $p0
            call $_ZN3std3sys4wasi2os12error_string17h0414d6055eae23aaE
            local.get $l2
            i32.const 60
            i32.add
            i32.const 2
            i32.store
            local.get $l2
            i32.const 36
            i32.add
            i32.const 3
            i32.store
            local.get $l2
            i64.const 3
            i64.store offset=44 align=4
            local.get $l2
            i32.const 1050484
            i32.store offset=40
            local.get $l2
            i32.const 4
            i32.store offset=28
            local.get $l2
            local.get $l2
            i32.const 24
            i32.add
            i32.store offset=56
            local.get $l2
            local.get $l2
            i32.const 4
            i32.add
            i32.store offset=32
            local.get $l2
            local.get $l2
            i32.const 8
            i32.add
            i32.store offset=24
            local.get $p1
            local.get $l2
            i32.const 40
            i32.add
            call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
            local.set $p0
            local.get $l2
            i32.load offset=12
            local.tee $p1
            i32.eqz
            br_if $B0
            local.get $l2
            i32.load offset=8
            local.get $p1
            i32.const 1
            call $__rust_dealloc
            br $B0
          end
          local.get $p0
          i32.load8_u offset=1
          local.set $p0
          local.get $l2
          i32.const 60
          i32.add
          i32.const 1
          i32.store
          local.get $l2
          i64.const 1
          i64.store offset=44 align=4
          local.get $l2
          i32.const 1049524
          i32.store offset=40
          local.get $l2
          i32.const 5
          i32.store offset=12
          local.get $l2
          local.get $p0
          i32.const 32
          i32.xor
          i32.const 63
          i32.and
          i32.const 2
          i32.shl
          local.tee $p0
          i32.const 1052868
          i32.add
          i32.load
          i32.store offset=28
          local.get $l2
          local.get $p0
          i32.const 1053124
          i32.add
          i32.load
          i32.store offset=24
          local.get $l2
          local.get $l2
          i32.const 8
          i32.add
          i32.store offset=56
          local.get $l2
          local.get $l2
          i32.const 24
          i32.add
          i32.store offset=8
          local.get $p1
          local.get $l2
          i32.const 40
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
          local.set $p0
          br $B0
        end
        local.get $p0
        i32.const 4
        i32.add
        i32.load
        local.tee $p0
        i32.load
        local.get $p0
        i32.load offset=4
        local.get $p1
        call $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE
        local.set $p0
        br $B0
      end
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $p0
      i32.load
      local.get $p1
      local.get $p0
      i32.load offset=4
      i32.load offset=16
      call_indirect (type $t3) $T0
      local.set $p0
    end
    local.get $l2
    i32.const 64
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN3std3env4args17h02d9a592c6ab5652E (type $t0) (param $p0 i32)
    local.get $p0
    call $_ZN3std3env7args_os17hd21bfff4539d3ffdE)
  (func $_ZN3std3env7args_os17hd21bfff4539d3ffdE (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i64) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32) (local $l13 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l1
    global.set $g0
    local.get $l1
    i32.const 16
    i32.add
    call $_ZN4wasi13lib_generated14args_sizes_get17h4999a6465b98090cE
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  block $B7
                    block $B8
                      local.get $l1
                      i32.load16_u offset=16
                      br_if $B8
                      local.get $l1
                      i32.const 24
                      i32.add
                      i32.load
                      local.set $l2
                      block $B9
                        block $B10
                          local.get $l1
                          i32.load offset=20
                          local.tee $l3
                          br_if $B10
                          i32.const 4
                          local.set $l4
                          br $B9
                        end
                        local.get $l3
                        i32.const 1073741823
                        i32.and
                        local.tee $l5
                        local.get $l3
                        i32.ne
                        br_if $B5
                        local.get $l3
                        i32.const 2
                        i32.shl
                        local.tee $l6
                        i32.const -1
                        i32.le_s
                        br_if $B5
                        local.get $l5
                        local.get $l3
                        i32.eq
                        i32.const 2
                        i32.shl
                        local.set $l5
                        block $B11
                          block $B12
                            local.get $l6
                            br_if $B12
                            local.get $l5
                            local.set $l4
                            br $B11
                          end
                          local.get $l6
                          local.get $l5
                          call $__rust_alloc
                          local.set $l4
                        end
                        local.get $l4
                        i32.eqz
                        br_if $B7
                      end
                      block $B13
                        block $B14
                          local.get $l2
                          i32.eqz
                          br_if $B14
                          local.get $l2
                          i32.const 0
                          i32.lt_s
                          br_if $B5
                          local.get $l2
                          i32.const 1
                          call $__rust_alloc
                          local.tee $l7
                          i32.eqz
                          br_if $B3
                          local.get $l1
                          i32.const 8
                          i32.add
                          local.get $l4
                          local.get $l7
                          call $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E
                          local.get $l1
                          i32.load16_u offset=8
                          i32.eqz
                          br_if $B13
                          local.get $l7
                          local.get $l2
                          i32.const 1
                          call $__rust_dealloc
                          br $B2
                        end
                        i32.const 1
                        local.set $l7
                        local.get $l1
                        local.get $l4
                        i32.const 1
                        call $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E
                        local.get $l1
                        i32.load16_u
                        br_if $B2
                      end
                      block $B15
                        block $B16
                          local.get $l3
                          br_if $B16
                          local.get $l1
                          i64.const 4
                          i64.store offset=16
                          i32.const 0
                          local.set $l6
                          br $B15
                        end
                        local.get $l3
                        i64.extend_i32_u
                        i64.const 12
                        i64.mul
                        local.tee $l8
                        i64.const 32
                        i64.shr_u
                        i32.wrap_i64
                        local.tee $l5
                        br_if $B5
                        local.get $l8
                        i32.wrap_i64
                        local.tee $l6
                        i32.const -1
                        i32.le_s
                        br_if $B5
                        local.get $l5
                        i32.eqz
                        i32.const 2
                        i32.shl
                        local.set $l5
                        block $B17
                          block $B18
                            local.get $l6
                            br_if $B18
                            local.get $l5
                            local.set $l9
                            br $B17
                          end
                          local.get $l6
                          local.get $l5
                          call $__rust_alloc
                          local.set $l9
                        end
                        local.get $l9
                        i32.eqz
                        br_if $B6
                        local.get $l1
                        i32.const 0
                        i32.store offset=24
                        local.get $l1
                        local.get $l9
                        i32.store offset=16
                        local.get $l1
                        local.get $l3
                        i32.store offset=20
                        local.get $l3
                        i32.const 2
                        i32.shl
                        local.set $l10
                        i32.const 0
                        local.set $l6
                        local.get $l4
                        local.set $l11
                        loop $L19
                          i32.const 1
                          local.set $l12
                          block $B20
                            local.get $l11
                            i32.load
                            local.tee $l13
                            call $strlen
                            local.tee $l5
                            i32.eqz
                            br_if $B20
                            local.get $l5
                            i32.const 0
                            i32.lt_s
                            br_if $B5
                            local.get $l5
                            i32.const 1
                            call $__rust_alloc
                            local.tee $l12
                            i32.eqz
                            br_if $B4
                          end
                          local.get $l12
                          local.get $l13
                          local.get $l5
                          call $memcpy
                          local.set $l13
                          block $B21
                            local.get $l6
                            local.get $l1
                            i32.load offset=20
                            i32.ne
                            br_if $B21
                            local.get $l1
                            i32.const 16
                            i32.add
                            local.get $l6
                            call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17h49fd6398be4a4a9aE
                            local.get $l1
                            i32.load offset=16
                            local.set $l9
                            local.get $l1
                            i32.load offset=24
                            local.set $l6
                          end
                          local.get $l11
                          i32.const 4
                          i32.add
                          local.set $l11
                          local.get $l9
                          local.get $l6
                          i32.const 12
                          i32.mul
                          i32.add
                          local.tee $l12
                          local.get $l5
                          i32.store offset=8
                          local.get $l12
                          local.get $l5
                          i32.store offset=4
                          local.get $l12
                          local.get $l13
                          i32.store
                          local.get $l1
                          local.get $l6
                          i32.const 1
                          i32.add
                          local.tee $l6
                          i32.store offset=24
                          local.get $l10
                          i32.const -4
                          i32.add
                          local.tee $l10
                          br_if $L19
                        end
                        local.get $l3
                        i32.const 2
                        i32.shl
                        local.tee $l5
                        i32.eqz
                        br_if $B15
                        local.get $l4
                        local.get $l5
                        i32.const 4
                        call $__rust_dealloc
                      end
                      local.get $l1
                      i32.load offset=20
                      local.set $l11
                      local.get $l1
                      i32.load offset=16
                      local.set $l5
                      block $B22
                        local.get $l2
                        i32.eqz
                        br_if $B22
                        local.get $l7
                        local.get $l2
                        i32.const 1
                        call $__rust_dealloc
                      end
                      local.get $l5
                      br_if $B0
                    end
                    i32.const 4
                    local.set $l5
                    i32.const 0
                    local.set $l11
                    br $B1
                  end
                  local.get $l6
                  local.get $l5
                  call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
                  unreachable
                end
                local.get $l6
                local.get $l5
                call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
                unreachable
              end
              call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
              unreachable
            end
            local.get $l5
            i32.const 1
            call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
            unreachable
          end
          local.get $l2
          i32.const 1
          call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
          unreachable
        end
        i32.const 0
        local.set $l11
        block $B23
          local.get $l3
          br_if $B23
          i32.const 4
          local.set $l5
          br $B1
        end
        i32.const 4
        local.set $l5
        local.get $l4
        local.get $l3
        i32.const 2
        i32.shl
        i32.const 4
        call $__rust_dealloc
      end
      i32.const 0
      local.set $l6
    end
    local.get $p0
    local.get $l5
    i32.store offset=8
    local.get $p0
    local.get $l11
    i32.store offset=4
    local.get $p0
    local.get $l5
    i32.store
    local.get $p0
    local.get $l5
    local.get $l6
    i32.const 12
    i32.mul
    i32.add
    i32.store offset=12
    local.get $l1
    i32.const 32
    i32.add
    global.set $g0)
  (func $_ZN73_$LT$std..env..Args$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h1330ff0958e7dcc9E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i64)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.load offset=8
            local.tee $l3
            local.get $p1
            i32.load offset=12
            i32.eq
            br_if $B3
            local.get $p1
            local.get $l3
            i32.const 12
            i32.add
            i32.store offset=8
            local.get $l3
            i32.load
            local.tee $p1
            br_if $B2
          end
          local.get $p0
          i32.const 0
          i32.store
          br $B1
        end
        local.get $l3
        i32.load offset=4
        local.set $l4
        local.get $l2
        i32.const 40
        i32.add
        local.get $p1
        local.get $l3
        i32.load offset=8
        local.tee $l3
        call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
        local.get $l2
        i32.load offset=40
        br_if $B0
        local.get $l2
        i32.const 24
        i32.add
        i32.const 8
        i32.add
        local.get $l4
        i32.store
        local.get $l2
        i32.const 8
        i32.add
        i32.const 8
        i32.add
        local.get $l3
        i32.store
        local.get $l2
        local.get $p1
        i32.store offset=28
        local.get $l2
        local.get $l2
        i64.load offset=28 align=4
        local.tee $l5
        i64.store offset=8
        local.get $p0
        i32.const 8
        i32.add
        local.get $l3
        i32.store
        local.get $p0
        local.get $l5
        i64.store align=4
      end
      local.get $l2
      i32.const 64
      i32.add
      global.set $g0
      return
    end
    local.get $l2
    local.get $l2
    i64.load offset=44 align=4
    i64.store offset=52 align=4
    local.get $l2
    local.get $l3
    i32.store offset=48
    local.get $l2
    local.get $l4
    i32.store offset=44
    local.get $l2
    local.get $p1
    i32.store offset=40
    local.get $l2
    i32.const 24
    i32.add
    i32.const 4
    i32.or
    local.get $l2
    i32.const 40
    i32.add
    call $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $l2
    i32.const 36
    i32.add
    i32.load
    local.tee $p1
    i32.store
    local.get $l2
    local.get $l2
    i64.load offset=28 align=4
    local.tee $l5
    i64.store offset=8
    local.get $l2
    i32.const 40
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.store
    local.get $l2
    local.get $l5
    i64.store offset=40
    i32.const 1048920
    i32.const 43
    local.get $l2
    i32.const 40
    i32.add
    i32.const 1049536
    i32.const 1049508
    call $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE
    unreachable)
  (func $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32)
    i32.const 40
    local.set $l1
    block $B0
      local.get $p0
      i32.const 65535
      i32.gt_u
      br_if $B0
      i32.const 2
      local.set $l1
      i32.const 1052594
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 3
      local.set $l1
      i32.const 1052596
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 1
      local.set $l1
      i32.const 1052598
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 1052600
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 11
      local.set $l1
      i32.const 1052602
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 7
      local.set $l1
      i32.const 1052604
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 6
      local.set $l1
      i32.const 1052606
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 9
      local.set $l1
      i32.const 1052608
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 8
      local.set $l1
      i32.const 1052610
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 0
      local.set $l1
      i32.const 1052612
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 35
      local.set $l1
      i32.const 1052614
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 20
      local.set $l1
      i32.const 1052616
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 22
      local.set $l1
      i32.const 1052618
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 12
      local.set $l1
      i32.const 1052620
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 13
      local.set $l1
      i32.const 1052622
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 36
      local.set $l1
      i32.const 1052624
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      br_if $B0
      i32.const 38
      i32.const 40
      i32.const 1052626
      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
      i32.const 65535
      i32.and
      local.get $p0
      i32.eq
      select
      local.set $l1
    end
    local.get $l1)
  (func $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          local.get $p1
          i32.const 8
          i32.add
          i32.load
          local.tee $l3
          br_if $B2
          local.get $p0
          i32.const 4
          i32.store8
          br $B1
        end
        local.get $p1
        i32.load
        local.set $l4
        i32.const 0
        local.set $l5
        loop $L3
          block $B4
            block $B5
              block $B6
                local.get $l3
                local.get $l5
                i32.lt_u
                br_if $B6
                local.get $l2
                local.get $l3
                local.get $l5
                i32.sub
                local.tee $l6
                i32.store offset=12
                local.get $l2
                local.get $l4
                local.get $l5
                i32.add
                local.tee $l7
                i32.store offset=8
                local.get $l2
                i32.const 16
                i32.add
                i32.const 1
                local.get $l2
                i32.const 8
                i32.add
                i32.const 1
                call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
                block $B7
                  block $B8
                    block $B9
                      block $B10
                        local.get $l2
                        i32.load16_u offset=16
                        br_if $B10
                        local.get $l2
                        i32.load offset=20
                        local.set $l8
                        br $B9
                      end
                      local.get $l2
                      local.get $l2
                      i32.load16_u offset=18
                      i32.store16 offset=30
                      local.get $l6
                      local.set $l8
                      local.get $l2
                      i32.const 30
                      i32.add
                      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                      i32.const 65535
                      i32.and
                      local.tee $l9
                      i32.const 1052592
                      call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                      i32.const 65535
                      i32.and
                      i32.ne
                      br_if $B8
                    end
                    local.get $p1
                    i32.const 0
                    i32.store8 offset=12
                    local.get $l8
                    i32.eqz
                    br_if $B7
                    local.get $l8
                    local.get $l5
                    i32.add
                    local.set $l5
                    br $B4
                  end
                  local.get $p1
                  i32.const 0
                  i32.store8 offset=12
                  local.get $l9
                  call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                  i32.const 255
                  i32.and
                  i32.const 35
                  i32.eq
                  br_if $B4
                  local.get $p0
                  i32.const 0
                  i32.store
                  local.get $p0
                  i32.const 4
                  i32.add
                  local.get $l9
                  i32.store
                  br $B5
                end
                local.get $p0
                i32.const 1049588
                i64.extend_i32_u
                i64.const 32
                i64.shl
                i64.const 2
                i64.or
                i64.store align=4
                br $B5
              end
              local.get $l5
              local.get $l3
              i32.const 1049640
              call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
              unreachable
            end
            local.get $l5
            i32.eqz
            br_if $B1
            local.get $p1
            i32.const 8
            i32.add
            local.tee $l5
            i32.const 0
            i32.store
            local.get $l6
            i32.eqz
            br_if $B1
            local.get $l4
            local.get $l7
            local.get $l6
            call $memmove
            drop
            local.get $l5
            local.get $l6
            i32.store
            br $B1
          end
          local.get $l3
          local.get $l5
          i32.gt_u
          br_if $L3
        end
        local.get $p0
        i32.const 4
        i32.store8
        local.get $l5
        i32.eqz
        br_if $B1
        local.get $l3
        local.get $l5
        i32.lt_u
        br_if $B0
        local.get $p1
        i32.const 8
        i32.add
        local.tee $l8
        i32.const 0
        i32.store
        local.get $l3
        local.get $l5
        i32.sub
        local.tee $l3
        i32.eqz
        br_if $B1
        local.get $p1
        i32.load
        local.tee $l6
        local.get $l6
        local.get $l5
        i32.add
        local.get $l3
        call $memmove
        drop
        local.get $l8
        local.get $l3
        i32.store
      end
      local.get $l2
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    local.get $l5
    local.get $l3
    i32.const 1049144
    call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
    unreachable)
  (func $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$14write_all_cold17h2b92cc5afab342a7E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i64)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        local.get $p1
        i32.const 4
        i32.add
        local.tee $l5
        i32.load
        local.get $p1
        i32.const 8
        i32.add
        i32.load
        i32.sub
        local.get $p3
        i32.ge_u
        br_if $B1
        local.get $l4
        i32.const 8
        i32.add
        local.get $p1
        call $_ZN3std2io8buffered9bufwriter18BufWriter$LT$W$GT$9flush_buf17h03d73f4955d10e5aE
        local.get $l4
        i32.load8_u offset=8
        i32.const 4
        i32.eq
        br_if $B1
        local.get $l4
        i64.load offset=8
        local.tee $l6
        i32.wrap_i64
        i32.const 255
        i32.and
        i32.const 4
        i32.eq
        br_if $B1
        local.get $p0
        local.get $l6
        i64.store align=4
        br $B0
      end
      block $B2
        local.get $l5
        i32.load
        local.get $p3
        i32.le_u
        br_if $B2
        local.get $p1
        i32.load
        local.get $p1
        i32.const 8
        i32.add
        local.tee $p1
        i32.load
        local.tee $l5
        i32.add
        local.get $p2
        local.get $p3
        call $memcpy
        drop
        local.get $p0
        i32.const 4
        i32.store8
        local.get $p1
        local.get $l5
        local.get $p3
        i32.add
        i32.store
        br $B0
      end
      local.get $p1
      i32.const 1
      i32.store8 offset=12
      local.get $l4
      i32.const 8
      i32.add
      local.get $p1
      local.get $p2
      local.get $p3
      call $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E
      local.get $p1
      i32.const 0
      i32.store8 offset=12
      local.get $p0
      local.get $l4
      i64.load offset=8
      i64.store align=4
    end
    local.get $l4
    i32.const 16
    i32.add
    global.set $g0)
  (func $_ZN60_$LT$std..io..stdio..StdoutRaw$u20$as$u20$std..io..Write$GT$9write_all17hc26b97e06418f486E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i64) (local $l6 i32) (local $l7 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l4
    global.set $g0
    i64.const 4
    local.set $l5
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p3
              i32.eqz
              br_if $B4
              loop $L5
                local.get $l4
                local.get $p3
                i32.store offset=12
                local.get $l4
                local.get $p2
                i32.store offset=8
                local.get $l4
                i32.const 16
                i32.add
                i32.const 1
                local.get $l4
                i32.const 8
                i32.add
                i32.const 1
                call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
                block $B6
                  block $B7
                    block $B8
                      local.get $l4
                      i32.load16_u offset=16
                      local.tee $l6
                      br_if $B8
                      local.get $l4
                      i32.load offset=20
                      local.tee $l7
                      br_if $B7
                      i32.const 1050536
                      local.set $l7
                      i64.const 2
                      local.set $l5
                      br $B2
                    end
                    local.get $l4
                    local.get $l4
                    i32.load16_u offset=18
                    i32.store16 offset=30
                    local.get $l4
                    i32.const 30
                    i32.add
                    call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                    i32.const 65535
                    i32.and
                    local.tee $l7
                    call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                    i32.const 255
                    i32.and
                    i32.const 35
                    i32.eq
                    br_if $B6
                    i64.const 0
                    local.set $l5
                    br $B2
                  end
                  local.get $p3
                  local.get $l7
                  i32.lt_u
                  br_if $B3
                  local.get $p2
                  local.get $l7
                  i32.add
                  local.set $p2
                  local.get $p3
                  local.get $l7
                  i32.sub
                  local.set $p3
                end
                local.get $p3
                br_if $L5
              end
            end
            br $B1
          end
          local.get $l7
          local.get $p3
          i32.const 1050692
          call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
          unreachable
        end
        i32.const 1052592
        call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
        local.set $p3
        local.get $l6
        i32.eqz
        br_if $B1
        local.get $l7
        local.get $p3
        i32.const 65535
        i32.and
        i32.ne
        br_if $B1
        local.get $p0
        i32.const 4
        i32.store8
        br $B0
      end
      local.get $p0
      local.get $l7
      i64.extend_i32_u
      i64.const 32
      i64.shl
      local.get $l5
      i64.or
      i64.store align=4
    end
    local.get $l4
    i32.const 32
    i32.add
    global.set $g0)
  (func $_ZN3std3sys4wasi2os12error_string17h0414d6055eae23aaE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 1056
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            local.get $l2
            i32.const 0
            i32.const 1024
            call $memset
            local.tee $l2
            i32.const 1024
            call $strerror_r
            i32.const 0
            i32.lt_s
            br_if $B3
            local.get $l2
            i32.const 1024
            i32.add
            local.get $l2
            local.get $l2
            call $strlen
            call $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E
            local.get $l2
            i32.load offset=1024
            br_if $B2
            local.get $l2
            i32.load offset=1028
            local.set $l3
            block $B4
              block $B5
                local.get $l2
                i32.const 1032
                i32.add
                i32.load
                local.tee $p1
                br_if $B5
                i32.const 1
                local.set $l4
                br $B4
              end
              local.get $p1
              i32.const 0
              i32.lt_s
              br_if $B1
              local.get $p1
              i32.const 1
              call $__rust_alloc
              local.tee $l4
              i32.eqz
              br_if $B0
            end
            local.get $l4
            local.get $l3
            local.get $p1
            call $memcpy
            local.set $l3
            local.get $p0
            local.get $p1
            i32.store offset=8
            local.get $p0
            local.get $p1
            i32.store offset=4
            local.get $p0
            local.get $l3
            i32.store
            local.get $l2
            i32.const 1056
            i32.add
            global.set $g0
            return
          end
          local.get $l2
          i32.const 1044
          i32.add
          i32.const 0
          i32.store
          local.get $l2
          i32.const 1048792
          i32.store offset=1040
          local.get $l2
          i64.const 1
          i64.store offset=1028 align=4
          local.get $l2
          i32.const 1052444
          i32.store offset=1024
          local.get $l2
          i32.const 1024
          i32.add
          i32.const 1052484
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $l2
        local.get $l2
        i64.load offset=1028 align=4
        i64.store offset=1048
        i32.const 1048920
        i32.const 43
        local.get $l2
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
    local.get $p1
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5write17hfeddc07cce766a8dE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32)
    block $B0
      local.get $p1
      i32.const 4
      i32.add
      i32.load
      local.get $p1
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $l5
      i32.sub
      local.get $p3
      i32.ge_u
      br_if $B0
      local.get $p1
      local.get $l5
      local.get $p3
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $l5
    end
    local.get $p1
    i32.load
    local.get $l5
    i32.add
    local.get $p2
    local.get $p3
    call $memcpy
    drop
    local.get $p0
    local.get $p3
    i32.store offset=4
    local.get $l4
    local.get $l5
    local.get $p3
    i32.add
    i32.store
    local.get $p0
    i32.const 0
    i32.store)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$14write_vectored17h5e2f18a5d90c2318E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32)
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p3
            i32.const 3
            i32.shl
            local.tee $l4
            i32.eqz
            br_if $B3
            local.get $p3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            local.tee $l5
            i32.const 1
            i32.add
            local.tee $l6
            i32.const 7
            i32.and
            local.set $l7
            local.get $l5
            i32.const 7
            i32.ge_u
            br_if $B2
            i32.const 0
            local.set $l6
            local.get $p2
            local.set $l5
            br $B1
          end
          local.get $p1
          i32.const 4
          i32.add
          local.set $l8
          local.get $p1
          i32.const 8
          i32.add
          local.set $l5
          i32.const 0
          local.set $l6
          br $B0
        end
        local.get $p2
        i32.const 60
        i32.add
        local.set $l5
        local.get $l6
        i32.const 1073741816
        i32.and
        local.set $l9
        i32.const 0
        local.set $l6
        loop $L4
          local.get $l5
          i32.load
          local.get $l5
          i32.const -8
          i32.add
          i32.load
          local.get $l5
          i32.const -16
          i32.add
          i32.load
          local.get $l5
          i32.const -24
          i32.add
          i32.load
          local.get $l5
          i32.const -32
          i32.add
          i32.load
          local.get $l5
          i32.const -40
          i32.add
          i32.load
          local.get $l5
          i32.const -48
          i32.add
          i32.load
          local.get $l5
          i32.const -56
          i32.add
          i32.load
          local.get $l6
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          i32.add
          local.set $l6
          local.get $l5
          i32.const 64
          i32.add
          local.set $l5
          local.get $l9
          i32.const -8
          i32.add
          local.tee $l9
          br_if $L4
        end
        local.get $l5
        i32.const -60
        i32.add
        local.set $l5
      end
      block $B5
        local.get $l7
        i32.eqz
        br_if $B5
        local.get $l5
        i32.const 4
        i32.add
        local.set $l5
        loop $L6
          local.get $l5
          i32.load
          local.get $l6
          i32.add
          local.set $l6
          local.get $l5
          i32.const 8
          i32.add
          local.set $l5
          local.get $l7
          i32.const -1
          i32.add
          local.tee $l7
          br_if $L6
        end
      end
      local.get $p1
      i32.const 8
      i32.add
      local.set $l5
      local.get $p1
      i32.const 4
      i32.add
      local.tee $l8
      i32.load
      local.get $p1
      i32.load offset=8
      local.tee $l7
      i32.sub
      local.get $l6
      i32.ge_u
      br_if $B0
      local.get $p1
      local.get $l7
      local.get $l6
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
    end
    block $B7
      local.get $p3
      i32.eqz
      br_if $B7
      local.get $p2
      local.get $l4
      i32.add
      local.set $p3
      local.get $l5
      i32.load
      local.set $l5
      loop $L8
        local.get $p2
        i32.load
        local.set $l9
        block $B9
          local.get $l8
          i32.load
          local.get $l5
          i32.sub
          local.get $p2
          i32.const 4
          i32.add
          i32.load
          local.tee $l7
          i32.ge_u
          br_if $B9
          local.get $p1
          local.get $l5
          local.get $l7
          call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
          local.get $p1
          i32.load offset=8
          local.set $l5
        end
        local.get $p1
        i32.load
        local.get $l5
        i32.add
        local.get $l9
        local.get $l7
        call $memcpy
        drop
        local.get $p1
        local.get $l5
        local.get $l7
        i32.add
        local.tee $l5
        i32.store offset=8
        local.get $p3
        local.get $p2
        i32.const 8
        i32.add
        local.tee $p2
        i32.ne
        br_if $L8
      end
    end
    local.get $p0
    i32.const 0
    i32.store
    local.get $p0
    local.get $l6
    i32.store offset=4)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$17is_write_vectored17hbcc64e0de2a7bd6eE (type $t10) (param $p0 i32) (result i32)
    i32.const 1)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$9write_all17hacb9a4b06548536cE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32)
    block $B0
      local.get $p1
      i32.const 4
      i32.add
      i32.load
      local.get $p1
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $l5
      i32.sub
      local.get $p3
      i32.ge_u
      br_if $B0
      local.get $p1
      local.get $l5
      local.get $p3
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $l5
    end
    local.get $p1
    i32.load
    local.get $l5
    i32.add
    local.get $p2
    local.get $p3
    call $memcpy
    drop
    local.get $p0
    i32.const 4
    i32.store8
    local.get $l4
    local.get $l5
    local.get $p3
    i32.add
    i32.store)
  (func $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5flush17h3e171420e6883b71E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    i32.const 4
    i32.store8)
  (func $_ZN3std2io5Write18write_all_vectored17h3007a4983f10eddeE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p3
              br_if $B4
              i32.const 0
              local.set $l5
              br $B3
            end
            local.get $p2
            i32.const 4
            i32.add
            local.set $l6
            local.get $p3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.set $l7
            i32.const 0
            local.set $l5
            block $B5
              loop $L6
                local.get $l6
                i32.load
                br_if $B5
                local.get $l6
                i32.const 8
                i32.add
                local.set $l6
                local.get $l7
                local.get $l5
                i32.const 1
                i32.add
                local.tee $l5
                i32.ne
                br_if $L6
              end
              local.get $l7
              local.set $l5
            end
            local.get $l5
            local.get $p3
            i32.gt_u
            br_if $B2
          end
          local.get $p3
          local.get $l5
          i32.sub
          local.tee $l7
          i32.eqz
          br_if $B1
          local.get $p2
          local.get $l5
          i32.const 3
          i32.shl
          i32.add
          local.set $l5
          block $B7
            loop $L8
              local.get $l4
              i32.const 8
              i32.add
              i32.const 2
              local.get $l5
              local.get $l7
              call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
              block $B9
                block $B10
                  local.get $l4
                  i32.load16_u offset=8
                  br_if $B10
                  local.get $l4
                  i32.load offset=12
                  local.tee $l8
                  br_if $B9
                  local.get $p0
                  i32.const 1050536
                  i64.extend_i32_u
                  i64.const 32
                  i64.shl
                  i64.const 2
                  i64.or
                  i64.store align=4
                  br $B0
                end
                local.get $l4
                local.get $l4
                i32.load16_u offset=10
                i32.store16 offset=6
                local.get $l4
                i32.const 6
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee $l6
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if $L8
                local.get $p0
                i32.const 0
                i32.store
                local.get $p0
                i32.const 4
                i32.add
                local.get $l6
                i32.store
                br $B0
              end
              local.get $l5
              i32.const 4
              i32.add
              local.set $l6
              local.get $l7
              i32.const -1
              i32.add
              i32.const 536870911
              i32.and
              i32.const 1
              i32.add
              local.set $l9
              i32.const 0
              local.set $p3
              i32.const 0
              local.set $p2
              block $B11
                loop $L12
                  local.get $l6
                  i32.load
                  local.get $p2
                  i32.add
                  local.tee $l10
                  local.get $l8
                  i32.gt_u
                  br_if $B11
                  local.get $l6
                  i32.const 8
                  i32.add
                  local.set $l6
                  local.get $l10
                  local.set $p2
                  local.get $l9
                  local.get $p3
                  i32.const 1
                  i32.add
                  local.tee $p3
                  i32.ne
                  br_if $L12
                end
                local.get $l10
                local.set $p2
                local.get $l9
                local.set $p3
              end
              block $B13
                local.get $l7
                local.get $p3
                i32.lt_u
                br_if $B13
                local.get $l7
                local.get $p3
                i32.sub
                local.tee $l7
                i32.eqz
                br_if $B1
                local.get $l5
                local.get $p3
                i32.const 3
                i32.shl
                i32.add
                local.tee $l5
                i32.load offset=4
                local.tee $p3
                local.get $l8
                local.get $p2
                i32.sub
                local.tee $l6
                i32.lt_u
                br_if $B7
                local.get $l5
                i32.const 4
                i32.add
                local.get $p3
                local.get $l6
                i32.sub
                i32.store
                local.get $l5
                local.get $l5
                i32.load
                local.get $l6
                i32.add
                i32.store
                br $L8
              end
            end
            local.get $p3
            local.get $l7
            i32.const 1050676
            call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
            unreachable
          end
          local.get $l4
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get $l4
          i32.const 1048792
          i32.store offset=24
          local.get $l4
          i64.const 1
          i64.store offset=12 align=4
          local.get $l4
          i32.const 1052112
          i32.store offset=8
          local.get $l4
          i32.const 8
          i32.add
          i32.const 1052152
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $l5
        local.get $p3
        i32.const 1050676
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get $p0
      i32.const 4
      i32.store8
    end
    local.get $l4
    i32.const 32
    i32.add
    global.set $g0)
  (func $_ZN61_$LT$$RF$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17hc123b657c412bbeeE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.load
            i32.load
            local.tee $p1
            i32.load
            i32.const 1059292
            i32.eq
            br_if $B3
            local.get $p1
            i32.load8_u offset=28
            local.set $l4
            local.get $p1
            i32.const 1
            i32.store8 offset=28
            local.get $l3
            local.get $l4
            i32.const 1
            i32.and
            local.tee $l4
            i32.store8 offset=8
            local.get $l4
            br_if $B1
            local.get $p1
            i32.const 1
            i32.store offset=4
            local.get $p1
            i32.const 1059292
            i32.store
            br $B2
          end
          local.get $p1
          i32.load offset=4
          local.tee $l4
          i32.const 1
          i32.add
          local.tee $l5
          local.get $l4
          i32.lt_u
          br_if $B0
          local.get $p1
          local.get $l5
          i32.store offset=4
        end
        local.get $l3
        local.get $p1
        i32.store offset=4
        local.get $l3
        i32.const 4
        i32.store8 offset=12
        local.get $l3
        local.get $l3
        i32.const 4
        i32.add
        i32.store offset=8
        local.get $l3
        i32.const 24
        i32.add
        i32.const 16
        i32.add
        local.get $p2
        i32.const 16
        i32.add
        i64.load align=4
        i64.store
        local.get $l3
        i32.const 24
        i32.add
        i32.const 8
        i32.add
        local.get $p2
        i32.const 8
        i32.add
        i64.load align=4
        i64.store
        local.get $l3
        local.get $p2
        i64.load align=4
        i64.store offset=24
        block $B4
          block $B5
            local.get $l3
            i32.const 8
            i32.add
            i32.const 1050760
            local.get $l3
            i32.const 24
            i32.add
            call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
            i32.eqz
            br_if $B5
            block $B6
              local.get $l3
              i32.load8_u offset=12
              i32.const 4
              i32.ne
              br_if $B6
              local.get $p0
              i32.const 1050724
              i64.extend_i32_u
              i64.const 32
              i64.shl
              i64.const 2
              i64.or
              i64.store align=4
              br $B4
            end
            local.get $p0
            local.get $l3
            i64.load offset=12 align=4
            i64.store align=4
            br $B4
          end
          local.get $p0
          i32.const 4
          i32.store8
          local.get $l3
          i32.load8_u offset=12
          i32.const 3
          i32.ne
          br_if $B4
          local.get $l3
          i32.const 16
          i32.add
          i32.load
          local.tee $p1
          i32.load
          local.get $p1
          i32.load offset=4
          i32.load
          call_indirect (type $t0) $T0
          block $B7
            local.get $p1
            i32.load offset=4
            local.tee $p2
            i32.load offset=4
            local.tee $p0
            i32.eqz
            br_if $B7
            local.get $p1
            i32.load
            local.get $p0
            local.get $p2
            i32.load offset=8
            call $__rust_dealloc
          end
          local.get $l3
          i32.load offset=16
          i32.const 12
          i32.const 4
          call $__rust_dealloc
        end
        local.get $l3
        i32.load offset=4
        local.tee $p1
        local.get $p1
        i32.load offset=4
        i32.const -1
        i32.add
        local.tee $p2
        i32.store offset=4
        block $B8
          local.get $p2
          br_if $B8
          local.get $p1
          i32.const 0
          i32.store8 offset=28
          local.get $p1
          i32.const 0
          i32.store
        end
        local.get $l3
        i32.const 48
        i32.add
        global.set $g0
        return
      end
      local.get $l3
      i32.const 44
      i32.add
      i32.const 0
      i32.store
      local.get $l3
      i32.const 40
      i32.add
      i32.const 1048792
      i32.store
      local.get $l3
      i64.const 1
      i64.store offset=28 align=4
      local.get $l3
      i32.const 1052308
      i32.store offset=24
      local.get $l3
      i32.const 8
      i32.add
      local.get $l3
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
  (func $_ZN3std2io5stdio6_print17ha3d1f6ac95b4f6cfE (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 96
    i32.sub
    local.tee $l1
    global.set $g0
    local.get $l1
    i32.const 16
    i32.add
    local.get $p0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l1
    i32.const 8
    i32.add
    local.get $p0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l1
    local.get $p0
    i64.load align=4
    i64.store
    local.get $l1
    i32.const 6
    i32.store offset=28
    local.get $l1
    i32.const 1050644
    i32.store offset=24
    block $B0
      block $B1
        i32.const 0
        i32.load8_u offset=1059218
        i32.eqz
        br_if $B1
        block $B2
          i32.const 0
          i32.load offset=1059284
          br_if $B2
          i32.const 0
          i64.const 1
          i64.store offset=1059284 align=4
          br $B1
        end
        i32.const 0
        i32.load offset=1059288
        local.set $p0
        i32.const 0
        i32.const 0
        i32.store offset=1059288
        local.get $p0
        i32.eqz
        br_if $B1
        local.get $p0
        i32.load8_u offset=8
        local.set $l2
        i32.const 1
        local.set $l3
        local.get $p0
        i32.const 1
        i32.store8 offset=8
        local.get $l1
        local.get $l2
        i32.const 1
        i32.and
        local.tee $l2
        i32.store8 offset=56
        block $B3
          local.get $l2
          br_if $B3
          block $B4
            i32.const 0
            i32.load offset=1059280
            i32.const 2147483647
            i32.and
            i32.eqz
            br_if $B4
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            local.set $l3
          end
          local.get $l1
          i32.const 4
          i32.store8 offset=60
          local.get $l1
          local.get $p0
          i32.const 12
          i32.add
          i32.store offset=56
          local.get $l1
          i32.const 72
          i32.add
          i32.const 16
          i32.add
          local.get $l1
          i32.const 16
          i32.add
          i64.load
          i64.store
          local.get $l1
          i32.const 72
          i32.add
          i32.const 8
          i32.add
          local.get $l1
          i32.const 8
          i32.add
          i64.load
          i64.store
          local.get $l1
          local.get $l1
          i64.load
          i64.store offset=72
          block $B5
            block $B6
              local.get $l1
              i32.const 56
              i32.add
              i32.const 1050784
              local.get $l1
              i32.const 72
              i32.add
              call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
              i32.eqz
              br_if $B6
              local.get $l1
              i32.load8_u offset=60
              i32.const 4
              i32.eq
              br_if $B5
              local.get $l1
              i32.load8_u offset=60
              i32.const 3
              i32.ne
              br_if $B5
              local.get $l1
              i32.const 64
              i32.add
              i32.load
              local.tee $l2
              i32.load
              local.get $l2
              i32.load offset=4
              i32.load
              call_indirect (type $t0) $T0
              block $B7
                local.get $l2
                i32.load offset=4
                local.tee $l4
                i32.load offset=4
                local.tee $l5
                i32.eqz
                br_if $B7
                local.get $l2
                i32.load
                local.get $l5
                local.get $l4
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get $l2
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              br $B5
            end
            local.get $l1
            i32.load8_u offset=60
            i32.const 3
            i32.ne
            br_if $B5
            local.get $l1
            i32.const 64
            i32.add
            i32.load
            local.tee $l2
            i32.load
            local.get $l2
            i32.load offset=4
            i32.load
            call_indirect (type $t0) $T0
            block $B8
              local.get $l2
              i32.load offset=4
              local.tee $l4
              i32.load offset=4
              local.tee $l5
              i32.eqz
              br_if $B8
              local.get $l2
              i32.load
              local.get $l5
              local.get $l4
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get $l1
            i32.load offset=64
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          block $B9
            local.get $l3
            i32.eqz
            br_if $B9
            i32.const 0
            i32.load offset=1059280
            i32.const 2147483647
            i32.and
            i32.eqz
            br_if $B9
            call $_ZN3std9panicking11panic_count17is_zero_slow_path17h1d2739269967e7eeE
            br_if $B9
            local.get $p0
            i32.const 1
            i32.store8 offset=9
          end
          local.get $p0
          i32.const 0
          i32.store8 offset=8
          i32.const 0
          i32.load offset=1059288
          local.set $l3
          i32.const 0
          local.get $p0
          i32.store offset=1059288
          local.get $l3
          i32.eqz
          br_if $B0
          local.get $l3
          local.get $l3
          i32.load
          local.tee $p0
          i32.const -1
          i32.add
          i32.store
          local.get $p0
          i32.const 1
          i32.ne
          br_if $B0
          local.get $l3
          call $_ZN5alloc4sync12Arc$LT$T$GT$9drop_slow17hb59be42dcf1ca9f1E
          br $B0
        end
        local.get $l1
        i32.const 92
        i32.add
        i32.const 0
        i32.store
        local.get $l1
        i32.const 88
        i32.add
        i32.const 1048792
        i32.store
        local.get $l1
        i64.const 1
        i64.store offset=76 align=4
        local.get $l1
        i32.const 1052308
        i32.store offset=72
        local.get $l1
        i32.const 56
        i32.add
        local.get $l1
        i32.const 72
        i32.add
        call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
        unreachable
      end
      block $B10
        i32.const 0
        i32.load offset=1059220
        i32.const 3
        i32.eq
        br_if $B10
        i32.const 0
        i32.load offset=1059220
        i32.const 3
        i32.eq
        br_if $B10
        local.get $l1
        i32.const 1059224
        i32.store offset=56
        local.get $l1
        local.get $l1
        i32.const 56
        i32.add
        i32.store offset=72
        i32.const 1059220
        i32.const 1
        local.get $l1
        i32.const 72
        i32.add
        i32.const 1050892
        i32.const 1050876
        call $_ZN3std4sync4once4Once10call_inner17h90795951c1d06984E
      end
      local.get $l1
      i32.const 1059224
      i32.store offset=44
      local.get $l1
      local.get $l1
      i32.const 44
      i32.add
      i32.store offset=56
      local.get $l1
      i32.const 72
      i32.add
      i32.const 16
      i32.add
      local.get $l1
      i32.const 16
      i32.add
      i64.load
      i64.store
      local.get $l1
      i32.const 72
      i32.add
      i32.const 8
      i32.add
      local.get $l1
      i32.const 8
      i32.add
      i64.load
      i64.store
      local.get $l1
      local.get $l1
      i64.load
      i64.store offset=72
      local.get $l1
      i32.const 32
      i32.add
      local.get $l1
      i32.const 56
      i32.add
      local.get $l1
      i32.const 72
      i32.add
      call $_ZN61_$LT$$RF$std..io..stdio..Stdout$u20$as$u20$std..io..Write$GT$9write_fmt17hc123b657c412bbeeE
      local.get $l1
      i32.load8_u offset=32
      i32.const 4
      i32.eq
      br_if $B0
      local.get $l1
      local.get $l1
      i64.load offset=32
      i64.store offset=48
      local.get $l1
      i32.const 92
      i32.add
      i32.const 2
      i32.store
      local.get $l1
      i32.const 68
      i32.add
      i32.const 6
      i32.store
      local.get $l1
      i64.const 2
      i64.store offset=76 align=4
      local.get $l1
      i32.const 1050612
      i32.store offset=72
      local.get $l1
      i32.const 5
      i32.store offset=60
      local.get $l1
      local.get $l1
      i32.const 56
      i32.add
      i32.store offset=88
      local.get $l1
      local.get $l1
      i32.const 48
      i32.add
      i32.store offset=64
      local.get $l1
      local.get $l1
      i32.const 24
      i32.add
      i32.store offset=56
      local.get $l1
      i32.const 72
      i32.add
      i32.const 1050628
      call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
      unreachable
    end
    local.get $l1
    i32.const 96
    i32.add
    global.set $g0)
  (func $_ZN3std2io5Write9write_all17hf2d1f3fd20cf6d1dE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        block $B2
          local.get $p3
          i32.eqz
          br_if $B2
          loop $L3
            local.get $l4
            local.get $p3
            i32.store offset=12
            local.get $l4
            local.get $p2
            i32.store offset=8
            local.get $l4
            i32.const 16
            i32.add
            i32.const 2
            local.get $l4
            i32.const 8
            i32.add
            i32.const 1
            call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
            block $B4
              block $B5
                block $B6
                  local.get $l4
                  i32.load16_u offset=16
                  br_if $B6
                  local.get $l4
                  i32.load offset=20
                  local.tee $l5
                  br_if $B5
                  local.get $p0
                  i32.const 1050536
                  i64.extend_i32_u
                  i64.const 32
                  i64.shl
                  i64.const 2
                  i64.or
                  i64.store align=4
                  br $B1
                end
                local.get $l4
                local.get $l4
                i32.load16_u offset=18
                i32.store16 offset=30
                local.get $l4
                i32.const 30
                i32.add
                call $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E
                i32.const 65535
                i32.and
                local.tee $l5
                call $_ZN3std3sys4wasi17decode_error_kind17h4ed3627b1f4053d7E
                i32.const 255
                i32.and
                i32.const 35
                i32.eq
                br_if $B4
                local.get $p0
                i32.const 0
                i32.store
                local.get $p0
                i32.const 4
                i32.add
                local.get $l5
                i32.store
                br $B1
              end
              local.get $p3
              local.get $l5
              i32.lt_u
              br_if $B0
              local.get $p2
              local.get $l5
              i32.add
              local.set $p2
              local.get $p3
              local.get $l5
              i32.sub
              local.set $p3
            end
            local.get $p3
            br_if $L3
          end
        end
        local.get $p0
        i32.const 4
        i32.store8
      end
      local.get $l4
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    local.get $l5
    local.get $p3
    i32.const 1050692
    call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
    unreachable)
  (func $_ZN3std2io5Write18write_all_vectored17h73403c20721d838fE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p3
              br_if $B4
              i32.const 0
              local.set $l5
              br $B3
            end
            local.get $p2
            i32.const 4
            i32.add
            local.set $l6
            local.get $p3
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.set $l7
            i32.const 0
            local.set $l5
            block $B5
              loop $L6
                local.get $l6
                i32.load
                br_if $B5
                local.get $l6
                i32.const 8
                i32.add
                local.set $l6
                local.get $l7
                local.get $l5
                i32.const 1
                i32.add
                local.tee $l5
                i32.ne
                br_if $L6
              end
              local.get $l7
              local.set $l5
            end
            local.get $l5
            local.get $p3
            i32.gt_u
            br_if $B2
          end
          local.get $p3
          local.get $l5
          i32.sub
          local.tee $l8
          i32.eqz
          br_if $B1
          local.get $p2
          local.get $l5
          i32.const 3
          i32.shl
          i32.add
          local.set $l9
          local.get $p1
          i32.const 4
          i32.add
          local.set $l10
          block $B7
            loop $L8
              local.get $l8
              i32.const -1
              i32.add
              i32.const 536870911
              i32.and
              local.tee $l6
              i32.const 1
              i32.add
              local.tee $l11
              i32.const 7
              i32.and
              local.set $l5
              block $B9
                block $B10
                  local.get $l6
                  i32.const 7
                  i32.ge_u
                  br_if $B10
                  i32.const 0
                  local.set $p3
                  local.get $l9
                  local.set $l6
                  br $B9
                end
                local.get $l9
                i32.const 60
                i32.add
                local.set $l6
                local.get $l11
                i32.const 1073741816
                i32.and
                local.set $l7
                i32.const 0
                local.set $p3
                loop $L11
                  local.get $l6
                  i32.load
                  local.get $l6
                  i32.const -8
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -16
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -24
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -32
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -40
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -48
                  i32.add
                  i32.load
                  local.get $l6
                  i32.const -56
                  i32.add
                  i32.load
                  local.get $p3
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  i32.add
                  local.set $p3
                  local.get $l6
                  i32.const 64
                  i32.add
                  local.set $l6
                  local.get $l7
                  i32.const -8
                  i32.add
                  local.tee $l7
                  br_if $L11
                end
                local.get $l6
                i32.const -60
                i32.add
                local.set $l6
              end
              block $B12
                local.get $l5
                i32.eqz
                br_if $B12
                local.get $l6
                i32.const 4
                i32.add
                local.set $l6
                loop $L13
                  local.get $l6
                  i32.load
                  local.get $p3
                  i32.add
                  local.set $p3
                  local.get $l6
                  i32.const 8
                  i32.add
                  local.set $l6
                  local.get $l5
                  i32.const -1
                  i32.add
                  local.tee $l5
                  br_if $L13
                end
              end
              local.get $l8
              i32.const 3
              i32.shl
              local.set $l5
              block $B14
                local.get $l10
                i32.load
                local.get $p1
                i32.load offset=8
                local.tee $l6
                i32.sub
                local.get $p3
                i32.ge_u
                br_if $B14
                local.get $p1
                local.get $l6
                local.get $p3
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                local.get $p1
                i32.load offset=8
                local.set $l6
              end
              local.get $l9
              local.get $l5
              i32.add
              local.set $l12
              local.get $l9
              local.set $l5
              loop $L15
                local.get $l5
                i32.load
                local.set $p2
                block $B16
                  local.get $l10
                  i32.load
                  local.get $l6
                  i32.sub
                  local.get $l5
                  i32.const 4
                  i32.add
                  i32.load
                  local.tee $l7
                  i32.ge_u
                  br_if $B16
                  local.get $p1
                  local.get $l6
                  local.get $l7
                  call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
                  local.get $p1
                  i32.load offset=8
                  local.set $l6
                end
                local.get $p1
                i32.load
                local.get $l6
                i32.add
                local.get $p2
                local.get $l7
                call $memcpy
                drop
                local.get $p1
                local.get $l6
                local.get $l7
                i32.add
                local.tee $l6
                i32.store offset=8
                local.get $l12
                local.get $l5
                i32.const 8
                i32.add
                local.tee $l5
                i32.ne
                br_if $L15
              end
              block $B17
                local.get $p3
                br_if $B17
                local.get $p0
                i32.const 1050536
                i64.extend_i32_u
                i64.const 32
                i64.shl
                i64.const 2
                i64.or
                i64.store align=4
                br $B0
              end
              local.get $l9
              i32.const 4
              i32.add
              local.set $l6
              i32.const 0
              local.set $l5
              i32.const 0
              local.set $l7
              block $B18
                loop $L19
                  local.get $l6
                  i32.load
                  local.get $l7
                  i32.add
                  local.tee $p2
                  local.get $p3
                  i32.gt_u
                  br_if $B18
                  local.get $l6
                  i32.const 8
                  i32.add
                  local.set $l6
                  local.get $p2
                  local.set $l7
                  local.get $l11
                  local.get $l5
                  i32.const 1
                  i32.add
                  local.tee $l5
                  i32.ne
                  br_if $L19
                end
                local.get $p2
                local.set $l7
                local.get $l11
                local.set $l5
              end
              block $B20
                local.get $l8
                local.get $l5
                i32.lt_u
                br_if $B20
                local.get $l8
                local.get $l5
                i32.sub
                local.tee $l8
                i32.eqz
                br_if $B1
                local.get $l9
                local.get $l5
                i32.const 3
                i32.shl
                local.tee $l5
                i32.add
                local.tee $p2
                i32.load offset=4
                local.tee $l12
                local.get $p3
                local.get $l7
                i32.sub
                local.tee $l6
                i32.lt_u
                br_if $B7
                local.get $p2
                i32.const 4
                i32.add
                local.get $l12
                local.get $l6
                i32.sub
                i32.store
                local.get $l9
                local.get $l5
                i32.add
                local.tee $l9
                local.get $l9
                i32.load
                local.get $l6
                i32.add
                i32.store
                br $L8
              end
            end
            local.get $l5
            local.get $l8
            i32.const 1050676
            call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
            unreachable
          end
          local.get $l4
          i32.const 28
          i32.add
          i32.const 0
          i32.store
          local.get $l4
          i32.const 1048792
          i32.store offset=24
          local.get $l4
          i64.const 1
          i64.store offset=12 align=4
          local.get $l4
          i32.const 1052112
          i32.store offset=8
          local.get $l4
          i32.const 8
          i32.add
          i32.const 1052152
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $l5
        local.get $p3
        i32.const 1050676
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get $p0
      i32.const 4
      i32.store8
    end
    local.get $l4
    i32.const 32
    i32.add
    global.set $g0)
  (func $_ZN3std2io5Write9write_fmt17hf06e34d13533586bE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 4
    i32.store8 offset=12
    local.get $l3
    local.get $p1
    i32.store offset=8
    local.get $l3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    local.get $p2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.get $p2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l3
    local.get $p2
    i64.load align=4
    i64.store offset=24
    block $B0
      block $B1
        local.get $l3
        i32.const 8
        i32.add
        i32.const 1050784
        local.get $l3
        i32.const 24
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        i32.eqz
        br_if $B1
        block $B2
          local.get $l3
          i32.load8_u offset=12
          i32.const 4
          i32.ne
          br_if $B2
          local.get $p0
          i32.const 1050724
          i64.extend_i32_u
          i64.const 32
          i64.shl
          i64.const 2
          i64.or
          i64.store align=4
          br $B0
        end
        local.get $p0
        local.get $l3
        i64.load offset=12 align=4
        i64.store align=4
        br $B0
      end
      local.get $p0
      i32.const 4
      i32.store8
      local.get $l3
      i32.load8_u offset=12
      i32.const 3
      i32.ne
      br_if $B0
      local.get $l3
      i32.const 16
      i32.add
      i32.load
      local.tee $p2
      i32.load
      local.get $p2
      i32.load offset=4
      i32.load
      call_indirect (type $t0) $T0
      block $B3
        local.get $p2
        i32.load offset=4
        local.tee $p1
        i32.load offset=4
        local.tee $p0
        i32.eqz
        br_if $B3
        local.get $p2
        i32.load
        local.get $p0
        local.get $p1
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get $l3
      i32.load offset=16
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    local.get $l3
    i32.const 48
    i32.add
    global.set $g0)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha099fde24b4a4dcaE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i64) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 8
    i32.add
    local.get $p0
    i32.load
    local.get $p1
    local.get $p2
    call $_ZN61_$LT$std..io..stdio..StdoutLock$u20$as$u20$std..io..Write$GT$9write_all17h37c1fbc98c9033e9E
    block $B0
      local.get $l3
      i32.load8_u offset=8
      local.tee $p1
      i32.const 4
      i32.eq
      br_if $B0
      local.get $l3
      i64.load offset=8
      local.set $l4
      block $B1
        local.get $p0
        i32.load8_u offset=4
        i32.const 3
        i32.ne
        br_if $B1
        local.get $p0
        i32.const 8
        i32.add
        i32.load
        local.tee $p2
        i32.load
        local.get $p2
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B2
          local.get $p2
          i32.load offset=4
          local.tee $l5
          i32.load offset=4
          local.tee $l6
          i32.eqz
          br_if $B2
          local.get $p2
          i32.load
          local.get $l6
          local.get $l5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $p2
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get $p0
      local.get $l4
      i64.store offset=4 align=4
    end
    local.get $l3
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1
    i32.const 4
    i32.ne)
  (func $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb12703cce11fe208E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32)
    block $B0
      local.get $p0
      i32.load
      local.tee $l3
      i32.const 4
      i32.add
      i32.load
      local.get $l3
      i32.const 8
      i32.add
      local.tee $l4
      i32.load
      local.tee $p0
      i32.sub
      local.get $p2
      i32.ge_u
      br_if $B0
      local.get $l3
      local.get $p0
      local.get $p2
      call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$7reserve21do_reserve_and_handle17h9330f4e100a101ccE
      local.get $l4
      i32.load
      local.set $p0
    end
    local.get $l3
    i32.load
    local.get $p0
    i32.add
    local.get $p1
    local.get $p2
    call $memcpy
    drop
    local.get $l4
    local.get $p0
    local.get $p2
    i32.add
    i32.store
    i32.const 0)
  (func $_ZN3std5panic19get_backtrace_style17h5dc7902a5c3a80f3E (type $t9) (result i32)
    (local $l0 i32) (local $l1 i32) (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l0
    global.set $g0
    i32.const 0
    local.set $l1
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              i32.const 0
              i32.load offset=1059256
              br_table $B1 $B0 $B3 $B2 $B4
            end
            i32.const 1049028
            i32.const 40
            i32.const 1050832
            call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
            unreachable
          end
          i32.const 1
          local.set $l1
          br $B0
        end
        i32.const 2
        local.set $l1
        br $B0
      end
      local.get $l0
      i32.const 1049437
      i32.const 14
      call $_ZN3std3env7_var_os17hac8a3e6bb094e6d3E
      block $B5
        block $B6
          local.get $l0
          i32.load
          local.tee $l1
          i32.eqz
          br_if $B6
          i32.const 0
          local.set $l2
          local.get $l0
          i32.load offset=4
          local.set $l3
          block $B7
            block $B8
              block $B9
                local.get $l0
                i32.const 8
                i32.add
                i32.load
                i32.const -1
                i32.add
                br_table $B9 $B7 $B7 $B8 $B7
              end
              i32.const -2
              i32.const 0
              local.get $l1
              i32.load8_u
              i32.const 48
              i32.eq
              select
              local.set $l2
              br $B7
            end
            local.get $l1
            i32.load align=1
            i32.const 1819047270
            i32.eq
            local.set $l2
          end
          block $B10
            local.get $l3
            i32.eqz
            br_if $B10
            local.get $l1
            local.get $l3
            i32.const 1
            call $__rust_dealloc
          end
          i32.const 1
          local.set $l3
          i32.const 0
          local.set $l1
          block $B11
            local.get $l2
            i32.const 3
            i32.and
            br_table $B5 $B11 $B6 $B5
          end
          i32.const 2
          local.set $l3
          i32.const 1
          local.set $l1
          br $B5
        end
        i32.const 3
        local.set $l3
        i32.const 2
        local.set $l1
      end
      i32.const 0
      local.get $l3
      i32.store offset=1059256
    end
    local.get $l0
    i32.const 16
    i32.add
    global.set $g0
    local.get $l1)
  (func $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17h3d2bbcbe4fd788bbE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.const 8
    i32.add
    i32.load
    local.get $p1
    call $_ZN66_$LT$std..sys..wasi..os_str..Slice$u20$as$u20$core..fmt..Debug$GT$3fmt17hb769d13a4a71030dE)
  (func $_ZN3std7process5abort17h4bdc2697b17f0111E (type $t7)
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $_ZN3std4sync4once4Once15call_once_force28_$u7b$$u7b$closure$u7d$$u7d$17h5da9e8acebb38efeE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    local.get $p0
    i32.load
    local.tee $l2
    i32.load
    local.set $p0
    local.get $l2
    i32.const 0
    i32.store
    block $B0
      block $B1
        local.get $p0
        i32.eqz
        br_if $B1
        i32.const 1024
        i32.const 1
        call $__rust_alloc
        local.tee $l2
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 0
        i32.store8 offset=28
        local.get $p0
        i32.const 0
        i32.store8 offset=24
        local.get $p0
        i64.const 1024
        i64.store offset=16 align=4
        local.get $p0
        local.get $l2
        i32.store offset=12
        local.get $p0
        i32.const 0
        i32.store offset=8
        local.get $p0
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
  (func $_ZN76_$LT$std..sync..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h45ee2ecae3ad0859E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 8
    i32.add
    local.get $p1
    i32.const 1051080
    i32.const 11
    call $_ZN4core3fmt9Formatter12debug_struct17h678028cd4a965ad7E
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt8builders11DebugStruct21finish_non_exhaustive17h9dcede22c3d51d75E
    local.set $p1
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hafba394e37912a5aE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load8_u
    local.set $l3
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN3std3env11current_dir17he9cc85f9a6781258E
    block $B0
      block $B1
        local.get $l2
        i32.load offset=8
        br_if $B1
        local.get $l2
        i32.const 16
        i32.add
        i32.load
        local.set $l4
        local.get $l2
        i32.load offset=12
        local.set $p0
        br $B0
      end
      i32.const 0
      local.set $p0
      block $B2
        local.get $l2
        i32.load8_u offset=12
        i32.const 3
        i32.ne
        br_if $B2
        local.get $l2
        i32.const 16
        i32.add
        i32.load
        local.tee $l4
        i32.load
        local.get $l4
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B3
          local.get $l4
          i32.load offset=4
          local.tee $l5
          i32.load offset=4
          local.tee $l6
          i32.eqz
          br_if $B3
          local.get $l4
          i32.load
          local.get $l6
          local.get $l5
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $l4
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
    end
    local.get $l2
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get $l2
    i32.const 1048792
    i32.store offset=24
    local.get $l2
    i64.const 1
    i64.store offset=12 align=4
    local.get $l2
    i32.const 1051108
    i32.store offset=8
    block $B4
      block $B5
        block $B6
          local.get $p1
          local.get $l2
          i32.const 8
          i32.add
          call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
          br_if $B6
          block $B7
            local.get $l3
            i32.const 255
            i32.and
            br_if $B7
            local.get $l2
            i32.const 28
            i32.add
            i32.const 0
            i32.store
            local.get $l2
            i32.const 1048792
            i32.store offset=24
            local.get $l2
            i64.const 1
            i64.store offset=12 align=4
            local.get $l2
            i32.const 1051204
            i32.store offset=8
            local.get $p1
            local.get $l2
            i32.const 8
            i32.add
            call $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE
            br_if $B6
          end
          i32.const 0
          local.set $p1
          local.get $p0
          i32.eqz
          br_if $B4
          local.get $l4
          i32.eqz
          br_if $B4
          br $B5
        end
        i32.const 1
        local.set $p1
        local.get $p0
        i32.eqz
        br_if $B4
        local.get $l4
        i32.eqz
        br_if $B4
      end
      local.get $p0
      local.get $l4
      i32.const 1
      call $__rust_dealloc
    end
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17ha133246273fcb7e9E (type $t0) (param $p0 i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.load offset=4
    local.get $p0
    i32.load offset=8
    call $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h20406b5ff3712528E
    unreachable)
  (func $_ZN3std9panicking19begin_panic_handler28_$u7b$$u7b$closure$u7d$$u7d$17h20406b5ff3712528E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $p0
    i32.const 20
    i32.add
    i32.load
    local.set $l4
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p0
            i32.const 4
            i32.add
            i32.load
            br_table $B3 $B2 $B0
          end
          local.get $l4
          br_if $B0
          i32.const 1048792
          local.set $p0
          i32.const 0
          local.set $l4
          br $B1
        end
        local.get $l4
        br_if $B0
        local.get $p0
        i32.load
        local.tee $p0
        i32.load offset=4
        local.set $l4
        local.get $p0
        i32.load
        local.set $p0
      end
      local.get $l3
      local.get $l4
      i32.store offset=4
      local.get $l3
      local.get $p0
      i32.store
      local.get $l3
      i32.const 1051836
      local.get $p1
      call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
      local.get $p2
      local.get $p1
      call $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE
      call $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E
      unreachable
    end
    local.get $l3
    i32.const 0
    i32.store offset=4
    local.get $l3
    local.get $p0
    i32.store
    local.get $l3
    i32.const 1051816
    local.get $p1
    call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
    local.get $p2
    local.get $p1
    call $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE
    call $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E
    unreachable)
  (func $_ZN3std5alloc24default_alloc_error_hook17h5a1a1d982f9bd818E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      i32.const 0
      i32.load8_u offset=1059216
      br_if $B0
      local.get $l2
      i32.const 7
      i32.store offset=4
      local.get $l2
      local.get $p0
      i32.store offset=12
      local.get $l2
      local.get $l2
      i32.const 12
      i32.add
      i32.store
      local.get $l2
      i32.const 4
      i32.store8 offset=20
      local.get $l2
      local.get $l2
      i32.const 56
      i32.add
      i32.store offset=16
      local.get $l2
      i32.const 52
      i32.add
      i32.const 1
      i32.store
      local.get $l2
      i64.const 2
      i64.store offset=36 align=4
      local.get $l2
      i32.const 1051400
      i32.store offset=32
      local.get $l2
      local.get $l2
      i32.store offset=48
      block $B1
        block $B2
          local.get $l2
          i32.const 16
          i32.add
          i32.const 1050736
          local.get $l2
          i32.const 32
          i32.add
          call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
          i32.eqz
          br_if $B2
          local.get $l2
          i32.load8_u offset=20
          i32.const 4
          i32.eq
          br_if $B1
          local.get $l2
          i32.load8_u offset=20
          i32.const 3
          i32.ne
          br_if $B1
          local.get $l2
          i32.const 24
          i32.add
          i32.load
          local.tee $p0
          i32.load
          local.get $p0
          i32.load offset=4
          i32.load
          call_indirect (type $t0) $T0
          block $B3
            local.get $p0
            i32.load offset=4
            local.tee $l3
            i32.load offset=4
            local.tee $l4
            i32.eqz
            br_if $B3
            local.get $p0
            i32.load
            local.get $l4
            local.get $l3
            i32.load offset=8
            call $__rust_dealloc
          end
          local.get $p0
          i32.const 12
          i32.const 4
          call $__rust_dealloc
          br $B1
        end
        local.get $l2
        i32.load8_u offset=20
        i32.const 3
        i32.ne
        br_if $B1
        local.get $l2
        i32.const 24
        i32.add
        i32.load
        local.tee $p0
        i32.load
        local.get $p0
        i32.load offset=4
        i32.load
        call_indirect (type $t0) $T0
        block $B4
          local.get $p0
          i32.load offset=4
          local.tee $l3
          i32.load offset=4
          local.tee $l4
          i32.eqz
          br_if $B4
          local.get $p0
          i32.load
          local.get $l4
          local.get $l3
          i32.load offset=8
          call $__rust_dealloc
        end
        local.get $l2
        i32.load offset=24
        i32.const 12
        i32.const 4
        call $__rust_dealloc
      end
      local.get $l2
      i32.const 64
      i32.add
      global.set $g0
      return
    end
    local.get $l2
    i32.const 52
    i32.add
    i32.const 1
    i32.store
    local.get $l2
    i64.const 2
    i64.store offset=36 align=4
    local.get $l2
    i32.const 1051400
    i32.store offset=32
    local.get $l2
    i32.const 7
    i32.store offset=20
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    local.get $l2
    i32.const 16
    i32.add
    i32.store offset=48
    local.get $l2
    local.get $l2
    i32.store offset=16
    local.get $l2
    i32.const 32
    i32.add
    i32.const 1051440
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $rust_oom (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    local.get $p0
    local.get $p1
    i32.const 0
    i32.load offset=1059264
    local.tee $l2
    i32.const 8
    local.get $l2
    select
    call_indirect (type $t2) $T0
    call $_ZN3std7process5abort17h4bdc2697b17f0111E
    unreachable)
  (func $__rdl_alloc (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    block $B0
      block $B1
        local.get $p1
        i32.const 8
        i32.gt_u
        br_if $B1
        local.get $p1
        local.get $p0
        i32.le_u
        br_if $B0
      end
      local.get $p1
      local.get $p0
      call $aligned_alloc
      return
    end
    local.get $p0
    call $malloc)
  (func $__rdl_dealloc (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    call $free)
  (func $__rdl_realloc (type $t6) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (result i32)
    block $B0
      block $B1
        local.get $p2
        i32.const 8
        i32.gt_u
        br_if $B1
        local.get $p2
        local.get $p3
        i32.le_u
        br_if $B0
      end
      block $B2
        local.get $p2
        local.get $p3
        call $aligned_alloc
        local.tee $p2
        br_if $B2
        i32.const 0
        return
      end
      local.get $p2
      local.get $p0
      local.get $p3
      local.get $p1
      local.get $p1
      local.get $p3
      i32.gt_u
      select
      call $memcpy
      local.set $p3
      local.get $p0
      call $free
      local.get $p3
      return
    end
    local.get $p0
    local.get $p3
    call $realloc)
  (func $_ZN3std9panicking12default_hook28_$u7b$$u7b$closure$u7d$$u7d$17h437bfcd332897e7eE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 20
    i32.add
    i32.const 3
    i32.store
    local.get $l3
    i32.const 32
    i32.add
    i32.const 20
    i32.add
    i32.const 9
    i32.store
    local.get $l3
    i32.const 44
    i32.add
    i32.const 5
    i32.store
    local.get $l3
    i64.const 4
    i64.store offset=4 align=4
    local.get $l3
    i32.const 1051632
    i32.store
    local.get $l3
    i32.const 5
    i32.store offset=36
    local.get $l3
    local.get $p0
    i32.load offset=8
    i32.store offset=48
    local.get $l3
    local.get $p0
    i32.load offset=4
    i32.store offset=40
    local.get $l3
    local.get $p0
    i32.load
    i32.store offset=32
    local.get $l3
    local.get $l3
    i32.const 32
    i32.add
    i32.store offset=16
    local.get $l3
    i32.const 24
    i32.add
    local.get $p1
    local.get $l3
    local.get $p2
    i32.load offset=36
    local.tee $l4
    call_indirect (type $t4) $T0
    block $B0
      local.get $l3
      i32.load8_u offset=24
      i32.const 3
      i32.ne
      br_if $B0
      local.get $l3
      i32.load offset=28
      local.tee $p2
      i32.load
      local.get $p2
      i32.load offset=4
      i32.load
      call_indirect (type $t0) $T0
      block $B1
        local.get $p2
        i32.load offset=4
        local.tee $l5
        i32.load offset=4
        local.tee $l6
        i32.eqz
        br_if $B1
        local.get $p2
        i32.load
        local.get $l6
        local.get $l5
        i32.load offset=8
        call $__rust_dealloc
      end
      local.get $p2
      i32.const 12
      i32.const 4
      call $__rust_dealloc
    end
    block $B2
      block $B3
        block $B4
          local.get $p0
          i32.load offset=12
          i32.load8_u
          local.tee $p0
          i32.const 3
          i32.eq
          br_if $B4
          block $B5
            block $B6
              block $B7
                local.get $p0
                br_table $B7 $B6 $B5 $B7
              end
              i32.const 0
              i32.load8_u offset=1059260
              local.set $p0
              i32.const 0
              i32.const 1
              i32.store8 offset=1059260
              local.get $l3
              local.get $p0
              i32.store8
              local.get $p0
              br_if $B3
              local.get $l3
              i32.const 52
              i32.add
              i32.const 1
              i32.store
              local.get $l3
              i64.const 1
              i64.store offset=36 align=4
              local.get $l3
              i32.const 1049524
              i32.store offset=32
              local.get $l3
              i32.const 10
              i32.store offset=4
              local.get $l3
              i32.const 0
              i32.store8 offset=63
              local.get $l3
              local.get $l3
              i32.store offset=48
              local.get $l3
              local.get $l3
              i32.const 63
              i32.add
              i32.store
              local.get $l3
              i32.const 24
              i32.add
              local.get $p1
              local.get $l3
              i32.const 32
              i32.add
              local.get $l4
              call_indirect (type $t4) $T0
              i32.const 0
              i32.const 0
              i32.store8 offset=1059260
              local.get $l3
              i32.load8_u offset=24
              i32.const 3
              i32.ne
              br_if $B4
              local.get $l3
              i32.load offset=28
              local.tee $p0
              i32.load
              local.get $p0
              i32.load offset=4
              i32.load
              call_indirect (type $t0) $T0
              block $B8
                local.get $p0
                i32.load offset=4
                local.tee $p1
                i32.load offset=4
                local.tee $p2
                i32.eqz
                br_if $B8
                local.get $p0
                i32.load
                local.get $p2
                local.get $p1
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get $p0
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              br $B4
            end
            i32.const 0
            i32.load8_u offset=1059260
            local.set $p0
            i32.const 0
            i32.const 1
            i32.store8 offset=1059260
            local.get $l3
            local.get $p0
            i32.store8
            local.get $p0
            br_if $B2
            local.get $l3
            i32.const 52
            i32.add
            i32.const 1
            i32.store
            local.get $l3
            i64.const 1
            i64.store offset=36 align=4
            local.get $l3
            i32.const 1049524
            i32.store offset=32
            local.get $l3
            i32.const 10
            i32.store offset=4
            local.get $l3
            i32.const 1
            i32.store8 offset=63
            local.get $l3
            local.get $l3
            i32.store offset=48
            local.get $l3
            local.get $l3
            i32.const 63
            i32.add
            i32.store
            local.get $l3
            i32.const 24
            i32.add
            local.get $p1
            local.get $l3
            i32.const 32
            i32.add
            local.get $l4
            call_indirect (type $t4) $T0
            i32.const 0
            i32.const 0
            i32.store8 offset=1059260
            local.get $l3
            i32.load8_u offset=24
            i32.const 3
            i32.ne
            br_if $B4
            local.get $l3
            i32.load offset=28
            local.tee $p0
            i32.load
            local.get $p0
            i32.load offset=4
            i32.load
            call_indirect (type $t0) $T0
            block $B9
              local.get $p0
              i32.load offset=4
              local.tee $p1
              i32.load offset=4
              local.tee $p2
              i32.eqz
              br_if $B9
              local.get $p0
              i32.load
              local.get $p2
              local.get $p1
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get $p0
            i32.const 12
            i32.const 4
            call $__rust_dealloc
            br $B4
          end
          i32.const 0
          i32.load8_u offset=1059208
          local.set $p0
          i32.const 0
          i32.const 0
          i32.store8 offset=1059208
          local.get $p0
          i32.eqz
          br_if $B4
          local.get $l3
          i32.const 52
          i32.add
          i32.const 0
          i32.store
          local.get $l3
          i32.const 1048792
          i32.store offset=48
          local.get $l3
          i64.const 1
          i64.store offset=36 align=4
          local.get $l3
          i32.const 1051744
          i32.store offset=32
          local.get $l3
          local.get $p1
          local.get $l3
          i32.const 32
          i32.add
          local.get $l4
          call_indirect (type $t4) $T0
          local.get $l3
          i32.load8_u
          i32.const 3
          i32.ne
          br_if $B4
          local.get $l3
          i32.load offset=4
          local.tee $p0
          i32.load
          local.get $p0
          i32.load offset=4
          i32.load
          call_indirect (type $t0) $T0
          block $B10
            local.get $p0
            i32.load offset=4
            local.tee $p1
            i32.load offset=4
            local.tee $p2
            i32.eqz
            br_if $B10
            local.get $p0
            i32.load
            local.get $p2
            local.get $p1
            i32.load offset=8
            call $__rust_dealloc
          end
          local.get $p0
          i32.const 12
          i32.const 4
          call $__rust_dealloc
        end
        local.get $l3
        i32.const 64
        i32.add
        global.set $g0
        return
      end
      local.get $l3
      i32.const 52
      i32.add
      i32.const 0
      i32.store
      local.get $l3
      i32.const 48
      i32.add
      i32.const 1048792
      i32.store
      local.get $l3
      i64.const 1
      i64.store offset=36 align=4
      local.get $l3
      i32.const 1052308
      i32.store offset=32
      local.get $l3
      local.get $l3
      i32.const 32
      i32.add
      call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
      unreachable
    end
    local.get $l3
    i32.const 52
    i32.add
    i32.const 0
    i32.store
    local.get $l3
    i32.const 48
    i32.add
    i32.const 1048792
    i32.store
    local.get $l3
    i64.const 1
    i64.store offset=36 align=4
    local.get $l3
    i32.const 1052308
    i32.store offset=32
    local.get $l3
    local.get $l3
    i32.const 32
    i32.add
    call $_ZN4core9panicking13assert_failed17h0dd60f0f5b291c72E
    unreachable)
  (func $rust_begin_unwind (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l1
    global.set $g0
    local.get $p0
    call $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE
    i32.const 1051752
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17h38e7befd79b1f826E
    local.set $l2
    local.get $p0
    call $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E
    call $_ZN4core6option15Option$LT$T$GT$6unwrap17h88efffa862cbe9d0E
    local.set $l3
    local.get $l1
    local.get $l2
    i32.store offset=8
    local.get $l1
    local.get $p0
    i32.store offset=4
    local.get $l1
    local.get $l3
    i32.store
    local.get $l1
    call $_ZN3std10sys_common9backtrace26__rust_end_short_backtrace17ha133246273fcb7e9E
    unreachable)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h34d0cd2380313f86E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i64)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p1
    i32.const 4
    i32.add
    local.set $l3
    block $B0
      local.get $p1
      i32.load offset=4
      br_if $B0
      local.get $p1
      i32.load
      local.set $l4
      local.get $l2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee $l5
      i32.const 0
      i32.store
      local.get $l2
      i64.const 1
      i64.store offset=8
      local.get $l2
      local.get $l2
      i32.const 8
      i32.add
      i32.store offset=20
      local.get $l2
      i32.const 24
      i32.add
      i32.const 16
      i32.add
      local.get $l4
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get $l2
      i32.const 24
      i32.add
      i32.const 8
      i32.add
      local.get $l4
      i32.const 8
      i32.add
      i64.load align=4
      i64.store
      local.get $l2
      local.get $l4
      i64.load align=4
      i64.store offset=24
      local.get $l2
      i32.const 20
      i32.add
      i32.const 1048768
      local.get $l2
      i32.const 24
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      drop
      local.get $l3
      i32.const 8
      i32.add
      local.get $l5
      i32.load
      i32.store
      local.get $l3
      local.get $l2
      i64.load offset=8
      i64.store align=4
    end
    local.get $l2
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    local.tee $l4
    local.get $l3
    i32.const 8
    i32.add
    i32.load
    i32.store
    local.get $p1
    i32.const 12
    i32.add
    i32.const 0
    i32.store
    local.get $l3
    i64.load align=4
    local.set $l6
    local.get $p1
    i64.const 1
    i64.store offset=4 align=4
    local.get $l2
    local.get $l6
    i64.store offset=24
    block $B1
      i32.const 12
      i32.const 4
      call $__rust_alloc
      local.tee $p1
      br_if $B1
      i32.const 12
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get $p1
    local.get $l2
    i64.load offset=24
    i64.store align=4
    local.get $p1
    i32.const 8
    i32.add
    local.get $l4
    i32.load
    i32.store
    local.get $p0
    i32.const 1051784
    i32.store offset=4
    local.get $p0
    local.get $p1
    i32.store
    local.get $l2
    i32.const 48
    i32.add
    global.set $g0)
  (func $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17ha161f6684539a1beE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p1
    i32.const 4
    i32.add
    local.set $l3
    block $B0
      local.get $p1
      i32.load offset=4
      br_if $B0
      local.get $p1
      i32.load
      local.set $p1
      local.get $l2
      i32.const 8
      i32.add
      i32.const 8
      i32.add
      local.tee $l4
      i32.const 0
      i32.store
      local.get $l2
      i64.const 1
      i64.store offset=8
      local.get $l2
      local.get $l2
      i32.const 8
      i32.add
      i32.store offset=20
      local.get $l2
      i32.const 24
      i32.add
      i32.const 16
      i32.add
      local.get $p1
      i32.const 16
      i32.add
      i64.load align=4
      i64.store
      local.get $l2
      i32.const 24
      i32.add
      i32.const 8
      i32.add
      local.get $p1
      i32.const 8
      i32.add
      i64.load align=4
      i64.store
      local.get $l2
      local.get $p1
      i64.load align=4
      i64.store offset=24
      local.get $l2
      i32.const 20
      i32.add
      i32.const 1048768
      local.get $l2
      i32.const 24
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      drop
      local.get $l3
      i32.const 8
      i32.add
      local.get $l4
      i32.load
      i32.store
      local.get $l3
      local.get $l2
      i64.load offset=8
      i64.store align=4
    end
    local.get $p0
    i32.const 1051784
    i32.store offset=4
    local.get $p0
    local.get $l3
    i32.store
    local.get $l2
    i32.const 48
    i32.add
    global.set $g0)
  (func $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h652349175b493901E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32)
    local.get $p1
    i32.load offset=4
    local.set $l2
    local.get $p1
    i32.load
    local.set $l3
    block $B0
      i32.const 8
      i32.const 4
      call $__rust_alloc
      local.tee $p1
      br_if $B0
      i32.const 8
      i32.const 4
      call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
      unreachable
    end
    local.get $p1
    local.get $l2
    i32.store offset=4
    local.get $p1
    local.get $l3
    i32.store
    local.get $p0
    i32.const 1051800
    i32.store offset=4
    local.get $p0
    local.get $p1
    i32.store)
  (func $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17hdfcd929547320219E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    i32.const 1051800
    i32.store offset=4
    local.get $p0
    local.get $p1
    i32.store)
  (func $_ZN3std9panicking20rust_panic_with_hook17h4823f6570b841b34E (type $t11) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32)
    (local $l5 i32) (local $l6 i32) (local $l7 i32)
    global.get $g0
    i32.const 112
    i32.sub
    local.tee $l5
    global.set $g0
    i32.const 0
    i32.const 0
    i32.load offset=1059280
    local.tee $l6
    i32.const 1
    i32.add
    i32.store offset=1059280
    i32.const 0
    i32.const 0
    i32.load offset=1059304
    i32.const 1
    i32.add
    local.tee $l7
    i32.store offset=1059304
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $l6
            i32.const 0
            i32.lt_s
            br_if $B3
            local.get $l7
            i32.const 2
            i32.gt_u
            br_if $B3
            local.get $l5
            local.get $p4
            i32.store8 offset=32
            local.get $l5
            local.get $p3
            i32.store offset=28
            local.get $l5
            local.get $p2
            i32.store offset=24
            i32.const 0
            i32.load offset=1059268
            local.tee $l6
            i32.const -1
            i32.le_s
            br_if $B1
            i32.const 0
            local.get $l6
            i32.const 1
            i32.add
            i32.store offset=1059268
            i32.const 0
            i32.load offset=1059276
            local.tee $l6
            i32.eqz
            br_if $B2
            i32.const 0
            i32.load offset=1059272
            local.set $p2
            local.get $l5
            i32.const 8
            i32.add
            local.get $p0
            local.get $p1
            i32.load offset=16
            call_indirect (type $t2) $T0
            local.get $l5
            local.get $l5
            i64.load offset=8
            i64.store offset=16
            local.get $p2
            local.get $l5
            i32.const 16
            i32.add
            local.get $l6
            i32.load offset=20
            call_indirect (type $t2) $T0
            br $B0
          end
          block $B4
            block $B5
              local.get $l7
              i32.const 2
              i32.gt_u
              br_if $B5
              local.get $l5
              local.get $p4
              i32.store8 offset=64
              local.get $l5
              local.get $p3
              i32.store offset=60
              local.get $l5
              local.get $p2
              i32.store offset=56
              local.get $l5
              i32.const 1048808
              i32.store offset=52
              local.get $l5
              i32.const 1048792
              i32.store offset=48
              local.get $l5
              i32.const 11
              i32.store offset=76
              local.get $l5
              local.get $l5
              i32.const 48
              i32.add
              i32.store offset=72
              local.get $l5
              i32.const 4
              i32.store8 offset=20
              local.get $l5
              local.get $l5
              i32.const 104
              i32.add
              i32.store offset=16
              local.get $l5
              i32.const 100
              i32.add
              i32.const 1
              i32.store
              local.get $l5
              i64.const 2
              i64.store offset=84 align=4
              local.get $l5
              i32.const 1051968
              i32.store offset=80
              local.get $l5
              local.get $l5
              i32.const 72
              i32.add
              i32.store offset=96
              block $B6
                local.get $l5
                i32.const 16
                i32.add
                i32.const 1050736
                local.get $l5
                i32.const 80
                i32.add
                call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
                i32.eqz
                br_if $B6
                local.get $l5
                i32.load8_u offset=20
                i32.const 4
                i32.eq
                br_if $B4
                local.get $l5
                i32.load8_u offset=20
                i32.const 3
                i32.ne
                br_if $B4
                local.get $l5
                i32.const 24
                i32.add
                i32.load
                local.tee $l5
                i32.load
                local.get $l5
                i32.load offset=4
                i32.load
                call_indirect (type $t0) $T0
                block $B7
                  local.get $l5
                  i32.load offset=4
                  local.tee $l7
                  i32.load offset=4
                  local.tee $l6
                  i32.eqz
                  br_if $B7
                  local.get $l5
                  i32.load
                  local.get $l6
                  local.get $l7
                  i32.load offset=8
                  call $__rust_dealloc
                end
                local.get $l5
                i32.const 12
                i32.const 4
                call $__rust_dealloc
                call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
                unreachable
              end
              local.get $l5
              i32.load8_u offset=20
              i32.const 3
              i32.ne
              br_if $B4
              local.get $l5
              i32.const 24
              i32.add
              i32.load
              local.tee $l7
              i32.load
              local.get $l7
              i32.load offset=4
              i32.load
              call_indirect (type $t0) $T0
              block $B8
                local.get $l7
                i32.load offset=4
                local.tee $l6
                i32.load offset=4
                local.tee $p4
                i32.eqz
                br_if $B8
                local.get $l7
                i32.load
                local.get $p4
                local.get $l6
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get $l5
              i32.load offset=24
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
              unreachable
            end
            local.get $l5
            i32.const 4
            i32.store8 offset=52
            local.get $l5
            local.get $l5
            i32.const 104
            i32.add
            i32.store offset=48
            local.get $l5
            i32.const 100
            i32.add
            i32.const 0
            i32.store
            local.get $l5
            i32.const 1048792
            i32.store offset=96
            local.get $l5
            i64.const 1
            i64.store offset=84 align=4
            local.get $l5
            i32.const 1051908
            i32.store offset=80
            block $B9
              local.get $l5
              i32.const 48
              i32.add
              i32.const 1050736
              local.get $l5
              i32.const 80
              i32.add
              call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
              i32.eqz
              br_if $B9
              local.get $l5
              i32.load8_u offset=52
              i32.const 4
              i32.eq
              br_if $B4
              local.get $l5
              i32.load8_u offset=52
              i32.const 3
              i32.ne
              br_if $B4
              local.get $l5
              i32.const 56
              i32.add
              i32.load
              local.tee $l5
              i32.load
              local.get $l5
              i32.load offset=4
              i32.load
              call_indirect (type $t0) $T0
              block $B10
                local.get $l5
                i32.load offset=4
                local.tee $l7
                i32.load offset=4
                local.tee $l6
                i32.eqz
                br_if $B10
                local.get $l5
                i32.load
                local.get $l6
                local.get $l7
                i32.load offset=8
                call $__rust_dealloc
              end
              local.get $l5
              i32.const 12
              i32.const 4
              call $__rust_dealloc
              call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
              unreachable
            end
            local.get $l5
            i32.load8_u offset=52
            i32.const 3
            i32.ne
            br_if $B4
            local.get $l5
            i32.const 56
            i32.add
            i32.load
            local.tee $l7
            i32.load
            local.get $l7
            i32.load offset=4
            i32.load
            call_indirect (type $t0) $T0
            block $B11
              local.get $l7
              i32.load offset=4
              local.tee $l6
              i32.load offset=4
              local.tee $p4
              i32.eqz
              br_if $B11
              local.get $l7
              i32.load
              local.get $p4
              local.get $l6
              i32.load offset=8
              call $__rust_dealloc
            end
            local.get $l5
            i32.load offset=56
            i32.const 12
            i32.const 4
            call $__rust_dealloc
          end
          call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
          unreachable
        end
        local.get $l5
        local.get $p0
        local.get $p1
        i32.load offset=16
        call_indirect (type $t2) $T0
        local.get $l5
        local.get $l5
        i64.load
        i64.store offset=16
        local.get $l5
        i32.const 16
        i32.add
        call $_ZN3std9panicking12default_hook17h9e7a62be9c47f791E
        br $B0
      end
      local.get $l5
      i32.const 48
      i32.add
      i32.const 20
      i32.add
      i32.const 1
      i32.store
      local.get $l5
      i32.const 80
      i32.add
      i32.const 20
      i32.add
      i32.const 0
      i32.store
      local.get $l5
      i64.const 2
      i64.store offset=52 align=4
      local.get $l5
      i32.const 1049184
      i32.store offset=48
      local.get $l5
      i32.const 12
      i32.store offset=76
      local.get $l5
      i32.const 1048792
      i32.store offset=96
      local.get $l5
      i64.const 1
      i64.store offset=84 align=4
      local.get $l5
      i32.const 1052416
      i32.store offset=80
      local.get $l5
      local.get $l5
      i32.const 72
      i32.add
      i32.store offset=64
      local.get $l5
      local.get $l5
      i32.const 80
      i32.add
      i32.store offset=72
      local.get $l5
      i32.const 40
      i32.add
      local.get $l5
      i32.const 104
      i32.add
      local.get $l5
      i32.const 48
      i32.add
      call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
      local.get $l5
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
    block $B12
      local.get $l7
      i32.const 1
      i32.gt_u
      br_if $B12
      local.get $p4
      i32.eqz
      br_if $B12
      local.get $p0
      local.get $p1
      call $rust_panic
      unreachable
    end
    local.get $l5
    i32.const 100
    i32.add
    i32.const 0
    i32.store
    local.get $l5
    i32.const 1048792
    i32.store offset=96
    local.get $l5
    i64.const 1
    i64.store offset=84 align=4
    local.get $l5
    i32.const 1052028
    i32.store offset=80
    local.get $l5
    i32.const 48
    i32.add
    local.get $l5
    i32.const 104
    i32.add
    local.get $l5
    i32.const 80
    i32.add
    call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
    local.get $l5
    i32.const 48
    i32.add
    call $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $rust_panic (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 96
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p1
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    local.get $l2
    call $__rust_start_panic
    i32.store offset=12
    local.get $l2
    i32.const 24
    i32.add
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get $l2
    i32.const 56
    i32.add
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get $l2
    i64.const 2
    i64.store offset=28 align=4
    local.get $l2
    i32.const 1049184
    i32.store offset=24
    local.get $l2
    i32.const 12
    i32.store offset=52
    local.get $l2
    i64.const 1
    i64.store offset=60 align=4
    local.get $l2
    i32.const 1052068
    i32.store offset=56
    local.get $l2
    i32.const 7
    i32.store offset=84
    local.get $l2
    local.get $l2
    i32.const 48
    i32.add
    i32.store offset=40
    local.get $l2
    local.get $l2
    i32.const 56
    i32.add
    i32.store offset=48
    local.get $l2
    local.get $l2
    i32.const 80
    i32.add
    i32.store offset=72
    local.get $l2
    local.get $l2
    i32.const 12
    i32.add
    i32.store offset=80
    local.get $l2
    i32.const 16
    i32.add
    local.get $l2
    i32.const 88
    i32.add
    local.get $l2
    i32.const 24
    i32.add
    call $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E
    local.get $l2
    i32.const 16
    i32.add
    call $_ZN4core3ptr81drop_in_place$LT$core..result..Result$LT$$LP$$RP$$C$std..io..error..Error$GT$$GT$17ha8d3180e7b53602fE
    call $_ZN3std3sys4wasi14abort_internal17h9e26f4a1d0971cefE
    unreachable)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17h7fa424a882c725e9E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l4
    global.set $g0
    local.get $l4
    local.get $p3
    i32.store offset=12
    local.get $l4
    local.get $p2
    i32.store offset=8
    i32.const 1
    local.set $p2
    local.get $l4
    i32.const 16
    i32.add
    i32.const 2
    local.get $l4
    i32.const 8
    i32.add
    i32.const 1
    call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
    block $B0
      block $B1
        local.get $l4
        i32.load16_u offset=16
        br_if $B1
        local.get $p0
        local.get $l4
        i32.load offset=20
        i32.store offset=4
        i32.const 0
        local.set $p2
        br $B0
      end
      local.get $l4
      local.get $l4
      i32.load16_u offset=18
      i32.store16 offset=30
      local.get $p0
      local.get $l4
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
    local.get $p0
    local.get $p2
    i32.store
    local.get $l4
    i32.const 32
    i32.add
    global.set $g0)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17he7f7854621726461E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l4
    global.set $g0
    local.get $l4
    i32.const 2
    local.get $p2
    local.get $p3
    call $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE
    block $B0
      block $B1
        local.get $l4
        i32.load16_u
        br_if $B1
        local.get $p0
        local.get $l4
        i32.load offset=4
        i32.store offset=4
        i32.const 0
        local.set $p2
        br $B0
      end
      local.get $l4
      local.get $l4
      i32.load16_u offset=2
      i32.store16 offset=14
      local.get $p0
      local.get $l4
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
      local.set $p2
    end
    local.get $p0
    local.get $p2
    i32.store
    local.get $l4
    i32.const 16
    i32.add
    global.set $g0)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h1cc0993ef9fea8dfE (type $t10) (param $p0 i32) (result i32)
    i32.const 1)
  (func $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5flush17h5d2f24e96e822d71E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    i32.const 4
    i32.store8)
  (func $__rust_start_panic (type $t10) (param $p0 i32) (result i32)
    unreachable
    unreachable)
  (func $_ZN4wasi13lib_generated5Errno3raw17h4bc51f723e804d10E (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load16_u)
  (func $_ZN4wasi13lib_generated8args_get17hc905ebaa953b8d98E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    local.get $p2
    call $_ZN4wasi13lib_generated22wasi_snapshot_preview18args_get17hfe12d9463b2a7790E
    local.tee $p1
    i32.store16 offset=2
    local.get $p0
    local.get $p1
    i32.const 0
    i32.ne
    i32.store16)
  (func $_ZN4wasi13lib_generated14args_sizes_get17h4999a6465b98090cE (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l1
    global.set $g0
    block $B0
      block $B1
        local.get $l1
        i32.const 8
        i32.add
        local.get $l1
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview114args_sizes_get17h19746ec286063fe2E
        local.tee $l2
        br_if $B1
        local.get $p0
        i32.const 8
        i32.add
        local.get $l1
        i32.load offset=12
        i32.store
        local.get $p0
        i32.const 4
        i32.add
        local.get $l1
        i32.load offset=8
        i32.store
        i32.const 0
        local.set $l2
        br $B0
      end
      local.get $p0
      local.get $l2
      i32.store16 offset=2
      i32.const 1
      local.set $l2
    end
    local.get $p0
    local.get $l2
    i32.store16
    local.get $l1
    i32.const 16
    i32.add
    global.set $g0)
  (func $_ZN4wasi13lib_generated8fd_write17h7c1126c1e098a61fE (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l4
    global.set $g0
    block $B0
      block $B1
        local.get $p1
        local.get $p2
        local.get $p3
        local.get $l4
        i32.const 12
        i32.add
        call $_ZN4wasi13lib_generated22wasi_snapshot_preview18fd_write17h2fb5d28ef433e1b2E
        local.tee $p1
        br_if $B1
        local.get $p0
        i32.const 4
        i32.add
        local.get $l4
        i32.load offset=12
        i32.store
        i32.const 0
        local.set $p1
        br $B0
      end
      local.get $p0
      local.get $p1
      i32.store16 offset=2
      i32.const 1
      local.set $p1
    end
    local.get $p0
    local.get $p1
    i32.store16
    local.get $l4
    i32.const 16
    i32.add
    global.set $g0)
  (func $malloc (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    call $dlmalloc)
  (func $dlmalloc (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l1
    global.set $g0
    block $B0
      i32.const 0
      i32.load offset=1059332
      br_if $B0
      i32.const 0
      call $sbrk
      i32.const 1059856
      i32.sub
      local.tee $l2
      i32.const 89
      i32.lt_u
      br_if $B0
      i32.const 0
      local.set $l3
      block $B1
        i32.const 0
        i32.load offset=1059780
        local.tee $l4
        br_if $B1
        i32.const 0
        i64.const -1
        i64.store offset=1059792 align=4
        i32.const 0
        i64.const 281474976776192
        i64.store offset=1059784 align=4
        i32.const 0
        local.get $l1
        i32.const 8
        i32.add
        i32.const -16
        i32.and
        i32.const 1431655768
        i32.xor
        local.tee $l4
        i32.store offset=1059780
        i32.const 0
        i32.const 0
        i32.store offset=1059800
        i32.const 0
        i32.const 0
        i32.store offset=1059752
      end
      i32.const 0
      local.get $l2
      i32.store offset=1059760
      i32.const 0
      i32.const 1059856
      i32.store offset=1059756
      i32.const 0
      i32.const 1059856
      i32.store offset=1059324
      i32.const 0
      local.get $l4
      i32.store offset=1059344
      i32.const 0
      i32.const -1
      i32.store offset=1059340
      loop $L2
        local.get $l3
        i32.const 1059368
        i32.add
        local.get $l3
        i32.const 1059356
        i32.add
        local.tee $l4
        i32.store
        local.get $l4
        local.get $l3
        i32.const 1059348
        i32.add
        local.tee $l5
        i32.store
        local.get $l3
        i32.const 1059360
        i32.add
        local.get $l5
        i32.store
        local.get $l3
        i32.const 1059376
        i32.add
        local.get $l3
        i32.const 1059364
        i32.add
        local.tee $l5
        i32.store
        local.get $l5
        local.get $l4
        i32.store
        local.get $l3
        i32.const 1059384
        i32.add
        local.get $l3
        i32.const 1059372
        i32.add
        local.tee $l4
        i32.store
        local.get $l4
        local.get $l5
        i32.store
        local.get $l3
        i32.const 1059380
        i32.add
        local.get $l4
        i32.store
        local.get $l3
        i32.const 32
        i32.add
        local.tee $l3
        i32.const 256
        i32.ne
        br_if $L2
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
      local.tee $l3
      i32.add
      local.tee $l4
      i32.const 4
      i32.add
      local.get $l2
      i32.const -56
      i32.add
      local.tee $l5
      local.get $l3
      i32.sub
      local.tee $l3
      i32.const 1
      i32.or
      i32.store
      i32.const 0
      i32.const 0
      i32.load offset=1059796
      i32.store offset=1059336
      i32.const 0
      local.get $l3
      i32.store offset=1059320
      i32.const 0
      local.get $l4
      i32.store offset=1059332
      i32.const 1059856
      local.get $l5
      i32.add
      i32.const 56
      i32.store offset=4
    end
    block $B3
      block $B4
        block $B5
          block $B6
            block $B7
              block $B8
                block $B9
                  block $B10
                    block $B11
                      block $B12
                        block $B13
                          block $B14
                            local.get $p0
                            i32.const 236
                            i32.gt_u
                            br_if $B14
                            block $B15
                              i32.const 0
                              i32.load offset=1059308
                              local.tee $l6
                              i32.const 16
                              local.get $p0
                              i32.const 19
                              i32.add
                              i32.const -16
                              i32.and
                              local.get $p0
                              i32.const 11
                              i32.lt_u
                              select
                              local.tee $l2
                              i32.const 3
                              i32.shr_u
                              local.tee $l4
                              i32.shr_u
                              local.tee $l3
                              i32.const 3
                              i32.and
                              i32.eqz
                              br_if $B15
                              local.get $l3
                              i32.const 1
                              i32.and
                              local.get $l4
                              i32.or
                              i32.const 1
                              i32.xor
                              local.tee $l5
                              i32.const 3
                              i32.shl
                              local.tee $p0
                              i32.const 1059356
                              i32.add
                              i32.load
                              local.tee $l4
                              i32.const 8
                              i32.add
                              local.set $l3
                              block $B16
                                block $B17
                                  local.get $l4
                                  i32.load offset=8
                                  local.tee $l2
                                  local.get $p0
                                  i32.const 1059348
                                  i32.add
                                  local.tee $p0
                                  i32.ne
                                  br_if $B17
                                  i32.const 0
                                  local.get $l6
                                  i32.const -2
                                  local.get $l5
                                  i32.rotl
                                  i32.and
                                  i32.store offset=1059308
                                  br $B16
                                end
                                local.get $p0
                                local.get $l2
                                i32.store offset=8
                                local.get $l2
                                local.get $p0
                                i32.store offset=12
                              end
                              local.get $l4
                              local.get $l5
                              i32.const 3
                              i32.shl
                              local.tee $l5
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get $l4
                              local.get $l5
                              i32.add
                              local.tee $l4
                              local.get $l4
                              i32.load offset=4
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              br $B3
                            end
                            local.get $l2
                            i32.const 0
                            i32.load offset=1059316
                            local.tee $l7
                            i32.le_u
                            br_if $B13
                            block $B18
                              local.get $l3
                              i32.eqz
                              br_if $B18
                              block $B19
                                block $B20
                                  local.get $l3
                                  local.get $l4
                                  i32.shl
                                  i32.const 2
                                  local.get $l4
                                  i32.shl
                                  local.tee $l3
                                  i32.const 0
                                  local.get $l3
                                  i32.sub
                                  i32.or
                                  i32.and
                                  local.tee $l3
                                  i32.const 0
                                  local.get $l3
                                  i32.sub
                                  i32.and
                                  i32.const -1
                                  i32.add
                                  local.tee $l3
                                  local.get $l3
                                  i32.const 12
                                  i32.shr_u
                                  i32.const 16
                                  i32.and
                                  local.tee $l3
                                  i32.shr_u
                                  local.tee $l4
                                  i32.const 5
                                  i32.shr_u
                                  i32.const 8
                                  i32.and
                                  local.tee $l5
                                  local.get $l3
                                  i32.or
                                  local.get $l4
                                  local.get $l5
                                  i32.shr_u
                                  local.tee $l3
                                  i32.const 2
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  local.tee $l4
                                  i32.or
                                  local.get $l3
                                  local.get $l4
                                  i32.shr_u
                                  local.tee $l3
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 2
                                  i32.and
                                  local.tee $l4
                                  i32.or
                                  local.get $l3
                                  local.get $l4
                                  i32.shr_u
                                  local.tee $l3
                                  i32.const 1
                                  i32.shr_u
                                  i32.const 1
                                  i32.and
                                  local.tee $l4
                                  i32.or
                                  local.get $l3
                                  local.get $l4
                                  i32.shr_u
                                  i32.add
                                  local.tee $l5
                                  i32.const 3
                                  i32.shl
                                  local.tee $p0
                                  i32.const 1059356
                                  i32.add
                                  i32.load
                                  local.tee $l4
                                  i32.load offset=8
                                  local.tee $l3
                                  local.get $p0
                                  i32.const 1059348
                                  i32.add
                                  local.tee $p0
                                  i32.ne
                                  br_if $B20
                                  i32.const 0
                                  local.get $l6
                                  i32.const -2
                                  local.get $l5
                                  i32.rotl
                                  i32.and
                                  local.tee $l6
                                  i32.store offset=1059308
                                  br $B19
                                end
                                local.get $p0
                                local.get $l3
                                i32.store offset=8
                                local.get $l3
                                local.get $p0
                                i32.store offset=12
                              end
                              local.get $l4
                              i32.const 8
                              i32.add
                              local.set $l3
                              local.get $l4
                              local.get $l2
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              local.get $l4
                              local.get $l5
                              i32.const 3
                              i32.shl
                              local.tee $l5
                              i32.add
                              local.get $l5
                              local.get $l2
                              i32.sub
                              local.tee $l5
                              i32.store
                              local.get $l4
                              local.get $l2
                              i32.add
                              local.tee $p0
                              local.get $l5
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              block $B21
                                local.get $l7
                                i32.eqz
                                br_if $B21
                                local.get $l7
                                i32.const 3
                                i32.shr_u
                                local.tee $l8
                                i32.const 3
                                i32.shl
                                i32.const 1059348
                                i32.add
                                local.set $l2
                                i32.const 0
                                i32.load offset=1059328
                                local.set $l4
                                block $B22
                                  block $B23
                                    local.get $l6
                                    i32.const 1
                                    local.get $l8
                                    i32.shl
                                    local.tee $l8
                                    i32.and
                                    br_if $B23
                                    i32.const 0
                                    local.get $l6
                                    local.get $l8
                                    i32.or
                                    i32.store offset=1059308
                                    local.get $l2
                                    local.set $l8
                                    br $B22
                                  end
                                  local.get $l2
                                  i32.load offset=8
                                  local.set $l8
                                end
                                local.get $l8
                                local.get $l4
                                i32.store offset=12
                                local.get $l2
                                local.get $l4
                                i32.store offset=8
                                local.get $l4
                                local.get $l2
                                i32.store offset=12
                                local.get $l4
                                local.get $l8
                                i32.store offset=8
                              end
                              i32.const 0
                              local.get $p0
                              i32.store offset=1059328
                              i32.const 0
                              local.get $l5
                              i32.store offset=1059316
                              br $B3
                            end
                            i32.const 0
                            i32.load offset=1059312
                            local.tee $l9
                            i32.eqz
                            br_if $B13
                            local.get $l9
                            i32.const 0
                            local.get $l9
                            i32.sub
                            i32.and
                            i32.const -1
                            i32.add
                            local.tee $l3
                            local.get $l3
                            i32.const 12
                            i32.shr_u
                            i32.const 16
                            i32.and
                            local.tee $l3
                            i32.shr_u
                            local.tee $l4
                            i32.const 5
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee $l5
                            local.get $l3
                            i32.or
                            local.get $l4
                            local.get $l5
                            i32.shr_u
                            local.tee $l3
                            i32.const 2
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.tee $l4
                            i32.or
                            local.get $l3
                            local.get $l4
                            i32.shr_u
                            local.tee $l3
                            i32.const 1
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee $l4
                            i32.or
                            local.get $l3
                            local.get $l4
                            i32.shr_u
                            local.tee $l3
                            i32.const 1
                            i32.shr_u
                            i32.const 1
                            i32.and
                            local.tee $l4
                            i32.or
                            local.get $l3
                            local.get $l4
                            i32.shr_u
                            i32.add
                            i32.const 2
                            i32.shl
                            i32.const 1059612
                            i32.add
                            i32.load
                            local.tee $p0
                            i32.load offset=4
                            i32.const -8
                            i32.and
                            local.get $l2
                            i32.sub
                            local.set $l4
                            local.get $p0
                            local.set $l5
                            block $B24
                              loop $L25
                                block $B26
                                  local.get $l5
                                  i32.load offset=16
                                  local.tee $l3
                                  br_if $B26
                                  local.get $l5
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee $l3
                                  i32.eqz
                                  br_if $B24
                                end
                                local.get $l3
                                i32.load offset=4
                                i32.const -8
                                i32.and
                                local.get $l2
                                i32.sub
                                local.tee $l5
                                local.get $l4
                                local.get $l5
                                local.get $l4
                                i32.lt_u
                                local.tee $l5
                                select
                                local.set $l4
                                local.get $l3
                                local.get $p0
                                local.get $l5
                                select
                                local.set $p0
                                local.get $l3
                                local.set $l5
                                br $L25
                              end
                            end
                            local.get $p0
                            i32.load offset=24
                            local.set $l10
                            block $B27
                              local.get $p0
                              i32.load offset=12
                              local.tee $l8
                              local.get $p0
                              i32.eq
                              br_if $B27
                              i32.const 0
                              i32.load offset=1059324
                              local.get $p0
                              i32.load offset=8
                              local.tee $l3
                              i32.gt_u
                              drop
                              local.get $l8
                              local.get $l3
                              i32.store offset=8
                              local.get $l3
                              local.get $l8
                              i32.store offset=12
                              br $B4
                            end
                            block $B28
                              local.get $p0
                              i32.const 20
                              i32.add
                              local.tee $l5
                              i32.load
                              local.tee $l3
                              br_if $B28
                              local.get $p0
                              i32.load offset=16
                              local.tee $l3
                              i32.eqz
                              br_if $B12
                              local.get $p0
                              i32.const 16
                              i32.add
                              local.set $l5
                            end
                            loop $L29
                              local.get $l5
                              local.set $l11
                              local.get $l3
                              local.tee $l8
                              i32.const 20
                              i32.add
                              local.tee $l5
                              i32.load
                              local.tee $l3
                              br_if $L29
                              local.get $l8
                              i32.const 16
                              i32.add
                              local.set $l5
                              local.get $l8
                              i32.load offset=16
                              local.tee $l3
                              br_if $L29
                            end
                            local.get $l11
                            i32.const 0
                            i32.store
                            br $B4
                          end
                          i32.const -1
                          local.set $l2
                          local.get $p0
                          i32.const -65
                          i32.gt_u
                          br_if $B13
                          local.get $p0
                          i32.const 19
                          i32.add
                          local.tee $l3
                          i32.const -16
                          i32.and
                          local.set $l2
                          i32.const 0
                          i32.load offset=1059312
                          local.tee $l7
                          i32.eqz
                          br_if $B13
                          i32.const 0
                          local.set $l11
                          block $B30
                            local.get $l2
                            i32.const 256
                            i32.lt_u
                            br_if $B30
                            i32.const 31
                            local.set $l11
                            local.get $l2
                            i32.const 16777215
                            i32.gt_u
                            br_if $B30
                            local.get $l3
                            i32.const 8
                            i32.shr_u
                            local.tee $l3
                            local.get $l3
                            i32.const 1048320
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 8
                            i32.and
                            local.tee $l3
                            i32.shl
                            local.tee $l4
                            local.get $l4
                            i32.const 520192
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 4
                            i32.and
                            local.tee $l4
                            i32.shl
                            local.tee $l5
                            local.get $l5
                            i32.const 245760
                            i32.add
                            i32.const 16
                            i32.shr_u
                            i32.const 2
                            i32.and
                            local.tee $l5
                            i32.shl
                            i32.const 15
                            i32.shr_u
                            local.get $l3
                            local.get $l4
                            i32.or
                            local.get $l5
                            i32.or
                            i32.sub
                            local.tee $l3
                            i32.const 1
                            i32.shl
                            local.get $l2
                            local.get $l3
                            i32.const 21
                            i32.add
                            i32.shr_u
                            i32.const 1
                            i32.and
                            i32.or
                            i32.const 28
                            i32.add
                            local.set $l11
                          end
                          i32.const 0
                          local.get $l2
                          i32.sub
                          local.set $l4
                          block $B31
                            block $B32
                              block $B33
                                block $B34
                                  local.get $l11
                                  i32.const 2
                                  i32.shl
                                  i32.const 1059612
                                  i32.add
                                  i32.load
                                  local.tee $l5
                                  br_if $B34
                                  i32.const 0
                                  local.set $l3
                                  i32.const 0
                                  local.set $l8
                                  br $B33
                                end
                                i32.const 0
                                local.set $l3
                                local.get $l2
                                i32.const 0
                                i32.const 25
                                local.get $l11
                                i32.const 1
                                i32.shr_u
                                i32.sub
                                local.get $l11
                                i32.const 31
                                i32.eq
                                select
                                i32.shl
                                local.set $p0
                                i32.const 0
                                local.set $l8
                                loop $L35
                                  block $B36
                                    local.get $l5
                                    i32.load offset=4
                                    i32.const -8
                                    i32.and
                                    local.get $l2
                                    i32.sub
                                    local.tee $l6
                                    local.get $l4
                                    i32.ge_u
                                    br_if $B36
                                    local.get $l6
                                    local.set $l4
                                    local.get $l5
                                    local.set $l8
                                    local.get $l6
                                    br_if $B36
                                    i32.const 0
                                    local.set $l4
                                    local.get $l5
                                    local.set $l8
                                    local.get $l5
                                    local.set $l3
                                    br $B32
                                  end
                                  local.get $l3
                                  local.get $l5
                                  i32.const 20
                                  i32.add
                                  i32.load
                                  local.tee $l6
                                  local.get $l6
                                  local.get $l5
                                  local.get $p0
                                  i32.const 29
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  i32.add
                                  i32.const 16
                                  i32.add
                                  i32.load
                                  local.tee $l5
                                  i32.eq
                                  select
                                  local.get $l3
                                  local.get $l6
                                  select
                                  local.set $l3
                                  local.get $p0
                                  i32.const 1
                                  i32.shl
                                  local.set $p0
                                  local.get $l5
                                  br_if $L35
                                end
                              end
                              block $B37
                                local.get $l3
                                local.get $l8
                                i32.or
                                br_if $B37
                                i32.const 0
                                local.set $l8
                                i32.const 2
                                local.get $l11
                                i32.shl
                                local.tee $l3
                                i32.const 0
                                local.get $l3
                                i32.sub
                                i32.or
                                local.get $l7
                                i32.and
                                local.tee $l3
                                i32.eqz
                                br_if $B13
                                local.get $l3
                                i32.const 0
                                local.get $l3
                                i32.sub
                                i32.and
                                i32.const -1
                                i32.add
                                local.tee $l3
                                local.get $l3
                                i32.const 12
                                i32.shr_u
                                i32.const 16
                                i32.and
                                local.tee $l3
                                i32.shr_u
                                local.tee $l5
                                i32.const 5
                                i32.shr_u
                                i32.const 8
                                i32.and
                                local.tee $p0
                                local.get $l3
                                i32.or
                                local.get $l5
                                local.get $p0
                                i32.shr_u
                                local.tee $l3
                                i32.const 2
                                i32.shr_u
                                i32.const 4
                                i32.and
                                local.tee $l5
                                i32.or
                                local.get $l3
                                local.get $l5
                                i32.shr_u
                                local.tee $l3
                                i32.const 1
                                i32.shr_u
                                i32.const 2
                                i32.and
                                local.tee $l5
                                i32.or
                                local.get $l3
                                local.get $l5
                                i32.shr_u
                                local.tee $l3
                                i32.const 1
                                i32.shr_u
                                i32.const 1
                                i32.and
                                local.tee $l5
                                i32.or
                                local.get $l3
                                local.get $l5
                                i32.shr_u
                                i32.add
                                i32.const 2
                                i32.shl
                                i32.const 1059612
                                i32.add
                                i32.load
                                local.set $l3
                              end
                              local.get $l3
                              i32.eqz
                              br_if $B31
                            end
                            loop $L38
                              local.get $l3
                              i32.load offset=4
                              i32.const -8
                              i32.and
                              local.get $l2
                              i32.sub
                              local.tee $l6
                              local.get $l4
                              i32.lt_u
                              local.set $p0
                              block $B39
                                local.get $l3
                                i32.load offset=16
                                local.tee $l5
                                br_if $B39
                                local.get $l3
                                i32.const 20
                                i32.add
                                i32.load
                                local.set $l5
                              end
                              local.get $l6
                              local.get $l4
                              local.get $p0
                              select
                              local.set $l4
                              local.get $l3
                              local.get $l8
                              local.get $p0
                              select
                              local.set $l8
                              local.get $l5
                              local.set $l3
                              local.get $l5
                              br_if $L38
                            end
                          end
                          local.get $l8
                          i32.eqz
                          br_if $B13
                          local.get $l4
                          i32.const 0
                          i32.load offset=1059316
                          local.get $l2
                          i32.sub
                          i32.ge_u
                          br_if $B13
                          local.get $l8
                          i32.load offset=24
                          local.set $l11
                          block $B40
                            local.get $l8
                            i32.load offset=12
                            local.tee $p0
                            local.get $l8
                            i32.eq
                            br_if $B40
                            i32.const 0
                            i32.load offset=1059324
                            local.get $l8
                            i32.load offset=8
                            local.tee $l3
                            i32.gt_u
                            drop
                            local.get $p0
                            local.get $l3
                            i32.store offset=8
                            local.get $l3
                            local.get $p0
                            i32.store offset=12
                            br $B5
                          end
                          block $B41
                            local.get $l8
                            i32.const 20
                            i32.add
                            local.tee $l5
                            i32.load
                            local.tee $l3
                            br_if $B41
                            local.get $l8
                            i32.load offset=16
                            local.tee $l3
                            i32.eqz
                            br_if $B11
                            local.get $l8
                            i32.const 16
                            i32.add
                            local.set $l5
                          end
                          loop $L42
                            local.get $l5
                            local.set $l6
                            local.get $l3
                            local.tee $p0
                            i32.const 20
                            i32.add
                            local.tee $l5
                            i32.load
                            local.tee $l3
                            br_if $L42
                            local.get $p0
                            i32.const 16
                            i32.add
                            local.set $l5
                            local.get $p0
                            i32.load offset=16
                            local.tee $l3
                            br_if $L42
                          end
                          local.get $l6
                          i32.const 0
                          i32.store
                          br $B5
                        end
                        block $B43
                          i32.const 0
                          i32.load offset=1059316
                          local.tee $l3
                          local.get $l2
                          i32.lt_u
                          br_if $B43
                          i32.const 0
                          i32.load offset=1059328
                          local.set $l4
                          block $B44
                            block $B45
                              local.get $l3
                              local.get $l2
                              i32.sub
                              local.tee $l5
                              i32.const 16
                              i32.lt_u
                              br_if $B45
                              local.get $l4
                              local.get $l2
                              i32.add
                              local.tee $p0
                              local.get $l5
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              i32.const 0
                              local.get $l5
                              i32.store offset=1059316
                              i32.const 0
                              local.get $p0
                              i32.store offset=1059328
                              local.get $l4
                              local.get $l3
                              i32.add
                              local.get $l5
                              i32.store
                              local.get $l4
                              local.get $l2
                              i32.const 3
                              i32.or
                              i32.store offset=4
                              br $B44
                            end
                            local.get $l4
                            local.get $l3
                            i32.const 3
                            i32.or
                            i32.store offset=4
                            local.get $l4
                            local.get $l3
                            i32.add
                            local.tee $l3
                            local.get $l3
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
                          local.get $l4
                          i32.const 8
                          i32.add
                          local.set $l3
                          br $B3
                        end
                        block $B46
                          i32.const 0
                          i32.load offset=1059320
                          local.tee $p0
                          local.get $l2
                          i32.le_u
                          br_if $B46
                          i32.const 0
                          i32.load offset=1059332
                          local.tee $l3
                          local.get $l2
                          i32.add
                          local.tee $l4
                          local.get $p0
                          local.get $l2
                          i32.sub
                          local.tee $l5
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          i32.const 0
                          local.get $l5
                          i32.store offset=1059320
                          i32.const 0
                          local.get $l4
                          i32.store offset=1059332
                          local.get $l3
                          local.get $l2
                          i32.const 3
                          i32.or
                          i32.store offset=4
                          local.get $l3
                          i32.const 8
                          i32.add
                          local.set $l3
                          br $B3
                        end
                        block $B47
                          block $B48
                            i32.const 0
                            i32.load offset=1059780
                            i32.eqz
                            br_if $B48
                            i32.const 0
                            i32.load offset=1059788
                            local.set $l4
                            br $B47
                          end
                          i32.const 0
                          i64.const -1
                          i64.store offset=1059792 align=4
                          i32.const 0
                          i64.const 281474976776192
                          i64.store offset=1059784 align=4
                          i32.const 0
                          local.get $l1
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
                          local.set $l4
                        end
                        i32.const 0
                        local.set $l3
                        block $B49
                          local.get $l4
                          local.get $l2
                          i32.const 71
                          i32.add
                          local.tee $l7
                          i32.add
                          local.tee $l6
                          i32.const 0
                          local.get $l4
                          i32.sub
                          local.tee $l11
                          i32.and
                          local.tee $l8
                          local.get $l2
                          i32.gt_u
                          br_if $B49
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059804
                          br $B3
                        end
                        block $B50
                          i32.const 0
                          i32.load offset=1059748
                          local.tee $l3
                          i32.eqz
                          br_if $B50
                          block $B51
                            i32.const 0
                            i32.load offset=1059740
                            local.tee $l4
                            local.get $l8
                            i32.add
                            local.tee $l5
                            local.get $l4
                            i32.le_u
                            br_if $B51
                            local.get $l5
                            local.get $l3
                            i32.le_u
                            br_if $B50
                          end
                          i32.const 0
                          local.set $l3
                          i32.const 0
                          i32.const 48
                          i32.store offset=1059804
                          br $B3
                        end
                        i32.const 0
                        i32.load8_u offset=1059752
                        i32.const 4
                        i32.and
                        br_if $B8
                        block $B52
                          block $B53
                            block $B54
                              i32.const 0
                              i32.load offset=1059332
                              local.tee $l4
                              i32.eqz
                              br_if $B54
                              i32.const 1059756
                              local.set $l3
                              loop $L55
                                block $B56
                                  local.get $l3
                                  i32.load
                                  local.tee $l5
                                  local.get $l4
                                  i32.gt_u
                                  br_if $B56
                                  local.get $l5
                                  local.get $l3
                                  i32.load offset=4
                                  i32.add
                                  local.get $l4
                                  i32.gt_u
                                  br_if $B53
                                end
                                local.get $l3
                                i32.load offset=8
                                local.tee $l3
                                br_if $L55
                              end
                            end
                            i32.const 0
                            call $sbrk
                            local.tee $p0
                            i32.const -1
                            i32.eq
                            br_if $B9
                            local.get $l8
                            local.set $l6
                            block $B57
                              i32.const 0
                              i32.load offset=1059784
                              local.tee $l3
                              i32.const -1
                              i32.add
                              local.tee $l4
                              local.get $p0
                              i32.and
                              i32.eqz
                              br_if $B57
                              local.get $l8
                              local.get $p0
                              i32.sub
                              local.get $l4
                              local.get $p0
                              i32.add
                              i32.const 0
                              local.get $l3
                              i32.sub
                              i32.and
                              i32.add
                              local.set $l6
                            end
                            local.get $l6
                            local.get $l2
                            i32.le_u
                            br_if $B9
                            local.get $l6
                            i32.const 2147483646
                            i32.gt_u
                            br_if $B9
                            block $B58
                              i32.const 0
                              i32.load offset=1059748
                              local.tee $l3
                              i32.eqz
                              br_if $B58
                              i32.const 0
                              i32.load offset=1059740
                              local.tee $l4
                              local.get $l6
                              i32.add
                              local.tee $l5
                              local.get $l4
                              i32.le_u
                              br_if $B9
                              local.get $l5
                              local.get $l3
                              i32.gt_u
                              br_if $B9
                            end
                            local.get $l6
                            call $sbrk
                            local.tee $l3
                            local.get $p0
                            i32.ne
                            br_if $B52
                            br $B7
                          end
                          local.get $l6
                          local.get $p0
                          i32.sub
                          local.get $l11
                          i32.and
                          local.tee $l6
                          i32.const 2147483646
                          i32.gt_u
                          br_if $B9
                          local.get $l6
                          call $sbrk
                          local.tee $p0
                          local.get $l3
                          i32.load
                          local.get $l3
                          i32.load offset=4
                          i32.add
                          i32.eq
                          br_if $B10
                          local.get $p0
                          local.set $l3
                        end
                        block $B59
                          local.get $l3
                          i32.const -1
                          i32.eq
                          br_if $B59
                          local.get $l2
                          i32.const 72
                          i32.add
                          local.get $l6
                          i32.le_u
                          br_if $B59
                          block $B60
                            local.get $l7
                            local.get $l6
                            i32.sub
                            i32.const 0
                            i32.load offset=1059788
                            local.tee $l4
                            i32.add
                            i32.const 0
                            local.get $l4
                            i32.sub
                            i32.and
                            local.tee $l4
                            i32.const 2147483646
                            i32.le_u
                            br_if $B60
                            local.get $l3
                            local.set $p0
                            br $B7
                          end
                          block $B61
                            local.get $l4
                            call $sbrk
                            i32.const -1
                            i32.eq
                            br_if $B61
                            local.get $l4
                            local.get $l6
                            i32.add
                            local.set $l6
                            local.get $l3
                            local.set $p0
                            br $B7
                          end
                          i32.const 0
                          local.get $l6
                          i32.sub
                          call $sbrk
                          drop
                          br $B9
                        end
                        local.get $l3
                        local.set $p0
                        local.get $l3
                        i32.const -1
                        i32.ne
                        br_if $B7
                        br $B9
                      end
                      i32.const 0
                      local.set $l8
                      br $B4
                    end
                    i32.const 0
                    local.set $p0
                    br $B5
                  end
                  local.get $p0
                  i32.const -1
                  i32.ne
                  br_if $B7
                end
                i32.const 0
                i32.const 0
                i32.load offset=1059752
                i32.const 4
                i32.or
                i32.store offset=1059752
              end
              local.get $l8
              i32.const 2147483646
              i32.gt_u
              br_if $B6
              local.get $l8
              call $sbrk
              local.set $p0
              i32.const 0
              call $sbrk
              local.set $l3
              local.get $p0
              i32.const -1
              i32.eq
              br_if $B6
              local.get $l3
              i32.const -1
              i32.eq
              br_if $B6
              local.get $p0
              local.get $l3
              i32.ge_u
              br_if $B6
              local.get $l3
              local.get $p0
              i32.sub
              local.tee $l6
              local.get $l2
              i32.const 56
              i32.add
              i32.le_u
              br_if $B6
            end
            i32.const 0
            i32.const 0
            i32.load offset=1059740
            local.get $l6
            i32.add
            local.tee $l3
            i32.store offset=1059740
            block $B62
              local.get $l3
              i32.const 0
              i32.load offset=1059744
              i32.le_u
              br_if $B62
              i32.const 0
              local.get $l3
              i32.store offset=1059744
            end
            block $B63
              block $B64
                block $B65
                  block $B66
                    i32.const 0
                    i32.load offset=1059332
                    local.tee $l4
                    i32.eqz
                    br_if $B66
                    i32.const 1059756
                    local.set $l3
                    loop $L67
                      local.get $p0
                      local.get $l3
                      i32.load
                      local.tee $l5
                      local.get $l3
                      i32.load offset=4
                      local.tee $l8
                      i32.add
                      i32.eq
                      br_if $B65
                      local.get $l3
                      i32.load offset=8
                      local.tee $l3
                      br_if $L67
                      br $B64
                    end
                  end
                  block $B68
                    block $B69
                      i32.const 0
                      i32.load offset=1059324
                      local.tee $l3
                      i32.eqz
                      br_if $B69
                      local.get $p0
                      local.get $l3
                      i32.ge_u
                      br_if $B68
                    end
                    i32.const 0
                    local.get $p0
                    i32.store offset=1059324
                  end
                  i32.const 0
                  local.set $l3
                  i32.const 0
                  local.get $l6
                  i32.store offset=1059760
                  i32.const 0
                  local.get $p0
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
                  loop $L70
                    local.get $l3
                    i32.const 1059368
                    i32.add
                    local.get $l3
                    i32.const 1059356
                    i32.add
                    local.tee $l4
                    i32.store
                    local.get $l4
                    local.get $l3
                    i32.const 1059348
                    i32.add
                    local.tee $l5
                    i32.store
                    local.get $l3
                    i32.const 1059360
                    i32.add
                    local.get $l5
                    i32.store
                    local.get $l3
                    i32.const 1059376
                    i32.add
                    local.get $l3
                    i32.const 1059364
                    i32.add
                    local.tee $l5
                    i32.store
                    local.get $l5
                    local.get $l4
                    i32.store
                    local.get $l3
                    i32.const 1059384
                    i32.add
                    local.get $l3
                    i32.const 1059372
                    i32.add
                    local.tee $l4
                    i32.store
                    local.get $l4
                    local.get $l5
                    i32.store
                    local.get $l3
                    i32.const 1059380
                    i32.add
                    local.get $l4
                    i32.store
                    local.get $l3
                    i32.const 32
                    i32.add
                    local.tee $l3
                    i32.const 256
                    i32.ne
                    br_if $L70
                  end
                  local.get $p0
                  i32.const -8
                  local.get $p0
                  i32.sub
                  i32.const 15
                  i32.and
                  i32.const 0
                  local.get $p0
                  i32.const 8
                  i32.add
                  i32.const 15
                  i32.and
                  select
                  local.tee $l3
                  i32.add
                  local.tee $l4
                  local.get $l6
                  i32.const -56
                  i32.add
                  local.tee $l5
                  local.get $l3
                  i32.sub
                  local.tee $l3
                  i32.const 1
                  i32.or
                  i32.store offset=4
                  i32.const 0
                  i32.const 0
                  i32.load offset=1059796
                  i32.store offset=1059336
                  i32.const 0
                  local.get $l3
                  i32.store offset=1059320
                  i32.const 0
                  local.get $l4
                  i32.store offset=1059332
                  local.get $p0
                  local.get $l5
                  i32.add
                  i32.const 56
                  i32.store offset=4
                  br $B63
                end
                local.get $l3
                i32.load8_u offset=12
                i32.const 8
                i32.and
                br_if $B64
                local.get $l5
                local.get $l4
                i32.gt_u
                br_if $B64
                local.get $p0
                local.get $l4
                i32.le_u
                br_if $B64
                local.get $l4
                i32.const -8
                local.get $l4
                i32.sub
                i32.const 15
                i32.and
                i32.const 0
                local.get $l4
                i32.const 8
                i32.add
                i32.const 15
                i32.and
                select
                local.tee $l5
                i32.add
                local.tee $p0
                i32.const 0
                i32.load offset=1059320
                local.get $l6
                i32.add
                local.tee $l11
                local.get $l5
                i32.sub
                local.tee $l5
                i32.const 1
                i32.or
                i32.store offset=4
                local.get $l3
                local.get $l8
                local.get $l6
                i32.add
                i32.store offset=4
                i32.const 0
                i32.const 0
                i32.load offset=1059796
                i32.store offset=1059336
                i32.const 0
                local.get $l5
                i32.store offset=1059320
                i32.const 0
                local.get $p0
                i32.store offset=1059332
                local.get $l4
                local.get $l11
                i32.add
                i32.const 56
                i32.store offset=4
                br $B63
              end
              block $B71
                local.get $p0
                i32.const 0
                i32.load offset=1059324
                local.tee $l8
                i32.ge_u
                br_if $B71
                i32.const 0
                local.get $p0
                i32.store offset=1059324
                local.get $p0
                local.set $l8
              end
              local.get $p0
              local.get $l6
              i32.add
              local.set $l5
              i32.const 1059756
              local.set $l3
              block $B72
                block $B73
                  block $B74
                    block $B75
                      block $B76
                        block $B77
                          block $B78
                            loop $L79
                              local.get $l3
                              i32.load
                              local.get $l5
                              i32.eq
                              br_if $B78
                              local.get $l3
                              i32.load offset=8
                              local.tee $l3
                              br_if $L79
                              br $B77
                            end
                          end
                          local.get $l3
                          i32.load8_u offset=12
                          i32.const 8
                          i32.and
                          i32.eqz
                          br_if $B76
                        end
                        i32.const 1059756
                        local.set $l3
                        loop $L80
                          block $B81
                            local.get $l3
                            i32.load
                            local.tee $l5
                            local.get $l4
                            i32.gt_u
                            br_if $B81
                            local.get $l5
                            local.get $l3
                            i32.load offset=4
                            i32.add
                            local.tee $l5
                            local.get $l4
                            i32.gt_u
                            br_if $B75
                          end
                          local.get $l3
                          i32.load offset=8
                          local.set $l3
                          br $L80
                        end
                      end
                      local.get $l3
                      local.get $p0
                      i32.store
                      local.get $l3
                      local.get $l3
                      i32.load offset=4
                      local.get $l6
                      i32.add
                      i32.store offset=4
                      local.get $p0
                      i32.const -8
                      local.get $p0
                      i32.sub
                      i32.const 15
                      i32.and
                      i32.const 0
                      local.get $p0
                      i32.const 8
                      i32.add
                      i32.const 15
                      i32.and
                      select
                      i32.add
                      local.tee $l11
                      local.get $l2
                      i32.const 3
                      i32.or
                      i32.store offset=4
                      local.get $l5
                      i32.const -8
                      local.get $l5
                      i32.sub
                      i32.const 15
                      i32.and
                      i32.const 0
                      local.get $l5
                      i32.const 8
                      i32.add
                      i32.const 15
                      i32.and
                      select
                      i32.add
                      local.tee $l6
                      local.get $l11
                      local.get $l2
                      i32.add
                      local.tee $l2
                      i32.sub
                      local.set $l5
                      block $B82
                        local.get $l4
                        local.get $l6
                        i32.ne
                        br_if $B82
                        i32.const 0
                        local.get $l2
                        i32.store offset=1059332
                        i32.const 0
                        i32.const 0
                        i32.load offset=1059320
                        local.get $l5
                        i32.add
                        local.tee $l3
                        i32.store offset=1059320
                        local.get $l2
                        local.get $l3
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        br $B73
                      end
                      block $B83
                        i32.const 0
                        i32.load offset=1059328
                        local.get $l6
                        i32.ne
                        br_if $B83
                        i32.const 0
                        local.get $l2
                        i32.store offset=1059328
                        i32.const 0
                        i32.const 0
                        i32.load offset=1059316
                        local.get $l5
                        i32.add
                        local.tee $l3
                        i32.store offset=1059316
                        local.get $l2
                        local.get $l3
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        local.get $l2
                        local.get $l3
                        i32.add
                        local.get $l3
                        i32.store
                        br $B73
                      end
                      block $B84
                        local.get $l6
                        i32.load offset=4
                        local.tee $l3
                        i32.const 3
                        i32.and
                        i32.const 1
                        i32.ne
                        br_if $B84
                        local.get $l3
                        i32.const -8
                        i32.and
                        local.set $l7
                        block $B85
                          block $B86
                            local.get $l3
                            i32.const 255
                            i32.gt_u
                            br_if $B86
                            local.get $l6
                            i32.load offset=8
                            local.tee $l4
                            local.get $l3
                            i32.const 3
                            i32.shr_u
                            local.tee $l8
                            i32.const 3
                            i32.shl
                            i32.const 1059348
                            i32.add
                            local.tee $p0
                            i32.eq
                            drop
                            block $B87
                              local.get $l6
                              i32.load offset=12
                              local.tee $l3
                              local.get $l4
                              i32.ne
                              br_if $B87
                              i32.const 0
                              i32.const 0
                              i32.load offset=1059308
                              i32.const -2
                              local.get $l8
                              i32.rotl
                              i32.and
                              i32.store offset=1059308
                              br $B85
                            end
                            local.get $l3
                            local.get $p0
                            i32.eq
                            drop
                            local.get $l3
                            local.get $l4
                            i32.store offset=8
                            local.get $l4
                            local.get $l3
                            i32.store offset=12
                            br $B85
                          end
                          local.get $l6
                          i32.load offset=24
                          local.set $l9
                          block $B88
                            block $B89
                              local.get $l6
                              i32.load offset=12
                              local.tee $p0
                              local.get $l6
                              i32.eq
                              br_if $B89
                              local.get $l8
                              local.get $l6
                              i32.load offset=8
                              local.tee $l3
                              i32.gt_u
                              drop
                              local.get $p0
                              local.get $l3
                              i32.store offset=8
                              local.get $l3
                              local.get $p0
                              i32.store offset=12
                              br $B88
                            end
                            block $B90
                              local.get $l6
                              i32.const 20
                              i32.add
                              local.tee $l3
                              i32.load
                              local.tee $l4
                              br_if $B90
                              local.get $l6
                              i32.const 16
                              i32.add
                              local.tee $l3
                              i32.load
                              local.tee $l4
                              br_if $B90
                              i32.const 0
                              local.set $p0
                              br $B88
                            end
                            loop $L91
                              local.get $l3
                              local.set $l8
                              local.get $l4
                              local.tee $p0
                              i32.const 20
                              i32.add
                              local.tee $l3
                              i32.load
                              local.tee $l4
                              br_if $L91
                              local.get $p0
                              i32.const 16
                              i32.add
                              local.set $l3
                              local.get $p0
                              i32.load offset=16
                              local.tee $l4
                              br_if $L91
                            end
                            local.get $l8
                            i32.const 0
                            i32.store
                          end
                          local.get $l9
                          i32.eqz
                          br_if $B85
                          block $B92
                            block $B93
                              local.get $l6
                              i32.load offset=28
                              local.tee $l4
                              i32.const 2
                              i32.shl
                              i32.const 1059612
                              i32.add
                              local.tee $l3
                              i32.load
                              local.get $l6
                              i32.ne
                              br_if $B93
                              local.get $l3
                              local.get $p0
                              i32.store
                              local.get $p0
                              br_if $B92
                              i32.const 0
                              i32.const 0
                              i32.load offset=1059312
                              i32.const -2
                              local.get $l4
                              i32.rotl
                              i32.and
                              i32.store offset=1059312
                              br $B85
                            end
                            local.get $l9
                            i32.const 16
                            i32.const 20
                            local.get $l9
                            i32.load offset=16
                            local.get $l6
                            i32.eq
                            select
                            i32.add
                            local.get $p0
                            i32.store
                            local.get $p0
                            i32.eqz
                            br_if $B85
                          end
                          local.get $p0
                          local.get $l9
                          i32.store offset=24
                          block $B94
                            local.get $l6
                            i32.load offset=16
                            local.tee $l3
                            i32.eqz
                            br_if $B94
                            local.get $p0
                            local.get $l3
                            i32.store offset=16
                            local.get $l3
                            local.get $p0
                            i32.store offset=24
                          end
                          local.get $l6
                          i32.load offset=20
                          local.tee $l3
                          i32.eqz
                          br_if $B85
                          local.get $p0
                          i32.const 20
                          i32.add
                          local.get $l3
                          i32.store
                          local.get $l3
                          local.get $p0
                          i32.store offset=24
                        end
                        local.get $l7
                        local.get $l5
                        i32.add
                        local.set $l5
                        local.get $l6
                        local.get $l7
                        i32.add
                        local.set $l6
                      end
                      local.get $l6
                      local.get $l6
                      i32.load offset=4
                      i32.const -2
                      i32.and
                      i32.store offset=4
                      local.get $l2
                      local.get $l5
                      i32.add
                      local.get $l5
                      i32.store
                      local.get $l2
                      local.get $l5
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      block $B95
                        local.get $l5
                        i32.const 255
                        i32.gt_u
                        br_if $B95
                        local.get $l5
                        i32.const 3
                        i32.shr_u
                        local.tee $l4
                        i32.const 3
                        i32.shl
                        i32.const 1059348
                        i32.add
                        local.set $l3
                        block $B96
                          block $B97
                            i32.const 0
                            i32.load offset=1059308
                            local.tee $l5
                            i32.const 1
                            local.get $l4
                            i32.shl
                            local.tee $l4
                            i32.and
                            br_if $B97
                            i32.const 0
                            local.get $l5
                            local.get $l4
                            i32.or
                            i32.store offset=1059308
                            local.get $l3
                            local.set $l4
                            br $B96
                          end
                          local.get $l3
                          i32.load offset=8
                          local.set $l4
                        end
                        local.get $l4
                        local.get $l2
                        i32.store offset=12
                        local.get $l3
                        local.get $l2
                        i32.store offset=8
                        local.get $l2
                        local.get $l3
                        i32.store offset=12
                        local.get $l2
                        local.get $l4
                        i32.store offset=8
                        br $B73
                      end
                      i32.const 31
                      local.set $l3
                      block $B98
                        local.get $l5
                        i32.const 16777215
                        i32.gt_u
                        br_if $B98
                        local.get $l5
                        i32.const 8
                        i32.shr_u
                        local.tee $l3
                        local.get $l3
                        i32.const 1048320
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 8
                        i32.and
                        local.tee $l3
                        i32.shl
                        local.tee $l4
                        local.get $l4
                        i32.const 520192
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 4
                        i32.and
                        local.tee $l4
                        i32.shl
                        local.tee $p0
                        local.get $p0
                        i32.const 245760
                        i32.add
                        i32.const 16
                        i32.shr_u
                        i32.const 2
                        i32.and
                        local.tee $p0
                        i32.shl
                        i32.const 15
                        i32.shr_u
                        local.get $l3
                        local.get $l4
                        i32.or
                        local.get $p0
                        i32.or
                        i32.sub
                        local.tee $l3
                        i32.const 1
                        i32.shl
                        local.get $l5
                        local.get $l3
                        i32.const 21
                        i32.add
                        i32.shr_u
                        i32.const 1
                        i32.and
                        i32.or
                        i32.const 28
                        i32.add
                        local.set $l3
                      end
                      local.get $l2
                      local.get $l3
                      i32.store offset=28
                      local.get $l2
                      i64.const 0
                      i64.store offset=16 align=4
                      local.get $l3
                      i32.const 2
                      i32.shl
                      i32.const 1059612
                      i32.add
                      local.set $l4
                      block $B99
                        i32.const 0
                        i32.load offset=1059312
                        local.tee $p0
                        i32.const 1
                        local.get $l3
                        i32.shl
                        local.tee $l8
                        i32.and
                        br_if $B99
                        local.get $l4
                        local.get $l2
                        i32.store
                        i32.const 0
                        local.get $p0
                        local.get $l8
                        i32.or
                        i32.store offset=1059312
                        local.get $l2
                        local.get $l4
                        i32.store offset=24
                        local.get $l2
                        local.get $l2
                        i32.store offset=8
                        local.get $l2
                        local.get $l2
                        i32.store offset=12
                        br $B73
                      end
                      local.get $l5
                      i32.const 0
                      i32.const 25
                      local.get $l3
                      i32.const 1
                      i32.shr_u
                      i32.sub
                      local.get $l3
                      i32.const 31
                      i32.eq
                      select
                      i32.shl
                      local.set $l3
                      local.get $l4
                      i32.load
                      local.set $p0
                      loop $L100
                        local.get $p0
                        local.tee $l4
                        i32.load offset=4
                        i32.const -8
                        i32.and
                        local.get $l5
                        i32.eq
                        br_if $B74
                        local.get $l3
                        i32.const 29
                        i32.shr_u
                        local.set $p0
                        local.get $l3
                        i32.const 1
                        i32.shl
                        local.set $l3
                        local.get $l4
                        local.get $p0
                        i32.const 4
                        i32.and
                        i32.add
                        i32.const 16
                        i32.add
                        local.tee $l8
                        i32.load
                        local.tee $p0
                        br_if $L100
                      end
                      local.get $l8
                      local.get $l2
                      i32.store
                      local.get $l2
                      local.get $l4
                      i32.store offset=24
                      local.get $l2
                      local.get $l2
                      i32.store offset=12
                      local.get $l2
                      local.get $l2
                      i32.store offset=8
                      br $B73
                    end
                    local.get $p0
                    i32.const -8
                    local.get $p0
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get $p0
                    i32.const 8
                    i32.add
                    i32.const 15
                    i32.and
                    select
                    local.tee $l3
                    i32.add
                    local.tee $l11
                    local.get $l6
                    i32.const -56
                    i32.add
                    local.tee $l8
                    local.get $l3
                    i32.sub
                    local.tee $l3
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    local.get $p0
                    local.get $l8
                    i32.add
                    i32.const 56
                    i32.store offset=4
                    local.get $l4
                    local.get $l5
                    i32.const 55
                    local.get $l5
                    i32.sub
                    i32.const 15
                    i32.and
                    i32.const 0
                    local.get $l5
                    i32.const -55
                    i32.add
                    i32.const 15
                    i32.and
                    select
                    i32.add
                    i32.const -63
                    i32.add
                    local.tee $l8
                    local.get $l8
                    local.get $l4
                    i32.const 16
                    i32.add
                    i32.lt_u
                    select
                    local.tee $l8
                    i32.const 35
                    i32.store offset=4
                    i32.const 0
                    i32.const 0
                    i32.load offset=1059796
                    i32.store offset=1059336
                    i32.const 0
                    local.get $l3
                    i32.store offset=1059320
                    i32.const 0
                    local.get $l11
                    i32.store offset=1059332
                    local.get $l8
                    i32.const 16
                    i32.add
                    i32.const 0
                    i64.load offset=1059764 align=4
                    i64.store align=4
                    local.get $l8
                    i32.const 0
                    i64.load offset=1059756 align=4
                    i64.store offset=8 align=4
                    i32.const 0
                    local.get $l8
                    i32.const 8
                    i32.add
                    i32.store offset=1059764
                    i32.const 0
                    local.get $l6
                    i32.store offset=1059760
                    i32.const 0
                    local.get $p0
                    i32.store offset=1059756
                    i32.const 0
                    i32.const 0
                    i32.store offset=1059768
                    local.get $l8
                    i32.const 36
                    i32.add
                    local.set $l3
                    loop $L101
                      local.get $l3
                      i32.const 7
                      i32.store
                      local.get $l5
                      local.get $l3
                      i32.const 4
                      i32.add
                      local.tee $l3
                      i32.gt_u
                      br_if $L101
                    end
                    local.get $l8
                    local.get $l4
                    i32.eq
                    br_if $B63
                    local.get $l8
                    local.get $l8
                    i32.load offset=4
                    i32.const -2
                    i32.and
                    i32.store offset=4
                    local.get $l8
                    local.get $l8
                    local.get $l4
                    i32.sub
                    local.tee $l6
                    i32.store
                    local.get $l4
                    local.get $l6
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    block $B102
                      local.get $l6
                      i32.const 255
                      i32.gt_u
                      br_if $B102
                      local.get $l6
                      i32.const 3
                      i32.shr_u
                      local.tee $l5
                      i32.const 3
                      i32.shl
                      i32.const 1059348
                      i32.add
                      local.set $l3
                      block $B103
                        block $B104
                          i32.const 0
                          i32.load offset=1059308
                          local.tee $p0
                          i32.const 1
                          local.get $l5
                          i32.shl
                          local.tee $l5
                          i32.and
                          br_if $B104
                          i32.const 0
                          local.get $p0
                          local.get $l5
                          i32.or
                          i32.store offset=1059308
                          local.get $l3
                          local.set $l5
                          br $B103
                        end
                        local.get $l3
                        i32.load offset=8
                        local.set $l5
                      end
                      local.get $l5
                      local.get $l4
                      i32.store offset=12
                      local.get $l3
                      local.get $l4
                      i32.store offset=8
                      local.get $l4
                      local.get $l3
                      i32.store offset=12
                      local.get $l4
                      local.get $l5
                      i32.store offset=8
                      br $B63
                    end
                    i32.const 31
                    local.set $l3
                    block $B105
                      local.get $l6
                      i32.const 16777215
                      i32.gt_u
                      br_if $B105
                      local.get $l6
                      i32.const 8
                      i32.shr_u
                      local.tee $l3
                      local.get $l3
                      i32.const 1048320
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 8
                      i32.and
                      local.tee $l3
                      i32.shl
                      local.tee $l5
                      local.get $l5
                      i32.const 520192
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 4
                      i32.and
                      local.tee $l5
                      i32.shl
                      local.tee $p0
                      local.get $p0
                      i32.const 245760
                      i32.add
                      i32.const 16
                      i32.shr_u
                      i32.const 2
                      i32.and
                      local.tee $p0
                      i32.shl
                      i32.const 15
                      i32.shr_u
                      local.get $l3
                      local.get $l5
                      i32.or
                      local.get $p0
                      i32.or
                      i32.sub
                      local.tee $l3
                      i32.const 1
                      i32.shl
                      local.get $l6
                      local.get $l3
                      i32.const 21
                      i32.add
                      i32.shr_u
                      i32.const 1
                      i32.and
                      i32.or
                      i32.const 28
                      i32.add
                      local.set $l3
                    end
                    local.get $l4
                    i64.const 0
                    i64.store offset=16 align=4
                    local.get $l4
                    i32.const 28
                    i32.add
                    local.get $l3
                    i32.store
                    local.get $l3
                    i32.const 2
                    i32.shl
                    i32.const 1059612
                    i32.add
                    local.set $l5
                    block $B106
                      i32.const 0
                      i32.load offset=1059312
                      local.tee $p0
                      i32.const 1
                      local.get $l3
                      i32.shl
                      local.tee $l8
                      i32.and
                      br_if $B106
                      local.get $l5
                      local.get $l4
                      i32.store
                      i32.const 0
                      local.get $p0
                      local.get $l8
                      i32.or
                      i32.store offset=1059312
                      local.get $l4
                      i32.const 24
                      i32.add
                      local.get $l5
                      i32.store
                      local.get $l4
                      local.get $l4
                      i32.store offset=8
                      local.get $l4
                      local.get $l4
                      i32.store offset=12
                      br $B63
                    end
                    local.get $l6
                    i32.const 0
                    i32.const 25
                    local.get $l3
                    i32.const 1
                    i32.shr_u
                    i32.sub
                    local.get $l3
                    i32.const 31
                    i32.eq
                    select
                    i32.shl
                    local.set $l3
                    local.get $l5
                    i32.load
                    local.set $p0
                    loop $L107
                      local.get $p0
                      local.tee $l5
                      i32.load offset=4
                      i32.const -8
                      i32.and
                      local.get $l6
                      i32.eq
                      br_if $B72
                      local.get $l3
                      i32.const 29
                      i32.shr_u
                      local.set $p0
                      local.get $l3
                      i32.const 1
                      i32.shl
                      local.set $l3
                      local.get $l5
                      local.get $p0
                      i32.const 4
                      i32.and
                      i32.add
                      i32.const 16
                      i32.add
                      local.tee $l8
                      i32.load
                      local.tee $p0
                      br_if $L107
                    end
                    local.get $l8
                    local.get $l4
                    i32.store
                    local.get $l4
                    i32.const 24
                    i32.add
                    local.get $l5
                    i32.store
                    local.get $l4
                    local.get $l4
                    i32.store offset=12
                    local.get $l4
                    local.get $l4
                    i32.store offset=8
                    br $B63
                  end
                  local.get $l4
                  i32.load offset=8
                  local.tee $l3
                  local.get $l2
                  i32.store offset=12
                  local.get $l4
                  local.get $l2
                  i32.store offset=8
                  local.get $l2
                  i32.const 0
                  i32.store offset=24
                  local.get $l2
                  local.get $l4
                  i32.store offset=12
                  local.get $l2
                  local.get $l3
                  i32.store offset=8
                end
                local.get $l11
                i32.const 8
                i32.add
                local.set $l3
                br $B3
              end
              local.get $l5
              i32.load offset=8
              local.tee $l3
              local.get $l4
              i32.store offset=12
              local.get $l5
              local.get $l4
              i32.store offset=8
              local.get $l4
              i32.const 24
              i32.add
              i32.const 0
              i32.store
              local.get $l4
              local.get $l5
              i32.store offset=12
              local.get $l4
              local.get $l3
              i32.store offset=8
            end
            i32.const 0
            i32.load offset=1059320
            local.tee $l3
            local.get $l2
            i32.le_u
            br_if $B6
            i32.const 0
            i32.load offset=1059332
            local.tee $l4
            local.get $l2
            i32.add
            local.tee $l5
            local.get $l3
            local.get $l2
            i32.sub
            local.tee $l3
            i32.const 1
            i32.or
            i32.store offset=4
            i32.const 0
            local.get $l3
            i32.store offset=1059320
            i32.const 0
            local.get $l5
            i32.store offset=1059332
            local.get $l4
            local.get $l2
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $l4
            i32.const 8
            i32.add
            local.set $l3
            br $B3
          end
          i32.const 0
          local.set $l3
          i32.const 0
          i32.const 48
          i32.store offset=1059804
          br $B3
        end
        block $B108
          local.get $l11
          i32.eqz
          br_if $B108
          block $B109
            block $B110
              local.get $l8
              local.get $l8
              i32.load offset=28
              local.tee $l5
              i32.const 2
              i32.shl
              i32.const 1059612
              i32.add
              local.tee $l3
              i32.load
              i32.ne
              br_if $B110
              local.get $l3
              local.get $p0
              i32.store
              local.get $p0
              br_if $B109
              i32.const 0
              local.get $l7
              i32.const -2
              local.get $l5
              i32.rotl
              i32.and
              local.tee $l7
              i32.store offset=1059312
              br $B108
            end
            local.get $l11
            i32.const 16
            i32.const 20
            local.get $l11
            i32.load offset=16
            local.get $l8
            i32.eq
            select
            i32.add
            local.get $p0
            i32.store
            local.get $p0
            i32.eqz
            br_if $B108
          end
          local.get $p0
          local.get $l11
          i32.store offset=24
          block $B111
            local.get $l8
            i32.load offset=16
            local.tee $l3
            i32.eqz
            br_if $B111
            local.get $p0
            local.get $l3
            i32.store offset=16
            local.get $l3
            local.get $p0
            i32.store offset=24
          end
          local.get $l8
          i32.const 20
          i32.add
          i32.load
          local.tee $l3
          i32.eqz
          br_if $B108
          local.get $p0
          i32.const 20
          i32.add
          local.get $l3
          i32.store
          local.get $l3
          local.get $p0
          i32.store offset=24
        end
        block $B112
          block $B113
            local.get $l4
            i32.const 15
            i32.gt_u
            br_if $B113
            local.get $l8
            local.get $l4
            local.get $l2
            i32.add
            local.tee $l3
            i32.const 3
            i32.or
            i32.store offset=4
            local.get $l8
            local.get $l3
            i32.add
            local.tee $l3
            local.get $l3
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            br $B112
          end
          local.get $l8
          local.get $l2
          i32.add
          local.tee $p0
          local.get $l4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $l8
          local.get $l2
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $p0
          local.get $l4
          i32.add
          local.get $l4
          i32.store
          block $B114
            local.get $l4
            i32.const 255
            i32.gt_u
            br_if $B114
            local.get $l4
            i32.const 3
            i32.shr_u
            local.tee $l4
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.set $l3
            block $B115
              block $B116
                i32.const 0
                i32.load offset=1059308
                local.tee $l5
                i32.const 1
                local.get $l4
                i32.shl
                local.tee $l4
                i32.and
                br_if $B116
                i32.const 0
                local.get $l5
                local.get $l4
                i32.or
                i32.store offset=1059308
                local.get $l3
                local.set $l4
                br $B115
              end
              local.get $l3
              i32.load offset=8
              local.set $l4
            end
            local.get $l4
            local.get $p0
            i32.store offset=12
            local.get $l3
            local.get $p0
            i32.store offset=8
            local.get $p0
            local.get $l3
            i32.store offset=12
            local.get $p0
            local.get $l4
            i32.store offset=8
            br $B112
          end
          i32.const 31
          local.set $l3
          block $B117
            local.get $l4
            i32.const 16777215
            i32.gt_u
            br_if $B117
            local.get $l4
            i32.const 8
            i32.shr_u
            local.tee $l3
            local.get $l3
            i32.const 1048320
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 8
            i32.and
            local.tee $l3
            i32.shl
            local.tee $l5
            local.get $l5
            i32.const 520192
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 4
            i32.and
            local.tee $l5
            i32.shl
            local.tee $l2
            local.get $l2
            i32.const 245760
            i32.add
            i32.const 16
            i32.shr_u
            i32.const 2
            i32.and
            local.tee $l2
            i32.shl
            i32.const 15
            i32.shr_u
            local.get $l3
            local.get $l5
            i32.or
            local.get $l2
            i32.or
            i32.sub
            local.tee $l3
            i32.const 1
            i32.shl
            local.get $l4
            local.get $l3
            i32.const 21
            i32.add
            i32.shr_u
            i32.const 1
            i32.and
            i32.or
            i32.const 28
            i32.add
            local.set $l3
          end
          local.get $p0
          local.get $l3
          i32.store offset=28
          local.get $p0
          i64.const 0
          i64.store offset=16 align=4
          local.get $l3
          i32.const 2
          i32.shl
          i32.const 1059612
          i32.add
          local.set $l5
          block $B118
            local.get $l7
            i32.const 1
            local.get $l3
            i32.shl
            local.tee $l2
            i32.and
            br_if $B118
            local.get $l5
            local.get $p0
            i32.store
            i32.const 0
            local.get $l7
            local.get $l2
            i32.or
            i32.store offset=1059312
            local.get $p0
            local.get $l5
            i32.store offset=24
            local.get $p0
            local.get $p0
            i32.store offset=8
            local.get $p0
            local.get $p0
            i32.store offset=12
            br $B112
          end
          local.get $l4
          i32.const 0
          i32.const 25
          local.get $l3
          i32.const 1
          i32.shr_u
          i32.sub
          local.get $l3
          i32.const 31
          i32.eq
          select
          i32.shl
          local.set $l3
          local.get $l5
          i32.load
          local.set $l2
          block $B119
            loop $L120
              local.get $l2
              local.tee $l5
              i32.load offset=4
              i32.const -8
              i32.and
              local.get $l4
              i32.eq
              br_if $B119
              local.get $l3
              i32.const 29
              i32.shr_u
              local.set $l2
              local.get $l3
              i32.const 1
              i32.shl
              local.set $l3
              local.get $l5
              local.get $l2
              i32.const 4
              i32.and
              i32.add
              i32.const 16
              i32.add
              local.tee $l6
              i32.load
              local.tee $l2
              br_if $L120
            end
            local.get $l6
            local.get $p0
            i32.store
            local.get $p0
            local.get $l5
            i32.store offset=24
            local.get $p0
            local.get $p0
            i32.store offset=12
            local.get $p0
            local.get $p0
            i32.store offset=8
            br $B112
          end
          local.get $l5
          i32.load offset=8
          local.tee $l3
          local.get $p0
          i32.store offset=12
          local.get $l5
          local.get $p0
          i32.store offset=8
          local.get $p0
          i32.const 0
          i32.store offset=24
          local.get $p0
          local.get $l5
          i32.store offset=12
          local.get $p0
          local.get $l3
          i32.store offset=8
        end
        local.get $l8
        i32.const 8
        i32.add
        local.set $l3
        br $B3
      end
      block $B121
        local.get $l10
        i32.eqz
        br_if $B121
        block $B122
          block $B123
            local.get $p0
            local.get $p0
            i32.load offset=28
            local.tee $l5
            i32.const 2
            i32.shl
            i32.const 1059612
            i32.add
            local.tee $l3
            i32.load
            i32.ne
            br_if $B123
            local.get $l3
            local.get $l8
            i32.store
            local.get $l8
            br_if $B122
            i32.const 0
            local.get $l9
            i32.const -2
            local.get $l5
            i32.rotl
            i32.and
            i32.store offset=1059312
            br $B121
          end
          local.get $l10
          i32.const 16
          i32.const 20
          local.get $l10
          i32.load offset=16
          local.get $p0
          i32.eq
          select
          i32.add
          local.get $l8
          i32.store
          local.get $l8
          i32.eqz
          br_if $B121
        end
        local.get $l8
        local.get $l10
        i32.store offset=24
        block $B124
          local.get $p0
          i32.load offset=16
          local.tee $l3
          i32.eqz
          br_if $B124
          local.get $l8
          local.get $l3
          i32.store offset=16
          local.get $l3
          local.get $l8
          i32.store offset=24
        end
        local.get $p0
        i32.const 20
        i32.add
        i32.load
        local.tee $l3
        i32.eqz
        br_if $B121
        local.get $l8
        i32.const 20
        i32.add
        local.get $l3
        i32.store
        local.get $l3
        local.get $l8
        i32.store offset=24
      end
      block $B125
        block $B126
          local.get $l4
          i32.const 15
          i32.gt_u
          br_if $B126
          local.get $p0
          local.get $l4
          local.get $l2
          i32.add
          local.tee $l3
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $p0
          local.get $l3
          i32.add
          local.tee $l3
          local.get $l3
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          br $B125
        end
        local.get $p0
        local.get $l2
        i32.add
        local.tee $l5
        local.get $l4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $p0
        local.get $l2
        i32.const 3
        i32.or
        i32.store offset=4
        local.get $l5
        local.get $l4
        i32.add
        local.get $l4
        i32.store
        block $B127
          local.get $l7
          i32.eqz
          br_if $B127
          local.get $l7
          i32.const 3
          i32.shr_u
          local.tee $l8
          i32.const 3
          i32.shl
          i32.const 1059348
          i32.add
          local.set $l2
          i32.const 0
          i32.load offset=1059328
          local.set $l3
          block $B128
            block $B129
              i32.const 1
              local.get $l8
              i32.shl
              local.tee $l8
              local.get $l6
              i32.and
              br_if $B129
              i32.const 0
              local.get $l8
              local.get $l6
              i32.or
              i32.store offset=1059308
              local.get $l2
              local.set $l8
              br $B128
            end
            local.get $l2
            i32.load offset=8
            local.set $l8
          end
          local.get $l8
          local.get $l3
          i32.store offset=12
          local.get $l2
          local.get $l3
          i32.store offset=8
          local.get $l3
          local.get $l2
          i32.store offset=12
          local.get $l3
          local.get $l8
          i32.store offset=8
        end
        i32.const 0
        local.get $l5
        i32.store offset=1059328
        i32.const 0
        local.get $l4
        i32.store offset=1059316
      end
      local.get $p0
      i32.const 8
      i32.add
      local.set $l3
    end
    local.get $l1
    i32.const 16
    i32.add
    global.set $g0
    local.get $l3)
  (func $free (type $t0) (param $p0 i32)
    local.get $p0
    call $dlfree)
  (func $dlfree (type $t0) (param $p0 i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32)
    block $B0
      local.get $p0
      i32.eqz
      br_if $B0
      local.get $p0
      i32.const -8
      i32.add
      local.tee $l1
      local.get $p0
      i32.const -4
      i32.add
      i32.load
      local.tee $l2
      i32.const -8
      i32.and
      local.tee $p0
      i32.add
      local.set $l3
      block $B1
        local.get $l2
        i32.const 1
        i32.and
        br_if $B1
        local.get $l2
        i32.const 3
        i32.and
        i32.eqz
        br_if $B0
        local.get $l1
        local.get $l1
        i32.load
        local.tee $l2
        i32.sub
        local.tee $l1
        i32.const 0
        i32.load offset=1059324
        local.tee $l4
        i32.lt_u
        br_if $B0
        local.get $l2
        local.get $p0
        i32.add
        local.set $p0
        block $B2
          i32.const 0
          i32.load offset=1059328
          local.get $l1
          i32.eq
          br_if $B2
          block $B3
            local.get $l2
            i32.const 255
            i32.gt_u
            br_if $B3
            local.get $l1
            i32.load offset=8
            local.tee $l4
            local.get $l2
            i32.const 3
            i32.shr_u
            local.tee $l5
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.tee $l6
            i32.eq
            drop
            block $B4
              local.get $l1
              i32.load offset=12
              local.tee $l2
              local.get $l4
              i32.ne
              br_if $B4
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get $l5
              i32.rotl
              i32.and
              i32.store offset=1059308
              br $B1
            end
            local.get $l2
            local.get $l6
            i32.eq
            drop
            local.get $l2
            local.get $l4
            i32.store offset=8
            local.get $l4
            local.get $l2
            i32.store offset=12
            br $B1
          end
          local.get $l1
          i32.load offset=24
          local.set $l7
          block $B5
            block $B6
              local.get $l1
              i32.load offset=12
              local.tee $l6
              local.get $l1
              i32.eq
              br_if $B6
              local.get $l4
              local.get $l1
              i32.load offset=8
              local.tee $l2
              i32.gt_u
              drop
              local.get $l6
              local.get $l2
              i32.store offset=8
              local.get $l2
              local.get $l6
              i32.store offset=12
              br $B5
            end
            block $B7
              local.get $l1
              i32.const 20
              i32.add
              local.tee $l2
              i32.load
              local.tee $l4
              br_if $B7
              local.get $l1
              i32.const 16
              i32.add
              local.tee $l2
              i32.load
              local.tee $l4
              br_if $B7
              i32.const 0
              local.set $l6
              br $B5
            end
            loop $L8
              local.get $l2
              local.set $l5
              local.get $l4
              local.tee $l6
              i32.const 20
              i32.add
              local.tee $l2
              i32.load
              local.tee $l4
              br_if $L8
              local.get $l6
              i32.const 16
              i32.add
              local.set $l2
              local.get $l6
              i32.load offset=16
              local.tee $l4
              br_if $L8
            end
            local.get $l5
            i32.const 0
            i32.store
          end
          local.get $l7
          i32.eqz
          br_if $B1
          block $B9
            block $B10
              local.get $l1
              i32.load offset=28
              local.tee $l4
              i32.const 2
              i32.shl
              i32.const 1059612
              i32.add
              local.tee $l2
              i32.load
              local.get $l1
              i32.ne
              br_if $B10
              local.get $l2
              local.get $l6
              i32.store
              local.get $l6
              br_if $B9
              i32.const 0
              i32.const 0
              i32.load offset=1059312
              i32.const -2
              local.get $l4
              i32.rotl
              i32.and
              i32.store offset=1059312
              br $B1
            end
            local.get $l7
            i32.const 16
            i32.const 20
            local.get $l7
            i32.load offset=16
            local.get $l1
            i32.eq
            select
            i32.add
            local.get $l6
            i32.store
            local.get $l6
            i32.eqz
            br_if $B1
          end
          local.get $l6
          local.get $l7
          i32.store offset=24
          block $B11
            local.get $l1
            i32.load offset=16
            local.tee $l2
            i32.eqz
            br_if $B11
            local.get $l6
            local.get $l2
            i32.store offset=16
            local.get $l2
            local.get $l6
            i32.store offset=24
          end
          local.get $l1
          i32.load offset=20
          local.tee $l2
          i32.eqz
          br_if $B1
          local.get $l6
          i32.const 20
          i32.add
          local.get $l2
          i32.store
          local.get $l2
          local.get $l6
          i32.store offset=24
          br $B1
        end
        local.get $l3
        i32.load offset=4
        local.tee $l2
        i32.const 3
        i32.and
        i32.const 3
        i32.ne
        br_if $B1
        local.get $l3
        local.get $l2
        i32.const -2
        i32.and
        i32.store offset=4
        i32.const 0
        local.get $p0
        i32.store offset=1059316
        local.get $l1
        local.get $p0
        i32.add
        local.get $p0
        i32.store
        local.get $l1
        local.get $p0
        i32.const 1
        i32.or
        i32.store offset=4
        return
      end
      local.get $l3
      local.get $l1
      i32.le_u
      br_if $B0
      local.get $l3
      i32.load offset=4
      local.tee $l2
      i32.const 1
      i32.and
      i32.eqz
      br_if $B0
      block $B12
        block $B13
          local.get $l2
          i32.const 2
          i32.and
          br_if $B13
          block $B14
            i32.const 0
            i32.load offset=1059332
            local.get $l3
            i32.ne
            br_if $B14
            i32.const 0
            local.get $l1
            i32.store offset=1059332
            i32.const 0
            i32.const 0
            i32.load offset=1059320
            local.get $p0
            i32.add
            local.tee $p0
            i32.store offset=1059320
            local.get $l1
            local.get $p0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $l1
            i32.const 0
            i32.load offset=1059328
            i32.ne
            br_if $B0
            i32.const 0
            i32.const 0
            i32.store offset=1059316
            i32.const 0
            i32.const 0
            i32.store offset=1059328
            return
          end
          block $B15
            i32.const 0
            i32.load offset=1059328
            local.get $l3
            i32.ne
            br_if $B15
            i32.const 0
            local.get $l1
            i32.store offset=1059328
            i32.const 0
            i32.const 0
            i32.load offset=1059316
            local.get $p0
            i32.add
            local.tee $p0
            i32.store offset=1059316
            local.get $l1
            local.get $p0
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $l1
            local.get $p0
            i32.add
            local.get $p0
            i32.store
            return
          end
          local.get $l2
          i32.const -8
          i32.and
          local.get $p0
          i32.add
          local.set $p0
          block $B16
            block $B17
              local.get $l2
              i32.const 255
              i32.gt_u
              br_if $B17
              local.get $l3
              i32.load offset=8
              local.tee $l4
              local.get $l2
              i32.const 3
              i32.shr_u
              local.tee $l5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee $l6
              i32.eq
              drop
              block $B18
                local.get $l3
                i32.load offset=12
                local.tee $l2
                local.get $l4
                i32.ne
                br_if $B18
                i32.const 0
                i32.const 0
                i32.load offset=1059308
                i32.const -2
                local.get $l5
                i32.rotl
                i32.and
                i32.store offset=1059308
                br $B16
              end
              local.get $l2
              local.get $l6
              i32.eq
              drop
              local.get $l2
              local.get $l4
              i32.store offset=8
              local.get $l4
              local.get $l2
              i32.store offset=12
              br $B16
            end
            local.get $l3
            i32.load offset=24
            local.set $l7
            block $B19
              block $B20
                local.get $l3
                i32.load offset=12
                local.tee $l6
                local.get $l3
                i32.eq
                br_if $B20
                i32.const 0
                i32.load offset=1059324
                local.get $l3
                i32.load offset=8
                local.tee $l2
                i32.gt_u
                drop
                local.get $l6
                local.get $l2
                i32.store offset=8
                local.get $l2
                local.get $l6
                i32.store offset=12
                br $B19
              end
              block $B21
                local.get $l3
                i32.const 20
                i32.add
                local.tee $l2
                i32.load
                local.tee $l4
                br_if $B21
                local.get $l3
                i32.const 16
                i32.add
                local.tee $l2
                i32.load
                local.tee $l4
                br_if $B21
                i32.const 0
                local.set $l6
                br $B19
              end
              loop $L22
                local.get $l2
                local.set $l5
                local.get $l4
                local.tee $l6
                i32.const 20
                i32.add
                local.tee $l2
                i32.load
                local.tee $l4
                br_if $L22
                local.get $l6
                i32.const 16
                i32.add
                local.set $l2
                local.get $l6
                i32.load offset=16
                local.tee $l4
                br_if $L22
              end
              local.get $l5
              i32.const 0
              i32.store
            end
            local.get $l7
            i32.eqz
            br_if $B16
            block $B23
              block $B24
                local.get $l3
                i32.load offset=28
                local.tee $l4
                i32.const 2
                i32.shl
                i32.const 1059612
                i32.add
                local.tee $l2
                i32.load
                local.get $l3
                i32.ne
                br_if $B24
                local.get $l2
                local.get $l6
                i32.store
                local.get $l6
                br_if $B23
                i32.const 0
                i32.const 0
                i32.load offset=1059312
                i32.const -2
                local.get $l4
                i32.rotl
                i32.and
                i32.store offset=1059312
                br $B16
              end
              local.get $l7
              i32.const 16
              i32.const 20
              local.get $l7
              i32.load offset=16
              local.get $l3
              i32.eq
              select
              i32.add
              local.get $l6
              i32.store
              local.get $l6
              i32.eqz
              br_if $B16
            end
            local.get $l6
            local.get $l7
            i32.store offset=24
            block $B25
              local.get $l3
              i32.load offset=16
              local.tee $l2
              i32.eqz
              br_if $B25
              local.get $l6
              local.get $l2
              i32.store offset=16
              local.get $l2
              local.get $l6
              i32.store offset=24
            end
            local.get $l3
            i32.load offset=20
            local.tee $l2
            i32.eqz
            br_if $B16
            local.get $l6
            i32.const 20
            i32.add
            local.get $l2
            i32.store
            local.get $l2
            local.get $l6
            i32.store offset=24
          end
          local.get $l1
          local.get $p0
          i32.add
          local.get $p0
          i32.store
          local.get $l1
          local.get $p0
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $l1
          i32.const 0
          i32.load offset=1059328
          i32.ne
          br_if $B12
          i32.const 0
          local.get $p0
          i32.store offset=1059316
          return
        end
        local.get $l3
        local.get $l2
        i32.const -2
        i32.and
        i32.store offset=4
        local.get $l1
        local.get $p0
        i32.add
        local.get $p0
        i32.store
        local.get $l1
        local.get $p0
        i32.const 1
        i32.or
        i32.store offset=4
      end
      block $B26
        local.get $p0
        i32.const 255
        i32.gt_u
        br_if $B26
        local.get $p0
        i32.const 3
        i32.shr_u
        local.tee $l2
        i32.const 3
        i32.shl
        i32.const 1059348
        i32.add
        local.set $p0
        block $B27
          block $B28
            i32.const 0
            i32.load offset=1059308
            local.tee $l4
            i32.const 1
            local.get $l2
            i32.shl
            local.tee $l2
            i32.and
            br_if $B28
            i32.const 0
            local.get $l4
            local.get $l2
            i32.or
            i32.store offset=1059308
            local.get $p0
            local.set $l2
            br $B27
          end
          local.get $p0
          i32.load offset=8
          local.set $l2
        end
        local.get $l2
        local.get $l1
        i32.store offset=12
        local.get $p0
        local.get $l1
        i32.store offset=8
        local.get $l1
        local.get $p0
        i32.store offset=12
        local.get $l1
        local.get $l2
        i32.store offset=8
        return
      end
      i32.const 31
      local.set $l2
      block $B29
        local.get $p0
        i32.const 16777215
        i32.gt_u
        br_if $B29
        local.get $p0
        i32.const 8
        i32.shr_u
        local.tee $l2
        local.get $l2
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee $l2
        i32.shl
        local.tee $l4
        local.get $l4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.tee $l4
        i32.shl
        local.tee $l6
        local.get $l6
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.tee $l6
        i32.shl
        i32.const 15
        i32.shr_u
        local.get $l2
        local.get $l4
        i32.or
        local.get $l6
        i32.or
        i32.sub
        local.tee $l2
        i32.const 1
        i32.shl
        local.get $p0
        local.get $l2
        i32.const 21
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        i32.or
        i32.const 28
        i32.add
        local.set $l2
      end
      local.get $l1
      i64.const 0
      i64.store offset=16 align=4
      local.get $l1
      i32.const 28
      i32.add
      local.get $l2
      i32.store
      local.get $l2
      i32.const 2
      i32.shl
      i32.const 1059612
      i32.add
      local.set $l4
      block $B30
        block $B31
          i32.const 0
          i32.load offset=1059312
          local.tee $l6
          i32.const 1
          local.get $l2
          i32.shl
          local.tee $l3
          i32.and
          br_if $B31
          local.get $l4
          local.get $l1
          i32.store
          i32.const 0
          local.get $l6
          local.get $l3
          i32.or
          i32.store offset=1059312
          local.get $l1
          i32.const 24
          i32.add
          local.get $l4
          i32.store
          local.get $l1
          local.get $l1
          i32.store offset=8
          local.get $l1
          local.get $l1
          i32.store offset=12
          br $B30
        end
        local.get $p0
        i32.const 0
        i32.const 25
        local.get $l2
        i32.const 1
        i32.shr_u
        i32.sub
        local.get $l2
        i32.const 31
        i32.eq
        select
        i32.shl
        local.set $l2
        local.get $l4
        i32.load
        local.set $l6
        block $B32
          loop $L33
            local.get $l6
            local.tee $l4
            i32.load offset=4
            i32.const -8
            i32.and
            local.get $p0
            i32.eq
            br_if $B32
            local.get $l2
            i32.const 29
            i32.shr_u
            local.set $l6
            local.get $l2
            i32.const 1
            i32.shl
            local.set $l2
            local.get $l4
            local.get $l6
            i32.const 4
            i32.and
            i32.add
            i32.const 16
            i32.add
            local.tee $l3
            i32.load
            local.tee $l6
            br_if $L33
          end
          local.get $l3
          local.get $l1
          i32.store
          local.get $l1
          i32.const 24
          i32.add
          local.get $l4
          i32.store
          local.get $l1
          local.get $l1
          i32.store offset=12
          local.get $l1
          local.get $l1
          i32.store offset=8
          br $B30
        end
        local.get $l4
        i32.load offset=8
        local.tee $p0
        local.get $l1
        i32.store offset=12
        local.get $l4
        local.get $l1
        i32.store offset=8
        local.get $l1
        i32.const 24
        i32.add
        i32.const 0
        i32.store
        local.get $l1
        local.get $l4
        i32.store offset=12
        local.get $l1
        local.get $p0
        i32.store offset=8
      end
      i32.const 0
      i32.const 0
      i32.load offset=1059340
      i32.const -1
      i32.add
      local.tee $l1
      i32.const -1
      local.get $l1
      select
      i32.store offset=1059340
    end)
  (func $calloc (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i64)
    block $B0
      block $B1
        local.get $p0
        br_if $B1
        i32.const 0
        local.set $l2
        br $B0
      end
      local.get $p0
      i64.extend_i32_u
      local.get $p1
      i64.extend_i32_u
      i64.mul
      local.tee $l3
      i32.wrap_i64
      local.set $l2
      local.get $p1
      local.get $p0
      i32.or
      i32.const 65536
      i32.lt_u
      br_if $B0
      i32.const -1
      local.get $l2
      local.get $l3
      i64.const 32
      i64.shr_u
      i32.wrap_i64
      i32.const 0
      i32.ne
      select
      local.set $l2
    end
    block $B2
      local.get $l2
      call $dlmalloc
      local.tee $p0
      i32.eqz
      br_if $B2
      local.get $p0
      i32.const -4
      i32.add
      i32.load8_u
      i32.const 3
      i32.and
      i32.eqz
      br_if $B2
      local.get $p0
      i32.const 0
      local.get $l2
      call $memset
      drop
    end
    local.get $p0)
  (func $realloc (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32)
    block $B0
      local.get $p0
      br_if $B0
      local.get $p1
      call $dlmalloc
      return
    end
    block $B1
      local.get $p1
      i32.const -64
      i32.lt_u
      br_if $B1
      i32.const 0
      i32.const 48
      i32.store offset=1059804
      i32.const 0
      return
    end
    i32.const 16
    local.get $p1
    i32.const 19
    i32.add
    i32.const -16
    i32.and
    local.get $p1
    i32.const 11
    i32.lt_u
    select
    local.set $l2
    local.get $p0
    i32.const -4
    i32.add
    local.tee $l3
    i32.load
    local.tee $l4
    i32.const -8
    i32.and
    local.set $l5
    block $B2
      block $B3
        block $B4
          local.get $l4
          i32.const 3
          i32.and
          br_if $B4
          local.get $l2
          i32.const 256
          i32.lt_u
          br_if $B3
          local.get $l5
          local.get $l2
          i32.const 4
          i32.or
          i32.lt_u
          br_if $B3
          local.get $l5
          local.get $l2
          i32.sub
          i32.const 0
          i32.load offset=1059788
          i32.const 1
          i32.shl
          i32.le_u
          br_if $B2
          br $B3
        end
        local.get $p0
        i32.const -8
        i32.add
        local.tee $l6
        local.get $l5
        i32.add
        local.set $l7
        block $B5
          local.get $l5
          local.get $l2
          i32.lt_u
          br_if $B5
          local.get $l5
          local.get $l2
          i32.sub
          local.tee $p1
          i32.const 16
          i32.lt_u
          br_if $B2
          local.get $l3
          local.get $l2
          local.get $l4
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get $l6
          local.get $l2
          i32.add
          local.tee $l2
          local.get $p1
          i32.const 3
          i32.or
          i32.store offset=4
          local.get $l7
          local.get $l7
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $l2
          local.get $p1
          call $dispose_chunk
          local.get $p0
          return
        end
        block $B6
          i32.const 0
          i32.load offset=1059332
          local.get $l7
          i32.ne
          br_if $B6
          i32.const 0
          i32.load offset=1059320
          local.get $l5
          i32.add
          local.tee $l5
          local.get $l2
          i32.le_u
          br_if $B3
          local.get $l3
          local.get $l2
          local.get $l4
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          i32.const 0
          local.get $l6
          local.get $l2
          i32.add
          local.tee $p1
          i32.store offset=1059332
          i32.const 0
          local.get $l5
          local.get $l2
          i32.sub
          local.tee $l2
          i32.store offset=1059320
          local.get $p1
          local.get $l2
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $p0
          return
        end
        block $B7
          i32.const 0
          i32.load offset=1059328
          local.get $l7
          i32.ne
          br_if $B7
          i32.const 0
          i32.load offset=1059316
          local.get $l5
          i32.add
          local.tee $l5
          local.get $l2
          i32.lt_u
          br_if $B3
          block $B8
            block $B9
              local.get $l5
              local.get $l2
              i32.sub
              local.tee $p1
              i32.const 16
              i32.lt_u
              br_if $B9
              local.get $l3
              local.get $l2
              local.get $l4
              i32.const 1
              i32.and
              i32.or
              i32.const 2
              i32.or
              i32.store
              local.get $l6
              local.get $l2
              i32.add
              local.tee $l2
              local.get $p1
              i32.const 1
              i32.or
              i32.store offset=4
              local.get $l6
              local.get $l5
              i32.add
              local.tee $l5
              local.get $p1
              i32.store
              local.get $l5
              local.get $l5
              i32.load offset=4
              i32.const -2
              i32.and
              i32.store offset=4
              br $B8
            end
            local.get $l3
            local.get $l4
            i32.const 1
            i32.and
            local.get $l5
            i32.or
            i32.const 2
            i32.or
            i32.store
            local.get $l6
            local.get $l5
            i32.add
            local.tee $p1
            local.get $p1
            i32.load offset=4
            i32.const 1
            i32.or
            i32.store offset=4
            i32.const 0
            local.set $p1
            i32.const 0
            local.set $l2
          end
          i32.const 0
          local.get $l2
          i32.store offset=1059328
          i32.const 0
          local.get $p1
          i32.store offset=1059316
          local.get $p0
          return
        end
        local.get $l7
        i32.load offset=4
        local.tee $l8
        i32.const 2
        i32.and
        br_if $B3
        local.get $l8
        i32.const -8
        i32.and
        local.get $l5
        i32.add
        local.tee $l9
        local.get $l2
        i32.lt_u
        br_if $B3
        local.get $l9
        local.get $l2
        i32.sub
        local.set $l10
        block $B10
          block $B11
            local.get $l8
            i32.const 255
            i32.gt_u
            br_if $B11
            local.get $l7
            i32.load offset=8
            local.tee $p1
            local.get $l8
            i32.const 3
            i32.shr_u
            local.tee $l11
            i32.const 3
            i32.shl
            i32.const 1059348
            i32.add
            local.tee $l8
            i32.eq
            drop
            block $B12
              local.get $l7
              i32.load offset=12
              local.tee $l5
              local.get $p1
              i32.ne
              br_if $B12
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get $l11
              i32.rotl
              i32.and
              i32.store offset=1059308
              br $B10
            end
            local.get $l5
            local.get $l8
            i32.eq
            drop
            local.get $l5
            local.get $p1
            i32.store offset=8
            local.get $p1
            local.get $l5
            i32.store offset=12
            br $B10
          end
          local.get $l7
          i32.load offset=24
          local.set $l12
          block $B13
            block $B14
              local.get $l7
              i32.load offset=12
              local.tee $l8
              local.get $l7
              i32.eq
              br_if $B14
              i32.const 0
              i32.load offset=1059324
              local.get $l7
              i32.load offset=8
              local.tee $p1
              i32.gt_u
              drop
              local.get $l8
              local.get $p1
              i32.store offset=8
              local.get $p1
              local.get $l8
              i32.store offset=12
              br $B13
            end
            block $B15
              local.get $l7
              i32.const 20
              i32.add
              local.tee $p1
              i32.load
              local.tee $l5
              br_if $B15
              local.get $l7
              i32.const 16
              i32.add
              local.tee $p1
              i32.load
              local.tee $l5
              br_if $B15
              i32.const 0
              local.set $l8
              br $B13
            end
            loop $L16
              local.get $p1
              local.set $l11
              local.get $l5
              local.tee $l8
              i32.const 20
              i32.add
              local.tee $p1
              i32.load
              local.tee $l5
              br_if $L16
              local.get $l8
              i32.const 16
              i32.add
              local.set $p1
              local.get $l8
              i32.load offset=16
              local.tee $l5
              br_if $L16
            end
            local.get $l11
            i32.const 0
            i32.store
          end
          local.get $l12
          i32.eqz
          br_if $B10
          block $B17
            block $B18
              local.get $l7
              i32.load offset=28
              local.tee $l5
              i32.const 2
              i32.shl
              i32.const 1059612
              i32.add
              local.tee $p1
              i32.load
              local.get $l7
              i32.ne
              br_if $B18
              local.get $p1
              local.get $l8
              i32.store
              local.get $l8
              br_if $B17
              i32.const 0
              i32.const 0
              i32.load offset=1059312
              i32.const -2
              local.get $l5
              i32.rotl
              i32.and
              i32.store offset=1059312
              br $B10
            end
            local.get $l12
            i32.const 16
            i32.const 20
            local.get $l12
            i32.load offset=16
            local.get $l7
            i32.eq
            select
            i32.add
            local.get $l8
            i32.store
            local.get $l8
            i32.eqz
            br_if $B10
          end
          local.get $l8
          local.get $l12
          i32.store offset=24
          block $B19
            local.get $l7
            i32.load offset=16
            local.tee $p1
            i32.eqz
            br_if $B19
            local.get $l8
            local.get $p1
            i32.store offset=16
            local.get $p1
            local.get $l8
            i32.store offset=24
          end
          local.get $l7
          i32.load offset=20
          local.tee $p1
          i32.eqz
          br_if $B10
          local.get $l8
          i32.const 20
          i32.add
          local.get $p1
          i32.store
          local.get $p1
          local.get $l8
          i32.store offset=24
        end
        block $B20
          local.get $l10
          i32.const 15
          i32.gt_u
          br_if $B20
          local.get $l3
          local.get $l4
          i32.const 1
          i32.and
          local.get $l9
          i32.or
          i32.const 2
          i32.or
          i32.store
          local.get $l6
          local.get $l9
          i32.add
          local.tee $p1
          local.get $p1
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $p0
          return
        end
        local.get $l3
        local.get $l2
        local.get $l4
        i32.const 1
        i32.and
        i32.or
        i32.const 2
        i32.or
        i32.store
        local.get $l6
        local.get $l2
        i32.add
        local.tee $p1
        local.get $l10
        i32.const 3
        i32.or
        i32.store offset=4
        local.get $l6
        local.get $l9
        i32.add
        local.tee $l2
        local.get $l2
        i32.load offset=4
        i32.const 1
        i32.or
        i32.store offset=4
        local.get $p1
        local.get $l10
        call $dispose_chunk
        local.get $p0
        return
      end
      block $B21
        local.get $p1
        call $dlmalloc
        local.tee $l2
        br_if $B21
        i32.const 0
        return
      end
      local.get $l2
      local.get $p0
      i32.const -4
      i32.const -8
      local.get $l3
      i32.load
      local.tee $l5
      i32.const 3
      i32.and
      select
      local.get $l5
      i32.const -8
      i32.and
      i32.add
      local.tee $l5
      local.get $p1
      local.get $l5
      local.get $p1
      i32.lt_u
      select
      call $memcpy
      local.set $p1
      local.get $p0
      call $dlfree
      local.get $p1
      local.set $p0
    end
    local.get $p0)
  (func $dispose_chunk (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32)
    local.get $p0
    local.get $p1
    i32.add
    local.set $l2
    block $B0
      block $B1
        local.get $p0
        i32.load offset=4
        local.tee $l3
        i32.const 1
        i32.and
        br_if $B1
        local.get $l3
        i32.const 3
        i32.and
        i32.eqz
        br_if $B0
        local.get $p0
        i32.load
        local.tee $l3
        local.get $p1
        i32.add
        local.set $p1
        block $B2
          block $B3
            i32.const 0
            i32.load offset=1059328
            local.get $p0
            local.get $l3
            i32.sub
            local.tee $p0
            i32.eq
            br_if $B3
            block $B4
              local.get $l3
              i32.const 255
              i32.gt_u
              br_if $B4
              local.get $p0
              i32.load offset=8
              local.tee $l4
              local.get $l3
              i32.const 3
              i32.shr_u
              local.tee $l5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee $l6
              i32.eq
              drop
              local.get $p0
              i32.load offset=12
              local.tee $l3
              local.get $l4
              i32.ne
              br_if $B2
              i32.const 0
              i32.const 0
              i32.load offset=1059308
              i32.const -2
              local.get $l5
              i32.rotl
              i32.and
              i32.store offset=1059308
              br $B1
            end
            local.get $p0
            i32.load offset=24
            local.set $l7
            block $B5
              block $B6
                local.get $p0
                i32.load offset=12
                local.tee $l6
                local.get $p0
                i32.eq
                br_if $B6
                i32.const 0
                i32.load offset=1059324
                local.get $p0
                i32.load offset=8
                local.tee $l3
                i32.gt_u
                drop
                local.get $l6
                local.get $l3
                i32.store offset=8
                local.get $l3
                local.get $l6
                i32.store offset=12
                br $B5
              end
              block $B7
                local.get $p0
                i32.const 20
                i32.add
                local.tee $l3
                i32.load
                local.tee $l4
                br_if $B7
                local.get $p0
                i32.const 16
                i32.add
                local.tee $l3
                i32.load
                local.tee $l4
                br_if $B7
                i32.const 0
                local.set $l6
                br $B5
              end
              loop $L8
                local.get $l3
                local.set $l5
                local.get $l4
                local.tee $l6
                i32.const 20
                i32.add
                local.tee $l3
                i32.load
                local.tee $l4
                br_if $L8
                local.get $l6
                i32.const 16
                i32.add
                local.set $l3
                local.get $l6
                i32.load offset=16
                local.tee $l4
                br_if $L8
              end
              local.get $l5
              i32.const 0
              i32.store
            end
            local.get $l7
            i32.eqz
            br_if $B1
            block $B9
              block $B10
                local.get $p0
                i32.load offset=28
                local.tee $l4
                i32.const 2
                i32.shl
                i32.const 1059612
                i32.add
                local.tee $l3
                i32.load
                local.get $p0
                i32.ne
                br_if $B10
                local.get $l3
                local.get $l6
                i32.store
                local.get $l6
                br_if $B9
                i32.const 0
                i32.const 0
                i32.load offset=1059312
                i32.const -2
                local.get $l4
                i32.rotl
                i32.and
                i32.store offset=1059312
                br $B1
              end
              local.get $l7
              i32.const 16
              i32.const 20
              local.get $l7
              i32.load offset=16
              local.get $p0
              i32.eq
              select
              i32.add
              local.get $l6
              i32.store
              local.get $l6
              i32.eqz
              br_if $B1
            end
            local.get $l6
            local.get $l7
            i32.store offset=24
            block $B11
              local.get $p0
              i32.load offset=16
              local.tee $l3
              i32.eqz
              br_if $B11
              local.get $l6
              local.get $l3
              i32.store offset=16
              local.get $l3
              local.get $l6
              i32.store offset=24
            end
            local.get $p0
            i32.load offset=20
            local.tee $l3
            i32.eqz
            br_if $B1
            local.get $l6
            i32.const 20
            i32.add
            local.get $l3
            i32.store
            local.get $l3
            local.get $l6
            i32.store offset=24
            br $B1
          end
          local.get $l2
          i32.load offset=4
          local.tee $l3
          i32.const 3
          i32.and
          i32.const 3
          i32.ne
          br_if $B1
          local.get $l2
          local.get $l3
          i32.const -2
          i32.and
          i32.store offset=4
          i32.const 0
          local.get $p1
          i32.store offset=1059316
          local.get $l2
          local.get $p1
          i32.store
          local.get $p0
          local.get $p1
          i32.const 1
          i32.or
          i32.store offset=4
          return
        end
        local.get $l3
        local.get $l6
        i32.eq
        drop
        local.get $l3
        local.get $l4
        i32.store offset=8
        local.get $l4
        local.get $l3
        i32.store offset=12
      end
      block $B12
        block $B13
          local.get $l2
          i32.load offset=4
          local.tee $l3
          i32.const 2
          i32.and
          br_if $B13
          block $B14
            i32.const 0
            i32.load offset=1059332
            local.get $l2
            i32.ne
            br_if $B14
            i32.const 0
            local.get $p0
            i32.store offset=1059332
            i32.const 0
            i32.const 0
            i32.load offset=1059320
            local.get $p1
            i32.add
            local.tee $p1
            i32.store offset=1059320
            local.get $p0
            local.get $p1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $p0
            i32.const 0
            i32.load offset=1059328
            i32.ne
            br_if $B0
            i32.const 0
            i32.const 0
            i32.store offset=1059316
            i32.const 0
            i32.const 0
            i32.store offset=1059328
            return
          end
          block $B15
            i32.const 0
            i32.load offset=1059328
            local.get $l2
            i32.ne
            br_if $B15
            i32.const 0
            local.get $p0
            i32.store offset=1059328
            i32.const 0
            i32.const 0
            i32.load offset=1059316
            local.get $p1
            i32.add
            local.tee $p1
            i32.store offset=1059316
            local.get $p0
            local.get $p1
            i32.const 1
            i32.or
            i32.store offset=4
            local.get $p0
            local.get $p1
            i32.add
            local.get $p1
            i32.store
            return
          end
          local.get $l3
          i32.const -8
          i32.and
          local.get $p1
          i32.add
          local.set $p1
          block $B16
            block $B17
              local.get $l3
              i32.const 255
              i32.gt_u
              br_if $B17
              local.get $l2
              i32.load offset=8
              local.tee $l4
              local.get $l3
              i32.const 3
              i32.shr_u
              local.tee $l5
              i32.const 3
              i32.shl
              i32.const 1059348
              i32.add
              local.tee $l6
              i32.eq
              drop
              block $B18
                local.get $l2
                i32.load offset=12
                local.tee $l3
                local.get $l4
                i32.ne
                br_if $B18
                i32.const 0
                i32.const 0
                i32.load offset=1059308
                i32.const -2
                local.get $l5
                i32.rotl
                i32.and
                i32.store offset=1059308
                br $B16
              end
              local.get $l3
              local.get $l6
              i32.eq
              drop
              local.get $l3
              local.get $l4
              i32.store offset=8
              local.get $l4
              local.get $l3
              i32.store offset=12
              br $B16
            end
            local.get $l2
            i32.load offset=24
            local.set $l7
            block $B19
              block $B20
                local.get $l2
                i32.load offset=12
                local.tee $l6
                local.get $l2
                i32.eq
                br_if $B20
                i32.const 0
                i32.load offset=1059324
                local.get $l2
                i32.load offset=8
                local.tee $l3
                i32.gt_u
                drop
                local.get $l6
                local.get $l3
                i32.store offset=8
                local.get $l3
                local.get $l6
                i32.store offset=12
                br $B19
              end
              block $B21
                local.get $l2
                i32.const 20
                i32.add
                local.tee $l4
                i32.load
                local.tee $l3
                br_if $B21
                local.get $l2
                i32.const 16
                i32.add
                local.tee $l4
                i32.load
                local.tee $l3
                br_if $B21
                i32.const 0
                local.set $l6
                br $B19
              end
              loop $L22
                local.get $l4
                local.set $l5
                local.get $l3
                local.tee $l6
                i32.const 20
                i32.add
                local.tee $l4
                i32.load
                local.tee $l3
                br_if $L22
                local.get $l6
                i32.const 16
                i32.add
                local.set $l4
                local.get $l6
                i32.load offset=16
                local.tee $l3
                br_if $L22
              end
              local.get $l5
              i32.const 0
              i32.store
            end
            local.get $l7
            i32.eqz
            br_if $B16
            block $B23
              block $B24
                local.get $l2
                i32.load offset=28
                local.tee $l4
                i32.const 2
                i32.shl
                i32.const 1059612
                i32.add
                local.tee $l3
                i32.load
                local.get $l2
                i32.ne
                br_if $B24
                local.get $l3
                local.get $l6
                i32.store
                local.get $l6
                br_if $B23
                i32.const 0
                i32.const 0
                i32.load offset=1059312
                i32.const -2
                local.get $l4
                i32.rotl
                i32.and
                i32.store offset=1059312
                br $B16
              end
              local.get $l7
              i32.const 16
              i32.const 20
              local.get $l7
              i32.load offset=16
              local.get $l2
              i32.eq
              select
              i32.add
              local.get $l6
              i32.store
              local.get $l6
              i32.eqz
              br_if $B16
            end
            local.get $l6
            local.get $l7
            i32.store offset=24
            block $B25
              local.get $l2
              i32.load offset=16
              local.tee $l3
              i32.eqz
              br_if $B25
              local.get $l6
              local.get $l3
              i32.store offset=16
              local.get $l3
              local.get $l6
              i32.store offset=24
            end
            local.get $l2
            i32.load offset=20
            local.tee $l3
            i32.eqz
            br_if $B16
            local.get $l6
            i32.const 20
            i32.add
            local.get $l3
            i32.store
            local.get $l3
            local.get $l6
            i32.store offset=24
          end
          local.get $p0
          local.get $p1
          i32.add
          local.get $p1
          i32.store
          local.get $p0
          local.get $p1
          i32.const 1
          i32.or
          i32.store offset=4
          local.get $p0
          i32.const 0
          i32.load offset=1059328
          i32.ne
          br_if $B12
          i32.const 0
          local.get $p1
          i32.store offset=1059316
          return
        end
        local.get $l2
        local.get $l3
        i32.const -2
        i32.and
        i32.store offset=4
        local.get $p0
        local.get $p1
        i32.add
        local.get $p1
        i32.store
        local.get $p0
        local.get $p1
        i32.const 1
        i32.or
        i32.store offset=4
      end
      block $B26
        local.get $p1
        i32.const 255
        i32.gt_u
        br_if $B26
        local.get $p1
        i32.const 3
        i32.shr_u
        local.tee $l3
        i32.const 3
        i32.shl
        i32.const 1059348
        i32.add
        local.set $p1
        block $B27
          block $B28
            i32.const 0
            i32.load offset=1059308
            local.tee $l4
            i32.const 1
            local.get $l3
            i32.shl
            local.tee $l3
            i32.and
            br_if $B28
            i32.const 0
            local.get $l4
            local.get $l3
            i32.or
            i32.store offset=1059308
            local.get $p1
            local.set $l3
            br $B27
          end
          local.get $p1
          i32.load offset=8
          local.set $l3
        end
        local.get $l3
        local.get $p0
        i32.store offset=12
        local.get $p1
        local.get $p0
        i32.store offset=8
        local.get $p0
        local.get $p1
        i32.store offset=12
        local.get $p0
        local.get $l3
        i32.store offset=8
        return
      end
      i32.const 31
      local.set $l3
      block $B29
        local.get $p1
        i32.const 16777215
        i32.gt_u
        br_if $B29
        local.get $p1
        i32.const 8
        i32.shr_u
        local.tee $l3
        local.get $l3
        i32.const 1048320
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 8
        i32.and
        local.tee $l3
        i32.shl
        local.tee $l4
        local.get $l4
        i32.const 520192
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 4
        i32.and
        local.tee $l4
        i32.shl
        local.tee $l6
        local.get $l6
        i32.const 245760
        i32.add
        i32.const 16
        i32.shr_u
        i32.const 2
        i32.and
        local.tee $l6
        i32.shl
        i32.const 15
        i32.shr_u
        local.get $l3
        local.get $l4
        i32.or
        local.get $l6
        i32.or
        i32.sub
        local.tee $l3
        i32.const 1
        i32.shl
        local.get $p1
        local.get $l3
        i32.const 21
        i32.add
        i32.shr_u
        i32.const 1
        i32.and
        i32.or
        i32.const 28
        i32.add
        local.set $l3
      end
      local.get $p0
      i64.const 0
      i64.store offset=16 align=4
      local.get $p0
      i32.const 28
      i32.add
      local.get $l3
      i32.store
      local.get $l3
      i32.const 2
      i32.shl
      i32.const 1059612
      i32.add
      local.set $l4
      block $B30
        i32.const 0
        i32.load offset=1059312
        local.tee $l6
        i32.const 1
        local.get $l3
        i32.shl
        local.tee $l2
        i32.and
        br_if $B30
        local.get $l4
        local.get $p0
        i32.store
        i32.const 0
        local.get $l6
        local.get $l2
        i32.or
        i32.store offset=1059312
        local.get $p0
        i32.const 24
        i32.add
        local.get $l4
        i32.store
        local.get $p0
        local.get $p0
        i32.store offset=8
        local.get $p0
        local.get $p0
        i32.store offset=12
        return
      end
      local.get $p1
      i32.const 0
      i32.const 25
      local.get $l3
      i32.const 1
      i32.shr_u
      i32.sub
      local.get $l3
      i32.const 31
      i32.eq
      select
      i32.shl
      local.set $l3
      local.get $l4
      i32.load
      local.set $l6
      block $B31
        loop $L32
          local.get $l6
          local.tee $l4
          i32.load offset=4
          i32.const -8
          i32.and
          local.get $p1
          i32.eq
          br_if $B31
          local.get $l3
          i32.const 29
          i32.shr_u
          local.set $l6
          local.get $l3
          i32.const 1
          i32.shl
          local.set $l3
          local.get $l4
          local.get $l6
          i32.const 4
          i32.and
          i32.add
          i32.const 16
          i32.add
          local.tee $l2
          i32.load
          local.tee $l6
          br_if $L32
        end
        local.get $l2
        local.get $p0
        i32.store
        local.get $p0
        i32.const 24
        i32.add
        local.get $l4
        i32.store
        local.get $p0
        local.get $p0
        i32.store offset=12
        local.get $p0
        local.get $p0
        i32.store offset=8
        return
      end
      local.get $l4
      i32.load offset=8
      local.tee $p1
      local.get $p0
      i32.store offset=12
      local.get $l4
      local.get $p0
      i32.store offset=8
      local.get $p0
      i32.const 24
      i32.add
      i32.const 0
      i32.store
      local.get $p0
      local.get $l4
      i32.store offset=12
      local.get $p0
      local.get $p1
      i32.store offset=8
    end)
  (func $internal_memalign (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    block $B0
      block $B1
        local.get $p0
        i32.const 16
        local.get $p0
        i32.const 16
        i32.gt_u
        select
        local.tee $l2
        local.get $l2
        i32.const -1
        i32.add
        i32.and
        br_if $B1
        local.get $l2
        local.set $p0
        br $B0
      end
      i32.const 32
      local.set $l3
      loop $L2
        local.get $l3
        local.tee $p0
        i32.const 1
        i32.shl
        local.set $l3
        local.get $p0
        local.get $l2
        i32.lt_u
        br_if $L2
      end
    end
    block $B3
      i32.const -64
      local.get $p0
      i32.sub
      local.get $p1
      i32.gt_u
      br_if $B3
      i32.const 0
      i32.const 48
      i32.store offset=1059804
      i32.const 0
      return
    end
    block $B4
      local.get $p0
      i32.const 16
      local.get $p1
      i32.const 19
      i32.add
      i32.const -16
      i32.and
      local.get $p1
      i32.const 11
      i32.lt_u
      select
      local.tee $p1
      i32.add
      i32.const 12
      i32.add
      call $dlmalloc
      local.tee $l3
      br_if $B4
      i32.const 0
      return
    end
    local.get $l3
    i32.const -8
    i32.add
    local.set $l2
    block $B5
      block $B6
        local.get $p0
        i32.const -1
        i32.add
        local.get $l3
        i32.and
        br_if $B6
        local.get $l2
        local.set $p0
        br $B5
      end
      local.get $l3
      i32.const -4
      i32.add
      local.tee $l4
      i32.load
      local.tee $l5
      i32.const -8
      i32.and
      local.get $l3
      local.get $p0
      i32.add
      i32.const -1
      i32.add
      i32.const 0
      local.get $p0
      i32.sub
      i32.and
      i32.const -8
      i32.add
      local.tee $l3
      i32.const 0
      local.get $p0
      local.get $l3
      local.get $l2
      i32.sub
      i32.const 15
      i32.gt_u
      select
      i32.add
      local.tee $p0
      local.get $l2
      i32.sub
      local.tee $l3
      i32.sub
      local.set $l6
      block $B7
        local.get $l5
        i32.const 3
        i32.and
        br_if $B7
        local.get $p0
        local.get $l6
        i32.store offset=4
        local.get $p0
        local.get $l2
        i32.load
        local.get $l3
        i32.add
        i32.store
        br $B5
      end
      local.get $p0
      local.get $l6
      local.get $p0
      i32.load offset=4
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store offset=4
      local.get $p0
      local.get $l6
      i32.add
      local.tee $l6
      local.get $l6
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $l4
      local.get $l3
      local.get $l4
      i32.load
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store
      local.get $l2
      local.get $l3
      i32.add
      local.tee $l6
      local.get $l6
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $l2
      local.get $l3
      call $dispose_chunk
    end
    block $B8
      local.get $p0
      i32.load offset=4
      local.tee $l3
      i32.const 3
      i32.and
      i32.eqz
      br_if $B8
      local.get $l3
      i32.const -8
      i32.and
      local.tee $l2
      local.get $p1
      i32.const 16
      i32.add
      i32.le_u
      br_if $B8
      local.get $p0
      local.get $p1
      local.get $l3
      i32.const 1
      i32.and
      i32.or
      i32.const 2
      i32.or
      i32.store offset=4
      local.get $p0
      local.get $p1
      i32.add
      local.tee $l3
      local.get $l2
      local.get $p1
      i32.sub
      local.tee $p1
      i32.const 3
      i32.or
      i32.store offset=4
      local.get $p0
      local.get $l2
      i32.add
      local.tee $l2
      local.get $l2
      i32.load offset=4
      i32.const 1
      i32.or
      i32.store offset=4
      local.get $l3
      local.get $p1
      call $dispose_chunk
    end
    local.get $p0
    i32.const 8
    i32.add)
  (func $aligned_alloc (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    block $B0
      local.get $p0
      i32.const 16
      i32.gt_u
      br_if $B0
      local.get $p1
      call $dlmalloc
      return
    end
    local.get $p0
    local.get $p1
    call $internal_memalign)
  (func $_Exit (type $t0) (param $p0 i32)
    local.get $p0
    call $__wasi_proc_exit
    unreachable)
  (func $sbrk (type $t10) (param $p0 i32) (result i32)
    block $B0
      local.get $p0
      br_if $B0
      memory.size
      i32.const 16
      i32.shl
      return
    end
    block $B1
      local.get $p0
      i32.const 65535
      i32.and
      br_if $B1
      local.get $p0
      i32.const -1
      i32.le_s
      br_if $B1
      block $B2
        local.get $p0
        i32.const 16
        i32.shr_u
        memory.grow
        local.tee $p0
        i32.const -1
        i32.ne
        br_if $B2
        i32.const 0
        i32.const 48
        i32.store offset=1059804
        i32.const -1
        return
      end
      local.get $p0
      i32.const 16
      i32.shl
      return
    end
    call $abort
    unreachable)
  (func $__wasilibc_ensure_environ (type $t7)
    block $B0
      i32.const 0
      i32.load offset=1059812
      i32.const -1
      i32.ne
      br_if $B0
      call $__wasilibc_initialize_environ
    end)
  (func $__wasilibc_initialize_environ (type $t7)
    (local $l0 i32) (local $l1 i32) (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l0
    global.set $g0
    block $B0
      block $B1
        block $B2
          local.get $l0
          i32.const 12
          i32.add
          local.get $l0
          i32.const 8
          i32.add
          call $__wasi_environ_sizes_get
          br_if $B2
          block $B3
            local.get $l0
            i32.load offset=12
            local.tee $l1
            br_if $B3
            i32.const 0
            i32.const 1059808
            i32.store offset=1059812
            br $B0
          end
          block $B4
            block $B5
              local.get $l1
              i32.const 1
              i32.add
              local.tee $l2
              local.get $l1
              i32.lt_u
              br_if $B5
              local.get $l0
              i32.load offset=8
              call $malloc
              local.tee $l3
              i32.eqz
              br_if $B5
              local.get $l2
              i32.const 4
              call $calloc
              local.tee $l1
              br_if $B4
              local.get $l3
              call $free
            end
            i32.const 70
            call $_Exit
            unreachable
          end
          local.get $l1
          local.get $l3
          call $__wasi_environ_get
          i32.eqz
          br_if $B1
          local.get $l3
          call $free
          local.get $l1
          call $free
        end
        i32.const 71
        call $_Exit
        unreachable
      end
      i32.const 0
      local.get $l1
      i32.store offset=1059812
    end
    local.get $l0
    i32.const 16
    i32.add
    global.set $g0)
  (func $__wasilibc_initialize_environ_eagerly (type $t7)
    call $__wasilibc_initialize_environ)
  (func $abort (type $t7)
    unreachable
    unreachable)
  (func $getcwd (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    i32.const 0
    i32.load offset=1059212
    local.set $l2
    block $B0
      block $B1
        local.get $p0
        br_if $B1
        local.get $l2
        call $strdup
        local.tee $p0
        br_if $B0
        i32.const 0
        i32.const 48
        i32.store offset=1059804
        i32.const 0
        return
      end
      block $B2
        local.get $l2
        call $strlen
        i32.const 1
        i32.add
        local.get $p1
        i32.gt_u
        br_if $B2
        local.get $p0
        local.get $l2
        call $strcpy
        return
      end
      i32.const 0
      local.set $p0
      i32.const 0
      i32.const 68
      i32.store offset=1059804
    end
    local.get $p0)
  (func $__wasi_environ_get (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    local.get $p1
    call $__imported_wasi_snapshot_preview1_environ_get
    i32.const 65535
    i32.and)
  (func $__wasi_environ_sizes_get (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    local.get $p1
    call $__imported_wasi_snapshot_preview1_environ_sizes_get
    i32.const 65535
    i32.and)
  (func $__wasi_proc_exit (type $t0) (param $p0 i32)
    local.get $p0
    call $__imported_wasi_snapshot_preview1_proc_exit
    unreachable)
  (func $dummy (type $t7))
  (func $__wasm_call_dtors (type $t7)
    call $dummy
    call $dummy)
  (func $getenv (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32)
    call $__wasilibc_ensure_environ
    block $B0
      local.get $p0
      i32.const 61
      call $__strchrnul
      local.get $p0
      i32.sub
      local.tee $l1
      br_if $B0
      i32.const 0
      return
    end
    i32.const 0
    local.set $l2
    block $B1
      local.get $p0
      local.get $l1
      i32.add
      i32.load8_u
      br_if $B1
      i32.const 0
      i32.load offset=1059812
      local.tee $l3
      i32.eqz
      br_if $B1
      local.get $l3
      i32.load
      local.tee $l4
      i32.eqz
      br_if $B1
      local.get $l3
      i32.const 4
      i32.add
      local.set $l3
      block $B2
        loop $L3
          block $B4
            local.get $p0
            local.get $l4
            local.get $l1
            call $strncmp
            br_if $B4
            local.get $l4
            local.get $l1
            i32.add
            local.tee $l4
            i32.load8_u
            i32.const 61
            i32.eq
            br_if $B2
          end
          local.get $l3
          i32.load
          local.set $l4
          local.get $l3
          i32.const 4
          i32.add
          local.set $l3
          local.get $l4
          br_if $L3
          br $B1
        end
      end
      local.get $l4
      i32.const 1
      i32.add
      local.set $l2
    end
    local.get $l2)
  (func $strdup (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32)
    block $B0
      local.get $p0
      call $strlen
      i32.const 1
      i32.add
      local.tee $l1
      call $malloc
      local.tee $l2
      i32.eqz
      br_if $B0
      local.get $l2
      local.get $p0
      local.get $l1
      call $memcpy
      drop
    end
    local.get $l2)
  (func $memmove (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32)
    block $B0
      local.get $p0
      local.get $p1
      i32.eq
      br_if $B0
      block $B1
        local.get $p1
        local.get $p0
        local.get $p2
        i32.add
        local.tee $l3
        i32.sub
        i32.const 0
        local.get $p2
        i32.const 1
        i32.shl
        i32.sub
        i32.gt_u
        br_if $B1
        local.get $p0
        local.get $p1
        local.get $p2
        call $memcpy
        drop
        br $B0
      end
      local.get $p1
      local.get $p0
      i32.xor
      i32.const 3
      i32.and
      local.set $l4
      block $B2
        block $B3
          block $B4
            local.get $p0
            local.get $p1
            i32.ge_u
            br_if $B4
            block $B5
              local.get $l4
              i32.eqz
              br_if $B5
              local.get $p2
              local.set $l4
              local.get $p0
              local.set $l3
              br $B2
            end
            block $B6
              local.get $p0
              i32.const 3
              i32.and
              br_if $B6
              local.get $p2
              local.set $l4
              local.get $p0
              local.set $l3
              br $B3
            end
            local.get $p2
            i32.eqz
            br_if $B0
            local.get $p0
            local.get $p1
            i32.load8_u
            i32.store8
            local.get $p2
            i32.const -1
            i32.add
            local.set $l4
            block $B7
              local.get $p0
              i32.const 1
              i32.add
              local.tee $l3
              i32.const 3
              i32.and
              br_if $B7
              local.get $p1
              i32.const 1
              i32.add
              local.set $p1
              br $B3
            end
            local.get $l4
            i32.eqz
            br_if $B0
            local.get $p0
            local.get $p1
            i32.load8_u offset=1
            i32.store8 offset=1
            local.get $p2
            i32.const -2
            i32.add
            local.set $l4
            block $B8
              local.get $p0
              i32.const 2
              i32.add
              local.tee $l3
              i32.const 3
              i32.and
              br_if $B8
              local.get $p1
              i32.const 2
              i32.add
              local.set $p1
              br $B3
            end
            local.get $l4
            i32.eqz
            br_if $B0
            local.get $p0
            local.get $p1
            i32.load8_u offset=2
            i32.store8 offset=2
            local.get $p2
            i32.const -3
            i32.add
            local.set $l4
            block $B9
              local.get $p0
              i32.const 3
              i32.add
              local.tee $l3
              i32.const 3
              i32.and
              br_if $B9
              local.get $p1
              i32.const 3
              i32.add
              local.set $p1
              br $B3
            end
            local.get $l4
            i32.eqz
            br_if $B0
            local.get $p0
            local.get $p1
            i32.load8_u offset=3
            i32.store8 offset=3
            local.get $p0
            i32.const 4
            i32.add
            local.set $l3
            local.get $p1
            i32.const 4
            i32.add
            local.set $p1
            local.get $p2
            i32.const -4
            i32.add
            local.set $l4
            br $B3
          end
          block $B10
            local.get $l4
            br_if $B10
            block $B11
              local.get $l3
              i32.const 3
              i32.and
              i32.eqz
              br_if $B11
              local.get $p2
              i32.eqz
              br_if $B0
              local.get $p0
              local.get $p2
              i32.const -1
              i32.add
              local.tee $l3
              i32.add
              local.tee $l4
              local.get $p1
              local.get $l3
              i32.add
              i32.load8_u
              i32.store8
              block $B12
                local.get $l4
                i32.const 3
                i32.and
                br_if $B12
                local.get $l3
                local.set $p2
                br $B11
              end
              local.get $l3
              i32.eqz
              br_if $B0
              local.get $p0
              local.get $p2
              i32.const -2
              i32.add
              local.tee $l3
              i32.add
              local.tee $l4
              local.get $p1
              local.get $l3
              i32.add
              i32.load8_u
              i32.store8
              block $B13
                local.get $l4
                i32.const 3
                i32.and
                br_if $B13
                local.get $l3
                local.set $p2
                br $B11
              end
              local.get $l3
              i32.eqz
              br_if $B0
              local.get $p0
              local.get $p2
              i32.const -3
              i32.add
              local.tee $l3
              i32.add
              local.tee $l4
              local.get $p1
              local.get $l3
              i32.add
              i32.load8_u
              i32.store8
              block $B14
                local.get $l4
                i32.const 3
                i32.and
                br_if $B14
                local.get $l3
                local.set $p2
                br $B11
              end
              local.get $l3
              i32.eqz
              br_if $B0
              local.get $p0
              local.get $p2
              i32.const -4
              i32.add
              local.tee $p2
              i32.add
              local.get $p1
              local.get $p2
              i32.add
              i32.load8_u
              i32.store8
            end
            local.get $p2
            i32.const 4
            i32.lt_u
            br_if $B10
            block $B15
              local.get $p2
              i32.const -4
              i32.add
              local.tee $l5
              i32.const 2
              i32.shr_u
              i32.const 1
              i32.add
              i32.const 3
              i32.and
              local.tee $l3
              i32.eqz
              br_if $B15
              local.get $p1
              i32.const -4
              i32.add
              local.set $l4
              local.get $p0
              i32.const -4
              i32.add
              local.set $l6
              loop $L16
                local.get $l6
                local.get $p2
                i32.add
                local.get $l4
                local.get $p2
                i32.add
                i32.load
                i32.store
                local.get $p2
                i32.const -4
                i32.add
                local.set $p2
                local.get $l3
                i32.const -1
                i32.add
                local.tee $l3
                br_if $L16
              end
            end
            local.get $l5
            i32.const 12
            i32.lt_u
            br_if $B10
            local.get $p1
            i32.const -16
            i32.add
            local.set $l6
            local.get $p0
            i32.const -16
            i32.add
            local.set $l5
            loop $L17
              local.get $l5
              local.get $p2
              i32.add
              local.tee $l3
              i32.const 12
              i32.add
              local.get $l6
              local.get $p2
              i32.add
              local.tee $l4
              i32.const 12
              i32.add
              i32.load
              i32.store
              local.get $l3
              i32.const 8
              i32.add
              local.get $l4
              i32.const 8
              i32.add
              i32.load
              i32.store
              local.get $l3
              i32.const 4
              i32.add
              local.get $l4
              i32.const 4
              i32.add
              i32.load
              i32.store
              local.get $l3
              local.get $l4
              i32.load
              i32.store
              local.get $p2
              i32.const -16
              i32.add
              local.tee $p2
              i32.const 3
              i32.gt_u
              br_if $L17
            end
          end
          local.get $p2
          i32.eqz
          br_if $B0
          local.get $p2
          i32.const -1
          i32.add
          local.set $l5
          block $B18
            local.get $p2
            i32.const 3
            i32.and
            local.tee $l3
            i32.eqz
            br_if $B18
            local.get $p1
            i32.const -1
            i32.add
            local.set $l4
            local.get $p0
            i32.const -1
            i32.add
            local.set $l6
            loop $L19
              local.get $l6
              local.get $p2
              i32.add
              local.get $l4
              local.get $p2
              i32.add
              i32.load8_u
              i32.store8
              local.get $p2
              i32.const -1
              i32.add
              local.set $p2
              local.get $l3
              i32.const -1
              i32.add
              local.tee $l3
              br_if $L19
            end
          end
          local.get $l5
          i32.const 3
          i32.lt_u
          br_if $B0
          local.get $p1
          i32.const -4
          i32.add
          local.set $l4
          local.get $p0
          i32.const -4
          i32.add
          local.set $l6
          loop $L20
            local.get $l6
            local.get $p2
            i32.add
            local.tee $p1
            i32.const 3
            i32.add
            local.get $l4
            local.get $p2
            i32.add
            local.tee $l3
            i32.const 3
            i32.add
            i32.load8_u
            i32.store8
            local.get $p1
            i32.const 2
            i32.add
            local.get $l3
            i32.const 2
            i32.add
            i32.load8_u
            i32.store8
            local.get $p1
            i32.const 1
            i32.add
            local.get $l3
            i32.const 1
            i32.add
            i32.load8_u
            i32.store8
            local.get $p1
            local.get $l3
            i32.load8_u
            i32.store8
            local.get $p2
            i32.const -4
            i32.add
            local.tee $p2
            br_if $L20
            br $B0
          end
        end
        local.get $l4
        i32.const 4
        i32.lt_u
        br_if $B2
        block $B21
          local.get $l4
          i32.const -4
          i32.add
          local.tee $l6
          i32.const 2
          i32.shr_u
          i32.const 1
          i32.add
          i32.const 7
          i32.and
          local.tee $p2
          i32.eqz
          br_if $B21
          loop $L22
            local.get $l3
            local.get $p1
            i32.load
            i32.store
            local.get $p1
            i32.const 4
            i32.add
            local.set $p1
            local.get $l3
            i32.const 4
            i32.add
            local.set $l3
            local.get $l4
            i32.const -4
            i32.add
            local.set $l4
            local.get $p2
            i32.const -1
            i32.add
            local.tee $p2
            br_if $L22
          end
        end
        local.get $l6
        i32.const 28
        i32.lt_u
        br_if $B2
        loop $L23
          local.get $l3
          local.get $p1
          i32.load
          i32.store
          local.get $l3
          i32.const 4
          i32.add
          local.get $p1
          i32.const 4
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 8
          i32.add
          local.get $p1
          i32.const 8
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 12
          i32.add
          local.get $p1
          i32.const 12
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 16
          i32.add
          local.get $p1
          i32.const 16
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 20
          i32.add
          local.get $p1
          i32.const 20
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 24
          i32.add
          local.get $p1
          i32.const 24
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 28
          i32.add
          local.get $p1
          i32.const 28
          i32.add
          i32.load
          i32.store
          local.get $l3
          i32.const 32
          i32.add
          local.set $l3
          local.get $p1
          i32.const 32
          i32.add
          local.set $p1
          local.get $l4
          i32.const -32
          i32.add
          local.tee $l4
          i32.const 3
          i32.gt_u
          br_if $L23
        end
      end
      local.get $l4
      i32.eqz
      br_if $B0
      local.get $l4
      i32.const -1
      i32.add
      local.set $l6
      block $B24
        local.get $l4
        i32.const 7
        i32.and
        local.tee $p2
        i32.eqz
        br_if $B24
        loop $L25
          local.get $l3
          local.get $p1
          i32.load8_u
          i32.store8
          local.get $l4
          i32.const -1
          i32.add
          local.set $l4
          local.get $l3
          i32.const 1
          i32.add
          local.set $l3
          local.get $p1
          i32.const 1
          i32.add
          local.set $p1
          local.get $p2
          i32.const -1
          i32.add
          local.tee $p2
          br_if $L25
        end
      end
      local.get $l6
      i32.const 7
      i32.lt_u
      br_if $B0
      loop $L26
        local.get $l3
        local.get $p1
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 1
        i32.add
        local.get $p1
        i32.const 1
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 2
        i32.add
        local.get $p1
        i32.const 2
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 3
        i32.add
        local.get $p1
        i32.const 3
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 4
        i32.add
        local.get $p1
        i32.const 4
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 5
        i32.add
        local.get $p1
        i32.const 5
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 6
        i32.add
        local.get $p1
        i32.const 6
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 7
        i32.add
        local.get $p1
        i32.const 7
        i32.add
        i32.load8_u
        i32.store8
        local.get $l3
        i32.const 8
        i32.add
        local.set $l3
        local.get $p1
        i32.const 8
        i32.add
        local.set $p1
        local.get $l4
        i32.const -8
        i32.add
        local.tee $l4
        br_if $L26
      end
    end
    local.get $p0)
  (func $__strchrnul (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 255
            i32.and
            local.tee $l2
            i32.eqz
            br_if $B3
            local.get $p0
            i32.const 3
            i32.and
            i32.eqz
            br_if $B1
            block $B4
              local.get $p0
              i32.load8_u
              local.tee $l3
              br_if $B4
              local.get $p0
              return
            end
            local.get $l3
            local.get $p1
            i32.const 255
            i32.and
            i32.ne
            br_if $B2
            local.get $p0
            return
          end
          local.get $p0
          local.get $p0
          call $strlen
          i32.add
          return
        end
        block $B5
          local.get $p0
          i32.const 1
          i32.add
          local.tee $l3
          i32.const 3
          i32.and
          br_if $B5
          local.get $l3
          local.set $p0
          br $B1
        end
        local.get $l3
        i32.load8_u
        local.tee $l4
        i32.eqz
        br_if $B0
        local.get $l4
        local.get $p1
        i32.const 255
        i32.and
        i32.eq
        br_if $B0
        block $B6
          local.get $p0
          i32.const 2
          i32.add
          local.tee $l3
          i32.const 3
          i32.and
          br_if $B6
          local.get $l3
          local.set $p0
          br $B1
        end
        local.get $l3
        i32.load8_u
        local.tee $l4
        i32.eqz
        br_if $B0
        local.get $l4
        local.get $p1
        i32.const 255
        i32.and
        i32.eq
        br_if $B0
        block $B7
          local.get $p0
          i32.const 3
          i32.add
          local.tee $l3
          i32.const 3
          i32.and
          br_if $B7
          local.get $l3
          local.set $p0
          br $B1
        end
        local.get $l3
        i32.load8_u
        local.tee $l4
        i32.eqz
        br_if $B0
        local.get $l4
        local.get $p1
        i32.const 255
        i32.and
        i32.eq
        br_if $B0
        local.get $p0
        i32.const 4
        i32.add
        local.set $p0
      end
      block $B8
        local.get $p0
        i32.load
        local.tee $l3
        i32.const -1
        i32.xor
        local.get $l3
        i32.const -16843009
        i32.add
        i32.and
        i32.const -2139062144
        i32.and
        br_if $B8
        local.get $l2
        i32.const 16843009
        i32.mul
        local.set $l2
        loop $L9
          local.get $l3
          local.get $l2
          i32.xor
          local.tee $l3
          i32.const -1
          i32.xor
          local.get $l3
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          br_if $B8
          local.get $p0
          i32.const 4
          i32.add
          local.tee $p0
          i32.load
          local.tee $l3
          i32.const -1
          i32.xor
          local.get $l3
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          i32.eqz
          br_if $L9
        end
      end
      local.get $p0
      i32.const -1
      i32.add
      local.set $l3
      loop $L10
        local.get $l3
        i32.const 1
        i32.add
        local.tee $l3
        i32.load8_u
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $p0
        local.get $p1
        i32.const 255
        i32.and
        i32.ne
        br_if $L10
      end
    end
    local.get $l3)
  (func $memcpy (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    block $B0
      block $B1
        local.get $p1
        i32.const 3
        i32.and
        i32.eqz
        br_if $B1
        local.get $p2
        i32.eqz
        br_if $B1
        local.get $p0
        local.get $p1
        i32.load8_u
        i32.store8
        local.get $p2
        i32.const -1
        i32.add
        local.set $l3
        local.get $p0
        i32.const 1
        i32.add
        local.set $l4
        local.get $p1
        i32.const 1
        i32.add
        local.tee $l5
        i32.const 3
        i32.and
        i32.eqz
        br_if $B0
        local.get $l3
        i32.eqz
        br_if $B0
        local.get $p0
        local.get $p1
        i32.load8_u offset=1
        i32.store8 offset=1
        local.get $p2
        i32.const -2
        i32.add
        local.set $l3
        local.get $p0
        i32.const 2
        i32.add
        local.set $l4
        local.get $p1
        i32.const 2
        i32.add
        local.tee $l5
        i32.const 3
        i32.and
        i32.eqz
        br_if $B0
        local.get $l3
        i32.eqz
        br_if $B0
        local.get $p0
        local.get $p1
        i32.load8_u offset=2
        i32.store8 offset=2
        local.get $p2
        i32.const -3
        i32.add
        local.set $l3
        local.get $p0
        i32.const 3
        i32.add
        local.set $l4
        local.get $p1
        i32.const 3
        i32.add
        local.tee $l5
        i32.const 3
        i32.and
        i32.eqz
        br_if $B0
        local.get $l3
        i32.eqz
        br_if $B0
        local.get $p0
        local.get $p1
        i32.load8_u offset=3
        i32.store8 offset=3
        local.get $p2
        i32.const -4
        i32.add
        local.set $l3
        local.get $p0
        i32.const 4
        i32.add
        local.set $l4
        local.get $p1
        i32.const 4
        i32.add
        local.set $l5
        br $B0
      end
      local.get $p2
      local.set $l3
      local.get $p0
      local.set $l4
      local.get $p1
      local.set $l5
    end
    block $B2
      block $B3
        block $B4
          local.get $l4
          i32.const 3
          i32.and
          local.tee $p1
          br_if $B4
          block $B5
            block $B6
              local.get $l3
              i32.const 16
              i32.lt_u
              br_if $B6
              block $B7
                local.get $l3
                i32.const -16
                i32.add
                local.tee $p1
                i32.const 16
                i32.and
                br_if $B7
                local.get $l4
                local.get $l5
                i64.load align=4
                i64.store align=4
                local.get $l4
                local.get $l5
                i64.load offset=8 align=4
                i64.store offset=8 align=4
                local.get $l4
                i32.const 16
                i32.add
                local.set $l4
                local.get $l5
                i32.const 16
                i32.add
                local.set $l5
                local.get $p1
                local.set $l3
              end
              local.get $p1
              i32.const 16
              i32.lt_u
              br_if $B5
              loop $L8
                local.get $l4
                local.get $l5
                i64.load align=4
                i64.store align=4
                local.get $l4
                i32.const 8
                i32.add
                local.get $l5
                i32.const 8
                i32.add
                i64.load align=4
                i64.store align=4
                local.get $l4
                i32.const 16
                i32.add
                local.get $l5
                i32.const 16
                i32.add
                i64.load align=4
                i64.store align=4
                local.get $l4
                i32.const 24
                i32.add
                local.get $l5
                i32.const 24
                i32.add
                i64.load align=4
                i64.store align=4
                local.get $l4
                i32.const 32
                i32.add
                local.set $l4
                local.get $l5
                i32.const 32
                i32.add
                local.set $l5
                local.get $l3
                i32.const -32
                i32.add
                local.tee $l3
                i32.const 15
                i32.gt_u
                br_if $L8
              end
            end
            local.get $l3
            local.set $p1
          end
          block $B9
            local.get $p1
            i32.const 8
            i32.and
            i32.eqz
            br_if $B9
            local.get $l4
            local.get $l5
            i64.load align=4
            i64.store align=4
            local.get $l5
            i32.const 8
            i32.add
            local.set $l5
            local.get $l4
            i32.const 8
            i32.add
            local.set $l4
          end
          block $B10
            local.get $p1
            i32.const 4
            i32.and
            i32.eqz
            br_if $B10
            local.get $l4
            local.get $l5
            i32.load
            i32.store
            local.get $l5
            i32.const 4
            i32.add
            local.set $l5
            local.get $l4
            i32.const 4
            i32.add
            local.set $l4
          end
          block $B11
            local.get $p1
            i32.const 2
            i32.and
            i32.eqz
            br_if $B11
            local.get $l4
            local.get $l5
            i32.load16_u align=1
            i32.store16 align=1
            local.get $l4
            i32.const 2
            i32.add
            local.set $l4
            local.get $l5
            i32.const 2
            i32.add
            local.set $l5
          end
          local.get $p1
          i32.const 1
          i32.and
          br_if $B3
          br $B2
        end
        block $B12
          local.get $l3
          i32.const 32
          i32.lt_u
          br_if $B12
          block $B13
            block $B14
              block $B15
                local.get $p1
                i32.const -1
                i32.add
                br_table $B15 $B14 $B13 $B12
              end
              local.get $l4
              local.get $l5
              i32.load
              local.tee $l6
              i32.store8
              local.get $l4
              local.get $l6
              i32.const 16
              i32.shr_u
              i32.store8 offset=2
              local.get $l4
              local.get $l6
              i32.const 8
              i32.shr_u
              i32.store8 offset=1
              local.get $l3
              i32.const -3
              i32.add
              local.set $l3
              local.get $l4
              i32.const 3
              i32.add
              local.set $l7
              i32.const 0
              local.set $p1
              loop $L16
                local.get $l7
                local.get $p1
                i32.add
                local.tee $l4
                local.get $l5
                local.get $p1
                i32.add
                local.tee $p2
                i32.const 4
                i32.add
                i32.load
                local.tee $l8
                i32.const 8
                i32.shl
                local.get $l6
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get $l4
                i32.const 4
                i32.add
                local.get $p2
                i32.const 8
                i32.add
                i32.load
                local.tee $l6
                i32.const 8
                i32.shl
                local.get $l8
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get $l4
                i32.const 8
                i32.add
                local.get $p2
                i32.const 12
                i32.add
                i32.load
                local.tee $l8
                i32.const 8
                i32.shl
                local.get $l6
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get $l4
                i32.const 12
                i32.add
                local.get $p2
                i32.const 16
                i32.add
                i32.load
                local.tee $l6
                i32.const 8
                i32.shl
                local.get $l8
                i32.const 24
                i32.shr_u
                i32.or
                i32.store
                local.get $p1
                i32.const 16
                i32.add
                local.set $p1
                local.get $l3
                i32.const -16
                i32.add
                local.tee $l3
                i32.const 16
                i32.gt_u
                br_if $L16
              end
              local.get $l7
              local.get $p1
              i32.add
              local.set $l4
              local.get $l5
              local.get $p1
              i32.add
              i32.const 3
              i32.add
              local.set $l5
              br $B12
            end
            local.get $l4
            local.get $l5
            i32.load
            local.tee $l6
            i32.store16 align=1
            local.get $l3
            i32.const -2
            i32.add
            local.set $l3
            local.get $l4
            i32.const 2
            i32.add
            local.set $l7
            i32.const 0
            local.set $p1
            loop $L17
              local.get $l7
              local.get $p1
              i32.add
              local.tee $l4
              local.get $l5
              local.get $p1
              i32.add
              local.tee $p2
              i32.const 4
              i32.add
              i32.load
              local.tee $l8
              i32.const 16
              i32.shl
              local.get $l6
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get $l4
              i32.const 4
              i32.add
              local.get $p2
              i32.const 8
              i32.add
              i32.load
              local.tee $l6
              i32.const 16
              i32.shl
              local.get $l8
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get $l4
              i32.const 8
              i32.add
              local.get $p2
              i32.const 12
              i32.add
              i32.load
              local.tee $l8
              i32.const 16
              i32.shl
              local.get $l6
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get $l4
              i32.const 12
              i32.add
              local.get $p2
              i32.const 16
              i32.add
              i32.load
              local.tee $l6
              i32.const 16
              i32.shl
              local.get $l8
              i32.const 16
              i32.shr_u
              i32.or
              i32.store
              local.get $p1
              i32.const 16
              i32.add
              local.set $p1
              local.get $l3
              i32.const -16
              i32.add
              local.tee $l3
              i32.const 17
              i32.gt_u
              br_if $L17
            end
            local.get $l7
            local.get $p1
            i32.add
            local.set $l4
            local.get $l5
            local.get $p1
            i32.add
            i32.const 2
            i32.add
            local.set $l5
            br $B12
          end
          local.get $l4
          local.get $l5
          i32.load
          local.tee $l6
          i32.store8
          local.get $l3
          i32.const -1
          i32.add
          local.set $l3
          local.get $l4
          i32.const 1
          i32.add
          local.set $l7
          i32.const 0
          local.set $p1
          loop $L18
            local.get $l7
            local.get $p1
            i32.add
            local.tee $l4
            local.get $l5
            local.get $p1
            i32.add
            local.tee $p2
            i32.const 4
            i32.add
            i32.load
            local.tee $l8
            i32.const 24
            i32.shl
            local.get $l6
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get $l4
            i32.const 4
            i32.add
            local.get $p2
            i32.const 8
            i32.add
            i32.load
            local.tee $l6
            i32.const 24
            i32.shl
            local.get $l8
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get $l4
            i32.const 8
            i32.add
            local.get $p2
            i32.const 12
            i32.add
            i32.load
            local.tee $l8
            i32.const 24
            i32.shl
            local.get $l6
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get $l4
            i32.const 12
            i32.add
            local.get $p2
            i32.const 16
            i32.add
            i32.load
            local.tee $l6
            i32.const 24
            i32.shl
            local.get $l8
            i32.const 8
            i32.shr_u
            i32.or
            i32.store
            local.get $p1
            i32.const 16
            i32.add
            local.set $p1
            local.get $l3
            i32.const -16
            i32.add
            local.tee $l3
            i32.const 18
            i32.gt_u
            br_if $L18
          end
          local.get $l7
          local.get $p1
          i32.add
          local.set $l4
          local.get $l5
          local.get $p1
          i32.add
          i32.const 1
          i32.add
          local.set $l5
        end
        block $B19
          local.get $l3
          i32.const 16
          i32.and
          i32.eqz
          br_if $B19
          local.get $l4
          local.get $l5
          i32.load8_u
          i32.store8
          local.get $l4
          local.get $l5
          i32.load offset=1 align=1
          i32.store offset=1 align=1
          local.get $l4
          local.get $l5
          i64.load offset=5 align=1
          i64.store offset=5 align=1
          local.get $l4
          local.get $l5
          i32.load16_u offset=13 align=1
          i32.store16 offset=13 align=1
          local.get $l4
          local.get $l5
          i32.load8_u offset=15
          i32.store8 offset=15
          local.get $l4
          i32.const 16
          i32.add
          local.set $l4
          local.get $l5
          i32.const 16
          i32.add
          local.set $l5
        end
        block $B20
          local.get $l3
          i32.const 8
          i32.and
          i32.eqz
          br_if $B20
          local.get $l4
          local.get $l5
          i64.load align=1
          i64.store align=1
          local.get $l4
          i32.const 8
          i32.add
          local.set $l4
          local.get $l5
          i32.const 8
          i32.add
          local.set $l5
        end
        block $B21
          local.get $l3
          i32.const 4
          i32.and
          i32.eqz
          br_if $B21
          local.get $l4
          local.get $l5
          i32.load align=1
          i32.store align=1
          local.get $l4
          i32.const 4
          i32.add
          local.set $l4
          local.get $l5
          i32.const 4
          i32.add
          local.set $l5
        end
        block $B22
          local.get $l3
          i32.const 2
          i32.and
          i32.eqz
          br_if $B22
          local.get $l4
          local.get $l5
          i32.load16_u align=1
          i32.store16 align=1
          local.get $l4
          i32.const 2
          i32.add
          local.set $l4
          local.get $l5
          i32.const 2
          i32.add
          local.set $l5
        end
        local.get $l3
        i32.const 1
        i32.and
        i32.eqz
        br_if $B2
      end
      local.get $l4
      local.get $l5
      i32.load8_u
      i32.store8
    end
    local.get $p0)
  (func $memset (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i64)
    block $B0
      local.get $p2
      i32.eqz
      br_if $B0
      local.get $p0
      local.get $p1
      i32.store8
      local.get $p2
      local.get $p0
      i32.add
      local.tee $l3
      i32.const -1
      i32.add
      local.get $p1
      i32.store8
      local.get $p2
      i32.const 3
      i32.lt_u
      br_if $B0
      local.get $p0
      local.get $p1
      i32.store8 offset=2
      local.get $p0
      local.get $p1
      i32.store8 offset=1
      local.get $l3
      i32.const -3
      i32.add
      local.get $p1
      i32.store8
      local.get $l3
      i32.const -2
      i32.add
      local.get $p1
      i32.store8
      local.get $p2
      i32.const 7
      i32.lt_u
      br_if $B0
      local.get $p0
      local.get $p1
      i32.store8 offset=3
      local.get $l3
      i32.const -4
      i32.add
      local.get $p1
      i32.store8
      local.get $p2
      i32.const 9
      i32.lt_u
      br_if $B0
      local.get $p0
      i32.const 0
      local.get $p0
      i32.sub
      i32.const 3
      i32.and
      local.tee $l4
      i32.add
      local.tee $l3
      local.get $p1
      i32.const 255
      i32.and
      i32.const 16843009
      i32.mul
      local.tee $p1
      i32.store
      local.get $l3
      local.get $p2
      local.get $l4
      i32.sub
      i32.const -4
      i32.and
      local.tee $l4
      i32.add
      local.tee $p2
      i32.const -4
      i32.add
      local.get $p1
      i32.store
      local.get $l4
      i32.const 9
      i32.lt_u
      br_if $B0
      local.get $l3
      local.get $p1
      i32.store offset=8
      local.get $l3
      local.get $p1
      i32.store offset=4
      local.get $p2
      i32.const -8
      i32.add
      local.get $p1
      i32.store
      local.get $p2
      i32.const -12
      i32.add
      local.get $p1
      i32.store
      local.get $l4
      i32.const 25
      i32.lt_u
      br_if $B0
      local.get $l3
      local.get $p1
      i32.store offset=24
      local.get $l3
      local.get $p1
      i32.store offset=20
      local.get $l3
      local.get $p1
      i32.store offset=16
      local.get $l3
      local.get $p1
      i32.store offset=12
      local.get $p2
      i32.const -16
      i32.add
      local.get $p1
      i32.store
      local.get $p2
      i32.const -20
      i32.add
      local.get $p1
      i32.store
      local.get $p2
      i32.const -24
      i32.add
      local.get $p1
      i32.store
      local.get $p2
      i32.const -28
      i32.add
      local.get $p1
      i32.store
      local.get $l4
      local.get $l3
      i32.const 4
      i32.and
      i32.const 24
      i32.or
      local.tee $l5
      i32.sub
      local.tee $p2
      i32.const 32
      i32.lt_u
      br_if $B0
      local.get $p1
      i64.extend_i32_u
      i64.const 4294967297
      i64.mul
      local.set $l6
      local.get $l3
      local.get $l5
      i32.add
      local.set $p1
      loop $L1
        local.get $p1
        local.get $l6
        i64.store
        local.get $p1
        i32.const 24
        i32.add
        local.get $l6
        i64.store
        local.get $p1
        i32.const 16
        i32.add
        local.get $l6
        i64.store
        local.get $p1
        i32.const 8
        i32.add
        local.get $l6
        i64.store
        local.get $p1
        i32.const 32
        i32.add
        local.set $p1
        local.get $p2
        i32.const -32
        i32.add
        local.tee $p2
        i32.const 31
        i32.gt_u
        br_if $L1
      end
    end
    local.get $p0)
  (func $strncmp (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32)
    block $B0
      local.get $p2
      br_if $B0
      i32.const 0
      return
    end
    i32.const 0
    local.set $l3
    block $B1
      local.get $p0
      i32.load8_u
      local.tee $l4
      i32.eqz
      br_if $B1
      local.get $p0
      i32.const 1
      i32.add
      local.set $p0
      local.get $p2
      i32.const -1
      i32.add
      local.set $p2
      loop $L2
        block $B3
          local.get $p1
          i32.load8_u
          local.tee $l5
          br_if $B3
          local.get $l4
          local.set $l3
          br $B1
        end
        block $B4
          local.get $p2
          br_if $B4
          local.get $l4
          local.set $l3
          br $B1
        end
        block $B5
          local.get $l4
          i32.const 255
          i32.and
          local.get $l5
          i32.eq
          br_if $B5
          local.get $l4
          local.set $l3
          br $B1
        end
        local.get $p2
        i32.const -1
        i32.add
        local.set $p2
        local.get $p1
        i32.const 1
        i32.add
        local.set $p1
        local.get $p0
        i32.load8_u
        local.set $l4
        local.get $p0
        i32.const 1
        i32.add
        local.set $p0
        local.get $l4
        br_if $L2
      end
    end
    local.get $l3
    i32.const 255
    i32.and
    local.get $p1
    i32.load8_u
    i32.sub)
  (func $strerror (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32)
    block $B0
      i32.const 0
      i32.load offset=1059840
      local.tee $l1
      br_if $B0
      i32.const 1059816
      local.set $l1
      i32.const 0
      i32.const 1059816
      i32.store offset=1059840
    end
    i32.const 0
    local.get $p0
    local.get $p0
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
    local.get $l1
    i32.load offset=20
    call $__lctrans)
  (func $strerror_r (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32)
    block $B0
      block $B1
        local.get $p0
        call $strerror
        local.tee $p0
        call $strlen
        local.tee $l3
        local.get $p2
        i32.lt_u
        br_if $B1
        i32.const 68
        local.set $l3
        local.get $p2
        i32.eqz
        br_if $B0
        local.get $p1
        local.get $p0
        local.get $p2
        i32.const -1
        i32.add
        local.tee $p2
        call $memcpy
        local.get $p2
        i32.add
        i32.const 0
        i32.store8
        i32.const 68
        return
      end
      local.get $p1
      local.get $p0
      local.get $l3
      i32.const 1
      i32.add
      call $memcpy
      drop
      i32.const 0
      local.set $l3
    end
    local.get $l3)
  (func $__stpcpy (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32)
    block $B0
      block $B1
        block $B2
          local.get $p1
          local.get $p0
          i32.xor
          i32.const 3
          i32.and
          i32.eqz
          br_if $B2
          local.get $p0
          local.set $l2
          br $B1
        end
        block $B3
          block $B4
            local.get $p1
            i32.const 3
            i32.and
            br_if $B4
            local.get $p0
            local.set $l2
            br $B3
          end
          local.get $p0
          local.get $p1
          i32.load8_u
          local.tee $l2
          i32.store8
          block $B5
            local.get $l2
            br_if $B5
            local.get $p0
            return
          end
          local.get $p0
          i32.const 1
          i32.add
          local.set $l2
          block $B6
            local.get $p1
            i32.const 1
            i32.add
            local.tee $l3
            i32.const 3
            i32.and
            br_if $B6
            local.get $l3
            local.set $p1
            br $B3
          end
          local.get $l2
          local.get $l3
          i32.load8_u
          local.tee $l3
          i32.store8
          local.get $l3
          i32.eqz
          br_if $B0
          local.get $p0
          i32.const 2
          i32.add
          local.set $l2
          block $B7
            local.get $p1
            i32.const 2
            i32.add
            local.tee $l3
            i32.const 3
            i32.and
            br_if $B7
            local.get $l3
            local.set $p1
            br $B3
          end
          local.get $l2
          local.get $l3
          i32.load8_u
          local.tee $l3
          i32.store8
          local.get $l3
          i32.eqz
          br_if $B0
          local.get $p0
          i32.const 3
          i32.add
          local.set $l2
          block $B8
            local.get $p1
            i32.const 3
            i32.add
            local.tee $l3
            i32.const 3
            i32.and
            br_if $B8
            local.get $l3
            local.set $p1
            br $B3
          end
          local.get $l2
          local.get $l3
          i32.load8_u
          local.tee $l3
          i32.store8
          local.get $l3
          i32.eqz
          br_if $B0
          local.get $p0
          i32.const 4
          i32.add
          local.set $l2
          local.get $p1
          i32.const 4
          i32.add
          local.set $p1
        end
        local.get $p1
        i32.load
        local.tee $p0
        i32.const -1
        i32.xor
        local.get $p0
        i32.const -16843009
        i32.add
        i32.and
        i32.const -2139062144
        i32.and
        br_if $B1
        loop $L9
          local.get $l2
          local.get $p0
          i32.store
          local.get $l2
          i32.const 4
          i32.add
          local.set $l2
          local.get $p1
          i32.const 4
          i32.add
          local.tee $p1
          i32.load
          local.tee $p0
          i32.const -1
          i32.xor
          local.get $p0
          i32.const -16843009
          i32.add
          i32.and
          i32.const -2139062144
          i32.and
          i32.eqz
          br_if $L9
        end
      end
      local.get $l2
      local.get $p1
      i32.load8_u
      local.tee $p0
      i32.store8
      local.get $p0
      i32.eqz
      br_if $B0
      local.get $p1
      i32.const 1
      i32.add
      local.set $p1
      loop $L10
        local.get $l2
        i32.const 1
        i32.add
        local.tee $l2
        local.get $p1
        i32.load8_u
        local.tee $p0
        i32.store8
        local.get $p1
        i32.const 1
        i32.add
        local.set $p1
        local.get $p0
        br_if $L10
      end
    end
    local.get $l2)
  (func $strcpy (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    local.get $p1
    call $__stpcpy
    drop
    local.get $p0)
  (func $strlen (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32)
    local.get $p0
    local.set $l1
    block $B0
      block $B1
        local.get $p0
        i32.const 3
        i32.and
        i32.eqz
        br_if $B1
        local.get $p0
        local.set $l1
        local.get $p0
        i32.load8_u
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 1
        i32.add
        local.tee $l1
        i32.const 3
        i32.and
        i32.eqz
        br_if $B1
        local.get $l1
        i32.load8_u
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 2
        i32.add
        local.tee $l1
        i32.const 3
        i32.and
        i32.eqz
        br_if $B1
        local.get $l1
        i32.load8_u
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 3
        i32.add
        local.tee $l1
        i32.const 3
        i32.and
        i32.eqz
        br_if $B1
        local.get $l1
        i32.load8_u
        i32.eqz
        br_if $B0
        local.get $p0
        i32.const 4
        i32.add
        local.set $l1
      end
      local.get $l1
      i32.const -4
      i32.add
      local.set $l1
      loop $L2
        local.get $l1
        i32.const 4
        i32.add
        local.tee $l1
        i32.load
        local.tee $l2
        i32.const -1
        i32.xor
        local.get $l2
        i32.const -16843009
        i32.add
        i32.and
        i32.const -2139062144
        i32.and
        i32.eqz
        br_if $L2
      end
      local.get $l2
      i32.const 255
      i32.and
      i32.eqz
      br_if $B0
      loop $L3
        local.get $l1
        i32.const 1
        i32.add
        local.tee $l1
        i32.load8_u
        br_if $L3
      end
    end
    local.get $l1
    local.get $p0
    i32.sub)
  (func $dummy.1 (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0)
  (func $__lctrans (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    local.get $p1
    call $dummy.1)
  (func $_ZN4core10intrinsics17const_eval_select17hbd6213c80b3cb031E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core3ops8function6FnOnce9call_once17hb51a445455a75a4cE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17hb51a445455a75a4cE (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN5alloc5alloc18handle_alloc_error8rt_error17haea38d28aaaeb3e5E
    unreachable)
  (func $_ZN5alloc5alloc18handle_alloc_error8rt_error17haea38d28aaaeb3e5E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $__rust_alloc_error_handler
    unreachable)
  (func $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32)
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  local.get $p2
                  i32.eqz
                  br_if $B6
                  i32.const 1
                  local.set $l4
                  local.get $p1
                  i32.const 0
                  i32.lt_s
                  br_if $B5
                  local.get $p3
                  i32.load offset=8
                  i32.eqz
                  br_if $B3
                  local.get $p3
                  i32.load offset=4
                  local.tee $l5
                  br_if $B4
                  local.get $p1
                  br_if $B2
                  local.get $p2
                  local.set $p3
                  br $B1
                end
                local.get $p0
                local.get $p1
                i32.store offset=4
                i32.const 1
                local.set $l4
              end
              i32.const 0
              local.set $p1
              br $B0
            end
            local.get $p3
            i32.load
            local.get $l5
            local.get $p2
            local.get $p1
            call $__rust_realloc
            local.set $p3
            br $B1
          end
          local.get $p1
          br_if $B2
          local.get $p2
          local.set $p3
          br $B1
        end
        local.get $p1
        local.get $p2
        call $__rust_alloc
        local.set $p3
      end
      block $B7
        local.get $p3
        i32.eqz
        br_if $B7
        local.get $p0
        local.get $p3
        i32.store offset=4
        i32.const 0
        local.set $l4
        br $B0
      end
      local.get $p0
      local.get $p1
      i32.store offset=4
      local.get $p2
      local.set $p1
    end
    local.get $p0
    local.get $l4
    i32.store
    local.get $p0
    i32.const 8
    i32.add
    local.get $p1
    i32.store)
  (func $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core10intrinsics17const_eval_select17hbd6213c80b3cb031E
    unreachable)
  (func $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE (type $t7)
    (local $l0 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l0
    global.set $g0
    local.get $l0
    i32.const 28
    i32.add
    i32.const 0
    i32.store
    local.get $l0
    i32.const 1055100
    i32.store offset=24
    local.get $l0
    i64.const 1
    i64.store offset=12 align=4
    local.get $l0
    i32.const 1055188
    i32.store offset=8
    local.get $l0
    i32.const 8
    i32.add
    i32.const 1055196
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17hf80c878abd8e9e00E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      local.get $p1
      i32.const 1
      i32.add
      local.tee $l3
      local.get $p1
      i32.lt_u
      br_if $B0
      local.get $p0
      i32.const 4
      i32.add
      i32.load
      local.tee $l4
      i32.const 1
      i32.shl
      local.tee $p1
      local.get $l3
      local.get $p1
      local.get $l3
      i32.gt_u
      select
      local.tee $p1
      i32.const 8
      local.get $p1
      i32.const 8
      i32.gt_u
      select
      local.set $p1
      block $B1
        block $B2
          local.get $l4
          br_if $B2
          i32.const 0
          local.set $l3
          br $B1
        end
        local.get $l2
        local.get $l4
        i32.store offset=20
        local.get $l2
        local.get $p0
        i32.load
        i32.store offset=16
        i32.const 1
        local.set $l3
      end
      local.get $l2
      local.get $l3
      i32.store offset=24
      local.get $l2
      local.get $p1
      i32.const 1
      local.get $l2
      i32.const 16
      i32.add
      call $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E
      block $B3
        local.get $l2
        i32.load
        i32.eqz
        br_if $B3
        local.get $l2
        i32.const 8
        i32.add
        i32.load
        local.tee $p0
        i32.eqz
        br_if $B0
        local.get $l2
        i32.load offset=4
        local.get $p0
        call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
        unreachable
      end
      local.get $l2
      i32.load offset=4
      local.set $l3
      local.get $p0
      i32.const 4
      i32.add
      local.get $p1
      i32.store
      local.get $p0
      local.get $l3
      i32.store
      local.get $l2
      i32.const 32
      i32.add
      global.set $g0
      return
    end
    call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
    unreachable)
  (func $__rg_oom (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $rust_oom
    unreachable)
  (func $_ZN72_$LT$$RF$str$u20$as$u20$alloc..ffi..c_str..CString..new..SpecNewImpl$GT$13spec_new_impl17h8200e939a8ca496bE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    block $B0
      block $B1
        block $B2
          local.get $p2
          i32.const 1
          i32.add
          local.tee $l4
          local.get $p2
          i32.lt_u
          br_if $B2
          local.get $l4
          i32.const -1
          i32.le_s
          br_if $B1
          local.get $l4
          i32.const 1
          call $__rust_alloc
          local.tee $l5
          i32.eqz
          br_if $B0
          local.get $l5
          local.get $p1
          local.get $p2
          call $memcpy
          local.set $l6
          block $B3
            block $B4
              local.get $p2
              i32.const 8
              i32.lt_u
              br_if $B4
              local.get $l3
              i32.const 8
              i32.add
              i32.const 0
              local.get $p1
              local.get $p2
              call $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E
              local.get $l3
              i32.load offset=12
              local.set $l7
              local.get $l3
              i32.load offset=8
              local.set $l5
              br $B3
            end
            block $B5
              local.get $p2
              br_if $B5
              i32.const 0
              local.set $l7
              i32.const 0
              local.set $l5
              br $B3
            end
            block $B6
              block $B7
                local.get $p1
                i32.load8_u
                br_if $B7
                i32.const 0
                local.set $l8
                br $B6
              end
              i32.const 1
              local.set $l8
              i32.const 0
              local.set $l5
              block $B8
                local.get $p2
                i32.const 1
                i32.ne
                br_if $B8
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p1
              i32.load8_u offset=1
              i32.eqz
              br_if $B6
              i32.const 2
              local.set $l8
              block $B9
                local.get $p2
                i32.const 2
                i32.ne
                br_if $B9
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p1
              i32.load8_u offset=2
              i32.eqz
              br_if $B6
              i32.const 3
              local.set $l8
              block $B10
                local.get $p2
                i32.const 3
                i32.ne
                br_if $B10
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p1
              i32.load8_u offset=3
              i32.eqz
              br_if $B6
              i32.const 4
              local.set $l8
              block $B11
                local.get $p2
                i32.const 4
                i32.ne
                br_if $B11
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p1
              i32.load8_u offset=4
              i32.eqz
              br_if $B6
              i32.const 5
              local.set $l8
              block $B12
                local.get $p2
                i32.const 5
                i32.ne
                br_if $B12
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p1
              i32.load8_u offset=5
              i32.eqz
              br_if $B6
              i32.const 6
              local.set $l8
              block $B13
                local.get $p2
                i32.const 6
                i32.ne
                br_if $B13
                local.get $p2
                local.set $l7
                br $B3
              end
              local.get $p2
              local.set $l7
              local.get $p1
              i32.load8_u offset=6
              br_if $B3
            end
            i32.const 1
            local.set $l5
            local.get $l8
            local.set $l7
          end
          block $B14
            block $B15
              local.get $l5
              br_if $B15
              local.get $l3
              local.get $p2
              i32.store offset=24
              local.get $l3
              local.get $l4
              i32.store offset=20
              local.get $l3
              local.get $l6
              i32.store offset=16
              local.get $l3
              local.get $l3
              i32.const 16
              i32.add
              call $_ZN5alloc3ffi5c_str7CString19_from_vec_unchecked17ha2258490c84ae819E
              local.get $p0
              local.get $l3
              i64.load
              i64.store offset=4 align=4
              i32.const 0
              local.set $p2
              br $B14
            end
            local.get $p0
            i32.const 16
            i32.add
            local.get $p2
            i32.store
            local.get $p0
            i32.const 12
            i32.add
            local.get $l4
            i32.store
            local.get $p0
            i32.const 8
            i32.add
            local.get $l6
            i32.store
            local.get $p0
            local.get $l7
            i32.store offset=4
            i32.const 1
            local.set $p2
          end
          local.get $p0
          local.get $p2
          i32.store
          local.get $l3
          i32.const 32
          i32.add
          global.set $g0
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
    local.get $l4
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN5alloc3ffi5c_str7CString19_from_vec_unchecked17ha2258490c84ae819E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                local.get $p1
                i32.const 4
                i32.add
                i32.load
                local.tee $l3
                local.get $p1
                i32.load offset=8
                local.tee $l4
                i32.ne
                br_if $B5
                local.get $l4
                i32.const 1
                i32.add
                local.tee $l3
                local.get $l4
                i32.lt_u
                br_if $B1
                block $B6
                  block $B7
                    local.get $l4
                    br_if $B7
                    i32.const 0
                    local.set $l5
                    br $B6
                  end
                  local.get $l2
                  local.get $l4
                  i32.store offset=20
                  local.get $l2
                  local.get $p1
                  i32.load
                  i32.store offset=16
                  i32.const 1
                  local.set $l5
                end
                local.get $l2
                local.get $l5
                i32.store offset=24
                local.get $l2
                local.get $l3
                i32.const 1
                local.get $l2
                i32.const 16
                i32.add
                call $_ZN5alloc7raw_vec11finish_grow17hae9e3b3c0e275c59E
                local.get $l2
                i32.load
                br_if $B4
                local.get $l2
                i32.load offset=4
                local.set $l5
                local.get $p1
                i32.const 4
                i32.add
                local.get $l3
                i32.store
                local.get $p1
                local.get $l5
                i32.store
              end
              block $B8
                local.get $l4
                local.get $l3
                i32.ne
                br_if $B8
                local.get $p1
                local.get $l4
                call $_ZN5alloc7raw_vec19RawVec$LT$T$C$A$GT$16reserve_for_push17hf80c878abd8e9e00E
                local.get $p1
                i32.const 4
                i32.add
                i32.load
                local.set $l3
                local.get $p1
                i32.load offset=8
                local.set $l4
              end
              local.get $p1
              local.get $l4
              i32.const 1
              i32.add
              local.tee $l5
              i32.store offset=8
              local.get $p1
              i32.load
              local.tee $p1
              local.get $l4
              i32.add
              i32.const 0
              i32.store8
              local.get $l3
              local.get $l5
              i32.gt_u
              br_if $B3
              local.get $p1
              local.set $l4
              br $B2
            end
            local.get $l2
            i32.const 8
            i32.add
            i32.load
            local.tee $p1
            i32.eqz
            br_if $B1
            local.get $l2
            i32.load offset=4
            local.get $p1
            call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
            unreachable
          end
          block $B9
            local.get $l5
            br_if $B9
            i32.const 1
            local.set $l4
            local.get $p1
            local.get $l3
            i32.const 1
            call $__rust_dealloc
            br $B2
          end
          local.get $p1
          local.get $l3
          i32.const 1
          local.get $l5
          call $__rust_realloc
          local.tee $l4
          i32.eqz
          br_if $B0
        end
        local.get $p0
        local.get $l5
        i32.store offset=4
        local.get $p0
        local.get $l4
        i32.store
        local.get $l2
        i32.const 32
        i32.add
        global.set $g0
        return
      end
      call $_ZN5alloc7raw_vec17capacity_overflow17hf760c9879129a8bcE
      unreachable
    end
    local.get $l5
    i32.const 1
    call $_ZN5alloc5alloc18handle_alloc_error17h4f1cfc9e59d81358E
    unreachable)
  (func $_ZN5alloc6string13FromUtf8Error10into_bytes17h285f138f92544852E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    i64.load align=4
    i64.store align=4
    local.get $p0
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i32.load
    i32.store)
  (func $_ZN4core3ops8function6FnOnce9call_once17h32ae774a248ad9dfE (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core5slice5index27slice_end_index_len_fail_rt17h9667e64b60d286b0E
    unreachable)
  (func $_ZN4core5slice5index27slice_end_index_len_fail_rt17h9667e64b60d286b0E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p1
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $l2
    i32.const 44
    i32.add
    i32.const 7
    i32.store
    local.get $l2
    i64.const 2
    i64.store offset=12 align=4
    local.get $l2
    i32.const 1056088
    i32.store offset=8
    local.get $l2
    i32.const 7
    i32.store offset=36
    local.get $l2
    local.get $l2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $l2
    local.get $l2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $l2
    local.get $l2
    i32.store offset=32
    local.get $l2
    i32.const 8
    i32.add
    i32.const 1056104
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17h3754f24dd73f738cE (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core5slice5index25slice_index_order_fail_rt17h328217a869652e55E
    unreachable)
  (func $_ZN4core5slice5index25slice_index_order_fail_rt17h328217a869652e55E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p1
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $l2
    i32.const 44
    i32.add
    i32.const 7
    i32.store
    local.get $l2
    i64.const 2
    i64.store offset=12 align=4
    local.get $l2
    i32.const 1056156
    i32.store offset=8
    local.get $l2
    i32.const 7
    i32.store offset=36
    local.get $l2
    local.get $l2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $l2
    local.get $l2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $l2
    local.get $l2
    i32.store offset=32
    local.get $l2
    i32.const 8
    i32.add
    i32.const 1056172
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17ha3f2b2a4e66a4793E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core5slice5index29slice_start_index_len_fail_rt17hd8d1c16b5bc67070E
    unreachable)
  (func $_ZN4core5slice5index29slice_start_index_len_fail_rt17hd8d1c16b5bc67070E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p1
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $l2
    i32.const 44
    i32.add
    i32.const 7
    i32.store
    local.get $l2
    i64.const 2
    i64.store offset=12 align=4
    local.get $l2
    i32.const 1056008
    i32.store offset=8
    local.get $l2
    i32.const 7
    i32.store offset=36
    local.get $l2
    local.get $l2
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $l2
    local.get $l2
    i32.const 4
    i32.add
    i32.store offset=40
    local.get $l2
    local.get $l2
    i32.store offset=32
    local.get $l2
    i32.const 8
    i32.add
    i32.const 1056056
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17hcb988a1f0067a302E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    local.get $p0
    local.get $p1
    local.get $p2
    local.get $p3
    call $_ZN4core3str19slice_error_fail_rt17h3d6d7e29ae750f51E
    unreachable)
  (func $_ZN4core3str19slice_error_fail_rt17h3d6d7e29ae750f51E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    global.get $g0
    i32.const 112
    i32.sub
    local.tee $l4
    global.set $g0
    local.get $l4
    local.get $p3
    i32.store offset=12
    local.get $l4
    local.get $p2
    i32.store offset=8
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  block $B7
                    local.get $p1
                    i32.const 257
                    i32.lt_u
                    br_if $B7
                    i32.const 256
                    local.set $l5
                    block $B8
                      local.get $p0
                      i32.load8_s offset=256
                      i32.const -65
                      i32.gt_s
                      br_if $B8
                      i32.const 255
                      local.set $l5
                      local.get $p0
                      i32.load8_s offset=255
                      i32.const -65
                      i32.gt_s
                      br_if $B8
                      i32.const 254
                      local.set $l5
                      local.get $p0
                      i32.load8_s offset=254
                      i32.const -65
                      i32.gt_s
                      br_if $B8
                      i32.const 253
                      local.set $l5
                    end
                    local.get $l5
                    local.get $p1
                    i32.lt_u
                    br_if $B6
                    local.get $l5
                    local.get $p1
                    i32.ne
                    br_if $B4
                  end
                  local.get $l4
                  local.get $p1
                  i32.store offset=20
                  local.get $l4
                  local.get $p0
                  i32.store offset=16
                  i32.const 0
                  local.set $l5
                  i32.const 1055260
                  local.set $l6
                  br $B5
                end
                local.get $l4
                local.get $l5
                i32.store offset=20
                local.get $l4
                local.get $p0
                i32.store offset=16
                i32.const 5
                local.set $l5
                i32.const 1056471
                local.set $l6
              end
              local.get $l4
              local.get $l5
              i32.store offset=28
              local.get $l4
              local.get $l6
              i32.store offset=24
              local.get $p2
              local.get $p1
              i32.gt_u
              local.tee $l5
              br_if $B3
              local.get $p3
              local.get $p1
              i32.gt_u
              br_if $B3
              block $B9
                local.get $p2
                local.get $p3
                i32.gt_u
                br_if $B9
                block $B10
                  block $B11
                    local.get $p2
                    i32.eqz
                    br_if $B11
                    block $B12
                      local.get $p2
                      local.get $p1
                      i32.lt_u
                      br_if $B12
                      local.get $p1
                      local.get $p2
                      i32.eq
                      br_if $B11
                      br $B10
                    end
                    local.get $p0
                    local.get $p2
                    i32.add
                    i32.load8_s
                    i32.const -64
                    i32.lt_s
                    br_if $B10
                  end
                  local.get $p3
                  local.set $p2
                end
                local.get $l4
                local.get $p2
                i32.store offset=32
                local.get $p1
                local.set $p3
                block $B13
                  local.get $p2
                  local.get $p1
                  i32.ge_u
                  br_if $B13
                  local.get $p2
                  i32.const 1
                  i32.add
                  local.tee $l5
                  i32.const 0
                  local.get $p2
                  i32.const -3
                  i32.add
                  local.tee $p3
                  local.get $p3
                  local.get $p2
                  i32.gt_u
                  select
                  local.tee $p3
                  i32.lt_u
                  br_if $B2
                  block $B14
                    local.get $p3
                    local.get $l5
                    i32.eq
                    br_if $B14
                    local.get $p0
                    local.get $l5
                    i32.add
                    local.get $p0
                    local.get $p3
                    i32.add
                    local.tee $l7
                    i32.sub
                    local.set $l5
                    block $B15
                      local.get $p0
                      local.get $p2
                      i32.add
                      local.tee $l8
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if $B15
                      local.get $l5
                      i32.const -1
                      i32.add
                      local.set $l6
                      br $B14
                    end
                    local.get $p3
                    local.get $p2
                    i32.eq
                    br_if $B14
                    block $B16
                      local.get $l8
                      i32.const -1
                      i32.add
                      local.tee $p2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if $B16
                      local.get $l5
                      i32.const -2
                      i32.add
                      local.set $l6
                      br $B14
                    end
                    local.get $l7
                    local.get $p2
                    i32.eq
                    br_if $B14
                    block $B17
                      local.get $l8
                      i32.const -2
                      i32.add
                      local.tee $p2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if $B17
                      local.get $l5
                      i32.const -3
                      i32.add
                      local.set $l6
                      br $B14
                    end
                    local.get $l7
                    local.get $p2
                    i32.eq
                    br_if $B14
                    block $B18
                      local.get $l8
                      i32.const -3
                      i32.add
                      local.tee $p2
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if $B18
                      local.get $l5
                      i32.const -4
                      i32.add
                      local.set $l6
                      br $B14
                    end
                    local.get $l7
                    local.get $p2
                    i32.eq
                    br_if $B14
                    local.get $l5
                    i32.const -5
                    i32.add
                    local.set $l6
                  end
                  local.get $l6
                  local.get $p3
                  i32.add
                  local.set $p3
                end
                block $B19
                  local.get $p3
                  i32.eqz
                  br_if $B19
                  block $B20
                    local.get $p3
                    local.get $p1
                    i32.lt_u
                    br_if $B20
                    local.get $p3
                    local.get $p1
                    i32.eq
                    br_if $B19
                    br $B0
                  end
                  local.get $p0
                  local.get $p3
                  i32.add
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if $B0
                end
                local.get $p3
                local.get $p1
                i32.eq
                br_if $B1
                block $B21
                  block $B22
                    block $B23
                      block $B24
                        local.get $p0
                        local.get $p3
                        i32.add
                        local.tee $p2
                        i32.load8_s
                        local.tee $p1
                        i32.const -1
                        i32.gt_s
                        br_if $B24
                        local.get $p2
                        i32.load8_u offset=1
                        i32.const 63
                        i32.and
                        local.set $p0
                        local.get $p1
                        i32.const 31
                        i32.and
                        local.set $l5
                        local.get $p1
                        i32.const -33
                        i32.gt_u
                        br_if $B23
                        local.get $l5
                        i32.const 6
                        i32.shl
                        local.get $p0
                        i32.or
                        local.set $p2
                        br $B22
                      end
                      local.get $l4
                      local.get $p1
                      i32.const 255
                      i32.and
                      i32.store offset=36
                      i32.const 1
                      local.set $p1
                      br $B21
                    end
                    local.get $p0
                    i32.const 6
                    i32.shl
                    local.get $p2
                    i32.load8_u offset=2
                    i32.const 63
                    i32.and
                    i32.or
                    local.set $p0
                    block $B25
                      local.get $p1
                      i32.const -16
                      i32.ge_u
                      br_if $B25
                      local.get $p0
                      local.get $l5
                      i32.const 12
                      i32.shl
                      i32.or
                      local.set $p2
                      br $B22
                    end
                    local.get $p0
                    i32.const 6
                    i32.shl
                    local.get $p2
                    i32.load8_u offset=3
                    i32.const 63
                    i32.and
                    i32.or
                    local.get $l5
                    i32.const 18
                    i32.shl
                    i32.const 1835008
                    i32.and
                    i32.or
                    local.tee $p2
                    i32.const 1114112
                    i32.eq
                    br_if $B1
                  end
                  local.get $l4
                  local.get $p2
                  i32.store offset=36
                  i32.const 1
                  local.set $p1
                  local.get $p2
                  i32.const 128
                  i32.lt_u
                  br_if $B21
                  i32.const 2
                  local.set $p1
                  local.get $p2
                  i32.const 2048
                  i32.lt_u
                  br_if $B21
                  i32.const 3
                  i32.const 4
                  local.get $p2
                  i32.const 65536
                  i32.lt_u
                  select
                  local.set $p1
                end
                local.get $l4
                local.get $p3
                i32.store offset=40
                local.get $l4
                local.get $p1
                local.get $p3
                i32.add
                i32.store offset=44
                local.get $l4
                i32.const 48
                i32.add
                i32.const 20
                i32.add
                i32.const 5
                i32.store
                local.get $l4
                i32.const 108
                i32.add
                i32.const 68
                i32.store
                local.get $l4
                i32.const 100
                i32.add
                i32.const 68
                i32.store
                local.get $l4
                i32.const 72
                i32.add
                i32.const 20
                i32.add
                i32.const 69
                i32.store
                local.get $l4
                i32.const 84
                i32.add
                i32.const 70
                i32.store
                local.get $l4
                i64.const 5
                i64.store offset=52 align=4
                local.get $l4
                i32.const 1056704
                i32.store offset=48
                local.get $l4
                i32.const 7
                i32.store offset=76
                local.get $l4
                local.get $l4
                i32.const 72
                i32.add
                i32.store offset=64
                local.get $l4
                local.get $l4
                i32.const 24
                i32.add
                i32.store offset=104
                local.get $l4
                local.get $l4
                i32.const 16
                i32.add
                i32.store offset=96
                local.get $l4
                local.get $l4
                i32.const 40
                i32.add
                i32.store offset=88
                local.get $l4
                local.get $l4
                i32.const 36
                i32.add
                i32.store offset=80
                local.get $l4
                local.get $l4
                i32.const 32
                i32.add
                i32.store offset=72
                local.get $l4
                i32.const 48
                i32.add
                i32.const 1056744
                call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
                unreachable
              end
              local.get $l4
              i32.const 100
              i32.add
              i32.const 68
              i32.store
              local.get $l4
              i32.const 72
              i32.add
              i32.const 20
              i32.add
              i32.const 68
              i32.store
              local.get $l4
              i32.const 84
              i32.add
              i32.const 7
              i32.store
              local.get $l4
              i32.const 48
              i32.add
              i32.const 20
              i32.add
              i32.const 4
              i32.store
              local.get $l4
              i64.const 4
              i64.store offset=52 align=4
              local.get $l4
              i32.const 1056588
              i32.store offset=48
              local.get $l4
              i32.const 7
              i32.store offset=76
              local.get $l4
              local.get $l4
              i32.const 72
              i32.add
              i32.store offset=64
              local.get $l4
              local.get $l4
              i32.const 24
              i32.add
              i32.store offset=96
              local.get $l4
              local.get $l4
              i32.const 16
              i32.add
              i32.store offset=88
              local.get $l4
              local.get $l4
              i32.const 12
              i32.add
              i32.store offset=80
              local.get $l4
              local.get $l4
              i32.const 8
              i32.add
              i32.store offset=72
              local.get $l4
              i32.const 48
              i32.add
              i32.const 1056620
              call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
              unreachable
            end
            local.get $p0
            local.get $p1
            i32.const 0
            local.get $l5
            local.get $l4
            call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
            unreachable
          end
          local.get $l4
          local.get $p2
          local.get $p3
          local.get $l5
          select
          i32.store offset=40
          local.get $l4
          i32.const 48
          i32.add
          i32.const 20
          i32.add
          i32.const 3
          i32.store
          local.get $l4
          i32.const 72
          i32.add
          i32.const 20
          i32.add
          i32.const 68
          i32.store
          local.get $l4
          i32.const 84
          i32.add
          i32.const 68
          i32.store
          local.get $l4
          i64.const 3
          i64.store offset=52 align=4
          local.get $l4
          i32.const 1056512
          i32.store offset=48
          local.get $l4
          i32.const 7
          i32.store offset=76
          local.get $l4
          local.get $l4
          i32.const 72
          i32.add
          i32.store offset=64
          local.get $l4
          local.get $l4
          i32.const 24
          i32.add
          i32.store offset=88
          local.get $l4
          local.get $l4
          i32.const 16
          i32.add
          i32.store offset=80
          local.get $l4
          local.get $l4
          i32.const 40
          i32.add
          i32.store offset=72
          local.get $l4
          i32.const 48
          i32.add
          i32.const 1056536
          call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
          unreachable
        end
        local.get $p3
        local.get $l5
        local.get $l4
        call $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E
        unreachable
      end
      i32.const 1055352
      i32.const 43
      i32.const 1056636
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get $p0
    local.get $p1
    local.get $p3
    local.get $p1
    local.get $l4
    call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
    unreachable)
  (func $_ZN4core3ops8function6FnOnce9call_once17he625032f56c99b4fE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    drop
    loop $L0 (result i32)
      br $L0
    end)
  (func $_ZN4core3ptr102drop_in_place$LT$$RF$core..iter..adapters..copied..Copied$LT$core..slice..iter..Iter$LT$u8$GT$$GT$$GT$17h0ae777612708bf9bE (type $t0) (param $p0 i32))
  (func $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 1
    i32.store8 offset=24
    local.get $l2
    local.get $p1
    i32.store offset=20
    local.get $l2
    local.get $p0
    i32.store offset=16
    local.get $l2
    i32.const 1055420
    i32.store offset=12
    local.get $l2
    i32.const 1055260
    i32.store offset=8
    local.get $l2
    i32.const 8
    i32.add
    call $rust_begin_unwind
    unreachable)
  (func $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    local.get $p1
    i32.store offset=4
    local.get $l3
    local.get $p0
    i32.store
    local.get $l3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    local.get $l3
    i32.const 44
    i32.add
    i32.const 7
    i32.store
    local.get $l3
    i64.const 2
    i64.store offset=12 align=4
    local.get $l3
    i32.const 1055336
    i32.store offset=8
    local.get $l3
    i32.const 7
    i32.store offset=36
    local.get $l3
    local.get $l3
    i32.const 32
    i32.add
    i32.store offset=24
    local.get $l3
    local.get $l3
    i32.store offset=40
    local.get $l3
    local.get $l3
    i32.const 4
    i32.add
    i32.store offset=32
    local.get $l3
    i32.const 8
    i32.add
    local.get $p2
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core10intrinsics17const_eval_select17hd6a7319a41be58fcE
    unreachable)
  (func $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core10intrinsics17const_eval_select17h045db9052655b575E
    unreachable)
  (func $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    local.get $p0
    i32.load offset=16
    local.set $l3
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                local.get $p0
                i32.load offset=8
                local.tee $l4
                i32.const 1
                i32.eq
                br_if $B5
                local.get $l3
                i32.const 1
                i32.ne
                br_if $B4
              end
              local.get $l3
              i32.const 1
              i32.ne
              br_if $B1
              local.get $p1
              local.get $p2
              i32.add
              local.set $l5
              local.get $p0
              i32.const 20
              i32.add
              i32.load
              local.tee $l6
              br_if $B3
              i32.const 0
              local.set $l7
              local.get $p1
              local.set $l8
              br $B2
            end
            local.get $p0
            i32.load offset=24
            local.get $p1
            local.get $p2
            local.get $p0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect (type $t5) $T0
            local.set $l3
            br $B0
          end
          i32.const 0
          local.set $l7
          local.get $p1
          local.set $l8
          loop $L6
            local.get $l8
            local.tee $l3
            local.get $l5
            i32.eq
            br_if $B1
            block $B7
              block $B8
                local.get $l3
                i32.load8_s
                local.tee $l8
                i32.const -1
                i32.le_s
                br_if $B8
                local.get $l3
                i32.const 1
                i32.add
                local.set $l8
                br $B7
              end
              block $B9
                local.get $l8
                i32.const -32
                i32.ge_u
                br_if $B9
                local.get $l3
                i32.const 2
                i32.add
                local.set $l8
                br $B7
              end
              block $B10
                local.get $l8
                i32.const -16
                i32.ge_u
                br_if $B10
                local.get $l3
                i32.const 3
                i32.add
                local.set $l8
                br $B7
              end
              local.get $l3
              i32.load8_u offset=2
              i32.const 63
              i32.and
              i32.const 6
              i32.shl
              local.get $l3
              i32.load8_u offset=1
              i32.const 63
              i32.and
              i32.const 12
              i32.shl
              i32.or
              local.get $l3
              i32.load8_u offset=3
              i32.const 63
              i32.and
              i32.or
              local.get $l8
              i32.const 255
              i32.and
              i32.const 18
              i32.shl
              i32.const 1835008
              i32.and
              i32.or
              i32.const 1114112
              i32.eq
              br_if $B1
              local.get $l3
              i32.const 4
              i32.add
              local.set $l8
            end
            local.get $l7
            local.get $l3
            i32.sub
            local.get $l8
            i32.add
            local.set $l7
            local.get $l6
            i32.const -1
            i32.add
            local.tee $l6
            br_if $L6
          end
        end
        local.get $l8
        local.get $l5
        i32.eq
        br_if $B1
        block $B11
          local.get $l8
          i32.load8_s
          local.tee $l3
          i32.const -1
          i32.gt_s
          br_if $B11
          local.get $l3
          i32.const -32
          i32.lt_u
          br_if $B11
          local.get $l3
          i32.const -16
          i32.lt_u
          br_if $B11
          local.get $l8
          i32.load8_u offset=2
          i32.const 63
          i32.and
          i32.const 6
          i32.shl
          local.get $l8
          i32.load8_u offset=1
          i32.const 63
          i32.and
          i32.const 12
          i32.shl
          i32.or
          local.get $l8
          i32.load8_u offset=3
          i32.const 63
          i32.and
          i32.or
          local.get $l3
          i32.const 255
          i32.and
          i32.const 18
          i32.shl
          i32.const 1835008
          i32.and
          i32.or
          i32.const 1114112
          i32.eq
          br_if $B1
        end
        block $B12
          block $B13
            block $B14
              local.get $l7
              br_if $B14
              i32.const 0
              local.set $l8
              br $B13
            end
            block $B15
              local.get $l7
              local.get $p2
              i32.lt_u
              br_if $B15
              i32.const 0
              local.set $l3
              local.get $p2
              local.set $l8
              local.get $l7
              local.get $p2
              i32.eq
              br_if $B13
              br $B12
            end
            i32.const 0
            local.set $l3
            local.get $l7
            local.set $l8
            local.get $p1
            local.get $l7
            i32.add
            i32.load8_s
            i32.const -64
            i32.lt_s
            br_if $B12
          end
          local.get $l8
          local.set $l7
          local.get $p1
          local.set $l3
        end
        local.get $l7
        local.get $p2
        local.get $l3
        select
        local.set $p2
        local.get $l3
        local.get $p1
        local.get $l3
        select
        local.set $p1
      end
      block $B16
        local.get $l4
        br_if $B16
        local.get $p0
        i32.load offset=24
        local.get $p1
        local.get $p2
        local.get $p0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        return
      end
      local.get $p0
      i32.const 12
      i32.add
      i32.load
      local.set $l5
      block $B17
        block $B18
          local.get $p2
          i32.const 16
          i32.lt_u
          br_if $B18
          local.get $p1
          local.get $p2
          call $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E
          local.set $l8
          br $B17
        end
        block $B19
          local.get $p2
          br_if $B19
          i32.const 0
          local.set $l8
          br $B17
        end
        local.get $p2
        i32.const 3
        i32.and
        local.set $l7
        block $B20
          block $B21
            local.get $p2
            i32.const -1
            i32.add
            i32.const 3
            i32.ge_u
            br_if $B21
            i32.const 0
            local.set $l8
            local.get $p1
            local.set $l3
            br $B20
          end
          local.get $p2
          i32.const -4
          i32.and
          local.set $l6
          i32.const 0
          local.set $l8
          local.get $p1
          local.set $l3
          loop $L22
            local.get $l8
            local.get $l3
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $l3
            i32.const 1
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $l3
            i32.const 2
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $l3
            i32.const 3
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set $l8
            local.get $l3
            i32.const 4
            i32.add
            local.set $l3
            local.get $l6
            i32.const -4
            i32.add
            local.tee $l6
            br_if $L22
          end
        end
        local.get $l7
        i32.eqz
        br_if $B17
        loop $L23
          local.get $l8
          local.get $l3
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set $l8
          local.get $l3
          i32.const 1
          i32.add
          local.set $l3
          local.get $l7
          i32.const -1
          i32.add
          local.tee $l7
          br_if $L23
        end
      end
      block $B24
        local.get $l5
        local.get $l8
        i32.le_u
        br_if $B24
        i32.const 0
        local.set $l3
        local.get $l5
        local.get $l8
        i32.sub
        local.tee $l7
        local.set $l6
        block $B25
          block $B26
            block $B27
              i32.const 0
              local.get $p0
              i32.load8_u offset=32
              local.tee $l8
              local.get $l8
              i32.const 3
              i32.eq
              select
              i32.const 3
              i32.and
              br_table $B25 $B27 $B26 $B25
            end
            i32.const 0
            local.set $l6
            local.get $l7
            local.set $l3
            br $B25
          end
          local.get $l7
          i32.const 1
          i32.shr_u
          local.set $l3
          local.get $l7
          i32.const 1
          i32.add
          i32.const 1
          i32.shr_u
          local.set $l6
        end
        local.get $l3
        i32.const 1
        i32.add
        local.set $l3
        local.get $p0
        i32.const 28
        i32.add
        i32.load
        local.set $l7
        local.get $p0
        i32.load offset=4
        local.set $l8
        local.get $p0
        i32.load offset=24
        local.set $p0
        block $B28
          loop $L29
            local.get $l3
            i32.const -1
            i32.add
            local.tee $l3
            i32.eqz
            br_if $B28
            local.get $p0
            local.get $l8
            local.get $l7
            i32.load offset=16
            call_indirect (type $t3) $T0
            i32.eqz
            br_if $L29
          end
          i32.const 1
          return
        end
        i32.const 1
        local.set $l3
        local.get $l8
        i32.const 1114112
        i32.eq
        br_if $B0
        local.get $p0
        local.get $p1
        local.get $p2
        local.get $l7
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        i32.const 0
        local.set $l3
        loop $L30
          block $B31
            local.get $l6
            local.get $l3
            i32.ne
            br_if $B31
            local.get $l6
            local.get $l6
            i32.lt_u
            return
          end
          local.get $l3
          i32.const 1
          i32.add
          local.set $l3
          local.get $p0
          local.get $l8
          local.get $l7
          i32.load offset=16
          call_indirect (type $t3) $T0
          i32.eqz
          br_if $L30
        end
        local.get $l3
        i32.const -1
        i32.add
        local.get $l6
        i32.lt_u
        return
      end
      local.get $p0
      i32.load offset=24
      local.get $p1
      local.get $p2
      local.get $p0
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type $t5) $T0
      return
    end
    local.get $l3)
  (func $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    local.get $l3
    i32.const 1055260
    i32.store offset=16
    local.get $l3
    i64.const 1
    i64.store offset=4 align=4
    local.get $l3
    local.get $p1
    i32.store offset=28
    local.get $l3
    local.get $p0
    i32.store offset=24
    local.get $l3
    local.get $l3
    i32.const 24
    i32.add
    i32.store
    local.get $l3
    local.get $p2
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core10intrinsics17const_eval_select17h61c8e8fbcbf6b317E
    unreachable)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i64.load32_u
    i32.const 1
    local.get $p1
    call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E)
  (func $_ZN4core10intrinsics17const_eval_select17h045db9052655b575E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core3ops8function6FnOnce9call_once17h32ae774a248ad9dfE
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17h5024e92cfa62bd24E (type $t0) (param $p0 i32)
    local.get $p0
    i32.load
    local.get $p0
    i32.load offset=4
    local.get $p0
    i32.load offset=8
    local.get $p0
    i32.load offset=12
    call $_ZN4core3ops8function6FnOnce9call_once17hcb988a1f0067a302E
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17h61c8e8fbcbf6b317E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core3ops8function6FnOnce9call_once17h3754f24dd73f738cE
    unreachable)
  (func $_ZN4core10intrinsics17const_eval_select17hd6a7319a41be58fcE (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    call $_ZN4core3ops8function6FnOnce9call_once17ha3f2b2a4e66a4793E
    unreachable)
  (func $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p1
              i32.load
              local.tee $l3
              i32.const 16
              i32.and
              br_if $B4
              local.get $l3
              i32.const 32
              i32.and
              br_if $B3
              local.get $p0
              i64.load32_u
              i32.const 1
              local.get $p1
              call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E
              local.set $p0
              br $B0
            end
            local.get $p0
            i32.load
            local.set $p0
            i32.const 0
            local.set $l3
            loop $L5
              local.get $l2
              local.get $l3
              i32.add
              i32.const 127
              i32.add
              i32.const 48
              i32.const 87
              local.get $p0
              i32.const 15
              i32.and
              local.tee $l4
              i32.const 10
              i32.lt_u
              select
              local.get $l4
              i32.add
              i32.store8
              local.get $l3
              i32.const -1
              i32.add
              local.set $l3
              local.get $p0
              i32.const 15
              i32.gt_u
              local.set $l4
              local.get $p0
              i32.const 4
              i32.shr_u
              local.set $p0
              local.get $l4
              br_if $L5
            end
            local.get $l3
            i32.const 128
            i32.add
            local.tee $p0
            i32.const 129
            i32.ge_u
            br_if $B2
            local.get $p1
            i32.const 1
            i32.const 1055716
            i32.const 2
            local.get $l2
            local.get $l3
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get $l3
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
            local.set $p0
            br $B0
          end
          local.get $p0
          i32.load
          local.set $p0
          i32.const 0
          local.set $l3
          loop $L6
            local.get $l2
            local.get $l3
            i32.add
            i32.const 127
            i32.add
            i32.const 48
            i32.const 55
            local.get $p0
            i32.const 15
            i32.and
            local.tee $l4
            i32.const 10
            i32.lt_u
            select
            local.get $l4
            i32.add
            i32.store8
            local.get $l3
            i32.const -1
            i32.add
            local.set $l3
            local.get $p0
            i32.const 15
            i32.gt_u
            local.set $l4
            local.get $p0
            i32.const 4
            i32.shr_u
            local.set $p0
            local.get $l4
            br_if $L6
          end
          local.get $l3
          i32.const 128
          i32.add
          local.tee $p0
          i32.const 129
          i32.ge_u
          br_if $B1
          local.get $p1
          i32.const 1
          i32.const 1055716
          i32.const 2
          local.get $l2
          local.get $l3
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get $l3
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
          local.set $p0
          br $B0
        end
        local.get $p0
        i32.const 128
        local.get $p0
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get $p0
      i32.const 128
      local.get $p0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $l2
    i32.const 128
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt5write17ha48fcb73fabd1d51E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    i32.const 36
    i32.add
    local.get $p1
    i32.store
    local.get $l3
    i32.const 3
    i32.store8 offset=40
    local.get $l3
    i64.const 137438953472
    i64.store offset=8
    local.get $l3
    local.get $p0
    i32.store offset=32
    i32.const 0
    local.set $l4
    local.get $l3
    i32.const 0
    i32.store offset=24
    local.get $l3
    i32.const 0
    i32.store offset=16
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p2
            i32.load offset=8
            local.tee $l5
            br_if $B3
            local.get $p2
            i32.const 20
            i32.add
            i32.load
            local.tee $l6
            i32.eqz
            br_if $B2
            local.get $p2
            i32.load
            local.set $p1
            local.get $p2
            i32.load offset=16
            local.set $p0
            local.get $l6
            i32.const -1
            i32.add
            i32.const 536870911
            i32.and
            i32.const 1
            i32.add
            local.tee $l4
            local.set $l6
            loop $L4
              block $B5
                local.get $p1
                i32.const 4
                i32.add
                i32.load
                local.tee $l7
                i32.eqz
                br_if $B5
                local.get $l3
                i32.load offset=32
                local.get $p1
                i32.load
                local.get $l7
                local.get $l3
                i32.load offset=36
                i32.load offset=12
                call_indirect (type $t5) $T0
                br_if $B1
              end
              local.get $p0
              i32.load
              local.get $l3
              i32.const 8
              i32.add
              local.get $p0
              i32.const 4
              i32.add
              i32.load
              call_indirect (type $t3) $T0
              br_if $B1
              local.get $p0
              i32.const 8
              i32.add
              local.set $p0
              local.get $p1
              i32.const 8
              i32.add
              local.set $p1
              local.get $l6
              i32.const -1
              i32.add
              local.tee $l6
              br_if $L4
              br $B2
            end
          end
          local.get $p2
          i32.const 12
          i32.add
          i32.load
          local.tee $p0
          i32.eqz
          br_if $B2
          local.get $p0
          i32.const 5
          i32.shl
          local.set $l8
          local.get $p0
          i32.const -1
          i32.add
          i32.const 134217727
          i32.and
          i32.const 1
          i32.add
          local.set $l4
          local.get $p2
          i32.load
          local.set $p1
          i32.const 0
          local.set $l6
          loop $L6
            block $B7
              local.get $p1
              i32.const 4
              i32.add
              i32.load
              local.tee $p0
              i32.eqz
              br_if $B7
              local.get $l3
              i32.load offset=32
              local.get $p1
              i32.load
              local.get $p0
              local.get $l3
              i32.load offset=36
              i32.load offset=12
              call_indirect (type $t5) $T0
              br_if $B1
            end
            local.get $l3
            local.get $l5
            local.get $l6
            i32.add
            local.tee $p0
            i32.const 28
            i32.add
            i32.load8_u
            i32.store8 offset=40
            local.get $l3
            local.get $p0
            i32.const 4
            i32.add
            i64.load align=4
            i64.const 32
            i64.rotl
            i64.store offset=8
            local.get $p0
            i32.const 24
            i32.add
            i32.load
            local.set $l9
            local.get $p2
            i32.load offset=16
            local.set $l10
            i32.const 0
            local.set $l11
            i32.const 0
            local.set $l7
            block $B8
              block $B9
                block $B10
                  local.get $p0
                  i32.const 20
                  i32.add
                  i32.load
                  br_table $B9 $B10 $B8 $B9
                end
                local.get $l9
                i32.const 3
                i32.shl
                local.set $l12
                i32.const 0
                local.set $l7
                local.get $l10
                local.get $l12
                i32.add
                local.tee $l12
                i32.load offset=4
                i32.const 71
                i32.ne
                br_if $B8
                local.get $l12
                i32.load
                i32.load
                local.set $l9
              end
              i32.const 1
              local.set $l7
            end
            local.get $l3
            local.get $l9
            i32.store offset=20
            local.get $l3
            local.get $l7
            i32.store offset=16
            local.get $p0
            i32.const 16
            i32.add
            i32.load
            local.set $l7
            block $B11
              block $B12
                block $B13
                  local.get $p0
                  i32.const 12
                  i32.add
                  i32.load
                  br_table $B12 $B13 $B11 $B12
                end
                local.get $l7
                i32.const 3
                i32.shl
                local.set $l9
                local.get $l10
                local.get $l9
                i32.add
                local.tee $l9
                i32.load offset=4
                i32.const 71
                i32.ne
                br_if $B11
                local.get $l9
                i32.load
                i32.load
                local.set $l7
              end
              i32.const 1
              local.set $l11
            end
            local.get $l3
            local.get $l7
            i32.store offset=28
            local.get $l3
            local.get $l11
            i32.store offset=24
            local.get $l10
            local.get $p0
            i32.load
            i32.const 3
            i32.shl
            i32.add
            local.tee $p0
            i32.load
            local.get $l3
            i32.const 8
            i32.add
            local.get $p0
            i32.load offset=4
            call_indirect (type $t3) $T0
            br_if $B1
            local.get $p1
            i32.const 8
            i32.add
            local.set $p1
            local.get $l8
            local.get $l6
            i32.const 32
            i32.add
            local.tee $l6
            i32.ne
            br_if $L6
          end
        end
        i32.const 0
        local.set $p0
        local.get $l4
        local.get $p2
        i32.load offset=4
        i32.lt_u
        local.tee $p1
        i32.eqz
        br_if $B0
        local.get $l3
        i32.load offset=32
        local.get $p2
        i32.load
        local.get $l4
        i32.const 3
        i32.shl
        i32.add
        i32.const 0
        local.get $p1
        select
        local.tee $p1
        i32.load
        local.get $p1
        i32.load offset=4
        local.get $l3
        i32.load offset=36
        i32.load offset=12
        call_indirect (type $t5) $T0
        i32.eqz
        br_if $B0
      end
      i32.const 1
      local.set $p0
    end
    local.get $l3
    i32.const 48
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17ha826dd87512c4c00E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    i32.const 1
    local.set $l3
    block $B0
      local.get $p0
      local.get $p1
      call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E
      br_if $B0
      local.get $p1
      i32.const 28
      i32.add
      i32.load
      local.set $l4
      local.get $p1
      i32.load offset=24
      local.set $l5
      local.get $l2
      i32.const 28
      i32.add
      i32.const 0
      i32.store
      local.get $l2
      i32.const 1055260
      i32.store offset=24
      local.get $l2
      i64.const 1
      i64.store offset=12 align=4
      local.get $l2
      i32.const 1055264
      i32.store offset=8
      local.get $l5
      local.get $l4
      local.get $l2
      i32.const 8
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      br_if $B0
      local.get $p0
      i32.const 4
      i32.add
      local.get $p1
      call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E
      local.set $l3
    end
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $l3)
  (func $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hd78f961f99573a2cE (type $t1) (param $p0 i32) (result i64)
    i64.const -887290134024487584)
  (func $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h651b426602cda903E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p1
    i32.load offset=24
    i32.const 1055272
    i32.const 14
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type $t5) $T0)
  (func $_ZN82_$LT$core..char..EscapeDebug$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h16491d8cb5c37ce0E (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32)
    i32.const 1114112
    local.set $l1
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p0
              i32.load
              br_table $B1 $B4 $B3 $B2 $B1
            end
            local.get $p0
            i32.const 0
            i32.store
            local.get $p0
            i32.load offset=4
            return
          end
          local.get $p0
          i32.const 1
          i32.store
          i32.const 92
          return
        end
        block $B5
          block $B6
            block $B7
              block $B8
                block $B9
                  local.get $p0
                  i32.const 12
                  i32.add
                  i32.load8_u
                  br_table $B1 $B9 $B5 $B8 $B7 $B6 $B1
                end
                local.get $p0
                i32.const 0
                i32.store8 offset=12
                i32.const 125
                return
              end
              local.get $p0
              i32.const 2
              i32.store8 offset=12
              i32.const 123
              return
            end
            local.get $p0
            i32.const 3
            i32.store8 offset=12
            i32.const 117
            return
          end
          local.get $p0
          i32.const 4
          i32.store8 offset=12
          i32.const 92
          return
        end
        i32.const 48
        i32.const 87
        local.get $p0
        i32.load offset=4
        local.get $p0
        i32.const 8
        i32.add
        i32.load
        local.tee $l2
        i32.const 2
        i32.shl
        i32.shr_u
        i32.const 15
        i32.and
        local.tee $l1
        i32.const 10
        i32.lt_u
        select
        local.get $l1
        i32.add
        local.set $l1
        local.get $l2
        i32.eqz
        br_if $B0
        local.get $p0
        local.get $l2
        i32.const -1
        i32.add
        i32.store offset=8
      end
      local.get $l1
      return
    end
    local.get $p0
    i32.const 1
    i32.store8 offset=12
    local.get $l1)
  (func $_ZN4core5slice6memchr19memchr_general_case17h494cc42176574201E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p2
            i32.const 3
            i32.add
            i32.const -4
            i32.and
            local.get $p2
            i32.sub
            local.tee $l4
            i32.eqz
            br_if $B3
            local.get $p3
            local.get $l4
            local.get $l4
            local.get $p3
            i32.gt_u
            select
            local.tee $l4
            i32.eqz
            br_if $B3
            i32.const 0
            local.set $l5
            local.get $p1
            i32.const 255
            i32.and
            local.set $l6
            i32.const 1
            local.set $l7
            loop $L4
              local.get $p2
              local.get $l5
              i32.add
              i32.load8_u
              local.get $l6
              i32.eq
              br_if $B0
              local.get $l4
              local.get $l5
              i32.const 1
              i32.add
              local.tee $l5
              i32.ne
              br_if $L4
            end
            local.get $l4
            local.get $p3
            i32.const -8
            i32.add
            local.tee $l8
            i32.gt_u
            br_if $B1
            br $B2
          end
          local.get $p3
          i32.const -8
          i32.add
          local.set $l8
          i32.const 0
          local.set $l4
        end
        local.get $p1
        i32.const 255
        i32.and
        i32.const 16843009
        i32.mul
        local.set $l5
        block $B5
          loop $L6
            local.get $p2
            local.get $l4
            i32.add
            local.tee $l6
            i32.load
            local.get $l5
            i32.xor
            local.tee $l7
            i32.const -1
            i32.xor
            local.get $l7
            i32.const -16843009
            i32.add
            i32.and
            local.get $l6
            i32.const 4
            i32.add
            i32.load
            local.get $l5
            i32.xor
            local.tee $l6
            i32.const -1
            i32.xor
            local.get $l6
            i32.const -16843009
            i32.add
            i32.and
            i32.or
            i32.const -2139062144
            i32.and
            br_if $B5
            local.get $l4
            i32.const 8
            i32.add
            local.tee $l4
            local.get $l8
            i32.le_u
            br_if $L6
          end
        end
        local.get $l4
        local.get $p3
        i32.le_u
        br_if $B1
        local.get $l4
        local.get $p3
        local.get $l4
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      block $B7
        local.get $l4
        local.get $p3
        i32.eq
        br_if $B7
        local.get $l4
        local.get $p3
        i32.sub
        local.set $l8
        local.get $p2
        local.get $l4
        i32.add
        local.set $l6
        i32.const 0
        local.set $l5
        local.get $p1
        i32.const 255
        i32.and
        local.set $l7
        block $B8
          loop $L9
            local.get $l6
            local.get $l5
            i32.add
            i32.load8_u
            local.get $l7
            i32.eq
            br_if $B8
            local.get $l8
            local.get $l5
            i32.const 1
            i32.add
            local.tee $l5
            i32.add
            i32.eqz
            br_if $B7
            br $L9
          end
        end
        local.get $l4
        local.get $l5
        i32.add
        local.set $l5
        i32.const 1
        local.set $l7
        br $B0
      end
      i32.const 0
      local.set $l7
    end
    local.get $p0
    local.get $l5
    i32.store offset=4
    local.get $p0
    local.get $l7
    i32.store)
  (func $_ZN4core3str8converts9from_utf817he88ceb775ab84bd1E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i64) (local $l9 i64) (local $l10 i32)
    block $B0
      local.get $p2
      i32.eqz
      br_if $B0
      i32.const 0
      local.get $p2
      i32.const -7
      i32.add
      local.tee $l3
      local.get $l3
      local.get $p2
      i32.gt_u
      select
      local.set $l4
      local.get $p1
      i32.const 3
      i32.add
      i32.const -4
      i32.and
      local.get $p1
      i32.sub
      local.set $l5
      i32.const 0
      local.set $l3
      block $B1
        block $B2
          block $B3
            block $B4
              loop $L5
                block $B6
                  block $B7
                    block $B8
                      local.get $p1
                      local.get $l3
                      i32.add
                      i32.load8_u
                      local.tee $l6
                      i32.const 24
                      i32.shl
                      i32.const 24
                      i32.shr_s
                      local.tee $l7
                      i32.const 0
                      i32.lt_s
                      br_if $B8
                      local.get $l5
                      i32.const -1
                      i32.eq
                      br_if $B7
                      local.get $l5
                      local.get $l3
                      i32.sub
                      i32.const 3
                      i32.and
                      br_if $B7
                      block $B9
                        local.get $l3
                        local.get $l4
                        i32.ge_u
                        br_if $B9
                        loop $L10
                          local.get $p1
                          local.get $l3
                          i32.add
                          local.tee $l6
                          i32.load
                          local.get $l6
                          i32.const 4
                          i32.add
                          i32.load
                          i32.or
                          i32.const -2139062144
                          i32.and
                          br_if $B9
                          local.get $l3
                          i32.const 8
                          i32.add
                          local.tee $l3
                          local.get $l4
                          i32.lt_u
                          br_if $L10
                        end
                      end
                      local.get $l3
                      local.get $p2
                      i32.ge_u
                      br_if $B6
                      loop $L11
                        local.get $p1
                        local.get $l3
                        i32.add
                        i32.load8_s
                        i32.const 0
                        i32.lt_s
                        br_if $B6
                        local.get $p2
                        local.get $l3
                        i32.const 1
                        i32.add
                        local.tee $l3
                        i32.ne
                        br_if $L11
                        br $B0
                      end
                    end
                    i64.const 1099511627776
                    local.set $l8
                    i64.const 4294967296
                    local.set $l9
                    block $B12
                      block $B13
                        block $B14
                          block $B15
                            block $B16
                              block $B17
                                block $B18
                                  block $B19
                                    block $B20
                                      local.get $l6
                                      i32.const 1056188
                                      i32.add
                                      i32.load8_u
                                      i32.const -2
                                      i32.add
                                      br_table $B20 $B19 $B18 $B1
                                    end
                                    local.get $l3
                                    i32.const 1
                                    i32.add
                                    local.tee $l6
                                    local.get $p2
                                    i32.lt_u
                                    br_if $B13
                                    i64.const 0
                                    local.set $l8
                                    br $B2
                                  end
                                  i64.const 0
                                  local.set $l8
                                  local.get $l3
                                  i32.const 1
                                  i32.add
                                  local.tee $l10
                                  local.get $p2
                                  i32.ge_u
                                  br_if $B2
                                  local.get $p1
                                  local.get $l10
                                  i32.add
                                  i32.load8_s
                                  local.set $l10
                                  local.get $l6
                                  i32.const -224
                                  i32.add
                                  br_table $B17 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B15 $B16 $B15
                                end
                                i64.const 0
                                local.set $l8
                                local.get $l3
                                i32.const 1
                                i32.add
                                local.tee $l10
                                local.get $p2
                                i32.ge_u
                                br_if $B2
                                local.get $p1
                                local.get $l10
                                i32.add
                                i32.load8_s
                                local.set $l10
                                block $B21
                                  block $B22
                                    block $B23
                                      block $B24
                                        local.get $l6
                                        i32.const -240
                                        i32.add
                                        br_table $B23 $B24 $B24 $B24 $B22 $B24
                                      end
                                      local.get $l7
                                      i32.const 15
                                      i32.add
                                      i32.const 255
                                      i32.and
                                      i32.const 2
                                      i32.gt_u
                                      br_if $B3
                                      local.get $l10
                                      i32.const -1
                                      i32.gt_s
                                      br_if $B3
                                      local.get $l10
                                      i32.const -64
                                      i32.ge_u
                                      br_if $B3
                                      br $B21
                                    end
                                    local.get $l10
                                    i32.const 112
                                    i32.add
                                    i32.const 255
                                    i32.and
                                    i32.const 48
                                    i32.ge_u
                                    br_if $B3
                                    br $B21
                                  end
                                  local.get $l10
                                  i32.const -113
                                  i32.gt_s
                                  br_if $B3
                                end
                                local.get $l3
                                i32.const 2
                                i32.add
                                local.tee $l6
                                local.get $p2
                                i32.ge_u
                                br_if $B2
                                local.get $p1
                                local.get $l6
                                i32.add
                                i32.load8_s
                                i32.const -65
                                i32.gt_s
                                br_if $B4
                                i64.const 0
                                local.set $l9
                                local.get $l3
                                i32.const 3
                                i32.add
                                local.tee $l6
                                local.get $p2
                                i32.ge_u
                                br_if $B1
                                local.get $p1
                                local.get $l6
                                i32.add
                                i32.load8_s
                                i32.const -65
                                i32.le_s
                                br_if $B12
                                i64.const 3298534883328
                                local.set $l8
                                i64.const 4294967296
                                local.set $l9
                                br $B1
                              end
                              local.get $l10
                              i32.const -32
                              i32.and
                              i32.const -96
                              i32.ne
                              br_if $B3
                              br $B14
                            end
                            local.get $l10
                            i32.const -96
                            i32.ge_s
                            br_if $B3
                            br $B14
                          end
                          block $B25
                            local.get $l7
                            i32.const 31
                            i32.add
                            i32.const 255
                            i32.and
                            i32.const 12
                            i32.lt_u
                            br_if $B25
                            local.get $l7
                            i32.const -2
                            i32.and
                            i32.const -18
                            i32.ne
                            br_if $B3
                            local.get $l10
                            i32.const -1
                            i32.gt_s
                            br_if $B3
                            local.get $l10
                            i32.const -64
                            i32.ge_u
                            br_if $B3
                            br $B14
                          end
                          local.get $l10
                          i32.const -65
                          i32.gt_s
                          br_if $B3
                        end
                        i64.const 0
                        local.set $l9
                        local.get $l3
                        i32.const 2
                        i32.add
                        local.tee $l6
                        local.get $p2
                        i32.ge_u
                        br_if $B1
                        local.get $p1
                        local.get $l6
                        i32.add
                        i32.load8_s
                        i32.const -65
                        i32.gt_s
                        br_if $B4
                        br $B12
                      end
                      i64.const 1099511627776
                      local.set $l8
                      i64.const 4294967296
                      local.set $l9
                      local.get $p1
                      local.get $l6
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.gt_s
                      br_if $B1
                    end
                    local.get $l6
                    i32.const 1
                    i32.add
                    local.set $l3
                    br $B6
                  end
                  local.get $l3
                  i32.const 1
                  i32.add
                  local.set $l3
                end
                local.get $l3
                local.get $p2
                i32.lt_u
                br_if $L5
                br $B0
              end
            end
            i64.const 2199023255552
            local.set $l8
            i64.const 4294967296
            local.set $l9
            br $B1
          end
          i64.const 1099511627776
          local.set $l8
          i64.const 4294967296
          local.set $l9
          br $B1
        end
        i64.const 0
        local.set $l9
      end
      local.get $p0
      local.get $l8
      local.get $l3
      i64.extend_i32_u
      i64.or
      local.get $l9
      i64.or
      i64.store offset=4 align=4
      local.get $p0
      i32.const 1
      i32.store
      return
    end
    local.get $p0
    local.get $p1
    i32.store offset=4
    local.get $p0
    i32.const 8
    i32.add
    local.get $p2
    i32.store
    local.get $p0
    i32.const 0
    i32.store)
  (func $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE (type $t12) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32) (result i32)
    (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i64) (local $l11 i64)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l5
    global.set $g0
    i32.const 1
    local.set $l6
    block $B0
      local.get $p0
      i32.load8_u offset=4
      br_if $B0
      local.get $p0
      i32.load8_u offset=5
      local.set $l7
      block $B1
        local.get $p0
        i32.load
        local.tee $l8
        i32.load
        local.tee $l9
        i32.const 4
        i32.and
        br_if $B1
        i32.const 1
        local.set $l6
        local.get $l8
        i32.load offset=24
        i32.const 1055669
        i32.const 1055671
        local.get $l7
        i32.const 255
        i32.and
        local.tee $l7
        select
        i32.const 2
        i32.const 3
        local.get $l7
        select
        local.get $l8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        i32.const 1
        local.set $l6
        local.get $l8
        i32.load offset=24
        local.get $p1
        local.get $p2
        local.get $l8
        i32.load offset=28
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        i32.const 1
        local.set $l6
        local.get $l8
        i32.load offset=24
        i32.const 1055616
        i32.const 2
        local.get $l8
        i32.load offset=28
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        local.get $p3
        local.get $l8
        local.get $p4
        i32.load offset=12
        call_indirect (type $t3) $T0
        local.set $l6
        br $B0
      end
      block $B2
        local.get $l7
        i32.const 255
        i32.and
        br_if $B2
        i32.const 1
        local.set $l6
        local.get $l8
        i32.load offset=24
        i32.const 1055664
        i32.const 3
        local.get $l8
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        local.get $l8
        i32.load
        local.set $l9
      end
      i32.const 1
      local.set $l6
      local.get $l5
      i32.const 1
      i32.store8 offset=23
      local.get $l5
      i32.const 52
      i32.add
      i32.const 1055636
      i32.store
      local.get $l5
      i32.const 16
      i32.add
      local.get $l5
      i32.const 23
      i32.add
      i32.store
      local.get $l5
      local.get $l9
      i32.store offset=24
      local.get $l5
      local.get $l8
      i64.load offset=24 align=4
      i64.store offset=8
      local.get $l8
      i64.load offset=8 align=4
      local.set $l10
      local.get $l8
      i64.load offset=16 align=4
      local.set $l11
      local.get $l5
      local.get $l8
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get $l5
      local.get $l8
      i32.load offset=4
      i32.store offset=28
      local.get $l5
      local.get $l11
      i64.store offset=40
      local.get $l5
      local.get $l10
      i64.store offset=32
      local.get $l5
      local.get $l5
      i32.const 8
      i32.add
      i32.store offset=48
      local.get $l5
      i32.const 8
      i32.add
      local.get $p1
      local.get $p2
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
      br_if $B0
      local.get $l5
      i32.const 8
      i32.add
      i32.const 1055616
      i32.const 2
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
      br_if $B0
      local.get $p3
      local.get $l5
      i32.const 24
      i32.add
      local.get $p4
      i32.load offset=12
      call_indirect (type $t3) $T0
      br_if $B0
      local.get $l5
      i32.load offset=48
      i32.const 1055667
      i32.const 2
      local.get $l5
      i32.load offset=52
      i32.load offset=12
      call_indirect (type $t5) $T0
      local.set $l6
    end
    local.get $p0
    i32.const 1
    i32.store8 offset=5
    local.get $p0
    local.get $l6
    i32.store8 offset=4
    local.get $l5
    i32.const 64
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core6option13expect_failed17h8d17c0af9e73185dE (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p1
    local.get $p2
    call $_ZN4core9panicking9panic_str17h62305f3b2d2b36d0E
    unreachable)
  (func $_ZN4core9panicking9panic_str17h62305f3b2d2b36d0E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l3
    global.set $g0
    local.get $l3
    local.get $p1
    i32.store offset=12
    local.get $l3
    local.get $p0
    i32.store offset=8
    local.get $l3
    i32.const 8
    i32.add
    local.get $p2
    call $_ZN4core9panicking13panic_display17hba1c2a50fab5b809E
    unreachable)
  (func $_ZN70_$LT$core..panic..location..Location$u20$as$u20$core..fmt..Display$GT$3fmt17h679c135f41a338baE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 20
    i32.add
    i32.const 7
    i32.store
    local.get $l2
    i32.const 12
    i32.add
    i32.const 7
    i32.store
    local.get $l2
    i32.const 68
    i32.store offset=4
    local.get $l2
    local.get $p0
    i32.store
    local.get $l2
    local.get $p0
    i32.const 12
    i32.add
    i32.store offset=16
    local.get $l2
    local.get $p0
    i32.const 8
    i32.add
    i32.store offset=8
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    local.set $p0
    local.get $p1
    i32.load offset=24
    local.set $p1
    local.get $l2
    i32.const 24
    i32.add
    i32.const 20
    i32.add
    i32.const 3
    i32.store
    local.get $l2
    i64.const 3
    i64.store offset=28 align=4
    local.get $l2
    i32.const 1055396
    i32.store offset=24
    local.get $l2
    local.get $l2
    i32.store offset=40
    local.get $p1
    local.get $p0
    local.get $l2
    i32.const 24
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p0
    local.get $l2
    i32.const 48
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h8790479cff165904E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p1
    local.get $p0
    i32.load
    local.get $p0
    i32.load offset=4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core5panic10panic_info9PanicInfo7payload17h2544d0125a322a49E (type $t2) (param $p0 i32) (param $p1 i32)
    local.get $p0
    local.get $p1
    i64.load align=4
    i64.store)
  (func $_ZN4core5panic10panic_info9PanicInfo7message17h2e3eed4d70a04eb0E (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load offset=8)
  (func $_ZN4core5panic10panic_info9PanicInfo8location17h8bd40eef2a5c837dE (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load offset=12)
  (func $_ZN4core5panic10panic_info9PanicInfo10can_unwind17h6d20834e3dcd35adE (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load8_u offset=16)
  (func $_ZN73_$LT$core..panic..panic_info..PanicInfo$u20$as$u20$core..fmt..Display$GT$3fmt17h92a87928d361b08eE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l2
    global.set $g0
    i32.const 1
    local.set $l3
    block $B0
      local.get $p1
      i32.load offset=24
      local.tee $l4
      i32.const 1055436
      i32.const 12
      local.get $p1
      i32.const 28
      i32.add
      i32.load
      local.tee $p1
      i32.load offset=12
      call_indirect (type $t5) $T0
      br_if $B0
      block $B1
        block $B2
          local.get $p0
          i32.load offset=8
          local.tee $l3
          i32.eqz
          br_if $B2
          local.get $l2
          local.get $l3
          i32.store offset=12
          local.get $l2
          i32.const 72
          i32.store offset=20
          local.get $l2
          local.get $l2
          i32.const 12
          i32.add
          i32.store offset=16
          i32.const 1
          local.set $l3
          local.get $l2
          i32.const 60
          i32.add
          i32.const 1
          i32.store
          local.get $l2
          i64.const 2
          i64.store offset=44 align=4
          local.get $l2
          i32.const 1055452
          i32.store offset=40
          local.get $l2
          local.get $l2
          i32.const 16
          i32.add
          i32.store offset=56
          local.get $l4
          local.get $p1
          local.get $l2
          i32.const 40
          i32.add
          call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
          i32.eqz
          br_if $B1
          br $B0
        end
        local.get $p0
        i32.load
        local.tee $l3
        local.get $p0
        i32.load offset=4
        i32.load offset=12
        call_indirect (type $t1) $T0
        i64.const -5139102199292759541
        i64.ne
        br_if $B1
        local.get $l2
        local.get $l3
        i32.store offset=12
        local.get $l2
        i32.const 73
        i32.store offset=20
        local.get $l2
        local.get $l2
        i32.const 12
        i32.add
        i32.store offset=16
        i32.const 1
        local.set $l3
        local.get $l2
        i32.const 60
        i32.add
        i32.const 1
        i32.store
        local.get $l2
        i64.const 2
        i64.store offset=44 align=4
        local.get $l2
        i32.const 1055452
        i32.store offset=40
        local.get $l2
        local.get $l2
        i32.const 16
        i32.add
        i32.store offset=56
        local.get $l4
        local.get $p1
        local.get $l2
        i32.const 40
        i32.add
        call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
        br_if $B0
      end
      local.get $p0
      i32.load offset=12
      local.set $l3
      local.get $l2
      i32.const 16
      i32.add
      i32.const 20
      i32.add
      i32.const 7
      i32.store
      local.get $l2
      i32.const 16
      i32.add
      i32.const 12
      i32.add
      i32.const 7
      i32.store
      local.get $l2
      local.get $l3
      i32.const 12
      i32.add
      i32.store offset=32
      local.get $l2
      local.get $l3
      i32.const 8
      i32.add
      i32.store offset=24
      local.get $l2
      i32.const 68
      i32.store offset=20
      local.get $l2
      local.get $l3
      i32.store offset=16
      local.get $l2
      i32.const 40
      i32.add
      i32.const 20
      i32.add
      i32.const 3
      i32.store
      local.get $l2
      i64.const 3
      i64.store offset=44 align=4
      local.get $l2
      i32.const 1055396
      i32.store offset=40
      local.get $l2
      local.get $l2
      i32.const 16
      i32.add
      i32.store offset=56
      local.get $l4
      local.get $p1
      local.get $l2
      i32.const 40
      i32.add
      call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
      local.set $l3
    end
    local.get $l2
    i32.const 64
    i32.add
    global.set $g0
    local.get $l3)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h866d2163a1275370E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    local.set $l3
    local.get $p1
    i32.load offset=24
    local.set $l4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p0
    i32.load
    local.tee $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l4
    local.get $l3
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h0d0f7379d9aa8697E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p1
    local.get $p0
    i32.load
    local.tee $p0
    i32.load
    local.get $p0
    i32.load offset=4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core9panicking13panic_display17hba1c2a50fab5b809E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 20
    i32.add
    i32.const 1
    i32.store
    local.get $l2
    i64.const 1
    i64.store offset=4 align=4
    local.get $l2
    i32.const 1055468
    i32.store
    local.get $l2
    i32.const 68
    i32.store offset=28
    local.get $l2
    local.get $p0
    i32.store offset=24
    local.get $l2
    local.get $l2
    i32.const 24
    i32.add
    i32.store offset=16
    local.get $l2
    local.get $p1
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN4core9panicking19assert_failed_inner17h8bf6bb373c925ba9E (type $t13) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32) (param $p5 i32) (param $p6 i32)
    (local $l7 i32)
    global.get $g0
    i32.const 112
    i32.sub
    local.tee $l7
    global.set $g0
    local.get $l7
    local.get $p2
    i32.store offset=12
    local.get $l7
    local.get $p1
    i32.store offset=8
    local.get $l7
    local.get $p4
    i32.store offset=20
    local.get $l7
    local.get $p3
    i32.store offset=16
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p0
            i32.const 255
            i32.and
            br_table $B3 $B2 $B1 $B3
          end
          local.get $l7
          i32.const 1055485
          i32.store offset=24
          i32.const 2
          local.set $p0
          br $B0
        end
        local.get $l7
        i32.const 1055483
        i32.store offset=24
        i32.const 2
        local.set $p0
        br $B0
      end
      local.get $l7
      i32.const 1055476
      i32.store offset=24
      i32.const 7
      local.set $p0
    end
    local.get $l7
    local.get $p0
    i32.store offset=28
    block $B4
      local.get $p5
      i32.load
      br_if $B4
      local.get $l7
      i32.const 56
      i32.add
      i32.const 20
      i32.add
      i32.const 74
      i32.store
      local.get $l7
      i32.const 68
      i32.add
      i32.const 74
      i32.store
      local.get $l7
      i32.const 88
      i32.add
      i32.const 20
      i32.add
      i32.const 3
      i32.store
      local.get $l7
      i64.const 4
      i64.store offset=92 align=4
      local.get $l7
      i32.const 1055584
      i32.store offset=88
      local.get $l7
      i32.const 68
      i32.store offset=60
      local.get $l7
      local.get $l7
      i32.const 56
      i32.add
      i32.store offset=104
      local.get $l7
      local.get $l7
      i32.const 16
      i32.add
      i32.store offset=72
      local.get $l7
      local.get $l7
      i32.const 8
      i32.add
      i32.store offset=64
      local.get $l7
      local.get $l7
      i32.const 24
      i32.add
      i32.store offset=56
      local.get $l7
      i32.const 88
      i32.add
      local.get $p6
      call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
      unreachable
    end
    local.get $l7
    i32.const 32
    i32.add
    i32.const 16
    i32.add
    local.get $p5
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l7
    i32.const 32
    i32.add
    i32.const 8
    i32.add
    local.get $p5
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l7
    local.get $p5
    i64.load align=4
    i64.store offset=32
    local.get $l7
    i32.const 88
    i32.add
    i32.const 20
    i32.add
    i32.const 4
    i32.store
    local.get $l7
    i32.const 84
    i32.add
    i32.const 12
    i32.store
    local.get $l7
    i32.const 56
    i32.add
    i32.const 20
    i32.add
    i32.const 74
    i32.store
    local.get $l7
    i32.const 68
    i32.add
    i32.const 74
    i32.store
    local.get $l7
    i64.const 4
    i64.store offset=92 align=4
    local.get $l7
    i32.const 1055548
    i32.store offset=88
    local.get $l7
    i32.const 68
    i32.store offset=60
    local.get $l7
    local.get $l7
    i32.const 56
    i32.add
    i32.store offset=104
    local.get $l7
    local.get $l7
    i32.const 32
    i32.add
    i32.store offset=80
    local.get $l7
    local.get $l7
    i32.const 16
    i32.add
    i32.store offset=72
    local.get $l7
    local.get $l7
    i32.const 8
    i32.add
    i32.store offset=64
    local.get $l7
    local.get $l7
    i32.const 24
    i32.add
    i32.store offset=56
    local.get $l7
    i32.const 88
    i32.add
    local.get $p6
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hade274a01ebef0eeE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    local.get $p0
    i32.load offset=4
    i32.load offset=12
    call_indirect (type $t3) $T0)
  (func $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h5e2128c63f339bb4E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    local.set $l3
    local.get $p1
    i32.load offset=24
    local.set $p1
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p0
    i64.load align=4
    i64.store offset=8
    local.get $p1
    local.get $l3
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p0
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core6result13unwrap_failed17hc2b27db9016b40eaE (type $t11) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32)
    (local $l5 i32)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l5
    global.set $g0
    local.get $l5
    local.get $p1
    i32.store offset=12
    local.get $l5
    local.get $p0
    i32.store offset=8
    local.get $l5
    local.get $p3
    i32.store offset=20
    local.get $l5
    local.get $p2
    i32.store offset=16
    local.get $l5
    i32.const 44
    i32.add
    i32.const 2
    i32.store
    local.get $l5
    i32.const 60
    i32.add
    i32.const 74
    i32.store
    local.get $l5
    i64.const 2
    i64.store offset=28 align=4
    local.get $l5
    i32.const 1055620
    i32.store offset=24
    local.get $l5
    i32.const 68
    i32.store offset=52
    local.get $l5
    local.get $l5
    i32.const 48
    i32.add
    i32.store offset=40
    local.get $l5
    local.get $l5
    i32.const 16
    i32.add
    i32.store offset=56
    local.get $l5
    local.get $l5
    i32.const 8
    i32.add
    i32.store offset=48
    local.get $l5
    i32.const 24
    i32.add
    local.get $p4
    call $_ZN4core9panicking9panic_fmt17he0228c274e9a8b0dE
    unreachable)
  (func $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32)
    block $B0
      block $B1
        local.get $p2
        i32.eqz
        br_if $B1
        local.get $p0
        i32.load offset=4
        local.set $l3
        local.get $p0
        i32.load
        local.set $l4
        local.get $p0
        i32.load offset=8
        local.set $l5
        loop $L2
          block $B3
            local.get $l5
            i32.load8_u
            i32.eqz
            br_if $B3
            local.get $l4
            i32.const 1055660
            i32.const 4
            local.get $l3
            i32.load offset=12
            call_indirect (type $t5) $T0
            i32.eqz
            br_if $B3
            i32.const 1
            return
          end
          i32.const 0
          local.set $l6
          local.get $p2
          local.set $l7
          block $B4
            block $B5
              block $B6
                block $B7
                  loop $L8
                    local.get $p1
                    local.get $l6
                    i32.add
                    local.set $l8
                    block $B9
                      block $B10
                        block $B11
                          block $B12
                            block $B13
                              local.get $l7
                              i32.const 8
                              i32.lt_u
                              br_if $B13
                              block $B14
                                local.get $l8
                                i32.const 3
                                i32.add
                                i32.const -4
                                i32.and
                                local.get $l8
                                i32.sub
                                local.tee $p0
                                br_if $B14
                                local.get $l7
                                i32.const -8
                                i32.add
                                local.set $l9
                                i32.const 0
                                local.set $p0
                                br $B11
                              end
                              local.get $l7
                              local.get $p0
                              local.get $p0
                              local.get $l7
                              i32.gt_u
                              select
                              local.set $p0
                              i32.const 0
                              local.set $l10
                              loop $L15
                                local.get $l8
                                local.get $l10
                                i32.add
                                i32.load8_u
                                i32.const 10
                                i32.eq
                                br_if $B9
                                local.get $p0
                                local.get $l10
                                i32.const 1
                                i32.add
                                local.tee $l10
                                i32.eq
                                br_if $B12
                                br $L15
                              end
                            end
                            local.get $l7
                            i32.eqz
                            br_if $B7
                            i32.const 0
                            local.set $l10
                            local.get $l8
                            i32.load8_u
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 1
                            i32.eq
                            br_if $B7
                            i32.const 1
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=1
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 2
                            i32.eq
                            br_if $B7
                            i32.const 2
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=2
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 3
                            i32.eq
                            br_if $B7
                            i32.const 3
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=3
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 4
                            i32.eq
                            br_if $B7
                            i32.const 4
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=4
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 5
                            i32.eq
                            br_if $B7
                            i32.const 5
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=5
                            i32.const 10
                            i32.eq
                            br_if $B9
                            local.get $l7
                            i32.const 6
                            i32.eq
                            br_if $B7
                            i32.const 6
                            local.set $l10
                            local.get $l8
                            i32.load8_u offset=6
                            i32.const 10
                            i32.ne
                            br_if $B7
                            br $B9
                          end
                          local.get $p0
                          local.get $l7
                          i32.const -8
                          i32.add
                          local.tee $l9
                          i32.gt_u
                          br_if $B10
                        end
                        block $B16
                          loop $L17
                            local.get $l8
                            local.get $p0
                            i32.add
                            local.tee $l10
                            i32.load
                            local.tee $l11
                            i32.const -1
                            i32.xor
                            local.get $l11
                            i32.const 168430090
                            i32.xor
                            i32.const -16843009
                            i32.add
                            i32.and
                            local.get $l10
                            i32.const 4
                            i32.add
                            i32.load
                            local.tee $l10
                            i32.const -1
                            i32.xor
                            local.get $l10
                            i32.const 168430090
                            i32.xor
                            i32.const -16843009
                            i32.add
                            i32.and
                            i32.or
                            i32.const -2139062144
                            i32.and
                            br_if $B16
                            local.get $p0
                            i32.const 8
                            i32.add
                            local.tee $p0
                            local.get $l9
                            i32.le_u
                            br_if $L17
                          end
                        end
                        local.get $p0
                        local.get $l7
                        i32.le_u
                        br_if $B10
                        local.get $p0
                        local.get $l7
                        local.get $p0
                        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
                        unreachable
                      end
                      local.get $p0
                      local.get $l7
                      i32.eq
                      br_if $B7
                      local.get $p0
                      local.get $l7
                      i32.sub
                      local.set $l11
                      local.get $l8
                      local.get $p0
                      i32.add
                      local.set $l8
                      i32.const 0
                      local.set $l10
                      block $B18
                        loop $L19
                          local.get $l8
                          local.get $l10
                          i32.add
                          i32.load8_u
                          i32.const 10
                          i32.eq
                          br_if $B18
                          local.get $l11
                          local.get $l10
                          i32.const 1
                          i32.add
                          local.tee $l10
                          i32.add
                          br_if $L19
                          br $B7
                        end
                      end
                      local.get $p0
                      local.get $l10
                      i32.add
                      local.set $l10
                    end
                    block $B20
                      local.get $l10
                      local.get $l6
                      i32.add
                      local.tee $p0
                      i32.const 1
                      i32.add
                      local.tee $l6
                      local.get $p0
                      i32.lt_u
                      br_if $B20
                      local.get $p2
                      local.get $l6
                      i32.lt_u
                      br_if $B20
                      local.get $p1
                      local.get $p0
                      i32.add
                      i32.load8_u
                      i32.const 10
                      i32.ne
                      br_if $B20
                      local.get $l5
                      i32.const 1
                      i32.store8
                      local.get $p2
                      local.get $l6
                      i32.le_u
                      br_if $B6
                      local.get $l6
                      local.set $p0
                      local.get $p1
                      local.get $l6
                      i32.add
                      i32.load8_s
                      i32.const -65
                      i32.le_s
                      br_if $B5
                      br $B4
                    end
                    local.get $p2
                    local.get $l6
                    i32.sub
                    local.set $l7
                    local.get $p2
                    local.get $l6
                    i32.ge_u
                    br_if $L8
                  end
                end
                local.get $l5
                i32.const 0
                i32.store8
                local.get $p2
                local.set $l6
              end
              local.get $p2
              local.set $p0
              local.get $p2
              local.get $l6
              i32.eq
              br_if $B4
            end
            local.get $p1
            local.get $p2
            i32.const 0
            local.get $l6
            local.get $p0
            call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
            unreachable
          end
          block $B21
            local.get $l4
            local.get $p1
            local.get $p0
            local.get $l3
            i32.load offset=12
            call_indirect (type $t5) $T0
            i32.eqz
            br_if $B21
            i32.const 1
            return
          end
          block $B22
            block $B23
              local.get $p2
              local.get $p0
              i32.gt_u
              br_if $B23
              local.get $p2
              local.get $p0
              i32.eq
              br_if $B22
              br $B0
            end
            local.get $p1
            local.get $p0
            i32.add
            i32.load8_s
            i32.const -65
            i32.le_s
            br_if $B0
          end
          local.get $p1
          local.get $p0
          i32.add
          local.set $p1
          local.get $p2
          local.get $p0
          i32.sub
          local.tee $p2
          br_if $L2
        end
      end
      i32.const 0
      return
    end
    local.get $p1
    local.get $p2
    local.get $p0
    local.get $p2
    local.get $p0
    call $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE
    unreachable)
  (func $_ZN4core3str16slice_error_fail17hdcbbde47ffbc972eE (type $t11) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32)
    (local $l5 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l5
    global.set $g0
    local.get $l5
    local.get $p3
    i32.store offset=12
    local.get $l5
    local.get $p2
    i32.store offset=8
    local.get $l5
    local.get $p1
    i32.store offset=4
    local.get $l5
    local.get $p0
    i32.store
    local.get $l5
    call $_ZN4core10intrinsics17const_eval_select17h5024e92cfa62bd24E
    unreachable)
  (func $_ZN4core3fmt8builders11DebugStruct21finish_non_exhaustive17h9dcede22c3d51d75E (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l1
    global.set $g0
    i32.const 1
    local.set $l2
    block $B0
      local.get $p0
      i32.load8_u offset=4
      br_if $B0
      local.get $p0
      i32.load
      local.set $l3
      block $B1
        local.get $p0
        i32.load8_u offset=5
        br_if $B1
        local.get $l3
        i32.load offset=24
        i32.const 1055684
        i32.const 7
        local.get $l3
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        local.set $l2
        br $B0
      end
      block $B2
        local.get $l3
        i32.load8_u
        i32.const 4
        i32.and
        br_if $B2
        local.get $l3
        i32.load offset=24
        i32.const 1055678
        i32.const 6
        local.get $l3
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        local.set $l2
        br $B0
      end
      i32.const 1
      local.set $l2
      local.get $l1
      i32.const 1
      i32.store8 offset=15
      local.get $l1
      i32.const 8
      i32.add
      local.get $l1
      i32.const 15
      i32.add
      i32.store
      local.get $l1
      local.get $l3
      i64.load offset=24 align=4
      i64.store
      local.get $l1
      i32.const 1055674
      i32.const 3
      call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
      br_if $B0
      local.get $l3
      i32.load offset=24
      i32.const 1055677
      i32.const 1
      local.get $l3
      i32.load offset=28
      i32.load offset=12
      call_indirect (type $t5) $T0
      local.set $l2
    end
    local.get $p0
    local.get $l2
    i32.store8 offset=4
    local.get $l1
    i32.const 16
    i32.add
    global.set $g0
    local.get $l2)
  (func $_ZN4core3fmt8builders10DebugTuple5field17h963fe1aaa21a4b0fE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i64) (local $l9 i64)
    global.get $g0
    i32.const 64
    i32.sub
    local.tee $l3
    global.set $g0
    block $B0
      block $B1
        local.get $p0
        i32.load8_u offset=8
        i32.eqz
        br_if $B1
        local.get $p0
        i32.load offset=4
        local.set $l4
        i32.const 1
        local.set $l5
        br $B0
      end
      local.get $p0
      i32.load offset=4
      local.set $l4
      block $B2
        local.get $p0
        i32.load
        local.tee $l6
        i32.load
        local.tee $l7
        i32.const 4
        i32.and
        br_if $B2
        i32.const 1
        local.set $l5
        local.get $l6
        i32.load offset=24
        i32.const 1055669
        i32.const 1055695
        local.get $l4
        select
        i32.const 2
        i32.const 1
        local.get $l4
        select
        local.get $l6
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B0
        local.get $p1
        local.get $l6
        local.get $p2
        i32.load offset=12
        call_indirect (type $t3) $T0
        local.set $l5
        br $B0
      end
      block $B3
        local.get $l4
        br_if $B3
        block $B4
          local.get $l6
          i32.load offset=24
          i32.const 1055693
          i32.const 2
          local.get $l6
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type $t5) $T0
          i32.eqz
          br_if $B4
          i32.const 1
          local.set $l5
          i32.const 0
          local.set $l4
          br $B0
        end
        local.get $l6
        i32.load
        local.set $l7
      end
      i32.const 1
      local.set $l5
      local.get $l3
      i32.const 1
      i32.store8 offset=23
      local.get $l3
      i32.const 52
      i32.add
      i32.const 1055636
      i32.store
      local.get $l3
      i32.const 16
      i32.add
      local.get $l3
      i32.const 23
      i32.add
      i32.store
      local.get $l3
      local.get $l7
      i32.store offset=24
      local.get $l3
      local.get $l6
      i64.load offset=24 align=4
      i64.store offset=8
      local.get $l6
      i64.load offset=8 align=4
      local.set $l8
      local.get $l6
      i64.load offset=16 align=4
      local.set $l9
      local.get $l3
      local.get $l6
      i32.load8_u offset=32
      i32.store8 offset=56
      local.get $l3
      local.get $l6
      i32.load offset=4
      i32.store offset=28
      local.get $l3
      local.get $l9
      i64.store offset=40
      local.get $l3
      local.get $l8
      i64.store offset=32
      local.get $l3
      local.get $l3
      i32.const 8
      i32.add
      i32.store offset=48
      local.get $p1
      local.get $l3
      i32.const 24
      i32.add
      local.get $p2
      i32.load offset=12
      call_indirect (type $t3) $T0
      br_if $B0
      local.get $l3
      i32.load offset=48
      i32.const 1055667
      i32.const 2
      local.get $l3
      i32.load offset=52
      i32.load offset=12
      call_indirect (type $t5) $T0
      local.set $l5
    end
    local.get $p0
    local.get $l5
    i32.store8 offset=8
    local.get $p0
    local.get $l4
    i32.const 1
    i32.add
    i32.store offset=4
    local.get $l3
    i32.const 64
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E (type $t14) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32) (param $p5 i32) (result i32)
    (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32)
    block $B0
      block $B1
        local.get $p1
        i32.eqz
        br_if $B1
        i32.const 43
        i32.const 1114112
        local.get $p0
        i32.load
        local.tee $p1
        i32.const 1
        i32.and
        local.tee $l6
        select
        local.set $l7
        local.get $l6
        local.get $p5
        i32.add
        local.set $l8
        br $B0
      end
      local.get $p5
      i32.const 1
      i32.add
      local.set $l8
      local.get $p0
      i32.load
      local.set $p1
      i32.const 45
      local.set $l7
    end
    block $B2
      block $B3
        local.get $p1
        i32.const 4
        i32.and
        br_if $B3
        i32.const 0
        local.set $p2
        br $B2
      end
      block $B4
        block $B5
          local.get $p3
          i32.const 16
          i32.lt_u
          br_if $B5
          local.get $p2
          local.get $p3
          call $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E
          local.set $l6
          br $B4
        end
        block $B6
          local.get $p3
          br_if $B6
          i32.const 0
          local.set $l6
          br $B4
        end
        local.get $p3
        i32.const 3
        i32.and
        local.set $l9
        block $B7
          block $B8
            local.get $p3
            i32.const -1
            i32.add
            i32.const 3
            i32.ge_u
            br_if $B8
            i32.const 0
            local.set $l6
            local.get $p2
            local.set $p1
            br $B7
          end
          local.get $p3
          i32.const -4
          i32.and
          local.set $l10
          i32.const 0
          local.set $l6
          local.get $p2
          local.set $p1
          loop $L9
            local.get $l6
            local.get $p1
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $p1
            i32.const 1
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $p1
            i32.const 2
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.get $p1
            i32.const 3
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set $l6
            local.get $p1
            i32.const 4
            i32.add
            local.set $p1
            local.get $l10
            i32.const -4
            i32.add
            local.tee $l10
            br_if $L9
          end
        end
        local.get $l9
        i32.eqz
        br_if $B4
        loop $L10
          local.get $l6
          local.get $p1
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set $l6
          local.get $p1
          i32.const 1
          i32.add
          local.set $p1
          local.get $l9
          i32.const -1
          i32.add
          local.tee $l9
          br_if $L10
        end
      end
      local.get $l6
      local.get $l8
      i32.add
      local.set $l8
    end
    block $B11
      block $B12
        local.get $p0
        i32.load offset=8
        br_if $B12
        i32.const 1
        local.set $p1
        local.get $p0
        local.get $l7
        local.get $p2
        local.get $p3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
        br_if $B11
        local.get $p0
        i32.load offset=24
        local.get $p4
        local.get $p5
        local.get $p0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        return
      end
      block $B13
        block $B14
          block $B15
            block $B16
              block $B17
                local.get $p0
                i32.const 12
                i32.add
                i32.load
                local.tee $l6
                local.get $l8
                i32.le_u
                br_if $B17
                local.get $p0
                i32.load8_u
                i32.const 8
                i32.and
                br_if $B13
                i32.const 0
                local.set $p1
                local.get $l6
                local.get $l8
                i32.sub
                local.tee $l9
                local.set $l8
                i32.const 1
                local.get $p0
                i32.load8_u offset=32
                local.tee $l6
                local.get $l6
                i32.const 3
                i32.eq
                select
                i32.const 3
                i32.and
                br_table $B14 $B16 $B15 $B14
              end
              i32.const 1
              local.set $p1
              local.get $p0
              local.get $l7
              local.get $p2
              local.get $p3
              call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
              br_if $B11
              local.get $p0
              i32.load offset=24
              local.get $p4
              local.get $p5
              local.get $p0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect (type $t5) $T0
              return
            end
            i32.const 0
            local.set $l8
            local.get $l9
            local.set $p1
            br $B14
          end
          local.get $l9
          i32.const 1
          i32.shr_u
          local.set $p1
          local.get $l9
          i32.const 1
          i32.add
          i32.const 1
          i32.shr_u
          local.set $l8
        end
        local.get $p1
        i32.const 1
        i32.add
        local.set $p1
        local.get $p0
        i32.const 28
        i32.add
        i32.load
        local.set $l9
        local.get $p0
        i32.load offset=4
        local.set $l6
        local.get $p0
        i32.load offset=24
        local.set $l10
        block $B18
          loop $L19
            local.get $p1
            i32.const -1
            i32.add
            local.tee $p1
            i32.eqz
            br_if $B18
            local.get $l10
            local.get $l6
            local.get $l9
            i32.load offset=16
            call_indirect (type $t3) $T0
            i32.eqz
            br_if $L19
          end
          i32.const 1
          return
        end
        i32.const 1
        local.set $p1
        local.get $l6
        i32.const 1114112
        i32.eq
        br_if $B11
        local.get $p0
        local.get $l7
        local.get $p2
        local.get $p3
        call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
        br_if $B11
        local.get $p0
        i32.load offset=24
        local.get $p4
        local.get $p5
        local.get $p0
        i32.load offset=28
        i32.load offset=12
        call_indirect (type $t5) $T0
        br_if $B11
        local.get $p0
        i32.load offset=28
        local.set $l9
        local.get $p0
        i32.load offset=24
        local.set $p0
        i32.const 0
        local.set $p1
        block $B20
          loop $L21
            block $B22
              local.get $l8
              local.get $p1
              i32.ne
              br_if $B22
              local.get $l8
              local.set $p1
              br $B20
            end
            local.get $p1
            i32.const 1
            i32.add
            local.set $p1
            local.get $p0
            local.get $l6
            local.get $l9
            i32.load offset=16
            call_indirect (type $t3) $T0
            i32.eqz
            br_if $L21
          end
          local.get $p1
          i32.const -1
          i32.add
          local.set $p1
        end
        local.get $p1
        local.get $l8
        i32.lt_u
        local.set $p1
        br $B11
      end
      local.get $p0
      i32.load offset=4
      local.set $l11
      local.get $p0
      i32.const 48
      i32.store offset=4
      local.get $p0
      i32.load8_u offset=32
      local.set $l12
      i32.const 1
      local.set $p1
      local.get $p0
      i32.const 1
      i32.store8 offset=32
      local.get $p0
      local.get $l7
      local.get $p2
      local.get $p3
      call $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E
      br_if $B11
      i32.const 0
      local.set $p1
      local.get $l6
      local.get $l8
      i32.sub
      local.tee $l9
      local.set $p3
      block $B23
        block $B24
          block $B25
            i32.const 1
            local.get $p0
            i32.load8_u offset=32
            local.tee $l6
            local.get $l6
            i32.const 3
            i32.eq
            select
            i32.const 3
            i32.and
            br_table $B23 $B25 $B24 $B23
          end
          i32.const 0
          local.set $p3
          local.get $l9
          local.set $p1
          br $B23
        end
        local.get $l9
        i32.const 1
        i32.shr_u
        local.set $p1
        local.get $l9
        i32.const 1
        i32.add
        i32.const 1
        i32.shr_u
        local.set $p3
      end
      local.get $p1
      i32.const 1
      i32.add
      local.set $p1
      local.get $p0
      i32.const 28
      i32.add
      i32.load
      local.set $l9
      local.get $p0
      i32.load offset=4
      local.set $l6
      local.get $p0
      i32.load offset=24
      local.set $l10
      block $B26
        loop $L27
          local.get $p1
          i32.const -1
          i32.add
          local.tee $p1
          i32.eqz
          br_if $B26
          local.get $l10
          local.get $l6
          local.get $l9
          i32.load offset=16
          call_indirect (type $t3) $T0
          i32.eqz
          br_if $L27
        end
        i32.const 1
        return
      end
      i32.const 1
      local.set $p1
      local.get $l6
      i32.const 1114112
      i32.eq
      br_if $B11
      local.get $p0
      i32.load offset=24
      local.get $p4
      local.get $p5
      local.get $p0
      i32.load offset=28
      i32.load offset=12
      call_indirect (type $t5) $T0
      br_if $B11
      local.get $p0
      i32.load offset=28
      local.set $p1
      local.get $p0
      i32.load offset=24
      local.set $l10
      i32.const 0
      local.set $l9
      block $B28
        loop $L29
          local.get $p3
          local.get $l9
          i32.eq
          br_if $B28
          local.get $l9
          i32.const 1
          i32.add
          local.set $l9
          local.get $l10
          local.get $l6
          local.get $p1
          i32.load offset=16
          call_indirect (type $t3) $T0
          i32.eqz
          br_if $L29
        end
        i32.const 1
        local.set $p1
        local.get $l9
        i32.const -1
        i32.add
        local.get $p3
        i32.lt_u
        br_if $B11
      end
      local.get $p0
      local.get $l12
      i32.store8 offset=32
      local.get $p0
      local.get $l11
      i32.store offset=4
      i32.const 0
      return
    end
    local.get $p1)
  (func $_ZN4core3fmt5Write10write_char17h13cfc975967354caE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    i32.const 0
    i32.store offset=12
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=12
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set $p1
    end
    local.get $p0
    local.get $l2
    i32.const 12
    i32.add
    local.get $p1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
    local.set $p1
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN4core3fmt5Write9write_fmt17hdb67d064950d9d5bE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1055920
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h84bb0430ffe0ae70E (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    local.get $p2
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5b8cfb95cae022fbE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load
    local.set $p0
    local.get $l2
    i32.const 0
    i32.store offset=12
    block $B0
      block $B1
        block $B2
          block $B3
            local.get $p1
            i32.const 128
            i32.lt_u
            br_if $B3
            local.get $p1
            i32.const 2048
            i32.lt_u
            br_if $B2
            local.get $p1
            i32.const 65536
            i32.ge_u
            br_if $B1
            local.get $l2
            local.get $p1
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=14
            local.get $l2
            local.get $p1
            i32.const 12
            i32.shr_u
            i32.const 224
            i32.or
            i32.store8 offset=12
            local.get $l2
            local.get $p1
            i32.const 6
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=13
            i32.const 3
            local.set $p1
            br $B0
          end
          local.get $l2
          local.get $p1
          i32.store8 offset=12
          i32.const 1
          local.set $p1
          br $B0
        end
        local.get $l2
        local.get $p1
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        local.get $l2
        local.get $p1
        i32.const 6
        i32.shr_u
        i32.const 192
        i32.or
        i32.store8 offset=12
        i32.const 2
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=15
      local.get $l2
      local.get $p1
      i32.const 18
      i32.shr_u
      i32.const 240
      i32.or
      i32.store8 offset=12
      local.get $l2
      local.get $p1
      i32.const 6
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=14
      local.get $l2
      local.get $p1
      i32.const 12
      i32.shr_u
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8 offset=13
      i32.const 4
      local.set $p1
    end
    local.get $p0
    local.get $l2
    i32.const 12
    i32.add
    local.get $p1
    call $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E
    local.set $p1
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h532266dfb8016bcfE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $l2
    local.get $p0
    i32.load
    i32.store offset=4
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $l2
    i32.const 4
    i32.add
    i32.const 1055920
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN4core3str5count14do_count_chars17h99d3b68dcb45cf00E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32)
    block $B0
      block $B1
        local.get $p0
        i32.const 3
        i32.add
        i32.const -4
        i32.and
        local.tee $l2
        local.get $p0
        i32.sub
        local.tee $l3
        local.get $p1
        i32.gt_u
        br_if $B1
        local.get $l3
        i32.const 4
        i32.gt_u
        br_if $B1
        local.get $p1
        local.get $l3
        i32.sub
        local.tee $l4
        i32.const 4
        i32.lt_u
        br_if $B1
        local.get $l4
        i32.const 3
        i32.and
        local.set $l5
        i32.const 0
        local.set $l6
        i32.const 0
        local.set $p1
        block $B2
          local.get $l3
          i32.eqz
          br_if $B2
          local.get $l3
          i32.const 3
          i32.and
          local.set $l7
          block $B3
            block $B4
              local.get $l2
              local.get $p0
              i32.const -1
              i32.xor
              i32.add
              i32.const 3
              i32.ge_u
              br_if $B4
              i32.const 0
              local.set $p1
              local.get $p0
              local.set $l2
              br $B3
            end
            local.get $l3
            i32.const -4
            i32.and
            local.set $l8
            i32.const 0
            local.set $p1
            local.get $p0
            local.set $l2
            loop $L5
              local.get $p1
              local.get $l2
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get $l2
              i32.const 1
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get $l2
              i32.const 2
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.get $l2
              i32.const 3
              i32.add
              i32.load8_s
              i32.const -65
              i32.gt_s
              i32.add
              local.set $p1
              local.get $l2
              i32.const 4
              i32.add
              local.set $l2
              local.get $l8
              i32.const -4
              i32.add
              local.tee $l8
              br_if $L5
            end
          end
          local.get $l7
          i32.eqz
          br_if $B2
          loop $L6
            local.get $p1
            local.get $l2
            i32.load8_s
            i32.const -65
            i32.gt_s
            i32.add
            local.set $p1
            local.get $l2
            i32.const 1
            i32.add
            local.set $l2
            local.get $l7
            i32.const -1
            i32.add
            local.tee $l7
            br_if $L6
          end
        end
        local.get $p0
        local.get $l3
        i32.add
        local.set $p0
        block $B7
          local.get $l5
          i32.eqz
          br_if $B7
          local.get $p0
          local.get $l4
          i32.const -4
          i32.and
          i32.add
          local.tee $l2
          i32.load8_s
          i32.const -65
          i32.gt_s
          local.set $l6
          local.get $l5
          i32.const 1
          i32.eq
          br_if $B7
          local.get $l6
          local.get $l2
          i32.load8_s offset=1
          i32.const -65
          i32.gt_s
          i32.add
          local.set $l6
          local.get $l5
          i32.const 2
          i32.eq
          br_if $B7
          local.get $l6
          local.get $l2
          i32.load8_s offset=2
          i32.const -65
          i32.gt_s
          i32.add
          local.set $l6
        end
        local.get $l4
        i32.const 2
        i32.shr_u
        local.set $l3
        local.get $l6
        local.get $p1
        i32.add
        local.set $l8
        loop $L8
          local.get $p0
          local.set $l6
          local.get $l3
          i32.eqz
          br_if $B0
          local.get $l3
          i32.const 192
          local.get $l3
          i32.const 192
          i32.lt_u
          select
          local.tee $l4
          i32.const 3
          i32.and
          local.set $l5
          local.get $l4
          i32.const 2
          i32.shl
          local.set $l9
          block $B9
            block $B10
              local.get $l4
              i32.const 252
              i32.and
              local.tee $l10
              i32.const 2
              i32.shl
              local.tee $p0
              br_if $B10
              i32.const 0
              local.set $l2
              br $B9
            end
            local.get $l6
            local.get $p0
            i32.add
            local.set $l7
            i32.const 0
            local.set $l2
            local.get $l6
            local.set $p0
            loop $L11
              local.get $p0
              i32.const 12
              i32.add
              i32.load
              local.tee $p1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get $p1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get $p0
              i32.const 8
              i32.add
              i32.load
              local.tee $p1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get $p1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get $p0
              i32.const 4
              i32.add
              i32.load
              local.tee $p1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get $p1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get $p0
              i32.load
              local.tee $p1
              i32.const -1
              i32.xor
              i32.const 7
              i32.shr_u
              local.get $p1
              i32.const 6
              i32.shr_u
              i32.or
              i32.const 16843009
              i32.and
              local.get $l2
              i32.add
              i32.add
              i32.add
              i32.add
              local.set $l2
              local.get $p0
              i32.const 16
              i32.add
              local.tee $p0
              local.get $l7
              i32.ne
              br_if $L11
            end
          end
          local.get $l6
          local.get $l9
          i32.add
          local.set $p0
          local.get $l3
          local.get $l4
          i32.sub
          local.set $l3
          local.get $l2
          i32.const 8
          i32.shr_u
          i32.const 16711935
          i32.and
          local.get $l2
          i32.const 16711935
          i32.and
          i32.add
          i32.const 65537
          i32.mul
          i32.const 16
          i32.shr_u
          local.get $l8
          i32.add
          local.set $l8
          local.get $l5
          i32.eqz
          br_if $L8
        end
        local.get $l6
        local.get $l10
        i32.const 2
        i32.shl
        i32.add
        local.set $p0
        local.get $l5
        i32.const 1073741823
        i32.add
        local.tee $l4
        i32.const 1073741823
        i32.and
        local.tee $l2
        i32.const 1
        i32.add
        local.tee $p1
        i32.const 3
        i32.and
        local.set $l3
        block $B12
          block $B13
            local.get $l2
            i32.const 3
            i32.ge_u
            br_if $B13
            i32.const 0
            local.set $l2
            br $B12
          end
          local.get $p1
          i32.const 2147483644
          i32.and
          local.set $p1
          i32.const 0
          local.set $l2
          loop $L14
            local.get $p0
            i32.const 12
            i32.add
            i32.load
            local.tee $l7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get $l7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get $p0
            i32.const 8
            i32.add
            i32.load
            local.tee $l7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get $l7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get $p0
            i32.const 4
            i32.add
            i32.load
            local.tee $l7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get $l7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get $p0
            i32.load
            local.tee $l7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get $l7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get $l2
            i32.add
            i32.add
            i32.add
            i32.add
            local.set $l2
            local.get $p0
            i32.const 16
            i32.add
            local.set $p0
            local.get $p1
            i32.const -4
            i32.add
            local.tee $p1
            br_if $L14
          end
        end
        block $B15
          local.get $l3
          i32.eqz
          br_if $B15
          local.get $l4
          i32.const -1073741823
          i32.add
          local.set $p1
          loop $L16
            local.get $p0
            i32.load
            local.tee $l7
            i32.const -1
            i32.xor
            i32.const 7
            i32.shr_u
            local.get $l7
            i32.const 6
            i32.shr_u
            i32.or
            i32.const 16843009
            i32.and
            local.get $l2
            i32.add
            local.set $l2
            local.get $p0
            i32.const 4
            i32.add
            local.set $p0
            local.get $p1
            i32.const -1
            i32.add
            local.tee $p1
            br_if $L16
          end
        end
        local.get $l2
        i32.const 8
        i32.shr_u
        i32.const 16711935
        i32.and
        local.get $l2
        i32.const 16711935
        i32.and
        i32.add
        i32.const 65537
        i32.mul
        i32.const 16
        i32.shr_u
        local.get $l8
        i32.add
        return
      end
      block $B17
        local.get $p1
        br_if $B17
        i32.const 0
        return
      end
      local.get $p1
      i32.const 3
      i32.and
      local.set $l2
      block $B18
        block $B19
          local.get $p1
          i32.const -1
          i32.add
          i32.const 3
          i32.ge_u
          br_if $B19
          i32.const 0
          local.set $l8
          br $B18
        end
        local.get $p1
        i32.const -4
        i32.and
        local.set $p1
        i32.const 0
        local.set $l8
        loop $L20
          local.get $l8
          local.get $p0
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get $p0
          i32.const 1
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get $p0
          i32.const 2
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.get $p0
          i32.const 3
          i32.add
          i32.load8_s
          i32.const -65
          i32.gt_s
          i32.add
          local.set $l8
          local.get $p0
          i32.const 4
          i32.add
          local.set $p0
          local.get $p1
          i32.const -4
          i32.add
          local.tee $p1
          br_if $L20
        end
      end
      local.get $l2
      i32.eqz
      br_if $B0
      loop $L21
        local.get $l8
        local.get $p0
        i32.load8_s
        i32.const -65
        i32.gt_s
        i32.add
        local.set $l8
        local.get $p0
        i32.const 1
        i32.add
        local.set $p0
        local.get $l2
        i32.const -1
        i32.add
        local.tee $l2
        br_if $L21
      end
    end
    local.get $l8)
  (func $_ZN4core3fmt9Formatter12pad_integral12write_prefix17h543cc78c434fbe99E (type $t6) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (result i32)
    (local $l4 i32)
    block $B0
      block $B1
        block $B2
          local.get $p1
          i32.const 1114112
          i32.eq
          br_if $B2
          i32.const 1
          local.set $l4
          local.get $p0
          i32.load offset=24
          local.get $p1
          local.get $p0
          i32.const 28
          i32.add
          i32.load
          i32.load offset=16
          call_indirect (type $t3) $T0
          br_if $B1
        end
        local.get $p2
        br_if $B0
        i32.const 0
        local.set $l4
      end
      local.get $l4
      return
    end
    local.get $p0
    i32.load offset=24
    local.get $p2
    local.get $p3
    local.get $p0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type $t5) $T0)
  (func $_ZN4core3fmt9Formatter9write_str17hbf098ed10828360dE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    local.get $p0
    i32.load offset=24
    local.get $p1
    local.get $p2
    local.get $p0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type $t5) $T0)
  (func $_ZN4core3fmt9Formatter9write_fmt17h818234b174645facE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 32
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.const 28
    i32.add
    i32.load
    local.set $l3
    local.get $p0
    i32.load offset=24
    local.set $p0
    local.get $l2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    local.get $p1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    local.get $p1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    local.get $l2
    local.get $p1
    i64.load align=4
    i64.store offset=8
    local.get $p0
    local.get $l3
    local.get $l2
    i32.const 8
    i32.add
    call $_ZN4core3fmt5write17ha48fcb73fabd1d51E
    local.set $p1
    local.get $l2
    i32.const 32
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN4core3fmt9Formatter15debug_lower_hex17h633f97836530945dE (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load8_u
    i32.const 16
    i32.and
    i32.const 4
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter15debug_upper_hex17h565dd489bc473806E (type $t10) (param $p0 i32) (result i32)
    local.get $p0
    i32.load8_u
    i32.const 32
    i32.and
    i32.const 5
    i32.shr_u)
  (func $_ZN4core3fmt9Formatter12debug_struct17h678028cd4a965ad7E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    local.get $p1
    i32.load offset=24
    local.get $p2
    local.get $p3
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type $t5) $T0
    local.set $p2
    local.get $p0
    i32.const 0
    i32.store8 offset=5
    local.get $p0
    local.get $p2
    i32.store8 offset=4
    local.get $p0
    local.get $p1
    i32.store)
  (func $_ZN57_$LT$core..fmt..Formatter$u20$as$u20$core..fmt..Write$GT$10write_char17h968628441d2ac37cE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load offset=24
    local.get $p1
    local.get $p0
    i32.const 28
    i32.add
    i32.load
    i32.load offset=16
    call_indirect (type $t3) $T0)
  (func $_ZN43_$LT$bool$u20$as$u20$core..fmt..Display$GT$3fmt17h107f9e8c546bfafcE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    block $B0
      local.get $p0
      i32.load8_u
      br_if $B0
      local.get $p1
      i32.const 1055948
      i32.const 5
      call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E
      return
    end
    local.get $p1
    i32.const 1055944
    i32.const 4
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32) (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32)
    local.get $p0
    i32.const 11
    i32.shl
    local.set $l1
    i32.const 0
    local.set $l2
    i32.const 32
    local.set $l3
    i32.const 32
    local.set $l4
    block $B0
      block $B1
        loop $L2
          block $B3
            block $B4
              local.get $l3
              i32.const 1
              i32.shr_u
              local.get $l2
              i32.add
              local.tee $l3
              i32.const 2
              i32.shl
              i32.const 1058364
              i32.add
              i32.load
              i32.const 11
              i32.shl
              local.tee $l5
              local.get $l1
              i32.lt_u
              br_if $B4
              local.get $l5
              local.get $l1
              i32.eq
              br_if $B1
              local.get $l3
              local.set $l4
              br $B3
            end
            local.get $l3
            i32.const 1
            i32.add
            local.set $l2
          end
          local.get $l4
          local.get $l2
          i32.sub
          local.set $l3
          local.get $l4
          local.get $l2
          i32.gt_u
          br_if $L2
          br $B0
        end
      end
      local.get $l3
      i32.const 1
      i32.add
      local.set $l2
    end
    block $B5
      block $B6
        block $B7
          local.get $l2
          i32.const 31
          i32.gt_u
          br_if $B7
          local.get $l2
          i32.const 2
          i32.shl
          local.set $l3
          i32.const 707
          local.set $l4
          block $B8
            local.get $l2
            i32.const 31
            i32.eq
            br_if $B8
            local.get $l3
            i32.const 1058368
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.set $l4
          end
          i32.const 0
          local.set $l1
          block $B9
            local.get $l2
            i32.const -1
            i32.add
            local.tee $l5
            local.get $l2
            i32.gt_u
            br_if $B9
            local.get $l5
            i32.const 32
            i32.ge_u
            br_if $B6
            local.get $l5
            i32.const 2
            i32.shl
            i32.const 1058364
            i32.add
            i32.load
            i32.const 2097151
            i32.and
            local.set $l1
          end
          block $B10
            local.get $l4
            local.get $l3
            i32.const 1058364
            i32.add
            i32.load
            i32.const 21
            i32.shr_u
            local.tee $l2
            i32.const -1
            i32.xor
            i32.add
            i32.eqz
            br_if $B10
            local.get $p0
            local.get $l1
            i32.sub
            local.set $l1
            local.get $l2
            i32.const 707
            local.get $l2
            i32.const 707
            i32.gt_u
            select
            local.set $l3
            local.get $l4
            i32.const -1
            i32.add
            local.set $l5
            i32.const 0
            local.set $l4
            loop $L11
              local.get $l3
              local.get $l2
              i32.eq
              br_if $B5
              local.get $l4
              local.get $l2
              i32.const 1058492
              i32.add
              i32.load8_u
              i32.add
              local.tee $l4
              local.get $l1
              i32.gt_u
              br_if $B10
              local.get $l5
              local.get $l2
              i32.const 1
              i32.add
              local.tee $l2
              i32.ne
              br_if $L11
            end
            local.get $l5
            local.set $l2
          end
          local.get $l2
          i32.const 1
          i32.and
          return
        end
        local.get $l2
        i32.const 32
        i32.const 1058244
        call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
        unreachable
      end
      local.get $l5
      i32.const 32
      i32.const 1058276
      call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
      unreachable
    end
    local.get $l3
    i32.const 707
    i32.const 1058260
    call $_ZN4core9panicking18panic_bounds_check17h7c1d752b88f3d981E
    unreachable)
  (func $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE (type $t10) (param $p0 i32) (result i32)
    (local $l1 i32)
    i32.const 0
    local.set $l1
    block $B0
      block $B1
        local.get $p0
        i32.const 32
        i32.lt_u
        br_if $B1
        i32.const 1
        local.set $l1
        local.get $p0
        i32.const 127
        i32.lt_u
        br_if $B1
        local.get $p0
        i32.const 65536
        i32.lt_u
        br_if $B0
        block $B2
          local.get $p0
          i32.const 131072
          i32.lt_u
          br_if $B2
          local.get $p0
          i32.const 2097150
          i32.and
          i32.const 178206
          i32.ne
          local.get $p0
          i32.const 2097120
          i32.and
          i32.const 173792
          i32.ne
          local.get $p0
          i32.const -177977
          i32.add
          i32.const 6
          i32.gt_u
          i32.and
          i32.and
          local.get $p0
          i32.const -183984
          i32.add
          i32.const -14
          i32.lt_u
          i32.and
          local.get $p0
          i32.const -194560
          i32.add
          i32.const -3103
          i32.lt_u
          i32.and
          local.get $p0
          i32.const -196608
          i32.add
          i32.const -1506
          i32.lt_u
          i32.and
          local.get $p0
          i32.const -917760
          i32.add
          i32.const -716213
          i32.lt_u
          i32.and
          local.get $p0
          i32.const 918000
          i32.lt_u
          i32.and
          return
        end
        local.get $p0
        i32.const 1057487
        i32.const 42
        i32.const 1057571
        i32.const 192
        i32.const 1057763
        i32.const 438
        call $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE
        local.set $l1
      end
      local.get $l1
      return
    end
    local.get $p0
    i32.const 1056816
    i32.const 40
    i32.const 1056896
    i32.const 288
    i32.const 1057184
    i32.const 303
    call $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE)
  (func $_ZN42_$LT$str$u20$as$u20$core..fmt..Display$GT$3fmt17hca79c6a7020710ddE (type $t5) (param $p0 i32) (param $p1 i32) (param $p2 i32) (result i32)
    local.get $p2
    local.get $p0
    local.get $p1
    call $_ZN4core3fmt9Formatter3pad17h5a089571fb8448f6E)
  (func $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hf9cabbcc6a6076ccE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i64) (local $l7 i32)
    i32.const 1
    local.set $l2
    block $B0
      local.get $p1
      i32.load offset=24
      local.tee $l3
      i32.const 39
      local.get $p1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=16
      local.tee $l4
      call_indirect (type $t3) $T0
      br_if $B0
      i32.const 2
      local.set $p1
      i32.const 48
      local.set $l5
      block $B1
        block $B2
          block $B3
            block $B4
              block $B5
                block $B6
                  block $B7
                    block $B8
                      block $B9
                        local.get $p0
                        i32.load
                        local.tee $p0
                        br_table $B1 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B7 $B5 $B8 $B8 $B6 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B8 $B4 $B9
                      end
                      local.get $p0
                      i32.const 92
                      i32.eq
                      br_if $B4
                    end
                    local.get $p0
                    call $_ZN4core7unicode12unicode_data15grapheme_extend6lookup17h39b85920c9f665d6E
                    i32.eqz
                    br_if $B3
                    local.get $p0
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
                    local.set $l6
                    br $B2
                  end
                  i32.const 116
                  local.set $l5
                  i32.const 2
                  local.set $p1
                  br $B1
                end
                i32.const 114
                local.set $l5
                i32.const 2
                local.set $p1
                br $B1
              end
              i32.const 110
              local.set $l5
              i32.const 2
              local.set $p1
              br $B1
            end
            i32.const 2
            local.set $p1
            local.get $p0
            local.set $l5
            br $B1
          end
          block $B10
            local.get $p0
            call $_ZN4core7unicode9printable12is_printable17h99649bbcba1e3d2dE
            i32.eqz
            br_if $B10
            i32.const 1
            local.set $p1
            local.get $p0
            local.set $l5
            br $B1
          end
          local.get $p0
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
          local.set $l6
        end
        i32.const 3
        local.set $p1
        local.get $p0
        local.set $l5
      end
      loop $L11
        local.get $p1
        local.set $l7
        i32.const 0
        local.set $p1
        local.get $l5
        local.set $p0
        block $B12
          block $B13
            block $B14
              block $B15
                block $B16
                  local.get $l7
                  br_table $B15 $B12 $B14 $B16 $B15
                end
                block $B17
                  block $B18
                    block $B19
                      block $B20
                        block $B21
                          local.get $l6
                          i64.const 32
                          i64.shr_u
                          i32.wrap_i64
                          i32.const 255
                          i32.and
                          br_table $B15 $B21 $B17 $B20 $B19 $B18 $B15
                        end
                        local.get $l6
                        i64.const -1095216660481
                        i64.and
                        local.set $l6
                        i32.const 125
                        local.set $p0
                        i32.const 3
                        local.set $p1
                        br $B12
                      end
                      local.get $l6
                      i64.const -1095216660481
                      i64.and
                      i64.const 8589934592
                      i64.or
                      local.set $l6
                      i32.const 123
                      local.set $p0
                      i32.const 3
                      local.set $p1
                      br $B12
                    end
                    local.get $l6
                    i64.const -1095216660481
                    i64.and
                    i64.const 12884901888
                    i64.or
                    local.set $l6
                    i32.const 117
                    local.set $p0
                    i32.const 3
                    local.set $p1
                    br $B12
                  end
                  local.get $l6
                  i64.const -1095216660481
                  i64.and
                  i64.const 17179869184
                  i64.or
                  local.set $l6
                  i32.const 92
                  local.set $p0
                  i32.const 3
                  local.set $p1
                  br $B12
                end
                i32.const 48
                i32.const 87
                local.get $l5
                local.get $l6
                i32.wrap_i64
                local.tee $p1
                i32.const 2
                i32.shl
                i32.shr_u
                i32.const 15
                i32.and
                local.tee $p0
                i32.const 10
                i32.lt_u
                select
                local.get $p0
                i32.add
                local.set $p0
                local.get $p1
                i32.eqz
                br_if $B13
                local.get $l6
                i64.const -1
                i64.add
                i64.const 4294967295
                i64.and
                local.get $l6
                i64.const -4294967296
                i64.and
                i64.or
                local.set $l6
                i32.const 3
                local.set $p1
                br $B12
              end
              local.get $l3
              i32.const 39
              local.get $l4
              call_indirect (type $t3) $T0
              local.set $l2
              br $B0
            end
            i32.const 92
            local.set $p0
            i32.const 1
            local.set $p1
            br $B12
          end
          local.get $l6
          i64.const -1095216660481
          i64.and
          i64.const 4294967296
          i64.or
          local.set $l6
          i32.const 3
          local.set $p1
        end
        local.get $l3
        local.get $p0
        local.get $l4
        call_indirect (type $t3) $T0
        i32.eqz
        br_if $L11
      end
    end
    local.get $l2)
  (func $_ZN4core5slice6memchr7memrchr17heb9113f3074b2e71E (type $t8) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32)
    (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32)
    local.get $p3
    i32.const 0
    local.get $p3
    local.get $p2
    i32.const 3
    i32.add
    i32.const -4
    i32.and
    local.get $p2
    i32.sub
    local.tee $l4
    i32.sub
    i32.const 7
    i32.and
    local.get $p3
    local.get $l4
    i32.lt_u
    local.tee $l5
    select
    local.tee $l6
    i32.sub
    local.set $l7
    block $B0
      block $B1
        local.get $p3
        local.get $l6
        i32.lt_u
        br_if $B1
        block $B2
          block $B3
            block $B4
              local.get $l6
              i32.eqz
              br_if $B4
              local.get $p2
              local.get $p3
              i32.add
              local.tee $l6
              local.get $p2
              local.get $l7
              i32.add
              local.tee $l8
              i32.sub
              local.set $l9
              block $B5
                local.get $l6
                i32.const -1
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B5
                local.get $l9
                i32.const -1
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B6
                local.get $l6
                i32.const -2
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B6
                local.get $l9
                i32.const -2
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B7
                local.get $l6
                i32.const -3
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B7
                local.get $l9
                i32.const -3
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B8
                local.get $l6
                i32.const -4
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B8
                local.get $l9
                i32.const -4
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B9
                local.get $l6
                i32.const -5
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B9
                local.get $l9
                i32.const -5
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B10
                local.get $l6
                i32.const -6
                i32.add
                local.tee $l10
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B10
                local.get $l9
                i32.const -6
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l10
              i32.eq
              br_if $B4
              block $B11
                local.get $l6
                i32.const -7
                i32.add
                local.tee $l6
                i32.load8_u
                local.get $p1
                i32.const 255
                i32.and
                i32.ne
                br_if $B11
                local.get $l9
                i32.const -7
                i32.add
                local.get $l7
                i32.add
                local.set $l6
                br $B3
              end
              local.get $l8
              local.get $l6
              i32.eq
              br_if $B4
              local.get $l9
              i32.const -8
              i32.add
              local.get $l7
              i32.add
              local.set $l6
              br $B3
            end
            local.get $p3
            local.get $l4
            local.get $l5
            select
            local.set $l8
            local.get $p1
            i32.const 255
            i32.and
            i32.const 16843009
            i32.mul
            local.set $l4
            block $B12
              loop $L13
                local.get $l7
                local.tee $l6
                local.get $l8
                i32.le_u
                br_if $B12
                local.get $l6
                i32.const -8
                i32.add
                local.set $l7
                local.get $p2
                local.get $l6
                i32.add
                local.tee $l5
                i32.const -8
                i32.add
                i32.load
                local.get $l4
                i32.xor
                local.tee $l9
                i32.const -1
                i32.xor
                local.get $l9
                i32.const -16843009
                i32.add
                i32.and
                local.get $l5
                i32.const -4
                i32.add
                i32.load
                local.get $l4
                i32.xor
                local.tee $l5
                i32.const -1
                i32.xor
                local.get $l5
                i32.const -16843009
                i32.add
                i32.and
                i32.or
                i32.const -2139062144
                i32.and
                i32.eqz
                br_if $L13
              end
            end
            local.get $l6
            local.get $p3
            i32.gt_u
            br_if $B0
            local.get $p2
            i32.const -1
            i32.add
            local.set $l4
            local.get $p1
            i32.const 255
            i32.and
            local.set $l5
            loop $L14
              block $B15
                local.get $l6
                br_if $B15
                i32.const 0
                local.set $l7
                br $B2
              end
              local.get $l4
              local.get $l6
              i32.add
              local.set $l7
              local.get $l6
              i32.const -1
              i32.add
              local.set $l6
              local.get $l7
              i32.load8_u
              local.get $l5
              i32.ne
              br_if $L14
            end
          end
          i32.const 1
          local.set $l7
        end
        local.get $p0
        local.get $l6
        i32.store offset=4
        local.get $p0
        local.get $l7
        i32.store
        return
      end
      local.get $l7
      local.get $p3
      local.get $l6
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $l6
    local.get $p3
    local.get $l6
    call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
    unreachable)
  (func $_ZN4core3str5lossy9Utf8Lossy10from_bytes17h9d8ade7d6641c0d4E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p2
    i32.store offset=4
    local.get $p0
    local.get $p1
    i32.store)
  (func $_ZN4core3str5lossy9Utf8Lossy6chunks17hdeccf1496c5995d4E (type $t4) (param $p0 i32) (param $p1 i32) (param $p2 i32)
    local.get $p0
    local.get $p2
    i32.store offset=4
    local.get $p0
    local.get $p1
    i32.store)
  (func $_ZN96_$LT$core..str..lossy..Utf8LossyChunksIter$u20$as$u20$core..iter..traits..iterator..Iterator$GT$4next17h6f6f2f836f7026a8E (type $t2) (param $p0 i32) (param $p1 i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32) (local $l5 i32) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    block $B0
      local.get $p1
      i32.load offset=4
      local.tee $l2
      i32.eqz
      br_if $B0
      local.get $p1
      i32.load
      local.set $l3
      i32.const 0
      local.set $l4
      block $B1
        loop $L2
          local.get $l4
          i32.const 1
          i32.add
          local.set $l5
          block $B3
            block $B4
              local.get $l3
              local.get $l4
              i32.add
              i32.load8_u
              local.tee $l6
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              local.tee $l7
              i32.const -1
              i32.le_s
              br_if $B4
              local.get $l5
              local.set $l4
              br $B3
            end
            block $B5
              block $B6
                block $B7
                  block $B8
                    block $B9
                      block $B10
                        block $B11
                          local.get $l6
                          i32.const 1056188
                          i32.add
                          i32.load8_u
                          i32.const -2
                          i32.add
                          br_table $B11 $B10 $B9 $B1
                        end
                        local.get $l3
                        local.get $l5
                        i32.add
                        i32.const 1055260
                        local.get $l5
                        local.get $l2
                        i32.lt_u
                        select
                        i32.load8_u
                        i32.const 192
                        i32.and
                        i32.const 128
                        i32.ne
                        br_if $B1
                        local.get $l4
                        i32.const 2
                        i32.add
                        local.set $l4
                        br $B3
                      end
                      local.get $l3
                      local.get $l5
                      i32.add
                      i32.const 1055260
                      local.get $l5
                      local.get $l2
                      i32.lt_u
                      select
                      i32.load8_s
                      local.set $l8
                      local.get $l6
                      i32.const -224
                      i32.add
                      br_table $B8 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B6 $B7 $B6
                    end
                    local.get $l3
                    local.get $l5
                    i32.add
                    i32.const 1055260
                    local.get $l5
                    local.get $l2
                    i32.lt_u
                    select
                    i32.load8_s
                    local.set $l8
                    block $B12
                      block $B13
                        block $B14
                          block $B15
                            local.get $l6
                            i32.const -240
                            i32.add
                            br_table $B14 $B15 $B15 $B15 $B13 $B15
                          end
                          local.get $l7
                          i32.const 15
                          i32.add
                          i32.const 255
                          i32.and
                          i32.const 2
                          i32.gt_u
                          br_if $B1
                          local.get $l8
                          i32.const -1
                          i32.gt_s
                          br_if $B1
                          local.get $l8
                          i32.const -64
                          i32.lt_u
                          br_if $B12
                          br $B1
                        end
                        local.get $l8
                        i32.const 112
                        i32.add
                        i32.const 255
                        i32.and
                        i32.const 48
                        i32.lt_u
                        br_if $B12
                        br $B1
                      end
                      local.get $l8
                      i32.const -113
                      i32.gt_s
                      br_if $B1
                    end
                    local.get $l3
                    local.get $l4
                    i32.const 2
                    i32.add
                    local.tee $l5
                    i32.add
                    i32.const 1055260
                    local.get $l5
                    local.get $l2
                    i32.lt_u
                    select
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.ne
                    br_if $B1
                    local.get $l3
                    local.get $l4
                    i32.const 3
                    i32.add
                    local.tee $l5
                    i32.add
                    i32.const 1055260
                    local.get $l5
                    local.get $l2
                    i32.lt_u
                    select
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.ne
                    br_if $B1
                    local.get $l4
                    i32.const 4
                    i32.add
                    local.set $l4
                    br $B3
                  end
                  local.get $l8
                  i32.const -32
                  i32.and
                  i32.const -96
                  i32.ne
                  br_if $B1
                  br $B5
                end
                local.get $l8
                i32.const -96
                i32.ge_s
                br_if $B1
                br $B5
              end
              block $B16
                local.get $l7
                i32.const 31
                i32.add
                i32.const 255
                i32.and
                i32.const 12
                i32.lt_u
                br_if $B16
                local.get $l7
                i32.const -2
                i32.and
                i32.const -18
                i32.ne
                br_if $B1
                local.get $l8
                i32.const -1
                i32.gt_s
                br_if $B1
                local.get $l8
                i32.const -64
                i32.ge_u
                br_if $B1
                br $B5
              end
              local.get $l8
              i32.const -65
              i32.gt_s
              br_if $B1
            end
            local.get $l3
            local.get $l4
            i32.const 2
            i32.add
            local.tee $l5
            i32.add
            i32.const 1055260
            local.get $l5
            local.get $l2
            i32.lt_u
            select
            i32.load8_u
            i32.const 192
            i32.and
            i32.const 128
            i32.ne
            br_if $B1
            local.get $l4
            i32.const 3
            i32.add
            local.set $l4
          end
          local.get $l4
          local.set $l5
          local.get $l4
          local.get $l2
          i32.lt_u
          br_if $L2
        end
      end
      local.get $p0
      local.get $l4
      i32.store offset=4
      local.get $p0
      local.get $l3
      i32.store
      local.get $p1
      local.get $l2
      local.get $l5
      i32.sub
      i32.store offset=4
      local.get $p1
      local.get $l3
      local.get $l5
      i32.add
      i32.store
      local.get $p0
      i32.const 12
      i32.add
      local.get $l5
      local.get $l4
      i32.sub
      i32.store
      local.get $p0
      i32.const 8
      i32.add
      local.get $l3
      local.get $l4
      i32.add
      i32.store
      return
    end
    local.get $p0
    i32.const 0
    i32.store)
  (func $_ZN4core7unicode9printable5check17h200e8aa24e8ac88cE (type $t15) (param $p0 i32) (param $p1 i32) (param $p2 i32) (param $p3 i32) (param $p4 i32) (param $p5 i32) (param $p6 i32) (result i32)
    (local $l7 i32) (local $l8 i32) (local $l9 i32) (local $l10 i32) (local $l11 i32) (local $l12 i32) (local $l13 i32)
    i32.const 1
    local.set $l7
    block $B0
      block $B1
        local.get $p2
        i32.eqz
        br_if $B1
        local.get $p1
        local.get $p2
        i32.const 1
        i32.shl
        i32.add
        local.set $l8
        local.get $p0
        i32.const 65280
        i32.and
        i32.const 8
        i32.shr_u
        local.set $l9
        i32.const 0
        local.set $l10
        local.get $p0
        i32.const 255
        i32.and
        local.set $l11
        block $B2
          loop $L3
            local.get $p1
            i32.const 2
            i32.add
            local.set $l12
            local.get $l10
            local.get $p1
            i32.load8_u offset=1
            local.tee $p2
            i32.add
            local.set $l13
            block $B4
              local.get $p1
              i32.load8_u
              local.tee $p1
              local.get $l9
              i32.eq
              br_if $B4
              local.get $p1
              local.get $l9
              i32.gt_u
              br_if $B1
              local.get $l13
              local.set $l10
              local.get $l12
              local.set $p1
              local.get $l12
              local.get $l8
              i32.ne
              br_if $L3
              br $B1
            end
            block $B5
              local.get $l13
              local.get $l10
              i32.lt_u
              br_if $B5
              local.get $l13
              local.get $p4
              i32.gt_u
              br_if $B2
              local.get $p3
              local.get $l10
              i32.add
              local.set $p1
              block $B6
                loop $L7
                  local.get $p2
                  i32.eqz
                  br_if $B6
                  local.get $p2
                  i32.const -1
                  i32.add
                  local.set $p2
                  local.get $p1
                  i32.load8_u
                  local.set $l10
                  local.get $p1
                  i32.const 1
                  i32.add
                  local.set $p1
                  local.get $l10
                  local.get $l11
                  i32.ne
                  br_if $L7
                end
                i32.const 0
                local.set $l7
                br $B0
              end
              local.get $l13
              local.set $l10
              local.get $l12
              local.set $p1
              local.get $l12
              local.get $l8
              i32.ne
              br_if $L3
              br $B1
            end
          end
          local.get $l10
          local.get $l13
          local.get $p2
          call $_ZN4core5slice5index22slice_index_order_fail17h1f302691b181a368E
          unreachable
        end
        local.get $l13
        local.get $p4
        local.get $p2
        call $_ZN4core5slice5index24slice_end_index_len_fail17h46f9c6e214bb08d6E
        unreachable
      end
      local.get $p6
      i32.eqz
      br_if $B0
      local.get $p5
      local.get $p6
      i32.add
      local.set $l11
      local.get $p0
      i32.const 65535
      i32.and
      local.set $p1
      i32.const 1
      local.set $l7
      block $B8
        loop $L9
          local.get $p5
          i32.const 1
          i32.add
          local.set $l10
          block $B10
            block $B11
              local.get $p5
              i32.load8_u
              local.tee $p2
              i32.const 24
              i32.shl
              i32.const 24
              i32.shr_s
              local.tee $l13
              i32.const 0
              i32.lt_s
              br_if $B11
              local.get $l10
              local.set $p5
              br $B10
            end
            local.get $l10
            local.get $l11
            i32.eq
            br_if $B8
            local.get $l13
            i32.const 127
            i32.and
            i32.const 8
            i32.shl
            local.get $p5
            i32.load8_u offset=1
            i32.or
            local.set $p2
            local.get $p5
            i32.const 2
            i32.add
            local.set $p5
          end
          local.get $p1
          local.get $p2
          i32.sub
          local.tee $p1
          i32.const 0
          i32.lt_s
          br_if $B0
          local.get $l7
          i32.const 1
          i32.xor
          local.set $l7
          local.get $p5
          local.get $l11
          i32.ne
          br_if $L9
          br $B0
        end
      end
      i32.const 1055352
      i32.const 43
      i32.const 1056800
      call $_ZN4core9panicking5panic17h53b9d6ba61e71bb9E
      unreachable
    end
    local.get $l7
    i32.const 1
    i32.and)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..LowerHex$u20$for$u20$i32$GT$3fmt17hb5f1486a0d6bf05cE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load
    local.set $p0
    i32.const 0
    local.set $l3
    loop $L0
      local.get $l2
      local.get $l3
      i32.add
      i32.const 127
      i32.add
      i32.const 48
      i32.const 87
      local.get $p0
      i32.const 15
      i32.and
      local.tee $l4
      i32.const 10
      i32.lt_u
      select
      local.get $l4
      i32.add
      i32.store8
      local.get $l3
      i32.const -1
      i32.add
      local.set $l3
      local.get $p0
      i32.const 15
      i32.gt_u
      local.set $l4
      local.get $p0
      i32.const 4
      i32.shr_u
      local.set $p0
      local.get $l4
      br_if $L0
    end
    block $B1
      local.get $l3
      i32.const 128
      i32.add
      local.tee $p0
      i32.const 129
      i32.lt_u
      br_if $B1
      local.get $p0
      i32.const 128
      local.get $p0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $p1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get $l2
    local.get $l3
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get $l3
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set $p0
    local.get $l2
    i32.const 128
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt3num49_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u8$GT$3fmt17h2f6ce406ff5467efE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        block $B2
          block $B3
            block $B4
              local.get $p1
              i32.load
              local.tee $l3
              i32.const 16
              i32.and
              br_if $B4
              local.get $l3
              i32.const 32
              i32.and
              br_if $B3
              local.get $p0
              i64.load8_u
              i32.const 1
              local.get $p1
              call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E
              local.set $p0
              br $B0
            end
            local.get $p0
            i32.load8_u
            local.set $l3
            i32.const 0
            local.set $p0
            loop $L5
              local.get $l2
              local.get $p0
              i32.add
              i32.const 127
              i32.add
              i32.const 48
              i32.const 87
              local.get $l3
              i32.const 15
              i32.and
              local.tee $l4
              i32.const 10
              i32.lt_u
              select
              local.get $l4
              i32.add
              i32.store8
              local.get $p0
              i32.const -1
              i32.add
              local.set $p0
              local.get $l3
              i32.const 255
              i32.and
              local.tee $l4
              i32.const 4
              i32.shr_u
              local.set $l3
              local.get $l4
              i32.const 15
              i32.gt_u
              br_if $L5
            end
            local.get $p0
            i32.const 128
            i32.add
            local.tee $l3
            i32.const 129
            i32.ge_u
            br_if $B2
            local.get $p1
            i32.const 1
            i32.const 1055716
            i32.const 2
            local.get $l2
            local.get $p0
            i32.add
            i32.const 128
            i32.add
            i32.const 0
            local.get $p0
            i32.sub
            call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
            local.set $p0
            br $B0
          end
          local.get $p0
          i32.load8_u
          local.set $l3
          i32.const 0
          local.set $p0
          loop $L6
            local.get $l2
            local.get $p0
            i32.add
            i32.const 127
            i32.add
            i32.const 48
            i32.const 55
            local.get $l3
            i32.const 15
            i32.and
            local.tee $l4
            i32.const 10
            i32.lt_u
            select
            local.get $l4
            i32.add
            i32.store8
            local.get $p0
            i32.const -1
            i32.add
            local.set $p0
            local.get $l3
            i32.const 255
            i32.and
            local.tee $l4
            i32.const 4
            i32.shr_u
            local.set $l3
            local.get $l4
            i32.const 15
            i32.gt_u
            br_if $L6
          end
          local.get $p0
          i32.const 128
          i32.add
          local.tee $l3
          i32.const 129
          i32.ge_u
          br_if $B1
          local.get $p1
          i32.const 1
          i32.const 1055716
          i32.const 2
          local.get $l2
          local.get $p0
          i32.add
          i32.const 128
          i32.add
          i32.const 0
          local.get $p0
          i32.sub
          call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
          local.set $p0
          br $B0
        end
        local.get $l3
        i32.const 128
        local.get $p0
        call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
        unreachable
      end
      local.get $l3
      i32.const 128
      local.get $p0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $l2
    i32.const 128
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E (type $t16) (param $p0 i64) (param $p1 i32) (param $p2 i32) (result i32)
    (local $l3 i32) (local $l4 i32) (local $l5 i64) (local $l6 i32) (local $l7 i32) (local $l8 i32)
    global.get $g0
    i32.const 48
    i32.sub
    local.tee $l3
    global.set $g0
    i32.const 39
    local.set $l4
    block $B0
      block $B1
        local.get $p0
        i64.const 10000
        i64.ge_u
        br_if $B1
        local.get $p0
        local.set $l5
        br $B0
      end
      i32.const 39
      local.set $l4
      loop $L2
        local.get $l3
        i32.const 9
        i32.add
        local.get $l4
        i32.add
        local.tee $l6
        i32.const -4
        i32.add
        local.get $p0
        local.get $p0
        i64.const 10000
        i64.div_u
        local.tee $l5
        i64.const 10000
        i64.mul
        i64.sub
        i32.wrap_i64
        local.tee $l7
        i32.const 65535
        i32.and
        i32.const 100
        i32.div_u
        local.tee $l8
        i32.const 1
        i32.shl
        i32.const 1055718
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        local.get $l6
        i32.const -2
        i32.add
        local.get $l7
        local.get $l8
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
        local.get $l4
        i32.const -4
        i32.add
        local.set $l4
        local.get $p0
        i64.const 99999999
        i64.gt_u
        local.set $l6
        local.get $l5
        local.set $p0
        local.get $l6
        br_if $L2
      end
    end
    block $B3
      local.get $l5
      i32.wrap_i64
      local.tee $l6
      i32.const 99
      i32.le_u
      br_if $B3
      local.get $l3
      i32.const 9
      i32.add
      local.get $l4
      i32.const -2
      i32.add
      local.tee $l4
      i32.add
      local.get $l5
      i32.wrap_i64
      local.tee $l6
      local.get $l6
      i32.const 65535
      i32.and
      i32.const 100
      i32.div_u
      local.tee $l6
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
    block $B4
      block $B5
        local.get $l6
        i32.const 10
        i32.lt_u
        br_if $B5
        local.get $l3
        i32.const 9
        i32.add
        local.get $l4
        i32.const -2
        i32.add
        local.tee $l4
        i32.add
        local.get $l6
        i32.const 1
        i32.shl
        i32.const 1055718
        i32.add
        i32.load16_u align=1
        i32.store16 align=1
        br $B4
      end
      local.get $l3
      i32.const 9
      i32.add
      local.get $l4
      i32.const -1
      i32.add
      local.tee $l4
      i32.add
      local.get $l6
      i32.const 48
      i32.add
      i32.store8
    end
    local.get $p2
    local.get $p1
    i32.const 1055260
    i32.const 0
    local.get $l3
    i32.const 9
    i32.add
    local.get $l4
    i32.add
    i32.const 39
    local.get $l4
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set $l4
    local.get $l3
    i32.const 48
    i32.add
    global.set $g0
    local.get $l4)
  (func $_ZN4core3fmt3num52_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i8$GT$3fmt17hb78061cd9b3bd312E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load8_u
    local.set $l3
    i32.const 0
    local.set $p0
    loop $L0
      local.get $l2
      local.get $p0
      i32.add
      i32.const 127
      i32.add
      i32.const 48
      i32.const 55
      local.get $l3
      i32.const 15
      i32.and
      local.tee $l4
      i32.const 10
      i32.lt_u
      select
      local.get $l4
      i32.add
      i32.store8
      local.get $p0
      i32.const -1
      i32.add
      local.set $p0
      local.get $l3
      i32.const 255
      i32.and
      local.tee $l4
      i32.const 4
      i32.shr_u
      local.set $l3
      local.get $l4
      i32.const 15
      i32.gt_u
      br_if $L0
    end
    block $B1
      local.get $p0
      i32.const 128
      i32.add
      local.tee $l3
      i32.const 129
      i32.lt_u
      br_if $B1
      local.get $l3
      i32.const 128
      local.get $p0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $p1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get $l2
    local.get $p0
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get $p0
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set $p0
    local.get $l2
    i32.const 128
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt3num53_$LT$impl$u20$core..fmt..UpperHex$u20$for$u20$i32$GT$3fmt17hfe79af84422ce847E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32) (local $l4 i32)
    global.get $g0
    i32.const 128
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p0
    i32.load
    local.set $p0
    i32.const 0
    local.set $l3
    loop $L0
      local.get $l2
      local.get $l3
      i32.add
      i32.const 127
      i32.add
      i32.const 48
      i32.const 55
      local.get $p0
      i32.const 15
      i32.and
      local.tee $l4
      i32.const 10
      i32.lt_u
      select
      local.get $l4
      i32.add
      i32.store8
      local.get $l3
      i32.const -1
      i32.add
      local.set $l3
      local.get $p0
      i32.const 15
      i32.gt_u
      local.set $l4
      local.get $p0
      i32.const 4
      i32.shr_u
      local.set $p0
      local.get $l4
      br_if $L0
    end
    block $B1
      local.get $l3
      i32.const 128
      i32.add
      local.tee $p0
      i32.const 129
      i32.lt_u
      br_if $B1
      local.get $p0
      i32.const 128
      local.get $p0
      call $_ZN4core5slice5index26slice_start_index_len_fail17h41b2836efbe52f4aE
      unreachable
    end
    local.get $p1
    i32.const 1
    i32.const 1055716
    i32.const 2
    local.get $l2
    local.get $l3
    i32.add
    i32.const 128
    i32.add
    i32.const 0
    local.get $l3
    i32.sub
    call $_ZN4core3fmt9Formatter12pad_integral17ha0b5e783f4f83bf3E
    local.set $p0
    local.get $l2
    i32.const 128
    i32.add
    global.set $g0
    local.get $p0)
  (func $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h4887f6c2cac827aeE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.tee $p0
    i64.extend_i32_u
    local.get $p0
    i32.const -1
    i32.xor
    i64.extend_i32_s
    i64.const 1
    i64.add
    local.get $p0
    i32.const -1
    i32.gt_s
    local.tee $p0
    select
    local.get $p0
    local.get $p1
    call $_ZN4core3fmt3num3imp7fmt_u6417h6b7d620637037ed6E)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h015fd4bc7d5a98ffE (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN4core3fmt3num50_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u32$GT$3fmt17h96d28800ef253020E)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hd1cef3d00f68bba3E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    block $B0
      block $B1
        local.get $p0
        i32.load
        local.tee $p0
        i32.load8_u
        br_if $B1
        local.get $p1
        i32.load offset=24
        i32.const 1058296
        i32.const 4
        local.get $p1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        local.set $p1
        br $B0
      end
      local.get $l2
      local.get $p1
      i32.load offset=24
      i32.const 1058292
      i32.const 4
      local.get $p1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type $t5) $T0
      i32.store8 offset=8
      local.get $l2
      local.get $p1
      i32.store
      local.get $l2
      i32.const 0
      i32.store8 offset=9
      local.get $l2
      i32.const 0
      i32.store offset=4
      i32.const 1
      local.set $p1
      local.get $l2
      local.get $p0
      i32.const 1
      i32.add
      i32.store offset=12
      local.get $l2
      local.get $l2
      i32.const 12
      i32.add
      i32.const 1055700
      call $_ZN4core3fmt8builders10DebugTuple5field17h963fe1aaa21a4b0fE
      drop
      local.get $l2
      i32.load8_u offset=8
      local.set $p0
      block $B2
        block $B3
          local.get $l2
          i32.load offset=4
          local.tee $l3
          br_if $B3
          local.get $p0
          local.set $p1
          br $B2
        end
        local.get $p0
        i32.const 255
        i32.and
        br_if $B2
        local.get $l2
        i32.load
        local.set $p0
        block $B4
          local.get $l3
          i32.const 1
          i32.ne
          br_if $B4
          local.get $l2
          i32.load8_u offset=9
          i32.const 255
          i32.and
          i32.eqz
          br_if $B4
          local.get $p0
          i32.load8_u
          i32.const 4
          i32.and
          br_if $B4
          i32.const 1
          local.set $p1
          local.get $p0
          i32.load offset=24
          i32.const 1055696
          i32.const 1
          local.get $p0
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type $t5) $T0
          br_if $B2
        end
        local.get $p0
        i32.load offset=24
        i32.const 1055261
        i32.const 1
        local.get $p0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        local.set $p1
      end
      local.get $p1
      i32.const 255
      i32.and
      i32.const 0
      i32.ne
      local.set $p1
    end
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1)
  (func $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17he00dc59ef87d3ec0E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    i32.load
    local.get $p1
    call $_ZN4core3fmt3num49_$LT$impl$u20$core..fmt..Debug$u20$for$u20$u8$GT$3fmt17h2f6ce406ff5467efE)
  (func $_ZN64_$LT$core..str..error..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17h6965576c5be6dee1E (type $t3) (param $p0 i32) (param $p1 i32) (result i32)
    (local $l2 i32) (local $l3 i32)
    global.get $g0
    i32.const 16
    i32.sub
    local.tee $l2
    global.set $g0
    local.get $p1
    i32.load offset=24
    i32.const 1058316
    i32.const 9
    local.get $p1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type $t5) $T0
    local.set $l3
    local.get $l2
    i32.const 0
    i32.store8 offset=5
    local.get $l2
    local.get $l3
    i32.store8 offset=4
    local.get $l2
    local.get $p1
    i32.store
    local.get $l2
    local.get $p0
    i32.store offset=12
    local.get $l2
    i32.const 1058325
    i32.const 11
    local.get $l2
    i32.const 12
    i32.add
    i32.const 1058300
    call $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE
    local.set $p1
    local.get $l2
    local.get $p0
    i32.const 4
    i32.add
    i32.store offset=12
    local.get $p1
    i32.const 1058336
    i32.const 9
    local.get $l2
    i32.const 12
    i32.add
    i32.const 1058348
    call $_ZN4core3fmt8builders11DebugStruct5field17h40ee1d7a0781493dE
    drop
    local.get $l2
    i32.load8_u offset=4
    local.set $p1
    block $B0
      local.get $l2
      i32.load8_u offset=5
      i32.eqz
      br_if $B0
      local.get $p1
      i32.const 255
      i32.and
      local.set $p0
      i32.const 1
      local.set $p1
      local.get $p0
      br_if $B0
      block $B1
        local.get $l2
        i32.load
        local.tee $p1
        i32.load8_u
        i32.const 4
        i32.and
        br_if $B1
        local.get $p1
        i32.load offset=24
        i32.const 1055691
        i32.const 2
        local.get $p1
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type $t5) $T0
        local.set $p1
        br $B0
      end
      local.get $p1
      i32.load offset=24
      i32.const 1055677
      i32.const 1
      local.get $p1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=12
      call_indirect (type $t5) $T0
      local.set $p1
    end
    local.get $l2
    i32.const 16
    i32.add
    global.set $g0
    local.get $p1
    i32.const 255
    i32.and
    i32.const 0
    i32.ne)
  (func $print_env.command_export (type $t7)
    call $__wasm_call_ctors
    call $print_env
    call $__wasm_call_dtors)
  (table $T0 86 86 funcref)
  (memory $memory 17)
  (global $g0 (mut i32) (i32.const 1048576))
  (global $__heap_base i32 (i32.const 1059856))
  (global $__data_end i32 (i32.const 1059844))
  (export "memory" (memory 0))
  (export "__heap_base" (global 1))
  (export "__data_end" (global 2))
  (export "print_env" (func $print_env.command_export))
  (elem $e0 (i32.const 1) $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E $_ZN45_$LT$$RF$T$u20$as$u20$core..fmt..UpperHex$GT$3fmt17h547981a8fa47a7d3E $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$i32$GT$3fmt17h4887f6c2cac827aeE $_ZN60_$LT$alloc..string..String$u20$as$u20$core..fmt..Display$GT$3fmt17h810fc2428ec8d357E.1 $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17hf8285e92300923c0E $_ZN60_$LT$std..io..error..Error$u20$as$u20$core..fmt..Display$GT$3fmt17h30941f502999929dE $_ZN4core3fmt3num3imp52_$LT$impl$u20$core..fmt..Display$u20$for$u20$u32$GT$3fmt17h3c5a075a47b3fc10E $_ZN3std5alloc24default_alloc_error_hook17h5a1a1d982f9bd818E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h32abcc04c39e85a4E $_ZN91_$LT$std..sys_common..backtrace.._print..DisplayBacktrace$u20$as$u20$core..fmt..Display$GT$3fmt17hafba394e37912a5aE $_ZN73_$LT$core..panic..panic_info..PanicInfo$u20$as$u20$core..fmt..Display$GT$3fmt17h92a87928d361b08eE $_ZN59_$LT$core..fmt..Arguments$u20$as$u20$core..fmt..Display$GT$3fmt17h5e2128c63f339bb4E $_ZN4core3ptr100drop_in_place$LT$$RF$mut$u20$std..io..Write..write_fmt..Adapter$LT$alloc..vec..Vec$LT$u8$GT$$GT$$GT$17hf1b3c7767b0b7765E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h35232905e30669aaE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17hbc3d3931408e41d6E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17hf0a5b3c4ac0d0496E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h7f904ebe63e601aaE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h9896f020280c35deE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h47c6b3c5d318955aE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h5e136cc95a131902E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h2ffc04b3c6716e0dE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17he4a59f9604beea94E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17he56d425d213d6944E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5a3b0091f8192a8fE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h8027bf4984b5aaecE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h5d3c77c4d604c6d6E $_ZN63_$LT$core..cell..BorrowMutError$u20$as$u20$core..fmt..Debug$GT$3fmt17h651b426602cda903E $_ZN4core3ptr103drop_in_place$LT$std..sync..poison..PoisonError$LT$std..sync..mutex..MutexGuard$LT$$LP$$RP$$GT$$GT$$GT$17h342b8354fe7ce8f5E $_ZN76_$LT$std..sync..poison..PoisonError$LT$T$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17h45ee2ecae3ad0859E $_ZN64_$LT$core..str..error..Utf8Error$u20$as$u20$core..fmt..Debug$GT$3fmt17h6965576c5be6dee1E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h51406901289b8250E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h72c85046dd08428aE $_ZN4core3ptr226drop_in_place$LT$std..error..$LT$impl$u20$core..convert..From$LT$alloc..string..String$GT$$u20$for$u20$alloc..boxed..Box$LT$dyn$u20$std..error..Error$u2b$core..marker..Send$u2b$core..marker..Sync$GT$$GT$..from..StringError$GT$17h8f80eb1a61eb416eE $_ZN55_$LT$std..path..PathBuf$u20$as$u20$core..fmt..Debug$GT$3fmt17h3d2bbcbe4fd788bbE $_ZN4core3ptr87drop_in_place$LT$std..io..Write..write_fmt..Adapter$LT$$RF$mut$u20$$u5b$u8$u5d$$GT$$GT$17h9e512fc24cfce3e7E $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha182898d6ab76de7E $_ZN4core3fmt5Write10write_char17hbe9f53f72de9fff4E $_ZN4core3fmt5Write9write_fmt17h9501b0d1bd73714aE $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17ha099fde24b4a4dcaE $_ZN4core3fmt5Write10write_char17ha6ee8ba883b29687E $_ZN4core3fmt5Write9write_fmt17h8a28623dee2384e2E $_ZN80_$LT$std..io..Write..write_fmt..Adapter$LT$T$GT$$u20$as$u20$core..fmt..Write$GT$9write_str17hb12703cce11fe208E $_ZN4core3fmt5Write10write_char17h46400a8194b99cf9E $_ZN4core3fmt5Write9write_fmt17hcb1cb3e9d099f402E $_ZN4core3ops8function6FnOnce40call_once$u7b$$u7b$vtable.shim$u7d$$u7d$17hd87567c1af08f732E $_ZN3std4sync4once4Once15call_once_force28_$u7b$$u7b$closure$u7d$$u7d$17h5da9e8acebb38efeE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5write17h7fa424a882c725e9E $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$14write_vectored17he7f7854621726461E $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$17is_write_vectored17h1cc0993ef9fea8dfE $_ZN64_$LT$std..sys..wasi..stdio..Stderr$u20$as$u20$std..io..Write$GT$5flush17h5d2f24e96e822d71E $_ZN3std2io5Write9write_all17hf2d1f3fd20cf6d1dE $_ZN3std2io5Write18write_all_vectored17h3007a4983f10eddeE $_ZN3std2io5Write9write_fmt17h5e7141e21f49a775E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5write17hfeddc07cce766a8dE $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$14write_vectored17h5e2f18a5d90c2318E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$17is_write_vectored17hbcc64e0de2a7bd6eE $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$5flush17h3e171420e6883b71E $_ZN3std2io5impls74_$LT$impl$u20$std..io..Write$u20$for$u20$alloc..vec..Vec$LT$u8$C$A$GT$$GT$9write_all17hacb9a4b06548536cE $_ZN3std2io5Write18write_all_vectored17h73403c20721d838fE $_ZN3std2io5Write9write_fmt17hf06e34d13533586bE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h2d0c1ef8155b3364E $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17h83af4092997449d4E $_ZN4core3ptr70drop_in_place$LT$std..panicking..begin_panic_handler..PanicPayload$GT$17ha86c38a5fb14f016E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h34d0cd2380313f86E $_ZN90_$LT$std..panicking..begin_panic_handler..PanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17ha161f6684539a1beE $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$8take_box17h652349175b493901E $_ZN93_$LT$std..panicking..begin_panic_handler..StrPanicPayload$u20$as$u20$core..panic..BoxMeUp$GT$3get17hdfcd929547320219E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h8790479cff165904E $_ZN71_$LT$core..ops..range..Range$LT$Idx$GT$$u20$as$u20$core..fmt..Debug$GT$3fmt17ha826dd87512c4c00E $_ZN41_$LT$char$u20$as$u20$core..fmt..Debug$GT$3fmt17hf9cabbcc6a6076ccE $_ZN4core3ops8function6FnOnce9call_once17he625032f56c99b4fE $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h866d2163a1275370E $_ZN44_$LT$$RF$T$u20$as$u20$core..fmt..Display$GT$3fmt17h0d0f7379d9aa8697E $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hade274a01ebef0eeE $_ZN4core3ptr102drop_in_place$LT$$RF$core..iter..adapters..copied..Copied$LT$core..slice..iter..Iter$LT$u8$GT$$GT$$GT$17h0ae777612708bf9bE $_ZN36_$LT$T$u20$as$u20$core..any..Any$GT$7type_id17hd78f961f99573a2cE $_ZN68_$LT$core..fmt..builders..PadAdapter$u20$as$u20$core..fmt..Write$GT$9write_str17h34d9524dd79c4786E $_ZN4core3fmt5Write10write_char17h13cfc975967354caE $_ZN4core3fmt5Write9write_fmt17hdb67d064950d9d5bE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17he00dc59ef87d3ec0E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_str17h84bb0430ffe0ae70E $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$10write_char17h5b8cfb95cae022fbE $_ZN50_$LT$$RF$mut$u20$W$u20$as$u20$core..fmt..Write$GT$9write_fmt17h532266dfb8016bcfE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17h015fd4bc7d5a98ffE $_ZN42_$LT$$RF$T$u20$as$u20$core..fmt..Debug$GT$3fmt17hd1cef3d00f68bba3E)
  (data $d0 (i32.const 1048576) "The env vars are as follows.\0a\00\00\00\00\00\10\00\1d\00\00\00: \0a\00\00\00\10\00\00\00\00\00(\00\10\00\02\00\00\00*\00\10\00\01\00\00\00The args are as follows.\0a\00\00\00D\00\10\00\19\00\00\00\00\00\10\00\00\00\00\00*\00\10\00\01\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\0e\00\00\00\0f\00\00\00\10\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\11\00\00\00\12\00\00\00\13\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\14\00\00\00\15\00\00\00\16\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\17\00\00\00\18\00\00\00\19\00\00\00already borrowed\0d\00\00\00\00\00\00\00\01\00\00\00\1a\00\00\00assertion failed: mid <= self.len()called `Option::unwrap()` on a `None` value\00\00\0d\00\00\00\00\00\00\00\01\00\00\00\1b\00\00\00called `Result::unwrap()` on an `Err` value\00\1c\00\00\00\08\00\00\00\04\00\00\00\1d\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00\1e\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00\1f\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00 \00\00\00internal error: entered unreachable code/rustc/546c826f0ccaab36e897860205281f490db274e6/library/alloc/src/vec/mod.rs\ec\01\10\00L\00\00\00>\07\00\00$\00\00\00fatal runtime error: \0a\00\00H\02\10\00\15\00\00\00]\02\10\00\01\00\00\00use of std::thread::current() is not possible after the thread's local data has been destroyedlibrary/std/src/thread/mod.rs\00\ce\02\10\00\1d\00\00\00\a5\02\00\00#\00\00\00failed to generate unique thread ID: bitspace exhausted\00\fc\02\10\007\00\00\00\ce\02\10\00\1d\00\00\00\13\04\00\00\11\00\00\00\ce\02\10\00\1d\00\00\00\19\04\00\00*\00\00\00\22RUST_BACKTRACElibrary/std/src/env.rs\00\00\00k\03\10\00\16\00\00\00\ab\00\00\009\00\00\00k\03\10\00\16\00\00\00\ab\00\00\00S\00\00\00k\03\10\00\16\00\00\00%\03\00\003\00\00\00\d8\00\10\00\00\00\00\00: \00\00!\00\00\00\0c\00\00\00\04\00\00\00\22\00\00\00\00failed to write the buffered data\00\00\d1\03\10\00!\00\00\00\17\00\00\00library/std/src/io/buffered/bufwriter.rs\00\04\10\00(\00\00\00\8d\00\00\00\12\00\00\00library/std/src/io/buffered/linewritershim.rs\00\00\008\04\10\00-\00\00\00\01\01\00\00)\00\00\00uncategorized errorother errorout of memoryunexpected end of fileunsupportedoperation interruptedargument list too longinvalid filenametoo many linkscross-device link or renamedeadlockexecutable file busyresource busyfile too largefilesystem quota exceededseek on unseekable fileno storage spacewrite zerotimed outinvalid datainvalid input parameterstale network file handlefilesystem loop or indirection limit (e.g. symlink loop)read-only filesystem or storage mediumdirectory not emptyis a directorynot a directoryoperation would blockentity already existsbroken pipenetwork downaddress not availableaddress in usenot connectedconnection abortednetwork unreachablehost unreachableconnection resetconnection refusedpermission deniedentity not found (os error )\00\00\00\d8\00\10\00\00\00\00\00e\07\10\00\0b\00\00\00p\07\10\00\01\00\00\00failed to write whole buffer\8c\07\10\00\1c\00\00\00\17\00\00\00library/std/src/io/stdio.rs\00\b4\07\10\00\1b\00\00\00\dc\02\00\00\14\00\00\00failed printing to \00\e0\07\10\00\13\00\00\00\bc\03\10\00\02\00\00\00\b4\07\10\00\1b\00\00\00\f8\03\00\00\09\00\00\00stdoutlibrary/std/src/io/mod.rs\00\1a\08\10\00\19\00\00\00\0a\05\00\00\16\00\00\00\1a\08\10\00\19\00\00\00\f1\05\00\00!\00\00\00formatter error\00T\08\10\00\0f\00\00\00(\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00$\00\00\00%\00\00\00&\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00'\00\00\00(\00\00\00)\00\00\00#\00\00\00\0c\00\00\00\04\00\00\00*\00\00\00+\00\00\00,\00\00\00library/std/src/panic.rs\b8\08\10\00\18\00\00\00\f0\00\00\00\12\00\00\00library/std/src/sync/once.rs\e0\08\10\00\1c\00\00\00N\01\00\00\0e\00\00\00\0d\00\00\00\04\00\00\00\04\00\00\00-\00\00\00.\00\00\00\e0\08\10\00\1c\00\00\00N\01\00\001\00\00\00assertion failed: state_and_queue.addr() & STATE_MASK == RUNNINGOnce instance has previously been poisoned\00\00p\09\10\00*\00\00\00\02\00\00\00\e0\08\10\00\1c\00\00\00\ff\01\00\00\09\00\00\00\e0\08\10\00\1c\00\00\00\0c\02\00\005\00\00\00PoisonErrorstack backtrace:\0a\d3\09\10\00\11\00\00\00note: Some details are omitted, run with `RUST_BACKTRACE=full` for a verbose backtrace.\0a\ec\09\10\00X\00\00\00lock count overflow in reentrant mutexlibrary/std/src/sys_common/remutex.rs\00r\0a\10\00%\00\00\00\a7\00\00\00\0e\00\00\00library/std/src/sys_common/thread_info.rs\00\00\00\a8\0a\10\00)\00\00\00\16\00\00\003\00\00\00memory allocation of  bytes failed\0a\00\e4\0a\10\00\15\00\00\00\f9\0a\10\00\0e\00\00\00library/std/src/alloc.rs\18\0b\10\00\18\00\00\00R\01\00\00\09\00\00\00library/std/src/panicking.rs@\0b\10\00\1c\00\00\00\11\01\00\00$\00\00\00Box<dyn Any><unnamed>\00\00\00\0d\00\00\00\00\00\00\00\01\00\00\00/\00\00\000\00\00\001\00\00\002\00\00\003\00\00\004\00\00\005\00\00\00!\00\00\00\0c\00\00\00\04\00\00\006\00\00\007\00\00\008\00\00\009\00\00\00:\00\00\00;\00\00\00<\00\00\00thread '' panicked at '', \00\00\d4\0b\10\00\08\00\00\00\dc\0b\10\00\0f\00\00\00\eb\0b\10\00\03\00\00\00]\02\10\00\01\00\00\00note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\0a\00\00\10\0c\10\00N\00\00\00@\0b\10\00\1c\00\00\00F\02\00\00\1f\00\00\00@\0b\10\00\1c\00\00\00G\02\00\00\1e\00\00\00!\00\00\00\0c\00\00\00\04\00\00\00=\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00>\00\00\00?\00\00\00\10\00\00\00\04\00\00\00@\00\00\00A\00\00\00\0d\00\00\00\08\00\00\00\04\00\00\00B\00\00\00C\00\00\00thread panicked while processing panic. aborting.\0a\00\00\d0\0c\10\002\00\00\00\0apanicked after panic::always_abort(), aborting.\0a\00\00\00\d8\00\10\00\00\00\00\00\0c\0d\10\001\00\00\00thread panicked while panicking. aborting.\0a\00P\0d\10\00+\00\00\00failed to initiate panic, error \84\0d\10\00 \00\00\00advancing IoSlice beyond its length\00\ac\0d\10\00#\00\00\00library/std/src/sys/wasi/io.rs\00\00\d8\0d\10\00\1e\00\00\00\16\00\00\00\0d\00\00\00condvar wait not supported\00\00\08\0e\10\00\1a\00\00\00library/std/src/sys/wasi/../unsupported/locks/condvar.rs,\0e\10\008\00\00\00\14\00\00\00\09\00\00\00cannot recursively acquire mutext\0e\10\00 \00\00\00library/std/src/sys/wasi/../unsupported/locks/mutex.rs\00\00\9c\0e\10\006\00\00\00\17\00\00\00\09\00\00\00rwlock locked for writing\00\00\00\e4\0e\10\00\19\00\00\00strerror_r failure\00\00\08\0f\10\00\12\00\00\00library/std/src/sys/wasi/os.rs\00\00$\0f\10\00\1e\00\00\00/\00\00\00\0d\00\00\00$\0f\10\00\1e\00\00\001\00\00\006\00\00\00$\0f\10\00\1e\00\00\00\ab\00\00\00'\00\00\00$\0f\10\00\1e\00\00\00\ac\00\00\00'\00\00\00\5cx\00\00\84\0f\10\00\02\00\00\00\00\00\00\00 \00\00\00\08\00\00\00\02\00\00\00\00\00\00\00\00\00\00\00\02\00\00\00\03\00\00\00\08\00\0e\00\0f\00?\00\02\00@\005\00\0d\00\04\00\03\00,\00\1b\00\1c\00I\00\14\00\06\004\000\00library/std/src/sys_common/thread_parker/generic.rs\00\d4\0f\10\003\00\00\00'\00\00\00&\00\00\00inconsistent park state\00\18\10\10\00\17\00\00\00\d4\0f\10\003\00\00\005\00\00\00\17\00\00\00park state changed unexpectedly\00H\10\10\00\1f\00\00\00\d4\0f\10\003\00\00\002\00\00\00\11\00\00\00inconsistent state in unpark\80\10\10\00\1c\00\00\00\d4\0f\10\003\00\00\00l\00\00\00\12\00\00\00\d4\0f\10\003\00\00\00z\00\00\00\1f\00\00\00\0e\00\00\00\10\00\00\00\16\00\00\00\15\00\00\00\0b\00\00\00\16\00\00\00\0d\00\00\00\0b\00\00\00\13\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\10\00\00\00\11\00\00\00\12\00\00\00\10\00\00\00\10\00\00\00\13\00\00\00\12\00\00\00\0d\00\00\00\0e\00\00\00\15\00\00\00\0c\00\00\00\0b\00\00\00\15\00\00\00\15\00\00\00\0f\00\00\00\0e\00\00\00\13\00\00\00&\00\00\008\00\00\00\19\00\00\00\17\00\00\00\0c\00\00\00\09\00\00\00\0a\00\00\00\10\00\00\00\17\00\00\00\19\00\00\00\0e\00\00\00\0d\00\00\00\14\00\00\00\08\00\00\00\1b\00\00\00\ff\04\10\00\ef\04\10\00\d9\04\10\00\c4\04\10\00\b9\04\10\00\a3\04\10\00\96\04\10\00\8b\04\10\00x\04\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00U\07\10\00D\07\10\002\07\10\00\22\07\10\00\12\07\10\00\ff\06\10\00\ed\06\10\00\e0\06\10\00\d2\06\10\00\bd\06\10\00\b1\06\10\00\a6\06\10\00\91\06\10\00|\06\10\00m\06\10\00_\06\10\00L\06\10\00&\06\10\00\ee\05\10\00\d5\05\10\00\be\05\10\00\b2\05\10\00\a9\05\10\00\9f\05\10\00\8f\05\10\00x\05\10\00_\05\10\00Q\05\10\00D\05\10\000\05\10\00(\05\10\00\0d\05\10\00/\00Success\00Illegal byte sequence\00Domain error\00Result not representable\00Not a tty\00Permission denied\00Operation not permitted\00No such file or directory\00No such process\00File exists\00Value too large for data type\00No space left on device\00Out of memory\00Resource busy\00Interrupted system call\00Resource temporarily unavailable\00Invalid seek\00Cross-device link\00Read-only file system\00Directory not empty\00Connection reset by peer\00Operation timed out\00Connection refused\00Host is unreachable\00Address in use\00Broken pipe\00I/O error\00No such device or address\00No such device\00Not a directory\00Is a directory\00Text file busy\00Exec format error\00Invalid argument\00Argument list too long\00Symbolic link loop\00Filename too long\00Too many open files in system\00No file descriptors available\00Bad file descriptor\00No child process\00Bad address\00File too large\00Too many links\00No locks available\00Resource deadlock would occur\00State not recoverable\00Previous owner died\00Operation canceled\00Function not implemented\00No message of desired type\00Identifier removed\00Link has been severed\00Protocol error\00Bad message\00Not a socket\00Destination address required\00Message too large\00Protocol wrong type for socket\00Protocol not available\00Protocol not supported\00Not supported\00Address family not supported by protocol\00Address not available\00Network is down\00Network unreachable\00Connection reset by network\00Connection aborted\00No buffer space available\00Socket is connected\00Socket not connected\00Operation already in progress\00Operation in progress\00Stale file handle\00Quota exceeded\00Multihop attempted\00Capabilities insufficient\00\00\00\00\00\00\00\00\00\00\00\00\00u\02N\00\d6\01\e2\04\b9\04\18\01\8e\05\ed\02\16\04\f2\00\97\03\01\038\05\af\01\82\01O\03/\04\1e\00\d4\05\a2\00\12\03\1e\03\c2\01\de\03\08\00\ac\05\00\01d\02\f1\01e\054\02\8c\02\cf\02-\03L\04\e3\05\9f\02\f8\04\1c\05\08\05\b1\02K\05\15\02x\00R\02<\03\f1\03\e4\00\c3\03}\04\cc\00\aa\03y\05$\02n\01m\03\22\04\ab\04D\00\fb\01\ae\00\83\03`\00\e5\01\07\04\94\04^\04+\00X\019\01\92\00\c2\05\9b\01C\02F\01\f6\05\00\00called `Option::unwrap()` on a `None` valuelibrary/alloc/src/raw_vec.rscapacity overflow\c3\19\10\00\11\00\00\00\a7\19\10\00\1c\00\00\00\05\02\00\00\05\00\00\00library/alloc/src/ffi/c_str.rs\00\00\ec\19\10\00\1e\00\00\00\1b\01\00\007\00\00\00\00)..\1e\1a\10\00\02\00\00\00BorrowMutErrorindex out of bounds: the len is  but the index is 6\1a\10\00 \00\00\00V\1a\10\00\12\00\00\00called `Option::unwrap()` on a `None` value:\1c\1a\10\00\00\00\00\00\a3\1a\10\00\01\00\00\00\a3\1a\10\00\01\00\00\00K\00\00\00\00\00\00\00\01\00\00\00L\00\00\00panicked at '', \d8\1a\10\00\01\00\00\00\d9\1a\10\00\03\00\00\00\1c\1a\10\00\00\00\00\00matches!===assertion failed: `(left  right)`\0a  left: ``,\0a right: ``: \00\00\00\ff\1a\10\00\19\00\00\00\18\1b\10\00\12\00\00\00*\1b\10\00\0c\00\00\006\1b\10\00\03\00\00\00`\00\00\00\ff\1a\10\00\19\00\00\00\18\1b\10\00\12\00\00\00*\1b\10\00\0c\00\00\00\5c\1b\10\00\01\00\00\00: \00\00\1c\1a\10\00\00\00\00\00\80\1b\10\00\02\00\00\00K\00\00\00\0c\00\00\00\04\00\00\00M\00\00\00N\00\00\00O\00\00\00     {\0a,\0a,  { ..\0a}, .. } { .. } }(\0a(,\00\00\00K\00\00\00\04\00\00\00\04\00\00\00P\00\00\000x00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899\00\00K\00\00\00\04\00\00\00\04\00\00\00Q\00\00\00R\00\00\00S\00\00\00truefalserange start index  out of range for slice of length \00\00\00\d1\1c\10\00\12\00\00\00\e3\1c\10\00\22\00\00\00library/core/src/slice/index.rs\00\18\1d\10\00\1f\00\00\004\00\00\00\05\00\00\00range end index H\1d\10\00\10\00\00\00\e3\1c\10\00\22\00\00\00\18\1d\10\00\1f\00\00\00I\00\00\00\05\00\00\00slice index starts at  but ends at \00x\1d\10\00\16\00\00\00\8e\1d\10\00\0d\00\00\00\18\1d\10\00\1f\00\00\00\5c\00\00\00\05\00\00\00\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\01\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\02\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\03\04\04\04\04\04\00\00\00\00\00\00\00\00\00\00\00library/core/src/str/mod.rs[...]byte index  is out of bounds of `\00\00\00\dc\1e\10\00\0b\00\00\00\e7\1e\10\00\16\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00k\00\00\00\09\00\00\00begin <= end ( <= ) when slicing `\00\00(\1f\10\00\0e\00\00\006\1f\10\00\04\00\00\00:\1f\10\00\10\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00o\00\00\00\05\00\00\00\bc\1e\10\00\1b\00\00\00}\00\00\00-\00\00\00 is not a char boundary; it is inside  (bytes ) of `\dc\1e\10\00\0b\00\00\00\8c\1f\10\00&\00\00\00\b2\1f\10\00\08\00\00\00\ba\1f\10\00\06\00\00\00\5c\1b\10\00\01\00\00\00\bc\1e\10\00\1b\00\00\00\7f\00\00\00\05\00\00\00library/core/src/unicode/printable.rs\00\00\00\f8\1f\10\00%\00\00\00\1a\00\00\006\00\00\00\00\01\03\05\05\06\06\02\07\06\08\07\09\11\0a\1c\0b\19\0c\1a\0d\10\0e\0d\0f\04\10\03\12\12\13\09\16\01\17\04\18\01\19\03\1a\07\1b\01\1c\02\1f\16 \03+\03-\0b.\010\031\022\01\a7\02\a9\02\aa\04\ab\08\fa\02\fb\05\fd\02\fe\03\ff\09\adxy\8b\8d\a20WX\8b\8c\90\1c\dd\0e\0fKL\fb\fc./?\5c]_\e2\84\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\ff\00\04\11\12)147:;=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91\9b\9d\c9\ce\cf\0d\11):;EIW[\5c^_de\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\0d\11EIde\80\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\8b\a4\a6\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfINOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7\11\16\17[\5c\f6\f7\fe\ff\80mq\de\df\0e\1fno\1c\1d_}~\ae\af\7f\bb\bc\16\17\1e\1fFGNOXZ\5c^~\7f\b5\c5\d4\d5\dc\f0\f1\f5rs\8ftu\96&./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\980\8f\1f\d2\d4\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91Sgu\c8\c9\d0\d1\d8\d9\e7\fe\ff\00 _\22\82\df\04\82D\08\1b\04\06\11\81\ac\0e\80\ab\05\1f\09\81\1b\03\19\08\01\04/\044\04\07\03\01\07\06\07\11\0aP\0f\12\07U\07\03\04\1c\0a\09\03\08\03\07\03\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05N\07\1b\07W\07\02\06\16\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%_ m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\16\09\18\09\14\0c\14\0cj\06\0a\06\1a\06Y\07+\05F\0a,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\0a\06/1M\03\80\a4\08<\03\0f\03<\078\08+\05\82\ff\11\18\08/\11-\03!\0f!\0f\80\8c\04\82\97\19\0b\15\88\94\05/\05;\07\02\0e\18\09\80\be\22t\0c\80\d6\1a\0c\05\80\ff\05\80\df\0c\f2\9d\037\09\81\5c\14\80\b8\08\80\cb\05\0a\18;\03\0a\068\08F\08\0c\06t\0b\1e\03Z\04Y\09\80\83\18\1c\0a\16\09L\04\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a6\10\81\f5\07\01 *\06L\04\80\8d\04\80\be\03\1b\03\0f\0d\00\06\01\01\03\01\04\02\05\07\07\02\08\08\09\02\0a\05\0b\02\0e\04\10\01\11\02\12\05\13\11\14\01\15\02\17\02\19\0d\1c\05\1d\08$\01j\04k\02\af\03\bc\02\cf\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e1\02\e7\04\e8\02\ee \f0\04\f8\02\fa\02\fb\01\0c';>NO\8f\9e\9e\9f{\8b\93\96\a2\b2\ba\86\b1\06\07\096=>V\f3\d0\d1\04\14\1867VW\7f\aa\ae\af\bd5\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:EFIJNOde\5c\b6\b7\1b\1c\07\08\0a\0b\14\1769:\a8\a9\d8\d9\097\90\91\a8\07\0a;>fi\8f\92o_\bf\ee\efZb\f4\fc\ff\9a\9b./'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\22%>?\e7\ec\ef\ff\c5\c6\04 #%&(38:HJLPSUVXZ\5c^`cefksx}\7f\8a\a4\aa\af\b0\c0\d0\ae\afno\93^\22{\05\03\04-\03f\03\01/.\80\82\1d\031\0f\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0bNC\817\09\16\0a\08\18;E9\03c\08\090\16\05!\03\1b\05\01@8\04K\05/\04\0a\07\09\07@ '\04\0c\096\03:\05\1a\07\04\0c\07PI73\0d3\07.\08\0a\81&RN(\08*\16\1a&\1c\14\17\09N\04$\09D\0d\19\07\0a\06H\08'\09u\0b?A*\06;\05\0a\06Q\06\01\05\10\03\05\80\8bb\1eH\08\0a\80\a6^\22E\0b\0a\06\0d\13:\06\0a6,\04\17\80\b9<dS\0cH\09\0aFE\1bH\08S\0dI\81\07F\0a\1d\03GI7\03\0e\08\0a\069\07\0a\816\19\80\b7\01\0f2\0d\83\9bfu\0b\80\c4\8aLc\0d\84/\8f\d1\82G\a1\b9\829\07*\04\5c\06&\0aF\0a(\05\13\82\b0[eK\049\07\11@\05\0b\02\0e\97\f8\08\84\d6*\09\a2\e7\813-\03\11\04\08\81\8c\89\04k\05\0d\03\09\07\10\92`G\09t<\80\f6\0as\08p\15F\80\9a\14\0cW\09\19\80\87\81G\03\85B\0f\15\84P\1f\80\e1+\80\d5-\03\1a\04\02\81@\1f\11:\05\01\84\e0\80\f7)L\04\0a\04\02\83\11DL=\80\c2<\06\01\04U\05\1b4\02\81\0e,\04d\0cV\0a\80\ae8\1d\0d,\04\09\07\02\0e\06\80\9a\83\d8\05\10\03\0d\03t\0cY\07\0c\04\01\0f\0c\048\08\0a\06(\08\22N\81T\0c\15\03\05\03\07\09\1d\03\0b\05\06\0a\0a\06\08\08\07\09\80\cb%\0a\84\06library/core/src/unicode/unicode_data.rs\00\00\00\99%\10\00(\00\00\00K\00\00\00(\00\00\00\99%\10\00(\00\00\00W\00\00\00\16\00\00\00\99%\10\00(\00\00\00R\00\00\00>\00\00\00SomeNoneK\00\00\00\04\00\00\00\04\00\00\00T\00\00\00Utf8Errorvalid_up_toerror_len\00\00\00K\00\00\00\04\00\00\00\04\00\00\00U\00\00\00\00\03\00\00\83\04 \00\91\05`\00]\13\a0\00\12\17 \1f\0c `\1f\ef,\a0+*0 ,o\a6\e0,\02\a8`-\1e\fb`.\00\fe 6\9e\ff`6\fd\01\e16\01\0a!7$\0d\e17\ab\0ea9/\18\a190\1c\e1G\f3\1e!L\f0j\e1OOo!P\9d\bc\a1P\00\cfaQe\d1\a1Q\00\da!R\00\e0\e1S0\e1aU\ae\e2\a1V\d0\e8\e1V \00nW\f0\01\ffW\00p\00\07\00-\01\01\01\02\01\02\01\01H\0b0\15\10\01e\07\02\06\02\02\01\04#\01\1e\1b[\0b:\09\09\01\18\04\01\09\01\03\01\05+\03<\08*\18\01 7\01\01\01\04\08\04\01\03\07\0a\02\1d\01:\01\01\01\02\04\08\01\09\01\0a\02\1a\01\02\029\01\04\02\04\02\02\03\03\01\1e\02\03\01\0b\029\01\04\05\01\02\04\01\14\02\16\06\01\01:\01\01\02\01\04\08\01\07\03\0a\02\1e\01;\01\01\01\0c\01\09\01(\01\03\017\01\01\03\05\03\01\04\07\02\0b\02\1d\01:\01\02\01\02\01\03\01\05\02\07\02\0b\02\1c\029\02\01\01\02\04\08\01\09\01\0a\02\1d\01H\01\04\01\02\03\01\01\08\01Q\01\02\07\0c\08b\01\02\09\0b\06J\02\1b\01\01\01\01\017\0e\01\05\01\02\05\0b\01$\09\01f\04\01\06\01\02\02\02\19\02\04\03\10\04\0d\01\02\02\06\01\0f\01\00\03\00\03\1d\02\1e\02\1e\02@\02\01\07\08\01\02\0b\09\01-\03\01\01u\02\22\01v\03\04\02\09\01\06\03\db\02\02\01:\01\01\07\01\01\01\01\02\08\06\0a\02\010\1f1\040\07\01\01\05\01(\09\0c\02 \04\02\02\01\038\01\01\02\03\01\01\03:\08\02\02\98\03\01\0d\01\07\04\01\06\01\03\02\c6@\00\01\c3!\00\03\8d\01` \00\06i\02\00\04\01\0a \02P\02\00\01\03\01\04\01\19\02\05\01\97\02\1a\12\0d\01&\08\19\0b.\030\01\02\04\02\02'\01C\06\02\02\02\02\0c\01\08\01/\013\01\01\03\02\02\05\02\01\01*\02\08\01\ee\01\02\01\04\01\00\01\00\10\10\10\00\02\00\01\e2\01\95\05\00\03\01\02\05\04(\03\04\01\a5\02\00\04\00\02\99\0b1\04{\016\0f)\01\02\02\0a\031\04\02\02\07\01=\03$\05\01\08>\01\0c\024\09\0a\04\02\01_\03\02\01\01\02\06\01\a0\01\03\08\15\029\02\01\01\01\01\16\01\0e\07\03\05\c3\08\02\03\01\01\17\01Q\01\02\06\01\01\02\01\01\02\01\02\eb\01\02\04\06\02\01\02\1b\02U\08\02\01\01\02j\01\01\01\02\06\01\01e\03\02\04\01\05\00\09\01\02\f5\01\0a\02\01\01\04\01\90\04\02\02\04\01 \0a(\06\02\04\08\01\09\06\02\03.\0d\01\02\00\07\01\06\01\01R\16\02\07\01\02\01\02z\06\03\01\01\02\01\07\01\01H\02\03\01\01\01\00\02\00\05;\07\00\01?\04Q\01\00\02\00.\02\17\00\01\01\03\04\05\08\08\02\07\1e\04\94\03\007\042\08\01\0e\01\16\05\01\0f\00\07\01\11\02\07\01\02\01\05\00\07\00\01=\04\00\07m\07\00`\80\f0\00")
  (data $d1 (i32.const 1059200) "\01\00\00\00\00\00\00\00\01\00\00\00\c4\12\10\00"))
