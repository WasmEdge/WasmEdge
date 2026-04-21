
(module

  (import "env" "log" (func $imported_func (param i32)))
  (import "env" "memory" (memory 1 10))
  (import "env" "table" (table 1 funcref))
  (import "env" "g_mut" (global $imported_g_mut (mut i32)))
  (import "env" "g_const" (global $imported_g_const i32))

  (type $swap_type (func (param i32 i32) (result i32 i32)))

  (global $g_i32 (export "g_i32") i32 (i32.const 42))
  (global $g_i64 (export "g_i64") i64 (i64.const 100))
  (global $g_f32 (export "g_f32") f32 (f32.const 3.14))
  (global $g_f64 (export "g_f64") f64 (f64.const 2.718))
  (global $g_mut_local (export "g_mut_local") (mut i32) (i32.const 0))
  (global $g_from_import i32 (global.get $imported_g_const))

  (func $nop_func (export "nop_func")
    nop
  )

  (func $add_func (export "add_func") (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.add
  )

  (func $swap_func (export "swap_func") (type $swap_type) (param i32 i32) (result i32 i32)
    local.get 1
    local.get 0
  )

  (func $call_import (export "call_import") (param i32)
    local.get 0
    call $imported_func
  )

  (export "memory" (memory 0))
  (export "table" (table 0))
  (export "g_from_import" (global $g_from_import))
)