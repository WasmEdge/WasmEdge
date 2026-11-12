
(module
  (table 2 3 funcref)
  (memory 1 2)
  (func $start)
  (start $start)
  (elem (i32.const 0) func $start)
  (elem func $start)
  (data (i32.const 0) "ab")
  (data "cd")
)
