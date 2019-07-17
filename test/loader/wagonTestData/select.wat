(module
  (func (export "select_i32") (param $lhs i32) (param $rhs i32) (param $cond i32) (result i32)
   (select (get_local $lhs) (get_local $rhs) (get_local $cond)))

  (func (export "select_i64") (param $lhs i64) (param $rhs i64) (param $cond i32) (result i64)
   (select (get_local $lhs) (get_local $rhs) (get_local $cond)))

  (func (export "select_f32") (param $lhs f32) (param $rhs f32) (param $cond i32) (result f32)
   (select (get_local $lhs) (get_local $rhs) (get_local $cond)))

  (func (export "select_f64") (param $lhs f64) (param $rhs f64) (param $cond i32) (result f64)
   (select (get_local $lhs) (get_local $rhs) (get_local $cond)))

  ;; Check that both sides of the select are evaluated
  (func (export "select_trap_l") (param $cond i32) (result i32)
    (select (unreachable) (i32.const 0) (get_local $cond))
  )
  (func (export "select_trap_r") (param $cond i32) (result i32)
    (select (i32.const 0) (unreachable) (get_local $cond))
  )

  (func (export "select_unreached")
    (unreachable) (select)
    (unreachable) (i32.const 0) (select)
    (unreachable) (i32.const 0) (i32.const 0) (select)
    (unreachable) (f32.const 0) (i32.const 0) (select)
    (unreachable)
  )
)