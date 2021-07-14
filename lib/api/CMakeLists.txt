# SPDX-License-Identifier: Apache-2.0

add_definitions(-DWASMEDGE_COMPILE_LIBRARY)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()

wasmedge_add_library(wasmedge_c SHARED
  wasmedge.cpp
  wasm_c.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/wasm/wasm.h
)

set_target_properties(wasmedge_c PROPERTIES
  PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
  PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/wasm/wasm.h
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
)

target_include_directories(wasmedge_c
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty
)

target_link_libraries(wasmedge_c
  PUBLIC
  wasmedgeVM
)

if (WASMEDGE_BUILD_AOT_RUNTIME)
  target_link_libraries(wasmedge_c
    PUBLIC
    wasmedgeAOT
  )
endif()

install(TARGETS wasmedge_c
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
