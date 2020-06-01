# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleONNC
  onncfunc.cpp
  onncmodule.cpp
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmHostModuleONNC
    PRIVATE
    ${ONNC_WASM_LIBRARY}
  )
endif()

target_include_directories(ssvmHostModuleONNC
  PUBLIC
  ${Boost_INCLUDE_DIR}
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparty
)
