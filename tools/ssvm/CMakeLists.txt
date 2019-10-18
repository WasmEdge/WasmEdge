configure_files(
  ${CMAKE_CURRENT_SOURCE_DIR}/ethereum
  ${CMAKE_CURRENT_BINARY_DIR}/ethereum
  COPYONLY
)

add_executable(ssvm
  main.cpp
)

target_link_libraries(ssvm
  PRIVATE
  ssvmVM
)
