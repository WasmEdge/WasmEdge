# SPDX-License-Identifier: Apache-2.0

add_definitions(-DWASMEDGE_COMPILE_LIBRARY)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()

if(WASMEDGE_BUILD_SHARED_LIB)

wasmedge_add_library(wasmedge_c SHARED
  wasmedge.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
)

set_target_properties(wasmedge_c PROPERTIES
  PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
  MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
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

endif()

if(WASMEDGE_BUILD_STATIC_LIB)

wasmedge_add_library(wasmedge_c_static STATIC
  wasmedge.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
)

set_target_properties(wasmedge_c_static PROPERTIES
  PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
)

target_link_libraries(wasmedge_c_static
  PUBLIC
  wasmedgeVM
)

if (WASMEDGE_BUILD_AOT_RUNTIME)
  target_link_libraries(wasmedge_c_static
    PUBLIC
    wasmedgeAOT
  )
endif()

set_target_properties(wasmedge_c_static PROPERTIES OUTPUT_NAME wasmedge_c)

install(TARGETS wasmedge_c_static
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

endif()
