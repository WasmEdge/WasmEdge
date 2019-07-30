(module
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 16) "\00\00\00\00\01\00\00\00\02\00\00\00\03\00\00\00\04\00\00\00")
 (data (i32.const 48) "\10\00\00\00\11\00\00\00\12\00\00\00\13\00\00\00\14\00\00\00\15\00\00\00")
 (export "memory" (memory $0))
 (export "_Z4funcv" (func $_Z4funcv))
 (func $_Z4funcv (; 0 ;) (result i32)
  (local $0 i32)
  (local $1 i32)
  (local $2 i32)
  (set_local $1
   (i32.const 0)
  )
  (set_local $0
   (i32.const 16)
  )
  (set_local $2
   (i32.const 0)
  )
  (loop $label$0
   (set_local $2
    (i32.add
     (i32.mul
      (i32.load
       (get_local $0)
      )
      (get_local $1)
     )
     (get_local $2)
    )
   )
   (set_local $0
    (i32.add
     (get_local $0)
     (i32.const 4)
    )
   )
   (br_if $label$0
    (i32.ne
     (tee_local $1
      (i32.add
       (get_local $1)
       (i32.const 1)
      )
     )
     (i32.const 5)
    )
   )
  )
  (set_local $1
   (i32.const 0)
  )
  (set_local $0
   (i32.const 48)
  )
  (loop $label$1
   (set_local $2
    (i32.add
     (i32.mul
      (i32.load
       (get_local $0)
      )
      (get_local $1)
     )
     (get_local $2)
    )
   )
   (set_local $0
    (i32.add
     (get_local $0)
     (i32.const 4)
    )
   )
   (br_if $label$1
    (i32.ne
     (tee_local $1
      (i32.add
       (get_local $1)
       (i32.const 1)
      )
     )
     (i32.const 6)
    )
   )
  )
  (get_local $2)
 )
)
