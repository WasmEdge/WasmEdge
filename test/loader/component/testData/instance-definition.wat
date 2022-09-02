(component
  (component $A)
  (component $B)
  (instance $a (instantiate $A))
  (instance $b (instantiate $B (with "a" (instance $a))))
)
