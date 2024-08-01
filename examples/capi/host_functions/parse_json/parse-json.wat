(module
  (func $i (import "extern" "func-parse-json") (param externref) (result externref) )
  (func (export "parseJson") (param externref) (result externref)
    local.get 0
    call $i
  )
)
