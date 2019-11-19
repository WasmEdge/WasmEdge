add_executable(ssvm-qitc
  main.cpp
)

if(ONNC_WASM_LIBRARY)
  add_definitions(-DONNC_WASM)
  target_link_libraries(ssvm-qitc
    PRIVATE
    ssvmVM
    ${ONNC_WASM_LIBRARY}
  )
else()
  target_link_libraries(ssvm-qitc
    PRIVATE
    ssvmVM
  )
endif()