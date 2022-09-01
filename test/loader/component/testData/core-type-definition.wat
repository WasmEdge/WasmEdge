(component $C
  (core type $C1 (module
    (type (func (param i32) (result i32)))
    (import "a" "b" (func (type 0)))
    (export "c" (func (type 0)))
  ))
  (core type $F (func (param i32) (result i32)))
  (core type $C2 (module
    (alias outer 1 $F (type))
    (import "a" "b" (func (type 0)))
    (export "c" (func (type 0)))
  ))
)
