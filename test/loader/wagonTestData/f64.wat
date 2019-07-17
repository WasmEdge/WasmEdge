(module
  (type (;0;) (func (param f64 f64) (result f64)))
  (type (;1;) (func (param f64) (result f64)))
  (func (;0;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.add)
  (func (;1;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.sub)
  (func (;2;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.mul)
  (func (;3;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.div)
  (func (;4;) (type 1) (param f64) (result f64)
    get_local 0
    f64.sqrt)
  (func (;5;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.min)
  (func (;6;) (type 0) (param f64 f64) (result f64)
    get_local 0
    get_local 1
    f64.max)
  (func (;7;) (type 1) (param f64) (result f64)
    get_local 0
    f64.ceil)
  (func (;8;) (type 1) (param f64) (result f64)
    get_local 0
    f64.floor)
  (func (;9;) (type 1) (param f64) (result f64)
    get_local 0
    f64.trunc)
  (func (;10;) (type 1) (param f64) (result f64)
    get_local 0
    f64.nearest)
  (export "add" (func 0))
  (export "sub" (func 1))
  (export "mul" (func 2))
  (export "div" (func 3))
  (export "sqrt" (func 4))
  (export "min" (func 5))
  (export "max" (func 6))
  (export "ceil" (func 7))
  (export "floor" (func 8))
  (export "trunc" (func 9))
  (export "nearest" (func 10)))
