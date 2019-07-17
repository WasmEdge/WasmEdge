(module
  (memory 1)
  (data (i32.const 0) "abcdefghijklmnopqrstuvwxyz")

  (func (export "good1") (param $i i32) (result i32)
    (i32.load8_u offset=0 (get_local $i))  ;; 97 'a'
  )
  (func (export "good2") (param $i i32) (result i32)
    (i32.load8_u offset=1 (get_local $i))  ;; 98 'b'
  )
  (func (export "good3") (param $i i32) (result i32)
    (i32.load8_u offset=2 (get_local $i))  ;; 99 'c'
  )
  (func (export "good4") (param $i i32) (result i32)
    (i32.load8_u offset=25 (get_local $i)) ;; 122 'z'
  )

  (func (export "good5") (param $i i32) (result i32)
    (i32.load16_u offset=0 (get_local $i))          ;; 25185 'ab'
  )
  (func (export "good6") (param $i i32) (result i32)
    (i32.load16_u align=1 (get_local $i))           ;; 25185 'ab'
  )
  (func (export "good7") (param $i i32) (result i32)
    (i32.load16_u offset=1 align=1 (get_local $i))  ;; 25442 'bc'
  )
  (func (export "good8") (param $i i32) (result i32)
    (i32.load16_u offset=2 (get_local $i))          ;; 25699 'cd'
  )
  (func (export "good9") (param $i i32) (result i32)
    (i32.load16_u offset=25 align=1 (get_local $i)) ;; 122 'z\0'
  )

  (func (export "good10") (param $i i32) (result i32)
    (i32.load offset=0 (get_local $i))          ;; 1684234849 'abcd'
  )
  (func (export "good11") (param $i i32) (result i32)
    (i32.load offset=1 align=1 (get_local $i))  ;; 1701077858 'bcde'
  )
  (func (export "good12") (param $i i32) (result i32)
    (i32.load offset=2 align=2 (get_local $i))  ;; 1717920867 'cdef'
  )
  (func (export "good13") (param $i i32) (result i32)
    (i32.load offset=25 align=1 (get_local $i)) ;; 122 'z\0\0\0'
  )

  (func (export "bad") (param $i i32)
    (drop (i32.load offset=4294967295 (get_local $i)))
  )
)