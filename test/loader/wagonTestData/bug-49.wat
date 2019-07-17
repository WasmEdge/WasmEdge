(module
  (type (;0;) (func (param i32 i32 i32) (result i32)))
  (type (;1;) (func (param i32)))
  (type (;2;) (func (param i32 i32) (result i32)))
  (type (;3;) (func (param i32) (result i64)))
  (type (;4;) (func (param i32 i32 i32)))
  (type (;5;) (func))
  (type (;6;) (func (param i32 i32)))
  (type (;7;) (func (param i32 i32 i32 i32)))
  (type (;8;) (func (result i32)))
  (type (;9;) (func (param i32 i32 i32 i32 i32 i32) (result i32)))
  (type (;10;) (func (param i32 i32 i32 i32 i32 i32 i32) (result i32)))
  (func (;0;) (type 1) (param i32)
    (local i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 0
              i32.const 4
              i32.add
              i32.load
              tee_local 1
              i32.eqz
              br_if 0 (;@5;)
              get_local 1
              i32.const 3
              i32.shl
              tee_local 2
              i32.const -1
              i32.le_s
              br_if 2 (;@3;)
              i32.const 1744
              get_local 0
              i32.load
              get_local 2
              call 58
              tee_local 2
              i32.eqz
              br_if 3 (;@2;)
              get_local 1
              i32.const 1
              i32.shl
              set_local 1
              br 1 (;@4;)
            end
            i32.const 1744
            i32.const 16
            call 57
            tee_local 2
            i32.eqz
            br_if 3 (;@1;)
            i32.const 4
            set_local 1
          end
          get_local 0
          get_local 2
          i32.store
          get_local 0
          i32.const 4
          i32.add
          get_local 1
          i32.store
          return
        end
        i32.const 84
        call 87
        unreachable
      end
      unreachable
      unreachable
    end
    unreachable
    unreachable)
  (func (;1;) (type 8) (result i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    i32.const 0
    i32.store offset=8
    get_local 2
    i64.const 4
    i64.store
    get_local 2
    call 0
    get_local 2
    i32.load
    tee_local 0
    get_local 2
    i32.load offset=8
    tee_local 1
    i32.const 2
    i32.shl
    i32.add
    i32.const 1
    i32.store
    get_local 2
    get_local 1
    i32.const 1
    i32.add
    tee_local 1
    i32.store offset=8
    block  ;; label = @1
      get_local 2
      i32.load offset=4
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1744
      get_local 0
      call 60
    end
    i32.const 0
    get_local 2
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;2;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 7
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 0
                  i32.load offset=4
                  tee_local 6
                  get_local 0
                  i32.load offset=8
                  tee_local 3
                  i32.sub
                  get_local 2
                  i32.ge_u
                  br_if 0 (;@7;)
                  get_local 3
                  get_local 2
                  i32.add
                  tee_local 4
                  get_local 3
                  i32.lt_u
                  br_if 4 (;@3;)
                  get_local 4
                  get_local 6
                  i32.const 1
                  i32.shl
                  tee_local 5
                  get_local 4
                  get_local 5
                  i32.ge_u
                  select
                  tee_local 5
                  i32.const -1
                  i32.le_s
                  br_if 5 (;@2;)
                  get_local 6
                  i32.eqz
                  br_if 1 (;@6;)
                  i32.const 1744
                  get_local 0
                  i32.load
                  get_local 5
                  call 58
                  tee_local 6
                  br_if 2 (;@5;)
                  get_local 7
                  get_local 5
                  i32.store offset=8
                  get_local 7
                  get_local 6
                  i32.store offset=4
                  get_local 7
                  i32.const 1
                  i32.store offset=12
                  unreachable
                  unreachable
                end
                get_local 3
                get_local 2
                i32.add
                set_local 4
                get_local 0
                i32.load
                set_local 6
                br 2 (;@4;)
              end
              i32.const 1744
              get_local 5
              call 57
              tee_local 6
              i32.eqz
              br_if 4 (;@1;)
            end
            get_local 0
            get_local 6
            i32.store
            get_local 0
            i32.const 4
            i32.add
            get_local 5
            i32.store
          end
          get_local 0
          i32.const 8
          i32.add
          get_local 4
          i32.store
          get_local 6
          get_local 3
          i32.add
          get_local 1
          get_local 2
          call 56
          drop
          i32.const 0
          get_local 7
          i32.const 16
          i32.add
          i32.store offset=4
          return
        end
        i32.const 816
        call 86
        unreachable
      end
      i32.const 836
      call 87
      unreachable
    end
    get_local 7
    get_local 5
    i32.store offset=8
    get_local 7
    get_local 6
    i32.store offset=4
    get_local 7
    i32.const 1
    i32.store offset=12
    unreachable
    unreachable)
  (func (;3;) (type 5)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 0
    i32.store offset=4
    get_local 0
    i32.const 57
    i32.store offset=12
    get_local 0
    i32.const 1520
    i32.store offset=8
    get_local 0
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 1
    i32.store
    get_local 0
    i32.const 2
    i32.store offset=44
    get_local 0
    get_local 0
    i32.const 56
    i32.add
    i32.store offset=48
    get_local 0
    i32.const 4720
    i32.store offset=24
    get_local 0
    i32.const 2
    i32.store offset=20
    get_local 0
    get_local 0
    i32.const 8
    i32.add
    i32.store offset=40
    get_local 0
    i32.const 108
    i32.store offset=16
    get_local 0
    i32.const 16
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 0
    get_local 0
    i32.const 40
    i32.add
    i32.store offset=32
    get_local 0
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    get_local 0
    i32.const 16
    i32.add
    i32.const 124
    call 89
    unreachable)
  (func (;4;) (type 5)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 0
    i32.store offset=4
    get_local 0
    i32.const 24
    i32.store offset=12
    get_local 0
    i32.const 1680
    i32.store offset=8
    get_local 0
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 3
    i32.store
    get_local 0
    i32.const 2
    i32.store offset=44
    get_local 0
    get_local 0
    i32.const 56
    i32.add
    i32.store offset=48
    get_local 0
    i32.const 4720
    i32.store offset=24
    get_local 0
    i32.const 2
    i32.store offset=20
    get_local 0
    get_local 0
    i32.const 8
    i32.add
    i32.store offset=40
    get_local 0
    i32.const 108
    i32.store offset=16
    get_local 0
    i32.const 16
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 0
    get_local 0
    i32.const 40
    i32.add
    i32.store offset=32
    get_local 0
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    get_local 0
    i32.const 16
    i32.add
    i32.const 124
    call 89
    unreachable)
  (func (;5;) (type 5)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 0
    i32.store offset=4
    get_local 0
    i32.const 16
    i32.store offset=12
    get_local 0
    i32.const 1664
    i32.store offset=8
    get_local 0
    i32.const 40
    i32.add
    i32.const 12
    i32.add
    i32.const 4
    i32.store
    get_local 0
    i32.const 2
    i32.store offset=44
    get_local 0
    get_local 0
    i32.const 56
    i32.add
    i32.store offset=48
    get_local 0
    i32.const 4720
    i32.store offset=24
    get_local 0
    i32.const 2
    i32.store offset=20
    get_local 0
    get_local 0
    i32.const 8
    i32.add
    i32.store offset=40
    get_local 0
    i32.const 108
    i32.store offset=16
    get_local 0
    i32.const 16
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 0
    get_local 0
    i32.const 40
    i32.add
    i32.store offset=32
    get_local 0
    i32.const 36
    i32.add
    i32.const 2
    i32.store
    get_local 0
    i32.const 16
    i32.add
    i32.const 124
    call 89
    unreachable)
  (func (;6;) (type 6) (param i32 i32)
    get_local 0
    i32.const 0
    i32.store)
  (func (;7;) (type 3) (param i32) (result i64)
    i64.const 3794405437016395809)
  (func (;8;) (type 6) (param i32 i32)
    get_local 0
    get_local 1
    i32.load offset=8
    i32.store offset=4
    get_local 0
    get_local 1
    i32.load
    i32.store)
  (func (;9;) (type 2) (param i32 i32) (result i32)
    get_local 1
    get_local 0
    i32.load
    get_local 0
    i32.load offset=8
    call 71)
  (func (;10;) (type 2) (param i32 i32) (result i32)
    (local i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    get_local 1
    i32.load offset=24
    i32.const 416
    i32.const 11
    get_local 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 0)
    i32.store8 offset=8
    get_local 3
    get_local 1
    i32.store
    get_local 3
    i32.const 0
    i32.store offset=4
    get_local 3
    i32.const 0
    i32.store8 offset=9
    get_local 3
    get_local 0
    i32.store offset=12
    get_local 3
    get_local 3
    i32.const 12
    i32.add
    i32.const 428
    call 84
    drop
    get_local 3
    i32.load8_u offset=8
    set_local 1
    block  ;; label = @1
      get_local 3
      i32.load offset=4
      tee_local 2
      i32.eqz
      br_if 0 (;@1;)
      get_local 1
      i32.const 255
      i32.and
      set_local 0
      i32.const 1
      set_local 1
      block  ;; label = @2
        get_local 0
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 3
          i32.load
          tee_local 0
          i32.load8_u
          i32.const 4
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1
          set_local 1
          get_local 0
          i32.load offset=24
          i32.const 3008
          i32.const 1
          get_local 0
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 0)
          br_if 1 (;@2;)
        end
        block  ;; label = @3
          get_local 2
          i32.const 1
          i32.ne
          br_if 0 (;@3;)
          get_local 3
          i32.const 9
          i32.add
          i32.load8_u
          i32.const 255
          i32.and
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1
          set_local 1
          get_local 0
          i32.load offset=24
          i32.const 2944
          i32.const 1
          get_local 0
          i32.const 28
          i32.add
          i32.load
          i32.load offset=12
          call_indirect (type 0)
          br_if 1 (;@2;)
        end
        get_local 0
        i32.load offset=24
        i32.const 3152
        i32.const 1
        get_local 0
        i32.const 28
        i32.add
        i32.load
        i32.load offset=12
        call_indirect (type 0)
        set_local 1
      end
      get_local 3
      i32.const 8
      i32.add
      get_local 1
      i32.store8
    end
    i32.const 0
    get_local 3
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1
    i32.const 255
    i32.and
    i32.const 0
    i32.ne)
  (func (;11;) (type 1) (param i32))
  (func (;12;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 7
    i32.store offset=4
    i32.const 0
    set_local 3
    get_local 7
    i32.const 0
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        get_local 1
        i32.const 127
        i32.gt_u
        br_if 0 (;@2;)
        get_local 7
        get_local 1
        i32.store8 offset=4
        i32.const 1
        set_local 6
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          get_local 1
          i32.const 2048
          i32.ge_u
          br_if 0 (;@3;)
          i32.const 2
          set_local 6
          i32.const 1
          set_local 5
          i32.const 192
          set_local 4
          i32.const 31
          set_local 2
          br 1 (;@2;)
        end
        block  ;; label = @3
          get_local 1
          i32.const 65535
          i32.gt_u
          br_if 0 (;@3;)
          get_local 7
          get_local 1
          i32.const 12
          i32.shr_u
          i32.const 15
          i32.and
          i32.const 224
          i32.or
          i32.store8 offset=4
          i32.const 3
          set_local 6
          i32.const 2
          set_local 5
          i32.const 128
          set_local 4
          i32.const 1
          set_local 3
          i32.const 63
          set_local 2
          br 1 (;@2;)
        end
        get_local 7
        get_local 1
        i32.const 18
        i32.shr_u
        i32.const 240
        i32.or
        i32.store8 offset=4
        i32.const 63
        set_local 2
        i32.const 128
        set_local 4
        get_local 7
        get_local 1
        i32.const 12
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=5
        i32.const 4
        set_local 6
        i32.const 3
        set_local 5
        i32.const 2
        set_local 3
      end
      get_local 7
      i32.const 4
      i32.add
      get_local 3
      i32.add
      get_local 2
      get_local 1
      i32.const 6
      i32.shr_u
      i32.and
      get_local 4
      i32.or
      i32.store8
      get_local 7
      i32.const 4
      i32.add
      get_local 5
      i32.add
      get_local 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8
    end
    get_local 7
    i32.const 8
    i32.add
    get_local 7
    i32.const 4
    i32.add
    get_local 6
    call 46
    i32.const 0
    set_local 1
    block  ;; label = @1
      get_local 7
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      get_local 7
      i64.load offset=8
      set_local 8
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          get_local 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        get_local 0
        i32.const 8
        i32.add
        i32.load
        tee_local 1
        i32.load
        get_local 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          get_local 1
          i32.load offset=4
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 1
          i32.load
          call 60
        end
        i32.const 1744
        get_local 1
        call 60
      end
      get_local 0
      i32.const 4
      i32.add
      get_local 8
      i64.store align=4
      i32.const 1
      set_local 1
    end
    i32.const 0
    get_local 7
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;13;) (type 2) (param i32 i32) (result i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    get_local 0
    i32.store offset=4
    get_local 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 2
    i32.const 4
    i32.add
    i32.const 444
    get_local 2
    i32.const 8
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 2
    i32.const 32
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;14;) (type 1) (param i32))
  (func (;15;) (type 1) (param i32)
    block  ;; label = @1
      get_local 0
      i32.load offset=4
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1744
      get_local 0
      i32.load
      call 60
    end)
  (func (;16;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    i32.const 8
    i32.add
    get_local 1
    get_local 2
    call 46
    i32.const 0
    set_local 1
    block  ;; label = @1
      get_local 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      get_local 3
      i64.load offset=8
      set_local 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          get_local 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        get_local 0
        i32.const 8
        i32.add
        i32.load
        tee_local 1
        i32.load
        get_local 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          get_local 1
          i32.load offset=4
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 1
          i32.load
          call 60
        end
        i32.const 1744
        get_local 1
        call 60
      end
      get_local 0
      i32.const 4
      i32.add
      get_local 4
      i64.store align=4
      i32.const 1
      set_local 1
    end
    i32.const 0
    get_local 3
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;17;) (type 7) (param i32 i32 i32 i32)
    (local i32 i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 6
    i32.store offset=4
    block  ;; label = @1
      get_local 3
      i32.const -1
      i32.le_s
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          get_local 3
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 3
          call 57
          tee_local 5
          br_if 1 (;@2;)
          get_local 6
          get_local 3
          i32.store offset=52
          get_local 6
          get_local 5
          i32.store offset=48
          get_local 6
          i32.const 1
          i32.store offset=56
          unreachable
          unreachable
        end
        i32.const 1
        set_local 5
      end
      get_local 6
      get_local 3
      i32.store offset=36
      get_local 6
      get_local 5
      i32.store offset=32
      get_local 6
      i32.const 0
      i32.store offset=40
      get_local 6
      i32.const 32
      i32.add
      get_local 2
      get_local 3
      call 61
      get_local 6
      i32.const 48
      i32.add
      i32.const 8
      i32.add
      tee_local 3
      get_local 6
      i32.load offset=40
      tee_local 2
      i32.store
      get_local 6
      i32.const 16
      i32.add
      i32.const 8
      i32.add
      tee_local 5
      get_local 2
      i32.store
      get_local 6
      get_local 6
      i32.load offset=32
      tee_local 2
      i32.store offset=48
      get_local 6
      get_local 6
      i32.load offset=36
      tee_local 4
      i32.store offset=52
      get_local 6
      get_local 4
      i32.store offset=20
      get_local 6
      get_local 2
      i32.store offset=16
      get_local 3
      get_local 5
      i32.load
      tee_local 2
      i32.store
      get_local 6
      i32.const 8
      i32.add
      tee_local 5
      get_local 2
      i32.store
      get_local 6
      get_local 6
      i32.load offset=16
      tee_local 2
      i32.store offset=48
      get_local 6
      get_local 6
      i32.load offset=20
      tee_local 4
      i32.store offset=52
      get_local 6
      get_local 4
      i32.store offset=4
      get_local 6
      get_local 2
      i32.store
      get_local 3
      get_local 5
      i32.load
      tee_local 2
      i32.store
      get_local 6
      i32.const 32
      i32.add
      i32.const 8
      i32.add
      tee_local 4
      get_local 2
      i32.store
      get_local 6
      get_local 6
      i32.load
      tee_local 2
      i32.store offset=48
      get_local 6
      get_local 6
      i32.load offset=4
      tee_local 5
      i32.store offset=52
      get_local 6
      get_local 5
      i32.store offset=36
      get_local 6
      get_local 2
      i32.store offset=32
      block  ;; label = @2
        i32.const 1744
        i32.const 12
        call 57
        tee_local 2
        i32.eqz
        br_if 0 (;@2;)
        get_local 2
        i32.const 8
        i32.add
        get_local 4
        i32.load
        tee_local 5
        i32.store
        get_local 3
        get_local 5
        i32.store
        get_local 2
        get_local 6
        i64.load offset=32
        tee_local 7
        i64.store align=4
        get_local 6
        get_local 7
        i64.store offset=48
        i32.const 1744
        i32.const 12
        call 57
        tee_local 3
        i32.eqz
        br_if 0 (;@2;)
        get_local 3
        get_local 2
        i32.store
        get_local 3
        i32.const 468
        i32.store offset=4
        get_local 6
        i32.const 48
        i32.add
        i32.const 2
        i32.add
        tee_local 2
        get_local 6
        i32.const 32
        i32.add
        i32.const 2
        i32.add
        i32.load8_u
        i32.store8
        get_local 6
        get_local 6
        i32.load16_u offset=32 align=1
        i32.store16 offset=48
        get_local 3
        get_local 1
        i32.store8 offset=8
        get_local 3
        i32.const 11
        i32.add
        get_local 2
        i32.load8_u
        i32.store8
        get_local 3
        get_local 6
        i32.load16_u offset=48
        i32.store16 offset=9 align=1
        get_local 0
        i32.const 2
        i32.store8
        get_local 0
        get_local 3
        i32.store offset=4
        get_local 0
        i32.const 3
        i32.add
        get_local 2
        i32.load8_u
        i32.store8
        get_local 0
        get_local 6
        i32.load16_u offset=48 align=1
        i32.store16 offset=1 align=1
        i32.const 0
        get_local 6
        i32.const 64
        i32.add
        i32.store offset=4
        return
      end
      unreachable
      unreachable
    end
    i32.const 2196
    call 87
    unreachable)
  (func (;18;) (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      get_local 0
      i32.load offset=16
      tee_local 1
      i32.eqz
      br_if 0 (;@1;)
      get_local 1
      i32.const 0
      i32.store8
      get_local 0
      i32.const 20
      i32.add
      i32.load
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1744
      get_local 0
      i32.const 16
      i32.add
      i32.load
      call 60
    end
    i32.const 1744
    get_local 0
    i32.const 28
    i32.add
    i32.load
    call 60
    get_local 0
    get_local 0
    i32.load offset=4
    tee_local 1
    i32.const -1
    i32.add
    i32.store offset=4
    block  ;; label = @1
      get_local 1
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      i32.const 1744
      get_local 0
      call 60
    end)
  (func (;19;) (type 3) (param i32) (result i64)
    i64.const -5063177421214143757)
  (func (;20;) (type 1) (param i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 3
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            get_local 0
            i32.load offset=4
            tee_local 2
            i32.eqz
            br_if 0 (;@4;)
            get_local 2
            i32.const 1
            i32.shl
            tee_local 2
            i32.const -1
            i32.le_s
            br_if 2 (;@2;)
            i32.const 1744
            get_local 0
            i32.load
            get_local 2
            call 58
            tee_local 1
            br_if 1 (;@3;)
            unreachable
            unreachable
          end
          i32.const 4
          set_local 2
          i32.const 1744
          i32.const 4
          call 57
          tee_local 1
          i32.eqz
          br_if 2 (;@1;)
        end
        get_local 0
        get_local 1
        i32.store
        get_local 0
        i32.const 4
        i32.add
        get_local 2
        i32.store
        i32.const 0
        get_local 3
        i32.const 16
        i32.add
        i32.store offset=4
        return
      end
      i32.const 836
      call 87
      unreachable
    end
    get_local 3
    i32.const 4
    i32.store offset=8
    get_local 3
    i32.const 1
    i32.store offset=12
    get_local 3
    get_local 1
    i32.store offset=4
    get_local 3
    get_local 3
    i64.load offset=8 align=4
    i64.store offset=8 align=4
    get_local 3
    get_local 1
    i32.store offset=4
    unreachable
    unreachable)
  (func (;21;) (type 2) (param i32 i32) (result i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    get_local 0
    i32.store offset=4
    get_local 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 2
    i32.const 4
    i32.add
    i32.const 900
    get_local 2
    i32.const 8
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 2
    i32.const 32
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;22;) (type 1) (param i32))
  (func (;23;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
    get_local 0
    i32.load
    tee_local 0
    i32.load offset=8
    set_local 3
    get_local 0
    i32.load
    set_local 2
    i32.const 1
    set_local 12
    block  ;; label = @1
      get_local 1
      i32.load offset=24
      tee_local 4
      i32.const 34
      get_local 1
      i32.const 28
      i32.add
      i32.load
      tee_local 5
      i32.load offset=16
      tee_local 6
      call_indirect (type 2)
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 3
                i32.eqz
                br_if 0 (;@6;)
                get_local 2
                get_local 3
                i32.add
                set_local 7
                get_local 2
                i32.const 1
                i32.add
                set_local 0
                i32.const 0
                set_local 11
                get_local 2
                i32.load8_s
                tee_local 1
                i32.const 0
                i32.lt_s
                br_if 1 (;@5;)
                get_local 1
                i32.const 255
                i32.and
                set_local 17
                br 2 (;@4;)
              end
              i32.const 0
              set_local 11
              br 2 (;@3;)
            end
            get_local 7
            set_local 8
            block  ;; label = @5
              get_local 3
              i32.const 1
              i32.eq
              br_if 0 (;@5;)
              get_local 2
              i32.const 1
              i32.add
              i32.load8_u
              i32.const 63
              i32.and
              set_local 11
              get_local 2
              i32.const 2
              i32.add
              tee_local 0
              set_local 8
            end
            get_local 1
            i32.const 31
            i32.and
            set_local 17
            get_local 11
            i32.const 255
            i32.and
            set_local 11
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 1
                  i32.const 255
                  i32.and
                  i32.const 224
                  i32.lt_u
                  br_if 0 (;@7;)
                  get_local 8
                  get_local 7
                  i32.eq
                  br_if 1 (;@6;)
                  get_local 8
                  i32.load8_u
                  i32.const 63
                  i32.and
                  set_local 10
                  get_local 8
                  i32.const 1
                  i32.add
                  tee_local 0
                  set_local 8
                  br 2 (;@5;)
                end
                get_local 11
                get_local 17
                i32.const 6
                i32.shl
                i32.or
                set_local 17
                br 2 (;@4;)
              end
              i32.const 0
              set_local 10
              get_local 7
              set_local 8
            end
            get_local 10
            i32.const 255
            i32.and
            get_local 11
            i32.const 6
            i32.shl
            i32.or
            set_local 10
            block  ;; label = @5
              get_local 1
              i32.const 255
              i32.and
              i32.const 240
              i32.lt_u
              br_if 0 (;@5;)
              i32.const 0
              set_local 11
              i32.const 0
              set_local 1
              block  ;; label = @6
                get_local 8
                get_local 7
                i32.eq
                br_if 0 (;@6;)
                get_local 8
                i32.const 1
                i32.add
                set_local 0
                get_local 8
                i32.load8_u
                i32.const 63
                i32.and
                set_local 1
              end
              get_local 10
              i32.const 6
              i32.shl
              get_local 17
              i32.const 18
              i32.shl
              i32.const 1835008
              i32.and
              i32.or
              get_local 1
              i32.const 255
              i32.and
              i32.or
              tee_local 17
              i32.const 1114112
              i32.ne
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            get_local 10
            get_local 17
            i32.const 12
            i32.shl
            i32.or
            set_local 17
          end
          get_local 3
          get_local 0
          i32.add
          set_local 9
          i32.const 0
          set_local 8
          i32.const 0
          set_local 11
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  loop  ;; label = @8
                    get_local 0
                    set_local 1
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 17
                                    i32.const 2097151
                                    i32.and
                                    tee_local 0
                                    i32.const -9
                                    i32.add
                                    tee_local 10
                                    i32.const 30
                                    i32.gt_u
                                    br_if 0 (;@16;)
                                    i32.const 116
                                    set_local 14
                                    i32.const 2
                                    set_local 0
                                    block  ;; label = @17
                                      get_local 10
                                      br_table 8 (;@9;) 0 (;@17;) 3 (;@14;) 3 (;@14;) 4 (;@13;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 2 (;@15;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 3 (;@14;) 2 (;@15;) 8 (;@9;)
                                    end
                                    i32.const 110
                                    set_local 14
                                    br 4 (;@12;)
                                  end
                                  get_local 0
                                  i32.const 92
                                  i32.ne
                                  br_if 1 (;@14;)
                                end
                                get_local 17
                                set_local 14
                                i32.const 2
                                set_local 0
                                br 5 (;@9;)
                              end
                              get_local 17
                              i32.const 65535
                              i32.gt_u
                              br_if 2 (;@11;)
                              i32.const 1
                              set_local 0
                              get_local 17
                              set_local 14
                              get_local 17
                              i32.const 3168
                              i32.const 41
                              i32.const 3264
                              i32.const 304
                              i32.const 3568
                              i32.const 326
                              call 85
                              i32.eqz
                              br_if 3 (;@10;)
                              br 4 (;@9;)
                            end
                            i32.const 114
                            set_local 14
                          end
                          i32.const 2
                          set_local 0
                          br 2 (;@9;)
                        end
                        block  ;; label = @11
                          get_local 17
                          i32.const 131072
                          i32.ge_u
                          br_if 0 (;@11;)
                          i32.const 1
                          set_local 0
                          get_local 17
                          set_local 14
                          get_local 17
                          i32.const 3904
                          i32.const 33
                          i32.const 3984
                          i32.const 150
                          i32.const 4144
                          i32.const 360
                          call 85
                          i32.eqz
                          br_if 1 (;@10;)
                          br 2 (;@9;)
                        end
                        get_local 17
                        i32.const -918000
                        i32.add
                        i32.const 196112
                        i32.lt_u
                        br_if 0 (;@10;)
                        i32.const 1
                        set_local 0
                        get_local 17
                        set_local 14
                        get_local 17
                        i32.const -195102
                        i32.add
                        i32.const 722657
                        i32.gt_u
                        get_local 17
                        i32.const -191457
                        i32.add
                        i32.const 3102
                        i32.gt_u
                        get_local 17
                        i32.const -183970
                        i32.add
                        i32.const 13
                        i32.gt_u
                        get_local 17
                        i32.const 2097150
                        i32.and
                        i32.const 178206
                        i32.ne
                        get_local 17
                        i32.const -173783
                        i32.add
                        i32.const 40
                        i32.gt_u
                        get_local 17
                        i32.const -177973
                        i32.add
                        i32.const 10
                        i32.gt_u
                        i32.and
                        i32.and
                        i32.and
                        i32.and
                        i32.and
                        br_if 1 (;@9;)
                      end
                      get_local 17
                      i32.const 1
                      i32.or
                      i32.clz
                      i32.const 2
                      i32.shr_u
                      i32.const 7
                      i32.xor
                      i64.extend_u/i32
                      i64.const 21474836480
                      i64.or
                      set_local 18
                      i32.const 3
                      set_local 0
                      get_local 17
                      set_local 14
                    end
                    block  ;; label = @9
                      get_local 0
                      i32.const 3
                      i32.and
                      tee_local 10
                      i32.const 1
                      i32.eq
                      br_if 0 (;@9;)
                      block  ;; label = @10
                        get_local 10
                        i32.const 3
                        i32.ne
                        br_if 0 (;@10;)
                        get_local 18
                        i64.const 32
                        i64.shr_u
                        i32.wrap/i64
                        i32.const 255
                        i32.and
                        i32.const 4
                        i32.xor
                        i32.const 2
                        i32.shl
                        i32.const 4896
                        i32.add
                        i32.load
                        get_local 18
                        i32.wrap/i64
                        i32.add
                        i32.const 1
                        i32.eq
                        br_if 1 (;@9;)
                      end
                      get_local 8
                      get_local 11
                      i32.lt_u
                      br_if 4 (;@5;)
                      block  ;; label = @10
                        get_local 11
                        i32.eqz
                        br_if 0 (;@10;)
                        get_local 11
                        get_local 3
                        i32.eq
                        br_if 0 (;@10;)
                        get_local 11
                        get_local 3
                        i32.ge_u
                        br_if 5 (;@5;)
                        get_local 2
                        get_local 11
                        i32.add
                        i32.load8_s
                        i32.const -65
                        i32.le_s
                        br_if 5 (;@5;)
                      end
                      block  ;; label = @10
                        get_local 8
                        i32.eqz
                        br_if 0 (;@10;)
                        get_local 8
                        get_local 3
                        i32.eq
                        br_if 0 (;@10;)
                        get_local 8
                        get_local 3
                        i32.ge_u
                        br_if 5 (;@5;)
                        get_local 2
                        get_local 8
                        i32.add
                        i32.load8_s
                        i32.const -65
                        i32.le_s
                        br_if 5 (;@5;)
                      end
                      get_local 4
                      get_local 2
                      get_local 11
                      i32.add
                      get_local 8
                      get_local 11
                      i32.sub
                      get_local 5
                      i32.const 12
                      i32.add
                      i32.load
                      call_indirect (type 0)
                      br_if 3 (;@6;)
                      get_local 18
                      i64.const 32
                      i64.shr_u
                      i32.wrap/i64
                      set_local 11
                      get_local 18
                      i32.wrap/i64
                      set_local 15
                      loop  ;; label = @10
                        get_local 11
                        set_local 10
                        block  ;; label = @11
                          get_local 0
                          i32.const 3
                          i32.and
                          tee_local 11
                          i32.const 1
                          i32.ne
                          br_if 0 (;@11;)
                          i32.const 0
                          set_local 0
                          get_local 10
                          set_local 11
                          get_local 4
                          get_local 14
                          get_local 6
                          call_indirect (type 2)
                          i32.eqz
                          br_if 1 (;@10;)
                          br 4 (;@7;)
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 11
                                    i32.const 2
                                    i32.ne
                                    br_if 0 (;@16;)
                                    i32.const 92
                                    set_local 16
                                    i32.const 1
                                    set_local 0
                                    get_local 10
                                    set_local 11
                                    br 1 (;@15;)
                                  end
                                  get_local 11
                                  i32.const 3
                                  i32.ne
                                  br_if 4 (;@11;)
                                  i32.const 4
                                  set_local 11
                                  get_local 10
                                  i32.const 7
                                  i32.and
                                  i32.const -1
                                  i32.add
                                  tee_local 13
                                  i32.const 4
                                  i32.gt_u
                                  br_if 4 (;@11;)
                                  i32.const 92
                                  set_local 16
                                  block  ;; label = @16
                                    get_local 13
                                    br_table 0 (;@16;) 2 (;@14;) 3 (;@13;) 4 (;@12;) 1 (;@15;) 0 (;@16;)
                                  end
                                  i32.const 0
                                  set_local 11
                                  get_local 4
                                  i32.const 125
                                  get_local 6
                                  call_indirect (type 2)
                                  i32.eqz
                                  br_if 5 (;@10;)
                                  br 8 (;@7;)
                                end
                                get_local 4
                                get_local 16
                                get_local 6
                                call_indirect (type 2)
                                i32.eqz
                                br_if 4 (;@10;)
                                br 7 (;@7;)
                              end
                              get_local 10
                              i32.const 1
                              get_local 15
                              select
                              set_local 11
                              get_local 15
                              i32.const 2
                              i32.shl
                              set_local 10
                              get_local 15
                              i32.const -1
                              i32.add
                              i32.const 0
                              get_local 15
                              select
                              set_local 15
                              get_local 4
                              i32.const 48
                              i32.const 87
                              get_local 14
                              get_local 10
                              i32.const 28
                              i32.and
                              i32.shr_u
                              i32.const 15
                              i32.and
                              tee_local 10
                              i32.const 10
                              i32.lt_u
                              select
                              get_local 10
                              i32.add
                              get_local 6
                              call_indirect (type 2)
                              i32.eqz
                              br_if 3 (;@10;)
                              br 6 (;@7;)
                            end
                            i32.const 2
                            set_local 11
                            get_local 4
                            i32.const 123
                            get_local 6
                            call_indirect (type 2)
                            i32.eqz
                            br_if 2 (;@10;)
                            br 5 (;@7;)
                          end
                          i32.const 3
                          set_local 11
                          get_local 4
                          i32.const 117
                          get_local 6
                          call_indirect (type 2)
                          i32.eqz
                          br_if 1 (;@10;)
                          br 4 (;@7;)
                        end
                      end
                      i32.const 1
                      set_local 0
                      block  ;; label = @10
                        get_local 17
                        i32.const 128
                        i32.lt_u
                        br_if 0 (;@10;)
                        i32.const 2
                        set_local 0
                        get_local 17
                        i32.const 2048
                        i32.lt_u
                        br_if 0 (;@10;)
                        i32.const 3
                        i32.const 4
                        get_local 17
                        i32.const 65536
                        i32.lt_u
                        select
                        set_local 0
                      end
                      get_local 0
                      get_local 8
                      i32.add
                      set_local 11
                    end
                    block  ;; label = @9
                      get_local 1
                      get_local 7
                      i32.eq
                      br_if 0 (;@9;)
                      get_local 1
                      i32.eqz
                      br_if 0 (;@9;)
                      get_local 1
                      i32.const 1
                      i32.add
                      set_local 0
                      block  ;; label = @10
                        block  ;; label = @11
                          get_local 1
                          i32.load8_s
                          tee_local 8
                          i32.const 0
                          i32.lt_s
                          br_if 0 (;@11;)
                          get_local 8
                          i32.const 255
                          i32.and
                          set_local 17
                          br 1 (;@10;)
                        end
                        block  ;; label = @11
                          block  ;; label = @12
                            get_local 0
                            get_local 7
                            i32.eq
                            br_if 0 (;@12;)
                            get_local 0
                            i32.load8_u
                            i32.const 63
                            i32.and
                            set_local 10
                            get_local 1
                            i32.const 2
                            i32.add
                            tee_local 17
                            set_local 0
                            br 1 (;@11;)
                          end
                          i32.const 0
                          set_local 10
                          get_local 7
                          set_local 17
                        end
                        get_local 8
                        i32.const 31
                        i32.and
                        set_local 14
                        get_local 10
                        i32.const 255
                        i32.and
                        set_local 10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              get_local 8
                              i32.const 255
                              i32.and
                              tee_local 8
                              i32.const 224
                              i32.lt_u
                              br_if 0 (;@13;)
                              get_local 17
                              get_local 7
                              i32.eq
                              br_if 1 (;@12;)
                              get_local 17
                              i32.load8_u
                              i32.const 63
                              i32.and
                              set_local 16
                              get_local 17
                              i32.const 1
                              i32.add
                              tee_local 0
                              set_local 17
                              br 2 (;@11;)
                            end
                            get_local 10
                            get_local 14
                            i32.const 6
                            i32.shl
                            i32.or
                            set_local 17
                            br 2 (;@10;)
                          end
                          i32.const 0
                          set_local 16
                          get_local 7
                          set_local 17
                        end
                        get_local 16
                        i32.const 255
                        i32.and
                        get_local 10
                        i32.const 6
                        i32.shl
                        i32.or
                        set_local 10
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              get_local 8
                              i32.const 240
                              i32.lt_u
                              br_if 0 (;@13;)
                              get_local 17
                              get_local 7
                              i32.eq
                              br_if 1 (;@12;)
                              get_local 17
                              i32.const 1
                              i32.add
                              set_local 0
                              get_local 17
                              i32.load8_u
                              i32.const 63
                              i32.and
                              set_local 8
                              br 2 (;@11;)
                            end
                            get_local 10
                            get_local 14
                            i32.const 12
                            i32.shl
                            i32.or
                            set_local 17
                            br 2 (;@10;)
                          end
                          i32.const 0
                          set_local 8
                        end
                        get_local 10
                        i32.const 6
                        i32.shl
                        get_local 14
                        i32.const 18
                        i32.shl
                        i32.const 1835008
                        i32.and
                        i32.or
                        get_local 8
                        i32.const 255
                        i32.and
                        i32.or
                        tee_local 17
                        i32.const 1114112
                        i32.eq
                        br_if 1 (;@9;)
                      end
                      get_local 7
                      get_local 1
                      i32.sub
                      get_local 9
                      get_local 7
                      i32.sub
                      tee_local 8
                      i32.add
                      get_local 0
                      i32.add
                      set_local 9
                      br 1 (;@8;)
                    end
                  end
                  get_local 11
                  i32.eqz
                  br_if 4 (;@3;)
                  get_local 3
                  get_local 11
                  i32.eq
                  br_if 4 (;@3;)
                  get_local 3
                  get_local 11
                  i32.le_u
                  br_if 3 (;@4;)
                  get_local 2
                  get_local 11
                  i32.add
                  tee_local 1
                  i32.load8_s
                  i32.const -65
                  i32.le_s
                  br_if 3 (;@4;)
                  br 5 (;@2;)
                end
                i32.const 1
                return
              end
              i32.const 1
              return
            end
            get_local 2
            get_local 3
            get_local 11
            get_local 8
            call 92
            unreachable
          end
          get_local 2
          get_local 3
          get_local 11
          get_local 3
          call 92
          unreachable
        end
        get_local 2
        get_local 11
        i32.add
        set_local 1
      end
      get_local 4
      get_local 1
      get_local 3
      get_local 11
      i32.sub
      get_local 5
      i32.load offset=12
      call_indirect (type 0)
      br_if 0 (;@1;)
      get_local 4
      i32.const 34
      get_local 6
      call_indirect (type 2)
      set_local 12
    end
    get_local 12)
  (func (;24;) (type 8) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    set_local 3
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=924
        tee_local 2
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=928
        set_local 0
        i32.const 1744
        i32.const 8
        call 57
        tee_local 2
        i32.eqz
        br_if 1 (;@1;)
        get_local 2
        i32.const 0
        i32.store
        get_local 2
        get_local 0
        i32.store offset=4
        i32.const 0
        i32.const 0
        i32.load offset=924
        tee_local 0
        get_local 2
        get_local 0
        select
        i32.store offset=924
        get_local 0
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 2
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 2
        call 60
        get_local 0
        set_local 2
      end
      block  ;; label = @2
        get_local 2
        i32.load
        tee_local 2
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          br_if 0 (;@3;)
          i32.const 1744
          i32.const 20
          call 57
          tee_local 2
          i32.eqz
          br_if 2 (;@1;)
          get_local 2
          i32.const 924
          i32.store
          get_local 2
          i32.const 3
          i32.store offset=8
          block  ;; label = @4
            i32.const 0
            i32.load offset=924
            tee_local 3
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=928
            set_local 0
            i32.const 1744
            i32.const 8
            call 57
            tee_local 3
            i32.eqz
            br_if 3 (;@1;)
            get_local 3
            i32.const 0
            i32.store
            get_local 3
            get_local 0
            i32.store offset=4
            i32.const 0
            i32.const 0
            i32.load offset=924
            tee_local 0
            get_local 3
            get_local 0
            select
            i32.store offset=924
            get_local 0
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              get_local 3
              i32.load offset=4
              tee_local 1
              i32.eqz
              br_if 0 (;@5;)
              get_local 3
              i32.load
              get_local 1
              call_indirect (type 1)
            end
            i32.const 1744
            get_local 3
            call 60
            get_local 0
            set_local 3
          end
          get_local 3
          get_local 2
          i32.store
          get_local 2
          i32.const 4
          i32.add
          return
        end
        get_local 2
        i32.const 4
        i32.add
        set_local 3
      end
      get_local 3
      return
    end
    unreachable
    unreachable)
  (func (;25;) (type 1) (param i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 1
        i32.load
        tee_local 4
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 2
        i32.const 1744
        i32.const 8
        call 57
        tee_local 4
        i32.eqz
        br_if 1 (;@1;)
        get_local 4
        get_local 2
        i32.store offset=4
        get_local 4
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 2
        get_local 4
        get_local 2
        select
        i32.store
        get_local 2
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 4
          i32.load offset=4
          tee_local 3
          i32.eqz
          br_if 0 (;@3;)
          get_local 4
          i32.load
          get_local 3
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 4
        call 60
        get_local 2
        set_local 4
      end
      get_local 4
      i32.const 1
      i32.store
      block  ;; label = @2
        get_local 0
        i32.load8_u offset=8
        i32.const 2
        i32.and
        br_if 0 (;@2;)
        get_local 0
        i32.load offset=16
        tee_local 4
        get_local 4
        i32.load
        tee_local 4
        i32.const -1
        i32.add
        i32.store
        get_local 4
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        get_local 0
        i32.const 16
        i32.add
        i32.load
        call 18
      end
      i32.const 1744
      get_local 0
      call 60
      block  ;; label = @2
        get_local 1
        i32.load
        tee_local 0
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 4
        i32.const 1744
        i32.const 8
        call 57
        tee_local 0
        i32.eqz
        br_if 1 (;@1;)
        get_local 0
        get_local 4
        i32.store offset=4
        get_local 0
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 4
        get_local 0
        get_local 4
        select
        i32.store
        get_local 4
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 0
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 0
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 0
        call 60
        get_local 4
        set_local 0
      end
      get_local 0
      i32.const 0
      i32.store
      return
    end
    unreachable
    unreachable)
  (func (;26;) (type 1) (param i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 1
        i32.load
        tee_local 4
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 2
        i32.const 1744
        i32.const 8
        call 57
        tee_local 4
        i32.eqz
        br_if 1 (;@1;)
        get_local 4
        get_local 2
        i32.store offset=4
        get_local 4
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 2
        get_local 4
        get_local 2
        select
        i32.store
        get_local 2
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 4
          i32.load offset=4
          tee_local 3
          i32.eqz
          br_if 0 (;@3;)
          get_local 4
          i32.load
          get_local 3
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 4
        call 60
        get_local 2
        set_local 4
      end
      get_local 4
      i32.const 1
      i32.store
      i32.const 1744
      get_local 0
      call 60
      block  ;; label = @2
        get_local 1
        i32.load
        tee_local 4
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 0
        i32.const 1744
        i32.const 8
        call 57
        tee_local 4
        i32.eqz
        br_if 1 (;@1;)
        get_local 4
        get_local 0
        i32.store offset=4
        get_local 4
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 0
        get_local 4
        get_local 0
        select
        i32.store
        get_local 0
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 4
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 4
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 4
        call 60
        get_local 0
        set_local 4
      end
      get_local 4
      i32.const 0
      i32.store
      return
    end
    unreachable
    unreachable)
  (func (;27;) (type 1) (param i32)
    (local i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 1
        i32.load
        tee_local 4
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 2
        i32.const 1744
        i32.const 8
        call 57
        tee_local 4
        i32.eqz
        br_if 1 (;@1;)
        get_local 4
        get_local 2
        i32.store offset=4
        get_local 4
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 2
        get_local 4
        get_local 2
        select
        i32.store
        get_local 2
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 4
          i32.load offset=4
          tee_local 3
          i32.eqz
          br_if 0 (;@3;)
          get_local 4
          i32.load
          get_local 3
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 4
        call 60
        get_local 2
        set_local 4
      end
      get_local 4
      i32.const 1
      i32.store
      block  ;; label = @2
        get_local 0
        i32.load offset=4
        i32.eqz
        br_if 0 (;@2;)
        get_local 0
        i32.load offset=12
        tee_local 4
        i32.eqz
        br_if 0 (;@2;)
        get_local 4
        get_local 0
        i32.load offset=16
        i32.load
        call_indirect (type 1)
        get_local 0
        i32.load offset=16
        i32.load offset=4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1744
        get_local 0
        i32.const 12
        i32.add
        i32.load
        call 60
      end
      i32.const 1744
      get_local 0
      call 60
      block  ;; label = @2
        get_local 1
        i32.load
        tee_local 0
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=4
        set_local 4
        i32.const 1744
        i32.const 8
        call 57
        tee_local 0
        i32.eqz
        br_if 1 (;@1;)
        get_local 0
        get_local 4
        i32.store offset=4
        get_local 0
        i32.const 0
        i32.store
        get_local 1
        get_local 1
        i32.load
        tee_local 4
        get_local 0
        get_local 4
        select
        i32.store
        get_local 4
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 0
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 0
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 0
        call 60
        get_local 4
        set_local 0
      end
      get_local 0
      i32.const 0
      i32.store
      return
    end
    unreachable
    unreachable)
  (func (;28;) (type 1) (param i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 1
    i32.store offset=4
    get_local 1
    i32.const 16
    i32.add
    i32.const 16
    i32.add
    get_local 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 1
    i32.const 16
    i32.add
    i32.const 8
    i32.add
    get_local 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 1
    get_local 0
    i64.load align=4
    i64.store offset=16
    get_local 1
    i32.const 8
    i32.add
    get_local 1
    i32.const 40
    i32.add
    get_local 1
    i32.const 16
    i32.add
    call 47
    get_local 1
    i32.load offset=12
    set_local 0
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        get_local 1
        i32.load offset=8
        i32.const 3
        i32.and
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      get_local 0
      i32.load
      get_local 0
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        get_local 0
        i32.load offset=4
        i32.load offset=4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1744
        get_local 0
        i32.load
        call 60
      end
      i32.const 1744
      get_local 0
      call 60
    end
    i32.const 0
    get_local 1
    i32.const 48
    i32.add
    i32.store offset=4)
  (func (;29;) (type 4) (param i32 i32 i32)
    (local i32)
    block  ;; label = @1
      i32.const 1744
      i32.const 8
      call 57
      tee_local 3
      br_if 0 (;@1;)
      unreachable
      unreachable
    end
    get_local 3
    get_local 1
    i32.store offset=4
    get_local 3
    get_local 0
    i32.store
    get_local 3
    i32.const 1452
    get_local 2
    call 32
    unreachable)
  (func (;30;) (type 6) (param i32 i32)
    (local i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    i32.const 0
    i32.store offset=16
    get_local 3
    i64.const 1
    i64.store offset=8
    get_local 3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    get_local 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    tee_local 2
    get_local 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    get_local 0
    i64.load align=4
    i64.store offset=24
    get_local 3
    i32.const 8
    i32.add
    get_local 3
    i32.const 24
    i32.add
    call 21
    drop
    get_local 2
    get_local 3
    i32.load offset=16
    i32.store
    get_local 3
    get_local 3
    i64.load offset=8
    i64.store offset=24
    get_local 3
    i32.const 24
    i32.add
    get_local 1
    call 31
    unreachable)
  (func (;31;) (type 6) (param i32 i32)
    (local i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    i32.const 8
    i32.add
    tee_local 2
    get_local 0
    i32.const 8
    i32.add
    i32.load
    i32.store
    get_local 3
    get_local 0
    i64.load align=4
    i64.store
    block  ;; label = @1
      i32.const 1744
      i32.const 12
      call 57
      tee_local 0
      br_if 0 (;@1;)
      unreachable
      unreachable
    end
    get_local 3
    i32.const 16
    i32.add
    i32.const 8
    i32.add
    get_local 2
    i32.load
    tee_local 2
    i32.store
    get_local 0
    get_local 3
    i64.load
    tee_local 4
    i64.store align=4
    get_local 0
    i32.const 8
    i32.add
    get_local 2
    i32.store
    get_local 3
    get_local 4
    i64.store offset=16
    get_local 0
    i32.const 932
    get_local 1
    call 32
    unreachable)
  (func (;32;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 96
    i32.sub
    tee_local 9
    i32.store offset=4
    get_local 2
    i32.load offset=12
    set_local 6
    get_local 2
    i32.load offset=8
    set_local 5
    get_local 2
    i32.load offset=4
    set_local 4
    get_local 2
    i32.load
    set_local 3
    block  ;; label = @1
      call 40
      tee_local 2
      br_if 0 (;@1;)
      call 3
      unreachable
    end
    i32.const 1
    set_local 7
    block  ;; label = @1
      block  ;; label = @2
        get_local 2
        i32.load
        i32.const 1
        i32.ne
        br_if 0 (;@2;)
        get_local 2
        get_local 2
        i32.load offset=4
        i32.const 1
        i32.add
        tee_local 7
        i32.store offset=4 align=1
        get_local 7
        i32.const 3
        i32.lt_u
        br_if 1 (;@1;)
        get_local 9
        i32.const 76
        i32.add
        i32.const 0
        i32.store
        get_local 9
        i32.const 1
        i32.store offset=60
        get_local 9
        i32.const 948
        i32.store offset=56
        get_local 9
        i32.const 0
        i32.store offset=64
        get_local 9
        i32.const 4852
        i32.store offset=72
        get_local 9
        i32.const 56
        i32.add
        call 28
        unreachable
        unreachable
      end
      get_local 2
      i64.const 1
      i64.store align=4
      get_local 2
      i32.const 1
      i32.store offset=4 align=1
    end
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=956
        tee_local 2
        i32.const -1
        i32.le_s
        br_if 0 (;@2;)
        i32.const 0
        get_local 2
        i32.const 1
        i32.add
        i32.store offset=956
        call 40
        tee_local 2
        br_if 1 (;@1;)
        call 3
        unreachable
      end
      i32.const 960
      i32.const 25
      i32.const 988
      call 29
      unreachable
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          get_local 2
          i32.load
          i32.const 1
          i32.ne
          br_if 0 (;@3;)
          i32.const 2
          set_local 8
          get_local 2
          i32.load offset=4
          i32.const 1
          i32.le_u
          br_if 1 (;@2;)
          br 2 (;@1;)
        end
        get_local 2
        i64.const 1
        i64.store align=4
        get_local 2
        i32.const 0
        i32.store offset=4 align=1
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            i32.const 0
            i32.load offset=292
            tee_local 2
            i32.const 3
            i32.gt_u
            br_if 0 (;@4;)
            i32.const 4
            set_local 8
            block  ;; label = @5
              get_local 2
              br_table 0 (;@5;) 4 (;@1;) 2 (;@3;) 3 (;@2;) 0 (;@5;)
            end
            i32.const 0
            i32.const 1
            i32.store offset=292
            br 3 (;@1;)
          end
          i32.const 304
          i32.const 40
          i32.const 344
          call 29
          unreachable
        end
        i32.const 2
        set_local 8
        br 1 (;@1;)
      end
      i32.const 3
      set_local 8
    end
    get_local 9
    get_local 8
    i32.store8 offset=15
    get_local 9
    get_local 4
    i32.store offset=20
    get_local 9
    get_local 3
    i32.store offset=16
    get_local 9
    get_local 5
    i32.store offset=24
    get_local 9
    get_local 6
    i32.store offset=28
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        get_local 1
        i32.load offset=12
        tee_local 2
        call_indirect (type 3)
        i64.const 1229646359891580772
        i64.ne
        br_if 0 (;@2;)
        get_local 9
        get_local 0
        i32.load
        i32.store offset=32
        get_local 0
        i32.load offset=4
        set_local 2
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          get_local 0
          get_local 2
          call_indirect (type 3)
          i64.const -5063177421214143757
          i64.ne
          br_if 0 (;@3;)
          get_local 0
          i32.load offset=8
          set_local 2
          get_local 0
          i32.load
          set_local 0
          br 1 (;@2;)
        end
        i32.const 8
        set_local 2
        i32.const 1088
        set_local 0
      end
      get_local 9
      get_local 0
      i32.store offset=32
    end
    get_local 9
    get_local 2
    i32.store offset=36
    i32.const 1
    set_local 2
    get_local 9
    i32.const 1
    i32.store8 offset=47
    block  ;; label = @1
      block  ;; label = @2
        call 45
        tee_local 6
        br_if 0 (;@2;)
        i32.const 0
        set_local 0
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          get_local 6
          i32.load offset=16
          tee_local 0
          br_if 0 (;@3;)
          i32.const 0
          set_local 0
          br 1 (;@2;)
        end
        get_local 6
        i32.const 20
        i32.add
        i32.load
        tee_local 2
        i32.const -1
        i32.add
        set_local 1
        get_local 2
        br_if 0 (;@2;)
        get_local 1
        i32.const 0
        call 75
        unreachable
      end
      get_local 0
      i32.eqz
      set_local 2
    end
    get_local 9
    i32.const 9
    get_local 1
    get_local 2
    select
    i32.store offset=52
    get_local 9
    i32.const 1072
    get_local 0
    get_local 2
    select
    i32.store offset=48
    get_local 9
    get_local 9
    i32.const 32
    i32.add
    i32.store offset=60
    get_local 9
    get_local 9
    i32.const 48
    i32.add
    i32.store offset=56
    get_local 9
    get_local 9
    i32.const 16
    i32.add
    i32.store offset=64
    get_local 9
    get_local 9
    i32.const 24
    i32.add
    i32.store offset=68
    get_local 9
    get_local 9
    i32.const 28
    i32.add
    i32.store offset=72
    get_local 9
    get_local 9
    i32.const 15
    i32.add
    i32.store offset=76
    block  ;; label = @1
      call 39
      tee_local 2
      br_if 0 (;@1;)
      call 3
      unreachable
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 2
              i32.load
              tee_local 0
              i32.const 1
              i32.eq
              br_if 0 (;@5;)
              get_local 2
              i64.const 1
              i64.store align=4
              get_local 2
              i32.load offset=8
              set_local 1
              get_local 2
              i32.const 0
              i32.store offset=8
              block  ;; label = @6
                get_local 0
                i32.eqz
                br_if 0 (;@6;)
                get_local 1
                i32.eqz
                br_if 0 (;@6;)
                get_local 1
                get_local 2
                i32.load offset=12
                tee_local 0
                i32.load
                call_indirect (type 1)
                get_local 0
                i32.load offset=4
                i32.eqz
                br_if 0 (;@6;)
                i32.const 1744
                get_local 1
                call 60
              end
              get_local 2
              i32.load
              i32.const 1
              i32.ne
              br_if 1 (;@4;)
            end
            block  ;; label = @5
              get_local 2
              i32.const 4
              i32.add
              tee_local 0
              i32.load
              br_if 0 (;@5;)
              get_local 0
              i32.const -1
              i32.store
              get_local 2
              i64.load offset=8 align=4
              set_local 10
              i32.const 0
              set_local 1
              get_local 2
              i32.const 0
              i32.store offset=8
              get_local 0
              i32.const 0
              i32.store
              get_local 9
              get_local 9
              i32.const 48
              i32.add
              i32.store offset=88
              get_local 9
              get_local 10
              i64.store offset=80
              get_local 10
              i32.wrap/i64
              tee_local 0
              br_if 2 (;@3;)
              get_local 9
              i32.const 56
              i32.add
              get_local 9
              i32.const 88
              i32.add
              i32.const 1096
              call 33
              get_local 6
              i32.eqz
              br_if 4 (;@1;)
              br 3 (;@2;)
            end
            call 5
            unreachable
          end
          i32.const 1484
          call 87
          unreachable
        end
        get_local 9
        i32.const 56
        i32.add
        get_local 0
        get_local 10
        i64.const 32
        i64.shr_u
        i32.wrap/i64
        tee_local 1
        call 33
        block  ;; label = @3
          call 39
          tee_local 2
          br_if 0 (;@3;)
          call 3
          unreachable
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 2
                i32.load
                tee_local 5
                i32.const 1
                i32.eq
                br_if 0 (;@6;)
                get_local 2
                i64.const 1
                i64.store align=4
                get_local 2
                i32.load offset=8
                set_local 4
                get_local 2
                i32.const 0
                i32.store offset=8
                block  ;; label = @7
                  get_local 5
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 4
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 4
                  get_local 2
                  i32.load offset=12
                  tee_local 5
                  i32.load
                  call_indirect (type 1)
                  get_local 5
                  i32.load offset=4
                  i32.eqz
                  br_if 0 (;@7;)
                  i32.const 1744
                  get_local 4
                  call 60
                end
                get_local 2
                i32.load
                i32.const 1
                i32.ne
                br_if 1 (;@5;)
              end
              block  ;; label = @6
                get_local 2
                i32.const 4
                i32.add
                tee_local 5
                i32.load
                br_if 0 (;@6;)
                get_local 5
                i32.const -1
                i32.store
                get_local 2
                i32.load offset=8
                tee_local 5
                br_if 2 (;@4;)
                get_local 2
                i32.const 12
                i32.add
                set_local 5
                br 3 (;@3;)
              end
              call 5
              unreachable
            end
            i32.const 1484
            call 87
            unreachable
          end
          get_local 5
          get_local 2
          i32.load offset=12
          i32.load
          call_indirect (type 1)
          get_local 2
          i32.const 12
          i32.add
          set_local 5
          get_local 2
          i32.load offset=12
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 2
          i32.const 8
          i32.add
          i32.load
          call 60
        end
        get_local 5
        get_local 1
        i32.store
        get_local 2
        i32.const 8
        i32.add
        get_local 0
        i32.store
        get_local 2
        i32.const 4
        i32.add
        i32.const 0
        i32.store
        i32.const 1
        set_local 1
        get_local 6
        i32.eqz
        br_if 1 (;@1;)
      end
      get_local 6
      get_local 6
      i32.load
      tee_local 2
      i32.const -1
      i32.add
      i32.store
      get_local 2
      i32.const 1
      i32.ne
      br_if 0 (;@1;)
      get_local 6
      call 18
    end
    block  ;; label = @1
      get_local 9
      i32.load offset=80
      tee_local 2
      i32.eqz
      get_local 1
      i32.or
      br_if 0 (;@1;)
      get_local 2
      get_local 9
      i32.load offset=84
      i32.load
      call_indirect (type 1)
      get_local 9
      i32.load offset=84
      i32.load offset=4
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1744
      get_local 9
      i32.load offset=80
      call 60
    end
    i32.const 0
    i32.const 0
    i32.load offset=956
    i32.const -1
    i32.add
    i32.store offset=956
    block  ;; label = @1
      get_local 7
      i32.const 2
      i32.lt_u
      br_if 0 (;@1;)
      get_local 9
      i32.const 76
      i32.add
      i32.const 0
      i32.store
      get_local 9
      i32.const 1
      i32.store offset=60
      get_local 9
      i32.const 1004
      i32.store offset=56
      get_local 9
      i32.const 0
      i32.store offset=64
      get_local 9
      i32.const 4852
      i32.store offset=72
      get_local 9
      i32.const 56
      i32.add
      call 28
    end
    unreachable
    unreachable)
  (func (;33;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 832
    i32.sub
    tee_local 7
    i32.store offset=4
    get_local 7
    get_local 0
    i32.load
    i32.store offset=32
    get_local 0
    i32.load offset=16
    set_local 3
    get_local 0
    i32.load offset=12
    set_local 4
    get_local 0
    i32.load offset=8
    set_local 5
    get_local 0
    i32.load offset=4
    set_local 6
    get_local 7
    i32.const 2
    i32.store offset=36
    get_local 7
    get_local 6
    i32.store offset=40
    get_local 7
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 7
    get_local 5
    i32.store offset=48
    get_local 7
    i32.const 32
    i32.add
    i32.const 20
    i32.add
    i32.const 2
    i32.store
    get_local 7
    get_local 4
    i32.store offset=56
    get_local 7
    i32.const 60
    i32.add
    i32.const 5
    i32.store
    get_local 7
    get_local 3
    i32.store offset=64
    get_local 7
    i32.const 68
    i32.add
    i32.const 5
    i32.store
    get_local 7
    i32.const 1128
    i32.store offset=8
    get_local 7
    i32.const 6
    i32.store offset=12
    get_local 7
    i32.const 5348
    i32.store offset=16
    get_local 7
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 5
    i32.store
    get_local 7
    get_local 7
    i32.const 32
    i32.add
    i32.store offset=24
    get_local 7
    i32.const 8
    i32.add
    i32.const 20
    i32.add
    i32.const 5
    i32.store
    get_local 7
    get_local 1
    get_local 7
    i32.const 8
    i32.add
    get_local 2
    i32.load offset=24
    tee_local 2
    call_indirect (type 4)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        get_local 7
        i32.load8_u
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      get_local 7
      i32.load offset=4
      tee_local 3
      i32.load
      get_local 3
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        get_local 3
        i32.load offset=4
        i32.load offset=4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1744
        get_local 3
        i32.load
        call 60
      end
      i32.const 1744
      get_local 3
      call 60
    end
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          get_local 0
          i32.load offset=20
          i32.load8_u
          i32.const 4
          i32.ne
          br_if 0 (;@3;)
          i32.const 0
          i32.const 0
          i32.const 0
          i32.load8_u offset=1184
          tee_local 0
          get_local 0
          i32.const 1
          i32.eq
          select
          i32.store8 offset=1184
          get_local 0
          i32.eqz
          br_if 1 (;@2;)
          get_local 7
          i32.const 52
          i32.add
          i32.const 0
          i32.store
          get_local 7
          i32.const 1
          i32.store offset=36
          get_local 7
          i32.const 1188
          i32.store offset=32
          get_local 7
          i32.const 0
          i32.store offset=40
          get_local 7
          i32.const 4852
          i32.store offset=48
          get_local 7
          i32.const 8
          i32.add
          get_local 1
          get_local 7
          i32.const 32
          i32.add
          get_local 2
          call_indirect (type 4)
          block  ;; label = @4
            i32.const 0
            br_if 0 (;@4;)
            get_local 7
            i32.load8_u offset=8
            i32.const 2
            i32.ne
            br_if 2 (;@2;)
          end
          get_local 7
          i32.load offset=12
          tee_local 0
          i32.load
          get_local 0
          i32.load offset=4
          i32.load
          call_indirect (type 1)
          block  ;; label = @4
            get_local 0
            i32.load offset=4
            i32.load offset=4
            i32.eqz
            br_if 0 (;@4;)
            i32.const 1744
            get_local 0
            i32.load
            call 60
          end
          i32.const 1744
          get_local 0
          call 60
          br 1 (;@2;)
        end
        i32.const 0
        i32.load8_u offset=288
        br_if 1 (;@1;)
        i32.const 0
        i32.const 1
        i32.store8 offset=288
        get_local 7
        i32.const 832
        i32.add
        set_local 1
        get_local 7
        i32.const 32
        i32.add
        set_local 0
        loop  ;; label = @3
          get_local 0
          i64.const 0
          i64.store align=4
          get_local 0
          i32.const 8
          i32.add
          tee_local 0
          get_local 1
          i32.ne
          br_if 0 (;@3;)
        end
        get_local 7
        i32.const 8
        i32.add
        i32.const 16
        i32.const 512
        i32.const 35
        call 17
        get_local 7
        i64.load offset=8
        set_local 8
        i32.const 0
        i32.const 0
        i32.store8 offset=288
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          get_local 8
          i32.wrap/i64
          i32.const 3
          i32.and
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        get_local 8
        i64.const 32
        i64.shr_u
        i32.wrap/i64
        tee_local 0
        i32.load
        get_local 0
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          get_local 0
          i32.load offset=4
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 0
          i32.load
          call 60
        end
        i32.const 1744
        get_local 0
        call 60
      end
      i32.const 0
      get_local 7
      i32.const 832
      i32.add
      i32.store offset=4
      return
    end
    i32.const 192
    i32.const 32
    i32.const 224
    call 29
    unreachable)
  (func (;34;) (type 1) (param i32))
  (func (;35;) (type 7) (param i32 i32 i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 4
    i32.store offset=4
    get_local 4
    i32.const 8
    i32.add
    i32.const 16
    i32.const 512
    i32.const 35
    call 17
    get_local 0
    i32.const 1
    i32.store
    get_local 0
    get_local 4
    i64.load offset=8
    i64.store offset=4 align=4
    i32.const 0
    get_local 4
    i32.const 16
    i32.add
    i32.store offset=4)
  (func (;36;) (type 6) (param i32 i32)
    get_local 0
    i32.const 3
    i32.store8)
  (func (;37;) (type 7) (param i32 i32 i32 i32)
    get_local 0
    get_local 2
    get_local 3
    call 46)
  (func (;38;) (type 4) (param i32 i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 1
    i32.load
    set_local 1
    get_local 3
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    get_local 2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    get_local 2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    get_local 2
    i64.load align=4
    i64.store offset=8
    get_local 0
    get_local 1
    get_local 3
    i32.const 8
    i32.add
    call 47
    i32.const 0
    get_local 3
    i32.const 32
    i32.add
    i32.store offset=4)
  (func (;39;) (type 8) (result i32)
    (local i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 4
    i32.store offset=4
    i32.const 0
    set_local 3
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=1332
        tee_local 2
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=1336
        set_local 0
        i32.const 1744
        i32.const 8
        call 57
        tee_local 2
        i32.eqz
        br_if 1 (;@1;)
        get_local 2
        i32.const 0
        i32.store
        get_local 2
        get_local 0
        i32.store offset=4
        i32.const 0
        i32.const 0
        i32.load offset=1332
        tee_local 0
        get_local 2
        get_local 0
        select
        i32.store offset=1332
        get_local 0
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 2
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 2
        call 60
        get_local 0
        set_local 2
      end
      block  ;; label = @2
        get_local 2
        i32.load
        tee_local 2
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          br_if 0 (;@3;)
          i32.const 1744
          i32.const 20
          call 57
          tee_local 2
          i32.eqz
          br_if 2 (;@1;)
          get_local 4
          i32.const 36
          i32.add
          i32.const 8
          i32.add
          get_local 4
          i32.const 24
          i32.add
          i32.const 8
          i32.add
          i32.load
          tee_local 3
          i32.store
          get_local 4
          i32.const 36
          i32.add
          i32.const 4
          i32.add
          get_local 4
          i32.const 24
          i32.add
          i32.const 4
          i32.add
          i32.load
          tee_local 0
          i32.store
          get_local 4
          i32.const 12
          i32.add
          i32.const 8
          i32.add
          tee_local 1
          get_local 3
          i32.store
          get_local 4
          i32.const 12
          i32.add
          i32.const 4
          i32.add
          tee_local 3
          get_local 0
          i32.store
          get_local 4
          get_local 4
          i32.load offset=24
          tee_local 0
          i32.store offset=36
          get_local 4
          get_local 0
          i32.store offset=12
          get_local 2
          i32.const 0
          i32.store offset=4
          get_local 2
          i32.const 1332
          i32.store
          get_local 2
          i32.const 16
          i32.add
          get_local 1
          i32.load
          i32.store
          get_local 2
          i32.const 12
          i32.add
          get_local 3
          i32.load
          i32.store
          get_local 2
          get_local 4
          i32.load offset=12
          i32.store offset=8
          block  ;; label = @4
            i32.const 0
            i32.load offset=1332
            tee_local 3
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1336
            set_local 0
            i32.const 1744
            i32.const 8
            call 57
            tee_local 3
            i32.eqz
            br_if 3 (;@1;)
            get_local 3
            i32.const 0
            i32.store
            get_local 3
            get_local 0
            i32.store offset=4
            i32.const 0
            i32.const 0
            i32.load offset=1332
            tee_local 0
            get_local 3
            get_local 0
            select
            i32.store offset=1332
            get_local 0
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              get_local 3
              i32.load offset=4
              tee_local 1
              i32.eqz
              br_if 0 (;@5;)
              get_local 3
              i32.load
              get_local 1
              call_indirect (type 1)
            end
            i32.const 1744
            get_local 3
            call 60
            get_local 0
            set_local 3
          end
          get_local 3
          get_local 2
          i32.store
          get_local 2
          i32.const 4
          i32.add
          set_local 3
          br 1 (;@2;)
        end
        get_local 2
        i32.const 4
        i32.add
        set_local 3
      end
      i32.const 0
      get_local 4
      i32.const 48
      i32.add
      i32.store offset=4
      get_local 3
      return
    end
    unreachable
    unreachable)
  (func (;40;) (type 8) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    set_local 3
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        i32.load offset=1444
        tee_local 2
        br_if 0 (;@2;)
        i32.const 0
        i32.load offset=1448
        set_local 0
        i32.const 1744
        i32.const 8
        call 57
        tee_local 2
        i32.eqz
        br_if 1 (;@1;)
        get_local 2
        i32.const 0
        i32.store
        get_local 2
        get_local 0
        i32.store offset=4
        i32.const 0
        i32.const 0
        i32.load offset=1444
        tee_local 0
        get_local 2
        get_local 0
        select
        i32.store offset=1444
        get_local 0
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          i32.load offset=4
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          get_local 2
          i32.load
          get_local 1
          call_indirect (type 1)
        end
        i32.const 1744
        get_local 2
        call 60
        get_local 0
        set_local 2
      end
      block  ;; label = @2
        get_local 2
        i32.load
        tee_local 2
        i32.const 1
        i32.eq
        br_if 0 (;@2;)
        block  ;; label = @3
          get_local 2
          br_if 0 (;@3;)
          i32.const 1744
          i32.const 12
          call 57
          tee_local 2
          i32.eqz
          br_if 2 (;@1;)
          get_local 2
          i32.const 1444
          i32.store
          get_local 2
          i64.const 0
          i64.store offset=4 align=4
          block  ;; label = @4
            i32.const 0
            i32.load offset=1444
            tee_local 3
            br_if 0 (;@4;)
            i32.const 0
            i32.load offset=1448
            set_local 0
            i32.const 1744
            i32.const 8
            call 57
            tee_local 3
            i32.eqz
            br_if 3 (;@1;)
            get_local 3
            i32.const 0
            i32.store
            get_local 3
            get_local 0
            i32.store offset=4
            i32.const 0
            i32.const 0
            i32.load offset=1444
            tee_local 0
            get_local 3
            get_local 0
            select
            i32.store offset=1444
            get_local 0
            i32.eqz
            br_if 0 (;@4;)
            block  ;; label = @5
              get_local 3
              i32.load offset=4
              tee_local 1
              i32.eqz
              br_if 0 (;@5;)
              get_local 3
              i32.load
              get_local 1
              call_indirect (type 1)
            end
            i32.const 1744
            get_local 3
            call 60
            get_local 0
            set_local 3
          end
          get_local 3
          get_local 2
          i32.store
          get_local 2
          i32.const 4
          i32.add
          return
        end
        get_local 2
        i32.const 4
        i32.add
        set_local 3
      end
      get_local 3
      return
    end
    unreachable
    unreachable)
  (func (;41;) (type 1) (param i32)
    block  ;; label = @1
      get_local 0
      i32.load offset=4
      i32.eqz
      br_if 0 (;@1;)
      i32.const 1744
      get_local 0
      i32.load
      call 60
    end)
  (func (;42;) (type 1) (param i32))
  (func (;43;) (type 3) (param i32) (result i64)
    i64.const 1229646359891580772)
  (func (;44;) (type 2) (param i32 i32) (result i32)
    get_local 1
    i32.load offset=24
    i32.const 1472
    i32.const 11
    get_local 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 0))
  (func (;45;) (type 8) (result i32)
    (local i32 i32 i32 i32 i32 i64)
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
                          call 24
                          tee_local 0
                          i32.eqz
                          br_if 0 (;@11;)
                          get_local 0
                          i32.load offset=4
                          tee_local 2
                          i32.const 3
                          i32.ne
                          br_if 1 (;@10;)
                          get_local 0
                          i64.const 8589934592
                          i64.store align=4
                          get_local 0
                          i32.const 0
                          i32.store
                          br 2 (;@9;)
                        end
                        i32.const 0
                        return
                      end
                      get_local 0
                      i32.load
                      tee_local 1
                      i32.const -1
                      i32.eq
                      br_if 8 (;@1;)
                      get_local 0
                      get_local 1
                      i32.store
                      get_local 2
                      i32.const 2
                      i32.ne
                      br_if 1 (;@8;)
                    end
                    i32.const 0
                    i32.load8_u offset=608
                    br_if 5 (;@3;)
                    i32.const 0
                    i32.const 1
                    i32.store8 offset=608
                    i32.const 0
                    i64.load offset=672
                    tee_local 5
                    i64.const -1
                    i64.eq
                    br_if 6 (;@2;)
                    i32.const 0
                    get_local 5
                    i64.const 1
                    i64.add
                    i64.store offset=672
                    i32.const 0
                    i32.const 0
                    i32.store8 offset=608
                    i32.const 1744
                    i32.const 1
                    call 57
                    tee_local 1
                    i32.eqz
                    br_if 2 (;@6;)
                    get_local 1
                    i32.const 0
                    i32.store8
                    i32.const 1744
                    i32.const 48
                    call 57
                    tee_local 2
                    i32.eqz
                    br_if 2 (;@6;)
                    get_local 2
                    get_local 5
                    i64.store offset=8
                    get_local 2
                    i64.const 4294967297
                    i64.store align=4
                    get_local 2
                    i64.const 0
                    i64.store offset=16
                    get_local 2
                    i32.const 0
                    i32.store offset=24
                    get_local 2
                    get_local 1
                    i32.store offset=28
                    get_local 2
                    i32.const 0
                    i32.store8 offset=32
                    get_local 2
                    i32.const 1
                    i32.store offset=36
                    get_local 2
                    i32.const 0
                    i32.store offset=40
                    get_local 0
                    i32.load
                    br_if 3 (;@5;)
                    get_local 0
                    i32.const -1
                    i32.store
                    block  ;; label = @9
                      get_local 0
                      i32.const 4
                      i32.add
                      tee_local 1
                      i32.load
                      i32.const 2
                      i32.eq
                      br_if 0 (;@9;)
                      get_local 0
                      i32.const 12
                      i32.add
                      tee_local 3
                      i32.load
                      tee_local 4
                      get_local 4
                      i32.load
                      tee_local 4
                      i32.const -1
                      i32.add
                      i32.store
                      get_local 4
                      i32.const 1
                      i32.ne
                      br_if 0 (;@9;)
                      get_local 3
                      i32.load
                      call 18
                    end
                    get_local 0
                    i32.const 0
                    i32.store
                    get_local 0
                    i32.const 12
                    i32.add
                    get_local 2
                    i32.store
                    get_local 1
                    i64.const 0
                    i64.store align=4
                    br 1 (;@7;)
                  end
                  get_local 1
                  br_if 2 (;@5;)
                end
                get_local 0
                i32.const -1
                i32.store
                get_local 0
                i32.load offset=12
                tee_local 2
                get_local 2
                i32.load
                tee_local 2
                i32.const 1
                i32.add
                i32.store
                get_local 2
                i32.const -1
                i32.le_s
                br_if 2 (;@4;)
                get_local 0
                i32.const 0
                i32.store
                get_local 0
                i32.const 12
                i32.add
                i32.load
                return
              end
              unreachable
              unreachable
            end
            call 5
            unreachable
          end
          unreachable
          unreachable
        end
        i32.const 624
        i32.const 32
        i32.const 656
        call 29
        unreachable
      end
      i32.const 0
      i32.const 0
      i32.store8 offset=608
      i32.const 688
      i32.const 55
      i32.const 744
      call 29
      unreachable
    end
    call 4
    unreachable)
  (func (;46;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 5
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        get_local 2
        i32.eqz
        br_if 0 (;@2;)
        block  ;; label = @3
          loop  ;; label = @4
            get_local 5
            i32.const 8
            i32.add
            i32.const 16
            i32.const 512
            i32.const 35
            call 17
            get_local 5
            i32.load offset=12
            set_local 2
            block  ;; label = @5
              block  ;; label = @6
                get_local 5
                i32.load offset=8
                tee_local 3
                i32.const 3
                i32.and
                tee_local 4
                i32.const 1
                i32.eq
                br_if 0 (;@6;)
                get_local 4
                i32.const 2
                i32.ne
                br_if 3 (;@3;)
                get_local 2
                i32.load8_u offset=8
                set_local 4
                br 1 (;@5;)
              end
              get_local 3
              i32.const 8
              i32.shr_u
              set_local 4
            end
            get_local 4
            i32.const 255
            i32.and
            i32.const 15
            i32.ne
            br_if 1 (;@3;)
            get_local 3
            i32.const 255
            i32.and
            i32.const 2
            i32.lt_u
            br_if 0 (;@4;)
            get_local 2
            i32.load
            get_local 2
            i32.load offset=4
            i32.load
            call_indirect (type 1)
            block  ;; label = @5
              get_local 2
              i32.load offset=4
              i32.load offset=4
              i32.eqz
              br_if 0 (;@5;)
              i32.const 1744
              get_local 2
              i32.load
              call 60
            end
            i32.const 1744
            get_local 2
            call 60
            br 0 (;@4;)
          end
          unreachable
        end
        get_local 0
        get_local 3
        i32.store
        get_local 0
        i32.const 4
        i32.add
        get_local 2
        i32.store
        br 1 (;@1;)
      end
      get_local 0
      i32.const 3
      i32.store8
    end
    i32.const 0
    get_local 5
    i32.const 16
    i32.add
    i32.store offset=4)
  (func (;47;) (type 4) (param i32 i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    i32.const 19
    i32.add
    get_local 3
    i32.const 30
    i32.add
    i32.load8_u
    i32.store8
    get_local 3
    i32.const 17
    i32.add
    get_local 3
    i32.const 28
    i32.add
    i32.load16_u align=1
    i32.store16 align=1
    get_local 3
    get_local 1
    i32.store offset=8
    get_local 3
    i32.const 3
    i32.store8 offset=12
    get_local 3
    get_local 3
    i32.load offset=24 align=1
    i32.store offset=13 align=1
    get_local 3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    get_local 2
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    get_local 2
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    get_local 2
    i64.load align=4
    i64.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 3
              i32.const 8
              i32.add
              i32.const 1704
              get_local 3
              i32.const 24
              i32.add
              call 69
              i32.eqz
              br_if 0 (;@5;)
              get_local 3
              i32.load8_u offset=12
              i32.const 3
              i32.ne
              br_if 3 (;@2;)
              get_local 3
              i32.const 24
              i32.add
              i32.const 16
              i32.const 1728
              i32.const 15
              call 17
              get_local 0
              get_local 3
              i64.load offset=24
              i64.store align=4
              i32.const 0
              i32.eqz
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            get_local 0
            i32.const 3
            i32.store8
            i32.const 0
            br_if 1 (;@3;)
          end
          get_local 3
          i32.load8_u offset=12
          i32.const 2
          i32.ne
          br_if 2 (;@1;)
        end
        get_local 3
        i32.const 16
        i32.add
        tee_local 0
        i32.load
        tee_local 2
        i32.load
        get_local 2
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          get_local 2
          i32.load offset=4
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 2
          i32.load
          call 60
        end
        i32.const 1744
        get_local 0
        i32.load
        call 60
        br 1 (;@1;)
      end
      get_local 0
      get_local 3
      i64.load offset=12 align=4
      i64.store align=4
    end
    i32.const 0
    get_local 3
    i32.const 48
    i32.add
    i32.store offset=4)
  (func (;48;) (type 1) (param i32)
    (local i32)
    block  ;; label = @1
      block  ;; label = @2
        i32.const 0
        br_if 0 (;@2;)
        get_local 0
        i32.load8_u offset=4
        i32.const 2
        i32.ne
        br_if 1 (;@1;)
      end
      get_local 0
      i32.const 8
      i32.add
      tee_local 1
      i32.load
      tee_local 0
      i32.load
      get_local 0
      i32.load offset=4
      i32.load
      call_indirect (type 1)
      block  ;; label = @2
        get_local 0
        i32.load offset=4
        i32.load offset=4
        i32.eqz
        br_if 0 (;@2;)
        i32.const 1744
        get_local 0
        i32.load
        call 60
      end
      i32.const 1744
      get_local 1
      i32.load
      call 60
    end)
  (func (;49;) (type 2) (param i32 i32) (result i32)
    get_local 1
    get_local 0
    i32.load
    get_local 0
    i32.load offset=4
    call 71)
  (func (;50;) (type 2) (param i32 i32) (result i32)
    get_local 0
    i32.load
    get_local 1
    call 12)
  (func (;51;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 8
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            get_local 1
            i32.const 128
            i32.ge_u
            br_if 0 (;@4;)
            get_local 0
            i32.load offset=8
            tee_local 4
            get_local 0
            i32.load offset=4
            i32.eq
            br_if 1 (;@3;)
            br 2 (;@2;)
          end
          i32.const 0
          set_local 4
          get_local 8
          i32.const 0
          i32.store offset=12
          block  ;; label = @4
            block  ;; label = @5
              get_local 1
              i32.const 2048
              i32.ge_u
              br_if 0 (;@5;)
              i32.const 2
              set_local 7
              i32.const 1
              set_local 6
              i32.const 192
              set_local 5
              i32.const 31
              set_local 3
              br 1 (;@4;)
            end
            block  ;; label = @5
              block  ;; label = @6
                get_local 1
                i32.const 65536
                i32.ge_u
                br_if 0 (;@6;)
                i32.const 3
                set_local 7
                i32.const 2
                set_local 6
                i32.const 1
                set_local 4
                i32.const 224
                set_local 5
                i32.const 0
                set_local 3
                i32.const 15
                set_local 2
                br 1 (;@5;)
              end
              get_local 8
              get_local 1
              i32.const 18
              i32.shr_u
              i32.const 240
              i32.or
              i32.store8 offset=12
              i32.const 4
              set_local 7
              i32.const 3
              set_local 6
              i32.const 2
              set_local 4
              i32.const 128
              set_local 5
              i32.const 1
              set_local 3
              i32.const 63
              set_local 2
            end
            get_local 8
            i32.const 12
            i32.add
            get_local 3
            i32.or
            get_local 2
            get_local 1
            i32.const 12
            i32.shr_u
            i32.and
            get_local 5
            i32.or
            i32.store8
            i32.const 128
            set_local 5
            i32.const 63
            set_local 3
          end
          get_local 8
          i32.const 12
          i32.add
          get_local 4
          i32.add
          get_local 3
          get_local 1
          i32.const 6
          i32.shr_u
          i32.and
          get_local 5
          i32.or
          i32.store8
          get_local 8
          i32.const 12
          i32.add
          get_local 6
          i32.add
          get_local 1
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8
          get_local 0
          get_local 8
          i32.const 12
          i32.add
          get_local 7
          call 2
          br 2 (;@1;)
        end
        get_local 0
        call 20
        get_local 0
        i32.const 8
        i32.add
        i32.load
        set_local 4
      end
      get_local 0
      i32.load
      get_local 4
      i32.add
      get_local 1
      i32.store8
      get_local 0
      i32.const 8
      i32.add
      tee_local 1
      get_local 1
      i32.load
      i32.const 1
      i32.add
      i32.store
    end
    i32.const 0
    get_local 8
    i32.const 16
    i32.add
    i32.store offset=4
    i32.const 0)
  (func (;52;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 4
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    get_local 4
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    tee_local 2
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    tee_local 3
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 4
    get_local 0
    i32.store offset=36
    get_local 4
    i32.const 40
    i32.add
    i32.const 16
    i32.add
    get_local 2
    i64.load
    i64.store
    get_local 4
    i32.const 40
    i32.add
    i32.const 8
    i32.add
    get_local 3
    i64.load
    i64.store
    get_local 4
    get_local 4
    i64.load offset=8
    i64.store offset=40
    get_local 4
    i32.const 36
    i32.add
    i32.const 444
    get_local 4
    i32.const 40
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 4
    i32.const 64
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;53;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 4
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    get_local 4
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    tee_local 2
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    tee_local 3
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 4
    get_local 0
    i32.store offset=36
    get_local 4
    i32.const 40
    i32.add
    i32.const 16
    i32.add
    get_local 2
    i64.load
    i64.store
    get_local 4
    i32.const 40
    i32.add
    i32.const 8
    i32.add
    get_local 3
    i64.load
    i64.store
    get_local 4
    get_local 4
    i64.load offset=8
    i64.store offset=40
    get_local 4
    i32.const 36
    i32.add
    i32.const 900
    get_local 4
    i32.const 40
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 4
    i32.const 64
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;54;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    get_local 3
    i32.const 8
    i32.add
    get_local 1
    get_local 2
    call 46
    i32.const 0
    set_local 1
    block  ;; label = @1
      get_local 3
      i32.load8_u offset=8
      i32.const 3
      i32.eq
      br_if 0 (;@1;)
      get_local 3
      i64.load offset=8
      set_local 4
      block  ;; label = @2
        block  ;; label = @3
          i32.const 0
          br_if 0 (;@3;)
          get_local 0
          i32.load8_u offset=4
          i32.const 2
          i32.ne
          br_if 1 (;@2;)
        end
        get_local 0
        i32.const 8
        i32.add
        i32.load
        tee_local 1
        i32.load
        get_local 1
        i32.load offset=4
        i32.load
        call_indirect (type 1)
        block  ;; label = @3
          get_local 1
          i32.load offset=4
          i32.load offset=4
          i32.eqz
          br_if 0 (;@3;)
          i32.const 1744
          get_local 1
          i32.load
          call 60
        end
        i32.const 1744
        get_local 1
        call 60
      end
      get_local 0
      i32.const 4
      i32.add
      get_local 4
      i64.store align=4
      i32.const 1
      set_local 1
    end
    i32.const 0
    get_local 3
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;55;) (type 0) (param i32 i32 i32) (result i32)
    get_local 0
    i32.load
    get_local 1
    get_local 2
    call 2
    i32.const 0)
  (func (;56;) (type 0) (param i32 i32 i32) (result i32)
    (local i32)
    block  ;; label = @1
      get_local 2
      i32.eqz
      br_if 0 (;@1;)
      get_local 0
      set_local 3
      loop  ;; label = @2
        get_local 3
        get_local 1
        i32.load8_u
        i32.store8
        get_local 1
        i32.const 1
        i32.add
        set_local 1
        get_local 3
        i32.const 1
        i32.add
        set_local 3
        get_local 2
        i32.const -1
        i32.add
        tee_local 2
        br_if 0 (;@2;)
      end
    end
    get_local 0)
  (func (;57;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64 i64)
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    set_local 10
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
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    block  ;; label = @25
                                                      block  ;; label = @26
                                                        block  ;; label = @27
                                                          block  ;; label = @28
                                                            block  ;; label = @29
                                                              block  ;; label = @30
                                                                block  ;; label = @31
                                                                  block  ;; label = @32
                                                                    block  ;; label = @33
                                                                      block  ;; label = @34
                                                                        block  ;; label = @35
                                                                          get_local 1
                                                                          i32.const 244
                                                                          i32.gt_u
                                                                          br_if 0 (;@35;)
                                                                          get_local 0
                                                                          i32.load
                                                                          tee_local 8
                                                                          i32.const 16
                                                                          get_local 1
                                                                          i32.const 11
                                                                          i32.add
                                                                          i32.const -8
                                                                          i32.and
                                                                          get_local 1
                                                                          i32.const 11
                                                                          i32.lt_u
                                                                          select
                                                                          tee_local 9
                                                                          i32.const 3
                                                                          i32.shr_u
                                                                          tee_local 3
                                                                          i32.const 31
                                                                          i32.and
                                                                          tee_local 4
                                                                          i32.shr_u
                                                                          tee_local 1
                                                                          i32.const 3
                                                                          i32.and
                                                                          i32.eqz
                                                                          br_if 1 (;@34;)
                                                                          get_local 0
                                                                          get_local 1
                                                                          i32.const -1
                                                                          i32.xor
                                                                          i32.const 1
                                                                          i32.and
                                                                          get_local 3
                                                                          i32.add
                                                                          tee_local 3
                                                                          i32.const 3
                                                                          i32.shl
                                                                          i32.add
                                                                          tee_local 9
                                                                          i32.const 16
                                                                          i32.add
                                                                          i32.load
                                                                          tee_local 1
                                                                          i32.load offset=8
                                                                          tee_local 4
                                                                          get_local 9
                                                                          i32.const 8
                                                                          i32.add
                                                                          tee_local 9
                                                                          i32.eq
                                                                          br_if 2 (;@33;)
                                                                          get_local 4
                                                                          get_local 9
                                                                          i32.store offset=12
                                                                          get_local 9
                                                                          i32.const 8
                                                                          i32.add
                                                                          get_local 4
                                                                          i32.store
                                                                          br 3 (;@32;)
                                                                        end
                                                                        i32.const 0
                                                                        set_local 6
                                                                        get_local 1
                                                                        i32.const -64
                                                                        i32.ge_u
                                                                        br_if 26 (;@8;)
                                                                        get_local 1
                                                                        i32.const 11
                                                                        i32.add
                                                                        tee_local 1
                                                                        i32.const -8
                                                                        i32.and
                                                                        set_local 9
                                                                        get_local 0
                                                                        i32.load offset=4
                                                                        tee_local 2
                                                                        i32.eqz
                                                                        br_if 9 (;@25;)
                                                                        i32.const 0
                                                                        set_local 7
                                                                        block  ;; label = @35
                                                                          get_local 1
                                                                          i32.const 8
                                                                          i32.shr_u
                                                                          tee_local 1
                                                                          i32.eqz
                                                                          br_if 0 (;@35;)
                                                                          i32.const 31
                                                                          set_local 7
                                                                          get_local 9
                                                                          i32.const 16777215
                                                                          i32.gt_u
                                                                          br_if 0 (;@35;)
                                                                          get_local 9
                                                                          i32.const 38
                                                                          get_local 1
                                                                          i32.clz
                                                                          tee_local 1
                                                                          i32.sub
                                                                          i32.const 31
                                                                          i32.and
                                                                          i32.shr_u
                                                                          i32.const 1
                                                                          i32.and
                                                                          i32.const 31
                                                                          get_local 1
                                                                          i32.sub
                                                                          i32.const 1
                                                                          i32.shl
                                                                          i32.or
                                                                          set_local 7
                                                                        end
                                                                        i32.const 0
                                                                        get_local 9
                                                                        i32.sub
                                                                        set_local 4
                                                                        get_local 0
                                                                        get_local 7
                                                                        i32.const 2
                                                                        i32.shl
                                                                        i32.add
                                                                        i32.const 272
                                                                        i32.add
                                                                        i32.load
                                                                        tee_local 1
                                                                        i32.eqz
                                                                        br_if 6 (;@28;)
                                                                        i32.const 0
                                                                        set_local 3
                                                                        get_local 9
                                                                        i32.const 0
                                                                        i32.const 25
                                                                        get_local 7
                                                                        i32.const 1
                                                                        i32.shr_u
                                                                        i32.sub
                                                                        i32.const 31
                                                                        i32.and
                                                                        get_local 7
                                                                        i32.const 31
                                                                        i32.eq
                                                                        select
                                                                        i32.shl
                                                                        set_local 6
                                                                        i32.const 0
                                                                        set_local 8
                                                                        loop  ;; label = @35
                                                                          block  ;; label = @36
                                                                            get_local 1
                                                                            i32.load offset=4
                                                                            i32.const -8
                                                                            i32.and
                                                                            tee_local 5
                                                                            get_local 9
                                                                            i32.lt_u
                                                                            br_if 0 (;@36;)
                                                                            get_local 5
                                                                            get_local 9
                                                                            i32.sub
                                                                            tee_local 5
                                                                            get_local 4
                                                                            i32.ge_u
                                                                            br_if 0 (;@36;)
                                                                            get_local 5
                                                                            set_local 4
                                                                            get_local 1
                                                                            set_local 8
                                                                            get_local 5
                                                                            i32.eqz
                                                                            br_if 6 (;@30;)
                                                                          end
                                                                          get_local 1
                                                                          i32.const 20
                                                                          i32.add
                                                                          i32.load
                                                                          tee_local 5
                                                                          get_local 3
                                                                          get_local 5
                                                                          get_local 1
                                                                          get_local 6
                                                                          i32.const 29
                                                                          i32.shr_u
                                                                          i32.const 4
                                                                          i32.and
                                                                          i32.add
                                                                          i32.const 16
                                                                          i32.add
                                                                          i32.load
                                                                          tee_local 1
                                                                          i32.ne
                                                                          select
                                                                          get_local 3
                                                                          get_local 5
                                                                          select
                                                                          set_local 3
                                                                          get_local 6
                                                                          i32.const 1
                                                                          i32.shl
                                                                          set_local 6
                                                                          get_local 1
                                                                          br_if 0 (;@35;)
                                                                        end
                                                                        get_local 3
                                                                        i32.eqz
                                                                        br_if 5 (;@29;)
                                                                        get_local 3
                                                                        set_local 1
                                                                        br 7 (;@27;)
                                                                      end
                                                                      get_local 9
                                                                      get_local 0
                                                                      i32.load offset=400
                                                                      i32.le_u
                                                                      br_if 8 (;@25;)
                                                                      get_local 1
                                                                      i32.eqz
                                                                      br_if 2 (;@31;)
                                                                      get_local 0
                                                                      get_local 1
                                                                      get_local 4
                                                                      i32.shl
                                                                      i32.const 2
                                                                      get_local 4
                                                                      i32.shl
                                                                      tee_local 1
                                                                      i32.const 0
                                                                      get_local 1
                                                                      i32.sub
                                                                      i32.or
                                                                      i32.and
                                                                      tee_local 1
                                                                      i32.const 0
                                                                      get_local 1
                                                                      i32.sub
                                                                      i32.and
                                                                      i32.ctz
                                                                      tee_local 3
                                                                      i32.const 3
                                                                      i32.shl
                                                                      i32.add
                                                                      tee_local 6
                                                                      i32.const 16
                                                                      i32.add
                                                                      i32.load
                                                                      tee_local 1
                                                                      i32.load offset=8
                                                                      tee_local 4
                                                                      get_local 6
                                                                      i32.const 8
                                                                      i32.add
                                                                      tee_local 6
                                                                      i32.eq
                                                                      br_if 9 (;@24;)
                                                                      get_local 4
                                                                      get_local 6
                                                                      i32.store offset=12
                                                                      get_local 6
                                                                      i32.const 8
                                                                      i32.add
                                                                      get_local 4
                                                                      i32.store
                                                                      br 10 (;@23;)
                                                                    end
                                                                    get_local 0
                                                                    get_local 8
                                                                    i32.const -2
                                                                    get_local 3
                                                                    i32.rotl
                                                                    i32.and
                                                                    i32.store
                                                                  end
                                                                  get_local 1
                                                                  get_local 3
                                                                  i32.const 3
                                                                  i32.shl
                                                                  tee_local 3
                                                                  i32.const 3
                                                                  i32.or
                                                                  i32.store offset=4
                                                                  get_local 1
                                                                  get_local 3
                                                                  i32.add
                                                                  tee_local 3
                                                                  get_local 3
                                                                  i32.load offset=4
                                                                  i32.const 1
                                                                  i32.or
                                                                  i32.store offset=4
                                                                  get_local 1
                                                                  i32.const 8
                                                                  i32.add
                                                                  return
                                                                end
                                                                get_local 0
                                                                i32.load offset=4
                                                                tee_local 1
                                                                i32.eqz
                                                                br_if 5 (;@25;)
                                                                get_local 0
                                                                get_local 1
                                                                i32.const 0
                                                                get_local 1
                                                                i32.sub
                                                                i32.and
                                                                i32.ctz
                                                                i32.const 2
                                                                i32.shl
                                                                i32.add
                                                                i32.const 272
                                                                i32.add
                                                                i32.load
                                                                tee_local 8
                                                                i32.load offset=4
                                                                i32.const -8
                                                                i32.and
                                                                get_local 9
                                                                i32.sub
                                                                set_local 3
                                                                get_local 8
                                                                set_local 4
                                                                get_local 8
                                                                i32.load offset=16
                                                                tee_local 1
                                                                i32.eqz
                                                                br_if 18 (;@12;)
                                                                i32.const 0
                                                                set_local 11
                                                                br 19 (;@11;)
                                                              end
                                                              i32.const 0
                                                              set_local 4
                                                              get_local 1
                                                              set_local 8
                                                              br 2 (;@27;)
                                                            end
                                                            get_local 8
                                                            br_if 2 (;@26;)
                                                          end
                                                          i32.const 0
                                                          set_local 8
                                                          get_local 2
                                                          i32.const 2
                                                          get_local 7
                                                          i32.const 31
                                                          i32.and
                                                          i32.shl
                                                          tee_local 1
                                                          i32.const 0
                                                          get_local 1
                                                          i32.sub
                                                          i32.or
                                                          i32.and
                                                          tee_local 1
                                                          i32.eqz
                                                          br_if 2 (;@25;)
                                                          get_local 0
                                                          get_local 1
                                                          i32.const 0
                                                          get_local 1
                                                          i32.sub
                                                          i32.and
                                                          i32.ctz
                                                          i32.const 2
                                                          i32.shl
                                                          i32.add
                                                          i32.const 272
                                                          i32.add
                                                          i32.load
                                                          tee_local 1
                                                          i32.eqz
                                                          br_if 2 (;@25;)
                                                        end
                                                        loop  ;; label = @27
                                                          get_local 1
                                                          tee_local 3
                                                          get_local 8
                                                          get_local 3
                                                          i32.load offset=4
                                                          i32.const -8
                                                          i32.and
                                                          tee_local 1
                                                          get_local 9
                                                          i32.ge_u
                                                          get_local 1
                                                          get_local 9
                                                          i32.sub
                                                          tee_local 1
                                                          get_local 4
                                                          i32.lt_u
                                                          i32.and
                                                          tee_local 6
                                                          select
                                                          set_local 8
                                                          get_local 1
                                                          get_local 4
                                                          get_local 6
                                                          select
                                                          set_local 4
                                                          get_local 3
                                                          i32.load offset=16
                                                          tee_local 1
                                                          br_if 0 (;@27;)
                                                          get_local 3
                                                          i32.const 20
                                                          i32.add
                                                          i32.load
                                                          tee_local 1
                                                          br_if 0 (;@27;)
                                                        end
                                                        get_local 8
                                                        i32.eqz
                                                        br_if 1 (;@25;)
                                                      end
                                                      get_local 4
                                                      get_local 9
                                                      i32.add
                                                      tee_local 7
                                                      get_local 0
                                                      i32.load offset=400
                                                      i32.ge_u
                                                      br_if 0 (;@25;)
                                                      get_local 8
                                                      i32.load offset=24
                                                      set_local 5
                                                      get_local 8
                                                      i32.load offset=12
                                                      tee_local 1
                                                      get_local 8
                                                      i32.eq
                                                      br_if 5 (;@20;)
                                                      get_local 8
                                                      i32.load offset=8
                                                      tee_local 3
                                                      get_local 1
                                                      i32.store offset=12
                                                      get_local 1
                                                      get_local 3
                                                      i32.store offset=8
                                                      get_local 5
                                                      br_if 11 (;@14;)
                                                      br 12 (;@13;)
                                                    end
                                                    block  ;; label = @25
                                                      block  ;; label = @26
                                                        block  ;; label = @27
                                                          block  ;; label = @28
                                                            get_local 0
                                                            i32.load offset=400
                                                            tee_local 1
                                                            get_local 9
                                                            i32.ge_u
                                                            br_if 0 (;@28;)
                                                            get_local 0
                                                            i32.load offset=404
                                                            tee_local 1
                                                            get_local 9
                                                            i32.le_u
                                                            br_if 1 (;@27;)
                                                            get_local 0
                                                            i32.const 404
                                                            i32.add
                                                            get_local 1
                                                            get_local 9
                                                            i32.sub
                                                            tee_local 3
                                                            i32.store
                                                            get_local 0
                                                            get_local 0
                                                            i32.load offset=412
                                                            tee_local 1
                                                            get_local 9
                                                            i32.add
                                                            tee_local 4
                                                            i32.store offset=412
                                                            get_local 4
                                                            get_local 3
                                                            i32.const 1
                                                            i32.or
                                                            i32.store offset=4
                                                            get_local 1
                                                            get_local 9
                                                            i32.const 3
                                                            i32.or
                                                            i32.store offset=4
                                                            get_local 1
                                                            i32.const 8
                                                            i32.add
                                                            return
                                                          end
                                                          get_local 0
                                                          i32.load offset=408
                                                          set_local 3
                                                          get_local 1
                                                          get_local 9
                                                          i32.sub
                                                          tee_local 4
                                                          i32.const 16
                                                          i32.ge_u
                                                          br_if 1 (;@26;)
                                                          get_local 0
                                                          i32.const 408
                                                          i32.add
                                                          i32.const 0
                                                          i32.store
                                                          get_local 0
                                                          i32.const 400
                                                          i32.add
                                                          i32.const 0
                                                          i32.store
                                                          get_local 3
                                                          get_local 1
                                                          i32.const 3
                                                          i32.or
                                                          i32.store offset=4
                                                          get_local 3
                                                          get_local 1
                                                          i32.add
                                                          tee_local 4
                                                          i32.const 4
                                                          i32.add
                                                          set_local 1
                                                          get_local 4
                                                          i32.load offset=4
                                                          i32.const 1
                                                          i32.or
                                                          set_local 4
                                                          br 2 (;@25;)
                                                        end
                                                        memory.size
                                                        set_local 1
                                                        get_local 9
                                                        i32.const 65583
                                                        i32.add
                                                        i32.const 16
                                                        i32.shr_u
                                                        tee_local 3
                                                        memory.grow
                                                        drop
                                                        i32.const 0
                                                        set_local 6
                                                        get_local 1
                                                        i32.const 16
                                                        i32.shl
                                                        tee_local 8
                                                        i32.eqz
                                                        br_if 18 (;@8;)
                                                        get_local 0
                                                        get_local 0
                                                        i32.load offset=416
                                                        get_local 3
                                                        i32.const 16
                                                        i32.shl
                                                        tee_local 7
                                                        i32.add
                                                        tee_local 1
                                                        i32.store offset=416
                                                        get_local 0
                                                        get_local 1
                                                        get_local 0
                                                        i32.load offset=420
                                                        tee_local 3
                                                        get_local 1
                                                        get_local 3
                                                        i32.ge_u
                                                        select
                                                        i32.store offset=420
                                                        get_local 0
                                                        i32.load offset=412
                                                        tee_local 3
                                                        i32.eqz
                                                        br_if 4 (;@22;)
                                                        get_local 0
                                                        i32.const 424
                                                        i32.add
                                                        tee_local 2
                                                        set_local 1
                                                        loop  ;; label = @27
                                                          get_local 8
                                                          get_local 1
                                                          i32.load
                                                          tee_local 4
                                                          get_local 1
                                                          i32.load offset=4
                                                          tee_local 5
                                                          i32.add
                                                          i32.eq
                                                          br_if 6 (;@21;)
                                                          get_local 1
                                                          i32.load offset=8
                                                          tee_local 1
                                                          br_if 0 (;@27;)
                                                          br 17 (;@10;)
                                                        end
                                                        unreachable
                                                      end
                                                      get_local 0
                                                      i32.const 400
                                                      i32.add
                                                      get_local 4
                                                      i32.store
                                                      get_local 0
                                                      i32.const 408
                                                      i32.add
                                                      get_local 3
                                                      get_local 9
                                                      i32.add
                                                      tee_local 1
                                                      i32.store
                                                      get_local 1
                                                      get_local 4
                                                      i32.const 1
                                                      i32.or
                                                      i32.store offset=4
                                                      get_local 1
                                                      get_local 4
                                                      i32.add
                                                      get_local 4
                                                      i32.store
                                                      get_local 9
                                                      i32.const 3
                                                      i32.or
                                                      set_local 4
                                                      get_local 3
                                                      i32.const 4
                                                      i32.add
                                                      set_local 1
                                                    end
                                                    get_local 1
                                                    get_local 4
                                                    i32.store
                                                    get_local 3
                                                    i32.const 8
                                                    i32.add
                                                    return
                                                  end
                                                  get_local 0
                                                  get_local 8
                                                  i32.const -2
                                                  get_local 3
                                                  i32.rotl
                                                  i32.and
                                                  i32.store
                                                end
                                                get_local 1
                                                get_local 9
                                                i32.const 3
                                                i32.or
                                                i32.store offset=4
                                                get_local 1
                                                get_local 9
                                                i32.add
                                                tee_local 8
                                                get_local 3
                                                i32.const 3
                                                i32.shl
                                                get_local 9
                                                i32.sub
                                                tee_local 3
                                                i32.const 1
                                                i32.or
                                                i32.store offset=4
                                                get_local 8
                                                get_local 3
                                                i32.add
                                                get_local 3
                                                i32.store
                                                get_local 0
                                                i32.const 400
                                                i32.add
                                                tee_local 6
                                                i32.load
                                                tee_local 4
                                                i32.eqz
                                                br_if 5 (;@17;)
                                                get_local 0
                                                get_local 4
                                                i32.const 3
                                                i32.shr_u
                                                tee_local 5
                                                i32.const 3
                                                i32.shl
                                                i32.add
                                                i32.const 8
                                                i32.add
                                                set_local 9
                                                get_local 0
                                                i32.const 408
                                                i32.add
                                                i32.load
                                                set_local 4
                                                get_local 0
                                                i32.load
                                                tee_local 10
                                                i32.const 1
                                                get_local 5
                                                i32.const 31
                                                i32.and
                                                i32.shl
                                                tee_local 5
                                                i32.and
                                                i32.eqz
                                                br_if 3 (;@19;)
                                                get_local 9
                                                i32.load offset=8
                                                set_local 5
                                                br 4 (;@18;)
                                              end
                                              block  ;; label = @22
                                                block  ;; label = @23
                                                  get_local 0
                                                  i32.load offset=444
                                                  tee_local 1
                                                  i32.eqz
                                                  br_if 0 (;@23;)
                                                  get_local 8
                                                  get_local 1
                                                  i32.ge_u
                                                  br_if 1 (;@22;)
                                                end
                                                get_local 0
                                                i32.const 444
                                                i32.add
                                                get_local 8
                                                i32.store
                                              end
                                              get_local 0
                                              get_local 8
                                              i32.store offset=424
                                              get_local 0
                                              i32.const 4095
                                              i32.store offset=448
                                              get_local 0
                                              i32.const 428
                                              i32.add
                                              get_local 7
                                              i32.store
                                              i32.const 0
                                              set_local 1
                                              get_local 0
                                              i32.const 436
                                              i32.add
                                              i32.const 0
                                              i32.store
                                              loop  ;; label = @22
                                                get_local 0
                                                get_local 1
                                                i32.add
                                                tee_local 3
                                                i32.const 16
                                                i32.add
                                                get_local 3
                                                i32.const 8
                                                i32.add
                                                tee_local 4
                                                i32.store
                                                get_local 3
                                                i32.const 20
                                                i32.add
                                                get_local 4
                                                i32.store
                                                get_local 1
                                                i32.const 8
                                                i32.add
                                                tee_local 1
                                                i32.const 256
                                                i32.ne
                                                br_if 0 (;@22;)
                                              end
                                              get_local 0
                                              i32.const 2097152
                                              i32.store offset=440
                                              get_local 0
                                              i32.const 404
                                              i32.add
                                              get_local 7
                                              i32.const -40
                                              i32.add
                                              tee_local 1
                                              i32.store
                                              get_local 0
                                              i32.const 412
                                              i32.add
                                              get_local 8
                                              i32.store
                                              get_local 8
                                              get_local 1
                                              i32.const 1
                                              i32.or
                                              i32.store offset=4
                                              get_local 8
                                              get_local 1
                                              i32.add
                                              i32.const 40
                                              i32.store offset=4
                                              br 12 (;@9;)
                                            end
                                            get_local 1
                                            i32.load offset=12
                                            i32.eqz
                                            br_if 4 (;@16;)
                                            br 10 (;@10;)
                                          end
                                          get_local 8
                                          i32.const 20
                                          i32.add
                                          tee_local 1
                                          get_local 8
                                          i32.const 16
                                          i32.add
                                          get_local 1
                                          i32.load
                                          select
                                          tee_local 3
                                          i32.load
                                          tee_local 1
                                          i32.eqz
                                          br_if 4 (;@15;)
                                          loop  ;; label = @20
                                            get_local 3
                                            set_local 6
                                            get_local 1
                                            i32.const 20
                                            i32.add
                                            tee_local 3
                                            get_local 1
                                            i32.const 16
                                            i32.add
                                            get_local 3
                                            i32.load
                                            select
                                            tee_local 3
                                            i32.load
                                            tee_local 1
                                            br_if 0 (;@20;)
                                          end
                                          get_local 6
                                          i32.load
                                          set_local 1
                                          get_local 6
                                          i32.const 0
                                          i32.store
                                          get_local 5
                                          br_if 5 (;@14;)
                                          br 6 (;@13;)
                                        end
                                        get_local 0
                                        get_local 10
                                        get_local 5
                                        i32.or
                                        i32.store
                                        get_local 9
                                        set_local 5
                                      end
                                      get_local 9
                                      i32.const 8
                                      i32.add
                                      get_local 4
                                      i32.store
                                      get_local 5
                                      get_local 4
                                      i32.store offset=12
                                      get_local 4
                                      get_local 9
                                      i32.store offset=12
                                      get_local 4
                                      get_local 5
                                      i32.store offset=8
                                    end
                                    get_local 0
                                    i32.const 408
                                    i32.add
                                    get_local 8
                                    i32.store
                                    get_local 6
                                    get_local 3
                                    i32.store
                                    get_local 1
                                    i32.const 8
                                    i32.add
                                    return
                                  end
                                  get_local 8
                                  get_local 3
                                  i32.le_u
                                  br_if 5 (;@10;)
                                  get_local 4
                                  get_local 3
                                  i32.gt_u
                                  br_if 5 (;@10;)
                                  get_local 1
                                  i32.const 4
                                  i32.add
                                  get_local 5
                                  get_local 7
                                  i32.add
                                  i32.store
                                  get_local 0
                                  i32.const 412
                                  i32.add
                                  tee_local 4
                                  i32.load
                                  tee_local 3
                                  i32.const 15
                                  i32.add
                                  i32.const -8
                                  i32.and
                                  tee_local 8
                                  i32.const -8
                                  i32.add
                                  tee_local 1
                                  get_local 0
                                  i32.const 404
                                  i32.add
                                  tee_local 5
                                  i32.load
                                  get_local 7
                                  i32.add
                                  get_local 8
                                  get_local 3
                                  i32.const 8
                                  i32.add
                                  i32.sub
                                  i32.sub
                                  tee_local 3
                                  i32.const 1
                                  i32.or
                                  i32.store offset=4
                                  get_local 4
                                  get_local 1
                                  i32.store
                                  get_local 5
                                  get_local 3
                                  i32.store
                                  get_local 1
                                  get_local 3
                                  i32.add
                                  i32.const 40
                                  i32.store offset=4
                                  get_local 0
                                  i32.const 2097152
                                  i32.store offset=440
                                  br 6 (;@9;)
                                end
                                i32.const 0
                                set_local 1
                                get_local 5
                                i32.eqz
                                br_if 1 (;@13;)
                              end
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 0
                                    get_local 8
                                    i32.load offset=28
                                    tee_local 6
                                    i32.const 2
                                    i32.shl
                                    i32.add
                                    i32.const 272
                                    i32.add
                                    tee_local 3
                                    i32.load
                                    get_local 8
                                    i32.eq
                                    br_if 0 (;@16;)
                                    get_local 5
                                    i32.const 16
                                    i32.add
                                    get_local 5
                                    i32.load offset=16
                                    get_local 8
                                    i32.ne
                                    i32.const 2
                                    i32.shl
                                    i32.add
                                    get_local 1
                                    i32.store
                                    get_local 1
                                    br_if 1 (;@15;)
                                    br 3 (;@13;)
                                  end
                                  get_local 3
                                  get_local 1
                                  i32.store
                                  get_local 1
                                  i32.eqz
                                  br_if 1 (;@14;)
                                end
                                get_local 1
                                get_local 5
                                i32.store offset=24
                                block  ;; label = @15
                                  get_local 8
                                  i32.load offset=16
                                  tee_local 3
                                  i32.eqz
                                  br_if 0 (;@15;)
                                  get_local 1
                                  get_local 3
                                  i32.store offset=16
                                  get_local 3
                                  get_local 1
                                  i32.store offset=24
                                end
                                get_local 8
                                i32.const 20
                                i32.add
                                i32.load
                                tee_local 3
                                i32.eqz
                                br_if 1 (;@13;)
                                get_local 1
                                i32.const 20
                                i32.add
                                get_local 3
                                i32.store
                                get_local 3
                                get_local 1
                                i32.store offset=24
                                br 1 (;@13;)
                              end
                              get_local 0
                              i32.const 4
                              i32.add
                              tee_local 1
                              get_local 1
                              i32.load
                              i32.const -2
                              get_local 6
                              i32.rotl
                              i32.and
                              i32.store
                            end
                            block  ;; label = @13
                              block  ;; label = @14
                                get_local 4
                                i32.const 15
                                i32.gt_u
                                br_if 0 (;@14;)
                                get_local 8
                                i32.const 4
                                i32.add
                                get_local 7
                                i32.const 3
                                i32.or
                                i32.store
                                get_local 8
                                get_local 7
                                i32.add
                                tee_local 1
                                get_local 1
                                i32.load offset=4
                                i32.const 1
                                i32.or
                                i32.store offset=4
                                br 1 (;@13;)
                              end
                              get_local 8
                              i32.const 4
                              i32.add
                              get_local 9
                              i32.const 3
                              i32.or
                              i32.store
                              get_local 8
                              get_local 9
                              i32.add
                              tee_local 3
                              get_local 4
                              i32.const 1
                              i32.or
                              i32.store offset=4
                              get_local 3
                              get_local 4
                              i32.add
                              get_local 4
                              i32.store
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        get_local 4
                                        i32.const 255
                                        i32.gt_u
                                        br_if 0 (;@18;)
                                        get_local 0
                                        get_local 4
                                        i32.const 3
                                        i32.shr_u
                                        tee_local 4
                                        i32.const 3
                                        i32.shl
                                        i32.add
                                        i32.const 8
                                        i32.add
                                        set_local 1
                                        get_local 0
                                        i32.load
                                        tee_local 9
                                        i32.const 1
                                        get_local 4
                                        i32.const 31
                                        i32.and
                                        i32.shl
                                        tee_local 4
                                        i32.and
                                        i32.eqz
                                        br_if 1 (;@17;)
                                        get_local 1
                                        i32.const 8
                                        i32.add
                                        set_local 9
                                        get_local 1
                                        i32.load offset=8
                                        set_local 4
                                        br 2 (;@16;)
                                      end
                                      get_local 4
                                      i32.const 8
                                      i32.shr_u
                                      tee_local 9
                                      i32.eqz
                                      br_if 2 (;@15;)
                                      i32.const 31
                                      set_local 1
                                      get_local 4
                                      i32.const 16777215
                                      i32.gt_u
                                      br_if 3 (;@14;)
                                      get_local 4
                                      i32.const 38
                                      get_local 9
                                      i32.clz
                                      tee_local 1
                                      i32.sub
                                      i32.const 31
                                      i32.and
                                      i32.shr_u
                                      i32.const 1
                                      i32.and
                                      i32.const 31
                                      get_local 1
                                      i32.sub
                                      i32.const 1
                                      i32.shl
                                      i32.or
                                      set_local 1
                                      br 3 (;@14;)
                                    end
                                    get_local 0
                                    get_local 9
                                    get_local 4
                                    i32.or
                                    i32.store
                                    get_local 1
                                    i32.const 8
                                    i32.add
                                    set_local 9
                                    get_local 1
                                    set_local 4
                                  end
                                  get_local 9
                                  get_local 3
                                  i32.store
                                  get_local 4
                                  get_local 3
                                  i32.store offset=12
                                  get_local 3
                                  get_local 1
                                  i32.store offset=12
                                  get_local 3
                                  get_local 4
                                  i32.store offset=8
                                  br 2 (;@13;)
                                end
                                i32.const 0
                                set_local 1
                              end
                              get_local 3
                              get_local 1
                              i32.store offset=28
                              get_local 3
                              i64.const 0
                              i64.store offset=16 align=4
                              get_local 0
                              get_local 1
                              i32.const 2
                              i32.shl
                              i32.add
                              i32.const 272
                              i32.add
                              set_local 9
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    block  ;; label = @17
                                      block  ;; label = @18
                                        get_local 0
                                        i32.const 4
                                        i32.add
                                        tee_local 6
                                        i32.load
                                        tee_local 5
                                        i32.const 1
                                        get_local 1
                                        i32.const 31
                                        i32.and
                                        i32.shl
                                        tee_local 0
                                        i32.and
                                        i32.eqz
                                        br_if 0 (;@18;)
                                        get_local 9
                                        i32.load
                                        tee_local 6
                                        i32.load offset=4
                                        i32.const -8
                                        i32.and
                                        get_local 4
                                        i32.ne
                                        br_if 1 (;@17;)
                                        get_local 6
                                        set_local 1
                                        br 2 (;@16;)
                                      end
                                      get_local 6
                                      get_local 5
                                      get_local 0
                                      i32.or
                                      i32.store
                                      get_local 3
                                      get_local 9
                                      i32.store offset=24
                                      get_local 9
                                      get_local 3
                                      i32.store
                                      br 3 (;@14;)
                                    end
                                    get_local 4
                                    i32.const 0
                                    i32.const 25
                                    get_local 1
                                    i32.const 1
                                    i32.shr_u
                                    i32.sub
                                    i32.const 31
                                    i32.and
                                    get_local 1
                                    i32.const 31
                                    i32.eq
                                    select
                                    i32.shl
                                    set_local 9
                                    loop  ;; label = @17
                                      get_local 6
                                      get_local 9
                                      i32.const 29
                                      i32.shr_u
                                      i32.const 4
                                      i32.and
                                      i32.add
                                      i32.const 16
                                      i32.add
                                      tee_local 5
                                      i32.load
                                      tee_local 1
                                      i32.eqz
                                      br_if 2 (;@15;)
                                      get_local 9
                                      i32.const 1
                                      i32.shl
                                      set_local 9
                                      get_local 1
                                      set_local 6
                                      get_local 1
                                      i32.load offset=4
                                      i32.const -8
                                      i32.and
                                      get_local 4
                                      i32.ne
                                      br_if 0 (;@17;)
                                    end
                                  end
                                  get_local 1
                                  i32.load offset=8
                                  tee_local 4
                                  get_local 3
                                  i32.store offset=12
                                  get_local 1
                                  get_local 3
                                  i32.store offset=8
                                  get_local 3
                                  get_local 1
                                  i32.store offset=12
                                  get_local 3
                                  get_local 4
                                  i32.store offset=8
                                  get_local 3
                                  i32.const 0
                                  i32.store offset=24
                                  br 2 (;@13;)
                                end
                                get_local 5
                                get_local 3
                                i32.store
                                get_local 3
                                get_local 6
                                i32.store offset=24
                              end
                              get_local 3
                              get_local 3
                              i32.store offset=12
                              get_local 3
                              get_local 3
                              i32.store offset=8
                            end
                            get_local 8
                            i32.const 8
                            i32.add
                            return
                          end
                          i32.const 1
                          set_local 11
                        end
                        loop  ;; label = @11
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
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    block  ;; label = @25
                                                      block  ;; label = @26
                                                        block  ;; label = @27
                                                          block  ;; label = @28
                                                            block  ;; label = @29
                                                              block  ;; label = @30
                                                                block  ;; label = @31
                                                                  block  ;; label = @32
                                                                    block  ;; label = @33
                                                                      block  ;; label = @34
                                                                        block  ;; label = @35
                                                                          block  ;; label = @36
                                                                            block  ;; label = @37
                                                                              block  ;; label = @38
                                                                                block  ;; label = @39
                                                                                  block  ;; label = @40
                                                                                    block  ;; label = @41
                                                                                      block  ;; label = @42
                                                                                        block  ;; label = @43
                                                                                          block  ;; label = @44
                                                                                            block  ;; label = @45
                                                                                              block  ;; label = @46
                                                                                                block  ;; label = @47
                                                                                                  block  ;; label = @48
                                                                                                    block  ;; label = @49
                                                                                                      block  ;; label = @50
                                                                                                        block  ;; label = @51
                                                                                                          block  ;; label = @52
                                                                                                            block  ;; label = @53
                                                                                                              block  ;; label = @54
                                                                                                                block  ;; label = @55
                                                                                                                  block  ;; label = @56
                                                                                                                    block  ;; label = @57
                                                                                                                      block  ;; label = @58
                                                                                                                        get_local 11
                                                                                                                        br_table 0 (;@58;) 1 (;@57;) 2 (;@56;) 3 (;@55;) 8 (;@50;) 9 (;@49;) 11 (;@47;) 12 (;@46;) 13 (;@45;) 14 (;@44;) 16 (;@42;) 18 (;@40;) 19 (;@39;) 20 (;@38;) 22 (;@36;) 23 (;@35;) 24 (;@34;) 21 (;@37;) 17 (;@41;) 10 (;@48;) 15 (;@43;) 4 (;@54;) 5 (;@53;) 6 (;@52;) 7 (;@51;) 7 (;@51;)
                                                                                                                      end
                                                                                                                      get_local 1
                                                                                                                      i32.load offset=4
                                                                                                                      i32.const -8
                                                                                                                      i32.and
                                                                                                                      get_local 9
                                                                                                                      i32.sub
                                                                                                                      tee_local 8
                                                                                                                      get_local 3
                                                                                                                      get_local 8
                                                                                                                      get_local 3
                                                                                                                      i32.lt_u
                                                                                                                      tee_local 8
                                                                                                                      select
                                                                                                                      set_local 3
                                                                                                                      get_local 1
                                                                                                                      get_local 4
                                                                                                                      get_local 8
                                                                                                                      select
                                                                                                                      set_local 4
                                                                                                                      get_local 1
                                                                                                                      tee_local 8
                                                                                                                      i32.load offset=16
                                                                                                                      tee_local 1
                                                                                                                      br_if 24 (;@33;)
                                                                                                                      i32.const 1
                                                                                                                      set_local 11
                                                                                                                      br 46 (;@11;)
                                                                                                                    end
                                                                                                                    get_local 8
                                                                                                                    i32.const 20
                                                                                                                    i32.add
                                                                                                                    i32.load
                                                                                                                    tee_local 1
                                                                                                                    br_if 24 (;@32;)
                                                                                                                    i32.const 2
                                                                                                                    set_local 11
                                                                                                                    br 45 (;@11;)
                                                                                                                  end
                                                                                                                  get_local 4
                                                                                                                  i32.load offset=24
                                                                                                                  set_local 5
                                                                                                                  get_local 4
                                                                                                                  i32.load offset=12
                                                                                                                  tee_local 1
                                                                                                                  get_local 4
                                                                                                                  i32.eq
                                                                                                                  br_if 24 (;@31;)
                                                                                                                  i32.const 3
                                                                                                                  set_local 11
                                                                                                                  br 44 (;@11;)
                                                                                                                end
                                                                                                                get_local 4
                                                                                                                i32.load offset=8
                                                                                                                tee_local 8
                                                                                                                get_local 1
                                                                                                                i32.store offset=12
                                                                                                                get_local 1
                                                                                                                get_local 8
                                                                                                                i32.store offset=8
                                                                                                                get_local 5
                                                                                                                br_if 25 (;@29;)
                                                                                                                br 24 (;@30;)
                                                                                                              end
                                                                                                              get_local 4
                                                                                                              i32.const 20
                                                                                                              i32.add
                                                                                                              tee_local 1
                                                                                                              get_local 4
                                                                                                              i32.const 16
                                                                                                              i32.add
                                                                                                              get_local 1
                                                                                                              i32.load
                                                                                                              select
                                                                                                              tee_local 8
                                                                                                              i32.load
                                                                                                              tee_local 1
                                                                                                              i32.eqz
                                                                                                              br_if 41 (;@12;)
                                                                                                              i32.const 22
                                                                                                              set_local 11
                                                                                                              br 42 (;@11;)
                                                                                                            end
                                                                                                            get_local 8
                                                                                                            set_local 6
                                                                                                            get_local 1
                                                                                                            i32.const 20
                                                                                                            i32.add
                                                                                                            tee_local 8
                                                                                                            get_local 1
                                                                                                            i32.const 16
                                                                                                            i32.add
                                                                                                            get_local 8
                                                                                                            i32.load
                                                                                                            select
                                                                                                            tee_local 8
                                                                                                            i32.load
                                                                                                            tee_local 1
                                                                                                            br_if 39 (;@13;)
                                                                                                            i32.const 23
                                                                                                            set_local 11
                                                                                                            br 41 (;@11;)
                                                                                                          end
                                                                                                          get_local 6
                                                                                                          i32.load
                                                                                                          set_local 1
                                                                                                          get_local 6
                                                                                                          i32.const 0
                                                                                                          i32.store
                                                                                                          get_local 5
                                                                                                          br_if 24 (;@27;)
                                                                                                          br 23 (;@28;)
                                                                                                        end
                                                                                                        i32.const 0
                                                                                                        set_local 1
                                                                                                        get_local 5
                                                                                                        i32.eqz
                                                                                                        br_if 24 (;@26;)
                                                                                                        i32.const 4
                                                                                                        set_local 11
                                                                                                        br 39 (;@11;)
                                                                                                      end
                                                                                                      get_local 0
                                                                                                      get_local 4
                                                                                                      i32.load offset=28
                                                                                                      tee_local 6
                                                                                                      i32.const 2
                                                                                                      i32.shl
                                                                                                      i32.add
                                                                                                      i32.const 272
                                                                                                      i32.add
                                                                                                      tee_local 8
                                                                                                      i32.load
                                                                                                      get_local 4
                                                                                                      i32.eq
                                                                                                      br_if 24 (;@25;)
                                                                                                      i32.const 5
                                                                                                      set_local 11
                                                                                                      br 38 (;@11;)
                                                                                                    end
                                                                                                    get_local 5
                                                                                                    i32.const 16
                                                                                                    i32.add
                                                                                                    get_local 5
                                                                                                    i32.load offset=16
                                                                                                    get_local 4
                                                                                                    i32.ne
                                                                                                    i32.const 2
                                                                                                    i32.shl
                                                                                                    i32.add
                                                                                                    get_local 1
                                                                                                    i32.store
                                                                                                    get_local 1
                                                                                                    br_if 25 (;@23;)
                                                                                                    br 24 (;@24;)
                                                                                                  end
                                                                                                  get_local 8
                                                                                                  get_local 1
                                                                                                  i32.store
                                                                                                  get_local 1
                                                                                                  i32.eqz
                                                                                                  br_if 25 (;@22;)
                                                                                                  i32.const 6
                                                                                                  set_local 11
                                                                                                  br 36 (;@11;)
                                                                                                end
                                                                                                get_local 1
                                                                                                get_local 5
                                                                                                i32.store offset=24
                                                                                                get_local 4
                                                                                                i32.load offset=16
                                                                                                tee_local 8
                                                                                                i32.eqz
                                                                                                br_if 25 (;@21;)
                                                                                                i32.const 7
                                                                                                set_local 11
                                                                                                br 35 (;@11;)
                                                                                              end
                                                                                              get_local 1
                                                                                              get_local 8
                                                                                              i32.store offset=16
                                                                                              get_local 8
                                                                                              get_local 1
                                                                                              i32.store offset=24
                                                                                              i32.const 8
                                                                                              set_local 11
                                                                                              br 34 (;@11;)
                                                                                            end
                                                                                            get_local 4
                                                                                            i32.const 20
                                                                                            i32.add
                                                                                            i32.load
                                                                                            tee_local 8
                                                                                            i32.eqz
                                                                                            br_if 24 (;@20;)
                                                                                            i32.const 9
                                                                                            set_local 11
                                                                                            br 33 (;@11;)
                                                                                          end
                                                                                          get_local 1
                                                                                          i32.const 20
                                                                                          i32.add
                                                                                          get_local 8
                                                                                          i32.store
                                                                                          get_local 8
                                                                                          get_local 1
                                                                                          i32.store offset=24
                                                                                          br 24 (;@19;)
                                                                                        end
                                                                                        get_local 0
                                                                                        i32.const 4
                                                                                        i32.add
                                                                                        tee_local 1
                                                                                        get_local 1
                                                                                        i32.load
                                                                                        i32.const -2
                                                                                        get_local 6
                                                                                        i32.rotl
                                                                                        i32.and
                                                                                        i32.store
                                                                                        i32.const 10
                                                                                        set_local 11
                                                                                        br 31 (;@11;)
                                                                                      end
                                                                                      get_local 3
                                                                                      i32.const 16
                                                                                      i32.ge_u
                                                                                      br_if 23 (;@18;)
                                                                                      i32.const 18
                                                                                      set_local 11
                                                                                      br 30 (;@11;)
                                                                                    end
                                                                                    get_local 4
                                                                                    get_local 3
                                                                                    get_local 9
                                                                                    i32.add
                                                                                    tee_local 1
                                                                                    i32.const 3
                                                                                    i32.or
                                                                                    i32.store offset=4
                                                                                    get_local 4
                                                                                    get_local 1
                                                                                    i32.add
                                                                                    tee_local 1
                                                                                    get_local 1
                                                                                    i32.load offset=4
                                                                                    i32.const 1
                                                                                    i32.or
                                                                                    i32.store offset=4
                                                                                    br 26 (;@14;)
                                                                                  end
                                                                                  get_local 4
                                                                                  get_local 9
                                                                                  i32.const 3
                                                                                  i32.or
                                                                                  i32.store offset=4
                                                                                  get_local 4
                                                                                  get_local 9
                                                                                  i32.add
                                                                                  tee_local 9
                                                                                  get_local 3
                                                                                  i32.const 1
                                                                                  i32.or
                                                                                  i32.store offset=4
                                                                                  get_local 9
                                                                                  get_local 3
                                                                                  i32.add
                                                                                  get_local 3
                                                                                  i32.store
                                                                                  get_local 0
                                                                                  i32.const 400
                                                                                  i32.add
                                                                                  tee_local 6
                                                                                  i32.load
                                                                                  tee_local 1
                                                                                  i32.eqz
                                                                                  br_if 22 (;@17;)
                                                                                  i32.const 12
                                                                                  set_local 11
                                                                                  br 28 (;@11;)
                                                                                end
                                                                                get_local 0
                                                                                get_local 1
                                                                                i32.const 3
                                                                                i32.shr_u
                                                                                tee_local 5
                                                                                i32.const 3
                                                                                i32.shl
                                                                                i32.add
                                                                                i32.const 8
                                                                                i32.add
                                                                                set_local 8
                                                                                get_local 0
                                                                                i32.const 408
                                                                                i32.add
                                                                                i32.load
                                                                                set_local 1
                                                                                get_local 0
                                                                                i32.load
                                                                                tee_local 10
                                                                                i32.const 1
                                                                                get_local 5
                                                                                i32.const 31
                                                                                i32.and
                                                                                i32.shl
                                                                                tee_local 5
                                                                                i32.and
                                                                                i32.eqz
                                                                                br_if 22 (;@16;)
                                                                                i32.const 13
                                                                                set_local 11
                                                                                br 27 (;@11;)
                                                                              end
                                                                              get_local 8
                                                                              i32.load offset=8
                                                                              set_local 5
                                                                              br 22 (;@15;)
                                                                            end
                                                                            get_local 0
                                                                            get_local 10
                                                                            get_local 5
                                                                            i32.or
                                                                            i32.store
                                                                            get_local 8
                                                                            set_local 5
                                                                            i32.const 14
                                                                            set_local 11
                                                                            br 25 (;@11;)
                                                                          end
                                                                          get_local 8
                                                                          i32.const 8
                                                                          i32.add
                                                                          get_local 1
                                                                          i32.store
                                                                          get_local 5
                                                                          get_local 1
                                                                          i32.store offset=12
                                                                          get_local 1
                                                                          get_local 8
                                                                          i32.store offset=12
                                                                          get_local 1
                                                                          get_local 5
                                                                          i32.store offset=8
                                                                          i32.const 15
                                                                          set_local 11
                                                                          br 24 (;@11;)
                                                                        end
                                                                        get_local 0
                                                                        i32.const 408
                                                                        i32.add
                                                                        get_local 9
                                                                        i32.store
                                                                        get_local 6
                                                                        get_local 3
                                                                        i32.store
                                                                        i32.const 16
                                                                        set_local 11
                                                                        br 23 (;@11;)
                                                                      end
                                                                      get_local 4
                                                                      i32.const 8
                                                                      i32.add
                                                                      return
                                                                    end
                                                                    i32.const 0
                                                                    set_local 11
                                                                    br 21 (;@11;)
                                                                  end
                                                                  i32.const 0
                                                                  set_local 11
                                                                  br 20 (;@11;)
                                                                end
                                                                i32.const 21
                                                                set_local 11
                                                                br 19 (;@11;)
                                                              end
                                                              i32.const 10
                                                              set_local 11
                                                              br 18 (;@11;)
                                                            end
                                                            i32.const 4
                                                            set_local 11
                                                            br 17 (;@11;)
                                                          end
                                                          i32.const 10
                                                          set_local 11
                                                          br 16 (;@11;)
                                                        end
                                                        i32.const 4
                                                        set_local 11
                                                        br 15 (;@11;)
                                                      end
                                                      i32.const 10
                                                      set_local 11
                                                      br 14 (;@11;)
                                                    end
                                                    i32.const 19
                                                    set_local 11
                                                    br 13 (;@11;)
                                                  end
                                                  i32.const 10
                                                  set_local 11
                                                  br 12 (;@11;)
                                                end
                                                i32.const 6
                                                set_local 11
                                                br 11 (;@11;)
                                              end
                                              i32.const 20
                                              set_local 11
                                              br 10 (;@11;)
                                            end
                                            i32.const 8
                                            set_local 11
                                            br 9 (;@11;)
                                          end
                                          i32.const 10
                                          set_local 11
                                          br 8 (;@11;)
                                        end
                                        i32.const 10
                                        set_local 11
                                        br 7 (;@11;)
                                      end
                                      i32.const 11
                                      set_local 11
                                      br 6 (;@11;)
                                    end
                                    i32.const 15
                                    set_local 11
                                    br 5 (;@11;)
                                  end
                                  i32.const 17
                                  set_local 11
                                  br 4 (;@11;)
                                end
                                i32.const 14
                                set_local 11
                                br 3 (;@11;)
                              end
                              i32.const 16
                              set_local 11
                              br 2 (;@11;)
                            end
                            i32.const 22
                            set_local 11
                            br 1 (;@11;)
                          end
                          i32.const 24
                          set_local 11
                          br 0 (;@11;)
                        end
                        unreachable
                      end
                      get_local 0
                      get_local 8
                      get_local 0
                      i32.load offset=444
                      tee_local 1
                      get_local 8
                      get_local 1
                      i32.le_u
                      select
                      i32.store offset=444
                      get_local 8
                      get_local 7
                      i32.add
                      set_local 4
                      get_local 2
                      set_local 1
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
                                            loop  ;; label = @21
                                              get_local 1
                                              i32.load
                                              get_local 4
                                              i32.eq
                                              br_if 1 (;@20;)
                                              get_local 1
                                              i32.load offset=8
                                              tee_local 1
                                              br_if 0 (;@21;)
                                              br 2 (;@19;)
                                            end
                                            unreachable
                                          end
                                          get_local 1
                                          i32.load offset=12
                                          i32.eqz
                                          br_if 1 (;@18;)
                                        end
                                        get_local 2
                                        set_local 1
                                        block  ;; label = @19
                                          loop  ;; label = @20
                                            block  ;; label = @21
                                              get_local 1
                                              i32.load
                                              tee_local 4
                                              get_local 3
                                              i32.gt_u
                                              br_if 0 (;@21;)
                                              get_local 4
                                              get_local 1
                                              i32.load offset=4
                                              i32.add
                                              tee_local 4
                                              get_local 3
                                              i32.gt_u
                                              br_if 2 (;@19;)
                                            end
                                            get_local 1
                                            i32.load offset=8
                                            set_local 1
                                            br 0 (;@20;)
                                          end
                                          unreachable
                                        end
                                        get_local 8
                                        get_local 7
                                        i32.const -40
                                        i32.add
                                        tee_local 1
                                        i32.const 1
                                        i32.or
                                        i32.store offset=4
                                        get_local 8
                                        get_local 1
                                        i32.add
                                        i32.const 40
                                        i32.store offset=4
                                        get_local 0
                                        i32.const 2097152
                                        i32.store offset=440
                                        get_local 0
                                        i32.const 404
                                        i32.add
                                        get_local 1
                                        i32.store
                                        get_local 0
                                        i32.const 412
                                        i32.add
                                        get_local 8
                                        i32.store
                                        get_local 3
                                        get_local 4
                                        i32.const -32
                                        i32.add
                                        i32.const -8
                                        i32.and
                                        i32.const -8
                                        i32.add
                                        tee_local 1
                                        get_local 1
                                        get_local 3
                                        i32.const 16
                                        i32.add
                                        i32.lt_u
                                        select
                                        tee_local 5
                                        i32.const 27
                                        i32.store offset=4
                                        get_local 2
                                        i64.load align=4
                                        set_local 13
                                        get_local 5
                                        i32.const 16
                                        i32.add
                                        get_local 2
                                        i32.const 8
                                        i32.add
                                        i64.load align=4
                                        tee_local 12
                                        i64.store align=4
                                        get_local 10
                                        i32.const 8
                                        i32.add
                                        get_local 12
                                        i64.store
                                        get_local 5
                                        get_local 13
                                        i64.store offset=8 align=4
                                        get_local 10
                                        get_local 13
                                        i64.store
                                        get_local 0
                                        i32.const 428
                                        i32.add
                                        get_local 7
                                        i32.store
                                        get_local 0
                                        i32.const 424
                                        i32.add
                                        get_local 8
                                        i32.store
                                        get_local 0
                                        i32.const 436
                                        i32.add
                                        i32.const 0
                                        i32.store
                                        get_local 0
                                        i32.const 432
                                        i32.add
                                        get_local 5
                                        i32.const 8
                                        i32.add
                                        i32.store
                                        get_local 5
                                        i32.const 28
                                        i32.add
                                        set_local 1
                                        loop  ;; label = @19
                                          get_local 1
                                          i32.const 7
                                          i32.store
                                          get_local 1
                                          i32.const 4
                                          i32.add
                                          tee_local 1
                                          get_local 4
                                          i32.lt_u
                                          br_if 0 (;@19;)
                                        end
                                        get_local 5
                                        get_local 3
                                        i32.eq
                                        br_if 9 (;@9;)
                                        get_local 5
                                        get_local 5
                                        i32.load offset=4
                                        i32.const -2
                                        i32.and
                                        i32.store offset=4
                                        get_local 3
                                        get_local 5
                                        get_local 3
                                        i32.sub
                                        tee_local 8
                                        i32.const 1
                                        i32.or
                                        i32.store offset=4
                                        get_local 5
                                        get_local 8
                                        i32.store
                                        block  ;; label = @19
                                          get_local 8
                                          i32.const 255
                                          i32.gt_u
                                          br_if 0 (;@19;)
                                          get_local 0
                                          get_local 8
                                          i32.const 3
                                          i32.shr_u
                                          tee_local 4
                                          i32.const 3
                                          i32.shl
                                          i32.add
                                          i32.const 8
                                          i32.add
                                          set_local 1
                                          get_local 0
                                          i32.load
                                          tee_local 8
                                          i32.const 1
                                          get_local 4
                                          i32.const 31
                                          i32.and
                                          i32.shl
                                          tee_local 4
                                          i32.and
                                          i32.eqz
                                          br_if 2 (;@17;)
                                          get_local 1
                                          i32.load offset=8
                                          set_local 4
                                          br 3 (;@16;)
                                        end
                                        get_local 8
                                        i32.const 8
                                        i32.shr_u
                                        tee_local 4
                                        i32.eqz
                                        br_if 3 (;@15;)
                                        i32.const 31
                                        set_local 1
                                        get_local 8
                                        i32.const 16777215
                                        i32.gt_u
                                        br_if 4 (;@14;)
                                        get_local 8
                                        i32.const 38
                                        get_local 4
                                        i32.clz
                                        tee_local 1
                                        i32.sub
                                        i32.const 31
                                        i32.and
                                        i32.shr_u
                                        i32.const 1
                                        i32.and
                                        i32.const 31
                                        get_local 1
                                        i32.sub
                                        i32.const 1
                                        i32.shl
                                        i32.or
                                        set_local 1
                                        br 4 (;@14;)
                                      end
                                      get_local 1
                                      get_local 8
                                      i32.store
                                      get_local 1
                                      get_local 1
                                      i32.load offset=4
                                      get_local 7
                                      i32.add
                                      i32.store offset=4
                                      get_local 8
                                      get_local 9
                                      i32.const 3
                                      i32.or
                                      i32.store offset=4
                                      get_local 8
                                      get_local 9
                                      i32.add
                                      set_local 1
                                      get_local 4
                                      get_local 8
                                      i32.sub
                                      get_local 9
                                      i32.sub
                                      set_local 3
                                      get_local 4
                                      get_local 0
                                      i32.const 412
                                      i32.add
                                      tee_local 9
                                      i32.load
                                      i32.eq
                                      br_if 4 (;@13;)
                                      get_local 4
                                      get_local 0
                                      i32.load offset=408
                                      i32.eq
                                      br_if 5 (;@12;)
                                      get_local 4
                                      i32.load offset=4
                                      tee_local 9
                                      i32.const 3
                                      i32.and
                                      i32.const 1
                                      i32.ne
                                      br_if 15 (;@2;)
                                      get_local 9
                                      i32.const -8
                                      i32.and
                                      tee_local 10
                                      i32.const 255
                                      i32.gt_u
                                      br_if 10 (;@7;)
                                      get_local 4
                                      i32.load offset=12
                                      tee_local 6
                                      get_local 4
                                      i32.load offset=8
                                      tee_local 5
                                      i32.eq
                                      br_if 11 (;@6;)
                                      get_local 5
                                      get_local 6
                                      i32.store offset=12
                                      get_local 6
                                      get_local 5
                                      i32.store offset=8
                                      br 14 (;@3;)
                                    end
                                    get_local 0
                                    get_local 8
                                    get_local 4
                                    i32.or
                                    i32.store
                                    get_local 1
                                    set_local 4
                                  end
                                  get_local 1
                                  i32.const 8
                                  i32.add
                                  get_local 3
                                  i32.store
                                  get_local 4
                                  get_local 3
                                  i32.store offset=12
                                  get_local 3
                                  get_local 1
                                  i32.store offset=12
                                  get_local 3
                                  get_local 4
                                  i32.store offset=8
                                  br 6 (;@9;)
                                end
                                i32.const 0
                                set_local 1
                              end
                              get_local 3
                              i64.const 0
                              i64.store offset=16 align=4
                              get_local 3
                              i32.const 28
                              i32.add
                              get_local 1
                              i32.store
                              get_local 0
                              get_local 1
                              i32.const 2
                              i32.shl
                              i32.add
                              i32.const 272
                              i32.add
                              set_local 4
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 0
                                    i32.load offset=4
                                    tee_local 5
                                    i32.const 1
                                    get_local 1
                                    i32.const 31
                                    i32.and
                                    i32.shl
                                    tee_local 10
                                    i32.and
                                    i32.eqz
                                    br_if 0 (;@16;)
                                    get_local 4
                                    i32.load
                                    tee_local 5
                                    i32.load offset=4
                                    i32.const -8
                                    i32.and
                                    get_local 8
                                    i32.ne
                                    br_if 1 (;@15;)
                                    get_local 5
                                    set_local 1
                                    br 2 (;@14;)
                                  end
                                  get_local 0
                                  i32.const 4
                                  i32.add
                                  get_local 5
                                  get_local 10
                                  i32.or
                                  i32.store
                                  get_local 3
                                  i32.const 24
                                  i32.add
                                  get_local 4
                                  i32.store
                                  get_local 4
                                  get_local 3
                                  i32.store
                                  br 5 (;@10;)
                                end
                                get_local 8
                                i32.const 0
                                i32.const 25
                                get_local 1
                                i32.const 1
                                i32.shr_u
                                i32.sub
                                i32.const 31
                                i32.and
                                get_local 1
                                i32.const 31
                                i32.eq
                                select
                                i32.shl
                                set_local 4
                                loop  ;; label = @15
                                  get_local 5
                                  get_local 4
                                  i32.const 29
                                  i32.shr_u
                                  i32.const 4
                                  i32.and
                                  i32.add
                                  i32.const 16
                                  i32.add
                                  tee_local 10
                                  i32.load
                                  tee_local 1
                                  i32.eqz
                                  br_if 4 (;@11;)
                                  get_local 4
                                  i32.const 1
                                  i32.shl
                                  set_local 4
                                  get_local 1
                                  set_local 5
                                  get_local 1
                                  i32.load offset=4
                                  i32.const -8
                                  i32.and
                                  get_local 8
                                  i32.ne
                                  br_if 0 (;@15;)
                                end
                              end
                              get_local 1
                              i32.load offset=8
                              tee_local 4
                              get_local 3
                              i32.store offset=12
                              get_local 1
                              get_local 3
                              i32.store offset=8
                              get_local 3
                              get_local 1
                              i32.store offset=12
                              get_local 3
                              get_local 4
                              i32.store offset=8
                              get_local 3
                              i32.const 24
                              i32.add
                              i32.const 0
                              i32.store
                              br 4 (;@9;)
                            end
                            get_local 9
                            get_local 1
                            i32.store
                            get_local 0
                            i32.const 404
                            i32.add
                            tee_local 4
                            get_local 4
                            i32.load
                            get_local 3
                            i32.add
                            tee_local 3
                            i32.store
                            get_local 1
                            get_local 3
                            i32.const 1
                            i32.or
                            i32.store offset=4
                            br 11 (;@1;)
                          end
                          get_local 1
                          get_local 0
                          i32.const 400
                          i32.add
                          tee_local 4
                          i32.load
                          get_local 3
                          i32.add
                          tee_local 3
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          get_local 0
                          i32.const 408
                          i32.add
                          get_local 1
                          i32.store
                          get_local 4
                          get_local 3
                          i32.store
                          get_local 1
                          get_local 3
                          i32.add
                          get_local 3
                          i32.store
                          br 10 (;@1;)
                        end
                        get_local 10
                        get_local 3
                        i32.store
                        get_local 3
                        i32.const 24
                        i32.add
                        get_local 5
                        i32.store
                      end
                      get_local 3
                      get_local 3
                      i32.store offset=12
                      get_local 3
                      get_local 3
                      i32.store offset=8
                    end
                    get_local 0
                    i32.const 404
                    i32.add
                    tee_local 1
                    i32.load
                    tee_local 3
                    get_local 9
                    i32.le_u
                    br_if 0 (;@8;)
                    get_local 1
                    get_local 3
                    get_local 9
                    i32.sub
                    tee_local 3
                    i32.store
                    get_local 0
                    i32.const 412
                    i32.add
                    tee_local 1
                    get_local 1
                    i32.load
                    tee_local 1
                    get_local 9
                    i32.add
                    tee_local 4
                    i32.store
                    get_local 4
                    get_local 3
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    get_local 1
                    get_local 9
                    i32.const 3
                    i32.or
                    i32.store offset=4
                    get_local 1
                    i32.const 8
                    i32.add
                    return
                  end
                  get_local 6
                  return
                end
                get_local 4
                i32.load offset=24
                set_local 7
                get_local 4
                i32.load offset=12
                tee_local 9
                get_local 4
                i32.eq
                br_if 1 (;@5;)
                get_local 4
                i32.load offset=8
                tee_local 6
                get_local 9
                i32.store offset=12
                get_local 9
                get_local 6
                i32.store offset=8
                get_local 7
                br_if 2 (;@4;)
                br 3 (;@3;)
              end
              get_local 0
              get_local 0
              i32.load
              i32.const -2
              get_local 9
              i32.const 3
              i32.shr_u
              i32.rotl
              i32.and
              i32.store
              br 2 (;@3;)
            end
            block  ;; label = @5
              get_local 4
              i32.const 20
              i32.add
              get_local 4
              i32.const 16
              i32.add
              get_local 4
              i32.load offset=20
              select
              tee_local 6
              i32.load
              tee_local 9
              i32.eqz
              br_if 0 (;@5;)
              loop  ;; label = @6
                get_local 6
                set_local 5
                get_local 9
                i32.const 20
                i32.add
                tee_local 6
                get_local 9
                i32.const 16
                i32.add
                get_local 6
                i32.load
                select
                tee_local 6
                i32.load
                tee_local 9
                br_if 0 (;@6;)
              end
              get_local 5
              i32.load
              set_local 9
              get_local 5
              i32.const 0
              i32.store
              get_local 7
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            i32.const 0
            set_local 9
            get_local 7
            i32.eqz
            br_if 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 0
                get_local 4
                i32.load offset=28
                tee_local 5
                i32.const 2
                i32.shl
                i32.add
                i32.const 272
                i32.add
                tee_local 6
                i32.load
                get_local 4
                i32.eq
                br_if 0 (;@6;)
                get_local 7
                i32.const 16
                i32.add
                get_local 7
                i32.load offset=16
                get_local 4
                i32.ne
                i32.const 2
                i32.shl
                i32.add
                get_local 9
                i32.store
                get_local 9
                br_if 1 (;@5;)
                br 3 (;@3;)
              end
              get_local 6
              get_local 9
              i32.store
              get_local 9
              i32.eqz
              br_if 1 (;@4;)
            end
            get_local 9
            get_local 7
            i32.store offset=24
            block  ;; label = @5
              get_local 4
              i32.load offset=16
              tee_local 6
              i32.eqz
              br_if 0 (;@5;)
              get_local 9
              get_local 6
              i32.store offset=16
              get_local 6
              get_local 9
              i32.store offset=24
            end
            get_local 4
            i32.load offset=20
            tee_local 6
            i32.eqz
            br_if 1 (;@3;)
            get_local 9
            i32.const 20
            i32.add
            get_local 6
            i32.store
            get_local 6
            get_local 9
            i32.store offset=24
            br 1 (;@3;)
          end
          get_local 0
          get_local 0
          i32.load offset=4
          i32.const -2
          get_local 5
          i32.rotl
          i32.and
          i32.store offset=4
        end
        get_local 10
        get_local 3
        i32.add
        set_local 3
        get_local 4
        get_local 10
        i32.add
        set_local 4
      end
      get_local 4
      get_local 4
      i32.load offset=4
      i32.const -2
      i32.and
      i32.store offset=4
      get_local 1
      get_local 3
      i32.const 1
      i32.or
      i32.store offset=4
      get_local 1
      get_local 3
      i32.add
      get_local 3
      i32.store
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 3
                i32.const 255
                i32.gt_u
                br_if 0 (;@6;)
                get_local 0
                get_local 3
                i32.const 3
                i32.shr_u
                tee_local 4
                i32.const 3
                i32.shl
                i32.add
                i32.const 8
                i32.add
                set_local 3
                get_local 0
                i32.load
                tee_local 9
                i32.const 1
                get_local 4
                i32.const 31
                i32.and
                i32.shl
                tee_local 4
                i32.and
                i32.eqz
                br_if 1 (;@5;)
                get_local 3
                i32.const 8
                i32.add
                set_local 9
                get_local 3
                i32.load offset=8
                set_local 4
                br 2 (;@4;)
              end
              get_local 3
              i32.const 8
              i32.shr_u
              tee_local 9
              i32.eqz
              br_if 2 (;@3;)
              i32.const 31
              set_local 4
              get_local 3
              i32.const 16777215
              i32.gt_u
              br_if 3 (;@2;)
              get_local 3
              i32.const 38
              get_local 9
              i32.clz
              tee_local 4
              i32.sub
              i32.const 31
              i32.and
              i32.shr_u
              i32.const 1
              i32.and
              i32.const 31
              get_local 4
              i32.sub
              i32.const 1
              i32.shl
              i32.or
              set_local 4
              br 3 (;@2;)
            end
            get_local 0
            get_local 9
            get_local 4
            i32.or
            i32.store
            get_local 3
            i32.const 8
            i32.add
            set_local 9
            get_local 3
            set_local 4
          end
          get_local 9
          get_local 1
          i32.store
          get_local 4
          get_local 1
          i32.store offset=12
          get_local 1
          get_local 3
          i32.store offset=12
          get_local 1
          get_local 4
          i32.store offset=8
          br 2 (;@1;)
        end
        i32.const 0
        set_local 4
      end
      get_local 1
      get_local 4
      i32.store offset=28
      get_local 1
      i64.const 0
      i64.store offset=16 align=4
      get_local 0
      get_local 4
      i32.const 2
      i32.shl
      i32.add
      i32.const 272
      i32.add
      set_local 9
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 0
                i32.load offset=4
                tee_local 6
                i32.const 1
                get_local 4
                i32.const 31
                i32.and
                i32.shl
                tee_local 5
                i32.and
                i32.eqz
                br_if 0 (;@6;)
                get_local 9
                i32.load
                tee_local 6
                i32.load offset=4
                i32.const -8
                i32.and
                get_local 3
                i32.ne
                br_if 1 (;@5;)
                get_local 6
                set_local 4
                br 2 (;@4;)
              end
              get_local 0
              i32.const 4
              i32.add
              get_local 6
              get_local 5
              i32.or
              i32.store
              get_local 1
              get_local 9
              i32.store offset=24
              get_local 9
              get_local 1
              i32.store
              br 3 (;@2;)
            end
            get_local 3
            i32.const 0
            i32.const 25
            get_local 4
            i32.const 1
            i32.shr_u
            i32.sub
            i32.const 31
            i32.and
            get_local 4
            i32.const 31
            i32.eq
            select
            i32.shl
            set_local 9
            loop  ;; label = @5
              get_local 6
              get_local 9
              i32.const 29
              i32.shr_u
              i32.const 4
              i32.and
              i32.add
              i32.const 16
              i32.add
              tee_local 5
              i32.load
              tee_local 4
              i32.eqz
              br_if 2 (;@3;)
              get_local 9
              i32.const 1
              i32.shl
              set_local 9
              get_local 4
              set_local 6
              get_local 4
              i32.load offset=4
              i32.const -8
              i32.and
              get_local 3
              i32.ne
              br_if 0 (;@5;)
            end
          end
          get_local 4
          i32.load offset=8
          tee_local 3
          get_local 1
          i32.store offset=12
          get_local 4
          get_local 1
          i32.store offset=8
          get_local 1
          get_local 4
          i32.store offset=12
          get_local 1
          get_local 3
          i32.store offset=8
          get_local 1
          i32.const 0
          i32.store offset=24
          br 2 (;@1;)
        end
        get_local 5
        get_local 1
        i32.store
        get_local 1
        get_local 6
        i32.store offset=24
      end
      get_local 1
      get_local 1
      i32.store offset=12
      get_local 1
      get_local 1
      i32.store offset=8
    end
    get_local 8
    i32.const 8
    i32.add)
  (func (;58;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    i32.const 0
    set_local 11
    block  ;; label = @1
      get_local 2
      i32.const -65
      i32.gt_u
      br_if 0 (;@1;)
      i32.const 16
      get_local 2
      i32.const 11
      i32.add
      i32.const -8
      i32.and
      get_local 2
      i32.const 11
      i32.lt_u
      select
      set_local 3
      get_local 1
      i32.const -4
      i32.add
      tee_local 5
      i32.load
      tee_local 6
      i32.const -8
      i32.and
      set_local 7
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
                              get_local 6
                              i32.const 3
                              i32.and
                              i32.eqz
                              br_if 0 (;@13;)
                              get_local 1
                              i32.const -8
                              i32.add
                              set_local 4
                              get_local 7
                              get_local 3
                              i32.ge_u
                              br_if 1 (;@12;)
                              get_local 4
                              get_local 7
                              i32.add
                              tee_local 8
                              get_local 0
                              i32.load offset=412
                              i32.eq
                              br_if 2 (;@11;)
                              get_local 8
                              get_local 0
                              i32.load offset=408
                              i32.eq
                              br_if 3 (;@10;)
                              get_local 8
                              i32.load offset=4
                              tee_local 6
                              i32.const 2
                              i32.and
                              br_if 4 (;@9;)
                              get_local 6
                              i32.const -8
                              i32.and
                              tee_local 9
                              get_local 7
                              i32.add
                              tee_local 7
                              get_local 3
                              i32.lt_u
                              br_if 4 (;@9;)
                              get_local 7
                              get_local 3
                              i32.sub
                              set_local 10
                              get_local 9
                              i32.const 255
                              i32.gt_u
                              br_if 6 (;@7;)
                              get_local 8
                              i32.load offset=12
                              tee_local 2
                              get_local 8
                              i32.load offset=8
                              tee_local 11
                              i32.eq
                              br_if 7 (;@6;)
                              get_local 11
                              get_local 2
                              i32.store offset=12
                              get_local 2
                              get_local 11
                              i32.store offset=8
                              br 10 (;@3;)
                            end
                            get_local 3
                            i32.const 256
                            i32.lt_u
                            br_if 3 (;@9;)
                            get_local 7
                            get_local 3
                            i32.const 4
                            i32.or
                            i32.lt_u
                            br_if 3 (;@9;)
                            get_local 7
                            get_local 3
                            i32.sub
                            i32.const 131073
                            i32.lt_u
                            br_if 10 (;@2;)
                            br 3 (;@9;)
                          end
                          get_local 7
                          get_local 3
                          i32.sub
                          tee_local 2
                          i32.const 16
                          i32.lt_u
                          br_if 9 (;@2;)
                          get_local 5
                          get_local 3
                          get_local 6
                          i32.const 1
                          i32.and
                          i32.or
                          i32.const 2
                          i32.or
                          i32.store
                          get_local 4
                          get_local 3
                          i32.add
                          tee_local 11
                          get_local 2
                          i32.const 3
                          i32.or
                          i32.store offset=4
                          get_local 11
                          get_local 2
                          i32.add
                          tee_local 3
                          get_local 3
                          i32.load offset=4
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          get_local 0
                          get_local 11
                          get_local 2
                          call 59
                          br 9 (;@2;)
                        end
                        get_local 0
                        i32.load offset=404
                        get_local 7
                        i32.add
                        tee_local 7
                        get_local 3
                        i32.le_u
                        br_if 1 (;@9;)
                        get_local 5
                        get_local 3
                        get_local 6
                        i32.const 1
                        i32.and
                        i32.or
                        i32.const 2
                        i32.or
                        i32.store
                        get_local 0
                        i32.const 412
                        i32.add
                        get_local 4
                        get_local 3
                        i32.add
                        tee_local 2
                        i32.store
                        get_local 0
                        i32.const 404
                        i32.add
                        get_local 7
                        get_local 3
                        i32.sub
                        tee_local 11
                        i32.store
                        get_local 2
                        get_local 11
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        br 8 (;@2;)
                      end
                      get_local 0
                      i32.load offset=400
                      get_local 7
                      i32.add
                      tee_local 7
                      get_local 3
                      i32.ge_u
                      br_if 1 (;@8;)
                    end
                    get_local 0
                    get_local 2
                    call 57
                    tee_local 3
                    i32.eqz
                    br_if 7 (;@1;)
                    get_local 3
                    get_local 1
                    get_local 5
                    i32.load
                    tee_local 11
                    i32.const -8
                    i32.and
                    i32.const 4
                    i32.const 8
                    get_local 11
                    i32.const 3
                    i32.and
                    select
                    i32.sub
                    tee_local 11
                    get_local 2
                    get_local 11
                    get_local 2
                    i32.le_u
                    select
                    call 56
                    set_local 2
                    get_local 0
                    get_local 1
                    call 60
                    get_local 2
                    return
                  end
                  block  ;; label = @8
                    block  ;; label = @9
                      get_local 7
                      get_local 3
                      i32.sub
                      tee_local 2
                      i32.const 16
                      i32.ge_u
                      br_if 0 (;@9;)
                      get_local 5
                      get_local 6
                      i32.const 1
                      i32.and
                      get_local 7
                      i32.or
                      i32.const 2
                      i32.or
                      i32.store
                      get_local 4
                      get_local 7
                      i32.add
                      tee_local 2
                      get_local 2
                      i32.load offset=4
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      i32.const 0
                      set_local 2
                      i32.const 0
                      set_local 11
                      br 1 (;@8;)
                    end
                    get_local 5
                    get_local 3
                    get_local 6
                    i32.const 1
                    i32.and
                    i32.or
                    i32.const 2
                    i32.or
                    i32.store
                    get_local 4
                    get_local 3
                    i32.add
                    tee_local 11
                    get_local 2
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    get_local 11
                    get_local 2
                    i32.add
                    tee_local 3
                    get_local 2
                    i32.store
                    get_local 3
                    get_local 3
                    i32.load offset=4
                    i32.const -2
                    i32.and
                    i32.store offset=4
                  end
                  get_local 0
                  i32.const 408
                  i32.add
                  get_local 11
                  i32.store
                  get_local 0
                  i32.const 400
                  i32.add
                  get_local 2
                  i32.store
                  br 5 (;@2;)
                end
                get_local 8
                i32.load offset=24
                set_local 9
                get_local 8
                i32.load offset=12
                tee_local 2
                get_local 8
                i32.eq
                br_if 1 (;@5;)
                get_local 8
                i32.load offset=8
                tee_local 11
                get_local 2
                i32.store offset=12
                get_local 2
                get_local 11
                i32.store offset=8
                get_local 9
                br_if 2 (;@4;)
                br 3 (;@3;)
              end
              get_local 0
              get_local 0
              i32.load
              i32.const -2
              get_local 6
              i32.const 3
              i32.shr_u
              i32.rotl
              i32.and
              i32.store
              br 2 (;@3;)
            end
            block  ;; label = @5
              get_local 8
              i32.const 20
              i32.add
              get_local 8
              i32.const 16
              i32.add
              get_local 8
              i32.load offset=20
              select
              tee_local 11
              i32.load
              tee_local 2
              i32.eqz
              br_if 0 (;@5;)
              loop  ;; label = @6
                get_local 11
                set_local 6
                get_local 2
                i32.const 20
                i32.add
                tee_local 11
                get_local 2
                i32.const 16
                i32.add
                get_local 11
                i32.load
                select
                tee_local 11
                i32.load
                tee_local 2
                br_if 0 (;@6;)
              end
              get_local 6
              i32.load
              set_local 2
              get_local 6
              i32.const 0
              i32.store
              get_local 9
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            i32.const 0
            set_local 2
            get_local 9
            i32.eqz
            br_if 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 0
                get_local 8
                i32.load offset=28
                tee_local 6
                i32.const 2
                i32.shl
                i32.add
                i32.const 272
                i32.add
                tee_local 11
                i32.load
                get_local 8
                i32.eq
                br_if 0 (;@6;)
                get_local 9
                i32.const 16
                i32.add
                get_local 9
                i32.load offset=16
                get_local 8
                i32.ne
                i32.const 2
                i32.shl
                i32.add
                get_local 2
                i32.store
                get_local 2
                br_if 1 (;@5;)
                br 3 (;@3;)
              end
              get_local 11
              get_local 2
              i32.store
              get_local 2
              i32.eqz
              br_if 1 (;@4;)
            end
            get_local 2
            get_local 9
            i32.store offset=24
            block  ;; label = @5
              get_local 8
              i32.load offset=16
              tee_local 11
              i32.eqz
              br_if 0 (;@5;)
              get_local 2
              get_local 11
              i32.store offset=16
              get_local 11
              get_local 2
              i32.store offset=24
            end
            get_local 8
            i32.load offset=20
            tee_local 11
            i32.eqz
            br_if 1 (;@3;)
            get_local 2
            i32.const 20
            i32.add
            get_local 11
            i32.store
            get_local 11
            get_local 2
            i32.store offset=24
            br 1 (;@3;)
          end
          get_local 0
          get_local 0
          i32.load offset=4
          i32.const -2
          get_local 6
          i32.rotl
          i32.and
          i32.store offset=4
        end
        block  ;; label = @3
          get_local 10
          i32.const 15
          i32.gt_u
          br_if 0 (;@3;)
          get_local 5
          get_local 7
          get_local 5
          i32.load
          i32.const 1
          i32.and
          i32.or
          i32.const 2
          i32.or
          i32.store
          get_local 4
          get_local 7
          i32.add
          tee_local 2
          get_local 2
          i32.load offset=4
          i32.const 1
          i32.or
          i32.store offset=4
          br 1 (;@2;)
        end
        get_local 5
        get_local 3
        get_local 5
        i32.load
        i32.const 1
        i32.and
        i32.or
        i32.const 2
        i32.or
        i32.store
        get_local 4
        get_local 3
        i32.add
        tee_local 2
        get_local 10
        i32.const 3
        i32.or
        i32.store offset=4
        get_local 2
        get_local 10
        i32.add
        tee_local 11
        get_local 11
        i32.load offset=4
        i32.const 1
        i32.or
        i32.store offset=4
        get_local 0
        get_local 2
        get_local 10
        call 59
      end
      get_local 1
      set_local 11
    end
    get_local 11)
  (func (;59;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    get_local 1
    get_local 2
    i32.add
    set_local 3
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            get_local 1
            i32.load offset=4
            tee_local 7
            i32.const 1
            i32.and
            br_if 0 (;@4;)
            get_local 7
            i32.const 3
            i32.and
            i32.eqz
            br_if 1 (;@3;)
            get_local 1
            i32.load
            tee_local 7
            get_local 2
            i32.add
            set_local 2
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      get_local 1
                      get_local 7
                      i32.sub
                      tee_local 1
                      get_local 0
                      i32.load offset=408
                      i32.eq
                      br_if 0 (;@9;)
                      get_local 7
                      i32.const 255
                      i32.gt_u
                      br_if 1 (;@8;)
                      get_local 1
                      i32.load offset=12
                      tee_local 6
                      get_local 1
                      i32.load offset=8
                      tee_local 5
                      i32.eq
                      br_if 2 (;@7;)
                      get_local 5
                      get_local 6
                      i32.store offset=12
                      get_local 6
                      get_local 5
                      i32.store offset=8
                      br 5 (;@4;)
                    end
                    get_local 3
                    i32.load offset=4
                    tee_local 7
                    i32.const 3
                    i32.and
                    i32.const 3
                    i32.ne
                    br_if 4 (;@4;)
                    get_local 3
                    i32.const 4
                    i32.add
                    get_local 7
                    i32.const -2
                    i32.and
                    i32.store
                    get_local 1
                    get_local 2
                    i32.const 1
                    i32.or
                    i32.store offset=4
                    get_local 0
                    get_local 2
                    i32.store offset=400
                    get_local 1
                    get_local 2
                    i32.add
                    get_local 2
                    i32.store
                    return
                  end
                  get_local 1
                  i32.load offset=24
                  set_local 4
                  get_local 1
                  i32.load offset=12
                  tee_local 7
                  get_local 1
                  i32.eq
                  br_if 1 (;@6;)
                  get_local 1
                  i32.load offset=8
                  tee_local 6
                  get_local 7
                  i32.store offset=12
                  get_local 7
                  get_local 6
                  i32.store offset=8
                  get_local 4
                  br_if 2 (;@5;)
                  br 3 (;@4;)
                end
                get_local 0
                get_local 0
                i32.load
                i32.const -2
                get_local 7
                i32.const 3
                i32.shr_u
                i32.rotl
                i32.and
                i32.store
                br 2 (;@4;)
              end
              block  ;; label = @6
                get_local 1
                i32.const 20
                i32.add
                get_local 1
                i32.const 16
                i32.add
                get_local 1
                i32.load offset=20
                select
                tee_local 6
                i32.load
                tee_local 7
                i32.eqz
                br_if 0 (;@6;)
                loop  ;; label = @7
                  get_local 6
                  set_local 5
                  get_local 7
                  i32.const 20
                  i32.add
                  tee_local 6
                  get_local 7
                  i32.const 16
                  i32.add
                  get_local 6
                  i32.load
                  select
                  tee_local 6
                  i32.load
                  tee_local 7
                  br_if 0 (;@7;)
                end
                get_local 5
                i32.load
                set_local 7
                get_local 5
                i32.const 0
                i32.store
                get_local 4
                br_if 1 (;@5;)
                br 2 (;@4;)
              end
              i32.const 0
              set_local 7
              get_local 4
              i32.eqz
              br_if 1 (;@4;)
            end
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 0
                  get_local 1
                  i32.load offset=28
                  tee_local 5
                  i32.const 2
                  i32.shl
                  i32.add
                  i32.const 272
                  i32.add
                  tee_local 6
                  i32.load
                  get_local 1
                  i32.eq
                  br_if 0 (;@7;)
                  get_local 4
                  i32.const 16
                  i32.add
                  get_local 4
                  i32.load offset=16
                  get_local 1
                  i32.ne
                  i32.const 2
                  i32.shl
                  i32.add
                  get_local 7
                  i32.store
                  get_local 7
                  br_if 1 (;@6;)
                  br 3 (;@4;)
                end
                get_local 6
                get_local 7
                i32.store
                get_local 7
                i32.eqz
                br_if 1 (;@5;)
              end
              get_local 7
              get_local 4
              i32.store offset=24
              block  ;; label = @6
                get_local 1
                i32.load offset=16
                tee_local 6
                i32.eqz
                br_if 0 (;@6;)
                get_local 7
                get_local 6
                i32.store offset=16
                get_local 6
                get_local 7
                i32.store offset=24
              end
              get_local 1
              i32.load offset=20
              tee_local 6
              i32.eqz
              br_if 1 (;@4;)
              get_local 7
              i32.const 20
              i32.add
              get_local 6
              i32.store
              get_local 6
              get_local 7
              i32.store offset=24
              br 1 (;@4;)
            end
            get_local 0
            get_local 0
            i32.load offset=4
            i32.const -2
            get_local 5
            i32.rotl
            i32.and
            i32.store offset=4
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          block  ;; label = @12
                            get_local 3
                            i32.load offset=4
                            tee_local 7
                            i32.const 2
                            i32.and
                            br_if 0 (;@12;)
                            get_local 3
                            get_local 0
                            i32.load offset=412
                            i32.eq
                            br_if 1 (;@11;)
                            get_local 3
                            get_local 0
                            i32.load offset=408
                            i32.eq
                            br_if 2 (;@10;)
                            get_local 7
                            i32.const -8
                            i32.and
                            tee_local 6
                            get_local 2
                            i32.add
                            set_local 2
                            get_local 6
                            i32.const 255
                            i32.gt_u
                            br_if 3 (;@9;)
                            get_local 3
                            i32.load offset=12
                            tee_local 6
                            get_local 3
                            i32.load offset=8
                            tee_local 3
                            i32.eq
                            br_if 4 (;@8;)
                            get_local 3
                            get_local 6
                            i32.store offset=12
                            get_local 6
                            get_local 3
                            i32.store offset=8
                            br 7 (;@5;)
                          end
                          get_local 3
                          i32.const 4
                          i32.add
                          get_local 7
                          i32.const -2
                          i32.and
                          i32.store
                          get_local 1
                          get_local 2
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          get_local 1
                          get_local 2
                          i32.add
                          get_local 2
                          i32.store
                          br 7 (;@4;)
                        end
                        get_local 0
                        i32.const 412
                        i32.add
                        get_local 1
                        i32.store
                        get_local 0
                        get_local 0
                        i32.load offset=404
                        get_local 2
                        i32.add
                        tee_local 2
                        i32.store offset=404
                        get_local 1
                        get_local 2
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        get_local 1
                        get_local 0
                        i32.load offset=408
                        i32.ne
                        br_if 7 (;@3;)
                        get_local 0
                        i32.const 0
                        i32.store offset=400
                        get_local 0
                        i32.const 408
                        i32.add
                        i32.const 0
                        i32.store
                        return
                      end
                      get_local 1
                      get_local 0
                      i32.load offset=400
                      get_local 2
                      i32.add
                      tee_local 2
                      i32.const 1
                      i32.or
                      i32.store offset=4
                      get_local 0
                      i32.const 408
                      i32.add
                      get_local 1
                      i32.store
                      get_local 0
                      get_local 2
                      i32.store offset=400
                      get_local 1
                      get_local 2
                      i32.add
                      get_local 2
                      i32.store
                      return
                    end
                    get_local 3
                    i32.load offset=24
                    set_local 4
                    get_local 3
                    i32.load offset=12
                    tee_local 7
                    get_local 3
                    i32.eq
                    br_if 1 (;@7;)
                    get_local 3
                    i32.load offset=8
                    tee_local 6
                    get_local 7
                    i32.store offset=12
                    get_local 7
                    get_local 6
                    i32.store offset=8
                    get_local 4
                    br_if 2 (;@6;)
                    br 3 (;@5;)
                  end
                  get_local 0
                  get_local 0
                  i32.load
                  i32.const -2
                  get_local 7
                  i32.const 3
                  i32.shr_u
                  i32.rotl
                  i32.and
                  i32.store
                  br 2 (;@5;)
                end
                block  ;; label = @7
                  get_local 3
                  i32.const 20
                  i32.add
                  get_local 3
                  i32.const 16
                  i32.add
                  get_local 3
                  i32.load offset=20
                  select
                  tee_local 6
                  i32.load
                  tee_local 7
                  i32.eqz
                  br_if 0 (;@7;)
                  loop  ;; label = @8
                    get_local 6
                    set_local 5
                    get_local 7
                    i32.const 20
                    i32.add
                    tee_local 6
                    get_local 7
                    i32.const 16
                    i32.add
                    get_local 6
                    i32.load
                    select
                    tee_local 6
                    i32.load
                    tee_local 7
                    br_if 0 (;@8;)
                  end
                  get_local 5
                  i32.load
                  set_local 7
                  get_local 5
                  i32.const 0
                  i32.store
                  get_local 4
                  br_if 1 (;@6;)
                  br 2 (;@5;)
                end
                i32.const 0
                set_local 7
                get_local 4
                i32.eqz
                br_if 1 (;@5;)
              end
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    get_local 0
                    get_local 3
                    i32.load offset=28
                    tee_local 5
                    i32.const 2
                    i32.shl
                    i32.add
                    i32.const 272
                    i32.add
                    tee_local 6
                    i32.load
                    get_local 3
                    i32.eq
                    br_if 0 (;@8;)
                    get_local 4
                    i32.const 16
                    i32.add
                    get_local 4
                    i32.load offset=16
                    get_local 3
                    i32.ne
                    i32.const 2
                    i32.shl
                    i32.add
                    get_local 7
                    i32.store
                    get_local 7
                    br_if 1 (;@7;)
                    br 3 (;@5;)
                  end
                  get_local 6
                  get_local 7
                  i32.store
                  get_local 7
                  i32.eqz
                  br_if 1 (;@6;)
                end
                get_local 7
                get_local 4
                i32.store offset=24
                block  ;; label = @7
                  get_local 3
                  i32.load offset=16
                  tee_local 6
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 7
                  get_local 6
                  i32.store offset=16
                  get_local 6
                  get_local 7
                  i32.store offset=24
                end
                get_local 3
                i32.load offset=20
                tee_local 3
                i32.eqz
                br_if 1 (;@5;)
                get_local 7
                i32.const 20
                i32.add
                get_local 3
                i32.store
                get_local 3
                get_local 7
                i32.store offset=24
                br 1 (;@5;)
              end
              get_local 0
              get_local 0
              i32.load offset=4
              i32.const -2
              get_local 5
              i32.rotl
              i32.and
              i32.store offset=4
            end
            get_local 1
            get_local 2
            i32.const 1
            i32.or
            i32.store offset=4
            get_local 1
            get_local 2
            i32.add
            get_local 2
            i32.store
            get_local 1
            get_local 0
            i32.const 408
            i32.add
            i32.load
            i32.ne
            br_if 0 (;@4;)
            get_local 0
            get_local 2
            i32.store offset=400
            return
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    get_local 2
                    i32.const 255
                    i32.gt_u
                    br_if 0 (;@8;)
                    get_local 0
                    get_local 2
                    i32.const 3
                    i32.shr_u
                    tee_local 3
                    i32.const 3
                    i32.shl
                    i32.add
                    i32.const 8
                    i32.add
                    set_local 2
                    get_local 0
                    i32.load
                    tee_local 7
                    i32.const 1
                    get_local 3
                    i32.const 31
                    i32.and
                    i32.shl
                    tee_local 3
                    i32.and
                    i32.eqz
                    br_if 1 (;@7;)
                    get_local 2
                    i32.load offset=8
                    set_local 0
                    br 2 (;@6;)
                  end
                  get_local 2
                  i32.const 8
                  i32.shr_u
                  tee_local 7
                  i32.eqz
                  br_if 2 (;@5;)
                  i32.const 31
                  set_local 3
                  get_local 2
                  i32.const 16777215
                  i32.gt_u
                  br_if 3 (;@4;)
                  get_local 2
                  i32.const 38
                  get_local 7
                  i32.clz
                  tee_local 3
                  i32.sub
                  i32.const 31
                  i32.and
                  i32.shr_u
                  i32.const 1
                  i32.and
                  i32.const 31
                  get_local 3
                  i32.sub
                  i32.const 1
                  i32.shl
                  i32.or
                  set_local 3
                  br 3 (;@4;)
                end
                get_local 0
                get_local 7
                get_local 3
                i32.or
                i32.store
                get_local 2
                set_local 0
              end
              get_local 2
              i32.const 8
              i32.add
              get_local 1
              i32.store
              get_local 0
              get_local 1
              i32.store offset=12
              get_local 1
              get_local 2
              i32.store offset=12
              get_local 1
              get_local 0
              i32.store offset=8
              return
            end
            i32.const 0
            set_local 3
          end
          get_local 1
          i64.const 0
          i64.store offset=16 align=4
          get_local 1
          i32.const 28
          i32.add
          get_local 3
          i32.store
          get_local 0
          get_local 3
          i32.const 2
          i32.shl
          i32.add
          i32.const 272
          i32.add
          set_local 7
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 0
                i32.load offset=4
                tee_local 6
                i32.const 1
                get_local 3
                i32.const 31
                i32.and
                i32.shl
                tee_local 5
                i32.and
                i32.eqz
                br_if 0 (;@6;)
                get_local 7
                i32.load
                tee_local 7
                i32.load offset=4
                i32.const -8
                i32.and
                get_local 2
                i32.ne
                br_if 1 (;@5;)
                get_local 7
                set_local 0
                br 2 (;@4;)
              end
              get_local 0
              i32.const 4
              i32.add
              get_local 6
              get_local 5
              i32.or
              i32.store
              get_local 1
              i32.const 24
              i32.add
              get_local 7
              i32.store
              get_local 7
              get_local 1
              i32.store
              br 4 (;@1;)
            end
            get_local 2
            i32.const 0
            i32.const 25
            get_local 3
            i32.const 1
            i32.shr_u
            i32.sub
            i32.const 31
            i32.and
            get_local 3
            i32.const 31
            i32.eq
            select
            i32.shl
            set_local 3
            loop  ;; label = @5
              get_local 7
              get_local 3
              i32.const 29
              i32.shr_u
              i32.const 4
              i32.and
              i32.add
              i32.const 16
              i32.add
              tee_local 6
              i32.load
              tee_local 0
              i32.eqz
              br_if 3 (;@2;)
              get_local 3
              i32.const 1
              i32.shl
              set_local 3
              get_local 0
              set_local 7
              get_local 0
              i32.load offset=4
              i32.const -8
              i32.and
              get_local 2
              i32.ne
              br_if 0 (;@5;)
            end
          end
          get_local 0
          i32.load offset=8
          tee_local 2
          get_local 1
          i32.store offset=12
          get_local 0
          get_local 1
          i32.store offset=8
          get_local 1
          get_local 0
          i32.store offset=12
          get_local 1
          get_local 2
          i32.store offset=8
          get_local 1
          i32.const 24
          i32.add
          i32.const 0
          i32.store
        end
        return
      end
      get_local 6
      get_local 1
      i32.store
      get_local 1
      i32.const 24
      i32.add
      get_local 7
      i32.store
    end
    get_local 1
    get_local 1
    i32.store offset=12
    get_local 1
    get_local 1
    i32.store offset=8)
  (func (;60;) (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    get_local 1
    i32.const -8
    i32.add
    tee_local 4
    get_local 1
    i32.const -4
    i32.add
    i32.load
    tee_local 6
    i32.const -8
    i32.and
    tee_local 1
    i32.add
    set_local 7
    block  ;; label = @1
      block  ;; label = @2
        get_local 6
        i32.const 1
        i32.and
        br_if 0 (;@2;)
        get_local 6
        i32.const 3
        i32.and
        i32.eqz
        br_if 1 (;@1;)
        get_local 4
        i32.load
        tee_local 6
        get_local 1
        i32.add
        set_local 1
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 4
                  get_local 6
                  i32.sub
                  tee_local 4
                  get_local 0
                  i32.load offset=408
                  i32.eq
                  br_if 0 (;@7;)
                  get_local 6
                  i32.const 255
                  i32.gt_u
                  br_if 1 (;@6;)
                  get_local 4
                  i32.load offset=12
                  tee_local 5
                  get_local 4
                  i32.load offset=8
                  tee_local 3
                  i32.eq
                  br_if 2 (;@5;)
                  get_local 3
                  get_local 5
                  i32.store offset=12
                  get_local 5
                  get_local 3
                  i32.store offset=8
                  br 5 (;@2;)
                end
                get_local 7
                i32.load offset=4
                tee_local 6
                i32.const 3
                i32.and
                i32.const 3
                i32.ne
                br_if 4 (;@2;)
                get_local 7
                i32.const 4
                i32.add
                get_local 6
                i32.const -2
                i32.and
                i32.store
                get_local 4
                get_local 1
                i32.const 1
                i32.or
                i32.store offset=4
                get_local 0
                get_local 1
                i32.store offset=400
                get_local 4
                get_local 1
                i32.add
                get_local 1
                i32.store
                return
              end
              get_local 4
              i32.load offset=24
              set_local 2
              get_local 4
              i32.load offset=12
              tee_local 6
              get_local 4
              i32.eq
              br_if 1 (;@4;)
              get_local 4
              i32.load offset=8
              tee_local 5
              get_local 6
              i32.store offset=12
              get_local 6
              get_local 5
              i32.store offset=8
              get_local 2
              br_if 2 (;@3;)
              br 3 (;@2;)
            end
            get_local 0
            get_local 0
            i32.load
            i32.const -2
            get_local 6
            i32.const 3
            i32.shr_u
            i32.rotl
            i32.and
            i32.store
            br 2 (;@2;)
          end
          block  ;; label = @4
            get_local 4
            i32.const 20
            i32.add
            get_local 4
            i32.const 16
            i32.add
            get_local 4
            i32.load offset=20
            select
            tee_local 5
            i32.load
            tee_local 6
            i32.eqz
            br_if 0 (;@4;)
            loop  ;; label = @5
              get_local 5
              set_local 3
              get_local 6
              i32.const 20
              i32.add
              tee_local 5
              get_local 6
              i32.const 16
              i32.add
              get_local 5
              i32.load
              select
              tee_local 5
              i32.load
              tee_local 6
              br_if 0 (;@5;)
            end
            get_local 3
            i32.load
            set_local 6
            get_local 3
            i32.const 0
            i32.store
            get_local 2
            br_if 1 (;@3;)
            br 2 (;@2;)
          end
          i32.const 0
          set_local 6
          get_local 2
          i32.eqz
          br_if 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 0
              get_local 4
              i32.load offset=28
              tee_local 3
              i32.const 2
              i32.shl
              i32.add
              i32.const 272
              i32.add
              tee_local 5
              i32.load
              get_local 4
              i32.eq
              br_if 0 (;@5;)
              get_local 2
              i32.const 16
              i32.add
              get_local 2
              i32.load offset=16
              get_local 4
              i32.ne
              i32.const 2
              i32.shl
              i32.add
              get_local 6
              i32.store
              get_local 6
              br_if 1 (;@4;)
              br 3 (;@2;)
            end
            get_local 5
            get_local 6
            i32.store
            get_local 6
            i32.eqz
            br_if 1 (;@3;)
          end
          get_local 6
          get_local 2
          i32.store offset=24
          block  ;; label = @4
            get_local 4
            i32.load offset=16
            tee_local 5
            i32.eqz
            br_if 0 (;@4;)
            get_local 6
            get_local 5
            i32.store offset=16
            get_local 5
            get_local 6
            i32.store offset=24
          end
          get_local 4
          i32.load offset=20
          tee_local 5
          i32.eqz
          br_if 1 (;@2;)
          get_local 6
          i32.const 20
          i32.add
          get_local 5
          i32.store
          get_local 5
          get_local 6
          i32.store offset=24
          br 1 (;@2;)
        end
        get_local 0
        get_local 0
        i32.load offset=4
        i32.const -2
        get_local 3
        i32.rotl
        i32.and
        i32.store offset=4
      end
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
                              get_local 7
                              i32.load offset=4
                              tee_local 6
                              i32.const 2
                              i32.and
                              br_if 0 (;@13;)
                              get_local 7
                              get_local 0
                              i32.load offset=412
                              i32.eq
                              br_if 1 (;@12;)
                              get_local 7
                              get_local 0
                              i32.load offset=408
                              i32.eq
                              br_if 2 (;@11;)
                              get_local 6
                              i32.const -8
                              i32.and
                              tee_local 5
                              get_local 1
                              i32.add
                              set_local 1
                              get_local 5
                              i32.const 255
                              i32.gt_u
                              br_if 3 (;@10;)
                              get_local 7
                              i32.load offset=12
                              tee_local 5
                              get_local 7
                              i32.load offset=8
                              tee_local 7
                              i32.eq
                              br_if 4 (;@9;)
                              get_local 7
                              get_local 5
                              i32.store offset=12
                              get_local 5
                              get_local 7
                              i32.store offset=8
                              br 10 (;@3;)
                            end
                            get_local 7
                            i32.const 4
                            i32.add
                            get_local 6
                            i32.const -2
                            i32.and
                            i32.store
                            get_local 4
                            get_local 1
                            i32.const 1
                            i32.or
                            i32.store offset=4
                            get_local 4
                            get_local 1
                            i32.add
                            get_local 1
                            i32.store
                            br 10 (;@2;)
                          end
                          get_local 0
                          i32.const 412
                          i32.add
                          get_local 4
                          i32.store
                          get_local 0
                          get_local 0
                          i32.load offset=404
                          get_local 1
                          i32.add
                          tee_local 1
                          i32.store offset=404
                          get_local 4
                          get_local 1
                          i32.const 1
                          i32.or
                          i32.store offset=4
                          block  ;; label = @12
                            get_local 4
                            get_local 0
                            i32.load offset=408
                            i32.ne
                            br_if 0 (;@12;)
                            get_local 0
                            i32.const 0
                            i32.store offset=400
                            get_local 0
                            i32.const 408
                            i32.add
                            i32.const 0
                            i32.store
                          end
                          get_local 0
                          i32.load offset=440
                          get_local 1
                          i32.ge_u
                          br_if 10 (;@1;)
                          block  ;; label = @12
                            get_local 1
                            i32.const 41
                            i32.lt_u
                            br_if 0 (;@12;)
                            get_local 0
                            i32.const 424
                            i32.add
                            set_local 1
                            loop  ;; label = @13
                              block  ;; label = @14
                                get_local 1
                                i32.load
                                tee_local 7
                                get_local 4
                                i32.gt_u
                                br_if 0 (;@14;)
                                get_local 7
                                get_local 1
                                i32.load offset=4
                                i32.add
                                get_local 4
                                i32.gt_u
                                br_if 2 (;@12;)
                              end
                              get_local 1
                              i32.load offset=8
                              tee_local 1
                              br_if 0 (;@13;)
                            end
                          end
                          get_local 0
                          i32.const 432
                          i32.add
                          i32.load
                          tee_local 1
                          i32.eqz
                          br_if 4 (;@7;)
                          i32.const 0
                          set_local 4
                          loop  ;; label = @12
                            get_local 4
                            i32.const 1
                            i32.add
                            set_local 4
                            get_local 1
                            i32.load offset=8
                            tee_local 1
                            br_if 0 (;@12;)
                            br 6 (;@6;)
                          end
                          unreachable
                        end
                        get_local 4
                        get_local 0
                        i32.load offset=400
                        get_local 1
                        i32.add
                        tee_local 1
                        i32.const 1
                        i32.or
                        i32.store offset=4
                        get_local 0
                        i32.const 408
                        i32.add
                        get_local 4
                        i32.store
                        get_local 0
                        get_local 1
                        i32.store offset=400
                        get_local 4
                        get_local 1
                        i32.add
                        get_local 1
                        i32.store
                        return
                      end
                      get_local 7
                      i32.load offset=24
                      set_local 2
                      get_local 7
                      i32.load offset=12
                      tee_local 6
                      get_local 7
                      i32.eq
                      br_if 1 (;@8;)
                      get_local 7
                      i32.load offset=8
                      tee_local 5
                      get_local 6
                      i32.store offset=12
                      get_local 6
                      get_local 5
                      i32.store offset=8
                      get_local 2
                      br_if 5 (;@4;)
                      br 6 (;@3;)
                    end
                    get_local 0
                    get_local 0
                    i32.load
                    i32.const -2
                    get_local 6
                    i32.const 3
                    i32.shr_u
                    i32.rotl
                    i32.and
                    i32.store
                    br 5 (;@3;)
                  end
                  get_local 7
                  i32.const 20
                  i32.add
                  get_local 7
                  i32.const 16
                  i32.add
                  get_local 7
                  i32.load offset=20
                  select
                  tee_local 5
                  i32.load
                  tee_local 6
                  i32.eqz
                  br_if 2 (;@5;)
                  loop  ;; label = @8
                    get_local 5
                    set_local 3
                    get_local 6
                    i32.const 20
                    i32.add
                    tee_local 5
                    get_local 6
                    i32.const 16
                    i32.add
                    get_local 5
                    i32.load
                    select
                    tee_local 5
                    i32.load
                    tee_local 6
                    br_if 0 (;@8;)
                  end
                  get_local 3
                  i32.load
                  set_local 6
                  get_local 3
                  i32.const 0
                  i32.store
                  get_local 2
                  br_if 3 (;@4;)
                  br 4 (;@3;)
                end
                i32.const 0
                set_local 4
              end
              get_local 0
              i32.const 440
              i32.add
              i32.const -1
              i32.store
              get_local 0
              get_local 4
              i32.const 4095
              get_local 4
              i32.const 4095
              i32.gt_u
              select
              i32.store offset=448
              return
            end
            i32.const 0
            set_local 6
            get_local 2
            i32.eqz
            br_if 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 0
                get_local 7
                i32.load offset=28
                tee_local 3
                i32.const 2
                i32.shl
                i32.add
                i32.const 272
                i32.add
                tee_local 5
                i32.load
                get_local 7
                i32.eq
                br_if 0 (;@6;)
                get_local 2
                i32.const 16
                i32.add
                get_local 2
                i32.load offset=16
                get_local 7
                i32.ne
                i32.const 2
                i32.shl
                i32.add
                get_local 6
                i32.store
                get_local 6
                br_if 1 (;@5;)
                br 3 (;@3;)
              end
              get_local 5
              get_local 6
              i32.store
              get_local 6
              i32.eqz
              br_if 1 (;@4;)
            end
            get_local 6
            get_local 2
            i32.store offset=24
            block  ;; label = @5
              get_local 7
              i32.load offset=16
              tee_local 5
              i32.eqz
              br_if 0 (;@5;)
              get_local 6
              get_local 5
              i32.store offset=16
              get_local 5
              get_local 6
              i32.store offset=24
            end
            get_local 7
            i32.load offset=20
            tee_local 7
            i32.eqz
            br_if 1 (;@3;)
            get_local 6
            i32.const 20
            i32.add
            get_local 7
            i32.store
            get_local 7
            get_local 6
            i32.store offset=24
            br 1 (;@3;)
          end
          get_local 0
          get_local 0
          i32.load offset=4
          i32.const -2
          get_local 3
          i32.rotl
          i32.and
          i32.store offset=4
        end
        get_local 4
        get_local 1
        i32.const 1
        i32.or
        i32.store offset=4
        get_local 4
        get_local 1
        i32.add
        get_local 1
        i32.store
        get_local 4
        get_local 0
        i32.const 408
        i32.add
        i32.load
        i32.ne
        br_if 0 (;@2;)
        get_local 0
        get_local 1
        i32.store offset=400
        return
      end
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 1
                i32.const 255
                i32.gt_u
                br_if 0 (;@6;)
                get_local 0
                get_local 1
                i32.const 3
                i32.shr_u
                tee_local 7
                i32.const 3
                i32.shl
                i32.add
                i32.const 8
                i32.add
                set_local 1
                get_local 0
                i32.load
                tee_local 6
                i32.const 1
                get_local 7
                i32.const 31
                i32.and
                i32.shl
                tee_local 7
                i32.and
                i32.eqz
                br_if 1 (;@5;)
                get_local 1
                i32.load offset=8
                set_local 0
                br 2 (;@4;)
              end
              get_local 1
              i32.const 8
              i32.shr_u
              tee_local 6
              i32.eqz
              br_if 2 (;@3;)
              i32.const 31
              set_local 7
              get_local 1
              i32.const 16777215
              i32.gt_u
              br_if 3 (;@2;)
              get_local 1
              i32.const 38
              get_local 6
              i32.clz
              tee_local 7
              i32.sub
              i32.const 31
              i32.and
              i32.shr_u
              i32.const 1
              i32.and
              i32.const 31
              get_local 7
              i32.sub
              i32.const 1
              i32.shl
              i32.or
              set_local 7
              br 3 (;@2;)
            end
            get_local 0
            get_local 6
            get_local 7
            i32.or
            i32.store
            get_local 1
            set_local 0
          end
          get_local 1
          i32.const 8
          i32.add
          get_local 4
          i32.store
          get_local 0
          get_local 4
          i32.store offset=12
          get_local 4
          get_local 1
          i32.store offset=12
          get_local 4
          get_local 0
          i32.store offset=8
          return
        end
        i32.const 0
        set_local 7
      end
      get_local 4
      i64.const 0
      i64.store offset=16 align=4
      get_local 4
      i32.const 28
      i32.add
      get_local 7
      i32.store
      get_local 0
      get_local 7
      i32.const 2
      i32.shl
      i32.add
      i32.const 272
      i32.add
      set_local 6
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 0
                  i32.load offset=4
                  tee_local 5
                  i32.const 1
                  get_local 7
                  i32.const 31
                  i32.and
                  i32.shl
                  tee_local 3
                  i32.and
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 6
                  i32.load
                  tee_local 5
                  i32.load offset=4
                  i32.const -8
                  i32.and
                  get_local 1
                  i32.ne
                  br_if 1 (;@6;)
                  get_local 5
                  set_local 7
                  br 2 (;@5;)
                end
                get_local 0
                i32.const 4
                i32.add
                get_local 5
                get_local 3
                i32.or
                i32.store
                get_local 4
                i32.const 24
                i32.add
                get_local 6
                i32.store
                get_local 6
                get_local 4
                i32.store
                br 3 (;@3;)
              end
              get_local 1
              i32.const 0
              i32.const 25
              get_local 7
              i32.const 1
              i32.shr_u
              i32.sub
              i32.const 31
              i32.and
              get_local 7
              i32.const 31
              i32.eq
              select
              i32.shl
              set_local 6
              loop  ;; label = @6
                get_local 5
                get_local 6
                i32.const 29
                i32.shr_u
                i32.const 4
                i32.and
                i32.add
                i32.const 16
                i32.add
                tee_local 3
                i32.load
                tee_local 7
                i32.eqz
                br_if 2 (;@4;)
                get_local 6
                i32.const 1
                i32.shl
                set_local 6
                get_local 7
                set_local 5
                get_local 7
                i32.load offset=4
                i32.const -8
                i32.and
                get_local 1
                i32.ne
                br_if 0 (;@6;)
              end
            end
            get_local 7
            i32.load offset=8
            tee_local 1
            get_local 4
            i32.store offset=12
            get_local 7
            get_local 4
            i32.store offset=8
            get_local 4
            get_local 7
            i32.store offset=12
            get_local 4
            get_local 1
            i32.store offset=8
            get_local 4
            i32.const 24
            i32.add
            i32.const 0
            i32.store
            br 2 (;@2;)
          end
          get_local 3
          get_local 4
          i32.store
          get_local 4
          i32.const 24
          i32.add
          get_local 5
          i32.store
        end
        get_local 4
        get_local 4
        i32.store offset=12
        get_local 4
        get_local 4
        i32.store offset=8
      end
      get_local 0
      get_local 0
      i32.load offset=448
      i32.const -1
      i32.add
      tee_local 4
      i32.store offset=448
      get_local 4
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          get_local 0
          i32.const 432
          i32.add
          i32.load
          tee_local 1
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          set_local 4
          loop  ;; label = @4
            get_local 4
            i32.const 1
            i32.add
            set_local 4
            get_local 1
            i32.load offset=8
            tee_local 1
            br_if 0 (;@4;)
            br 2 (;@2;)
          end
          unreachable
        end
        i32.const 0
        set_local 4
      end
      get_local 0
      i32.const 448
      i32.add
      get_local 4
      i32.const 4095
      get_local 4
      i32.const 4095
      i32.gt_u
      select
      i32.store
    end)
  (func (;61;) (type 4) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 7
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  get_local 0
                  i32.load offset=4
                  tee_local 6
                  get_local 0
                  i32.load offset=8
                  tee_local 3
                  i32.sub
                  get_local 2
                  i32.ge_u
                  br_if 0 (;@7;)
                  get_local 3
                  get_local 2
                  i32.add
                  tee_local 4
                  get_local 3
                  i32.lt_u
                  br_if 4 (;@3;)
                  get_local 4
                  get_local 6
                  i32.const 1
                  i32.shl
                  tee_local 5
                  get_local 4
                  get_local 5
                  i32.ge_u
                  select
                  tee_local 5
                  i32.const -1
                  i32.le_s
                  br_if 5 (;@2;)
                  get_local 6
                  i32.eqz
                  br_if 1 (;@6;)
                  i32.const 1744
                  get_local 0
                  i32.load
                  get_local 5
                  call 58
                  tee_local 6
                  br_if 2 (;@5;)
                  get_local 7
                  get_local 5
                  i32.store offset=8
                  get_local 7
                  get_local 6
                  i32.store offset=4
                  get_local 7
                  i32.const 1
                  i32.store offset=12
                  unreachable
                  unreachable
                end
                get_local 3
                get_local 2
                i32.add
                set_local 4
                get_local 0
                i32.load
                set_local 6
                br 2 (;@4;)
              end
              i32.const 1744
              get_local 5
              call 57
              tee_local 6
              i32.eqz
              br_if 4 (;@1;)
            end
            get_local 0
            get_local 6
            i32.store
            get_local 0
            i32.const 4
            i32.add
            get_local 5
            i32.store
          end
          get_local 0
          i32.const 8
          i32.add
          get_local 4
          i32.store
          get_local 6
          get_local 3
          i32.add
          get_local 1
          get_local 2
          call 56
          drop
          i32.const 0
          get_local 7
          i32.const 16
          i32.add
          i32.store offset=4
          return
        end
        i32.const 2224
        call 86
        unreachable
      end
      i32.const 2196
      call 87
      unreachable
    end
    get_local 7
    get_local 5
    i32.store offset=8
    get_local 7
    get_local 6
    i32.store offset=4
    get_local 7
    i32.const 1
    i32.store offset=12
    unreachable
    unreachable)
  (func (;62;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 5
    i32.store offset=4
    i32.const 39
    set_local 4
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 0
        i32.const 10000
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 39
        set_local 4
        loop  ;; label = @3
          get_local 5
          i32.const 9
          i32.add
          get_local 4
          i32.add
          tee_local 2
          i32.const -2
          i32.add
          get_local 0
          i32.const 10000
          i32.rem_u
          tee_local 3
          i32.const 100
          i32.rem_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 2
          i32.const -4
          i32.add
          get_local 3
          i32.const 100
          i32.div_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 4
          i32.const -4
          i32.add
          set_local 4
          get_local 0
          i32.const 99999999
          i32.gt_u
          set_local 2
          get_local 0
          i32.const 10000
          i32.div_u
          tee_local 3
          set_local 0
          get_local 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
        unreachable
      end
      get_local 0
      set_local 3
    end
    block  ;; label = @1
      get_local 3
      i32.const 100
      i32.lt_s
      br_if 0 (;@1;)
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 4
      i32.add
      get_local 3
      i32.const 100
      i32.rem_u
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
      get_local 3
      i32.const 100
      i32.div_u
      set_local 3
    end
    block  ;; label = @1
      block  ;; label = @2
        get_local 3
        i32.const 9
        i32.gt_s
        br_if 0 (;@2;)
        get_local 5
        i32.const 9
        i32.add
        get_local 4
        i32.const -1
        i32.add
        tee_local 0
        i32.add
        get_local 3
        i32.const 48
        i32.add
        i32.store8
        br 1 (;@1;)
      end
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 0
      i32.add
      get_local 3
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
    end
    get_local 1
    i32.const 1
    i32.const 2496
    i32.const 0
    get_local 5
    i32.const 9
    i32.add
    get_local 0
    i32.add
    i32.const 39
    get_local 0
    i32.sub
    call 70
    set_local 0
    i32.const 0
    get_local 5
    i32.const 48
    i32.add
    i32.store offset=4
    get_local 0)
  (func (;63;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 5
    i32.store offset=4
    i32.const 39
    set_local 4
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 0
        i32.const 10000
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 39
        set_local 4
        loop  ;; label = @3
          get_local 5
          i32.const 9
          i32.add
          get_local 4
          i32.add
          tee_local 2
          i32.const -2
          i32.add
          get_local 0
          i32.const 10000
          i32.rem_u
          tee_local 3
          i32.const 100
          i32.rem_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 2
          i32.const -4
          i32.add
          get_local 3
          i32.const 100
          i32.div_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 4
          i32.const -4
          i32.add
          set_local 4
          get_local 0
          i32.const 99999999
          i32.gt_u
          set_local 2
          get_local 0
          i32.const 10000
          i32.div_u
          tee_local 3
          set_local 0
          get_local 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
        unreachable
      end
      get_local 0
      set_local 3
    end
    block  ;; label = @1
      get_local 3
      i32.const 100
      i32.lt_s
      br_if 0 (;@1;)
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 4
      i32.add
      get_local 3
      i32.const 100
      i32.rem_u
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
      get_local 3
      i32.const 100
      i32.div_u
      set_local 3
    end
    block  ;; label = @1
      block  ;; label = @2
        get_local 3
        i32.const 9
        i32.gt_s
        br_if 0 (;@2;)
        get_local 5
        i32.const 9
        i32.add
        get_local 4
        i32.const -1
        i32.add
        tee_local 0
        i32.add
        get_local 3
        i32.const 48
        i32.add
        i32.store8
        br 1 (;@1;)
      end
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 0
      i32.add
      get_local 3
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
    end
    get_local 1
    i32.const 1
    i32.const 2496
    i32.const 0
    get_local 5
    i32.const 9
    i32.add
    get_local 0
    i32.add
    i32.const 39
    get_local 0
    i32.sub
    call 70
    set_local 0
    i32.const 0
    get_local 5
    i32.const 48
    i32.add
    i32.store offset=4
    get_local 0)
  (func (;64;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 5
    i32.store offset=4
    i32.const 39
    set_local 4
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 0
        i32.const 10000
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 39
        set_local 4
        loop  ;; label = @3
          get_local 5
          i32.const 9
          i32.add
          get_local 4
          i32.add
          tee_local 2
          i32.const -2
          i32.add
          get_local 0
          i32.const 10000
          i32.rem_u
          tee_local 3
          i32.const 100
          i32.rem_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 2
          i32.const -4
          i32.add
          get_local 3
          i32.const 100
          i32.div_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 4
          i32.const -4
          i32.add
          set_local 4
          get_local 0
          i32.const 99999999
          i32.gt_u
          set_local 2
          get_local 0
          i32.const 10000
          i32.div_u
          tee_local 3
          set_local 0
          get_local 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
        unreachable
      end
      get_local 0
      set_local 3
    end
    block  ;; label = @1
      get_local 3
      i32.const 100
      i32.lt_s
      br_if 0 (;@1;)
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 4
      i32.add
      get_local 3
      i32.const 100
      i32.rem_u
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
      get_local 3
      i32.const 100
      i32.div_u
      set_local 3
    end
    block  ;; label = @1
      block  ;; label = @2
        get_local 3
        i32.const 9
        i32.gt_s
        br_if 0 (;@2;)
        get_local 5
        i32.const 9
        i32.add
        get_local 4
        i32.const -1
        i32.add
        tee_local 0
        i32.add
        get_local 3
        i32.const 48
        i32.add
        i32.store8
        br 1 (;@1;)
      end
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 0
      i32.add
      get_local 3
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
    end
    get_local 1
    i32.const 1
    i32.const 2496
    i32.const 0
    get_local 5
    i32.const 9
    i32.add
    get_local 0
    i32.add
    i32.const 39
    get_local 0
    i32.sub
    call 70
    set_local 0
    i32.const 0
    get_local 5
    i32.const 48
    i32.add
    i32.store offset=4
    get_local 0)
  (func (;65;) (type 0) (param i32 i32 i32) (result i32)
    get_local 0
    i32.load
    get_local 1
    get_local 2
    call 79)
  (func (;66;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 7
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    i32.const 0
    set_local 3
    get_local 7
    i32.const 0
    i32.store offset=12
    block  ;; label = @1
      block  ;; label = @2
        get_local 1
        i32.const 127
        i32.gt_u
        br_if 0 (;@2;)
        get_local 7
        get_local 1
        i32.store8 offset=12
        i32.const 1
        set_local 6
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          get_local 1
          i32.const 2048
          i32.ge_u
          br_if 0 (;@3;)
          i32.const 2
          set_local 6
          i32.const 1
          set_local 5
          i32.const 192
          set_local 4
          i32.const 31
          set_local 2
          br 1 (;@2;)
        end
        block  ;; label = @3
          get_local 1
          i32.const 65535
          i32.gt_u
          br_if 0 (;@3;)
          get_local 7
          get_local 1
          i32.const 12
          i32.shr_u
          i32.const 15
          i32.and
          i32.const 224
          i32.or
          i32.store8 offset=12
          i32.const 3
          set_local 6
          i32.const 2
          set_local 5
          i32.const 128
          set_local 4
          i32.const 1
          set_local 3
          i32.const 63
          set_local 2
          br 1 (;@2;)
        end
        get_local 7
        get_local 1
        i32.const 18
        i32.shr_u
        i32.const 240
        i32.or
        i32.store8 offset=12
        i32.const 63
        set_local 2
        i32.const 128
        set_local 4
        get_local 7
        get_local 1
        i32.const 12
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        i32.const 4
        set_local 6
        i32.const 3
        set_local 5
        i32.const 2
        set_local 3
      end
      get_local 7
      i32.const 12
      i32.add
      get_local 3
      i32.add
      get_local 2
      get_local 1
      i32.const 6
      i32.shr_u
      i32.and
      get_local 4
      i32.or
      i32.store8
      get_local 7
      i32.const 12
      i32.add
      get_local 5
      i32.add
      get_local 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8
    end
    get_local 0
    get_local 7
    i32.const 12
    i32.add
    get_local 6
    call 79
    set_local 1
    i32.const 0
    get_local 7
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;67;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 4
    i32.store offset=4
    get_local 0
    i32.load
    set_local 0
    get_local 4
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    tee_local 2
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    tee_local 3
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 4
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 4
    get_local 0
    i32.store offset=36
    get_local 4
    i32.const 40
    i32.add
    i32.const 16
    i32.add
    get_local 2
    i64.load
    i64.store
    get_local 4
    i32.const 40
    i32.add
    i32.const 8
    i32.add
    get_local 3
    i64.load
    i64.store
    get_local 4
    get_local 4
    i64.load offset=8
    i64.store offset=40
    get_local 4
    i32.const 36
    i32.add
    i32.const 2980
    get_local 4
    i32.const 40
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 4
    i32.const 64
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;68;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 5
    i32.store offset=4
    i32.const 39
    set_local 4
    block  ;; label = @1
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 0
        i32.const 10000
        i32.lt_u
        br_if 0 (;@2;)
        i32.const 39
        set_local 4
        loop  ;; label = @3
          get_local 5
          i32.const 9
          i32.add
          get_local 4
          i32.add
          tee_local 2
          i32.const -2
          i32.add
          get_local 0
          i32.const 10000
          i32.rem_u
          tee_local 3
          i32.const 100
          i32.rem_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 2
          i32.const -4
          i32.add
          get_local 3
          i32.const 100
          i32.div_u
          i32.const 1
          i32.shl
          i32.const 2292
          i32.add
          i32.load16_u
          i32.store16 align=1
          get_local 4
          i32.const -4
          i32.add
          set_local 4
          get_local 0
          i32.const 99999999
          i32.gt_u
          set_local 2
          get_local 0
          i32.const 10000
          i32.div_u
          tee_local 3
          set_local 0
          get_local 2
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
        unreachable
      end
      get_local 0
      set_local 3
    end
    block  ;; label = @1
      get_local 3
      i32.const 100
      i32.lt_s
      br_if 0 (;@1;)
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 4
      i32.add
      get_local 3
      i32.const 100
      i32.rem_u
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
      get_local 3
      i32.const 100
      i32.div_u
      set_local 3
    end
    block  ;; label = @1
      block  ;; label = @2
        get_local 3
        i32.const 9
        i32.gt_s
        br_if 0 (;@2;)
        get_local 5
        i32.const 9
        i32.add
        get_local 4
        i32.const -1
        i32.add
        tee_local 0
        i32.add
        get_local 3
        i32.const 48
        i32.add
        i32.store8
        br 1 (;@1;)
      end
      get_local 5
      i32.const 9
      i32.add
      get_local 4
      i32.const -2
      i32.add
      tee_local 0
      i32.add
      get_local 3
      i32.const 1
      i32.shl
      i32.const 2292
      i32.add
      i32.load16_u
      i32.store16 align=1
    end
    get_local 1
    i32.const 1
    i32.const 2496
    i32.const 0
    get_local 5
    i32.const 9
    i32.add
    get_local 0
    i32.add
    i32.const 39
    get_local 0
    i32.sub
    call 70
    set_local 0
    i32.const 0
    get_local 5
    i32.const 48
    i32.add
    i32.store offset=4
    get_local 0)
  (func (;69;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 13
    i32.store offset=4
    get_local 13
    i64.const 137438953472
    i64.store
    get_local 13
    i32.const 0
    i32.store offset=8
    get_local 13
    i32.const 0
    i32.store offset=16
    get_local 2
    i32.const 20
    i32.add
    i32.load
    set_local 5
    get_local 13
    i32.const 3
    i32.store8 offset=48
    get_local 2
    i32.load offset=16
    set_local 6
    get_local 13
    get_local 0
    i32.store offset=24
    get_local 13
    i32.const 28
    i32.add
    tee_local 7
    get_local 1
    i32.store
    get_local 13
    get_local 6
    i32.store offset=32
    get_local 13
    i32.const 36
    i32.add
    tee_local 8
    get_local 6
    get_local 5
    i32.const 3
    i32.shl
    tee_local 0
    i32.add
    i32.store
    get_local 13
    get_local 6
    i32.store offset=40
    get_local 13
    i32.const 44
    i32.add
    tee_local 9
    get_local 5
    i32.store
    get_local 13
    get_local 2
    i32.load
    tee_local 1
    i32.store offset=56
    get_local 13
    get_local 1
    get_local 2
    i32.load offset=4
    i32.const 3
    i32.shl
    tee_local 4
    i32.add
    tee_local 3
    i32.store offset=60
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      get_local 2
                      i32.load offset=8
                      tee_local 5
                      i32.eqz
                      br_if 0 (;@9;)
                      get_local 5
                      i32.const 28
                      i32.add
                      set_local 0
                      get_local 5
                      get_local 2
                      i32.const 12
                      i32.add
                      i32.load
                      i32.const 36
                      i32.mul
                      i32.add
                      set_local 4
                      get_local 13
                      i32.const 24
                      i32.add
                      set_local 2
                      get_local 13
                      i32.const 48
                      i32.add
                      set_local 10
                      get_local 13
                      i32.const 40
                      i32.add
                      set_local 11
                      loop  ;; label = @10
                        get_local 5
                        get_local 4
                        i32.eq
                        br_if 2 (;@8;)
                        get_local 13
                        i32.load offset=56
                        tee_local 6
                        get_local 3
                        i32.eq
                        br_if 4 (;@6;)
                        get_local 13
                        get_local 6
                        i32.const 8
                        i32.add
                        tee_local 1
                        i32.store offset=56
                        get_local 2
                        i32.load
                        get_local 6
                        i32.load
                        get_local 6
                        i32.load offset=4
                        get_local 7
                        i32.load
                        i32.load offset=12
                        call_indirect (type 0)
                        br_if 3 (;@7;)
                        get_local 10
                        get_local 5
                        i32.const 32
                        i32.add
                        i32.load8_u
                        i32.store8
                        get_local 13
                        get_local 5
                        i32.load offset=8
                        i32.store offset=4
                        get_local 13
                        get_local 5
                        i32.const 12
                        i32.add
                        i32.load
                        i32.store
                        i64.const 0
                        set_local 14
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 5
                                    i32.const 24
                                    i32.add
                                    i32.load
                                    tee_local 6
                                    i32.const 1
                                    i32.eq
                                    br_if 0 (;@16;)
                                    get_local 6
                                    i32.const 3
                                    i32.eq
                                    br_if 1 (;@15;)
                                    get_local 6
                                    i32.const 2
                                    i32.ne
                                    br_if 2 (;@14;)
                                    get_local 13
                                    i32.const 32
                                    i32.add
                                    tee_local 6
                                    i32.load
                                    tee_local 12
                                    get_local 8
                                    i32.load
                                    i32.eq
                                    br_if 4 (;@12;)
                                    get_local 6
                                    get_local 12
                                    i32.const 8
                                    i32.add
                                    i32.store
                                    get_local 12
                                    i32.load offset=4
                                    i32.const 6
                                    i32.ne
                                    br_if 5 (;@11;)
                                    get_local 12
                                    i32.load
                                    i32.load
                                    set_local 6
                                    br 3 (;@13;)
                                  end
                                  get_local 0
                                  i32.load
                                  tee_local 12
                                  get_local 9
                                  i32.load
                                  tee_local 6
                                  i32.ge_u
                                  br_if 13 (;@2;)
                                  get_local 11
                                  i32.load
                                  get_local 12
                                  i32.const 3
                                  i32.shl
                                  i32.add
                                  tee_local 12
                                  i32.load offset=4
                                  i32.const 6
                                  i32.ne
                                  br_if 4 (;@11;)
                                  get_local 12
                                  i32.load
                                  i32.load
                                  set_local 6
                                  br 2 (;@13;)
                                end
                                br 3 (;@11;)
                              end
                              get_local 0
                              i32.load
                              set_local 6
                            end
                            i64.const 1
                            set_local 14
                            br 1 (;@11;)
                          end
                        end
                        get_local 13
                        i32.const 8
                        i32.add
                        get_local 6
                        i64.extend_u/i32
                        i64.const 32
                        i64.shl
                        get_local 14
                        i64.or
                        i64.store
                        i64.const 0
                        set_local 14
                        block  ;; label = @11
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  block  ;; label = @16
                                    get_local 5
                                    i32.const 16
                                    i32.add
                                    i32.load
                                    tee_local 6
                                    i32.const 1
                                    i32.eq
                                    br_if 0 (;@16;)
                                    get_local 6
                                    i32.const 3
                                    i32.eq
                                    br_if 1 (;@15;)
                                    get_local 6
                                    i32.const 2
                                    i32.ne
                                    br_if 2 (;@14;)
                                    get_local 13
                                    i32.const 32
                                    i32.add
                                    tee_local 6
                                    i32.load
                                    tee_local 12
                                    get_local 8
                                    i32.load
                                    i32.eq
                                    br_if 4 (;@12;)
                                    get_local 6
                                    get_local 12
                                    i32.const 8
                                    i32.add
                                    i32.store
                                    get_local 12
                                    i32.load offset=4
                                    i32.const 6
                                    i32.ne
                                    br_if 5 (;@11;)
                                    get_local 12
                                    i32.load
                                    i32.load
                                    set_local 6
                                    br 3 (;@13;)
                                  end
                                  get_local 0
                                  i32.const -8
                                  i32.add
                                  i32.load
                                  tee_local 12
                                  get_local 9
                                  i32.load
                                  tee_local 6
                                  i32.ge_u
                                  br_if 14 (;@1;)
                                  get_local 11
                                  i32.load
                                  get_local 12
                                  i32.const 3
                                  i32.shl
                                  i32.add
                                  tee_local 12
                                  i32.load offset=4
                                  i32.const 6
                                  i32.ne
                                  br_if 4 (;@11;)
                                  get_local 12
                                  i32.load
                                  i32.load
                                  set_local 6
                                  br 2 (;@13;)
                                end
                                br 3 (;@11;)
                              end
                              get_local 0
                              i32.const -8
                              i32.add
                              i32.load
                              set_local 6
                            end
                            i64.const 1
                            set_local 14
                            br 1 (;@11;)
                          end
                        end
                        get_local 13
                        i32.const 16
                        i32.add
                        get_local 6
                        i64.extend_u/i32
                        i64.const 32
                        i64.shl
                        get_local 14
                        i64.or
                        i64.store
                        block  ;; label = @11
                          block  ;; label = @12
                            get_local 5
                            i32.load
                            i32.const 1
                            i32.ne
                            br_if 0 (;@12;)
                            get_local 0
                            i32.const -24
                            i32.add
                            i32.load
                            tee_local 6
                            get_local 9
                            i32.load
                            tee_local 12
                            i32.ge_u
                            br_if 8 (;@4;)
                            get_local 11
                            i32.load
                            get_local 6
                            i32.const 3
                            i32.shl
                            i32.add
                            set_local 6
                            br 1 (;@11;)
                          end
                          get_local 13
                          i32.const 32
                          i32.add
                          tee_local 12
                          i32.load
                          tee_local 6
                          get_local 8
                          i32.load
                          i32.eq
                          br_if 8 (;@3;)
                          get_local 12
                          get_local 6
                          i32.const 8
                          i32.add
                          i32.store
                        end
                        get_local 5
                        i32.const 36
                        i32.add
                        set_local 5
                        get_local 0
                        i32.const 36
                        i32.add
                        set_local 0
                        get_local 6
                        i32.load
                        get_local 13
                        get_local 6
                        i32.load offset=4
                        call_indirect (type 2)
                        i32.eqz
                        br_if 0 (;@10;)
                        br 3 (;@7;)
                      end
                      unreachable
                    end
                    get_local 13
                    i32.const 24
                    i32.add
                    set_local 9
                    loop  ;; label = @9
                      get_local 0
                      i32.eqz
                      br_if 1 (;@8;)
                      get_local 4
                      i32.eqz
                      br_if 1 (;@8;)
                      get_local 13
                      get_local 1
                      i32.const 8
                      i32.add
                      tee_local 5
                      i32.store offset=56
                      get_local 9
                      i32.load
                      get_local 1
                      i32.load
                      get_local 1
                      i32.const 4
                      i32.add
                      i32.load
                      get_local 7
                      i32.load
                      i32.load offset=12
                      call_indirect (type 0)
                      br_if 2 (;@7;)
                      get_local 0
                      i32.const -8
                      i32.add
                      set_local 0
                      get_local 4
                      i32.const -8
                      i32.add
                      set_local 4
                      get_local 6
                      i32.load
                      set_local 2
                      get_local 6
                      i32.load offset=4
                      set_local 8
                      get_local 5
                      set_local 1
                      get_local 6
                      i32.const 8
                      i32.add
                      set_local 6
                      get_local 2
                      get_local 13
                      get_local 8
                      call_indirect (type 2)
                      i32.eqz
                      br_if 0 (;@9;)
                      br 2 (;@7;)
                    end
                    unreachable
                  end
                  get_local 1
                  get_local 3
                  i32.eq
                  br_if 1 (;@6;)
                  get_local 13
                  get_local 1
                  i32.const 8
                  i32.add
                  i32.store offset=56
                  get_local 13
                  i32.const 24
                  i32.add
                  i32.load
                  get_local 1
                  i32.load
                  get_local 1
                  i32.load offset=4
                  get_local 13
                  i32.const 28
                  i32.add
                  i32.load
                  i32.load offset=12
                  call_indirect (type 0)
                  i32.eqz
                  br_if 1 (;@6;)
                end
                i32.const 1
                set_local 5
                br 1 (;@5;)
              end
              i32.const 0
              set_local 5
            end
            i32.const 0
            get_local 13
            i32.const 64
            i32.add
            i32.store offset=4
            get_local 5
            return
          end
          i32.const 2536
          get_local 6
          get_local 12
          call 88
          unreachable
        end
        i32.const 2512
        call 87
        unreachable
      end
      i32.const 2496
      get_local 12
      get_local 6
      call 88
      unreachable
    end
    i32.const 2496
    get_local 12
    get_local 6
    call 88
    unreachable)
  (func (;70;) (type 9) (param i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 17
    i32.store offset=4
    block  ;; label = @1
      block  ;; label = @2
        get_local 1
        i32.eqz
        br_if 0 (;@2;)
        i32.const 43
        i32.const 1114112
        get_local 0
        i32.load
        tee_local 7
        i32.const 1
        i32.and
        tee_local 1
        select
        set_local 6
        get_local 1
        get_local 5
        i32.add
        set_local 8
        br 1 (;@1;)
      end
      get_local 5
      i32.const 1
      i32.add
      set_local 8
      get_local 0
      i32.load
      set_local 7
      i32.const 45
      set_local 6
    end
    i32.const 0
    set_local 12
    block  ;; label = @1
      get_local 7
      i32.const 4
      i32.and
      i32.eqz
      br_if 0 (;@1;)
      block  ;; label = @2
        block  ;; label = @3
          get_local 3
          i32.eqz
          br_if 0 (;@3;)
          i32.const 0
          set_local 13
          get_local 3
          set_local 12
          get_local 2
          set_local 1
          loop  ;; label = @4
            get_local 1
            i32.load8_u
            i32.const 192
            i32.and
            i32.const 128
            i32.eq
            get_local 13
            i32.add
            set_local 13
            get_local 1
            i32.const 1
            i32.add
            set_local 1
            get_local 12
            i32.const -1
            i32.add
            tee_local 12
            br_if 0 (;@4;)
            br 2 (;@2;)
          end
          unreachable
        end
        i32.const 0
        set_local 13
      end
      get_local 8
      get_local 3
      i32.add
      get_local 13
      i32.sub
      set_local 8
      i32.const 1
      set_local 12
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
                                                block  ;; label = @23
                                                  block  ;; label = @24
                                                    block  ;; label = @25
                                                      block  ;; label = @26
                                                        block  ;; label = @27
                                                          block  ;; label = @28
                                                            block  ;; label = @29
                                                              block  ;; label = @30
                                                                block  ;; label = @31
                                                                  block  ;; label = @32
                                                                    block  ;; label = @33
                                                                      block  ;; label = @34
                                                                        block  ;; label = @35
                                                                          get_local 0
                                                                          i32.load offset=8
                                                                          i32.const 1
                                                                          i32.ne
                                                                          br_if 0 (;@35;)
                                                                          get_local 0
                                                                          i32.const 12
                                                                          i32.add
                                                                          i32.load
                                                                          tee_local 13
                                                                          get_local 8
                                                                          i32.le_u
                                                                          br_if 1 (;@34;)
                                                                          get_local 7
                                                                          i32.const 8
                                                                          i32.and
                                                                          br_if 2 (;@33;)
                                                                          get_local 13
                                                                          get_local 8
                                                                          i32.sub
                                                                          set_local 9
                                                                          i32.const 1
                                                                          get_local 0
                                                                          i32.load8_u offset=48
                                                                          tee_local 1
                                                                          get_local 1
                                                                          i32.const 3
                                                                          i32.eq
                                                                          select
                                                                          i32.const 3
                                                                          i32.and
                                                                          tee_local 1
                                                                          i32.const 2
                                                                          i32.eq
                                                                          br_if 6 (;@29;)
                                                                          get_local 1
                                                                          i32.eqz
                                                                          br_if 7 (;@28;)
                                                                          get_local 9
                                                                          set_local 14
                                                                          i32.const 0
                                                                          set_local 9
                                                                          br 8 (;@27;)
                                                                        end
                                                                        get_local 6
                                                                        i32.const 1114112
                                                                        i32.eq
                                                                        br_if 13 (;@21;)
                                                                        get_local 0
                                                                        i32.const 28
                                                                        i32.add
                                                                        i32.load
                                                                        set_local 13
                                                                        get_local 0
                                                                        i32.load offset=24
                                                                        set_local 8
                                                                        i32.const 0
                                                                        set_local 1
                                                                        get_local 17
                                                                        i32.const 0
                                                                        i32.store offset=12
                                                                        get_local 6
                                                                        i32.const 127
                                                                        i32.gt_u
                                                                        br_if 2 (;@32;)
                                                                        get_local 17
                                                                        get_local 6
                                                                        i32.store8 offset=12
                                                                        i32.const 1
                                                                        set_local 7
                                                                        br 12 (;@22;)
                                                                      end
                                                                      get_local 6
                                                                      i32.const 1114112
                                                                      i32.eq
                                                                      br_if 18 (;@15;)
                                                                      get_local 0
                                                                      i32.const 28
                                                                      i32.add
                                                                      i32.load
                                                                      set_local 13
                                                                      get_local 0
                                                                      i32.load offset=24
                                                                      set_local 8
                                                                      i32.const 0
                                                                      set_local 1
                                                                      get_local 17
                                                                      i32.const 0
                                                                      i32.store offset=12
                                                                      get_local 6
                                                                      i32.const 127
                                                                      i32.gt_u
                                                                      br_if 2 (;@31;)
                                                                      get_local 17
                                                                      get_local 6
                                                                      i32.store8 offset=12
                                                                      i32.const 1
                                                                      set_local 7
                                                                      br 17 (;@16;)
                                                                    end
                                                                    get_local 0
                                                                    i32.const 1
                                                                    i32.store8 offset=48
                                                                    get_local 0
                                                                    i32.const 48
                                                                    i32.store offset=4
                                                                    get_local 6
                                                                    i32.const 1114112
                                                                    i32.eq
                                                                    br_if 24 (;@8;)
                                                                    get_local 0
                                                                    i32.const 28
                                                                    i32.add
                                                                    i32.load
                                                                    set_local 7
                                                                    get_local 0
                                                                    i32.load offset=24
                                                                    set_local 14
                                                                    i32.const 0
                                                                    set_local 1
                                                                    get_local 17
                                                                    i32.const 0
                                                                    i32.store offset=12
                                                                    get_local 6
                                                                    i32.const 127
                                                                    i32.gt_u
                                                                    br_if 6 (;@26;)
                                                                    get_local 17
                                                                    get_local 6
                                                                    i32.store8 offset=12
                                                                    i32.const 1
                                                                    set_local 11
                                                                    br 23 (;@9;)
                                                                  end
                                                                  get_local 6
                                                                  i32.const 2048
                                                                  i32.ge_u
                                                                  br_if 1 (;@30;)
                                                                  i32.const 2
                                                                  set_local 7
                                                                  i32.const 1
                                                                  set_local 14
                                                                  i32.const 192
                                                                  set_local 11
                                                                  br 8 (;@23;)
                                                                end
                                                                get_local 6
                                                                i32.const 2048
                                                                i32.ge_u
                                                                br_if 5 (;@25;)
                                                                i32.const 2
                                                                set_local 7
                                                                i32.const 1
                                                                set_local 14
                                                                i32.const 192
                                                                set_local 11
                                                                br 13 (;@17;)
                                                              end
                                                              get_local 6
                                                              i32.const 65535
                                                              i32.gt_u
                                                              br_if 5 (;@24;)
                                                              get_local 17
                                                              i32.const 224
                                                              i32.store8 offset=12
                                                              i32.const 3
                                                              set_local 7
                                                              i32.const 2
                                                              set_local 14
                                                              i32.const 128
                                                              set_local 11
                                                              i32.const 1
                                                              set_local 1
                                                              br 6 (;@23;)
                                                            end
                                                            get_local 9
                                                            i32.const 1
                                                            i32.shr_u
                                                            set_local 14
                                                            get_local 9
                                                            i32.const 1
                                                            i32.add
                                                            i32.const 1
                                                            i32.shr_u
                                                            set_local 9
                                                            br 1 (;@27;)
                                                          end
                                                          i32.const 0
                                                          set_local 14
                                                        end
                                                        i32.const 0
                                                        set_local 13
                                                        get_local 17
                                                        i32.const 0
                                                        i32.store offset=8
                                                        block  ;; label = @27
                                                          get_local 0
                                                          i32.load offset=4
                                                          tee_local 1
                                                          i32.const 127
                                                          i32.gt_u
                                                          br_if 0 (;@27;)
                                                          get_local 17
                                                          get_local 1
                                                          i32.store8 offset=8
                                                          i32.const 1
                                                          set_local 7
                                                          br 15 (;@12;)
                                                        end
                                                        get_local 1
                                                        i32.const 2048
                                                        i32.ge_u
                                                        br_if 6 (;@20;)
                                                        i32.const 2
                                                        set_local 7
                                                        i32.const 1
                                                        set_local 8
                                                        i32.const 192
                                                        set_local 11
                                                        i32.const 31
                                                        set_local 10
                                                        br 13 (;@13;)
                                                      end
                                                      get_local 6
                                                      i32.const 2048
                                                      i32.ge_u
                                                      br_if 6 (;@19;)
                                                      i32.const 2
                                                      set_local 11
                                                      i32.const 1
                                                      set_local 9
                                                      i32.const 192
                                                      set_local 10
                                                      br 15 (;@10;)
                                                    end
                                                    get_local 6
                                                    i32.const 65535
                                                    i32.gt_u
                                                    br_if 6 (;@18;)
                                                    get_local 17
                                                    i32.const 224
                                                    i32.store8 offset=12
                                                    i32.const 3
                                                    set_local 7
                                                    i32.const 2
                                                    set_local 14
                                                    i32.const 128
                                                    set_local 11
                                                    i32.const 1
                                                    set_local 1
                                                    br 7 (;@17;)
                                                  end
                                                  i32.const 128
                                                  set_local 11
                                                  get_local 17
                                                  get_local 6
                                                  i32.const 12
                                                  i32.shr_u
                                                  i32.const 128
                                                  i32.or
                                                  i32.store8 offset=13
                                                  get_local 17
                                                  get_local 6
                                                  i32.const 18
                                                  i32.shr_u
                                                  i32.const 240
                                                  i32.or
                                                  i32.store8 offset=12
                                                  i32.const 4
                                                  set_local 7
                                                  i32.const 3
                                                  set_local 14
                                                  i32.const 2
                                                  set_local 1
                                                end
                                                get_local 17
                                                i32.const 12
                                                i32.add
                                                get_local 1
                                                i32.add
                                                get_local 11
                                                i32.store8
                                                get_local 17
                                                i32.const 12
                                                i32.add
                                                get_local 14
                                                i32.add
                                                get_local 6
                                                i32.const 128
                                                i32.or
                                                i32.store8
                                              end
                                              i32.const 1
                                              set_local 1
                                              get_local 8
                                              get_local 17
                                              i32.const 12
                                              i32.add
                                              get_local 7
                                              get_local 13
                                              i32.load offset=12
                                              call_indirect (type 0)
                                              br_if 20 (;@1;)
                                            end
                                            block  ;; label = @21
                                              get_local 12
                                              i32.eqz
                                              br_if 0 (;@21;)
                                              i32.const 1
                                              set_local 1
                                              get_local 0
                                              i32.const 24
                                              i32.add
                                              i32.load
                                              get_local 2
                                              get_local 3
                                              get_local 0
                                              i32.const 28
                                              i32.add
                                              i32.load
                                              i32.load offset=12
                                              call_indirect (type 0)
                                              br_if 20 (;@1;)
                                            end
                                            get_local 0
                                            i32.const 24
                                            i32.add
                                            i32.load
                                            get_local 4
                                            get_local 5
                                            get_local 0
                                            i32.const 28
                                            i32.add
                                            i32.load
                                            i32.load offset=12
                                            call_indirect (type 0)
                                            set_local 1
                                            br 19 (;@1;)
                                          end
                                          get_local 1
                                          i32.const 65535
                                          i32.gt_u
                                          br_if 5 (;@14;)
                                          get_local 17
                                          get_local 1
                                          i32.const 12
                                          i32.shr_u
                                          i32.const 15
                                          i32.and
                                          i32.const 224
                                          i32.or
                                          i32.store8 offset=8
                                          i32.const 3
                                          set_local 7
                                          i32.const 2
                                          set_local 8
                                          i32.const 128
                                          set_local 11
                                          i32.const 1
                                          set_local 13
                                          i32.const 63
                                          set_local 10
                                          br 6 (;@13;)
                                        end
                                        get_local 6
                                        i32.const 65535
                                        i32.gt_u
                                        br_if 7 (;@11;)
                                        get_local 17
                                        i32.const 224
                                        i32.store8 offset=12
                                        i32.const 3
                                        set_local 11
                                        i32.const 2
                                        set_local 9
                                        i32.const 128
                                        set_local 10
                                        i32.const 1
                                        set_local 1
                                        br 8 (;@10;)
                                      end
                                      i32.const 128
                                      set_local 11
                                      get_local 17
                                      get_local 6
                                      i32.const 12
                                      i32.shr_u
                                      i32.const 128
                                      i32.or
                                      i32.store8 offset=13
                                      get_local 17
                                      get_local 6
                                      i32.const 18
                                      i32.shr_u
                                      i32.const 240
                                      i32.or
                                      i32.store8 offset=12
                                      i32.const 4
                                      set_local 7
                                      i32.const 3
                                      set_local 14
                                      i32.const 2
                                      set_local 1
                                    end
                                    get_local 17
                                    i32.const 12
                                    i32.add
                                    get_local 1
                                    i32.add
                                    get_local 11
                                    i32.store8
                                    get_local 17
                                    i32.const 12
                                    i32.add
                                    get_local 14
                                    i32.add
                                    get_local 6
                                    i32.const 128
                                    i32.or
                                    i32.store8
                                  end
                                  i32.const 1
                                  set_local 1
                                  get_local 8
                                  get_local 17
                                  i32.const 12
                                  i32.add
                                  get_local 7
                                  get_local 13
                                  i32.load offset=12
                                  call_indirect (type 0)
                                  br_if 14 (;@1;)
                                end
                                block  ;; label = @15
                                  get_local 12
                                  i32.eqz
                                  br_if 0 (;@15;)
                                  i32.const 1
                                  set_local 1
                                  get_local 0
                                  i32.const 24
                                  i32.add
                                  i32.load
                                  get_local 2
                                  get_local 3
                                  get_local 0
                                  i32.const 28
                                  i32.add
                                  i32.load
                                  i32.load offset=12
                                  call_indirect (type 0)
                                  br_if 14 (;@1;)
                                end
                                get_local 0
                                i32.const 24
                                i32.add
                                i32.load
                                get_local 4
                                get_local 5
                                get_local 0
                                i32.const 28
                                i32.add
                                i32.load
                                i32.load offset=12
                                call_indirect (type 0)
                                set_local 1
                                br 13 (;@1;)
                              end
                              get_local 17
                              get_local 1
                              i32.const 18
                              i32.shr_u
                              i32.const 240
                              i32.or
                              i32.store8 offset=8
                              i32.const 63
                              set_local 10
                              i32.const 128
                              set_local 11
                              get_local 17
                              get_local 1
                              i32.const 12
                              i32.shr_u
                              i32.const 63
                              i32.and
                              i32.const 128
                              i32.or
                              i32.store8 offset=9
                              i32.const 4
                              set_local 7
                              i32.const 3
                              set_local 8
                              i32.const 2
                              set_local 13
                            end
                            get_local 17
                            i32.const 8
                            i32.add
                            get_local 13
                            i32.add
                            get_local 10
                            get_local 1
                            i32.const 6
                            i32.shr_u
                            i32.and
                            get_local 11
                            i32.or
                            i32.store8
                            get_local 17
                            i32.const 8
                            i32.add
                            get_local 8
                            i32.add
                            get_local 1
                            i32.const 63
                            i32.and
                            i32.const 128
                            i32.or
                            i32.store8
                          end
                          get_local 0
                          i32.load offset=24
                          set_local 8
                          i32.const 0
                          set_local 1
                          get_local 0
                          i32.const 28
                          i32.add
                          i32.load
                          tee_local 10
                          i32.const 12
                          i32.add
                          set_local 11
                          block  ;; label = @12
                            loop  ;; label = @13
                              get_local 1
                              get_local 14
                              i32.ge_u
                              br_if 1 (;@12;)
                              get_local 1
                              i32.const 1
                              i32.add
                              tee_local 13
                              get_local 1
                              i32.lt_u
                              br_if 1 (;@12;)
                              get_local 13
                              set_local 1
                              get_local 8
                              get_local 17
                              i32.const 8
                              i32.add
                              get_local 7
                              get_local 11
                              i32.load
                              call_indirect (type 0)
                              i32.eqz
                              br_if 0 (;@13;)
                              br 10 (;@3;)
                            end
                            unreachable
                          end
                          get_local 6
                          i32.const 1114112
                          i32.eq
                          br_if 7 (;@4;)
                          get_local 0
                          i32.const 28
                          i32.add
                          i32.load
                          set_local 1
                          get_local 0
                          i32.const 24
                          i32.add
                          i32.load
                          set_local 13
                          i32.const 0
                          set_local 11
                          get_local 17
                          i32.const 0
                          i32.store offset=12
                          block  ;; label = @12
                            get_local 6
                            i32.const 127
                            i32.gt_u
                            br_if 0 (;@12;)
                            get_local 17
                            get_local 6
                            i32.store8 offset=12
                            i32.const 1
                            set_local 14
                            br 7 (;@5;)
                          end
                          block  ;; label = @12
                            get_local 6
                            i32.const 2048
                            i32.ge_u
                            br_if 0 (;@12;)
                            i32.const 2
                            set_local 14
                            i32.const 1
                            set_local 16
                            i32.const 192
                            set_local 15
                            br 6 (;@6;)
                          end
                          get_local 6
                          i32.const 65535
                          i32.gt_u
                          br_if 4 (;@7;)
                          get_local 17
                          i32.const 224
                          i32.store8 offset=12
                          i32.const 3
                          set_local 14
                          i32.const 2
                          set_local 16
                          i32.const 128
                          set_local 15
                          i32.const 1
                          set_local 11
                          br 5 (;@6;)
                        end
                        i32.const 128
                        set_local 10
                        get_local 17
                        get_local 6
                        i32.const 12
                        i32.shr_u
                        i32.const 128
                        i32.or
                        i32.store8 offset=13
                        get_local 17
                        get_local 6
                        i32.const 18
                        i32.shr_u
                        i32.const 240
                        i32.or
                        i32.store8 offset=12
                        i32.const 4
                        set_local 11
                        i32.const 3
                        set_local 9
                        i32.const 2
                        set_local 1
                      end
                      get_local 17
                      i32.const 12
                      i32.add
                      get_local 1
                      i32.add
                      get_local 10
                      i32.store8
                      get_local 17
                      i32.const 12
                      i32.add
                      get_local 9
                      i32.add
                      get_local 6
                      i32.const 128
                      i32.or
                      i32.store8
                    end
                    i32.const 1
                    set_local 1
                    get_local 14
                    get_local 17
                    i32.const 12
                    i32.add
                    get_local 11
                    get_local 7
                    i32.load offset=12
                    call_indirect (type 0)
                    br_if 7 (;@1;)
                  end
                  block  ;; label = @8
                    get_local 12
                    i32.eqz
                    br_if 0 (;@8;)
                    i32.const 1
                    set_local 1
                    get_local 0
                    i32.const 24
                    i32.add
                    i32.load
                    get_local 2
                    get_local 3
                    get_local 0
                    i32.const 28
                    i32.add
                    i32.load
                    i32.load offset=12
                    call_indirect (type 0)
                    br_if 7 (;@1;)
                  end
                  i32.const 0
                  set_local 1
                  get_local 17
                  i32.const 0
                  i32.store offset=12
                  get_local 17
                  i32.const 48
                  i32.store8 offset=12
                  get_local 13
                  get_local 8
                  i32.sub
                  set_local 12
                  get_local 0
                  i32.const 24
                  i32.add
                  i32.load
                  set_local 6
                  get_local 0
                  i32.const 28
                  i32.add
                  i32.load
                  tee_local 3
                  i32.const 12
                  i32.add
                  set_local 0
                  block  ;; label = @8
                    loop  ;; label = @9
                      get_local 1
                      get_local 12
                      i32.ge_u
                      br_if 1 (;@8;)
                      get_local 1
                      i32.const 1
                      i32.add
                      tee_local 13
                      get_local 1
                      i32.lt_u
                      br_if 1 (;@8;)
                      get_local 13
                      set_local 1
                      get_local 6
                      get_local 17
                      i32.const 12
                      i32.add
                      i32.const 1
                      get_local 0
                      i32.load
                      call_indirect (type 0)
                      i32.eqz
                      br_if 0 (;@9;)
                      br 6 (;@3;)
                    end
                    unreachable
                  end
                  get_local 6
                  get_local 4
                  get_local 5
                  get_local 3
                  i32.const 12
                  i32.add
                  i32.load
                  call_indirect (type 0)
                  br_if 4 (;@3;)
                  i32.const 0
                  set_local 1
                  br 6 (;@1;)
                end
                i32.const 128
                set_local 15
                get_local 17
                get_local 6
                i32.const 12
                i32.shr_u
                i32.const 128
                i32.or
                i32.store8 offset=13
                get_local 17
                get_local 6
                i32.const 18
                i32.shr_u
                i32.const 240
                i32.or
                i32.store8 offset=12
                i32.const 4
                set_local 14
                i32.const 3
                set_local 16
                i32.const 2
                set_local 11
              end
              get_local 17
              i32.const 12
              i32.add
              get_local 11
              i32.add
              get_local 15
              i32.store8
              get_local 17
              i32.const 12
              i32.add
              get_local 16
              i32.add
              get_local 6
              i32.const 128
              i32.or
              i32.store8
            end
            get_local 13
            get_local 17
            i32.const 12
            i32.add
            get_local 14
            get_local 1
            i32.load offset=12
            call_indirect (type 0)
            br_if 1 (;@3;)
          end
          block  ;; label = @4
            get_local 12
            i32.eqz
            br_if 0 (;@4;)
            get_local 0
            i32.const 24
            i32.add
            i32.load
            get_local 2
            get_local 3
            get_local 0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect (type 0)
            br_if 1 (;@3;)
          end
          get_local 8
          get_local 4
          get_local 5
          get_local 10
          i32.const 12
          i32.add
          i32.load
          tee_local 12
          call_indirect (type 0)
          br_if 0 (;@3;)
          i32.const 0
          set_local 1
          loop  ;; label = @4
            get_local 1
            get_local 9
            i32.ge_u
            br_if 2 (;@2;)
            get_local 1
            i32.const 1
            i32.add
            tee_local 13
            get_local 1
            i32.lt_u
            br_if 2 (;@2;)
            get_local 13
            set_local 1
            get_local 8
            get_local 17
            i32.const 8
            i32.add
            get_local 7
            get_local 12
            call_indirect (type 0)
            i32.eqz
            br_if 0 (;@4;)
          end
        end
        i32.const 1
        set_local 1
        br 1 (;@1;)
      end
      i32.const 0
      set_local 1
    end
    i32.const 0
    get_local 17
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;71;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 11
    i32.store offset=4
    get_local 0
    i32.load offset=16
    set_local 9
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
                            block  ;; label = @13
                              block  ;; label = @14
                                block  ;; label = @15
                                  get_local 0
                                  i32.load offset=8
                                  tee_local 10
                                  i32.const 1
                                  i32.ne
                                  br_if 0 (;@15;)
                                  get_local 9
                                  br_if 1 (;@14;)
                                  br 9 (;@6;)
                                end
                                get_local 9
                                i32.eqz
                                br_if 1 (;@13;)
                              end
                              get_local 0
                              i32.const 20
                              i32.add
                              i32.load
                              set_local 9
                              get_local 11
                              get_local 1
                              i32.store offset=12
                              get_local 11
                              i32.const 16
                              i32.add
                              get_local 1
                              get_local 2
                              i32.add
                              tee_local 5
                              i32.store
                              get_local 11
                              i32.const 0
                              i32.store offset=8
                              get_local 11
                              get_local 9
                              i32.store offset=20
                              get_local 9
                              i32.eqz
                              br_if 1 (;@12;)
                              get_local 11
                              i32.const 20
                              i32.add
                              i32.const 0
                              i32.store
                              get_local 11
                              i32.const 24
                              i32.add
                              get_local 11
                              i32.const 8
                              i32.add
                              call 91
                              get_local 11
                              i32.load offset=28
                              i32.const 1114112
                              i32.eq
                              br_if 6 (;@7;)
                              get_local 9
                              i32.const -1
                              i32.xor
                              set_local 9
                              loop  ;; label = @14
                                get_local 9
                                i32.const 1
                                i32.add
                                tee_local 9
                                i32.eqz
                                br_if 3 (;@11;)
                                get_local 11
                                i32.const 24
                                i32.add
                                get_local 11
                                i32.const 8
                                i32.add
                                call 91
                                get_local 11
                                i32.load offset=28
                                i32.const 1114112
                                i32.ne
                                br_if 0 (;@14;)
                                br 7 (;@7;)
                              end
                              unreachable
                            end
                            get_local 0
                            i32.load offset=24
                            get_local 1
                            get_local 2
                            get_local 0
                            i32.const 28
                            i32.add
                            i32.load
                            i32.load offset=12
                            call_indirect (type 0)
                            set_local 9
                            br 10 (;@2;)
                          end
                          get_local 2
                          i32.eqz
                          br_if 4 (;@7;)
                          get_local 11
                          get_local 1
                          i32.const 1
                          i32.add
                          tee_local 9
                          i32.store offset=12
                          get_local 1
                          i32.load8_s
                          tee_local 8
                          i32.const -1
                          i32.gt_s
                          br_if 3 (;@8;)
                          get_local 2
                          i32.const 1
                          i32.ne
                          br_if 1 (;@10;)
                          i32.const 0
                          set_local 3
                          get_local 5
                          set_local 6
                          br 2 (;@9;)
                        end
                        block  ;; label = @11
                          get_local 11
                          i32.load offset=24
                          tee_local 9
                          i32.eqz
                          br_if 0 (;@11;)
                          get_local 9
                          get_local 2
                          i32.eq
                          br_if 0 (;@11;)
                          get_local 9
                          get_local 2
                          i32.ge_u
                          br_if 10 (;@1;)
                          get_local 1
                          get_local 9
                          i32.add
                          i32.load8_s
                          i32.const -65
                          i32.le_s
                          br_if 10 (;@1;)
                          get_local 9
                          set_local 2
                          get_local 10
                          br_if 5 (;@6;)
                          br 6 (;@5;)
                        end
                        get_local 9
                        set_local 2
                        get_local 10
                        br_if 4 (;@6;)
                        br 5 (;@5;)
                      end
                      get_local 11
                      get_local 1
                      i32.const 2
                      i32.add
                      tee_local 9
                      i32.store offset=12
                      get_local 1
                      i32.const 1
                      i32.add
                      i32.load8_u
                      i32.const 63
                      i32.and
                      set_local 3
                      get_local 9
                      set_local 6
                    end
                    get_local 8
                    i32.const 255
                    i32.and
                    i32.const 224
                    i32.lt_u
                    br_if 0 (;@8;)
                    block  ;; label = @9
                      block  ;; label = @10
                        get_local 6
                        get_local 5
                        i32.eq
                        br_if 0 (;@10;)
                        get_local 11
                        get_local 6
                        i32.const 1
                        i32.add
                        tee_local 9
                        i32.store offset=12
                        get_local 6
                        i32.load8_u
                        i32.const 63
                        i32.and
                        set_local 4
                        get_local 9
                        set_local 6
                        br 1 (;@9;)
                      end
                      i32.const 0
                      set_local 4
                      get_local 5
                      set_local 6
                    end
                    get_local 8
                    i32.const 255
                    i32.and
                    i32.const 240
                    i32.lt_u
                    br_if 0 (;@8;)
                    get_local 8
                    i32.const 31
                    i32.and
                    set_local 8
                    get_local 4
                    i32.const 255
                    i32.and
                    get_local 3
                    i32.const 255
                    i32.and
                    i32.const 6
                    i32.shl
                    i32.or
                    set_local 3
                    block  ;; label = @9
                      block  ;; label = @10
                        get_local 6
                        get_local 5
                        i32.eq
                        br_if 0 (;@10;)
                        get_local 11
                        get_local 6
                        i32.const 1
                        i32.add
                        tee_local 9
                        i32.store offset=12
                        get_local 6
                        i32.load8_u
                        i32.const 63
                        i32.and
                        set_local 6
                        br 1 (;@9;)
                      end
                      i32.const 0
                      set_local 6
                    end
                    get_local 3
                    i32.const 6
                    i32.shl
                    get_local 8
                    i32.const 18
                    i32.shl
                    i32.const 1835008
                    i32.and
                    i32.or
                    get_local 6
                    i32.const 255
                    i32.and
                    i32.or
                    i32.const 1114112
                    i32.eq
                    br_if 1 (;@7;)
                  end
                  get_local 11
                  get_local 9
                  get_local 2
                  i32.add
                  get_local 5
                  i32.sub
                  i32.store offset=8
                  i32.const 0
                  set_local 2
                end
                get_local 10
                i32.eqz
                br_if 1 (;@5;)
              end
              get_local 0
              i32.const 12
              i32.add
              i32.load
              set_local 8
              get_local 2
              i32.eqz
              br_if 1 (;@4;)
              get_local 1
              get_local 2
              i32.add
              set_local 5
              i32.const 0
              set_local 10
              get_local 1
              set_local 9
              loop  ;; label = @6
                get_local 9
                i32.load8_u
                i32.const 192
                i32.and
                i32.const 128
                i32.eq
                get_local 10
                i32.add
                set_local 10
                get_local 5
                get_local 9
                i32.const 1
                i32.add
                tee_local 9
                i32.ne
                br_if 0 (;@6;)
                br 3 (;@3;)
              end
              unreachable
            end
            get_local 0
            i32.load offset=24
            get_local 1
            get_local 2
            get_local 0
            i32.const 28
            i32.add
            i32.load
            i32.load offset=12
            call_indirect (type 0)
            set_local 9
            br 2 (;@2;)
          end
          i32.const 0
          set_local 10
        end
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 2
                get_local 10
                i32.sub
                get_local 8
                i32.ge_u
                br_if 0 (;@6;)
                i32.const 0
                set_local 10
                block  ;; label = @7
                  get_local 2
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 1
                  get_local 2
                  i32.add
                  set_local 5
                  i32.const 0
                  set_local 10
                  get_local 1
                  set_local 9
                  loop  ;; label = @8
                    get_local 9
                    i32.load8_u
                    i32.const 192
                    i32.and
                    i32.const 128
                    i32.eq
                    get_local 10
                    i32.add
                    set_local 10
                    get_local 5
                    get_local 9
                    i32.const 1
                    i32.add
                    tee_local 9
                    i32.ne
                    br_if 0 (;@8;)
                  end
                end
                get_local 10
                get_local 2
                i32.sub
                get_local 8
                i32.add
                set_local 3
                i32.const 0
                get_local 0
                i32.load8_u offset=48
                tee_local 9
                get_local 9
                i32.const 3
                i32.eq
                select
                i32.const 3
                i32.and
                tee_local 9
                i32.const 2
                i32.eq
                br_if 1 (;@5;)
                get_local 9
                i32.eqz
                br_if 2 (;@4;)
                get_local 3
                set_local 6
                i32.const 0
                set_local 3
                br 3 (;@3;)
              end
              get_local 0
              i32.load offset=24
              get_local 1
              get_local 2
              get_local 0
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect (type 0)
              set_local 9
              br 3 (;@2;)
            end
            get_local 3
            i32.const 1
            i32.shr_u
            set_local 6
            get_local 3
            i32.const 1
            i32.add
            i32.const 1
            i32.shr_u
            set_local 3
            br 1 (;@3;)
          end
          i32.const 0
          set_local 6
        end
        i32.const 0
        set_local 10
        get_local 11
        i32.const 0
        i32.store offset=8
        block  ;; label = @3
          block  ;; label = @4
            get_local 0
            i32.load offset=4
            tee_local 9
            i32.const 127
            i32.gt_u
            br_if 0 (;@4;)
            get_local 11
            get_local 9
            i32.store8 offset=8
            i32.const 1
            set_local 8
            br 1 (;@3;)
          end
          block  ;; label = @4
            block  ;; label = @5
              get_local 9
              i32.const 2048
              i32.ge_u
              br_if 0 (;@5;)
              i32.const 2
              set_local 8
              i32.const 1
              set_local 5
              i32.const 192
              set_local 4
              i32.const 31
              set_local 7
              br 1 (;@4;)
            end
            block  ;; label = @5
              get_local 9
              i32.const 65535
              i32.gt_u
              br_if 0 (;@5;)
              get_local 11
              get_local 9
              i32.const 12
              i32.shr_u
              i32.const 15
              i32.and
              i32.const 224
              i32.or
              i32.store8 offset=8
              i32.const 3
              set_local 8
              i32.const 2
              set_local 5
              i32.const 128
              set_local 4
              i32.const 1
              set_local 10
              i32.const 63
              set_local 7
              br 1 (;@4;)
            end
            get_local 11
            get_local 9
            i32.const 18
            i32.shr_u
            i32.const 240
            i32.or
            i32.store8 offset=8
            i32.const 63
            set_local 7
            i32.const 128
            set_local 4
            get_local 11
            get_local 9
            i32.const 12
            i32.shr_u
            i32.const 63
            i32.and
            i32.const 128
            i32.or
            i32.store8 offset=9
            i32.const 4
            set_local 8
            i32.const 3
            set_local 5
            i32.const 2
            set_local 10
          end
          get_local 11
          i32.const 8
          i32.add
          get_local 10
          i32.add
          get_local 7
          get_local 9
          i32.const 6
          i32.shr_u
          i32.and
          get_local 4
          i32.or
          i32.store8
          get_local 11
          i32.const 8
          i32.add
          get_local 5
          i32.add
          get_local 9
          i32.const 63
          i32.and
          i32.const 128
          i32.or
          i32.store8
        end
        get_local 0
        i32.load offset=24
        set_local 5
        i32.const 0
        set_local 9
        get_local 0
        i32.const 28
        i32.add
        i32.load
        tee_local 4
        i32.const 12
        i32.add
        set_local 0
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              loop  ;; label = @6
                get_local 9
                get_local 6
                i32.ge_u
                br_if 1 (;@5;)
                get_local 9
                i32.const 1
                i32.add
                tee_local 10
                get_local 9
                i32.lt_u
                br_if 1 (;@5;)
                get_local 10
                set_local 9
                get_local 5
                get_local 11
                i32.const 8
                i32.add
                get_local 8
                get_local 0
                i32.load
                call_indirect (type 0)
                i32.eqz
                br_if 0 (;@6;)
                br 2 (;@4;)
              end
              unreachable
            end
            get_local 5
            get_local 1
            get_local 2
            get_local 4
            i32.const 12
            i32.add
            i32.load
            tee_local 0
            call_indirect (type 0)
            br_if 0 (;@4;)
            i32.const 0
            set_local 9
            loop  ;; label = @5
              get_local 9
              get_local 3
              i32.ge_u
              br_if 2 (;@3;)
              get_local 9
              i32.const 1
              i32.add
              tee_local 10
              get_local 9
              i32.lt_u
              br_if 2 (;@3;)
              get_local 10
              set_local 9
              get_local 5
              get_local 11
              i32.const 8
              i32.add
              get_local 8
              get_local 0
              call_indirect (type 0)
              i32.eqz
              br_if 0 (;@5;)
            end
          end
          i32.const 1
          set_local 9
          br 1 (;@2;)
        end
        i32.const 0
        set_local 9
      end
      i32.const 0
      get_local 11
      i32.const 32
      i32.add
      i32.store offset=4
      get_local 9
      return
    end
    get_local 1
    get_local 2
    i32.const 0
    get_local 9
    call 92
    unreachable)
  (func (;72;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i64)
    i32.const 1
    set_local 6
    block  ;; label = @1
      get_local 1
      i32.load offset=24
      tee_local 2
      i32.const 39
      get_local 1
      i32.const 28
      i32.add
      i32.load
      i32.load offset=16
      tee_local 3
      call_indirect (type 2)
      br_if 0 (;@1;)
      i32.const 2
      set_local 6
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        get_local 0
                        i32.load
                        tee_local 1
                        i32.const -9
                        i32.add
                        tee_local 0
                        i32.const 30
                        i32.gt_u
                        br_if 0 (;@10;)
                        i32.const 116
                        set_local 5
                        block  ;; label = @11
                          get_local 0
                          br_table 9 (;@2;) 0 (;@11;) 2 (;@9;) 2 (;@9;) 3 (;@8;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 6 (;@5;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 2 (;@9;) 6 (;@5;) 9 (;@2;)
                        end
                        i32.const 110
                        set_local 5
                        br 8 (;@2;)
                      end
                      get_local 1
                      i32.const 92
                      i32.eq
                      br_if 4 (;@5;)
                    end
                    get_local 1
                    i32.const 65535
                    i32.gt_u
                    br_if 1 (;@7;)
                    get_local 1
                    i32.const 3168
                    i32.const 41
                    i32.const 3264
                    i32.const 304
                    i32.const 3568
                    i32.const 326
                    call 85
                    br_if 2 (;@6;)
                    br 4 (;@4;)
                  end
                  i32.const 114
                  set_local 5
                  br 5 (;@2;)
                end
                block  ;; label = @7
                  get_local 1
                  i32.const 131072
                  i32.ge_u
                  br_if 0 (;@7;)
                  get_local 1
                  i32.const 3904
                  i32.const 33
                  i32.const 3984
                  i32.const 150
                  i32.const 4144
                  i32.const 360
                  call 85
                  br_if 1 (;@6;)
                  br 3 (;@4;)
                end
                get_local 1
                i32.const -918000
                i32.add
                i32.const 196112
                i32.lt_u
                br_if 2 (;@4;)
                get_local 1
                i32.const -195102
                i32.add
                i32.const 722657
                i32.gt_u
                get_local 1
                i32.const -191457
                i32.add
                i32.const 3102
                i32.gt_u
                get_local 1
                i32.const -183970
                i32.add
                i32.const 13
                i32.gt_u
                get_local 1
                i32.const 2097150
                i32.and
                i32.const 178206
                i32.ne
                get_local 1
                i32.const -173783
                i32.add
                i32.const 40
                i32.gt_u
                get_local 1
                i32.const -177973
                i32.add
                i32.const 10
                i32.gt_u
                i32.and
                i32.and
                i32.and
                i32.and
                i32.and
                i32.eqz
                br_if 2 (;@4;)
              end
              i32.const 1
              set_local 6
            end
            br 1 (;@3;)
          end
          get_local 1
          i32.const 1
          i32.or
          i32.clz
          i32.const 2
          i32.shr_u
          i32.const 7
          i32.xor
          i64.extend_u/i32
          i64.const 21474836480
          i64.or
          set_local 9
          i32.const 3
          set_local 6
        end
        get_local 1
        set_local 5
      end
      get_local 9
      i64.const 32
      i64.shr_u
      i32.wrap/i64
      set_local 1
      get_local 9
      i32.wrap/i64
      set_local 7
      block  ;; label = @2
        block  ;; label = @3
          loop  ;; label = @4
            get_local 1
            set_local 0
            block  ;; label = @5
              block  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    block  ;; label = @9
                      block  ;; label = @10
                        get_local 6
                        i32.const 3
                        i32.and
                        tee_local 1
                        i32.const 1
                        i32.eq
                        br_if 0 (;@10;)
                        get_local 1
                        i32.const 2
                        i32.eq
                        br_if 1 (;@9;)
                        get_local 1
                        i32.const 3
                        i32.ne
                        br_if 8 (;@2;)
                        i32.const 4
                        set_local 1
                        get_local 0
                        i32.const 7
                        i32.and
                        i32.const -1
                        i32.add
                        tee_local 4
                        i32.const 4
                        i32.gt_u
                        br_if 8 (;@2;)
                        i32.const 92
                        set_local 8
                        block  ;; label = @11
                          get_local 4
                          br_table 0 (;@11;) 4 (;@7;) 5 (;@6;) 6 (;@5;) 3 (;@8;) 0 (;@11;)
                        end
                        i32.const 0
                        set_local 1
                        get_local 2
                        i32.const 125
                        get_local 3
                        call_indirect (type 2)
                        i32.eqz
                        br_if 6 (;@4;)
                        br 7 (;@3;)
                      end
                      i32.const 0
                      set_local 6
                      get_local 0
                      set_local 1
                      get_local 2
                      get_local 5
                      get_local 3
                      call_indirect (type 2)
                      i32.eqz
                      br_if 5 (;@4;)
                      br 6 (;@3;)
                    end
                    i32.const 92
                    set_local 8
                    i32.const 1
                    set_local 6
                    get_local 0
                    set_local 1
                  end
                  get_local 2
                  get_local 8
                  get_local 3
                  call_indirect (type 2)
                  i32.eqz
                  br_if 3 (;@4;)
                  br 4 (;@3;)
                end
                get_local 0
                i32.const 1
                get_local 7
                select
                set_local 1
                get_local 7
                i32.const 2
                i32.shl
                set_local 0
                get_local 7
                i32.const -1
                i32.add
                i32.const 0
                get_local 7
                select
                set_local 7
                get_local 2
                i32.const 48
                i32.const 87
                get_local 5
                get_local 0
                i32.const 28
                i32.and
                i32.shr_u
                i32.const 15
                i32.and
                tee_local 0
                i32.const 10
                i32.lt_u
                select
                get_local 0
                i32.add
                get_local 3
                call_indirect (type 2)
                i32.eqz
                br_if 2 (;@4;)
                br 3 (;@3;)
              end
              i32.const 2
              set_local 1
              get_local 2
              i32.const 123
              get_local 3
              call_indirect (type 2)
              i32.eqz
              br_if 1 (;@4;)
              br 2 (;@3;)
            end
            i32.const 3
            set_local 1
            get_local 2
            i32.const 117
            get_local 3
            call_indirect (type 2)
            i32.eqz
            br_if 0 (;@4;)
          end
        end
        i32.const 1
        return
      end
      get_local 2
      i32.const 39
      get_local 3
      call_indirect (type 2)
      set_local 6
    end
    get_local 6)
  (func (;73;) (type 2) (param i32 i32) (result i32)
    get_local 0
    i32.load
    get_local 1
    get_local 0
    i32.load offset=4
    i32.load offset=12
    call_indirect (type 2))
  (func (;74;) (type 2) (param i32 i32) (result i32)
    get_local 1
    get_local 0
    i32.load
    get_local 0
    i32.load offset=4
    call 71)
  (func (;75;) (type 6) (param i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    get_local 0
    i32.store
    get_local 2
    get_local 1
    i32.store offset=4
    get_local 2
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 7
    i32.store
    get_local 2
    i32.const 7
    i32.store offset=36
    get_local 2
    get_local 2
    i32.const 4
    i32.add
    i32.store offset=40
    get_local 2
    i32.const 4720
    i32.store offset=16
    get_local 2
    i32.const 2
    i32.store offset=12
    get_local 2
    get_local 2
    i32.store offset=32
    get_local 2
    i32.const 2672
    i32.store offset=8
    get_local 2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 2
    get_local 2
    i32.const 32
    i32.add
    i32.store offset=24
    get_local 2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    get_local 2
    i32.const 8
    i32.add
    i32.const 2688
    call 89
    unreachable)
  (func (;76;) (type 6) (param i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    get_local 0
    i32.store
    get_local 2
    get_local 1
    i32.store offset=4
    get_local 2
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 7
    i32.store
    get_local 2
    i32.const 7
    i32.store offset=36
    get_local 2
    get_local 2
    i32.const 4
    i32.add
    i32.store offset=40
    get_local 2
    i32.const 4720
    i32.store offset=16
    get_local 2
    i32.const 2
    i32.store offset=12
    get_local 2
    get_local 2
    i32.store offset=32
    get_local 2
    i32.const 2804
    i32.store offset=8
    get_local 2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 2
    get_local 2
    i32.const 32
    i32.add
    i32.store offset=24
    get_local 2
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    get_local 2
    i32.const 8
    i32.add
    i32.const 2820
    call 89
    unreachable)
  (func (;77;) (type 2) (param i32 i32) (result i32)
    get_local 1
    i32.load offset=24
    i32.const 2896
    i32.const 11
    get_local 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 0))
  (func (;78;) (type 2) (param i32 i32) (result i32)
    get_local 1
    i32.load offset=24
    i32.const 2912
    i32.const 14
    get_local 1
    i32.const 28
    i32.add
    i32.load
    i32.load offset=12
    call_indirect (type 0))
  (func (;79;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          get_local 2
          i32.eqz
          br_if 0 (;@3;)
          get_local 0
          i32.load8_u offset=4
          set_local 11
          get_local 0
          i32.const 4
          i32.add
          set_local 8
          block  ;; label = @4
            loop  ;; label = @5
              block  ;; label = @6
                get_local 11
                i32.const 255
                i32.and
                i32.eqz
                br_if 0 (;@6;)
                get_local 0
                i32.load
                tee_local 4
                i32.load offset=24
                i32.const 2928
                i32.const 4
                get_local 4
                i32.const 28
                i32.add
                i32.load
                i32.load offset=12
                call_indirect (type 0)
                br_if 2 (;@4;)
              end
              get_local 1
              get_local 2
              i32.add
              set_local 3
              i32.const 0
              set_local 12
              get_local 1
              set_local 4
              block  ;; label = @6
                block  ;; label = @7
                  loop  ;; label = @8
                    get_local 12
                    set_local 13
                    get_local 4
                    get_local 4
                    i32.const 1
                    i32.add
                    get_local 4
                    get_local 3
                    i32.eq
                    tee_local 12
                    select
                    set_local 11
                    block  ;; label = @9
                      block  ;; label = @10
                        block  ;; label = @11
                          get_local 12
                          br_if 0 (;@11;)
                          get_local 4
                          i32.eqz
                          br_if 0 (;@11;)
                          block  ;; label = @12
                            get_local 4
                            i32.load8_s
                            tee_local 12
                            i32.const 0
                            i32.lt_s
                            br_if 0 (;@12;)
                            get_local 12
                            i32.const 255
                            i32.and
                            set_local 7
                            br 2 (;@10;)
                          end
                          block  ;; label = @12
                            block  ;; label = @13
                              get_local 11
                              get_local 3
                              i32.eq
                              br_if 0 (;@13;)
                              get_local 11
                              i32.load8_u
                              i32.const 63
                              i32.and
                              set_local 9
                              get_local 11
                              i32.const 1
                              i32.add
                              tee_local 7
                              set_local 11
                              br 1 (;@12;)
                            end
                            i32.const 0
                            set_local 9
                            get_local 3
                            set_local 7
                          end
                          get_local 12
                          i32.const 31
                          i32.and
                          set_local 6
                          get_local 9
                          i32.const 255
                          i32.and
                          set_local 9
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                get_local 12
                                i32.const 255
                                i32.and
                                tee_local 12
                                i32.const 224
                                i32.lt_u
                                br_if 0 (;@14;)
                                get_local 7
                                get_local 3
                                i32.eq
                                br_if 1 (;@13;)
                                get_local 7
                                i32.load8_u
                                i32.const 63
                                i32.and
                                set_local 10
                                get_local 7
                                i32.const 1
                                i32.add
                                tee_local 11
                                set_local 7
                                br 2 (;@12;)
                              end
                              get_local 9
                              get_local 6
                              i32.const 6
                              i32.shl
                              i32.or
                              set_local 7
                              br 3 (;@10;)
                            end
                            i32.const 0
                            set_local 10
                            get_local 3
                            set_local 7
                          end
                          get_local 10
                          i32.const 255
                          i32.and
                          get_local 9
                          i32.const 6
                          i32.shl
                          i32.or
                          set_local 9
                          block  ;; label = @12
                            block  ;; label = @13
                              block  ;; label = @14
                                get_local 12
                                i32.const 240
                                i32.lt_u
                                br_if 0 (;@14;)
                                get_local 7
                                get_local 3
                                i32.eq
                                br_if 1 (;@13;)
                                get_local 7
                                i32.const 1
                                i32.add
                                set_local 11
                                get_local 7
                                i32.load8_u
                                i32.const 63
                                i32.and
                                set_local 12
                                br 2 (;@12;)
                              end
                              get_local 9
                              get_local 6
                              i32.const 12
                              i32.shl
                              i32.or
                              set_local 7
                              br 3 (;@10;)
                            end
                            i32.const 0
                            set_local 12
                          end
                          get_local 9
                          i32.const 6
                          i32.shl
                          get_local 6
                          i32.const 18
                          i32.shl
                          i32.const 1835008
                          i32.and
                          i32.or
                          get_local 12
                          i32.const 255
                          i32.and
                          i32.or
                          tee_local 7
                          i32.const 1114112
                          i32.ne
                          br_if 1 (;@10;)
                        end
                        get_local 13
                        set_local 12
                        get_local 5
                        set_local 13
                        i32.const 2
                        i32.const 3
                        i32.and
                        tee_local 7
                        br_if 1 (;@9;)
                        br 3 (;@7;)
                      end
                      get_local 13
                      get_local 4
                      i32.sub
                      get_local 11
                      i32.add
                      set_local 12
                      get_local 7
                      i32.const 10
                      i32.ne
                      i32.const 3
                      i32.and
                      tee_local 7
                      i32.eqz
                      br_if 2 (;@7;)
                    end
                    get_local 11
                    set_local 4
                    get_local 13
                    set_local 5
                    get_local 7
                    i32.const 2
                    i32.ne
                    br_if 0 (;@8;)
                  end
                  i32.const 0
                  set_local 11
                  get_local 8
                  i32.const 0
                  i32.store8
                  get_local 2
                  set_local 4
                  br 1 (;@6;)
                end
                i32.const 1
                set_local 11
                get_local 8
                i32.const 1
                i32.store8
                get_local 13
                i32.const 1
                i32.add
                set_local 4
              end
              get_local 0
              i32.load
              set_local 13
              block  ;; label = @6
                get_local 4
                i32.eqz
                get_local 2
                get_local 4
                i32.eq
                i32.or
                tee_local 12
                br_if 0 (;@6;)
                get_local 2
                get_local 4
                i32.le_u
                br_if 4 (;@2;)
                get_local 1
                get_local 4
                i32.add
                i32.load8_s
                i32.const -65
                i32.le_s
                br_if 4 (;@2;)
              end
              get_local 13
              i32.load offset=24
              get_local 1
              get_local 4
              get_local 13
              i32.const 28
              i32.add
              i32.load
              i32.load offset=12
              call_indirect (type 0)
              br_if 1 (;@4;)
              block  ;; label = @6
                block  ;; label = @7
                  get_local 12
                  i32.eqz
                  br_if 0 (;@7;)
                  get_local 1
                  get_local 4
                  i32.add
                  set_local 13
                  br 1 (;@6;)
                end
                get_local 2
                get_local 4
                i32.le_u
                br_if 5 (;@1;)
                get_local 1
                get_local 4
                i32.add
                tee_local 13
                i32.load8_s
                i32.const -65
                i32.le_s
                br_if 5 (;@1;)
              end
              get_local 13
              set_local 1
              get_local 2
              get_local 4
              i32.sub
              tee_local 2
              br_if 0 (;@5;)
            end
            i32.const 0
            return
          end
          i32.const 1
          return
        end
        i32.const 0
        return
      end
      get_local 1
      get_local 2
      i32.const 0
      get_local 4
      call 92
      unreachable
    end
    get_local 1
    get_local 2
    get_local 4
    get_local 2
    call 92
    unreachable)
  (func (;80;) (type 1) (param i32))
  (func (;81;) (type 2) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 16
    i32.sub
    tee_local 7
    i32.store offset=4
    i32.const 0
    set_local 3
    get_local 7
    i32.const 0
    i32.store offset=12
    block  ;; label = @1
      block  ;; label = @2
        get_local 1
        i32.const 127
        i32.gt_u
        br_if 0 (;@2;)
        get_local 7
        get_local 1
        i32.store8 offset=12
        i32.const 1
        set_local 6
        br 1 (;@1;)
      end
      block  ;; label = @2
        block  ;; label = @3
          get_local 1
          i32.const 2048
          i32.ge_u
          br_if 0 (;@3;)
          i32.const 2
          set_local 6
          i32.const 1
          set_local 5
          i32.const 192
          set_local 4
          i32.const 31
          set_local 2
          br 1 (;@2;)
        end
        block  ;; label = @3
          get_local 1
          i32.const 65535
          i32.gt_u
          br_if 0 (;@3;)
          get_local 7
          get_local 1
          i32.const 12
          i32.shr_u
          i32.const 15
          i32.and
          i32.const 224
          i32.or
          i32.store8 offset=12
          i32.const 3
          set_local 6
          i32.const 2
          set_local 5
          i32.const 128
          set_local 4
          i32.const 1
          set_local 3
          i32.const 63
          set_local 2
          br 1 (;@2;)
        end
        get_local 7
        get_local 1
        i32.const 18
        i32.shr_u
        i32.const 240
        i32.or
        i32.store8 offset=12
        i32.const 63
        set_local 2
        i32.const 128
        set_local 4
        get_local 7
        get_local 1
        i32.const 12
        i32.shr_u
        i32.const 63
        i32.and
        i32.const 128
        i32.or
        i32.store8 offset=13
        i32.const 4
        set_local 6
        i32.const 3
        set_local 5
        i32.const 2
        set_local 3
      end
      get_local 7
      i32.const 12
      i32.add
      get_local 3
      i32.add
      get_local 2
      get_local 1
      i32.const 6
      i32.shr_u
      i32.and
      get_local 4
      i32.or
      i32.store8
      get_local 7
      i32.const 12
      i32.add
      get_local 5
      i32.add
      get_local 1
      i32.const 63
      i32.and
      i32.const 128
      i32.or
      i32.store8
    end
    get_local 0
    get_local 7
    i32.const 12
    i32.add
    get_local 6
    call 79
    set_local 1
    i32.const 0
    get_local 7
    i32.const 16
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;82;) (type 2) (param i32 i32) (result i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 32
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    get_local 0
    i32.store offset=4
    get_local 2
    i32.const 8
    i32.add
    i32.const 16
    i32.add
    get_local 1
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    i32.const 8
    i32.add
    i32.const 8
    i32.add
    get_local 1
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 2
    get_local 1
    i64.load align=4
    i64.store offset=8
    get_local 2
    i32.const 4
    i32.add
    i32.const 2980
    get_local 2
    i32.const 8
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 2
    i32.const 32
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;83;) (type 1) (param i32))
  (func (;84;) (type 0) (param i32 i32 i32) (result i32)
    (local i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 80
    i32.sub
    tee_local 4
    i32.store offset=4
    get_local 4
    get_local 2
    i32.store offset=4
    get_local 4
    get_local 1
    i32.store
    get_local 0
    i32.load offset=4
    set_local 1
    i32.const 1
    set_local 2
    block  ;; label = @1
      get_local 0
      i32.load8_u offset=8
      br_if 0 (;@1;)
      get_local 4
      i32.const 1
      i32.store offset=12
      get_local 4
      i32.const 2944
      i32.const 3024
      get_local 1
      select
      i32.store offset=8
      get_local 4
      get_local 1
      i32.const 0
      i32.ne
      i32.store offset=20
      get_local 4
      i32.const 2976
      i32.const 2976
      get_local 1
      select
      i32.store offset=16
      block  ;; label = @2
        get_local 0
        i32.load
        tee_local 2
        i32.load8_u
        i32.const 4
        i32.and
        br_if 0 (;@2;)
        get_local 4
        i32.const 32
        i32.add
        i32.const 12
        i32.add
        i32.const 8
        i32.store
        get_local 4
        i32.const 32
        i32.add
        i32.const 20
        i32.add
        i32.const 9
        i32.store
        get_local 4
        i32.const 8
        i32.store offset=36
        get_local 2
        i32.const 28
        i32.add
        i32.load
        set_local 3
        get_local 4
        get_local 4
        i32.const 8
        i32.add
        i32.store offset=32
        get_local 4
        get_local 4
        i32.const 16
        i32.add
        i32.store offset=40
        get_local 4
        get_local 4
        i32.store offset=48
        get_local 2
        i32.load offset=24
        set_local 2
        get_local 4
        i32.const 56
        i32.add
        i32.const 12
        i32.add
        i32.const 3
        i32.store
        get_local 4
        i32.const 56
        i32.add
        i32.const 20
        i32.add
        i32.const 3
        i32.store
        get_local 4
        i32.const 3
        i32.store offset=60
        get_local 4
        i32.const 3116
        i32.store offset=56
        get_local 4
        i32.const 4968
        i32.store offset=64
        get_local 4
        get_local 4
        i32.const 32
        i32.add
        i32.store offset=72
        get_local 2
        get_local 3
        get_local 4
        i32.const 56
        i32.add
        call 69
        set_local 2
        br 1 (;@1;)
      end
      get_local 4
      i32.const 0
      i32.store8 offset=28
      get_local 4
      get_local 2
      i32.store offset=24
      get_local 4
      i32.const 32
      i32.add
      i32.const 12
      i32.add
      i32.const 9
      i32.store
      get_local 4
      i32.const 8
      i32.store offset=36
      get_local 4
      i32.const 2
      i32.store offset=60
      get_local 4
      get_local 4
      i32.store offset=40
      get_local 4
      i32.const 3044
      i32.store offset=64
      get_local 4
      get_local 4
      i32.const 8
      i32.add
      i32.store offset=32
      get_local 4
      i32.const 3028
      i32.store offset=56
      get_local 4
      i32.const 56
      i32.add
      i32.const 12
      i32.add
      i32.const 2
      i32.store
      get_local 4
      get_local 4
      i32.const 32
      i32.add
      i32.store offset=72
      get_local 4
      i32.const 76
      i32.add
      i32.const 2
      i32.store
      get_local 4
      i32.const 24
      i32.add
      i32.const 2948
      get_local 4
      i32.const 56
      i32.add
      call 69
      set_local 2
    end
    get_local 0
    i32.const 4
    i32.add
    get_local 1
    i32.const 1
    i32.add
    i32.store
    get_local 0
    i32.const 8
    i32.add
    get_local 2
    i32.store8
    i32.const 0
    get_local 4
    i32.const 80
    i32.add
    i32.store offset=4
    get_local 0)
  (func (;85;) (type 10) (param i32 i32 i32 i32 i32 i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32)
    i32.const 1
    set_local 13
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              block  ;; label = @6
                get_local 2
                i32.eqz
                br_if 0 (;@6;)
                get_local 1
                get_local 2
                i32.const 1
                i32.shl
                i32.add
                set_local 8
                get_local 0
                i32.const 65280
                i32.and
                i32.const 8
                i32.shr_u
                set_local 7
                i32.const 0
                set_local 12
                get_local 0
                i32.const 255
                i32.and
                set_local 11
                loop  ;; label = @7
                  get_local 1
                  i32.const 2
                  i32.add
                  set_local 9
                  get_local 1
                  i32.load8_u offset=1
                  tee_local 2
                  get_local 12
                  i32.add
                  set_local 10
                  block  ;; label = @8
                    block  ;; label = @9
                      get_local 7
                      get_local 1
                      i32.load8_u
                      tee_local 1
                      i32.ne
                      br_if 0 (;@9;)
                      get_local 10
                      get_local 12
                      i32.lt_u
                      br_if 7 (;@2;)
                      get_local 10
                      get_local 4
                      i32.gt_u
                      br_if 8 (;@1;)
                      get_local 3
                      get_local 12
                      i32.add
                      set_local 1
                      loop  ;; label = @10
                        get_local 2
                        i32.eqz
                        br_if 2 (;@8;)
                        get_local 2
                        i32.const -1
                        i32.add
                        set_local 2
                        get_local 1
                        i32.load8_u
                        set_local 12
                        get_local 1
                        i32.const 1
                        i32.add
                        set_local 1
                        get_local 12
                        get_local 11
                        i32.ne
                        br_if 0 (;@10;)
                        br 5 (;@5;)
                      end
                      unreachable
                    end
                    get_local 7
                    get_local 1
                    i32.lt_u
                    br_if 2 (;@6;)
                    get_local 9
                    set_local 1
                    get_local 10
                    set_local 12
                    get_local 9
                    get_local 8
                    i32.ne
                    br_if 1 (;@7;)
                    br 2 (;@6;)
                  end
                  get_local 9
                  set_local 1
                  get_local 10
                  set_local 12
                  get_local 9
                  get_local 8
                  i32.ne
                  br_if 0 (;@7;)
                end
              end
              get_local 6
              i32.eqz
              br_if 1 (;@4;)
              get_local 5
              get_local 6
              i32.add
              set_local 11
              get_local 0
              i32.const 65535
              i32.and
              set_local 12
              get_local 5
              i32.const 1
              i32.add
              set_local 2
              i32.const 1
              set_local 13
              loop  ;; label = @6
                block  ;; label = @7
                  block  ;; label = @8
                    get_local 5
                    i32.load8_u
                    tee_local 1
                    i32.const 24
                    i32.shl
                    i32.const 24
                    i32.shr_s
                    tee_local 10
                    i32.const -1
                    i32.le_s
                    br_if 0 (;@8;)
                    get_local 2
                    set_local 5
                    br 1 (;@7;)
                  end
                  get_local 2
                  get_local 11
                  i32.eq
                  br_if 4 (;@3;)
                  get_local 2
                  i32.const 1
                  i32.add
                  set_local 5
                  get_local 2
                  i32.load8_u
                  get_local 10
                  i32.const 127
                  i32.and
                  i32.const 8
                  i32.shl
                  i32.or
                  set_local 1
                end
                get_local 12
                get_local 1
                i32.sub
                tee_local 12
                i32.const 0
                i32.lt_s
                br_if 2 (;@4;)
                get_local 13
                i32.const 1
                i32.xor
                set_local 13
                get_local 5
                get_local 11
                i32.eq
                br_if 2 (;@4;)
                get_local 5
                i32.const 1
                i32.add
                set_local 2
                br 0 (;@6;)
              end
              unreachable
            end
            i32.const 0
            set_local 13
          end
          get_local 13
          i32.const 1
          i32.and
          return
        end
        i32.const 4504
        call 87
        unreachable
      end
      get_local 12
      get_local 10
      call 76
      unreachable
    end
    get_local 10
    get_local 4
    call 75
    unreachable)
  (func (;86;) (type 1) (param i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 1
    i32.store offset=4
    get_local 1
    i32.const 17
    i32.store offset=12
    get_local 1
    get_local 0
    i32.store offset=8
    get_local 1
    i32.const 28
    i32.add
    i32.const 1
    i32.store
    get_local 1
    i32.const 1
    i32.store offset=20
    get_local 1
    i32.const 8
    i32.store offset=44
    get_local 1
    i32.const 4616
    i32.store offset=24
    get_local 1
    get_local 1
    i32.const 8
    i32.add
    i32.store offset=40
    get_local 1
    i32.const 4608
    i32.store offset=16
    get_local 1
    get_local 1
    i32.const 40
    i32.add
    i32.store offset=32
    get_local 1
    i32.const 36
    i32.add
    i32.const 1
    i32.store
    get_local 1
    i32.const 16
    i32.add
    i32.const 4652
    call 89
    unreachable)
  (func (;87;) (type 1) (param i32)
    (local i32 i64 i64 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 1
    i32.store offset=4
    get_local 0
    i64.load offset=16 align=4
    set_local 2
    get_local 0
    i64.load offset=8 align=4
    set_local 3
    get_local 0
    i64.load align=4
    set_local 4
    get_local 1
    i32.const 20
    i32.add
    i32.const 0
    i32.store
    get_local 1
    get_local 4
    i64.store offset=24
    get_local 1
    i32.const 1
    i32.store offset=4
    get_local 1
    i32.const 0
    i32.store offset=8
    get_local 1
    i32.const 4852
    i32.store offset=16
    get_local 1
    get_local 1
    i32.const 24
    i32.add
    i32.store
    get_local 1
    get_local 3
    i64.store offset=32
    get_local 1
    get_local 2
    i64.store offset=40
    get_local 1
    get_local 1
    i32.const 32
    i32.add
    call 89
    unreachable)
  (func (;88;) (type 4) (param i32 i32 i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 3
    get_local 1
    i32.store
    get_local 3
    get_local 2
    i32.store offset=4
    get_local 3
    i32.const 32
    i32.add
    i32.const 12
    i32.add
    i32.const 7
    i32.store
    get_local 3
    i32.const 7
    i32.store offset=36
    get_local 3
    get_local 3
    i32.store offset=40
    get_local 3
    i32.const 4720
    i32.store offset=16
    get_local 3
    i32.const 2
    i32.store offset=12
    get_local 3
    get_local 3
    i32.const 4
    i32.add
    i32.store offset=32
    get_local 3
    i32.const 4704
    i32.store offset=8
    get_local 3
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 3
    get_local 3
    i32.const 32
    i32.add
    i32.store offset=24
    get_local 3
    i32.const 28
    i32.add
    i32.const 2
    i32.store
    get_local 3
    i32.const 8
    i32.add
    get_local 0
    call 89
    unreachable)
  (func (;89;) (type 6) (param i32 i32)
    (local i32 i32 i64 i64)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 64
    i32.sub
    tee_local 3
    i32.store offset=4
    get_local 1
    i64.load offset=8 align=4
    set_local 4
    get_local 1
    i64.load align=4
    set_local 5
    get_local 3
    i32.const 16
    i32.add
    tee_local 1
    get_local 0
    i32.const 16
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    i32.const 8
    i32.add
    tee_local 2
    get_local 0
    i32.const 8
    i32.add
    i64.load align=4
    i64.store
    get_local 3
    get_local 0
    i64.load align=4
    i64.store
    get_local 3
    i32.const 24
    i32.add
    i32.const 16
    i32.add
    get_local 1
    i64.load
    i64.store
    get_local 3
    i32.const 24
    i32.add
    i32.const 8
    i32.add
    get_local 2
    i64.load
    i64.store
    get_local 3
    get_local 3
    i64.load
    i64.store offset=24
    get_local 3
    get_local 5
    i64.store offset=48
    get_local 3
    get_local 4
    i64.store offset=56
    get_local 3
    i32.const 24
    i32.add
    get_local 3
    i32.const 48
    i32.add
    call 30
    unreachable)
  (func (;90;) (type 2) (param i32 i32) (result i32)
    (local i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 48
    i32.sub
    tee_local 2
    i32.store offset=4
    get_local 2
    i32.const 8
    i32.add
    i32.const 12
    i32.add
    i32.const 10
    i32.store
    get_local 2
    i32.const 10
    i32.store offset=12
    get_local 2
    get_local 0
    i32.store offset=8
    get_local 2
    get_local 0
    i32.const 4
    i32.add
    i32.store offset=16
    get_local 1
    i32.const 28
    i32.add
    i32.load
    set_local 0
    get_local 1
    i32.load offset=24
    set_local 1
    get_local 2
    i32.const 24
    i32.add
    i32.const 12
    i32.add
    i32.const 2
    i32.store
    get_local 2
    i32.const 44
    i32.add
    i32.const 2
    i32.store
    get_local 2
    i32.const 2
    i32.store offset=28
    get_local 2
    i32.const 4868
    i32.store offset=24
    get_local 2
    i32.const 4720
    i32.store offset=32
    get_local 2
    get_local 2
    i32.const 8
    i32.add
    i32.store offset=40
    get_local 1
    get_local 0
    get_local 2
    i32.const 24
    i32.add
    call 69
    set_local 1
    i32.const 0
    get_local 2
    i32.const 48
    i32.add
    i32.store offset=4
    get_local 1)
  (func (;91;) (type 6) (param i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    i32.const 1114112
    set_local 9
    block  ;; label = @1
      get_local 1
      i32.load offset=4
      tee_local 2
      get_local 1
      i32.const 8
      i32.add
      i32.load
      tee_local 3
      i32.eq
      br_if 0 (;@1;)
      get_local 1
      i32.const 4
      i32.add
      get_local 2
      i32.const 1
      i32.add
      tee_local 7
      i32.store
      get_local 2
      i32.eqz
      br_if 0 (;@1;)
      i32.const 0
      set_local 8
      block  ;; label = @2
        block  ;; label = @3
          get_local 2
          i32.load8_s
          tee_local 9
          i32.const 0
          i32.lt_s
          br_if 0 (;@3;)
          get_local 9
          i32.const 255
          i32.and
          set_local 8
          br 1 (;@2;)
        end
        block  ;; label = @3
          block  ;; label = @4
            get_local 7
            get_local 3
            i32.eq
            br_if 0 (;@4;)
            get_local 1
            i32.const 4
            i32.add
            get_local 2
            i32.const 2
            i32.add
            tee_local 7
            i32.store
            get_local 2
            i32.const 1
            i32.add
            i32.load8_u
            i32.const 63
            i32.and
            set_local 8
            get_local 7
            set_local 5
            br 1 (;@3;)
          end
          get_local 3
          set_local 5
        end
        get_local 9
        i32.const 31
        i32.and
        set_local 4
        get_local 8
        i32.const 255
        i32.and
        set_local 8
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 9
              i32.const 255
              i32.and
              i32.const 224
              i32.lt_u
              br_if 0 (;@5;)
              get_local 5
              get_local 3
              i32.eq
              br_if 1 (;@4;)
              get_local 1
              i32.const 4
              i32.add
              get_local 5
              i32.const 1
              i32.add
              tee_local 7
              i32.store
              get_local 5
              i32.load8_u
              i32.const 63
              i32.and
              set_local 6
              get_local 7
              set_local 5
              br 2 (;@3;)
            end
            get_local 8
            get_local 4
            i32.const 6
            i32.shl
            i32.or
            set_local 8
            br 2 (;@2;)
          end
          i32.const 0
          set_local 6
          get_local 3
          set_local 5
        end
        get_local 6
        i32.const 255
        i32.and
        get_local 8
        i32.const 6
        i32.shl
        i32.or
        set_local 8
        block  ;; label = @3
          block  ;; label = @4
            block  ;; label = @5
              get_local 9
              i32.const 255
              i32.and
              i32.const 240
              i32.lt_u
              br_if 0 (;@5;)
              get_local 5
              get_local 3
              i32.eq
              br_if 1 (;@4;)
              get_local 1
              i32.const 4
              i32.add
              get_local 5
              i32.const 1
              i32.add
              tee_local 7
              i32.store
              get_local 5
              i32.load8_u
              i32.const 63
              i32.and
              set_local 5
              br 2 (;@3;)
            end
            get_local 8
            get_local 4
            i32.const 12
            i32.shl
            i32.or
            set_local 8
            br 2 (;@2;)
          end
          i32.const 0
          set_local 5
        end
        i32.const 1114112
        set_local 9
        get_local 8
        i32.const 6
        i32.shl
        get_local 4
        i32.const 18
        i32.shl
        i32.const 1835008
        i32.and
        i32.or
        get_local 5
        i32.const 255
        i32.and
        i32.or
        tee_local 8
        i32.const 1114112
        i32.eq
        br_if 1 (;@1;)
      end
      get_local 0
      get_local 1
      i32.load
      tee_local 9
      i32.store
      get_local 1
      get_local 9
      get_local 3
      get_local 2
      i32.sub
      i32.add
      get_local 7
      i32.add
      get_local 3
      i32.sub
      i32.store
      get_local 8
      set_local 9
    end
    get_local 0
    get_local 9
    i32.store offset=4)
  (func (;92;) (type 7) (param i32 i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32)
    i32.const 0
    i32.const 0
    i32.load offset=4
    i32.const 112
    i32.sub
    tee_local 9
    i32.store offset=4
    get_local 9
    get_local 2
    i32.store offset=8
    get_local 9
    get_local 3
    i32.store offset=12
    i32.const 0
    set_local 5
    get_local 1
    set_local 6
    block  ;; label = @1
      get_local 1
      i32.const 257
      i32.lt_u
      br_if 0 (;@1;)
      i32.const 0
      get_local 1
      i32.sub
      set_local 4
      i32.const 256
      set_local 7
      block  ;; label = @2
        loop  ;; label = @3
          block  ;; label = @4
            get_local 7
            get_local 1
            i32.ge_u
            br_if 0 (;@4;)
            get_local 0
            get_local 7
            i32.add
            i32.load8_s
            i32.const -65
            i32.gt_s
            br_if 2 (;@2;)
          end
          get_local 7
          i32.const -1
          i32.add
          set_local 6
          i32.const 1
          set_local 5
          get_local 7
          i32.const 1
          i32.eq
          br_if 2 (;@1;)
          get_local 4
          get_local 7
          i32.add
          set_local 8
          get_local 6
          set_local 7
          get_local 8
          i32.const 1
          i32.ne
          br_if 0 (;@3;)
          br 2 (;@1;)
        end
        unreachable
      end
      i32.const 1
      set_local 5
      get_local 7
      set_local 6
    end
    get_local 9
    get_local 6
    i32.store offset=20
    get_local 9
    get_local 0
    i32.store offset=16
    get_local 9
    i32.const 5
    i32.const 0
    get_local 5
    select
    i32.store offset=28
    get_local 9
    i32.const 4928
    i32.const 4944
    get_local 5
    select
    i32.store offset=24
    block  ;; label = @1
      block  ;; label = @2
        block  ;; label = @3
          get_local 2
          get_local 1
          i32.gt_u
          tee_local 7
          br_if 0 (;@3;)
          get_local 3
          get_local 1
          i32.gt_u
          br_if 0 (;@3;)
          get_local 2
          get_local 3
          i32.gt_u
          br_if 1 (;@2;)
          block  ;; label = @4
            block  ;; label = @5
              get_local 2
              i32.eqz
              br_if 0 (;@5;)
              get_local 2
              get_local 1
              i32.eq
              br_if 0 (;@5;)
              get_local 2
              get_local 1
              i32.ge_u
              br_if 1 (;@4;)
              get_local 0
              get_local 2
              i32.add
              i32.load8_s
              i32.const -64
              i32.lt_s
              br_if 1 (;@4;)
            end
            get_local 3
            set_local 2
          end
          get_local 9
          get_local 2
          i32.store offset=32
          get_local 2
          i32.eqz
          set_local 8
          block  ;; label = @4
            block  ;; label = @5
              get_local 2
              i32.eqz
              br_if 0 (;@5;)
              get_local 2
              get_local 1
              i32.eq
              br_if 0 (;@5;)
              get_local 1
              i32.const 1
              i32.add
              set_local 5
              block  ;; label = @6
                loop  ;; label = @7
                  block  ;; label = @8
                    get_local 2
                    get_local 1
                    i32.ge_u
                    br_if 0 (;@8;)
                    get_local 0
                    get_local 2
                    i32.add
                    i32.load8_s
                    i32.const -64
                    i32.ge_s
                    br_if 2 (;@6;)
                  end
                  get_local 2
                  i32.const -1
                  i32.add
                  set_local 7
                  get_local 2
                  i32.const 1
                  i32.eq
                  tee_local 8
                  br_if 3 (;@4;)
                  get_local 5
                  get_local 2
                  i32.eq
                  set_local 6
                  get_local 7
                  set_local 2
                  get_local 6
                  i32.eqz
                  br_if 0 (;@7;)
                  br 3 (;@4;)
                end
                unreachable
              end
              i32.const 0
              set_local 8
            end
            get_local 2
            set_local 7
          end
          block  ;; label = @4
            block  ;; label = @5
              get_local 8
              br_if 0 (;@5;)
              get_local 7
              get_local 1
              i32.eq
              br_if 0 (;@5;)
              block  ;; label = @6
                get_local 7
                get_local 1
                i32.ge_u
                br_if 0 (;@6;)
                get_local 0
                get_local 7
                i32.add
                tee_local 2
                i32.load8_s
                i32.const -65
                i32.gt_s
                br_if 2 (;@4;)
              end
              get_local 0
              get_local 1
              get_local 7
              get_local 1
              call 92
              unreachable
            end
            get_local 0
            get_local 7
            i32.add
            set_local 2
          end
          get_local 2
          get_local 0
          get_local 7
          i32.add
          tee_local 8
          get_local 1
          get_local 7
          i32.sub
          i32.add
          tee_local 6
          i32.eq
          tee_local 5
          br_if 2 (;@1;)
          i32.const 0
          set_local 0
          block  ;; label = @4
            block  ;; label = @5
              get_local 2
              i32.load8_s
              tee_local 1
              i32.const 0
              i32.lt_s
              br_if 0 (;@5;)
              get_local 1
              i32.const 255
              i32.and
              set_local 2
              br 1 (;@4;)
            end
            get_local 6
            set_local 4
            block  ;; label = @5
              get_local 2
              get_local 8
              i32.const 1
              i32.add
              get_local 5
              select
              tee_local 2
              get_local 6
              i32.eq
              br_if 0 (;@5;)
              get_local 2
              i32.const 1
              i32.add
              set_local 4
              get_local 2
              i32.load8_u
              i32.const 63
              i32.and
              set_local 0
            end
            get_local 1
            i32.const 31
            i32.and
            set_local 8
            get_local 0
            i32.const 255
            i32.and
            set_local 2
            block  ;; label = @5
              block  ;; label = @6
                get_local 1
                i32.const 255
                i32.and
                i32.const 224
                i32.lt_u
                br_if 0 (;@6;)
                i32.const 0
                set_local 5
                get_local 6
                set_local 0
                block  ;; label = @7
                  get_local 4
                  get_local 6
                  i32.eq
                  br_if 0 (;@7;)
                  get_local 4
                  i32.const 1
                  i32.add
                  set_local 0
                  get_local 4
                  i32.load8_u
                  i32.const 63
                  i32.and
                  set_local 5
                end
                get_local 5
                i32.const 255
                i32.and
                get_local 2
                i32.const 6
                i32.shl
                i32.or
                set_local 2
                get_local 1
                i32.const 255
                i32.and
                i32.const 240
                i32.lt_u
                br_if 1 (;@5;)
                i32.const 0
                set_local 1
                block  ;; label = @7
                  get_local 0
                  get_local 6
                  i32.eq
                  br_if 0 (;@7;)
                  get_local 0
                  i32.load8_u
                  i32.const 63
                  i32.and
                  set_local 1
                end
                get_local 2
                i32.const 6
                i32.shl
                get_local 8
                i32.const 18
                i32.shl
                i32.const 1835008
                i32.and
                i32.or
                get_local 1
                i32.const 255
                i32.and
                i32.or
                tee_local 2
                i32.const 1114112
                i32.eq
                br_if 5 (;@1;)
                br 2 (;@4;)
              end
              get_local 2
              get_local 8
              i32.const 6
              i32.shl
              i32.or
              set_local 2
              br 1 (;@4;)
            end
            get_local 2
            get_local 8
            i32.const 12
            i32.shl
            i32.or
            set_local 2
          end
          get_local 9
          get_local 2
          i32.store offset=36
          i32.const 1
          set_local 6
          block  ;; label = @4
            get_local 2
            i32.const 128
            i32.lt_u
            br_if 0 (;@4;)
            i32.const 2
            set_local 6
            get_local 2
            i32.const 2048
            i32.lt_u
            br_if 0 (;@4;)
            i32.const 3
            i32.const 4
            get_local 2
            i32.const 65536
            i32.lt_u
            select
            set_local 6
          end
          get_local 9
          get_local 7
          i32.store offset=40
          get_local 9
          get_local 6
          get_local 7
          i32.add
          i32.store offset=44
          get_local 9
          i32.const 72
          i32.add
          i32.const 12
          i32.add
          i32.const 11
          i32.store
          get_local 9
          i32.const 72
          i32.add
          i32.const 20
          i32.add
          i32.const 12
          i32.store
          get_local 9
          i32.const 7
          i32.store offset=76
          get_local 9
          get_local 9
          i32.const 32
          i32.add
          i32.store offset=72
          get_local 9
          get_local 9
          i32.const 36
          i32.add
          i32.store offset=80
          get_local 9
          get_local 9
          i32.const 40
          i32.add
          i32.store offset=88
          get_local 9
          get_local 9
          i32.const 16
          i32.add
          i32.store offset=96
          get_local 9
          i32.const 100
          i32.add
          i32.const 8
          i32.store
          get_local 9
          get_local 9
          i32.const 24
          i32.add
          i32.store offset=104
          get_local 9
          i32.const 108
          i32.add
          i32.const 8
          i32.store
          get_local 9
          i32.const 5308
          i32.store offset=48
          get_local 9
          i32.const 5
          i32.store offset=52
          get_local 9
          i32.const 5348
          i32.store offset=56
          get_local 9
          i32.const 48
          i32.add
          i32.const 12
          i32.add
          i32.const 5
          i32.store
          get_local 9
          get_local 9
          i32.const 72
          i32.add
          i32.store offset=64
          get_local 9
          i32.const 48
          i32.add
          i32.const 20
          i32.add
          i32.const 5
          i32.store
          get_local 9
          i32.const 48
          i32.add
          i32.const 5528
          call 89
          unreachable
        end
        get_local 9
        get_local 2
        get_local 3
        get_local 7
        select
        i32.store offset=40
        get_local 9
        i32.const 72
        i32.add
        i32.const 12
        i32.add
        i32.const 8
        i32.store
        get_local 9
        i32.const 72
        i32.add
        i32.const 20
        i32.add
        i32.const 8
        i32.store
        get_local 9
        i32.const 7
        i32.store offset=76
        get_local 9
        i32.const 3
        i32.store offset=52
        get_local 9
        get_local 9
        i32.const 16
        i32.add
        i32.store offset=80
        get_local 9
        i32.const 4968
        i32.store offset=56
        get_local 9
        get_local 9
        i32.const 40
        i32.add
        i32.store offset=72
        get_local 9
        i32.const 4944
        i32.store offset=48
        get_local 9
        get_local 9
        i32.const 24
        i32.add
        i32.store offset=88
        get_local 9
        i32.const 48
        i32.add
        i32.const 12
        i32.add
        i32.const 3
        i32.store
        get_local 9
        get_local 9
        i32.const 72
        i32.add
        i32.store offset=64
        get_local 9
        i32.const 48
        i32.add
        i32.const 20
        i32.add
        i32.const 3
        i32.store
        get_local 9
        i32.const 48
        i32.add
        i32.const 5076
        call 89
        unreachable
      end
      get_local 9
      i32.const 72
      i32.add
      i32.const 12
      i32.add
      i32.const 7
      i32.store
      get_local 9
      i32.const 72
      i32.add
      i32.const 20
      i32.add
      i32.const 8
      i32.store
      get_local 9
      i32.const 7
      i32.store offset=76
      get_local 9
      get_local 9
      i32.const 8
      i32.add
      i32.store offset=72
      get_local 9
      get_local 9
      i32.const 12
      i32.add
      i32.store offset=80
      get_local 9
      get_local 9
      i32.const 16
      i32.add
      i32.store offset=88
      get_local 9
      get_local 9
      i32.const 24
      i32.add
      i32.store offset=96
      get_local 9
      i32.const 100
      i32.add
      i32.const 8
      i32.store
      get_local 9
      i32.const 5092
      i32.store offset=48
      get_local 9
      i32.const 4
      i32.store offset=52
      get_local 9
      i32.const 5124
      i32.store offset=56
      get_local 9
      i32.const 48
      i32.add
      i32.const 12
      i32.add
      i32.const 4
      i32.store
      get_local 9
      get_local 9
      i32.const 72
      i32.add
      i32.store offset=64
      get_local 9
      i32.const 48
      i32.add
      i32.const 20
      i32.add
      i32.const 4
      i32.store
      get_local 9
      i32.const 48
      i32.add
      i32.const 5268
      call 89
      unreachable
    end
    i32.const 5284
    call 87
    unreachable)
  (func (;93;) (type 5)
    unreachable)
  (table (;0;) 53 53 anyfunc)
  (memory (;0;) 17)
  (export "memory" (memory 0))
  (export "sample" (func 1))
  (elem (i32.const 0) 93 44 49 77 78 64 68 63 74 73 62 72 90 11 23 14 54 50 52 15 8 6 7 9 10 22 55 51 53 25 41 19 34 35 36 37 38 27 26 42 43 48 16 12 13 80 79 81 82 83 65 66 67)
  (data (i32.const 4) "\e0\16\10\00")
  (data (i32.const 16) "capacity overflow")
  (data (i32.const 48) "/checkout/src/liballoc/raw_vec.rs")
  (data (i32.const 84) "\10\00\00\00\11\00\00\000\00\00\00!\00\00\00\ca\02\00\00\08\00\00\00")
  (data (i32.const 108) "\b0\00\00\00\00\00\00\00\b0\00\00\00\02\00\00\00")
  (data (i32.const 124) "\90\00\00\00\1f\00\00\00\94\03\00\00\04\00\00\00")
  (data (i32.const 144) "/checkout/src/libcore/result.rs")
  (data (i32.const 176) ": ")
  (data (i32.const 192) "cannot recursively acquire mutex")
  (data (i32.const 224) "\f0\00\00\00&\00\00\00 \00\00\00\08\00\00\00")
  (data (i32.const 240) "/checkout/src/libstd/sys/wasm/mutex.rs")
  (data (i32.const 288) "\00")
  (data (i32.const 292) "\00\00\00\00")
  (data (i32.const 304) "internal error: entered unreachable code")
  (data (i32.const 344) "p\01\00\00,\00\00\00\9a\00\00\00\0d\00\00\00")
  (data (i32.const 368) "/checkout/src/libstd/sys_common/backtrace.rs")
  (data (i32.const 416) "StringError")
  (data (i32.const 428) "\0d\00\00\00\04\00\00\00\04\00\00\00\0e\00\00\00")
  (data (i32.const 444) "\0f\00\00\00\04\00\00\00\04\00\00\00\10\00\00\00\11\00\00\00\12\00\00\00")
  (data (i32.const 468) "\13\00\00\00\0c\00\00\00\04\00\00\00\14\00\00\00\15\00\00\00\16\00\00\00\17\00\00\00\18\00\00\00")
  (data (i32.const 512) "operation not supported on wasm yet")
  (data (i32.const 560) "/checkout/src/libstd/thread/mod.rs")
  (data (i32.const 608) "\00")
  (data (i32.const 624) "cannot recursively acquire mutex")
  (data (i32.const 656) "\00\03\00\00&\00\00\00 \00\00\00\08\00\00\00")
  (data (i32.const 672) "\00\00\00\00\00\00\00\00")
  (data (i32.const 688) "failed to generate unique thread ID: bitspace exhausted")
  (data (i32.const 744) "0\02\00\00\22\00\00\00\ad\03\00\00\10\00\00\00")
  (data (i32.const 768) "/checkout/src/libstd/sys/wasm/mutex.rs")
  (data (i32.const 816) "capacity overflow")
  (data (i32.const 836) "0\03\00\00\11\00\00\00`\03\00\00!\00\00\00\ca\02\00\00\08\00\00\00")
  (data (i32.const 864) "/checkout/src/liballoc/raw_vec.rs")
  (data (i32.const 900) "\19\00\00\00\04\00\00\00\04\00\00\00\1a\00\00\00\1b\00\00\00\1c\00\00\00")
  (data (i32.const 924) "\00\00\00\00\1d\00\00\00")
  (data (i32.const 932) "\1e\00\00\00\0c\00\00\00\04\00\00\00\1f\00\00\00")
  (data (i32.const 948) "p\05\00\002\00\00\00")
  (data (i32.const 956) "\00\00\00\00")
  (data (i32.const 960) "rwlock locked for writing")
  (data (i32.const 988) "@\05\00\00'\00\00\00!\00\00\00\0c\00\00\00")
  (data (i32.const 1004) "\00\04\00\00+\00\00\00")
  (data (i32.const 1024) "thread panicked while panicking. aborting.\0a")
  (data (i32.const 1072) "<unnamed>")
  (data (i32.const 1088) "Box<Any>")
  (data (i32.const 1096) " \00\00\00\04\00\00\00\04\00\00\00!\00\00\00\22\00\00\00#\00\00\00$\00\00\00\00\00\00\00")
  (data (i32.const 1128) "\f0\04\00\00\08\00\00\00\00\05\00\00\0f\00\00\00\10\05\00\00\03\00\00\00 \05\00\00\01\00\00\00 \05\00\00\01\00\00\000\05\00\00\01\00\00\00")
  (data (i32.const 1184) "\01")
  (data (i32.const 1188) "\b0\04\00\003\00\00\00")
  (data (i32.const 1200) "note: Run with `RUST_BACKTRACE=1` for a backtrace.\0a")
  (data (i32.const 1264) "thread '")
  (data (i32.const 1280) "' panicked at '")
  (data (i32.const 1296) "', ")
  (data (i32.const 1312) ":")
  (data (i32.const 1328) "\0a")
  (data (i32.const 1332) "\00\00\00\00%\00\00\00")
  (data (i32.const 1344) "/checkout/src/libstd/sys/wasm/rwlock.rs")
  (data (i32.const 1392) "thread panicked while processing panic. aborting.\0a")
  (data (i32.const 1444) "\00\00\00\00&\00\00\00")
  (data (i32.const 1452) "'\00\00\00\08\00\00\00\04\00\00\00(\00\00\00")
  (data (i32.const 1472) "AccessError")
  (data (i32.const 1484) "0\06\00\00+\00\00\00`\06\00\00\1f\00\00\00O\01\00\00\14\00\00\00")
  (data (i32.const 1520) "cannot access a TLS value during or after it is destroyed")
  (data (i32.const 1584) "called `Option::unwrap()` on a `None` value")
  (data (i32.const 1632) "/checkout/src/libcore/option.rs")
  (data (i32.const 1664) "already borrowed")
  (data (i32.const 1680) "already mutably borrowed")
  (data (i32.const 1704) ")\00\00\00\0c\00\00\00\04\00\00\00*\00\00\00+\00\00\00,\00\00\00")
  (data (i32.const 1728) "formatter error")
  (data (i32.const 1744) "\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00")
  (data (i32.const 2196) "\b0\08\00\00\11\00\00\00\d0\08\00\00!\00\00\00\ca\02\00\00\08\00\00\00")
  (data (i32.const 2224) "capacity overflow")
  (data (i32.const 2256) "/checkout/src/liballoc/raw_vec.rs")
  (data (i32.const 2292) "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899")
  (data (i32.const 2496) "\00\0a\00\00 \00\00\00'\04\00\00\11\00\00\00")
  (data (i32.const 2512) " \0a\00\00+\00\00\00P\0a\00\00\1f\00\00\00O\01\00\00\14\00\00\00")
  (data (i32.const 2536) "\00\0a\00\00 \00\00\00\1b\04\00\00(\00\00\00")
  (data (i32.const 2560) "/checkout/src/libcore/fmt/mod.rs")
  (data (i32.const 2592) "called `Option::unwrap()` on a `None` value")
  (data (i32.const 2640) "/checkout/src/libcore/option.rs")
  (data (i32.const 2672) "\c0\0a\00\00\06\00\00\00\d0\0a\00\00\22\00\00\00")
  (data (i32.const 2688) "\90\0a\00\00\22\00\00\00\e9\02\00\00\04\00\00\00")
  (data (i32.const 2704) "/checkout/src/libcore/slice/mod.rs")
  (data (i32.const 2752) "index ")
  (data (i32.const 2768) " out of range for slice of length ")
  (data (i32.const 2804) " \0b\00\00\16\00\00\00@\0b\00\00\0d\00\00\00")
  (data (i32.const 2820) "\90\0a\00\00\22\00\00\00\ef\02\00\00\04\00\00\00")
  (data (i32.const 2848) "slice index starts at ")
  (data (i32.const 2880) " but ends at ")
  (data (i32.const 2896) "BorrowError")
  (data (i32.const 2912) "BorrowMutError")
  (data (i32.const 2928) "    ")
  (data (i32.const 2944) ",")
  (data (i32.const 2948) "-\00\00\00\08\00\00\00\04\00\00\00.\00\00\00/\00\00\000\00\00\00")
  (data (i32.const 2976) " ")
  (data (i32.const 2980) "1\00\00\00\04\00\00\00\04\00\00\002\00\00\003\00\00\004\00\00\00")
  (data (i32.const 3008) "\0a")
  (data (i32.const 3024) "(")
  (data (i32.const 3028) "\a0\0b\00\00\00\00\00\00\c0\0b\00\00\01\00\00\00")
  (data (i32.const 3044) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\04\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 3116) "\a0\0b\00\00\00\00\00\00\a0\0b\00\00\00\00\00\00\a0\0b\00\00\00\00\00\00")
  (data (i32.const 3152) ")")
  (data (i32.const 3168) "\00\01\03\05\05\08\06\03\07\04\08\08\09\10\0a\1b\0b\19\0c\16\0d\12\0e\16\0f\04\10\03\12\12\13\09\16\01\17\05\18\02\19\03\1a\07\1d\01\1f\16 \03+\05,\02-\0b.\010\031\032\02\a7\01\a8\02\a9\02\aa\04\ab\08\fa\02\fb\05\fd\04\fe\03\ff\09")
  (data (i32.const 3264) "\adxy\8b\8d\a20WX`\88\8b\8c\90\1c\1d\dd\0e\0fKL./?\5c]_\b5\e2\84\8d\8e\91\92\a9\b1\ba\bb\c5\c6\c9\ca\de\e4\e5\04\11\12)147:;=IJ]\84\8e\92\a9\b1\b4\ba\bb\c6\ca\ce\cf\e4\e5\00\04\0d\0e\11\12)14:;EFIJ^de\84\91\9b\9d\c9\ce\cf\04\0d\11)EIWde\84\8d\91\a9\b4\ba\bb\c5\c9\df\e4\e5\f0\04\0d\11EIde\80\81\84\b2\bc\be\bf\d5\d7\f0\f1\83\85\86\89\8b\8c\98\a0\a4\a6\a8\a9\ac\ba\be\bf\c5\c7\ce\cf\da\dbH\98\bd\cd\c6\ce\cfINOWY^_\89\8e\8f\b1\b6\b7\bf\c1\c6\c7\d7\11\16\17[\5c\f6\f7\fe\ff\80\0dmq\de\df\0e\0f\1fno\1c\1d_}~\ae\af\fa\16\17\1e\1fFGNOXZ\5c^~\7f\b5\c5\d4\d5\dc\f0\f1\f5rs\8ftu\96\97\c9/_&./\a7\af\b7\bf\c7\cf\d7\df\9a@\97\98/0\8f\1f\ff\af\fe\ff\ce\ffNOZ[\07\08\0f\10'/\ee\efno7=?BE\90\91\fe\ffSgu\c8\c9\d0\d1\d8\d9\e7\fe\ff")
  (data (i32.const 3568) "\00 _\22\82\df\04\82D\08\1b\05\05\11\81\ac\0e;\05k5\1e\16\80\df\03\19\08\01\04\22\03\0a\044\04\07\03\01\07\06\07\10\0bP\0f\12\07U\08\02\04\1c\0a\09\03\08\03\07\03\02\03\03\03\0c\04\05\03\0b\06\01\0e\15\05:\03\11\07\06\05\10\08V\07\02\07\15\0dP\04C\03-\03\01\04\11\06\0f\0c:\04\1d%\0d\06L m\04j%\80\c8\05\82\b0\03\1a\06\82\fd\03Y\07\15\0b\17\09\14\0c\14\0cj\06\0a\06\1a\06X\08+\05F\0a,\04\0c\04\01\031\0b,\04\1a\06\0b\03\80\ac\06\0a\06\1fAL\04-\03t\08<\03\0f\03<7\08\08*\06\82\ff\11\18\08/\11-\03 \10!\0f\80\8c\04\82\97\19\0b\15\87Z\03\16\19\04\10\80\f4\05/\05;\07\02\0e\18\09\80\aa6t\0c\80\d6\1a\0c\05\80\ff\05\80\b6\05$\0c\9b\c6\0a\d2+\15\84\8d\037\09\81\5c\14\80\b8\08\80\b8?5\04\0a\068\08F\08\0c\06t\0b\1e\03Z\04Y\09\80\83\18\1c\0a\16\09F\0a\80\8a\06\ab\a4\0c\17\041\a1\04\81\da&\07\0c\05\05\80\a5\11\81m\10x(*\06L\04\80\8d\04\80\be\03\1b\03\0f\0d")
  (data (i32.const 3904) "\00\06\01\01\03\01\04\02\08\08\09\02\0a\03\0b\02\10\01\11\04\12\05\13\12\14\02\15\02\1a\03\1c\05\1d\04$\01j\03k\02\bc\02\d1\02\d4\0c\d5\09\d6\02\d7\02\da\01\e0\05\e8\02\ee \f0\04\f1\01\f9\01")
  (data (i32.const 3984) "\0c';>NO\8f\9e\9e\9f\06\07\096=>V\f3\d0\d1\04\14\18VW\bd5\ce\cf\e0\12\87\89\8e\9e\04\0d\0e\11\12)14:;EFIJNOdeZ\5c\b6\b7\84\85\9d\097\90\91\a8\07\0a;>o_\ee\efZb\9a\9b'(U\9d\a0\a1\a3\a4\a7\a8\ad\ba\bc\c4\06\0b\0c\15\1d:?EQ\a6\a7\cc\cd\a0\07\19\1a\22%\c5\c6\04 #%&(38:HJLPSUVXZ\5c^`cefksx}\7f\8a\a4\aa\af\b0\c0\d0/?")
  (data (i32.const 4144) "^\22{\05\03\04-\03e\04\01/.\80\82\1d\031\0f\1c\04$\09\1e\05+\05D\04\0e*\80\aa\06$\04$\04(\084\0b\01\80\90\817\09\16\0a\08\80\989\03c\08\090\16\05!\03\1b\05\01@8\04K\05(\04\03\04\09\08\09\07@ '\04\0c\096\03:\05\1a\07\04\0c\07PI73\0d3\07\06\81`\1f\81\81N\04\1e\0fC\0e\19\07\0a\06D\0c'\09u\0b?A*\06;\05\0a\06Q\06\01\05\10\03\05\80\8b^\22H\08\0a\80\a6^\22E\0b\0a\06\0d\138\08\0a6\1a\03\0f\04\10\81`S\0c\01\81\00H\08S\1d9\81\07F\0a\1d\03GI7\03\0e\08\0a\82\a6\83\9afu\0b\80\c4\8a\bc\84/\8f\d1\82G\a1\b9\829\07*\04\02`&\0aF\0a(\05\13\83pE\0b/\10\11@\02\1e\97\ed\13\82\f3\a5\0d\81\1fQ\81\8c\89\04k\05\0d\03\09\07\10\93`\80\f6\0as\08n\17F\80\baW\09\12\80\8e\81G\03\85B\0f\15\85P+\87\d5\80\d7)K\05\0a\04\02\84\a0<\06\01\04U\05\1b4\02\81\0e,\04d\0cV\0a\0d\03\5c\04=9\1d\0d,\04\09\07\02\0e\06\80\9a\83\d5\0b\0d\03\09\07t\0cU+\0c\048\08\0a\06(\08\1eR\0c\04=\03\1c\14\18(\01\0f\17\86\19")
  (data (i32.const 4504) "\b0\11\00\00+\00\00\00\e0\11\00\00\1f\00\00\00O\01\00\00\14\00\00\00")
  (data (i32.const 4528) "called `Option::unwrap()` on a `None` value")
  (data (i32.const 4576) "/checkout/src/libcore/option.rs")
  (data (i32.const 4608) "`\12\00\00\00\00\00\00")
  (data (i32.const 4616) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 4652) "@\12\00\00\1f\00\00\00{\03\00\00\04\00\00\00")
  (data (i32.const 4672) "/checkout/src/libcore/option.rs")
  (data (i32.const 4704) "\c0\12\00\00 \00\00\00\e0\12\00\00\12\00\00\00")
  (data (i32.const 4720) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 4800) "index out of bounds: the len is ")
  (data (i32.const 4832) " but the index is ")
  (data (i32.const 4864) "..")
  (data (i32.const 4868) " \13\00\00\00\00\00\00\00\13\00\00\02\00\00\00")
  (data (i32.const 4896) "\04\00\00\00\05\00\00\00\00\00\00\00\00\00\00\00\00\00\00\00\01\00\00\00\02\00\00\00\03\00\00\00")
  (data (i32.const 4928) "[...]")
  (data (i32.const 4944) "\d0\15\00\00\0b\00\00\00\c0\16\00\00\16\00\00\000\16\00\00\01\00\00\00")
  (data (i32.const 4968) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 5076) "\b0\15\00\00 \00\00\00\a9\08\00\00\08\00\00\00")
  (data (i32.const 5092) "\90\16\00\00\0e\00\00\00\a0\16\00\00\04\00\00\00\b0\16\00\00\10\00\00\000\16\00\00\01\00\00\00")
  (data (i32.const 5124) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\03\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 5268) "\b0\15\00\00 \00\00\00\ad\08\00\00\04\00\00\00")
  (data (i32.const 5284) "@\16\00\00+\00\00\00p\16\00\00\1f\00\00\00O\01\00\00\14\00\00\00")
  (data (i32.const 5308) "\d0\15\00\00\0b\00\00\00\e0\15\00\00&\00\00\00\10\16\00\00\08\00\00\00 \16\00\00\06\00\00\000\16\00\00\01\00\00\00")
  (data (i32.const 5348) "\01\00\00\00\00\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\01\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\02\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\03\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\01\00\00\00\04\00\00\00 \00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00\00\00\00\00\03\00\00\00")
  (data (i32.const 5528) "\b0\15\00\00 \00\00\00\ba\08\00\00\04\00\00\00")
  (data (i32.const 5552) "/checkout/src/libcore/str/mod.rs")
  (data (i32.const 5584) "byte index ")
  (data (i32.const 5600) " is not a char boundary; it is inside ")
  (data (i32.const 5648) " (bytes ")
  (data (i32.const 5664) ") of `")
  (data (i32.const 5680) "`")
  (data (i32.const 5696) "called `Option::unwrap()` on a `None` value")
  (data (i32.const 5744) "/checkout/src/libcore/option.rs")
  (data (i32.const 5776) "begin <= end (")
  (data (i32.const 5792) " <= ")
  (data (i32.const 5808) ") when slicing `")
  (data (i32.const 5824) " is out of bounds of `"))
