(component $c1
  (component $c2)
  (instance $i
    (export "f" (func 0))
  )
  (alias export $i "f" (func))
  (alias outer $c1 $c2 (component))
)
