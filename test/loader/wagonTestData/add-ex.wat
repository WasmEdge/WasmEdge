;; a simple module with one exported function "iadd".
;;
;; see https://github.com/WebAssembly/spec/tree/master/interpreter/#s-expression-syntax
;; for a reference about the syntax.
(module
  (type (;0;) (func (param i32 i32) (result i32)))
  (func (;0;) (type 0) (param i32 i32) (result i32)
    get_local 0
    get_local 1
    i32.add)
  (export "iadd" (func 0)))
