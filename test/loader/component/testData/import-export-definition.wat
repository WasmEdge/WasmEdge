(component
  (import "c" (instance $c
    (export "f" (func (result string)))
  ))
  (import "d" (component $D
    (import "c" (instance $c
      (export "f" (func (result string)))
    ))
    (export "g" (func (result string)))
  ))
)
