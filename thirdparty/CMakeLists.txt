add_subdirectory(evmc)
add_subdirectory(keccak)

if(BUILD_TESTS)
  add_subdirectory(googletest)
endif()
