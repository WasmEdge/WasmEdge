(module
  (func (export "br") (block (br 0)))
  (func (export "br_if") (block (br_if 0 (i32.const 1))))
  (func (export "br_table") (block (br_table 0 (i32.const 0))))
)