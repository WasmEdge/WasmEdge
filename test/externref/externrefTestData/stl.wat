(module
  (type $t0 (func (param externref externref)))
  (type $t1 (func (param externref i32)))
  (type $t2 (func (param externref externref externref)))
  (type $t3 (func (param externref externref) (result i32)))
  (import "extern_module" "stl_ostream_str" (func $stl_ostream_str (type $t0)))
  (import "extern_module" "stl_ostream_u32" (func $stl_ostream_u32 (type $t1)))
  (import "extern_module" "stl_map_insert" (func $stl_map_insert (type $t2)))
  (import "extern_module" "stl_map_erase" (func $stl_map_erase (type $t0)))
  (import "extern_module" "stl_set_insert" (func $stl_set_insert (type $t1)))
  (import "extern_module" "stl_set_erase" (func $stl_set_erase (type $t1)))
  (import "extern_module" "stl_vector_push" (func $stl_vector_push (type $t1)))
  (import "extern_module" "stl_vector_sum" (func $stl_vector_sum (type $t3)))
  (func $call_ostream_str (export "call_ostream_str") (type $t0) (param $p0 externref) (param $p1 externref)
    (call $stl_ostream_str
      (local.get $p0)
      (local.get $p1)))
  (func $call_ostream_u32 (export "call_ostream_u32") (type $t1) (param $p0 externref) (param $p1 i32)
    (call $stl_ostream_u32
      (local.get $p0)
      (local.get $p1)))
  (func $call_map_insert (export "call_map_insert") (type $t2) (param $p0 externref) (param $p1 externref) (param $p2 externref)
    (call $stl_map_insert
      (local.get $p0)
      (local.get $p1)
      (local.get $p2)))
  (func $call_map_erase (export "call_map_erase") (type $t0) (param $p0 externref) (param $p1 externref)
    (call $stl_map_erase
      (local.get $p0)
      (local.get $p1)))
  (func $call_set_insert (export "call_set_insert") (type $t1) (param $p0 externref) (param $p1 i32)
    (call $stl_set_insert
      (local.get $p0)
      (local.get $p1)))
  (func $call_set_erase (export "call_set_erase") (type $t1) (param $p0 externref) (param $p1 i32)
    (call $stl_set_erase
      (local.get $p0)
      (local.get $p1)))
  (func $call_vector_push (export "call_vector_push") (type $t1) (param $p0 externref) (param $p1 i32)
    (call $stl_vector_push
      (local.get $p0)
      (local.get $p1)))
  (func $call_vector_sum (export "call_vector_sum") (type $t3) (param $p0 externref) (param $p1 externref) (result i32)
    (call $stl_vector_sum
      (local.get $p0)
      (local.get $p1)))
  (memory $memory (export "memory") 1))
