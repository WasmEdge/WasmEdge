(module $mandelbrot_c.wasm
  (type (;0;) (func (param i32 i32 i32) (result i32)))
  (type (;1;) (func (param f64 f64 i32) (result i32)))
  (type (;2;) (func (param f64 f64 i32 i32) (result f64)))
  (type (;3;) (func (param i32 f64 f64 f64)))
  (type (;4;) (func (result i32)))
  (func $colour (type 0) (param i32 i32 i32) (result i32)
    block  ;; label = @1
      block  ;; label = @2
        local.get 2
        local.get 0
        i32.mul
        local.get 1
        i32.add
        i32.const 1024
        i32.rem_s
        local.tee 0
        i32.const 255
        i32.gt_s
        br_if 0 (;@2;)
        local.get 0
        local.set 2
        br 1 (;@1;)
      end
      i32.const 0
      local.set 2
      local.get 0
      i32.const 511
      i32.gt_s
      br_if 0 (;@1;)
      i32.const -2
      local.get 0
      i32.sub
      local.set 2
    end
    local.get 2
    i32.const 255
    i32.and)
  (func $iterateEquation (type 1) (param f64 f64 i32) (result i32)
    (local i32 f64 f64 f64 f64 i32)
    block  ;; label = @1
      local.get 2
      i32.const 1
      i32.ge_s
      br_if 0 (;@1;)
      i32.const 0
      return
    end
    i32.const 0
    local.set 3
    f64.const 0x0p+0 (;=0;)
    local.set 4
    f64.const 0x0p+0 (;=0;)
    local.set 5
    loop  ;; label = @1
      local.get 3
      local.set 3
      block  ;; label = @2
        local.get 5
        local.tee 5
        local.get 5
        f64.mul
        local.tee 6
        local.get 4
        local.tee 4
        local.get 4
        f64.mul
        local.tee 7
        f64.add
        f64.const 0x1p+2 (;=4;)
        f64.le
        br_if 0 (;@2;)
        local.get 3
        return
      end
      local.get 3
      i32.const 1
      i32.add
      local.tee 8
      local.set 3
      local.get 5
      local.get 5
      f64.add
      local.get 4
      f64.mul
      local.get 1
      f64.add
      local.set 4
      local.get 0
      local.get 6
      local.get 7
      f64.sub
      f64.add
      local.set 5
      local.get 2
      local.get 8
      i32.ne
      br_if 0 (;@1;)
    end
    local.get 2)
  (func $scale (type 2) (param f64 f64 i32 i32) (result f64)
    local.get 1
    local.get 3
    local.get 2
    i32.sub
    f64.convert_i32_s
    local.get 2
    f64.convert_i32_s
    f64.div
    f64.mul
    local.get 0
    f64.add)
  (func $mandelbrot (type 3) (param i32 f64 f64 f64)
    (local f64 i32 i32 i32 i32 f64 i32 i32 f64 f64 f64 f64 f64 i32 i32 i32 i32)
    local.get 3
    f64.const 0x1.9p+9 (;=800;)
    f64.mul
    f64.const 0x1.2cp+10 (;=1200;)
    f64.div
    local.set 4
    local.get 0
    i32.const 1
    i32.lt_s
    local.set 5
    i32.const 0
    local.set 6
    loop  ;; label = @1
      local.get 6
      local.tee 7
      i32.const 1200
      i32.mul
      local.set 8
      local.get 4
      local.get 7
      i32.const -800
      i32.add
      f64.convert_i32_s
      f64.const 0x1.9p+9 (;=800;)
      f64.div
      f64.mul
      local.get 2
      f64.add
      local.set 9
      i32.const 0
      local.set 6
      loop  ;; label = @2
        local.get 6
        local.set 10
        block  ;; label = @3
          block  ;; label = @4
            local.get 5
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 11
            br 1 (;@3;)
          end
          local.get 3
          local.get 10
          i32.const -1200
          i32.add
          f64.convert_i32_s
          f64.const 0x1.2cp+10 (;=1200;)
          f64.div
          f64.mul
          local.get 1
          f64.add
          local.set 12
          i32.const 0
          local.set 6
          f64.const 0x0p+0 (;=0;)
          local.set 13
          f64.const 0x0p+0 (;=0;)
          local.set 14
          loop  ;; label = @4
            local.get 6
            local.set 6
            block  ;; label = @5
              local.get 14
              local.tee 14
              local.get 14
              f64.mul
              local.tee 15
              local.get 13
              local.tee 13
              local.get 13
              f64.mul
              local.tee 16
              f64.add
              f64.const 0x1p+2 (;=4;)
              f64.le
              br_if 0 (;@5;)
              local.get 6
              local.set 11
              br 2 (;@3;)
            end
            local.get 6
            i32.const 1
            i32.add
            local.tee 11
            local.set 6
            local.get 14
            local.get 14
            f64.add
            local.get 13
            f64.mul
            local.get 9
            f64.add
            local.set 13
            local.get 12
            local.get 15
            local.get 16
            f64.sub
            f64.add
            local.set 14
            local.get 0
            local.get 11
            i32.ne
            br_if 0 (;@4;)
          end
          local.get 0
          local.set 11
        end
        local.get 10
        local.get 8
        i32.add
        i32.const 2
        i32.shl
        local.set 6
        block  ;; label = @3
          block  ;; label = @4
            local.get 11
            local.tee 17
            local.get 0
            i32.eq
            local.tee 18
            br_if 0 (;@4;)
            block  ;; label = @5
              block  ;; label = @6
                local.get 17
                i32.const 2
                i32.shl
                local.tee 11
                i32.const 1020
                i32.and
                local.tee 19
                i32.const 255
                i32.gt_u
                br_if 0 (;@6;)
                local.get 11
                local.set 20
                br 1 (;@5;)
              end
              i32.const 0
              local.set 20
              local.get 19
              i32.const 511
              i32.gt_u
              br_if 0 (;@5;)
              i32.const -2
              local.get 11
              i32.sub
              local.set 20
            end
            local.get 6
            local.get 20
            i32.store8 offset=65536
            block  ;; label = @5
              local.get 11
              i32.const 128
              i32.add
              local.tee 20
              i32.const 1020
              i32.and
              local.tee 19
              i32.const 255
              i32.gt_u
              br_if 0 (;@5;)
              local.get 20
              local.set 11
              br 2 (;@3;)
            end
            i32.const 0
            local.set 11
            local.get 19
            i32.const 511
            i32.gt_u
            br_if 1 (;@3;)
            i32.const -2
            local.get 20
            i32.sub
            local.set 11
            br 1 (;@3;)
          end
          local.get 6
          i32.const 0
          i32.store8 offset=65536
          i32.const 0
          local.set 11
        end
        local.get 6
        local.get 11
        i32.store8 offset=65537
        block  ;; label = @3
          block  ;; label = @4
            local.get 18
            i32.eqz
            br_if 0 (;@4;)
            i32.const 0
            local.set 11
            br 1 (;@3;)
          end
          block  ;; label = @4
            local.get 17
            i32.const 2
            i32.shl
            i32.const 356
            i32.add
            local.tee 17
            i32.const 1020
            i32.and
            local.tee 18
            i32.const 255
            i32.gt_u
            br_if 0 (;@4;)
            local.get 17
            local.set 11
            br 1 (;@3;)
          end
          i32.const 0
          local.set 11
          local.get 18
          i32.const 511
          i32.gt_u
          br_if 0 (;@3;)
          i32.const -2
          local.get 17
          i32.sub
          local.set 11
        end
        local.get 6
        i32.const 65539
        i32.add
        i32.const 255
        i32.store8
        local.get 6
        i32.const 65538
        i32.add
        local.get 11
        i32.store8
        local.get 10
        i32.const 1
        i32.add
        local.tee 11
        local.set 6
        local.get 11
        i32.const 1200
        i32.ne
        br_if 0 (;@2;)
      end
      local.get 7
      i32.const 1
      i32.add
      local.tee 11
      local.set 6
      local.get 11
      i32.const 800
      i32.ne
      br_if 0 (;@1;)
    end)
  (func $getImage (type 4) (result i32)
    i32.const 65536)
  (memory (;0;) 60)
  (global $__stack_pointer (mut i32) (i32.const 65536))
  (export "memory" (memory 0))
  (export "colour" (func $colour))
  (export "iterateEquation" (func $iterateEquation))
  (export "scale" (func $scale))
  (export "mandelbrot" (func $mandelbrot))
  (export "getImage" (func $getImage)))
