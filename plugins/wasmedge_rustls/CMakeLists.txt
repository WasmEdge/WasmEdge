if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CARGO_CMD cargo build)
  set(TARGET_DIR "debug")
else()
  set(CARGO_CMD cargo build --release)
  set(TARGET_DIR "release")
endif()

set(RS_SO ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libwasmedge_rustls${CMAKE_SHARED_LIBRARY_SUFFIX})

set(WASMEDGE_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../lib/api)

add_custom_target(wasmedge_rustls ALL
  COMMAND WASMEDGE_LIB_DIR=${WASMEDGE_LIB_DIR} LD_LIBARAY_PATH=${WASMEDGE_LIB_DIR} CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD}
  COMMAND cp ${RS_SO} ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND rm -rf ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS wasmedge_shared
)
