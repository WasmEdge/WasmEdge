(component
  (import "a" (instance $i))
  (core func $f1 (canon lower (func $i "f1") string-encoding=utf8))

  (core instance $ci)
  (func $f2 (canon lift (core func $ci "f2") string-encoding=latin1+utf16 (memory 0) (realloc 0) (post-return 0)))
)
