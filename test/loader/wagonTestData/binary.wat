(module
  (type $t0 (func (result i32)))
  (type $t1 (func (result i64)))
  (type $t2 (func (result f32)))
  (type $t3 (func (result f64)))
  (func $i32_add (export "i32_add") (type $t0) (result i32)
    (i32.add
      (i32.const 1)
      (i32.const 2)))
  (func $i32_sub (export "i32_sub") (type $t0) (result i32)
    (i32.sub
      (i32.const 20)
      (i32.const 4)))
  (func $i32_mul (export "i32_mul") (type $t0) (result i32)
    (i32.mul
      (i32.const 3)
      (i32.const 7)))
  (func $i32_div_s (export "i32_div_s") (type $t0) (result i32)
    (i32.div_s
      (i32.const -4)
      (i32.const 2)))
  (func $i32_div_u (export "i32_div_u") (type $t0) (result i32)
    (i32.div_u
      (i32.const -4)
      (i32.const 2)))
  (func $i32_rem_s (export "i32_rem_s") (type $t0) (result i32)
    (i32.rem_s
      (i32.const -5)
      (i32.const 2)))
  (func $i32_rem_u (export "i32_rem_u") (type $t0) (result i32)
    (i32.rem_u
      (i32.const -5)
      (i32.const 2)))
  (func $i32_and (export "i32_and") (type $t0) (result i32)
    (i32.and
      (i32.const 11)
      (i32.const 5)))
  (func $i32_or (export "i32_or") (type $t0) (result i32)
    (i32.or
      (i32.const 11)
      (i32.const 5)))
  (func $i32_xor (export "i32_xor") (type $t0) (result i32)
    (i32.xor
      (i32.const 11)
      (i32.const 5)))
  (func $i32_shl (export "i32_shl") (type $t0) (result i32)
    (i32.shl
      (i32.const -100)
      (i32.const 3)))
  (func $i32_shr_u (export "i32_shr_u") (type $t0) (result i32)
    (i32.shr_u
      (i32.const -100)
      (i32.const 3)))
  (func $i32_shr_s (export "i32_shr_s") (type $t0) (result i32)
    (i32.shr_s
      (i32.const -100)
      (i32.const 3)))
  (func $i32_rotl (export "i32_rotl") (type $t0) (result i32)
    (i32.rotl
      (i32.const -100)
      (i32.const 3)))
  (func $i32_rotr (export "i32_rotr") (type $t0) (result i32)
    (i32.rotr
      (i32.const -100)
      (i32.const 3)))
  (func $i64_add (export "i64_add") (type $t1) (result i64)
    (i64.add
      (i64.const 1)
      (i64.const 2)))
  (func $i64_sub (export "i64_sub") (type $t1) (result i64)
    (i64.sub
      (i64.const 20)
      (i64.const 4)))
  (func $i64_mul (export "i64_mul") (type $t1) (result i64)
    (i64.mul
      (i64.const 3)
      (i64.const 7)))
  (func $i64_div_s (export "i64_div_s") (type $t1) (result i64)
    (i64.div_s
      (i64.const -4)
      (i64.const 2)))
  (func $i64_div_u (export "i64_div_u") (type $t1) (result i64)
    (i64.div_u
      (i64.const -4)
      (i64.const 2)))
  (func $i64_rem_s (export "i64_rem_s") (type $t1) (result i64)
    (i64.rem_s
      (i64.const -5)
      (i64.const 2)))
  (func $i64_rem_u (export "i64_rem_u") (type $t1) (result i64)
    (i64.rem_u
      (i64.const -5)
      (i64.const 2)))
  (func $i64_and (export "i64_and") (type $t1) (result i64)
    (i64.and
      (i64.const 11)
      (i64.const 5)))
  (func $i64_or (export "i64_or") (type $t1) (result i64)
    (i64.or
      (i64.const 11)
      (i64.const 5)))
  (func $i64_xor (export "i64_xor") (type $t1) (result i64)
    (i64.xor
      (i64.const 11)
      (i64.const 5)))
  (func $i64_shl (export "i64_shl") (type $t1) (result i64)
    (i64.shl
      (i64.const -100)
      (i64.const 3)))
  (func $i64_shr_u (export "i64_shr_u") (type $t1) (result i64)
    (i64.shr_u
      (i64.const -100)
      (i64.const 3)))
  (func $i64_shr_s (export "i64_shr_s") (type $t1) (result i64)
    (i64.shr_s
      (i64.const -100)
      (i64.const 3)))
  (func $i64_rotl (export "i64_rotl") (type $t1) (result i64)
    (i64.rotl
      (i64.const -100)
      (i64.const 3)))
  (func $i64_rotr (export "i64_rotr") (type $t1) (result i64)
    (i64.rotr
      (i64.const -100)
      (i64.const 3)))
  (func $f32_add (export "f32_add") (type $t2) (result f32)
    (f32.add
      (f32.const 0x1.4p+0 (;=1.25;))
      (f32.const 0x1.ep+1 (;=3.75;))))
  (func $f32_sub (export "f32_sub") (type $t2) (result f32)
    (f32.sub
      (f32.const 0x1.2p+2 (;=4.5;))
      (f32.const 0x1.388p+13 (;=10000;))))
  (func $f32_mul (export "f32_mul") (type $t2) (result f32)
    (f32.mul
      (f32.const 0x1.34ap+10 (;=1234.5;))
      (f32.const -0x1.b8p+2 (;=-6.875;))))
  (func $f32_div (export "f32_div") (type $t2) (result f32)
    (f32.div
      (f32.const 0x1.6bcc42p+46 (;=1e+14;))
      (f32.const -0x1.86ap+17 (;=-200000;))))
  (func $f32_min (export "f32_min") (type $t2) (result f32)
    (f32.min
      (f32.const 0x0p+0 (;=0;))
      (f32.const 0x0p+0 (;=0;))))
  (func $f32_max (export "f32_max") (type $t2) (result f32)
    (f32.max
      (f32.const 0x0p+0 (;=0;))
      (f32.const 0x0p+0 (;=0;))))
  (func $f32_copysign (export "f32_copysign") (type $t2) (result f32)
    (f32.copysign
      (f32.const 0x0p+0 (;=0;))
      (f32.const 0x0p+0 (;=0;))))
  (func $f64_add (export "f64_add") (type $t3) (result f64)
    (f64.add
      (f64.const 0x1.d6f34588p+29 (;=9.87654e+08;))
      (f64.const 0x1.d6f3454p+26 (;=1.23457e+08;))))
  (func $f64_sub (export "f64_sub") (type $t3) (result f64)
    (f64.sub
      (f64.const 0x1.3a8a41d39b24ep+196 (;=1.234e+59;))
      (f64.const 0x1.d1de3d2d5c713p+78 (;=5.5e+23;))))
  (func $f64_mul (export "f64_mul") (type $t3) (result f64)
    (f64.mul
      (f64.const -0x1.2c4bp+20 (;=-1.23e+06;))
      (f64.const 0x1.789fe4p+23 (;=1.23412e+07;))))
  (func $f64_div (export "f64_div") (type $t3) (result f64)
    (f64.div
      (f64.const 0x1.4e718d7d7625ap+664 (;=1e+200;))
      (f64.const 0x1.11b0ec57e649ap+166 (;=1e+50;))))
  (func $f64_min (export "f64_min") (type $t3) (result f64)
    (f64.min
      (f64.const 0x0p+0 (;=0;))
      (f64.const 0x0p+0 (;=0;))))
  (func $f64_max (export "f64_max") (type $t3) (result f64)
    (f64.max
      (f64.const 0x0p+0 (;=0;))
      (f64.const 0x0p+0 (;=0;))))
  (func $f64_copysign (export "f64_copysign") (type $t3) (result f64)
    (f64.copysign
      (f64.const 0x0p+0 (;=0;))
      (f64.const 0x0p+0 (;=0;)))))
